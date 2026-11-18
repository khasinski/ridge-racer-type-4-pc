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
| `DS_LOWRAM_LOG=<file>` | with any breakpoint set (the interpreter's checked path; `ds_script.py` with `DUMMYBP=1`), append each new code address/data address pair where game code, or the kernel called from it, reads or writes PS1 RAM 0..0xFFFF: the original's null-pointer accesses. `ds_lowram_report.py <file>` groups them by function |
| `DUCKSTATION_SPU_REG_TRACE=1` | log CPU writes to the SPU voice registers with the frame number |
| `DUCKSTATION_SPU_KEY_TRACE=1` | log voice key-on (with the system tick) and key-off |
| `DUCKSTATION_ADPCM_DUMP`, `DUCKSTATION_VOICE18_DUMP` | dump voice 18's decoded blocks / output samples (XA debugging) |
| `DUCKSTATION_CD_TRACE=1` | log each CD command as it executes (`CDCMD`) and completes (`CDDONE`, a second response), each seek-time draw of the drive's generator (`CDSEEK n ... from to jitter`, absolute sectors), each vblank start (`VBLANK`) and each root counter 2 write (`RCNT2`), with the system tick and frame; compare with the port's `PSYZ_CD_TRACE` (its `cd seek draw n` lines count the same draws) |

Port 1 is a digital pad (upstream regtest uses an analog one), port 2 and
card slot 2 are empty, audio goes to the null backend and the renderer is
software.

## Replaying a scenario

`ds_script.py frames 'pad script' [out]` plays an `RR4_PAD_SCRIPT` (the
regression scenarios' button steps, mode anchors included) on DuckStation
and prints the port's `RR4_TRACE=1 RR4_TRACE_RAND=1` lines (vsync,
FrameCounter, mode, rand) for each field, so the two traces diff directly.
Fields count from the port's first VSync (DuckStation frame - 225, the
BIOS boot). The controller sends a step's buttons at the exchange of the
blank before it, as the port's script does, so with one pad the receive
buffer shows them over the step's fields on both. `RR4_DS_PADLOG=1` adds
whether libpad's vblank handler ran, `RR4_DS_PEEK=addr:len,...` RAM bytes.

## Timing a call

A GDB breakpoint, and single-stepping, switch DuckStation from its
recompiler to its interpreter, whose clocks differ per routine by -45% to
+20%; the reference runs use the recompiler. `ds_calltime.py` measures
calls there without breakpoints: it writes a timing stub into kernel RAM
the game leaves unused and points each function at it, then prints the
clocks of each call (`ds_calltime.py 2000 2200 PadUpdate PsyqBzero:tail:arg=a1`,
through `run_capped.sh`; its docstring has the options). `DUMMYBP=1` sets a
never-hit breakpoint, for the same run on the interpreter.
`ds_mark.py` logs when functions are entered instead (a stub writes a
marker to root counter 2, which `DUCKSTATION_CD_TRACE` logs with the tick):
for calls too long for root counter 0 and for timelines across fields, such
as the boot (`ds_mark.py 293 440 MainLoop SystemInit CdInit SsInit`).

## Following a scenario with marks

`ds_frames.py run <fields> <trace> <log>` plays a regression scenario
(`DSF_PADS`, `DSF_POKES` as in `regress.sh`, `DSF_MASKS` for
`RR4_AUTODRIVE`'s input from `ds_frames.py masks`) and writes the port's
trace lines, with `regress.sh`'s watched globals, for every field from the
game's first VSync (DuckStation field 296); run the port headless with
`PSYZ_CARD_UNPLUG=2` to compare. `DSF_MARKS` marks function entries (and
returns, `name:ret`) through writes to EXP2 0x1F802090, which the patched
regtest logs as `XMARK` with its tick under `DUCKSTATION_CD_TRACE`; unlike
`ds_mark.py`'s root counter 2 writes they leave the sequencer alone, so
the game follows its course after them. `DSF_EXC=1` also marks the
exception vector (I_STAT, DICR, the cause) and both exception returns, for
interrupt dispatch times. `ds_frames.py timeline <log> [from to]` lists the
marks and CD commands per field with their clocks after the vblank.

The port's side: a build with `-DRR4_FUNC_MARKS=ON` (Clang or GCC) compiles
the game and PsyZ with `-finstrument-functions`; `RR4_MARK=name,...`
prints `XMARK <name> <clocks>` at each entry with the emulated clock
(`Psyz_EmulatedClocks`, headless only), and `PSYZ_PAD_TRACE=1` each
blank's exact time (`at <ns>`).
