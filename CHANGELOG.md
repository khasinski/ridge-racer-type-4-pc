# Changelog

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
