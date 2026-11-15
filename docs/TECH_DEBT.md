# Technical debt and known risks

This is the register of known bugs, likely bugs, fidelity gaps and technical
debt in the port and its PsyZ fork. It was compiled on 2026-11-12 from a
read-only audit of `src/main`, `src/overlays`, `include`, `src/port`, `tools`,
CI and `third_party/psyz`, plus the open items left by the DuckStation
comparison work.

Each entry has an ID, a severity and a location, and says how to fix it. When
you fix something, delete its entry in the same commit. When you find a bug,
check whether the same pattern occurs elsewhere and record that too.

Severity:

- **High**: can visibly break the game, lose data or stop it from starting
  for some players.
- **Medium**: wrong behaviour in a narrow case, undefined behaviour that
  happens to work today, or debt that is actively hiding bugs.
- **Low**: cleanup, robustness or a diagnostic-only problem.

Entries marked *(speculative)* are plausible from reading the code but have
not been reproduced.

## 1. Bugs in the game modules

| ID | Sev | Where | Problem | Fix |
| --- | --- | --- | --- | --- |
| G8 | Low *(speculative)* | `src/main/menu/menu13.c:118-124`; `car/carwheel.c:480-489`; `menu/mnlist.c:378-405` | The port drops the PS1 store to address 7 through a null `Dialogue`. Two separate copies of low-RAM bytes 0..11 still return the kernel value 0x27 there. After the Grand Prix intro the console would read 0, so reads of unloaded car slots may differ. | Keep one shared PS1 low-RAM image that the emulated null stores write into. |

## 2. Bugs in the host layer, tooling and PsyZ

| ID | Sev | Where | Problem | Fix |
| --- | --- | --- | --- | --- |
| H20 | Low | `src/port/replay_session.c` | A loaded VS replay (2 cars) matched the recorded one in 2659 of 2663 frames; frames 1242, 1244, 1629 and 1630 differ by 6–23 pixels (a short dark line, likely a car spark in a tunnel). The game state in `__DATA` used by the two cars is identical at frame 1242; the load script loads car slots 2 and 3 that the VS menus never load. Time Attack (2956 frames) and Grand Prix (3133) are bit-identical. *(cause unknown)* | Load only the mode's car slots (as the previews do) and compare the spark rings and VRAM at frame 1242. |

## 3. Fidelity gaps against the console

These are measured against the modified DuckStation (`tools/port/duckstation.py`, built as `tools/port/DUCKSTATION.md` describes).
Game logic and 1x rendering already match, and every one-pad scenario (attract, all menus with and without a save, garage, emblem editor, Time Attack, Extra Trial, the full Grand Prix) enters each mode on DuckStation's field with the same FrameCounter and rand. What remains:

| ID | Where | Gap |
| --- | --- | --- |
| F1 | Versus (2 pads), libpad | The title after the movie skip now starts on DuckStation's field. What remains is libpad timing the port does not model (`ReadPadsOnVsync` stores every port at the blank). On DuckStation libpad stores a port's received bytes when it starts the next transfer: with a pad in port 2, port 1's data is in the buffer in the same vblank and port 2's one vblank later; with port 2 empty, port 1's appears one vblank later (VS movie skip, frames 1499/1500). And its vblank test misses a blank that arrives while a root-counter interrupt (the 120 Hz sequencer timer) is being dispatched after that test, so that vblank has no exchange: every 17 fields in the Versus menus (frames 2624, 2641, 2658), for blanks within about 4k clocks of a non-ticking timer interrupt or inside a sequencer tick. `DSLEAD=1` in the DuckStation script stands in for the one-pad delay; two-pad references need `DSLEAD=0`, and then port 2 presses and pad 1 presses on a skipped vblank reach the game a field later than in the port, so mode 1/57 is entered a field early with FrameCounter one lower and the race seed differs. Modelling the skip needs the sequencer timer's phase (F6); modelling the store order changes every one-pad reference to `DSLEAD=0`. |
| F5 | Memory card | DuckStation shifts its port alternation once in the splash polling loop (around field 727). This has no visible effect. *(speculative)* The BIOS card driver probably misses a vblank the same way libpad does (F1): one arriving while the sequencer timer interrupt is dispatched. |
| F6 | Audio, boot timeline | Sequenced title and menu music starts 343 samples (7.5 ms) early. The sequencer's root counter 2 starts in SsStart (after SoundSystemInit's CdReset), and its phase against the vblanks is set by the boot timeline up to there, which the port runs 12 ms shorter than DuckStation's field position: on DuckStation (fast boot, SCPH-1001) MainLoop starts 242k clocks (0.356 field) after a vblank where the port starts on one; LoadR4Directory's loop over R4.BIN's 950 entries takes 1.42M clocks (42 ms) after the read, which the port does not spend; the first Setloc comes 2.2 ms later and CdReset(1) 2.4 ms longer than in the port. Port SetRCnt lands 5.1 ms after its blank, DuckStation's 18.8 ms (frame 426). Adding the first two moves it to 6.8 ms; the rest is not found, so the phase is not changed yet. Race XA music (about 35 ms early, weak match) was not re-checked. |
| F8 | JogCon | The motor-command echo in the pad packet arrives 1–2 fields earlier than on DuckStation. This changes the force-feedback pattern slightly, but not steering. *(speculative)* It is libpad's store order (F1): the received bytes reach the buffer one vblank after the exchange. |
| F9 | NTSC mode | `RR4_VIDEO=ntsc` is the PAL game at 60 Hz, not the Japanese release (which the port runs from its own disc, `docs/REGIONS.md`). The splash length, display-on frame and the PAL full-screen clear stay PAL by design (`docs/ARCHITECTURE.md`). The PAL release also retuned 34 car records (the first car of each manufacturer: gear-4 speed, top speed, target frames), so NTSC timing races those cars with PAL data. |
| F10 | Frame interpolation | A face that is new in a frame is placed by the motion of the paired vertices next to it, which can shear it: in the attract sweep along a car (frame 2740, 1x and 4x) the rear wing coming into view is drawn as a skewed plate for one field. The same sweep scores 4–6% unexplained pixels, most of it the car's texture moving farther than the metric's reach. Outlines of a wall entering the view (no pose in the frame before) float a pixel or two above its edge (race frame 6842, also in HEAD). Thin lines that remain in the seam scan are thin geometry (poles, gantries) and a light texel column at a near wall's edge (chase frame 3292, also in HEAD). Cuts, level-of-detail switches, near-plane crossings and newly visible faces are handled by holding the previous frame or by snapping to paired neighbours, not by a true midpoint. The midpoint no longer depends on how a frame is split into batches: its geometry is kept for the frame and placed once the frame is complete (a race frame at 8x queues up to about 89,000 vertices, a Time Attack replay frame up to about 165,000; the store holds 262,144). It is still split, with a warning and a `PSYZ_INTERP_STATS` count, when the store or the CLUT cache fills or a VRAM write reaches what it samples or draws over; the 8× all-effects GP run reproduces CLUT exhaustion, with its first split warning at game frame 7558. Earlier forced-small-batch checks used 16 primitives (`PSYZ_BATCH_PRIMITIVES`, 1,500–2,300 a frame) or 4,096 vertices (`PSYZ_BATCH_VERTICES`); they do not establish that the full-settings run stays unsplit. The fixed tables count what they drop (`PSYZ_INTERP_STATS`, a warning the first time): the old pair-ring allocator (`INTERP_PAIRS`) lost 1–20 live pairs a run (most in VS split screen and replays), which then started as new; the identity hash probes 64 slots. The 8× all-effects audit reproduced pair and ghost-history vertex-store warnings. Pair allocation now searches the whole ring and protects queued tags; ghost banks grow to six vertices per pair. Both defects have failing-old/passing-new regressions, and the full GP capture run reaches 46923 retained vertices without pair/store/order overflow. The full-settings TA/replay/retry and VS runs also complete without interpolation table overflows; reset/allocation-failure boundaries and other modes still need broader coverage. Packet address maps now probe to an empty slot and compact stale entries: a randomized churn test retains all 6000 live tags, and full game captures match across two packet-arena layouts (see `STABILITY_AUDIT.md`). The same scripted run now draws the same frames and midpoints every time; before, three runs of one build gave three different sets of midpoints, and a rare frame showed a hairline seam of wrong texels, which is what made 16-primitive batches (`PSYZ_BATCH_PRIMITIVES=16`) look different from normal ones. With 16-primitive batches the attract run at 2x (402 captures) and the race frames checked (3840–3860) are identical to normal batches. The newer independent 60/120 FPS path presents to a hidden 1280×720 GPU target in headless GPU runs and reports actual/generated output rates. Older headless measurements included midpoint drawing but omitted final presentation; they are not directly comparable to this path. |
| F11 | Widescreen | The squeeze also applies to native VRAM. StoreImage/MoveImage read-backs of the display buffers therefore see squeezed pixels, and 1x loses horizontal resolution. The double buffers are found by a VRAM layout rule. Widescreen exists only on SDL_GPU. |
| F12 | Japanese release | Verified against DuckStation for the Time Attack race and the front end up to it; the front end reaches each mode 1 to 6 fields early (a splash frame and the menu loads, `docs/REGIONS.md`). Grand Prix (dialogue, results, epilogue credits), Extra Trial, VS, garage, records, options and card saves are ported from the Japanese executable but not yet compared field for field. |

## 4. Structural debt

| ID | Sev | Where | Debt | Fix |
| --- | --- | --- | --- | --- |
| D3 | Medium | game code | Literal PS1 sizes and strides (for example G12) survive where structs gained pointers. | Grep for hex size literals passed to `PsyqBzero`/`memcpy`/`memset`/`PsyqMemcpy`, and for `+ 0x…` pointer arithmetic. |
| D4 | Medium | `tools/port/sweep_layout.py`, `sweep_layout_allow.txt` | The aliasing and truncation sweep needs the decomp tree and an unpinned libclang, so it does not run in `check.sh` or CI. The 19 allow entries (11 `TRUNC`, the `SharedWorkArena` overrun absorbed from the original game, the `SpotlightConeRingOuter` pad read) are not re-checked. The generator for the "Initial values of the globals this module owns" blocks is no longer in the repo. | Pin the requirements and run the sweep in CI. Restore or document the globals generator. |
| D6 | Low | PsyZ `src/timing_profile.h`, game `include/rr/cpu_time.h` | The costs that decide fields were refit on DuckStation's recompiler without breakpoints (`tools/port/ds_calltime.py`, `[rec]`); these remain breakpoint values (`[call]`), which ran the interpreter: PadUpdate (2.0k or 5.7k on the recompiler depending on its path, charged a fixed 6.2k), IntroScriptStep, UpdateParticles and AddTileGrid2x2 (their own cost without callees, and their callees vary), the interrupt handlers (kernel dispatch, vblank handler, libpad exchange, card ISR), the card events and BIOS directory search, the primitive setters and AddPrim (a few clocks, under the stub's own), and SpuClearReverbWorkArea's chunk cost. The in-race CD audio part of SoundUpdate (about 1.4k of its 9.8k) is not charged. No scenario's fields changed with the refit, so these have not decided a field so far. | Time PadUpdate's branches and the BIOS exception handler (a stub on the RAM copy of the kernel); split the remaining exclusive costs by timing their callees. |
| D8 | Medium | PsyZ history | The fork has 108 non-merge commits (+51.5k/−4.4k lines) since upstream 6d9c573, with Parasite Eve, PE2 and Crash branches mixed in. Timing hooks are woven into the decomp `libsnd`/`libspu`/`libgpu` sources under `#ifdef __psyz`, and the public headers grew (`video.h` +319). Rebasing on upstream is expensive. | Keep RR4 changes in topic commits. Upstream generic pieces (drive model, libcard, controller types) separately. Write down the upstream base. |
| D11 | Low | `include/rr_asm.h:89-101` | 12 of the 13 GTE `.word` string macros are unused. `RR_NOP_DEP*` are empty. The macros pun through `*(u32 *)` on unaligned data. | Delete the unused ones; turn the rest into `static inline` with `memcpy`. |
| D13 | Low | game code | 478 uses of 168 `D_80xxxxxx` names and 44 `unk*` fields; worst in `sound/sndque.c`, `demo/opening.c`, `ui/hudmap2.c`, `menu/mnpulse.c`, `menu/mnrows.c`. | Rename as the code is understood. |
| D14 | Low | `src/overlays/*` | About 50 resident globals are defined in overlay files (`PlayerCars`, `Options`, `RaceRecords`, …) and are deliberately not reset with the overlay. | Move them to `src/main`. |
| D15 | Low | `drawlist.c:74` (~950 lines), `objdraw.c:65` (690), `ovl665_update_controller_options` (547), `gtepoly1.c:840` (473); duplication between overlays (`serial_link/car_selection.c` vs `versus/preview.c` ~200 lines, `time_attack/preview.c` vs `extra_trial/preview.c` ~137, `grand_prix/race.c` vs `extra_trial/race.c` ~110) | Long functions and copied code. The copies exist in the original game; under the porting contract this is a reading aid, not a merge target. | Leave them as they are; split a function only when it is being fixed anyway. |
| D16 | Low | `include/rr/memory.h:14` | `_copy_memcard_patch` is declared but never defined. | Delete it. |
| D18 | Low | `include/rr/runtime_settings.h:22` (`Options.storyBranch3C`) | Named and commented as an "epilogue story branch 0..4", but it is the PAL language: `ModeSelectMenuStep` steps it through 5 values, `LoadDialogueStep` reads a fifth of each dialogue entry by it, `trkld8.c` and the credits pick one of 5×4 team sets, and the Japanese release has none of these (`docs/REGIONS.md`). | Rename it (`language3C`) and fix the comment and the users. |

## 5. Tests, CI and releases

Automated game runs (scans, captures, frame-time and memory runs) go
through `tools/port/run_capped.sh`, headless (`PSYZ_HEADLESS=1` or `gpu`,
`SDL_AUDIODRIVER=dummy`). At most `RR4_RUN_SLOTS` (default 2) capped runs
execute at once machine-wide; the others wait. It kills the game when its
physical footprint, which on macOS includes GPU memory that `ps` and RSS do
not show, passes `RR4_MEM_LIMIT_MB` (default 6000), and `RR4_MEM_LOG`
records the footprint over the run. A GPU memory leak once took runs past
100 GB; if a run is killed, find the leak instead of raising the limit.

| ID | Sev | Where | Debt | Fix |
| --- | --- | --- | --- | --- |
| P2 | Medium | CI | No game-level test runs in CI, because they need a disc (`tools/port/regress.sh` runs them locally against a reference commit). The host layer's parsers, the disc identity check and the startup's handling of images that fail are unit-tested without one (`tests/port/startup_test.c`, `diagnostics_test.c`); the message boxes and the file dialog themselves are not. | Run `regress.sh` before every release; a redistributable test image would let CI boot the game. |
