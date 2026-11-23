# Changelog

## Unreleased

- **The 1x copy of the VRAM is drawn only when the game reads it.** With a
  scaled picture every batch used to be drawn twice, at the scale and into
  the native 1x VRAM that serves read-backs (the replay blur's copy of the
  previous frame, 24-bit movie frames, palettes, StoreImage). A batch that
  draws only into the shown buffers now keeps its 1x draw queued and draws
  it, in order, only when something reads or overwrites those pixels; race
  frames nobody reads are never drawn at 1x. Captures of the attract mode,
  Time Attack, Grand Prix (with the replay blur switched on), Versus and
  their menus, movies and transitions at 1x, 4x and 6x + antialiasing are
  byte-identical to before (40,380 pictures), and a diagnostic shadow 1x
  VRAM (`PSYZ_NATIVE_VERIFY=1`) matched at every one of ~600,000 checked
  reads. On Metal the scaled target no longer has a second copy for
  semi-transparency (framebuffer fetch does not use it): half the scaled
  VRAM memory (242 MiB less at 11x). The GPU's command buffer rollovers
  were audited for lost or reordered draws (none found); a test now forces
  one before every render pass and compares the pictures.

- **Resolution follows the window; antialiasing and high scales stay
  affordable.** Internal resolution has a new default, Auto (window): the
  game draws at the whole multiple of the PlayStation picture that matches
  the window's drawable height (5x at 1080p, 9x at 4K or a Retina
  MacBook's fullscreen), chosen again after a resize. Whatever is selected,
  the picture drawn is held to about twice the window's pixels (at most
  12x): a saved 8x with antialiasing drew a 16x picture (4096x3840, in a
  16384x8192 scaled VRAM) even for a 1080p window; it now draws 6x there
  and 11x on a 3456x2234 display. Settings files keep their value
  (`internal_resolution = "auto"` is the new default; 1-8 stay valid).
  The GPU's time for a Time Attack race frame on an M5 Max (Metal System
  Trace): 8x + antialiasing 5.5 ms (p99 9.1) -> 3.2 ms (p99 6.3) at
  3456x2234, 5.8 ms (p99 9.0) -> 1.0 ms (p99 2.6) at 1920x1080; Auto
  without antialiasing at 1280x720 0.3 ms. docs/PERFORMANCE.md has the
  measurements.
- Palette loads are batched: a race frame loaded a few dozen palettes
  some hundred times, each a separate GPU copy (a compute dispatch on
  Metal); now a palette is copied once until the VRAM changes and the
  copies go to the GPU together. The frame's copy time went from 1.5 to
  0.4 ms on an M5 Max; captures are pixel-identical.

- Clean-up, no change in play: the race and car modules no longer carry
  the calls of the removed PS1 CPU time model (`RR_CPU_CYCLES`,
  `RR_WORK_CALL`, the course objects' per-segment clock table and their
  wrappers); `include/rr/cpu_work.h`, `cpu_time.h` and `ps1_sizes.h` are
  gone. PsyZ drops its unused DMA controller model (`Psyz_DmaWrite`), its
  sound mixer no longer copies the 512 KB sound RAM through the stack on a
  hot reset, and its log says "SPU mixer initialized". Headless runs of the
  attract mode, Time Attack, Grand Prix and Versus give the same state
  traces, pictures and mixed audio as before.
- Race frame rate: the player's car drives exactly as at 25/30 fps, and
  its motion is smooth at 60 fps. The car's game tick is computed whole on
  the tick's first field at the original rate. That field draws the car
  half way from the previous tick, and the second field draws the tick's
  result. Replaying the same stick inputs at both rates, the car's speed,
  revs, heading, pitch/roll and position match at every tick over three
  NTSC laps. With the closed-loop reference driver,
  Time Attack laps on three courses land within 0.03% of the original
  rate's (two courses identical to the millisecond). `RR4_AUTODRIVE=3`
  (diagnostics) is the reference driver used for these comparisons.
- **60 Hz by default: the game runs at the NTSC original's speed.** Game
  timing (Escape > Graphics) now defaults to NTSC: the European disc runs
  at 60 fields a second with the Japanese/US release's timing, so with Race
  frame rate on a race is 60 game frames a second, and the cars, timers,
  countdown, fades and attract scripts run as in the NTSC original (the
  PAL release ran 5/6 as fast at 50 Hz). At NTSC timing the 34 car records
  the PAL release retuned (gear record 4's speed, top speed, target
  frames; the first car of each maker, docs/REGIONS.md) now take the NTSC
  release's values, so those cars handle as in the NTSC original (earlier
  NTSC timing raced them with the PAL data); the splash turns the display
  on at its second frame as the NTSC release does. PAL stays available
  (Game timing: PAL, or `RR4_VIDEO=pal`). The settings file's
  `video_timing` key, which every earlier file wrote as "pal" (then the
  default), is dropped and replaced by `game_timing`: settings files from
  0.3 and before start at NTSC, and a player who wants PAL chooses it again.
  Replays saved with the earlier NTSC timing (PAL car data) are refused.
  Movies (25 frames a second on the PAL disc) keep their own clock on the
  60 Hz display: each picture is shown at the first field at or after its
  time, two or three fields (2-3-2-3-2, 24.99 fps over the opening movie),
  in step with their sound; the opening movie lasts 130.5 s at either
  timing.
- Fixed: with Race frame rate on, the Grand Prix crashed on the first
  fields of a race on course 7 (a rotating beacon light was drawn as the
  start banner with a missing model). A race now also always starts on a
  whole original frame, as on the console.
- **No more PlayStation timing emulation: fast loads and a responsive
  game.** The CD drive, movies, memory cards, sound transfers and the GPU
  no longer imitate the console's hardware delays, and the game's code no
  longer waits out the console's CPU time. The game reaches the title
  menu (opening movie skipped) in 6.9 s instead of 16.4 s; the 6-second
  black screen between the Namco logo and the opening movie is gone (the
  movie now starts 4.3 s after the first frame instead of 12 s); menu
  loads such as Time Attack's course select take about a second instead of
  four and a half. Saving and loading in Save and Load finish at once.
  Waits the game itself counts (logos, the race's opening banner) keep
  their length. Movies play at their own rate (25 frames a second, 30 on
  the Japanese disc) on the host's clock, in step with their sound, each
  picture for an even two fields (a one-field picture about every 4 s where
  the movie's 25 fps and the display's 49.9 Hz drift apart), and their
  pictures are decoded on the PC as they are needed.
- Headless test runs (`PSYZ_HEADLESS`) use a deterministic field clock:
  time passes only in the game's waits, so a run's fields are the same every
  time. `PSYZ_CD_TRACE=1` logs the drive's commands and deliveries; audio
  underruns are logged as warnings.
- Removed: `PSYZ_CD_TIMING`, `PSYZ_CARD_TIMING`, `PSYZ_SPU_TIMING` (the
  instant behaviour is the only one now), the `RR4_FUNC_MARKS` build and
  `RR4_MARK` diagnostics, and the DuckStation timing-fit tools
  (`ds_compare.py`, `ds_mark.py`, `ds_stall.py`).
- **Races at 50 / 60 frames a second (work in progress).** Graphics >
  Race frame rate (default 60 FPS; `race_field_rate` in the settings file,
  `RR4_RACE_FIELD_RATE`) runs a race's frame every video field instead of
  every second one, with no interpolation. The race's timers, countdown,
  voices, lap timing and HUD keep the original speed (they step once per
  original frame, and button presses between them are kept). The player's
  car (engine, gears, steering, grip, slides and spins) and the chase
  camera now move every field, tuned to drive the same line as at 25/30
  fps for the same inputs; the contacts, suspension and
  effects still step once per original frame and follow. Replays record
  every field; replays saved before keep the original rate. The link race
  and the attract demo stay at the original rate.
- Race frame rate: the start's orbit, the replay and Grand Prix fly-by
  cameras, the course's moving objects (blimps, planes, the helicopter,
  the train), fireworks, light and brake trails, flashing lights,
  scrolling textures and HUD pulses now move every field at the original
  speed; the engine pitch, Doppler and stereo position follow the cars
  every field. Their timers (camera cuts, object keyframes, colour
  cycles, sound fades and the menus' button repeat) keep the original
  timing.
- Race frame rate: the wall, kerb and car-to-car contact, the impact and
  landing shakes, the body's roll, pitch and suspension, the wheels' bumps
  and jumps and the wheels' spin run every field with the original's
  real-time behaviour (`RR4_CAR_FIELD_STEP=0` keeps them at the original
  tick, for comparisons). Fixed: at field rate a wall hit cost the car
  far too little speed (its losses applied on every second field only).
- **Race frame rate: the opponents drive at 50 / 60 Hz.** With Race frame
  rate on, the computer cars' driving (steering, lane changes and
  overtakes, corner turns, speed, gear shifts and engine revs) updates every
  video field with half the original frame's step, keeping the original's
  timing: the same shift points and over a two-lap Grand Prix race the
  same finishing order, with each car's finishing time within 0.45 s
  (about 0.2 %) of the original rate's.
- **Frame interpolation removed.** The 60 / 120 FPS frame rate setting
  guessed in-between pictures from the PlayStation's draw lists, which drew
  visible artifacts and made movies stutter. Every game frame is now
  presented once, at its own field. (Native 60 FPS will come from running the
  game at 60 frames a second, not from in-between pictures.) Settings files
  from 0.3 load as before: their `frame_rate` line is ignored.
- Fixed: with 60 FPS selected, menus, the title screen and other 2D screens
  were shown squashed (too wide, with bars above and below) in both
  Widescreen and Classic 4:3.
- Fixed: with 60 FPS selected, movies stuttered: their frames were shown
  late or skipped while the next one decoded.
- The game no longer waits for the display when the swapchain is full
  while the frame limiter paces it (VSync present mode on a display at about
  the game's rate, e.g. Windows/D3D12); that frame is skipped instead of
  stalling the game's timing.
- `RR4_CAPTURE_DISPLAY=1` (diagnostics) also captures the final presented
  1280x720 image of a hidden run at each `RR4_CAPTURE_AT` field.

## 0.3-alpha

- **Smooth 60 FPS at high resolution** (issue #5). With 60 FPS presentation
  at 4x, widescreen and either draw distance, races no longer drop frames on
  our test Mac (1% low ~50 FPS, was ~27; no frame over 25 ms, was dozens).
  The interpolation's lookup tables use a proper hash, its junction search
  runs on worker threads (`PSYZ_INTERP_THREADS`), and the presentation
  deadlines are checked during long draw-list work, so in-between frames are
  no longer late or skipped. The interpolation tables have twice the room.
- **Extended draw distance no longer changes the game's speed**, and no
  longer crashes late in a Grand Prix race at 4x as 0.2 could.
- `PSYZ_PRESENT_LOG=<file>` writes one line per presented image, to report
  frame pacing (Windows numbers are welcome).
- **Console-accurate timing.** The game now runs on the PlayStation's own
  timeline: CPU time of the race, menus and movies, the CD drive, the memory
  card driver, the pads, the sequencer and XA audio are timed as measured on
  the console (DuckStation in its exact interpreter mode is the reference).
  33 of the 35 compared scenarios — boot, menus, card paths, Time Attack,
  Grand Prix, Versus, Extra Trial, replays and results — make every
  decision on the same video field as the console. Random events seeded
  from timing (such as the race music picked on course 7) now come out as
  on the console.
- Fixed: a Grand Prix could hang when an XA music seek failed; long replays
  stalled after about nine minutes (a 64-bit clock overflow); the course 7
  load could stall for over two minutes; course 7 played a different music
  track than the console; opening a pulled-out memory card could succeed;
  the PocketStation probe and card file creation took the wrong time.
- The DualShock and digital pad follow libpad's real state machine,
  including the second pad's exchange and the DualShock's motors.
- XA music is decoded and resampled exactly as on the console.
- The original's null-pointer reads see the PS1's low memory, as on the
  console, instead of host memory.

## 0.2-alpha

- **Escape > Graphics > Draw distance** offers Original (default) or
  Extended (2x). The extended view draws additional course scenery and
  doubles the renderer's depth range, including mirrors and split screen.
  It costs more CPU and GPU time and is saved in the desktop preferences.
- The application icon now uses [amame-a's contribution](https://github.com/khasinski/ridge-racer-type-4-pc/issues/2),
  with matching macOS, Windows and Linux icons and the Orbitron font notice.

- The **Japanese release** (R4 - Ridge Racer Type 4, SLPS-01800, also the
  JogCon bundle) runs from its own disc: 60 Hz, its 240-line picture,
  Japanese text, R4.BIN layout, card saves (`BISLPSP01800RR4_*`) and record
  passwords. A Time Attack race matches the console (DuckStation) field for
  field; the other modes are ported from its executable but not yet
  compared (docs/REGIONS.md).
- **Escape > Controls** chooses the controller each pad emulates: digital pad
  (default, as before), DualShock (analog left stick steering), neGcon
  (stick twist, analog I/II from the triggers) or JogCon (stick turns the
  dial). The game recognises them as on the console, including its
  controller setup and neGcon calibration screens. Steering linearity and
  gamepad rumble for the DualShock's and JogCon's motors are in the same
  page; dead zone, saturation, linearity and scaling of the steering,
  throttle and brake axes are in the `[input]` section of the preferences
  (DuckStation's neGcon terms).

- **Escape > Replays** saves race replays to files and plays them from the
  title screen. Saved replays retain the race setup, textures and effects;
  files are checked for damage and build/video-timing compatibility.
- **Escape > Link cable** connects the original two-console battle mode over
  TCP, with host and join settings. Network play is experimental.
- PsyZ uses bounded GPU upload buffers, fixes GPU memory growth at high
  resolutions, and makes geometry correction and frame interpolation
  independent of address randomization and primitive batch boundaries.
- Systems without an SDL GPU device can draw at the original resolution
  through the CPU rasterizer; unsupported enhancements are disabled in settings.
- Startup checks the selected disc, recovers from missing or incompatible
  saved paths, and keeps memory cards and logs beside the preferences.
  Windows memory-card formatting and native MSVC builds are supported.
- Opponent initialization clears the complete native car array, including
  fields beyond the original PS1 allocation. Leaving a saved replay no
  longer runs physics against the restored but unloaded title-screen cars.
- Linux packages now include AppImages for x86-64 and ARM64. Release builds
  omit diagnostic hooks and include the source revision and license texts.

### Known limitations

- This remains an alpha. Windows and network play have less gameplay coverage.
- Japanese Time Attack has been compared against DuckStation; the remaining
  Japanese modes have not yet been compared field for field.
- A full VS replay comparison found four frames with differences of 6–23
  pixels. Time Attack and Grand Prix replay comparisons were pixel-identical.
- Automated wall-clock CD streaming can stall during the opening movie;
  network timing tests currently require an instant-CD diagnostic workaround.
  Interactive behavior on that path has not been verified.
- Frame interpolation can still produce artifacts on newly visible geometry;
  enhancements remain off by default. US discs and CHD images are unsupported.

## 0.1

First public build of the native port. It is an early alpha: expect bugs.

- The whole original game — resident program and every mode module (Grand
  Prix, Time Attack, VS battle, Extra Trial, garage and emblem editor,
  records, options, movie playback) — runs natively on 64-bit macOS, Linux
  and Windows on our PsyZ fork, from your own **European (PAL) CUE/BIN** disc
  image of *Ridge Racer Type 4* (`SCES-017.06`). Game data is not included.
- Every mode, the full Grand Prix season with its ending and unlocks, races,
  replays, VS split screen, the memory card screens and the attract demo have
  been checked against DuckStation reference runs. At 1x internal resolution
  frames are pixel-identical to the PS1 GPU; disc loads, the opening movie,
  memory card operations and the game's frame timing follow the console.
- Sequenced music, sound effects, CD audio and movie audio play through the
  emulated SPU.
- Memory cards are kept as `bu00`/`bu10` folders (in the working directory,
  or beside the preferences when the macOS app is started from Finder);
  saves are interchangeable with emulator memory cards.
- **Escape** opens a settings menu:
  - Graphics: internal resolution (1x to 8x), fullscreen and a Fullscreen
    50 Hz display mode, VSync, aspect ratio (Classic 4:3 or Widescreen 16:9
    for races, menus stay 4:3), dithering (off gives full colour),
    antialiasing, texture filtering, geometry correction (perspective-correct
    textures, no wobble or road gaps), frame interpolation (in-between frames
    in races) and video timing (PAL 25 fps races, or NTSC 30 fps races with
    the NTSC release's timings; applies on the next start).
  - Keyboard and gamepad bindings, master volume and the disc image.
  - Settings are saved to `ridge-racer-type-4.toml` (portable when that file
    sits beside the executable or the app).
- Without a disc argument the game starts the image chosen last time, or asks
  for the CUE file with a file dialog.
- Release packages: a signed and notarized macOS app (Apple Silicon), Linux
  x86-64 and ARM64 builds and an experimental Windows x86-64 build.

### Known limitations

- Serial link (two-console) play is not supported.
- Enhancements are off by default. Frame interpolation can still show small
  seams or holes in in-between frames (mostly in replays); turn it off for
  the original picture.
- Only the European (PAL) release is supported; other regions and CHD images
  are not.
- Windows is experimental and has had less testing than macOS and Linux.
