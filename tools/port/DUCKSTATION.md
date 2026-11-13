# DuckStation reference build

Every "matches the console" result in `docs/` is measured against a
patched `duckstation-regtest` driven by `tools/port/duckstation.py`. The
patch is `duckstation-regtest.patch`; it applies to upstream DuckStation
commit `2c527f4f2c689f3db06246a2c9c566fbc2323dd8`
(<https://github.com/stenzek/duckstation>, "Qt: Add reset buttons for
tolerance/depth threshold", 2026-06-13). Other commits change emulation and
are not the reference.

## Build (macOS)

    git clone https://github.com/stenzek/duckstation.git ds && cd ds
    git checkout 2c527f4f2c689f3db06246a2c9c566fbc2323dd8
    git apply --binary /path/to/tools/port/duckstation-regtest.patch
    # prebuilt dependencies from https://github.com/duckstation/dependencies
    # extracted to dep/prebuilt/macos-universal
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_REGTEST=ON \
        -DBUILD_TESTS=OFF -DENABLE_OPENGL=OFF -DENABLE_VULKAN=OFF
    make -C build -j8 duckstation-regtest
    touch build/bin/portable.txt
    mkdir build/bin/bios && cp SCPH1001.BIN build/bin/bios/

The patch carries `src/duckstation-regtest/metal_shaders.metallib`, the
compiled `src/util/metal_shaders.metal`, because the Xcode command line
tools have no Metal compiler; the build copies it next to the binary.

The BIOS of the reference runs is SCPH-1001 (v2.2 12-04-95 A, MD5
`924e392ed05558ffdb115408c263dccf`), the only one in `bios/`. DuckStation
warns that it is not a PAL BIOS and runs the PAL console with it. Regtest
patches the BIOS for fast boot unless `-slowboot` is given.

A build made this way from the patch gave the same per-field trace
(mode, FrameCounter, rand) and the same system tick count over 1500 idle
fields as the original reference build.

Point the scripts at it:

    export RR4_DUCKSTATION=$PWD/build/bin/duckstation-regtest
    export RR4_DISC=".../Ridge Racer Type 4 (Europe) (En,Fr,De,Es,It).cue"

Run it through `tools/port/run_capped.sh` like the game. `duckstation.py`
picks a free port pair in 47300-47999 unless `RR4_DS_PORTS=ai,gdb` (or
`ai=`/`gdb=`) names one, and stops the emulator on `close()`, at the end
of a `with` block, or when it cannot connect.

## What the patch adds

Command line:

| Option | Effect |
| --- | --- |
| `-ai-port <port>` | text control server on 127.0.0.1 (verbs below) |
| `-gdb <port>` | GDB remote stub (RAM reads `m`, writes `M`, breakpoints `Z0`) |
| `-start-paused` | pause right after boot |
| `-wait-for-debugger` | stay paused until a GDB client connects |
| `-slowboot` | no BIOS fast-boot patch (full BIOS intro) |

Control verbs (one line each, answered `OK ...` or `ERR ...`):

| Verb | Effect |
| --- | --- |
| `status` | paused, running, frames and advance frames remaining |
| `pause`, `resume` | |
| `advance <n>` | run n frames (vblanks), then pause |
| `buttons[2] Name=v ...` | set digital pad buttons on port 1 (2) |
| `bind[2] index=v ...` | set a controller's bind by index (buttons, then half axes) |
| `release[2]` | clear every bind on port 1 (2) |
| `screenshot` | `SCREENSHOT w h pitch size` followed by RGBA8 bytes |
| `vram <path>`, `spuram <path>` | write raw VRAM (1024x512x16) or SPU RAM |
| `audiodump [path]` | append the mixed SPU output (44100 Hz s16 stereo); no path closes it |
| `disc <path>` | change the disc |
| `ticks` | system tick counter (33.8688 MHz) and frame number |

Environment:

| Variable | Effect |
| --- | --- |
| `REGTEST_PAD1_TYPE`, `REGTEST_PAD2_TYPE` | controller type name (`DigitalController` default on port 1, `AnalogController`, `NeGcon`, `JogCon`, ...) |
| `REGTEST_PAD2=1` | digital pad in port 2 (default empty) |
| `REGTEST_CARD1` | `none`: no card in slot 1; a path: that card file (shared); unset: a blank non-persistent card |
| `REGTEST_VRAM_OUT` | write the final VRAM at exit |
| `DUCKSTATION_SPU_REG_TRACE=1` | log CPU writes to the SPU voice registers with the frame number |
| `DUCKSTATION_SPU_KEY_TRACE=1` | log voice key-on/off |
| `DUCKSTATION_ADPCM_DUMP`, `DUCKSTATION_VOICE18_DUMP` | dump voice 18's decoded blocks / output samples (XA debugging) |

Port 1 is a digital pad (upstream regtest uses an analog one), port 2 and
card slot 2 are empty, audio goes to the null backend and the renderer is
software.

## Timing a call

A GDB breakpoint, and single-stepping, switch DuckStation from its
recompiler to its interpreter, whose clocks differ per routine by -45% to
+20%; the reference runs use the recompiler. `ds_calltime.py` measures
calls there without breakpoints: it writes a timing stub into kernel RAM
the game leaves unused and points each function at it, then prints the
clocks of each call (`ds_calltime.py 2000 2200 PadUpdate PsyqBzero:tail:arg=a1`,
through `run_capped.sh`; its docstring has the options). `DUMMYBP=1` sets a
never-hit breakpoint, for the same run on the interpreter.
