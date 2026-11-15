# C conventions

Use C11 and the native host compiler. Prefer clear functions, typed pointers,
subsystem headers and names that explain the role of a value.

Use fixed-width integer types (`u8`, `s8`, `u16`, `s16`, `u32`, `s32`) for
serialized disc and save data. Separate serialized 32-bit offset tables in
`R4.BIN` / `.rel` containers from host runtime pointers; decode offsets
explicitly instead of storing 64-bit host pointers in `s32` / `u32` fields.
Do not assume `long` or a pointer is 32 bits. Use `size_t` for host sizes,
`ptrdiff_t` for pointer differences and `uintptr_t` only when an integer
representation of a pointer is necessary.

Original code addresses, register bindings (`asm("$...")`), compiler scheduling
barriers (`__asm__("")`), symbol aliases (`__asm__("Symbol+1")`) and padding
added to match instructions are not part of the native design. Remove them.

Validate behavior with native tests and headless gameplay scenarios
(`PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy`).

## Host model

These rules are shared by every module; follow them instead of inventing a
local variant.

### Compilation

Every file must compile with the root CMake flags:
`-std=c11 -D__psyz -Werror=int-conversion -Werror=pointer-to-int-cast
-Werror=int-to-pointer-cast -Werror=implicit-function-declaration
-fno-strict-aliasing -fwrapv`. Silencing a diagnostic with a cast that
truncates a pointer is not a fix.

The game also builds for 64-bit Windows, where `long` is 32 bits (LLP64).
`tools/port/check.sh` checks game/native sources and each project header as a
standalone C translation unit. Besides the native pass it checks Clang's
64-bit MinGW target with `-Wshorten-64-to-32 -Wformat` and its 32-bit i686 target
(ILP32). Install `mingw-w64` (Homebrew) to run both locally. Linux CI requires
both header sets; `RR4_CHECK_LLP64=1` and `RR4_CHECK_ILP32=1` make a missing
cross-target fail instead of skip. Separate installations can set
`RR4_MINGW_SYSROOT` and `RR4_MINGW32_SYSROOT`.

`RR_ASSERT_SIZE` and `RR_ASSERT_OFFSET` are active C11 static assertions on
every host. Fixed disc/card/wire records keep fixed sizes; pointer-bearing
records and GPU packets express their sizes using native pointer/tag widths
and alignment. Use `RR_GPU_PACKET_SIZE` for a packed primitive payload's
allocation stride, not its DMA word count. Never keep a pointer or a host
size in `long` / `unsigned long`; SDK prototypes keep `long` only where PsyZ
declares `long`, and PsyZ `u_long` (pointer width) is `PsyqULong`.

### GTE

`include/rr_asm.h` expresses the original COP2 sequences in C over the PsyZ
GTE (`Psyz_GteDataRead/Write`, `Psyz_GteCtrlRead/Write`, `Psyz_GteCommand`).
Any remaining inline MIPS is translated the same way: `lwc2/mtc2` write a data
register, `swc2/mfc2` read one, `ctc2/cfc2` access control registers and a
`.word 0x4Axxxxxx` command becomes `Psyz_GteCommand(word & 0x1FFFFFF)`.

Projected screen words (SXY0..SXYP) carry PsyZ's precise vertex (subpixel
x/y and depth, drawn by the geometry correction option) by the address they
are stored at. Store them with `RR_GTE_STORE_SXY` (or an `rr_asm.h` swc2
macro), load a stored word back into the GTE with `RR_GTE_LOAD_SXY`, and copy
one to a cache or a packet with `RR_SXY_COPY` from the location it was stored
to, not from a by-value local. A plain store still writes the same word, but
that vertex is drawn snapped to whole pixels with affine texturing.

### Scratchpad

`SCRATCHPAD` is the host buffer `RrScratchpad` (1 KB, `include/scratchpad.h`).
Scalar work areas keep their original offsets. Host pointers are never stored
in it. The packet cursor and current ordering table (the first two PS1
scratch words) are the global `GpuScratch`, reached through `RR_GPU_SCRATCH`.
A scratch view that started with `GpuScratchPointers` keeps an 8-byte
placeholder at offset 0 and its users read the pointers from `RR_GPU_SCRATCH`.
Any other pointer a scratch view held moves to an ordinary static/global.

### GPU packets and ordering tables

Packets use the PsyZ `P_TAG` header: `GpuDmaTag { uintptr_t address;
uintptr_t length; }` (`include/rr/gpu.h`), 16 bytes, followed by the PS1
payload words (colour/code word first). Ordering-table entries are bare
`GpuDmaTag`s (`FrameOrderingTable.tags`). Translate PS1 idioms:

| PS1 | Host |
| --- | --- |
| `u32 tag` / `addr:24,len:8` first field | `GpuDmaTag tag;` |
| `p->tag = n << 24` | `p->tag.length = n` |
| `p->tag = (p->tag & 0xFF000000) \| (*ot & 0xFFFFFF); *ot = (*ot & 0xFF000000) \| ((u32)p & 0xFFFFFF)` | `RR_GPU_LINK(ot, p)` |
| set length and link | `RR_GPU_ADD_PRIM(ot, p, n)` |
| end marker `0xFFFFFF` | `RR_GPU_END_LINK` |
| `((u8 *)p)[7]` (command code) | the code byte of the payload's first word |
| `u32 *ot` ordering words | `GpuDmaTag *ot` |

PsyZ reads a packet's payload as packed 32-bit GP0 words only when its first
word is a shape command (code `0x20..0x7F`) or it contains the `MargePrim`
NOP; otherwise (state-only packets: `E1..E6` draw mode, texture window, draw
area/offset, mask) each payload word is a `uintptr_t` (`u_long`), as in the
PsyZ `DR_MODE`/`DR_AREA`/`DR_TWIN` structs. Such packets use `uintptr_t`
command words (e.g. `GpuDrawModePacket`, `GpuCommandPacket`). A PS1 packet
that put a state command before a shape in one packet is split into a state
packet and a shape packet.

Packet sizes and cursor advances use `sizeof` of the packet struct, never PS1
byte counts. GTE stores into packets (`swc2 $12, 8(p)`) target the payload
field by name, not a PS1 offset. Packet arenas are sized for host headers.

### SDK and PsyZ

PsyZ (`third_party/psyz`) implements the PlayStation SDK: libgpu, libgte,
libetc, libapi, libcd, libspu, libsnd, libpad, libcard, libc. The PsyQ
implementations under `src/main/psyq/` that drive hardware are removed; game
headers under `include/psyq/` keep only prototypes whose types are
ABI-compatible with PsyZ's `__psyz` declarations (pointers stay pointers,
`OT_TYPE`/`P_TAG` layouts as above). Game modules do not include PsyZ's SDK
headers directly when a game header already declares the same function.

### Fixed RAM and hardware addresses

Literal PS1 addresses (`0x800F0000`, `0x1F801xxx` registers, overlay windows
`0x80114878`) are not dereferenced. RAM addresses become the named global that
lives there; hardware registers become PsyZ calls or are removed with the
driver that used them.

### Globals

Every global has one ordinary definition. Initialized main-executable data
is defined in C with its original values; pointer-valued words are real
pointers. Objects with several layout views of the same storage get one owner
definition with named views (a union or accessor), not several aliasing
objects.

### Overlays

The nine mode overlays (`grand_prix`, `time_attack`, `versus`, `garage`,
`records`, `options`, `extra_trial`, `serial_link`, `movie_playback`) are
linked statically. Their entry points are registered explicitly in the mode
tables (`ModeGroupNFuncTable`) instead of being loaded into the
`0x80114878` window. Each overlay keeps its `ovlNNN_` namespace and resets
its own state (`ovlNNN_reset_module`) where the loader would have read its
image (`OverlayModuleBind`), restoring the data and zeroed BSS of a fresh
load. A unit that defines overlay state lists it in its
`ovlNNN_<unit>_state` table (`rr/overlay_module.h`). Data
the loader read from `R4.BIN` (models, textures, sounds) is still read from
the user's disc image.
