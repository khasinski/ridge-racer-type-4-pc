# Third-Party Notices

The Ridge Racer Type 4 PC executable links the components below. Their
licenses apply to those components only; they do not relicense the original
game, whose code, data and trademarks belong to Bandai Namco Entertainment.
Game data, disc images and Sony PsyQ SDK materials are not distributed.

Release packages carry the full license texts in `licenses/` (on macOS in
`Ridge Racer Type 4.app/Contents/Resources/licenses/`), at the same relative
paths as in the source tree.

## PsyZ

The game runs on our fork of [PsyZ](https://github.com/khasinski/ridge-racer-type-4-psyz), pinned as the `third_party/psyz` submodule, which provides the
matching source. Per its top-level `LICENSE`:

- `psyz/src/platform/` and `psyz/src/psyz/` (runtime and SDL3 platform layer)
  are under the **Mozilla Public License 2.0**
  (`psyz/src/platform/LICENSE`, `psyz/src/psyz/LICENSE`).
- `decomp/` is under the **MIT License**, Copyright (c) 2024 github.com/psyqz
  members and Copyright (c) 2025 github.com/xeeynamo/sotn-decomp contributors
  (`decomp/LICENSE`).
- `psyz/include` and `psyz/src/psyq` are listed as unlicensed.

## SDL 3

[SDL](https://libsdl.org/) 3.4 (`external/SDL` in PsyZ), statically linked,
Copyright (C) 1997-2026 Sam Lantinga, under the **zlib License**
(`external/SDL/LICENSE.txt`). Bundled components with their own notices:

- HIDAPI (`src/hidapi/`), used under its **BSD-style license**
  (`LICENSE.txt`, `LICENSE-bsd.txt`).
- yuv2rgb (`src/video/yuv2rgb/`), Copyright (c) 2016 Adrien Descamps,
  **BSD 3-Clause License** (`LICENSE`).

## libwebp

[libwebp](https://chromium.googlesource.com/webm/libwebp) 1.6.0 (the WebP
decoder), fetched and statically linked by PsyZ's CMake build, Copyright (c)
2010 Google Inc., under the **BSD 3-Clause License** with Google's additional
patent grant (`licenses/libwebp/COPYING`, `licenses/libwebp/PATENTS`).

## IBM Plex Sans

The Escape settings menu embeds a subset of IBM Plex Sans Regular
(`src/port/fonts/ibm_plex_sans.ttf.inc`), Copyright (c) 2017 IBM Corp. with
Reserved Font Name "Plex", under the **SIL Open Font License 1.1**
(`src/port/fonts/OFL-IBMPlex.txt`).

## stb_truetype

`src/port/third_party/stb_truetype.h` (v1.26) by Sean Barrett,
<https://github.com/nothings/stb>, rasterizes the settings menu font. It is
dual-licensed as public domain or under the MIT License (see the end of the
file).

## Application icon

The application icon (`packaging/icon/`) is original artwork made for this
port; it contains no game artwork or logos.
