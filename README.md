# Ridge Racer Type 4

[![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white)](https://discord.gg/hbGvPbFguk)

<img src="packaging/icon/ridge-racer-type-4.png" alt="" width="128" align="right">

A native PC port of *Ridge Racer Type 4*, built from the original game code
with [our PsyZ fork](https://github.com/khasinski/ridge-racer-type-4-psyz). It runs
on macOS, Linux and Windows. This is an early alpha.

## Discord

Join the [Discord server](https://discord.gg/hbGvPbFguk) for release updates and bug reports.

## Play

Download the game from [GitHub Releases](https://github.com/khasinski/ridge-racer-type-4-pc/releases).
On Linux, take the `.AppImage` (make it executable and start it) or the
`.tar.gz` with the same executable.
See the [changelog](CHANGELOG.md) for changes.

You need your own **European (PAL) or Japanese CUE/BIN image** of the game
(SCES-01706 or SLPS-01800; the Japanese release runs at 60 Hz with its own
picture, text and saves, see [docs/REGIONS.md](docs/REGIONS.md)). Game data is
not included. Keep the CUE file next to its BIN file.

Start the game and choose the CUE file, or pass it on the command line:

```sh
./ridge-racer-type-4-pc "Ridge Racer Type 4 (Europe).cue"
```

**Keys:** arrows steer, `X` Cross, `Z` Square, `D` Circle, `S` Triangle,
`Q`/`R` L1/R1, `W`/`E` L2/R2, `Enter` Start, `Backspace` Select, `F4`
fullscreen. A gamepad works as player 1, a second one as player 2.

For analog steering choose the controller each pad emulates under
**Escape > Controls**: the original digital pad (default), a DualShock (the
left stick steers), a neGcon (the left stick twists, the right and left
triggers press I and II) or a JogCon (the left stick turns the dial). The
game's own Options then show and calibrate that controller.

**Escape** opens the settings: resolution up to 8x, fullscreen, Classic 4:3
or Widescreen 16:9, PAL or NTSC timing, frame interpolation, geometry
correction and other graphics options, controls, volume and the disc image.
Enhancements are off by default, so the game looks and plays like the
original. **Graphics > Draw distance > Extended (2x)** doubles the course
view depth and draws more scenery, at a higher CPU and GPU cost.

**Escape > Replays** saves the replay of the last race (Time Attack, Grand
Prix, VS Battle, Extra Trial) to a `replays` folder beside the settings
file, and plays a saved one from the title screen (or while one plays).
Replay files require a compatible build and the same PAL/NTSC video timing.
Serial link races are not saved.

### Link battle over the network

The game's two-console link cable mode works over a network: the serial
port's bytes and handshake lines travel over TCP, and the game's own link
protocol runs on top. Both players need the same version of the port and
the same video timing (PAL or NTSC).

1. One player opens **Escape > Link cable**, sets it to **Host** and lets
   the other player know their address. The default port is 47400 (TCP);
   over the internet, forward it on the host's router.
2. The other player sets **Join**, enters the host's address under **Host
   address** (Ctrl+V pastes) and, if it was changed, the port.
3. The status line shows **Connected** with the round trip time. In the
   main menu a new last entry appears once both games are there; when one
   player chooses it, the other follows.

The game's protocol was made for a cable: the race advances one step per
exchange, and the menus wait about 0.1 s for an answer. On a LAN it plays
exactly like the cable. With 40 ms round trip the race runs at half speed,
with 100 ms at about a seventh, and from about 90 ms round trip the menus'
exchanges end in the game's link error. Opening the Escape menu during a
link session pauses this game, so the other player gets the link error.
For scripts, `RR4_LINK=host[:port]` or `RR4_LINK=join:address[:port]` set
the link without the menu.

Settings are saved in `ridge-racer-type-4.toml` in your user configuration
folder, or next to the executable if that file already exists there. The
memory cards are kept as `bu00`/`bu10` folders in the same folder as that
file, wherever the game is started from (cards an earlier version left in
the working folder are copied there once). Each start also writes
`ridge-racer-type-4.log` to that folder; please attach it to bug reports.

## Build

Requires Git, CMake 3.21 or newer and a C11 compiler.

```sh
git clone https://github.com/khasinski/ridge-racer-type-4-pc
cd ridge-racer-type-4-pc
git submodule update --init third_party/psyz
git -C third_party/psyz submodule update --init external/SDL
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
./build/ridge-racer-type-4-pc
```

On macOS, `-DRR4_MACOS_BUNDLE=ON` builds `Ridge Racer Type 4.app`.

For development, testing and releases, see [contributing](CONTRIBUTING.md),
[architecture](docs/ARCHITECTURE.md), [conventions](docs/CONVENTIONS.md) and
[releasing](docs/RELEASING.md); known bugs and debt are tracked in
[technical debt](docs/TECH_DEBT.md). Test and diagnostic options are listed at the
top of `src/port/diagnostics.c`.

This repository contains source only. Disc images and proprietary SDK files
are not distributed.
