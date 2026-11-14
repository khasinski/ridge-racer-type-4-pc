# Changelog

## Unreleased

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
