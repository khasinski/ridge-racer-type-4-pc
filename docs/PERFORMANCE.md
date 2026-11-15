# Performance measurements

## Independent 60 / 120 FPS presentation (2026-11-15)

Graphics now exposes **Frame rate: Original / 60 FPS / 120 FPS**, independently
of **Game timing: PAL / NTSC**. The old interpolation switch generated one
midpoint per race frame and followed the native field clock (~49.75 Hz PAL);
it could not produce 60 or 120 distinct positions per second in PAL.

The new path keeps simulation, audio and VBlank timing original. It generates
up to five geometry samples per native frame at independent presentation
fractions, preserving view-space depth interpolation and T-junction edges.
It reuses the existing pairing, motion-field and CLUT/VRAM split handling.
Twelve cropped output slots add one original game frame of display latency.
Cooperative presentation during the course/model CPU passes and interpolation
solver avoids missing deadlines while the next frame is prepared. Repeated
output images do not count as newly generated geometry in the FPS log.

On this M5 Max, PAL, hidden Metal framebuffer fetch, 8× output with 4× SSAA
(16× physical rendering), geometry correction, filtering, widescreen and
extended draw distance:

- Short GP replay: 60 FPS mode median 60.0 presentations/s; 120 FPS mode
  median 119.6. Both counters confirm distinct generated samples.
- Full scripted GP through field 9601, heavy section after field 8700:
  median **120.0 presentations/s**, **119.0 new generated samples/s** across
  36 one-second windows. Two windows include diagnostic image readbacks;
  excluding those, average presentation rate is 119.48 and the minimum is
  111.0. Including captures the minimum is 93.0. This is not a locked-120
  claim, nor a Windows measurement.
- Peak physical footprint in that full run: 5331 MB (6000 MB cap).
- The hidden renderer now completes the same final display pass into a
  1280×720 offscreen target. These numbers include output rendering, not a
  physical monitor's scanout. Older headless measurements omitted this pass.

Artifacts: `/tmp/rr4-output-cadence/long-120`, `race-60`, `race-120`.
`RR4_PRESENTATION_FPS=0|60|120` selects the rate for diagnostics.
`RR4_CAPTURE_SAMPLES=1` captures each generated sample at `RR4_CAPTURE_AT`
and logs its fraction, so repeated source frames cannot masquerade as 120 FPS.

Two visual fixes accompany the output clock:

- Extended horizon projection retains full transformed coordinates and
  depth instead of saturating view X/Y at ±32768 and Z at 65535. It is scoped
  to course horizon submission; the original projection remains the default.
  A 61-angle camera-yaw test checks precise positions and depth out to 120000
  units. The before/after GP replay has identical native game-state traces;
  changed pixels are in the distant projected geometry.
- With AA enabled, the final image is reduced in steps of at most 2× before
  the window resolve. A single nearest/bilinear reduction from 8× skipped
  detail and made distant texels flash. In the final-output alternating-stripe
  regression, the old resolve has mean squared phase change 32946; the new
  resolve has 147 (average colour 127). The old path fails, the new passes.
  Native VRAM and texture content are unchanged.

Validation: 17 root tests in Release and ASan/UBSan; 228 GPU/dither/GTE tests
in Release and ASan/UBSan; native, LLP64 and ILP32 compile checks of the changed
game modules. Tests cover both PAL and NTSC sample positions, native VRAM
preservation, settings persistence and legacy-boolean migration. A hidden
settings-panel run cycles Original → 60 → 120 → Original → 60 at 8×; its
paused VBlank count remains 28 from opening through closing.

## Heavy-race profile and rejected field-search experiments (2026-11-15)

A fresh profile keeps 8× output, full 16× SSAA, geometry correction,
filtering, interpolation, widescreen and extended visibility on the hidden
Metal framebuffer-fetch path. There is no limiter or capture in the timed
runs. The short racing window at fields 8100–8500 does not represent the
later, substantially more expensive part of the same GP script. Extend the
performance comparison through field 9600 before retaining a change.

Three-second main-thread samples of the current executable, taken near
fields 8100 and 9300 respectively:

| Sample | Main-thread samples | `InterpSelectJunction` | `InterpFieldMotion` |
| --- | ---: | ---: | ---: |
| Early race | 1975 | 399 | 305 |
| Later race | 2450 | 1465 | 786 |

The two searches occupy 91.9% of the later sampled main-thread stacks.
This identifies CPU search work in this particular scene; it is not a
whole-run percentage, GPU utilization measurement or Windows result. The
late profiling run completes all 9601 fields. The early profiling run was
intentionally stopped after its sample; it is not a completed regression.

The baseline also reports a CLUT-exhaustion split at game frame 7558. The
late profile includes interpolation calls from `ClutCacheUpdate` as well as
normal presentation. The previous full-settings capture reference already
contains that warning. Earlier claims that no tested run splits were too
broad; `TECH_DEBT.md` now records this case. No cache capacity, interpolation
admission or image-quality setting was changed in these experiments.

Three local field-search changes were tested and removed:

| Variant / race window | Pair 1: baseline → candidate (s) | Pair 2: baseline → candidate (s) |
| --- | ---: | ---: |
| Return on squared distance zero, 8100–8500 | 3.483 → 3.429 | 2.757 → 2.943 |
| Reuse queries within one primitive, 8100–8500 | 2.726 → 2.661 | 2.932 → 2.846 |
| Reuse queries within one primitive, 8100–9600 | 60.903 → 61.190 | 70.092 → 69.298 |
| Distance rejection before depth rejection, 8100–9600 | 62.342 → 91.056 | 116.961 → 60.348 |

Each comparison runs baseline, candidate, candidate, baseline. Unrelated
heavy CPU work was observed during the distance-first comparison; its
large, inconsistent differences cannot establish either a speedup or a
regression. It also preserves all 265733 differential query results under
ASan/UBSan, but was removed because its performance benefit is unproven.
Host load was not controlled, so small differences in these trials must
not be treated as statistically established improvements. Query reuse
looked 2–3% faster in the short window but did not demonstrate a repeatable
benefit in the longer one. The field is extended between pending primitives,
so a wider cache would need invalidation; the tested reuse deliberately
stayed inside the first loop of one primitive, before any entries are added.

A sanitizer harness using the actual before/after helpers checks 265733
field queries (131849 zero-distance matches) for the early-return variant.
A second harness checks 135172 primitive evaluations for query reuse,
including misses, depth/view differences, weighted accumulation, disabled
fields, non-finite inputs and a stale frame. Results are identical; the
reuse harness reduces query calls from 458444 to 329047. Those synthetic
counts do not establish a game throughput improvement.

Artifacts, copied source variants, saved baseline executable and drivers:
`/tmp/rr4-project-audit/field-search/`. The runtime source was restored to
its pre-experiment contents. All sixteen timed runs retain identical game
and interpolation traces for their common fields (85 samples in each short
run, 96 in each long run). Captures were not run for the rejected variants;
trace equality and the numeric harnesses are not visual regression results.
These trials establish no new performance gain.

## Conservative interpolation edge rejection (2026-11-15)

The junction search now rejects points outside a segment or clearly too far
from its line before doing floating-point divisions. The cross-product test
has a conservative rounding margin; every surviving candidate still uses
the original projection, distance, depth and score comparisons. Iteration
order and tie-breaking stay unchanged. No image-quality setting changes.
The filter is enabled by default; `PSYZ_INTERP_EDGE_REJECT=0` disables it.
No additional per-edge storage is needed.

Two sequential comparisons against the saved executable from before this
change, reversing run order for the second pair. M5 Max, hidden Metal with
framebuffer fetch, 8× output, full 16× SSAA and all enhancements, no limiter
or captures. The race slice is fields 8100–8500 of the 8501-field GP script.

| Run order | Original race (s) | Optimized race (s) | Race time reduction | Original whole run (s) | Optimized whole run (s) |
| --- | ---: | ---: | ---: | ---: | ---: |
| Original, optimized | 5.143 | 4.451 | 13.5% | 35.263 | 31.275 |
| Optimized, original | 7.427 | 6.542 | 11.9% | 41.553 | 37.180 |

Absolute times varied substantially, so these establish a local paired
improvement, not a universal percentage, displayed FPS or Windows result.
All four runs retain 85 identical game/interpolation trace samples. Logs:
`/tmp/rr4-project-audit/edge-line-timing.log`, `edge-line-repeat.log` and
`edge-line-timing-compare.log`. A preceding depth-only rejection experiment
had no repeatable benefit and was removed (`edge-reject-timing.log`).

With query and tree differential verification enabled, every junction query
is checked against the original unculled search. GP (9601 fields) retains
six exact images and all 32 sampled traces; Time Attack/replay/retry (6600
fields) retains six images, two held-frame markers and all 66 sampled traces.
Neither run reports a table overflow or a differing selected edge. A local
ASan/UBSan numeric harness using the actual helper from the source checks
two million random/boundary cases: 1526318 accepted by the original tests,
222379 early rejections, zero false rejections. All 87 GPU/dither tests pass
in Release and under ASan/UBSan (the latter with the filter on by default), and
all 16 Release port tests pass. A default-enabled 7000-field VS differential
run retains all 70 sampled game-state traces against the native reference,
with no table overflows; its field-6600 midpoint was visually inspected.
See `STABILITY_AUDIT.md` for the continuing wider audit and artifacts.

## Interpolation history retention (2026-11-15)

Pair allocation now searches the whole ring before evicting a live entry,
and ghost-history banks grow on demand to the bound implied by the pair
count. The full 9601-field 8×/full-AA/all-effects GP run retains all six
reference images and all 32 sampled traces, with no pair/store/order overflow.
Its peak history is 46923 vertices versus the old 32768-vertex limit. Focused
old/new tests reproduce both defects; Release and ASan/UBSan pass the fixes,
and all 87 GPU/dither tests pass. See `STABILITY_AUDIT.md` for artifacts.

This establishes correctness, not a new speedup. A one-second CPU sample
of the capture run finds `InterpSelectJunction` at the top of 532 of 760
main-thread samples and `InterpFieldMotion` in another 82. This identifies
CPU search work for the next profile; it is not a whole-run percentage or
GPU timing. The run overlapped other validation and is not a controlled
throughput measurement (`interp-history-gpu-sample.txt`). Keep 8× and all
effects for the next isolated timing comparison.

## Packet address maps and concurrency follow-up (2026-11-15)

Packet tag lookup previously stopped after four address collisions and could
replace a live tag. With guarded packet arenas this changed midpoint decisions
without changing game state. Continue probing to an empty slot and periodically
compact obsolete entries before the fixed table fills. The reusable scratch
buffer is 1.5 MiB on a 64-bit host and is freed with the GPU device. Failed
scratch allocation retains full probing; actual table exhaustion still has
the existing counted fallback.

A randomized 72000-address churn test fails the old lookup: the last checked
frame retains 5743/6000 tags and 5579/6000 pairs. The corrected lookup retains
all 6000 tags and all pairs after the initial frame. All 85 GPU/dither tests
pass. Two full 9601-field game runs with different protected arena layouts
have identical six images and all interpolation traces. At field 9600 both
have presented 1180 midpoints; the preceding reference had 909. Restoring
lost tags changes which midpoints are admitted, so the old midpoint captures
are not an unchanged-output reference for this correctness fix. Native CPU
state/capture regressions remain identical (38200 fields, 1273 images), in
Release and ASan/UBSan. See `STABILITY_AUDIT.md` for the CD/DMA fixes and TSan
coverage included in the current build.

Current single unpaced measurement, same M5 Max 8×/full-AA/all-effects GP
scenario and default Metal fetch, no captures: **29.348 s whole run,
3.107 s for fields 8100–8500**. These are workload times, not displayed FPS
or Windows results. The earlier 24.139/2.756-second result below predates
these correctness changes; this follow-up does not establish a speedup.
Pair-ring and ghost-history warnings in this earlier run are addressed by
the retention fixes above. Artifacts: `/tmp/rr4-project-audit/interp-map-*` and
`interp-layout-*`.

## Combined interpolation, default Metal path (2026-11-15)

Same 8501-field scenario, 8× output with every enhancement and full 16× SSAA,
M5 Max, hidden Metal/dummy audio, no limiter. Same binary for the two fetch
runs; only CPU query-cache/tree flags change. These are single sequential
measurements, not Windows measurements or displayed FPS.

| Rendering / CPU configuration | Whole run (s) | Race fields 8100–8500 (s) |
| --- | ---: | ---: |
| Target copies, original rectangles | 94.353 | 10.289 |
| Target copies, 16-pixel Metal workaround | 121.350 | 8.716 |
| Default Apple framebuffer fetch, CPU experiments off | 29.351 | 5.989 |
| Framebuffer fetch, query cache and depth groups on | 24.139 | 2.756 |

The combined CPU optimizations reduce this race slice by 54.0% (2.17×
throughput); whole-run reduction is 17.8%. Framebuffer fetch is already the
Apple-family default; earlier forced-copy tests exercise the fallback path.
The copy workaround is a correctness fix with a visible whole-run cost, not
a speedup. It is limited to Apple/Metal; Windows/Linux retain exact copy
rectangles. Racing-slice variability means the two copy runs do not establish
a performance advantage for rounding. The 16-pixel workaround passes 15
strengthened randomized tests and three full 84-test GPU/dither suites.

Combined per-vertex differential verification completed all 9601 fields
without a differing edge pointer. Six images, held markers and full traces
match the current root-state reference exactly (`combined-verified.log`).
Both CPU optimizations are now enabled by default; use
`PSYZ_INTERP_QUERY_CACHE=0` or `PSYZ_INTERP_EDGE_TREE=0` to disable either for
comparison. Cheaper AA remains opt-in because it changes edge smoothing. The final default-enabled build also
passes all 84 GPU/dither tests and an 8501-field run without optimization
flags matches the explicit-flags trace. The port's 14 tests pass in Release
and ASan+UBSan; four sanitizer gameplay scenarios match 38200 traces and
1273 captures (native rendering).

## Three follow-up experiments (2026-11-15)

All runs keep 8× output, interpolation, AA, geometry correction, filtering,
widescreen and extended visibility enabled. Metal is hidden, audio is dummy,
GPU fetch is disabled to exercise the target-copy renderer. Timings below are
unpaced racing throughput, not displayed FPS or measurements on Windows.

1. Shared junction queries (`PSYZ_INTERP_QUERY_CACHE=1`): cache exact pose,
   depth and viewport queries within a junction batch. Keep the two best
   results belonging to distinct faces so each caller can exclude its own
   face. Preserve strict score comparisons and first-winner tie order.
   A bounded four-probe table replaces entries on collision; misses recompute
   the full search. The cache is heap allocated and does not move game BSS.
   `PSYZ_INTERP_QUERY_VERIFY=1` compares every cached result against the
   original search and aborts on any differing edge pointer.
2. Hierarchical depth rejection (`PSYZ_INTERP_EDGE_TREE=1`): preserve bucket
   order and build conservative depth intervals over groups of 8 and 64
   consecutive references. Reject a whole group only when its interval cannot
   contain an accepted match. Unusual/nonpositive depths disable culling.
   `PSYZ_INTERP_TREE_VERIFY=1` checks every vertex against an unculled search.
3. Cheaper SSAA (`PSYZ_AA_CHEAP=1`): render 8× output at 10× instead of 16×.
   Retain the 8× output size and native VRAM. This deliberately changes image
   quality. A 10× RGBA VRAM target is 200 MiB versus 512 MiB at 16× (61% less
   memory per target). At 1×/2× retain the existing double-resolution SSAA;
   at higher scales use output scale plus two. All three were opt-in during
   these individual experiments; the combined validation above subsequently
   enabled the two CPU optimizations by default. Cheaper AA stays opt-in.

Single sequential controlled comparison, same rebuilt executable and settings:

| Variant | Race slice (s) | Whole run (s) | Race time reduction |
| --- | ---: | ---: | ---: |
| Baseline, experimental flags off | 8.327 | 78.864 | — |
| Shared queries | 5.902 | 78.699 | 29.1% |
| Hierarchical depth groups | 6.295 | 80.977 | 24.4% |
| Cheaper SSAA | 6.569 | 35.019 | 21.1% |

These are individual variants, not cumulative gains. The much shorter whole
run with cheaper AA includes menus/loading and must not be reported as a
racing FPS improvement. No statistical confidence interval or Windows result
is established by this one comparison.

The shared-query differential run completed 9601 fields without a pointer
mismatch. All six captures, held markers and state traces are byte-identical
to the accepted reference. At interpolation frame 7500 it had 3,988,267 query
lookups, 2,903,549 hits (72.8%), and 84,048 own-face exclusions; the latter
confirms the second-face fallback was actually exercised. The validation
run took 261.01 seconds because it also executes the reference search.
Performance numbers above disable that verification overhead.

The hierarchical differential run also completed all 9601 fields without an
edge-pointer mismatch. All six captures, held markers and traces are
byte-identical. It took 398.17 seconds with the expensive per-vertex reference
check, so that elapsed time is not a performance result.

The cheaper-AA run completed all 9601 fields in 186.68 seconds with captures.
Output dimensions remain 3641×2048 and all state/interpolation traces and held
markers match. Its six images intentionally differ from 16× SSAA: mean absolute
RGB-channel error ranges from 0.238 to 0.385 on a 0–255 scale; 0.272–0.452% of
channels differ by more than 16. These are image-difference metrics, not a
perceptual quality guarantee. Full images and a 1:1 HUD crop were inspected:
geometry/layout remain aligned, while edge/text smoothing changes. Static
captures do not establish temporal shimmer quality; retain this as an opt-in
tradeoff rather than silently lowering the existing AA setting.

All 12 port tests and all 83 GPU/dithering tests pass with default settings.
The two interpolation/SSAA tests also pass with all three experimental flags
and both differential checks enabled, including the 8×/10× AA path. All 85
controlled-run trace samples match for each individual variant after removing
host timestamps. CPU optimizations should be preferred first; combinations
have not been benchmarked and the individual gains must not be added.

Artifacts and scripts for this local comparison are under
`/tmp/rr4-three-tests/`. The controlled race slice is fields 8100–8500 of an
8501-field GP run without captures. Full visual checks stop at field 9601,
comparing three originals, three midpoints and deterministic state traces.

## CPU interpolation optimization (2026-11-15)

With 8× and every enhancement enabled, improve the CPU junction matching in
the existing PsyZ interpolation module:

- Cache each edge's squared lengths and lengths once when it is queued.
  Reject points outside a segment before computing their distance, and only
  test the second pose after the first pose qualifies.
- Pack each spatial-hash bucket into contiguous edge indices, retaining its
  original linked-list order, duplicates and hash collisions. Reuse this
  index until the frame or reference count changes. Small partial batches
  retain direct traversal rather than rebuilding the whole index.
- Use standard C `fabsf` instead of the SDL dynamic-dispatch wrapper in the
  hot interpolation math. No approximate math or fast-math flags are added.

The optional cache is about 2.02 MiB of host heap memory, allocated once and
freed with the GPU device. It leaves the geometry and packet-buffer layouts
unchanged. Allocation failure falls back to uncached lengths and linked-list
traversal. `PSYZ_INTERP_EDGE_CACHE=0` exercises that path; the earlier
second-pose rejection and direct math calls remain enabled.

### Controlled racing measurement

Same full configuration as below: 8×, AA (16× actual targets), geometry
correction, filtering, interpolation, widescreen, increased draw distance and
dithering off. Hidden Metal GPU with target-copy blending and dummy audio.
Compare the pre-optimization executable against the final implementation,
without captures or concurrent tests. `PSYZ_FRAME_LOG` and a trace every 100
fields record host timestamps. Both runs stop at field 8501.

| Measurement | Before | Optimized |
| --- | ---: | ---: |
| Racing, fields 8100–8500 | 13.804 s | 8.288 s |
| Whole run, including boot/menus/loading | 88.841 s | 90.719 s |

The racing slice covers 400 fields / 200 game frames and takes **40.0% less
time (1.67× throughput)**. All 85 mode/frame/interpolation trace samples match
after removing host timestamps. This does not show a startup improvement or
establish displayed FPS on Windows. It is one controlled comparison, not a
multi-run statistical benchmark.

### Longer visual regression

The 9601-field validation with captures took 349.83 → **200.51 s** (42.7%
less elapsed time). All three original and three midpoint 3641×2048 images
are byte-identical; held-frame markers and all mode/frame/interpolation trace
lines also match. Midpoint presentation counts remain 434/582/732/838/861/909
at fields 8100/8400/8700/9000/9300/9600. The improvement does not come from
skipping interpolation or loosening its safety rules.

Intermediate measurements were about 235 s for cached lengths and 208 s after
contiguous bucket traversal. A conservative bounding-box prefilter measured
241 s and was removed. The final GPU suite passes 83/83; port tests pass
12/12; the 8×+AA midpoint regression passes with statistics off and on.
The previously observed intermittent scaled-VRAM rasterizer-test discrepancy
remains open; this interpolation change does not claim to resolve it.

A follow-up experiment combining query reuse inside six-vertex quads with
conservative per-edge depth bounds measured 210.66 s versus the 200.51 s
reference. Images and traces still matched, but no speedup was demonstrated,
so this combined variant was removed. Broader query sharing across adjacent
faces and hierarchical rejection of whole candidate groups remain potential
algorithmic improvements; their benefit has not been measured.

## 8× with every enhancement (2026-11-15)

Configuration: `RR4_INTERNAL_RES=8`,
`RR4_GRAPHICS=geometry,nodither,aa,filter,interpolation,wide`,
`RR4_DRAW_DISTANCE=1`, hidden GPU, dummy audio, `PSYZ_GPU_FETCH=0`.
The AA implementation doubles each dimension before downsampling: this
configuration actually creates **16×, 16384×8192** VRAM targets. All of these
measurements remain Metal/target-copy results, not Windows/D3D12 results.

The combined configuration exposed two bugs:

1. The 384 KiB frame packet arena overflowed into adjacent state. The
   reproduced frame emitted 403,384 bytes into 393,216 bytes of storage;
   a later physics update followed a car-record pointer overwritten by a GPU
   command. Increase each native arena to 4 MiB and check/report emitted size
   after the mode's drawing code, before GPU dispatch and the next physics
   update. `RR4_PACKET_STATS=1` reports high-water marks. The completed test
   needed **608,832 bytes**, confirming the previous capacity was insufficient.
2. `InterpStatsFrame` cleared the areas used by midpoint admission only when
   statistics logging was enabled. With logging off, unpaired geometry from
   earlier scenes accumulated and suppressed later midpoints. Reset those
   per-frame arrays on the non-logging path too. A regression test first
   draws a large unpaired scene, then checks actual midpoint pixels and
   presentation count after changing scenes. It fails before this fix and
   passes with logging both off and on, including **8× plus AA**.

Both comparison binaries include these two fixes, isolating the opaque-draw
optimization. Each completed the same 9,601-field GP scenario with three
original and three midpoint captures. Capture-enabled elapsed times were
356.19 s before and 349.83 s after. This single diagnostic pair, with capture
overhead and other correctness checks during the run, does **not** establish
a significant performance improvement for the full configuration. The earlier
31.8% result applies to the simpler 8× configuration below.

The two runs produced byte-identical **3641×2048** original and midpoint images
at fields 9000, 9300 and 9600, and identical midpoint presentation counts:

| Field | Midpoints presented (cumulative) |
| --- | ---: |
| 8100 | 434 |
| 8400 | 582 |
| 8700 | 732 |
| 9000 | 838 |
| 9300 | 861 |
| 9600 | 909 |

Fields 9000 and 9300 held the previous image under the existing safety rules;
9600 presented interpolation. Thus interpolation is actually active, rather
than merely requested. Process physical footprint, including shared GPU
memory on this Mac, peaked at 2806/2812 MB; this is not dedicated VRAM usage
on a Windows card.

A three-second racing CPU sample was dominated by `InterpMidpointSync`,
junction/edge matching, `InterpFieldMotion` and repeated square roots/absolute
values. This CPU work is the next performance target with all enhancements;
the opaque GPU draw optimization does not address it.

Validation: all 12 port tests pass; the four CPU regression scenarios retain
identical state traces and all 1,273 images. The final graphics suite passes
83/83, and midpoint regression passes with stats off/on. **An intermittent
scaled-VRAM mismatch remains open** in the randomized rasterizer test:
repeated runs exposed it in both the previous and optimized renderer. Native
VRAM matched; sampled pixels of the scaled target sometimes differed. Do not
treat one passing suite as evidence this intermittent issue is resolved.
No image tolerance or expected output was changed to hide it.

## GPU 8× renderer (2026-11-14)

The target configuration is **8×**, which uses GPU rasterization. The earlier
1× results below do not establish performance at 8×.

`DrawTargetRuns` previously drew opaque geometry twice: once for RGB, then
again for the PS1 mask bit. The existing fragment shader's pass 3 already
outputs RGB and the correct mask together. Use that pass with the existing
unblended pipeline for unchecked opaque geometry; keep destination-alpha
blending for check-mask and the existing semi-transparent path. Old shader
headers retain the two-draw fallback. This halves draw calls and fragment
work for unchecked opaque runs without changing resolution or effects.

Measurement on Apple M5 Max: Release, diagnostics enabled, 12,600-field GP
script below, `RR4_INTERNAL_RES=8`, `PSYZ_HEADLESS=gpu`, `PSYZ_GPU_FETCH=0` and
dummy audio. The log confirms 8192×4096 VRAM targets and target-copy blending.
Disabling Metal framebuffer fetch exercises the copying algorithm also used
by D3D12/Vulkan; this remains a Metal measurement, not a Windows result.
Other enhancements are off. Both binaries include the earlier 1× changes,
isolating the combined opaque color/mask draw.

| Elapsed seconds | Before | Combined opaque draw |
| --- | ---: | ---: |
| Pair 1 | 24.90 | 20.72 |
| Pair 2 | 27.30 | 20.63 |
| Pair 3 | 29.27 | 24.18 |
| Median | 27.30 | 20.72 |

Three alternating pairs measured **24.1% less elapsed time, or 31.8% greater
throughput**. Individual timings vary; these are unpaced whole-scenario
times, including menus and loading, not displayed race FPS. All 42 sampled
mode/frame trace lines match for each pair. The GPU tests now explicitly
include 8× in randomized rasterizer comparisons and reset to 1× before
creating each CPU reference.

Validation: all 12 port tests and all 83 graphics/dithering tests pass on the
hidden GPU with framebuffer fetch disabled, including randomized comparisons
at 8×. Three complete 2560×2048 GP images (fields 9000, 10800 and 12600) are
byte-identical before/after; each capture is from racing mode `1/47`.

To reproduce, add `RR4_INTERNAL_RES=8 PSYZ_GPU_FETCH=0` to the GP command
below. A Windows tester run is still required, including the actual GPU,
backend and remaining enhancement settings.

## Native 1× renderer (2026-11-14)

The default 1× renderer rasterizes polygons on the CPU and presents through
SDL GPU when a device is available. CPU-only profiling identified polygon
rasterization as the largest cost. In the hidden Metal presentation path,
RGB555-to-RGBA conversion additionally took about 28% of main-thread samples.

The earlier 1× measurements isolated two changes in PsyZ:

- `sdl3_software.c`: clip polygon destinations once per span, avoiding repeated
  per-pixel bounds checks, and fill simple opaque, flat spans directly.
- `sdl3_common.h`: convert RGB555 using a 256 KiB table of all 65,536 input
  values, preserving exact DAC rounding and bit-15 alpha. Initialize once on
  the render thread; then copy one entry per pixel. Byte arrays and `memcpy`
  avoid alignment and endian assumptions.

Resolution, effects and game timing are unchanged. Experiments with specialized
pixel loops and cached extended visibility lists were removed because they did
not improve the measured GP workload reliably.

### Complete GP workload

Baseline: release `0.2` (`ea8f5b8e`), PsyZ `c782d1ab`. Both executables used
Release with diagnostics enabled on an Apple M5 Max ARM64. Each run advances
12,600 fields, including boot, GP selection, loading and sustained racing with
the test driver. No lap-distance pokes, replay files or captures. Mode traces
confirm racing in mode `1/47`.

`PSYZ_HEADLESS=gpu` retains real Metal rendering and presentation in a hidden
window, without pacing; `SDL_AUDIODRIVER=dummy` keeps runs silent. Time the game
with `/usr/bin/time -p` **inside** `run_capped.sh`, excluding the supervisor's
exit polling delay. Three alternating before/after pairs:

| Elapsed seconds | 0.2 | Both optimizations |
| --- | ---: | ---: |
| Pair 1 | 8.45 | 6.53 |
| Pair 2 | 8.48 | 6.53 |
| Pair 3 | 8.57 | 6.62 |
| Median | 8.48 | 6.53 |

This is **23.0% less elapsed time, or 29.9% greater throughput**. A preceding
three-pair run measured 8.51 → 6.48 seconds (31.3% greater throughput).
These are unpaced workload timings, not displayed FPS or Windows results.

The conversion table alone, compared with the already optimized polygon
rasterizer, measured 8.09 → 6.59 seconds median over three pairs (22.8% greater
throughput). With CPU-only `PSYZ_HEADLESS=1`, omitting GPU uploads, the polygon
change measured 5.62 → 5.22 seconds over two GP pairs (7.7% greater throughput).
The earlier 18,000-field mixed replay/attract workload measured 14.4%; these
workloads must not be treated as interchangeable.

Reproduce with a diagnostics build and a PAL disc:

```sh
gp_pads='start@m2/4:700+4%300,start@m1/4:200+4%150,start@m0/2:100+4%150'
for mode in 6 8 10 11 12 14 16 18 34 48; do
    gp_pads="$gp_pads,circle@m1/$mode:200+4%154"
done
gp_pads="$gp_pads,circle@m0/4:200+4%154,start@m1/22:120,down@m1/43:200,circle@m1/43:300+4%154"
PSYZ_HEADLESS=gpu SDL_AUDIODRIVER=dummy \
RR4_DISC="$PAL_CUE" RR4_FRAME_LIMIT=12600 RR4_AUTODRIVE=1 \
RR4_PAD_SCRIPT="$gp_pads" RR4_TRACE=300 \
tools/port/run_capped.sh /usr/bin/time -p "$BINARY"
```

Run from a writable scratch directory using absolute paths to the executable
and supervisor. Unset `RR4_REPLAY_LOAD` and `RR4_SETTINGS_SCRIPT`. Alternate the
binaries, use identical settings and avoid concurrent game runs.

### Correctness

- All 12 port tests pass.
- Headless CPU: 75 graphics/dithering tests pass; 8 GPU-only tests skip.
- Hidden Metal GPU: all 83 graphics/dithering tests pass.
- New test `gpu::rgb555_upload_preserves_every_color_and_mask` checks all 65,536
  values in GPU-upload and native-presentation modes: exact RGB888 channels
  plus 16-bit readback, including the mask bit. RGB888 checks catch rounding
  differences that a 16-bit roundtrip hides.
- Polygon regression: attract, time attack, GP and versus have identical state
  traces across 38,200 fields and identical 1,273 captures on the CPU backend.

### Windows reports

The reported 20 FPS still needs verification on an affected Windows machine.
Record CPU/GPU, internal resolution, interpolation, draw distance and whether
the slowdown happens during racing or in menus. `PSYZ_FRAME_STATS=100` prints
periodic summaries; `PSYZ_FRAME_LOG=frames.csv` records presentation timings.
Collect the same replay and settings before and after the change. Metal
measurements cannot establish whether Windows is limited by CPU rendering,
GPU work or VSync/presentation waits. Enhanced rendering also needs separate
measurement; these results concern native 1× rendering.
