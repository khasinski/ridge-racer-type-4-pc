# Performance and stability audit — 2026-11-15

Status: in progress. Scope is the entire native port, its PsyZ integration,
platform services, overlays, tooling and tests. A passing subset is not proof
that the full audit is complete. Existing worktree performance changes are
retained and must be included in final validation.

## Verified fixes

- Serial-link receive: separate the low three ring-index bits from the peer
  error flags. Previously a valid packet reporting an error could index beyond
  the eight-packet ring. Regression exercises all 256 flag values, verifies
  the selected slot, untouched slots, active/transmit indices and error code.
- Garage flood fill: replace recursive traversal with an 8 KiB explicit work
  list. Mark on enqueue, so each of 4096 pixels can be queued only once.
  Valid fills cannot reach the original 8188-depth cutoff, so the final image
  is unchanged. Reject invalid coordinates/colours and no-op replacement.
  Regression compares 300 cases, including full fills starting at all four
  corners and random images, against an unpacked breadth-first reference.
- UI glyph animation: compute the next frame index with one assignment,
  removing the nested increment/assignment.

- Memory-card tool: bound file reads, reject cyclic/out-of-range block chains,
  impossible file sizes, duplicate names and unsafe/non-ASCII filenames before
  exporting. Roundtrip and malformed-input regression added to CTest when a
  Python interpreter is available.

Validation so far: all 13 port tests pass with AddressSanitizer and UBSan
(`UBSAN_OPTIONS=halt_on_error=1`). All 270 game translation units pass the
existing compile checks on the native host and Windows LLP64 target.
Release build: all 14 tests pass, including the optional Python card-tool
regression. The first ASan+UBSan GP run completed all 12600 fields, exit 0,
without sanitizer reports (CPU-only hidden renderer). This does not cover
GPU enhancement execution. Subsequent prototype/arena changes also pass all
14 sanitizer tests and all 270 native and Windows LLP64 compile checks.

Additional fixes in the second audit pass:

- Garage arena initialization now publishes complete native pointers, removing
  unions that wrote only a 32-bit half and read an uninitialized 64-bit pointer.
  Scrollbar/preview drag removes equivalent pointer/integer no-op expressions.
- CD completion handler now returns a defined status through its real typed
  dispatch table; card-menu handlers also use checked prototypes without casts.
- Replaced unprototyped game declarations with headers matching definitions;
  added a small resident/overlay entrypoint header checked by overlay modules.
  Explicit GTE scalar prototypes preserve `long` argument width on LP64.
- Pause-menu triangle coordinates are explicitly packed into 32-bit XY words,
  rather than passing a structure to a function expecting an integer.
- Opponent initialization supplies the previously missing track-search limit
  (`TrackNodeCount + 1`, consistent with other whole-course searches).
- Paused Grand Prix HUD receives its previously missing pause argument, so
  split timers are not decremented based on an undefined argument value.
- Removed ignored, side-effect-free arguments from calls to no-argument
  functions. Completed header prototypes from their implementation signatures.
  No unspecified (empty-parenthesis) declarations remain in game code or
  its headers; `check.sh` rejects new ones (`-Wstrict-prototypes`).

The full four-scenario CPU regression passes: all 38200 field traces and all
1273 captures are identical to HEAD's stored reference (attract 12000/400,
time attack 6600/220, Grand Prix 12600/420, versus 7000/233).
Log: `/tmp/rr4-project-audit/prototype-regress.log`.

Local logs and scripts: `/tmp/rr4-project-audit/`.

## GPU/CD investigation, third pass

- Paced hidden GPU, all enhancements (8× + AA = 16×),
  `PSYZ_AUDIO_CLOCK=wall`, normal CD timing completed 1200 fields and exited 0.
  The movie progressed from frame 284 to 815 and reached menu mode 1/4.
  H19 did not reproduce in this attempt; this is not proof of a fix.
  Log/script: `/tmp/rr4-project-audit/paced-cd-before*`.
- Reproduced the intermittent scaled-VRAM bug: 1/5 isolated raster tests and
  2/3 full GPU suites failed. Native VRAM matches; decimation of scaled VRAM
  differs by 32–96 pixels, often seed 2 at 2×. Preserve exact comparisons.
- Splitting MoveImage's copy passes did not fix it (2/10 failures); reverted.
  Splitting each blend-destination copy into its own pass also failed (at
  least 12/30 failures); reverted. Neither experiment is a retained fix.
- Temporary per-command readback suppresses seed-2 differences; readback only
  at stream chunks still reproduces them. A prefix search reproduced after
  the first 91 commands of seed 2: 48 pixels, first (320,376), CPU d3f3 versus
  GPU bb11. Last command 90 is 25a01351 ...; command 89 is a semi-transparent
  Gouraud triangle. This narrows investigation to batching/order but does not
  yet establish cause. No tolerance or disabled effect has been introduced.
- Diagnostic test variants saved outside the repository:
  `tests-chunk-probe.c`, `tests-prefix91-probe.c`, and original accepted
  `tests-before-raster-probe.c` under `/tmp/rr4-project-audit/`. Prefix variant
  uses `PSYZ_RASTER_PREFIX=1`, optional `PSYZ_RASTER_WORDS=1`; existing test
  name `gpu::sdl_gpu_rasterizer_matches_cpu_rasterizer_at_every_scale`.
  To resume, copy the desired diagnostic variant over PsyZ test_libgpu.c,
  rebuild build-perf-psyz, run hidden/silent via run_capped.sh, then restore.
  Full command dump: `gpu-prefix91.log`; first prefix failure: `gpu-prefix.log`.

## Destination-copy workaround, fourth pass

Current retained candidate: `DrawTargetRuns` rounds each semi-transparent
blend snapshot outward to 64×64 target-pixel blocks, clamped to target bounds.
Primitive overlap decisions still use the original boxes. All copies read
one pre-pass image, so enlarged snapshot regions may overlap safely. This is
an empirically validated workaround for partial copies on Metal; the exact
driver/internal mechanism has not been established. No shader math, effects,
mask behavior or image-comparison tolerance was changed.

Evidence before the final game/performance checks:

- Full-target copies: 10 initial + 30 confirmation raster tests passed, but
  copying the whole target is too expensive to adopt without measurement.
- Single union rectangle: still failed (seed 3 at 8×); widening bounds one
  pixel also failed, individually and combined. Both experiments discarded.
- Original separate rectangles rounded to 64×64: 30/30 raster tests passed,
  then 10/10 full 83-test GPU/dither suites passed.
- Regression strengthened to repeat seed 2 at 2× twelve times while reusing
  targets. The unmodified renderer failed 1/3 runs; the aligned renderer
  passed 5/5. Exact comparisons are retained. Test source contains only this
  regression strengthening, none of the temporary probes.

Final 64-pixel-block checks:

- All six 8×/AA/interpolated game captures, held markers and traces match the
  accepted reference exactly (`aligned-game.log`, 9601 fields).
- Performance is too costly to accept as-is: same binary apart from copy
  rounding, sequential 8501-field runs: whole run 88.338 → 107.530 s;
  race fields 8100–8500: 7.836 → 12.933 s. Both force `PSYZ_GPU_FETCH=0`;
  Apple-family GPUs normally use framebuffer fetch and avoid this path.
  Logs/binaries: `/tmp/rr4-project-audit/copy-timings*`, `unaligned-binary`,
  `aligned-binary`. The copy workaround still needs optimization/selection.
- Current experiment reduces the blocks to 16 pixels and confines the
  workaround to Apple/Metal. 15/15 strengthened raster tests pass. All six
  game images and held markers match the old reference. Interpolation counts
  differ by one at fields 9300/9600 after the root ABI/state edits, so the full
  trace comparison correctly refused equality (`aligned16-game.log`). No
  image tolerance was relaxed. A new pair of binaries with identical root
  changes, differing only in copy rounding, was measured next.
- Completed timing session 1453: `current-timings.py`, sequential 8501-field
  runs: current unaligned, 16-pixel-aligned, default Apple framebuffer fetch,
  then fetch plus both experimental CPU interpolation optimizations. Logs:
  `current-timings.log`, `current-timings/{before,after,fetch,fetch_combined}`.
  Binaries `unaligned-current-binary` and `aligned16-current-binary`; no other
  GPU work during these measurements. Results: fallback original 94.353 /
  10.289 s (whole/race), fallback aligned16 121.350 / 8.716 s, default fetch
  29.351 / 5.989 s, fetch+combined CPU 24.139 / 2.756 s. Rounding is a
  correctness workaround with whole-run cost, not a claimed speedup.

## Overlay state and ABI checks, fifth pass

- Overlay images are captured at SystemInit entry before resident/card writes.
  Capture is idempotent and does not reset live state; actual binds, including
  duplicate disc entries, restore the captured initial state. Regression
  mutates versus names/choices before its first bind, checks repeated init,
  then checks both versus disc entries restore the linked data.
- Options state now has one union owner shared by menu and controller views,
  with unconditional native size/offset assertions. Removed the incompatible
  cross-translation-unit declarations of that owner.
- Unloaded car-record fallback is writable, matching its non-const users and
  the derived gear-rate writes in InitPlayerCar; removed cast-away const.
- Versus/link course rows use typed indexing instead of PS1 byte strides.
- The last unspecified queue callback prototype is now two native-width
  parameters, matching PsyZ addque2. Removed the unused, undefined
  GPU_DmaDoneRequeue declaration. `check.sh` now enforces strict prototypes.
- G11 inspected: InitMotionController begins by clearing the entire native
  PresentationController, including every early presentation flag store.
  No partial-size init or surviving field was found; no code change needed.

Validation: all 14 ASan+UBSan CTests pass; all 270 game translation units pass
native and Windows LLP64 compilation with strict prototypes enabled. Full
four-scenario sanitizer game regression passes: all 38200 field traces and
1273 captures identical to HEAD. Log:
`/tmp/rr4-project-audit/overlay-asan-regress.log`.

## Directory and renderer boundary checks, sixth pass

- R4.BIN parsing now validates the release's entry count, input length,
  monotonic sector boundaries, file extent and signed sector/byte arithmetic
  before publishing any directory entry. Invalid directories exit with a
  diagnostic instead of creating invalid reads. Unit regression covers short
  input, invalid counts/offsets, extent and arithmetic overflow, unchanged
  output on failure, and PAL/JP directory sizes.
- GPU masked MoveImage now clips/validates coordinates before passing them to
  the CPU merge. The former cast of an unsigned destination to short could
  create a negative VRAM index. Backend regression supplies UINT32_MAX and
  checks mask preservation/set behavior at the clamped destination. Normal
  GP0 dispatch already splits transfers within VRAM; this strengthens the
  backend boundary, not a reproduced crash in normal R4 gameplay.
- ReplayFile ReadAll already bounds allocations by its format size limit;
  decode verifies checksums and block/name bounds, with malformed-input tests.
  No new replay-parser defect confirmed in this pass.

## Combined CPU interpolation validation

- 9601-field run with both cache/tree and both reference-verification flags
  completed with no differing edge pointer. Six game images, held markers
  and full traces match the current-root `aligned16-game` reference exactly.
  Thus the one-count difference from the older reference belongs to the
  preceding root/layout changes, not the CPU optimizations or fetch path.
- Both CPU optimizations enabled by default after this result. Explicit
  `PSYZ_INTERP_QUERY_CACHE=0` and `PSYZ_INTERP_EDGE_TREE=0` retain comparison
  paths. Full AA is unchanged; cheaper SSAA remains opt-in.
- All 14 CTests pass in Release and ASan+UBSan after parser/bounds changes,
  all 270 game files pass native and Windows LLP64 strict-prototype checks.
  Three full 84-test GPU/dither suites pass with the 16-pixel Metal workaround
  and masked-copy boundary regression. Final Release, ASan and SDK builds
  pass. With the CPU defaults enabled, the full 84-test GPU/dither suite
  passes again and an 8501-field run with no CPU optimization flags matches
  all traces from the explicit-flags run (`defaults-smoke.log`).

## Guarded frame arenas and native concurrency

Each 4 MiB packet arena now has inaccessible host pages immediately before
and after it. Initialization verifies host page alignment and refuses startup
if protection fails. The final-cursor check remains; the new guards stop an
adjacent write before it reaches HUD state or the other frame bank. The new
`frame-storage` test verifies both valid end bytes, HUD writes and repeated
initialization, then checks all four protected boundaries in child processes.
A child succeeds only when its fault address is the intended guard byte.
Release and ASan/UBSan pass all 15 port tests. The 271-unit compile check,
including the new host protection helper, passes native and Windows LLP64.

ThreadSanitizer reproduced real native races in the kernel lock owner, CD
service reentrancy flag, read request state, callback pointers, DMA dispatch,
movie display rectangles and CD audio mixing. The fixes use the existing
recursive kernel lock for command/callback state and the audio mutex for CD
volume/mixing. The IRQ startup state is atomic and claimed before spawning;
MDEC state accesses and reset/cancel/complete transitions share the dispatch
lock. Wait loops release it while waiting for another thread. The movie's
short display-state updates and ready polling now use that same lock.
The CD status owner now matches its SDK byte declaration.

Validation artifacts in `/tmp/rr4-project-audit/`:

- `tsan-ring/run.log`, `tsan-gpu/run.log`: 1200-field startup/movie tests,
  paced wall clock, CPU and hidden Metal (8×, full AA and interpolation),
  both exit normally without a TSan report. No timing shortcut or race
  suppression. These do not establish coverage of all modes or H19's cause.
- `tsan-init-sdk-test.log`: all 32 CD/MDEC tests pass under TSan, including
  new concurrent status/transfer and 20000-completion DMA callback tests.
- `concurrency-sdk-final-test.log`: 110 SDK tests pass, one platform skip;
  the standalone fake-clock cases pass too.
- `concurrency-regress.log`, `concurrency-asan-regress.log`: both Release
  and ASan/UBSan retain all 38200 field traces and 1273 native captures.

The 8× capture comparison exposed a separate interpolation defect: its
four-slot address lookup evicts live tags depending on packet arena layout.
The original guarded build differs from the preceding captures despite equal
game state. Full probing with periodic reclamation of obsolete addresses now preserves
live tags. Reclamation uses a reusable 1.5 MiB host scratch allocation;
allocation failure retains full probing and the true-capacity fallback. A randomized 72000-address churn regression fails the old
lookup (5743 of 6000 tags remain by the last checked frame) and passes the
new one (all 6000 tags and pairs). The full GPU/dither suite passes 85/85.
Two 9601-field runs with 64 KiB versus 128 KiB guards (different packet
addresses and bank strides) produce identical six images and interpolation
traces (`interp-layout-standard`, `interp-layout-padded2`). Midpoint count at
field 9600 is 1180 in both. The earlier reference has fewer admitted midpoints
because of lost tags, so these are a correctness change, not a claim that
all old midpoint images remain identical. No reference files or comparison
tolerances were changed. The 9600 midpoint was visually inspected.

The first alternate-layout experiment had mixed old/new objects because a
header rewrite shared the object's timestamp second. Its boundary test and
movie read faulted. After rebuilding with distinct modification times, both
layout builds pass the frame-boundary test and the full scenario. The final
normal-layout Release build passes 15/15 tests again
(`interp-map-release-final-ctest.log`); ASan/UBSan also passes 15/15
(`interp-map-asan-ctest.log`). Final TSan CD/MDEC tests still pass 32/32
(`concurrency-tsan-final-test.log`).

## Bounded movie VLC decoding

The original in-place decoder now receives the compressed extent and command
buffer capacity. Every consumed bit and emitted halfword is bounded; the
header's command size must fit the 0xA000-byte destination. Version/quantizer
and invalid Huffman prefixes are checked. Partial calls retain their input
extent and remaining-bit budget; completion or failure invalidates resumable
state. Stream metadata is checked before changing display dimensions, and
malformed frames are freed without publishing their command-buffer selector.
Both movie startup and ongoing playback stop cleanly on the error result.

The original decoder speculatively reads a halfword beyond some complete
frames. Rejecting that refill would end a valid movie early. The corrected
reader supplies zero lookahead at the physical boundary while separately
checking actual bit consumption. A synthetic version-3 regression covers the
exact-end case without retail data; truncated end markers remain errors.

`movie-vlc` covers versions 2/3 with explicit expected run/level codes, partial
resumption, all shortened input/output extents, an oversized command header,
20000 randomized bounded streams and ten malformed frames through the actual
CD ring/playback consumer. All 16 port tests pass with ASan/UBSan.
A local comparison extracted all 3280 movie frames from the available PAL
image: all decode successfully and all emitted command bytes equal the old
decoder. The old decoder required four zero lookahead bytes in this isolated
comparison; without that padding ASan reproduces its out-of-bounds read.
All observed frame dimensions are 320×176 and the largest command is 36484
bytes, below the existing 40960-byte buffer. No retail data was added to the
repository. Logs: `vlc-disc-test.log` (old read overrun),
`vlc-disc-lookahead-test.log` (3280 identical), `vlc-stream-asan-ctest.log`.

The corrected Release game retains all 38200 state traces and 1273 native
captures (`vlc-lookahead-regress.log`). The 8× all-effects comparison also
retains all six images and all 32 sampled traces (`vlc-gpu-retry.log`). Its
first attempt timed out waiting for a capped-run slot without starting the
game; the retry uses the same saved executable. Release and ASan/UBSan pass
16/16 port tests; native and LLP64 syntax checks pass 271/271 translation
units. The sanitizer gameplay rerun also retains all 38200 state traces
and 1273 native captures across attract, GP, VS and Time Attack
(`vlc-asan-regress.log`).

## Interpolation pair and ghost-history retention

The pair allocator called a cluster of 65 live entries "full" even when
unused entries remained elsewhere in its 16384-entry ring. Search the whole
ring before falling back, and preserve tags queued for the upcoming frame
as well as recently drawn history. A 20-frame test keeps 512 identities
while introducing 2048 new ones each frame. The old allocator drops seven
live tags/pairs at the first wrap; the new allocator retains all 2560 tags
and all 512 recurring pairs in every checked frame.

Ghost history had a separate fixed limit of 32768 vertices, too small for
its pair table. Each of the two banks now grows on demand up to six vertices
per pair (98304 vertices), retaining earlier entries across growth and
allocation failure. The buffers are reused and freed with the GPU device;
order storage is tied to the pair count. A 9000-quad regression checks the
actual midpoint pixel of the last quad when it disappears: the old store
loses it (black), while the new store retains it (red). The game's native
frame remains black at that location.

Evidence under `/tmp/rr4-project-audit/`:

- `interp-pair-before-checked-test.log`: the old pair allocator fails.
- `interp-store-before-checked-test.log`: with the pair fix alone, the ghost
  pixel test fails; the other two history tests pass.
- `interp-history-checked-test.log`, `interp-history-asan-test.log`: all three
  history tests pass in Release and ASan/UBSan on hidden Metal.
- `interp-history-full-0.log`: all 87 GPU/dither tests pass, including the
  target-copy fallback and randomized rasterizer comparisons.
- `interp-history-gpu.log`: the full 9601-field GP scenario at 8×, full AA
  and all enhancements retains six exact images and all 32 sampled traces
  (including midpoint counters). No pair/store/order overflow occurs.
  Peak retained history is 46923 vertices, beyond the old store's capacity.
- `interp-history-ta.log`, `interp-history-ta-compare.log`: Time Attack,
  replay and retry complete 6600 fields at 8× with all enhancements. All 66
  sampled game-state traces equal the native reference; no interpolation
  table overflows occur (peak history 15441 vertices). Six captures were
  produced; fields 3000 and 4200 explicitly hold the original frame.
- `interp-history-game-ctest.log`: all 16 Release port tests pass.

These fix history loss; no new throughput claim is established. Broaden
mode coverage, history-reset/failure-path tests, and profile the remaining
CPU junction searches before claiming further performance improvements.

## Conservative interpolation edge rejection

The new default early filter in `InterpSelectJunction` uses segment dot and
cross products to avoid divisions for candidates the existing precise test
would reject. Its distance bound is expanded only for early rejection; the
original acceptance thresholds, candidate order and scoring remain intact.
`PSYZ_INTERP_EDGE_REJECT=0` disables it. With tree/query verification enabled,
the reference search bypasses this filter and compares the selected edge for
every query. The optional edge-cache allocation still has the original
uncached fallback.

Two reversed-order pairs of full-settings 8× GP timings show 13.5% and 11.9%
less race-slice time on this M5 Max. See `PERFORMANCE.md` for absolute times,
settings and measurement limits. A depth-only experiment was discarded.

Artifacts in `/tmp/rr4-project-audit/`:

- `edge-line-fuzz.c`, `edge-line-fuzz.log`: source-extracted helper tested
  under ASan/UBSan with two million random and near-threshold cases; zero
  false rejections among 1526318 originally accepted candidates.
- `edge-line-verified.log` (run directory `edge-reject-verified`): the full
  9601-field GP differential run retains six images and 32 full sampled traces.
- `edge-line-ta-verified.log`: 6600-field Time Attack/replay/retry differential
  run retains six images, two held markers and all 66 sampled traces.
- `edge-line-full-0.log`: all 87 GPU/dither tests pass with both differential
  checks and early rejection enabled, including target-copy rendering.
- `edge-line-default-ctest.log`: all 16 port tests pass after default enablement.
- `edge-line-asan-full.log`: all 87 GPU/dither tests pass under ASan/UBSan,
  with the default-enabled filter and both differential checks.
- `edge-line-vs-verified.log`: default-enabled 7000-field VS differential run
  completes, with all 70 sampled game-state traces equal to the native
  reference and no interpolation table overflow (peak retained history
  24006 vertices). Six captures were produced; field 6000 holds the original
  frame. The field-6600 midpoint was visually inspected: both split-screen
  views and their HUDs render correctly. No pre-change GPU image reference
  was used for this VS run.

## Shared global declarations and projection scratch ownership

A canonical-type Clang scan of the 286 game/native/host C translation units
found 21 incompatible external-object declarations. This resolves typedefs
inside arrays and function pointers and distinguishes compatible incomplete
outer array bounds from differing complete bounds. The earlier textual D1
counts included compatible aliases; the actual conflicts include the host
diagnostic declaration of `PlayerCars` as `unsigned char[]`.

- Dispatcher/menu state now has one declaration in `game/data/menuvars.h`.
  Consumers include it instead of declaring local signed/unsigned variants.
  In particular, `FadeCountdown` stays signed for its negative sentinel, and
  `ModeGroup` / `SubmodeState` use their definitions' unsigned halfwords.
- Card, load, link and garage globals use their existing owner headers.
  `CardPromptChoices` has four entries everywhere. The three root declarations
  of SDK `VBLANK_MINUS` also share the unsigned type of its PsyZ definition.
- Versus names have one `[2][12]` declaration and a matching definition; save
  loading and preview initialization index by player. The allocation and
  overlay-reset extent remain 24 bytes. The existing reset test now uses the
  same header instead of a flat-array redeclaration.
- Autodrive reads `PlayerCars[0]` through the existing vehicle header and named
  fields, preserving its signed halfword conversions. The unused PsyZ umbrella
  include is removed from diagnostics; its specific subsystem includes suffice.
- `PadDefaultButtonPresets` and `CardPortPollStatus` also use their owner headers;
  their previous incomplete/complete outer bounds were compatible, not defects.

The final scan includes the 11 C test units as well: **297 units, 1348 external
symbols, zero incompatible types** in those parsed units. Definitions and all
current consumers of the corrected globals see their shared declarations.
This is not an assertion about every unused header or every SDK internal type.
Release and ASan/UBSan builds pass, as do all 16 port tests. The 271-unit compile
check passes both native and Windows LLP64. The four Release game scenarios
retain all 38200 field traces and 1273 captures. A hidden Metal Time Attack /
replay / retry run at 8×, full AA, interpolation and all enhancements retains
six exact images, two held markers and 66 complete traces, with no interpolation
table overflow. These are correctness checks, not a measured speedup.
Artifacts: `/tmp/rr4-project-audit/globals/` (scanner sources, before/after JSON,
`check.log`, `ctest*.log`, `regress.log`, `gpu-ta.log`). The complete
four-scenario ASan/UBSan gameplay run also retains all 38200 traces and 1273
captures, with no sanitizer error (`regress-asan.log`).

The separate D12 check reproduces an include-order bug: including
`rr/selection_initializer.h` first makes `RR_PROJECTION_SCRATCH` point to the
absolute PS1 address `0x1F800000`, despite the native scratchpad allocation.
Remove all eight local macro definitions and define the typed view once in
`rr/projection_state.h`, beside its layout and using `RrScratchpad` from
`scratchpad.h`. The existing stability test includes the formerly bad header
first and checks both the host-buffer address and a scalar write's location
with adjacent bytes unchanged. The isolated ASan/UBSan reproducer fails before
the fix and passes afterward. After this additional change, the full native
and LLP64 compile checks and all 16 Release/ASan/UBSan port tests pass. All four
Release gameplay scenarios again retain 38200 traces and 1273 captures. The
sanitizer gameplay run above used the globals fix before this macro cleanup;
the final sanitizer stability test directly covers the include-order defect.
Artifacts: `/tmp/rr4-project-audit/scratch-owner/` (`before.log`, `after.log`,
`check.log`, `ctest*.log`, `regress.log`).

## Native layout assertions and standalone header checks

The old `RR_ASSERT_SIZE` / `RR_ASSERT_OFFSET` macros silently did nothing on
64-bit hosts (also MSVC, where `__SIZEOF_POINTER__` is not defined). Enabling
them against the actual native types exposed 161 stale checks in compiled
game modules. A plain 32-bit build was insufficient: PsyZ uses a two-word DMA
tag even with 32-bit pointers, unlike the single-word PS1 tag.

The macros now use C11 static assertions on every host. GPU packet checks
account for the native tag, payload, alignment, pointer-width DR command
cells and merged-primitive NOPs. Pointer-bearing camera, CD, garage, movie,
interrupt, vehicle and frame-arena checks use native widths and alignment.
Fixed disc/card/wire records retain fixed sizes. These edits change checks,
not the runtime data layouts. Existing SDK ABI comparisons remain enabled.

Checking all 615 project headers independently found three missing type
includes (`trkproj.h`, `screenframe.h`, `menu13.h`) and three more stale layout
checks in the CD callback view and the selection initializer. These are fixed;
the vehicle checks now live beside their owner types. The default `check.sh`
covers 272 game/native sources plus the 615 headers, with native, LLP64 and
ILP32 passes. Its toolchain probe reads actual CRT headers and checks pointer /
long widths, so an empty-file compile cannot disguise a missing sysroot. Both
MinGW header sets are required in the Linux CI job. The workflow is updated;
no remote CI run is claimed here.

Local validation:

- 887/887 files pass each of native macOS ARM64, Windows x86-64 LLP64 and
  Windows i686 ILP32 checks (`check-all.log`).
- Release and ASan/UBSan builds and all 16 port tests pass (`build*.log`,
  `ctest*.log`). No game throughput claim follows from these compile checks.
- A temporary mutation grows `TimeRecord16.name` from ten to eleven bytes.
  The previous macros accept it on the 64-bit host; the new checks reject
  its size and both following offsets (`mutation-old.log`, `mutation-new.log`).
- Required cross-checks with nonexistent header roots fail with status 1
  (`missing-toolchain.log`).

Artifacts: `/tmp/rr4-project-audit/layout/`, including the preliminary
native/Windows assertion inventory and precise pre-change header copies.
The literal-memory-operation sweep found the reviewed fixed clears in pad,
audio and options state to be scalar ranges. The follow-up review (D3,
closed) rewrote every remaining literal size, stride and byte offset on a
structured object as sizeof, field spans (`RR_FIELD_SPAN`/`RR_FIELD_TAIL`)
or indexing, with static assertions where the PS1 value is relied on; the
literals left are offsets into serialized disc, dialogue and save data and
the menu arena budget.

## Replay codec and car-history validation

`ReplayFileEncode` used an unbounded `strlen` on a fixed-size block name;
the new malformed-name test reproduces a heap-buffer-overflow under ASan.
Names are now bounded and unique, and every payload addition is checked
before summing, including on 32-bit hosts. Failure resets the returned size.
The reader checks stream errors, temporary output names use their complete
allocated path, and directory paths are rejected before filesystem operations
if the caller's buffer is too small. Header dates outside SDL's signed
nanosecond range no longer wrap; course labels use widened arithmetic.
Tests cover maximum payloads, invalid names/sizes/counts, duplicate names,
extreme metadata and undersized directory buffers. Erroring streams and
over-1100-byte output paths were reviewed in code, not fault-injected locally.

Session trail reads/writes previously tested `cursor + size`, allowing u32
wraparound. A direct regression fails the old implementation. The loader also
accepted an arbitrary `CarSlotBuffers` blob and could change live slots before
discovering its truncated contents. It now validates all sixteen entries and
their exact end before accepting or applying a nonempty history. Presence
flags, padded vertex lengths, ring counts/indices and history phase/count
relationships are checked against the renderer's actual ranges. Empty history
retains the writer's existing optional-history fallback. Model mismatches
still retain the freshly loaded buffers, and valid restores retain live
pointers. This adds validation at replay load, not a per-frame drawing cost.

The new `replay-session` test compiles the actual session implementation to
exercise its private serialized state without exposing another public API.
It checks every truncated prefix, trailing bytes, wrapping lengths, invalid
ring state, unchanged live data after rejection, exact restoration and model
mismatches. Three previously recorded TA/GP/VS replay files also pass the
session parser under ASan/UBSan. Hidden, silent sanitizer gameplay loads each
of these files, plays 188 frames before the scripted exit and retains all
21 reference captures (seven per mode). This covers the beginning of each
loaded replay, not the full-length VS discrepancy H20.

Artifacts: `/tmp/rr4-project-audit/replay-files/` (codec failing-before and
passing-after tests), `/tmp/rr4-project-audit/replay-trails/` (old cursor test
failure, builds, CTest, recorded fixtures, playback script/logs/captures).
The session source/test compile on native ARM64, Windows LLP64 and ILP32.
All 17 Release and ASan/UBSan port tests pass. The final phase/count guard is
covered by the parser tests and the three recorded files; the playback
comparison preceded that additional rejection-only guard.

The subsequent storage checks below address mode/population consistency,
snapshot locations and replay input lengths. Additional raw setup/context
and vehicle values still need semantic checks; CRC and block-size checks
alone do not prove that those game values are safe.

## Replay storage and setup consistency

The old session parser accepted a snapshot offset of zero: inside the storage
union but actually in the recorded-input region. Other accepted offsets could
be misaligned, overlap another car or select the wrong mode's layout. Missing
active snapshots were also allowed. The serialized setup could select a
different game mode/population from the checked header, and an unrestricted
`ReplayLength` could read beyond that mode's inputs.

Validation now follows the actual producers in `raceinit.c`, `racesamp.c` and
`bankrec.c`: single-player storage for modes 0/1/6, two-player storage for 2/3;
populations 1/8/2/2–4/2 respectively. Active snapshots must name their exact
typed slot; unused offsets must be -1 with no links. Only the two supported
link bits are accepted and the serialized host pointers must be null before
live pointers are restored. Recorded length is bounded by the selected input
array, preserving the producer's final reserved entry. Header frame counts
cannot exceed that length. Mode, car count, course and reverse direction must
agree with the setup, with course/direction constrained to the game tables.
The live capture path also checks canonical addresses before casting them.

The regression fails before the fix on offset zero, and now covers missing,
negative, misaligned, overlapping and wrong-layout offsets; pointer/link
data; mode/population mismatches; invalid course/direction; and length/frame
limits. All five supported modes and VS populations 2/3/4 have valid synthetic
cases. Additional cases encode and decode deliberately invalid state with
valid CRCs before the session rejects it. Capture rejects misaligned/live
input-region pointers without accessing a GPU or disc.

All 17 Release and ASan/UBSan tests pass; the changed source/test compile on
ARM64, Windows LLP64 and ILP32. Recorded TA/GP/VS files remain accepted under
ASan/UBSan. All four standard Release gameplay scenarios retain 38200 state
trace samples and 1273 identical captures. The separate hidden sanitizer
playback check loads the three recorded files and compares their first 188
frames (seven sampled captures per mode). Artifacts:
`/tmp/rr4-project-audit/replay-state/` (`before.log`, `ctest-*.log`, `check.log`,
`recorded.log`, `regress.log`, `playback.log`).

The asset checks below address car/model/emblem selections, custom TIMs and
GP stages. Vehicle/control values and context dependent on the loaded track
remain outside the storage validation above.

## Replay asset selections and custom emblems

The replay setup previously reached `LoadAllCarsStep` without checking its
model/team IDs, wheel variant or emblem selection. Wheel variant 17 passed
the session parser with valid CRCs, although `ApplyCarWheelTexturePatch`
indexes four rows of sixteen variants. A selected custom emblem could also
supply arbitrary block lengths/dimensions to `UploadCarClut`, which follows
those headers to read palette and pixel data.

The loader now checks all four slots consumed by that load script, even for
one-player replays: four team/model choices, wheel selections 0..16, emblem
selections 0..26. It validates a selected custom TIM against the editor's
fixed 64x64 4-bpp format: 16-color palette, both block lengths and dimensions,
ID and flags. VRAM origins stay unrestricted because the uploader replaces
them; unselected custom images may remain empty/stale. The modulo-20 car
parameter remains accepted across its byte range. GP cannot select presets
because its load path skips the preset bank. The garage code uses the final
preset for locked emblems; the local PAL bank confirms 25 complete images,
including index 24. Team/maker globals are bounded to their four-element
tables, GP stage to 0..7, and the retained VS lap setting to the menu's 2..9.

The new malformed-asset regression fails the old parser. Tests cover every
loaded slot, negative/out-of-range and maximum legal selections, all six
structural custom-TIM words, both custom images and their slot aliases,
overwritten origins, GP preset refusal, stage bounds and VS lap limits.
Malformed cases also pass through the encoder/decoder with valid CRCs.
All 17 Release and ASan/UBSan tests pass, and the session/test compile for
ARM64, Windows LLP64 and ILP32. The three existing recorded files remain
accepted.

Artifacts: `/tmp/rr4-project-audit/replay-assets/` (`before.log`, `ctest-*.log`,
`check.log`, `recorded.log`, and the generated boundary fixtures and playback
driver).

The actual boundary-fixture run exposed a separate bug that parser tests
cannot catch: a legal preset selection loaded from a fresh title screen
reached `UploadCarClut` with `PresetEmblemTimPtr == NULL` (UBSan,
`preset-before.log`). The link/demo script reused for replay loading assumed
the car menus had loaded that bank. Add a native replay script in the existing
`mnrows.c` dispatcher: the same operations, with `LoadCarClutStep` before
`LoadAllCarsStep`. Its initial arena/transition setup follows the existing
full-race script. The original link/demo script remains unchanged. GP still
skips the preset bank and its replay validation refuses preset selections.
All four standard Release scenarios again retain 38200 traces and 1273
captures after this dispatcher change (`regress.log`).
All six hidden/silent ASan/UBSan playback runs now complete 188 replay frames
each, with 42 exact reference captures: the three original files, TA and VS
with all custom-emblem slots selected, and TA with wheel 16/preset 26 in all
four load slots (`playback-fixed.log`). The saved VRAM restores the recorded
appearance after these alternate load selections. Both final 17-test suites
pass; all four changed source/header/test files pass the three host compile
checks. The failing preset fixture and pre-fix log are retained.

The next checks cover core vehicle indices, gear arrays and required pointer
links. Track-dependent bounds at installation and restored context/scratch
state remain open.

## First-lap timing and replay vehicle controls

`UpdateLapTiming` compared `BestLapTime` with `LapTimes[count - 2]` before
checking whether the new lap number was one. Crossing the start line with
count zero therefore read `LapTimes[-1]`. The existing incomplete array
declarations hid this from UBSan: the new focused test initially passed, but
declaring the actual ten-element arrays reproduces the out-of-bounds read
(`lap-before-bounds.log`). Check the lap number before reading a completed
lap. The test also crosses the next lap and checks that its time becomes
the new best, with the other nine entries untouched. These bounds remain
visible to future sanitizer builds.

Replay vehicle validation now distinguishes players from AI, which reuse
control fields differently. It checks player input IDs, gear index into the
seven-entry car record, lap count, both required reconstructed pointer links,
model slots and vehicle identity (including the valid lost-track sentinel).
AI checks its six-entry gear arrays, count and pending shift, a positive
steering span and a nonzero divisor when a boost is active. Player AI-only
gear/span fields legitimately remain zero and are not subjected to those
AI checks. The original parser accepts an invalid AI gear with valid CRCs;
the new regression rejects it and tests the other boundary cases and legal
pending shifts.

All 17 Release and ASan/UBSan tests pass; all five changed source/header/test
files compile for native ARM64, Windows LLP64 and ILP32. Recorded TA/GP/VS
files remain accepted by the session parser under ASan/UBSan. The four
Release gameplay scenarios retain 38200 trace samples and 1273 exact images.
The complete four-scenario ASan/UBSan run also passes with all 38200 trace
samples and 1273 images identical, and no sanitizer diagnostics
(`regress-asan.log`).
Artifacts: `/tmp/rr4-project-audit/replay-vehicles/`, including source snapshots,
before/after tests, the local recorded-field inventory, build/CTest/check logs
and gameplay regressions.

These are bounded control checks, not complete validation of a raw vehicle.
Player gear limits still need comparison with the actual loaded car record;
track-node/physics values and restored context/scratch state need checks at
the installation boundary. Other conditional arithmetic in player physics
requires review before malformed replay state can be considered covered.

## PS1 null-pointer accesses

The original reads and writes through pointers that are still null on the
console, which lands in the kernel's low RAM. The port used to answer these
with two separate copies of the first bytes of RAM (unloaded `CarRecordPtrs`
and `CarBoundsTable` slots), a dropped store (the dialogue page flag through
a null `Dialogue`, PS1 address 7) and a pointer that was always bound (the
presentation controller before `InitMotionController`). On the console all
of them share one memory, so a store through one is seen by the reads
through another. They now go to one image, `Ps1LowRamImage`
(`rr/ps1_low_ram.h`, PS1 RAM 0..0x3FF read on DuckStation): the unloaded car
slots point at its car views, the null-`Dialogue` store writes its byte 7,
and `MotionControllerView` returns the controller once
`InitMotionController` has set the PS1 pointer slot (`controllerSlot3D8`) and
the image before that.

To find every such access, the DuckStation patch logs game code (and BIOS
calls made from it) touching PS1 RAM 0..0xFFFF (`DS_LOWRAM_LOG`, with
`ds_script.py DUMMYBP=1`; `ds_lowram_report.py` groups the log). Runs of the
attract cycle (20000 fields), the TA, GP and VS menus with their races held
for 12000+ fields, and the four remaining main-menu entries found, apart from
the SDK's own kernel patches and libpad/libcard/kernel work areas:

- `SetPresentationDrawEnabled` storing 0 at 0x54 before the controller is
  bound (all front-end flows): now the image.
- `SetCarBounds` (0x0, 0x4, 0x6) and `DrawCarSpecBadgePanel` (0x1D2, 0x1F0,
  0x1F3) reading unloaded slots in the TA and VS car selects: the image.
- `AddTextSprites` reading 0x1F5 (`DrawCarSpecBadgePanel` drawing the
  name of an unloaded car record, an empty string there) in the Extra Trial
  car select: the image, whose 0x400 bytes equal DuckStation's low RAM at
  that moment.
- libsnd's `SsSeqSetVol` on a sequence that is not open (the title's mute)
  reading 0xD4 and storing the volumes at 0x94/0x96 through the null score
  pointer. PsyZ ignores the call; the volumes are 0 and those bytes stay 0 on
  the console (they are never written otherwise), so nothing differs.

The null-`Dialogue` store happens in the PAL release in one place: the Grand
Prix rules (`StepIntroScene`, mode 0/2, dialogue table R4.BIN group 1 entry
0x46, whose entry 0 has 13 pages and turns them by itself), watched to the
last page instead of skipped with Start. `UpdateDialogue` then stores 0
through `Dialogue`, which `LoadDialogueStep` sets only later in the Grand
Prix. Every entry of the other front-end tables (0x44 for the title,
options and garage, 0x45 for the card menus) has one page in all five
languages (read from the disc and on DuckStation), so options, garage,
emblem editor, records and card dialogues never reach it; the earlier runs
skipped the rules. Measured with that flow (select Grand Prix, no input
until the rules end, then the regression scenario's Grand Prix inputs with
the throttle held, retire the four attempts, game over, Time Attack's car
select and race; 24500 fields, `PSYZ_CARD_UNPLUG=2`, `ds_script.py`'s lines
at DuckStation frame - 295): DuckStation's `DS_LOWRAM_LOG` has
`UpdateDialogue` writing 0x7 (DuckStation frame 6379) and the port writes
`Ps1LowRamImage` byte 7 at the same field (vsync 6083); PS1 0x4..0x7
(0x275A0C80, then 0x005A0C80), the mode, FrameCounter, rand and
`PlayerCars`' bounds height (+0x108) are the same in every field. Nothing
reads the byte afterwards: the Grand Prix loads the player car slots, so
the Time Attack car select's `SetCarBounds` reads loaded bounds, not low
RAM (the log has no game access after the store). The byte would matter
only to a car select with unloaded slots after the rules, which the game's
flow does not allow (the Grand Prix's team select has no way back).

The other front-end entries were replayed on both too (DuckStation's
default blank card in slot 1, the port with `PSYZ_CARD_UNPLUG=2`): Options
(both controller setups, adjust screen, audio, brightness) then a Grand
Prix and its race (16000 fields), Records (lap and total records, 6500),
the Garage's emblem editor (sample load, drawing, the exit dialog) then a
Grand Prix and its race (16500), Extra Trial (trial and car select, a
race) and the DUAL SHOCK in analog mode through the Time Attack menus and
a race steered with both sticks: every field the same, the editor's
cursor and the race car's position too. The low RAM accesses were only the three above. The card menus (Garage name entry and save; Save and Load: load from the
empty card, save, load, PocketStation; a boot with the saved card) keep
the same mode, FrameCounter and rand sequences, with some card operations
a few fields off and one card request DuckStation never answers after a
save (TECH_DEBT F5). The scenarios are `tools/port/ds_compare.py`'s.

The presentation controller's fields after its script pointer (+0x64) sit at
host offsets on 64-bit hosts; only +0x54 was seen before binding.

The four regression scenarios stay identical. The VS replay loaded from a
file (H20) now matches the recorded replay in all 2663 frames, at 1x on the
CPU rasterizer and on the GPU backend, both with this change and at
63256f22: the earlier four-frame difference no longer reproduces.

## Work still required

- Four standard sanitizer gameplay scenarios now pass. Extend coverage to
  menus/garage/emblem editor and load/save/reset boundaries.
- Review remaining TECH_DEBT stability entries and SDK boundary types;
  root cross-unit globals, overlay capture, options owner types and strict
  prototypes now pass checks.
  Paced CD streaming needs longer TSan coverage and a reproducible H19 case.
- Inspect host file/disc/replay parsers, card tooling, allocations and resource
  lifecycle; exercise malformed inputs and failure paths where relevant.
  VLC bounds and malformed-frame handling now have direct tests; extend
  malformed-input coverage to other parsers and allocation failure paths.
- The 16-pixel Metal copy workaround passes image/randomized checks, with
  fallback performance cost documented. Broaden coverage to other hardware.
  Packet-layout validation and full-settings GP, TA/replay and VS checks pass;
  extend history/reset coverage to other modes and allocation failures.
- Extend protected frame-arena coverage to other hosts and modes; contiguous
  boundary writes now fault before adjacent state is damaged.
- Continue PsyZ renderer/interpolation, CD/audio and synchronization audit;
  combined CPU optimizations now pass 8× all-effects differential validation.
- Broaden performance evidence across modes, check release configurations and
  retain only improvements with measured benefit and verified behavior.
- Final builds, appropriate regressions, documentation and completion audit.

