# DuckStation reference build

A patched `duckstation-regtest` driven by `tools/port/duckstation.py` is
the reference emulator: it plays a scenario's input and shows what the
console does. The port has no PS1 timing (`docs/ARCHITECTURE.md`, Time):
its loads, card I/O and CD reads take the host's time, so its fields no
longer line up with DuckStation's from the boot on, and the per-field
timing comparisons (and the tools that fitted the old PS1 time model to
them) are gone. The
patch is `duckstation-regtest.patch`; it applies to upstream DuckStation
commit `2c527f4f2c689f3db06246a2c9c566fbc2323dd8`
(<https://github.com/stenzek/duckstation>, "Qt: Add reset buttons for
tolerance/depth threshold", 2026-06-13). Other commits change emulation and
are not the reference. The reference runs use its exact timing mode
(below), not the recompiler.

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
| `card <slot> none\|blank\|<path>` | pull the card out of slot 1 or 2, or insert a blank card or a card file (`ds_script.py`'s `RR4_DS_CARD`) |
| `ticks` | system tick counter (33.8688 MHz) and frame number |
| `timing` | `OK timing=exact` with `REGTEST_EXACT=1`, else the CPU mode (`Recompiler`, `Interpreter`, `CachedInterpreter`) |

Environment:

| Variable | Effect |
| --- | --- |
| `REGTEST_EXACT=1` | exact timing (below): the interpreter on the recompiler's instruction costs |
| `REGTEST_PAD1_TYPE`, `REGTEST_PAD2_TYPE` | controller type name (`DigitalController` default on port 1, `AnalogController`, `NeGcon`, `JogCon`, ...) |
| `REGTEST_PAD2=1` | digital pad in port 2 (default empty) |
| `REGTEST_CARD1` | `none`: no card in slot 1; a path: that card file (shared); unset: a blank non-persistent card |
| `REGTEST_CARD2` | slot 2: unset or `none` empty (the default), `blank` a blank non-persistent card, a path that card file (shared; a zero-filled 128 KiB file is an unformatted card) |
| `REGTEST_VRAM_OUT` | write the final VRAM at exit |
| `DS_LOWRAM_LOG=<file>` | with any breakpoint set (the interpreter's checked path; `ds_script.py` with `DUMMYBP=1`), append each new code address/data address pair where game code, or the kernel called from it, reads or writes PS1 RAM 0..0xFFFF: the original's null-pointer accesses. `ds_lowram_report.py <file>` groups them by function |
| `DUCKSTATION_SPU_REG_TRACE=1` | log CPU writes to the SPU voice registers with the frame number |
| `DUCKSTATION_SPU_KEY_TRACE=1` | log voice key-on (with the system tick) and key-off |
| `DUCKSTATION_ADPCM_DUMP`, `DUCKSTATION_VOICE18_DUMP` | dump voice 18's decoded blocks / output samples (XA debugging) |
| `DUCKSTATION_CARD_TRACE=1` | log each memory card selection (`CARDSEL`: the slot, whether it holds a card), each card command (`CARDOP`: the command byte and the FLAG byte sent; 0x52 read, 0x57 write, 0x53 ID; the game's raw 0x5A probe is refused), each read or write's sector (`CARDREAD`/`CARDWRITE`) and each frame's end (`CARDEND`: the last reply, 0x47 good), with the system tick and the frame (the vblank that started it). The BIOS card driver's current port (it serves the ports on alternate vblanks, skipping a vblank it misses) is the byte at PS1 0x7264 (SCPH-1001), `RR4_DS_PEEK=80007264:1`; the port's `PSYZ_CARD_TRACE` prints its port at each vblank it takes |
| `DUCKSTATION_CD_TRACE=1` | log each CD command register write (`CDWR <command> tick <t> pc <pc> delay <ack delay>`), each command as it executes (`CDCMD`) and completes (`CDDONE`, a second response), each seek-time draw of the drive's generator (`CDSEEK n ... from to jitter`, absolute sectors), each vblank start (`VBLANK`) and each root counter 2 write (`RCNT2`), with the system tick and frame; compare with the port's `PSYZ_CD_TRACE` (its `cd seek draw n` lines count the same draws) |
| `DUCKSTATION_DMA_TRACE=1` | log the DMA's work and the MDEC's units with the system tick, which a run does not change: each channel's transfer start (`DMAT <channel> sync <mode> blocks <left>`), the end of each linked-list slice with the CPU ticks it took (`DMAS <channel> used <ticks> done <0/1>`: the tick of these lines leaves out the CPU's pending ticks, so a slice's stall is its `used`; `tools/port` fits take a call's GPU-channel slices outside the exceptions as its stalls), a request-mode transfer stopping with blocks left (`DMAE`, `req` 0 when its device stopped asking), each completion (`DMAC`, the transfer's interrupt follows), each DMA halt (`DMAH`) and its end (`DMAU`, with `lag`: how many clocks after its time the event ran, the end of the recompiler block being executed, and the CPU's `pc` then), each MDEC unit's decode start (`MDSB`) and its copy into the output FIFO (`MDCO`, with `lag` and `pc`), and each interrupt exception's entry (`IRQX` with the PC and I_STAT); `ds_script.py` keeps the log with `RR4_DS_LOG=<file>` |
| `DUCKSTATION_GPU_TRACE=<lo>:<hi>` | in DuckStation frames lo to hi, log each GP0 command the GPU starts (`GPUC <code> ticks <added> pending <after> fifo <words> tick <t>`: the GPU ticks it adds and the command event's pending ticks then, which the event counts off only when it runs, so a late event shows as negative pending), a VRAM copy's four words (`GPUW`) and each time the GPU goes idle (`GPUI`, GPUSTAT bit 26), with the system tick; with `DUCKSTATION_DMA_TRACE` it shows where the list DMA waits for the GPU |
| `DUCKSTATION_XA_OUT_DUMP=<file>` | append every XA frame the resampler makes as it enters the CD audio FIFO (s16 left, right; before the CD volume): compare with the port's `PSYZ_XA_DUMP` |
| `DUCKSTATION_IOREAD_TRACE=1` | log every root counter value read (`IORD T<n> tick <t> pc <pc> val <value> sync <0/1> ra <ra>`: whether the read brought the CRTC up to date; `ra`, the CPU's return address register, names the VSync caller) and GPUSTAT read (`IORD GS tick pc sync ra`), and each CRTC update past a line's end (`CRTCL tick lines <n> line <scanline> tick_in_line <gpu ticks>`), with the global tick (pending ticks included); no effect on emulation. A read updates the CRTC only once a line's end has passed since its last update: the counter 1 (hblank) value changes exactly when DuckStation's scanline (system tick x 709379 / 451584 / 3406 from power-on) does, so VSync(-1), which reads it twice 12 clocks apart (PCs 8008b178, 8008b188), repeats its read (18 clocks) exactly when that scanline changes between them (479394 pairs of the options boot, none against), and the blank comes at ceil((G x 451584 + 2 x r) / 709379) for the blank line's GPU tick G and the last update's sub-tick remainder r (every blank of the boot) |
| `DUCKSTATION_PC_TRACE=<lo>:<hi>[,<lo>:<hi>...]` | log each instruction's start between those system ticks (`PCT <pc> <tick>`, up to 16 ranges): its clocks, where an interrupt is taken (at the start of the first instruction at or after it; IRQX with `DUCKSTATION_DMA_TRACE`) and an exception's length to the interrupted instruction's return; no effect on timing |
| `DUCKSTATION_CD_SECTORS=1` | with `DUCKSTATION_CD_TRACE`, also log each sector the drive reads (`CDSECT <absolute sector> tick <t> frame <f>`; its data interrupt comes then): compare with the port's `PSYZ_CD_TRACE` `cd irq <us> 3780 sector <n>` lines (n is the next sector, n - 1 + 150 the absolute one) |

Port 1 is a digital pad (upstream regtest uses an analog one), port 2 and
card slot 2 are empty unless `REGTEST_PAD2` / `REGTEST_CARD2` say otherwise, audio goes to the null backend and the renderer is
software.

## Timing modes

The recompiler (DuckStation's default CPU mode) runs its timing events, the
interrupts, DMA steps, the vblank and the MDEC's units among them, only at
the end of the recompiled block being executed, so they land up to a block
late: tens of clocks in the game's code, about 270 in the kernel's register
restore before an exception's `rfe` (0x80000f40..0x80001014), whole BIOS
loops. A console raises them at their clock. `REGTEST_EXACT=1` (the
`timing` verb answers `exact`) runs DuckStation's interpreter, which checks
for events after every instruction, with the recompiler's costs: it skips
the interpreter's instruction cache fills and its multiply/divide stalls,
which the recompiler does not model (`src/core/cpu_core_private.h`,
`cpu_core.cpp` in the patch), so the CPU, GTE, memory and DMA costs are the
same and only the events' placement differs. Compared on the same build:

| Measurement | Recompiler | Exact |
| --- | --- | --- |
| VSync(1) poll (`ds_calltime.py`, fields 2000-2300) | 104 | 104 (the interpreter with its cache: 112) |
| PadUpdate | 2003 | 2003 |
| exception lengths by I_STAT (`DSF_EXC`, boot fields 120-140) | same | same |
| MDEC macroblock (`DUCKSTATION_DMA_TRACE`, the opening movie) | 2959.8 | 2958.3 |
| opening movie slice (11 macroblocks) | 32.54k (27%) or 32.77k (61%) | 32.53k (83%), 32.3k after a sector copy |

The 32.77k slices are the recompiler's: the second macroblock's DMA halt
falls in the kernel's register restore and ends after it. Exact runs take
about as long as recompiler ones for these scripts.

`tools/port/duckstation.py` runs exact timing unless `RR4_DS_TIMING=recompiler`,
and refuses a build whose `timing` answer does not match (an older build
without the verb). Every reference trace was regenerated in exact mode on
2026-11-19 (`ds_frames.py`'s four scenarios, and the 19 of the since removed
`ds_compare.py`). What
changed against the recompiler's references, with the same scripts:

- The boot: SystemInit starts 717 clocks sooner after DuckStation's vblank
  296 (the BIOS's boot and its CD waits); the SPU reverb clear's two vblanks
  come after its chunks' DMA exceptions instead of inside them (2549 clocks
  more over the 96 chunks); the AudioState bzero's 30 exceptions, taken in
  the ROM's code, refetch its uncached instructions (1239 more). SsStart's
  SetRCnt lands 1909 clocks later, so the 120 Hz timer's exceptions sit
  that much later against the vblanks for the whole run, and the blanks
  libpad and the card driver miss in them (F14) are other blanks: the card
  scenarios' driver port alternation and pad flags differ from the boot on,
  while modes, FrameCounter and rand stay the same in most scenarios (Grand
  Prix from field 4922 with the old autodrive input, Save and Load and the
  card menus around the card's prompt, two cards from 290).
- With the events at their clock the windows in which a root counter's
  exception hides a vblank have sharp edges in all 19 scenarios: libpad
  makes its exchange at every blank up to 1179 clocks into one and at none
  from 1207, the card driver takes its turn up to 1278 and at none from
  1311 (the recompiler's references put both edges about 100 clocks
  sooner).

## Replaying a scenario

`ds_script.py frames 'pad script' [out]` plays an `RR4_PAD_SCRIPT` (the
regression scenarios' button steps, mode anchors included) on DuckStation
and prints the port's `RR4_TRACE=1 RR4_TRACE_RAND=1` lines (vsync,
FrameCounter, mode, rand) for each field, so the two traces diff directly.
Fields count from the port's first VSync: DuckStation frame - 295, the
BIOS boot (`RR4_DS_FIELD_OFFSET` overrides it; `ds_frames.py` below
explains the moment). Run the port with DuckStation's cards:
`PSYZ_CARD_UNPLUG=2` (a blank card in slot 1, slot 2 empty). The old
offset of 225 with the port's two cards happened to line up the boot (the
port's second card takes about the 71 fields the offset left out) but not
the card menus. The controller sends a step's buttons at the exchange of the
blank before it, as the port's script does, so with one pad the receive
buffer shows them over the step's fields on both. `RR4_DS_PADLOG=1` adds
whether libpad's vblank handler ran, `RR4_DS_PEEK=addr:len,...` RAM bytes.
`RR4_DS_POKE=addr:hexbytes@vsync,...` stores bytes at a field's blank like
the port's `RR4_POKE`. Stick steps (`lx=0.6@...`, a DUAL SHOCK with
`REGTEST_PAD1_TYPE=AnalogController`) bind the half axes so that
DuckStation sends the byte the port sends for that value.

## Determinism and resolution

The reference runs are deterministic: three runs of the Save and Load and
the Versus two-card scenarios gave the same trace with the pad flag, and
three `ds_frames.py` runs with every exception marked (`DSF_EXC`) the same
log to the clock. Marks are not free, though: each stub takes about 12.5
clocks (`_SsVmKeyOn` with its six callees marked takes 150 more than with
itself only), and an exception mark's stub delays the BIOS chains' vblank
tests, so a marked run's outcomes differ from the reference's: with
`DSF_EXC` libpad makes its exchange at Save and Load's blanks 642, 773 and
904, which the reference misses, and the scenario's fields differ from 2603
on. Take positions and durations from marked runs and outcomes (fields, the
pad flag, the card driver's port) from unmarked ones (`ds_script.py`). A `VSync` mark stops the game (it stays at FrameCounter
21), and a run with several marks can follow a different course (eight
marks in the Versus movie put it 78 frames behind by field 1381): check a
marked run's trace against the reference before using its times.

On the recompiler (`RR4_DS_TIMING=recompiler`) an exception's entry and a
vblank move by tens of clocks with the code that runs when they are due
(the events wait for the end of the recompiled block): root counter 2's
exceptions entered 0 to 50 clocks after their period, and the same blank's
offset into the same root counter exception differed by up to 18 clocks
between two scenarios, so a decision near a window's boundary could go
either way for the same port position (blank 1166 in Save and Load against
Versus with two cards). Exact timing has no such spread.

