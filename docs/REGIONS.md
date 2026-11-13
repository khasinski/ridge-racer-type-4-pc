# Other releases (NTSC-J, NTSC-U)

The port runs the PAL release (SCES-01706, `SCES_017.06`). This page
records how the other releases differ, measured on the disc images, and the
state of the Japanese release's support (below, "Japanese release:
status"). The disc check still accepts only the PAL boot file; diagnostic
builds run the Japanese disc with `RR4_UNVERIFIED_RELEASE=1`.

## Images examined

| Release | Image | Boot file | Result |
| --- | --- | --- | --- |
| PAL (Europe) | Redump `Ridge Racer Type 4 (Europe) (En,Fr,De,Es,It)` | `SCES_017.06` | supported |
| NTSC-J | Redump `R4 - Ridge Racer Type 4 (Japan, Asia)` | `SLPS_018.00` | compared below |
| NTSC-J JogCon bundle | Redump `R4 - Ridge Racer Type 4 (Japan, Asia) (R4 + JogCon)` | `SLPS_018.00` | the same disc: only the two ISO directory sectors (LBA 22–23, file timestamps) differ |
| NTSC-U (SCUS-94403) | none available | `SCUS_944.03` (expected) | not examined |
| Bonus discs (JP *Ridge Racer Turbo* / PAL *Hi-Spec Demo*) | not R4 | | separate programs with their own executable; out of scope |

The Japanese disc boots `SLPS_018.00` (SLPS-01800), not SLPS-01870/01871.
Without an NTSC-U image, nothing can be said about that release. It is
probably a third variant (NTSC timing and picture, English text), and it
needs the same comparison before any work is planned.

## How to reproduce

```
tools/port/region_diff.py PAL.cue OTHER.cue [--section boot|exe|ovl|r4|str]
```

The tool reads both images directly. It matches every PAL function from
the decomp (`configs/PAL/functions.txt`, `configs/PAL/overlays/*.json`)
against the other executable and overlay images, then compares the R4.BIN
directories and the R4.STR streams. All the numbers below come from it.

## What differs (PAL vs NTSC-J)

The PAL executable is the Japanese one, rebuilt. The function order and
library are the same, and each function sits between 612 bytes before and
1840 bytes after the place of its JP counterpart.
Beyond the 25 fps conversions (already reproduced, see below), the PAL
release changed the picture height, added languages and DualShock steering,
re-encoded the movies and retuned some cars. It also re-laid out R4.BIN.
**The two releases differ substantively in code, not only in data.**

### Main executable

| | count |
| --- | --- |
| PAL functions | 1671 |
| identical bytes | 306 |
| only branch/call targets and data addresses moved | 1240 |
| only immediate constants changed | 60 |
| instruction sequence changed | 65 |

The "moved only" class hides immediates that are not plain `li`/compare
operands, so the two changed classes are lower bounds. Some of the
structural changes are register allocation after a recompile (for example,
`UpdatePlayerSteering` swaps `s1`/`s2` throughout). Most are real changes.
By kind:

- **25 fps conversion** (frame counts ×5/6, `/25` instead of `/30`, RPM
  smoothing). Examples: `FramesToMilliseconds`, `FormatFrameTimeMMSS`,
  `DrawStartCountdown`, `PlayStartCountdownVoice`, `UpdateLapTiming`,
  `AttractDemoRaceUpdate`, `StepCarShowcaseScene`, `UpdateRaceOpeningOverlay`,
  `IntroScriptStep`, `CheckRaceFinish`, the race overlays. These are already
  handled behind `RR_PAL_NTSC` (`include/rr/video_timing.h`). The JP values
  are in the code.
- **256-line picture** (JP 240/480). Examples: `SetupRenderer` and
  `SetupDrawDispEnvs` (display/draw environments), the screen-bound culling
  in every renderer (`EmitRro*`, `EmitProjectedLitTrisGT3`, `SkyEmit*`,
  `DrawTrackObject*`, `EmitModelTrisGT3`: `slti 256/512` → `240/480`), HUD
  layout (`InitHudNumberTargets`, `DrawRaceMinimap`, `DrawSteeringIndicator`,
  `DrawSplitScreenDivider`, `DrawRaceOverlaySprites`, `EmitIconSpriteRow128`:
  y −8/−16), full-screen fades and backdrops (`UpdateScreenFade`,
  `AddScreenFadeTiles`, `AddScreenBackdropTile`, `DrawUiRow`, `DrawBadgeRow`,
  `UpdateMenuScript`: 512 → 480), and the link screens. None of these is
  handled: NTSC timing keeps the PAL picture (`Psyz_GpuSetNtscPalPicture`).
- **Languages** (PAL only). The field the port calls `Options.storyBranch3C`
  (`include/rr/runtime_settings.h`, "epilogue story branch 0..4") is the
  PAL language. `ModeSelectMenuStep` steps it through 5 values. Uses:
  `LoadDialogueStep` reads a fifth of each Grand Prix dialogue entry
  (`CdFileReadRange(..., s1 * lang, s1, ...)`) where JP reads whole entries;
  `LoadIntroSceneStep` picks one of 5×4 team image sets (JP 4);
  `DrawBadgeCountdownSprites` draws `lang % 5`; credit scroll speeds; the
  record password encodes it. Functions involved: `TitleMenuInit`,
  `TitleMenuUpdate`, `DrawModeSelectRows`, `ModeSelectMenuStep`,
  `DialogueModeUpdate`, `UpdateDialogue`, `LoadDialogueStep`,
  `LoadMenuCarModelsStep`, `InitBadgeRow`, `DrawBadgeTitleBar`,
  `InitDialogueState`.
- **Text drawing**. `DrawGlyphList`, `DrawGlyphListSloped`,
  `CenterGlyphListX`, `DrawAnimatedGlyph` and `DrawMenuFrame` differ
  structurally (more than 100 instructions in the glyph list drawers).
  `LoadGlyphRowsStep` loads group-1 entry 96 in JP (PAL 64). The data
  tables next to `HudGlyphUvRects` (0x800A1158) differ.
- **Input**. PAL adds a fourth steering class, `0x80`: a standard pad
  reporting type `0x73` (DualShock analog), with the stick as steering
  (`ReadCarInput`, `UpdatePlayerSteering`). JP has only `0x20` digital,
  `0x40` NeGcon and `0x60` wheel/JogCon. `PadUpdate` (about 160
  instructions), `PadPollPort`, `EnablePadAnalogMode` and the options
  overlay's controller pages differ with it.
- **Race logic**. `StepRaceFrame` and `ResolveCarCollisions` test
  `GameModeType` in JP (`== 4`, and `- 2 < 3`) where PAL tests a flag it
  computed earlier in the function. The effect on each mode has not been
  worked out.
- **Memory card and passwords**. The save file names are `BESCESP01706RR4…`
  vs `BISLPSP01800RR4…` (`RR4_GAME`, `RR4_PDA`, `RR4MARK1..3`).
  `BuildCardHeaderStatic` (title and icon header), `CardSaveStep`,
  `BuildSaveData`, `ApplySaveData` and `DrawSaveSlotTableHeader` differ.
  Saves do not carry over between releases. `EncodePasswordCode`,
  `Encode/Decode/ValidateRecordPassword` differ (0x4D8 bytes of PAL code
  against 0x428 in JP), so record passwords are incompatible.
- **Other changed values that are not 5/6 conversions**, such as idle
  timeouts: `ModeSelectMenuUpdate` and `NewCarModeUpdate` use 501 frames
  in PAL and 301 in JP, `InitDemoFadeContext` 206/104, `ResetDemoFadeState`
  140/64, `DialogueIntroModeUpdate` 108/88. Also `SplashLogoUpdate` and
  `FrmTaskModeUpdate`/`MainLoop`/`TileRectSceneModeUpdate` (boot and splash
  sequence).
- **Initialized data**: 581 of 14 395 words after the code differ. The
  biggest blocks are glyph UV and menu row script tables (JP has eight more
  10-word records at 0x800A0F4C).

### Overlays

The overlays sit at the same R4.BIN slots (659..668, copies 669..672) with
the same sizes and module IDs. They are linked for 0x801146F0 in JP and
0x80114878 in PAL.

| Overlay | functions | constants only | structural |
| --- | --- | --- | --- |
| grand_prix | 7 | 1 (timing) | 0 |
| time_attack | 14 | 2 (timing) | 1 (`ovl660_update_results`) |
| versus | 13 | 2 (timing) | 0 |
| garage | 156 | 1 (256→240) | 2 (`format_record_time`, `emit_draw_area`) |
| records | 14 | 0 | 0 |
| options | 17 | 0 | 6 (screen adjust, controller options) |
| extra_trial | 13 | 2 (timing) | 0 |
| serial_link | 27 | 2 (timing, 256→240) | 0 |
| movie_playback | 40 | 1 (512/256→480/240) | 5 (epilogue credits, letterbox) |

### R4.BIN

The PAL R4.BIN has 950 entries and the JP one 969. `GetR4FileIndex` group
bases:

| group | PAL base | JP base | PAL entries | JP entries | byte-identical at the same slot |
| --- | --- | --- | --- | --- | --- |
| course/car (`0..`) | 0 | 0 | 658 | 658 | 583 |
| 1 (scenes, menus, overlays) | 0x292 | 0x292 | 71 | 106 | 1 (plus 14 found at other slots) |
| 2 (cars in menus, sound, tracks) | 0x2D9 | 0x2FC | 171 | 155 | 147 |
| 3 | 0x384 | 0x397 | 19 | 19 | 19 |
| 4 | 0x397 | 0x3AA | 9 | 9 | 9 |
| 0 | 0x3A0 | 0x3B3 | 22 | 22 | 21 |

- Group 1 is laid out differently, not just shifted. Each PAL Grand Prix
  dialogue entry (100 sectors = 5 languages × 20) is a pair of entries in JP
  (about 4–7 + 16–23 sectors). The constant group-1 indices in the loaders
  change: `LoadGlyphRowsStep` 64 → 96, `LoadTrackNodesStep` `+56` → `+88`.
  `LoadDialogueStep`, `LoadMenuCarModelsStep` and `LoadIntroSceneStep`
  change in structure. Entries 0–16 (menu art with text) differ throughout.
- Group 2 ends with PAL's 20 per-language team image sets (entries
  0x97..0xAA). JP has 4.
- Course entries 0–17 differ in every course. Image placement words differ
  (for example, entry 16 has PAL x=0x3C0, y=0 where JP has x=0xC0,
  y=0x1F0..0x1F1): JP places course textures and CLUTs in VRAM lines
  480–511, which the PAL 2×256-line framebuffers occupy. JP course data is
  therefore unusable with the PAL display layout, and the reverse.
- Car entries: 34 of the 640 car records (`CarRecord`, entry +0x18) differ.
  These are the first car (`a2 = 0`) of each manufacturer block (PROMESSA,
  BONFIRE, AMBITIOUS, PROPHETIE) in both car groups. PAL raises
  `gears04[4].speed02` by 15 (for example, 254 → 269), raises
  `topSpeed1F0` (166 → 175) and lowers `targetFrames1E8` (4635/6060 →
  4475/5750). Some other car entries (`a2 = 19`) differ in texture pixels.
  So for those cars the car data is not the same in both releases, and NTSC
  timing races them with the PAL values.

### Movies (R4.STR) and audio

- Video is re-encoded. JP has two streams at 320×160, 5 sectors a frame
  (30 fps at 2×): 3937 and 7590 frames. PAL has the same two at 6 sectors a
  frame (25 fps): 3280 frames at 320×176 and 6325 at 320×160. The durations
  are equal. The letterbox and credits code in `movie_playback` follows the
  frame sizes.
- XA audio: the same channel layout and sector counts (channel 1 has 6 more
  sectors in JP). Groups 3, 4 and 0 (sound banks and the rest) are
  byte-identical apart from one group-0 entry. `FILE.DAT` is identical.

## Japanese release: status

The release profile (`include/rr/release.h`, `src/main/system/release.c`)
is selected from the mounted disc's `SYSTEM.CNF` (`Rr4DiscImageBoot`,
`src/port/main.c`): `GameRelease`, `RR_RELEASE_JP` and `RR_PAL_JP(pal, jp)`
choose the Japanese release's code and data next to the PAL code, in the
original modules. The Japanese release implies `VideoTimingNtsc` (its frame
rate values are those behind `RR_PAL_NTSC`) and shows its own 240-line
picture (`Psyz_GpuSetNtscPalPicture` stays off). `tests/port/release_test.c`
covers the profiles. With the PAL disc every value is the original one; the
regression scenarios (`tools/port/regress.sh`: attract, TA, GP, VS) are
identical to the commit before each change.

The Japanese executable is compared and decompiled with the decomp's
toolchain (spimdisasm + m2c on `SLPS_018.00` and the JP overlay images).

### Ported

- **R4.BIN map**: the directory table holds the Japanese release's 969
  entries (`RR_R4_ENTRIES_MAX`; 950 overran into the next globals);
  `GetR4FileIndex` group bases per release; glyph rows (96),
  track nodes (+88), the menu dialogue image and table per scene
  (100..105), Grand Prix dialogue as whole entries (17 a team from 20) with
  image lists, team image sets without languages.
- **240-line picture**: display/draw environments (no PAL screen height or
  interlace override), `SetupRenderer`, viewports and capture rectangles,
  culling bounds of every renderer (RRO, lit triangles, sky, track objects),
  HUD and menu positions, full-screen tiles and fades (one 320x480 tile for
  the PAL 2x2 grid), split-screen HUD, loading triangle, movie frame
  placement.
- **Boot and front end**: splash (no PAL clear, display on at the second
  frame), `SetVideoMode` in `SystemInit`, intro script and the two-part
  title logo, title and mode select without languages, menu frame (seven
  columns), dialogue (64 animated glyphs, continue mark, no Start skip),
  frame task (eight sprites), tile-rect scene (ten sprites), options
  (screen adjust range, menu image, row 2, controller pages without the
  analog-mode wait).
- **Text**: glyph lists without extended glyphs, 12-pixel animated glyphs (21
  a row), the glyph/badge/font UV tables, race opening overlay (its own
  glyph lists and element timing), menu text template 7.
- **Data tables** that differ: preset sprites, HUD presets, menu state
  defaults, trophy map, CD XA track table, card titles (Shift JIS) and
  icons, viewport and capture rectangles.
- **Saves and passwords**: card files `BISLPSP01800RR4_GAME` etc., animated
  three-frame card icon, the save without the PAL language word (checksum of
  0x6A8 words at 0x1AA0, converted to the PAL layout in `SaveBuffer`),
  five-byte record passwords.
- **Race logic and input**: opponent projections and collision knock-back by
  `GameModeType`, ranking without the PAL tie rule, split latch, no
  DualShock steering class (an analog pad steers with the d-pad), actuators
  set up at pad state 2, TA results password line.
- **Movie**: streams of 5 sectors a frame (the frame limit; the PAL release
  has 6), epilogue credits as twelve sprite pages, letterbox passes.

### Verified against DuckStation (JP disc, ports 47500/47501)

- **Time Attack race**: from the race's entry (rand set to DuckStation's at
  that field, as for the PAL comparisons) FrameCounter, RaceState, the player
  car's distance and rand are identical on every field for 2987 fields
  (about 1490 frames, accelerating with the digital pad into the walls of
  the first course). The race load (1/36 to 1/53) takes 636 fields on both.
- **Front end**: splash, opening movie, title, mode select, the Time Attack
  car and course menus reach every mode in the same order. VRAM outside the
  display (all textures, CLUTs and fonts the Japanese release loads) is
  identical to DuckStation's on the title menu; the menu picture is
  identical apart from its animated streaks and cursor, which are a few
  fields out of phase (below).
- **Mode entries**: the port reaches each mode 1 to 6 fields earlier than
  DuckStation after the splash: one splash frame takes three fields one
  frame earlier on DuckStation than in the port (290 against 291 splash
  fields), the movie skip and the title are 1 and 2 fields early, and the
  menus add one each. The PAL release matches DuckStation here; the
  difference is in the time of the Japanese release's loads (other files
  and sizes) and is not found yet.

### Remaining

- Field timing of the front end (above); the port's boot reaches the
  splash 286 fields before DuckStation does (BIOS and executable load), as
  for PAL.
- Verification of each mode (GP, Extra Trial, VS, garage, records, options,
  replay, card save/load) field for field, then enabling `SLPS_018.00` in
  `src/port/disc_check.c`.
- JP course textures in VRAM lines 480..511 (with the 240-line buffers they
  no longer collide; not yet checked visually in every course).

## What exists today

`RR4_VIDEO=ntsc` (Escape menu > Graphics > Video timing) runs the **PAL
disc** with the JP release's frame-rate values at 59.82 Hz
(`docs/ARCHITECTURE.md`, TECH_DEBT F9). This mode does not support the
Japanese disc. It keeps the PAL picture height, text, languages, R4.BIN
layout, DualShock steering, PAL car records and saves.

## Effort and recommended approach

Support for the Japanese disc on the PAL code path would generalize the
video-timing switch into a release profile:

1. **Detection.** Read the boot file from `SYSTEM.CNF` (`SCES_017.06` → PAL,
   `SLPS_018.00` → NTSC-J) once at mount. The JP profile implies NTSC
   timing. Extend the disc whitelist only once the JP profile is verified.
2. **R4.BIN index map.** Region bases in `GetR4FileIndex`, plus every
   constant group-1 index in the resident loaders and the overlays
   (`LoadGlyphRowsStep`, `LoadTrackNodesStep`, dialogue, intro and menu car
   steps). This needs a complete enumeration of `GetR4FileIndex` calls in
   the JP executable and overlays.
3. **240-line display.** Draw/display environments, the culling bounds of
   every renderer, HUD and menu layout, fades, movie letterbox. This is
   needed anyway, because JP course data puts textures in lines 480–511.
4. **Release logic.** At every structurally different site, port the JP
   logic from the JP executable next to the PAL code under the profile
   (`if (RegionNtscJ) ... else ...`), in the original module (porting
   contract): language-free dialogue/intro loading, glyph drawing, card
   header and save names, passwords, the boot/splash sequence, JP
   initialized data tables, and the JP options/credits overlay code. The
   PAL-only DualShock steering class can stay as a port option, off by
   default for JP fidelity.
5. **Verification.** Each mode (boot, splash, opening movie, title, menus,
   Grand Prix, Time Attack, Extra Trial, VS, garage, records, options,
   replay, card save/load) against DuckStation on the JP disc, field for
   field, as for PAL.

Size: about 125 main-executable functions and 15 overlay functions with
release-specific logic, about 60 of them beyond the constants already
behind `RR_PAL_NTSC`, plus data tables and the index map. The PAL timing
work changed about 50 functions in 47 files. The remainder is 2–3 times
that, and most of it is structural rather than constant swaps. Estimate:
2–4 weeks of focused work, dominated by DuckStation verification of every
mode. NTSC-U cannot be estimated without an image. If it is a third
executable, it needs the same comparison and a third profile.

The alternative of decompiling the JP executable as a second game is ruled
out by the porting contract (no parallel engine).

The disc check stays PAL-only until a profile is verified. When it refuses
a disc, it can name the release: `SLPS_018.00` is the Japanese release and
`SCUS_944.03` the USA release.
