# Performance measurements

## Target copies in levels; CLUT reuse; no sampler scaled target (2026-11-25)

Report (0.4-alpha, Windows, RTX 3080 Ti, probably D3D12, Auto at
1440p/4K): the GPU at 70% and more; an RTX 3060 slows down only on
Shooting Hoops, most on its last lap.

**Where the work went.** `PSYZ_GPU_COUNTERS=file` (new) writes one line a
present: render passes, copy passes, texture copies and their pixels,
draws, pipeline binds, submissions, blits and the CPU time of the level
ordering below. Without framebuffer fetch (D3D12, Vulkan, or
`PSYZ_GPU_FETCH=0` on Metal) a semi-transparent primitive read its
destination from a copy made before its render pass, and a pass ended at
every semi-transparent primitive overlapping anything drawn earlier in it.
In SDL's D3D12 backend every texture copy transitions its source and
destination and back (four resource barriers) and every render pass on
the scaled target (a sampler texture, at rest in the shader-resource
state) two more; each barrier makes the GPU finish the work before it.
Headless counts, Auto at 2560x1440 (6x), fetch off, per presented frame
(mean / max; before = 0.4 main):

| Scene (frames) | Render passes before -> after | Texture copies before -> after |
| --- | ---: | ---: |
| Attract: boot, movie, menus, demo (5990) | 147 / 1270 -> 67 / 671 | 178 / 1459 -> 76 / 767 |
| Time Attack race (2100) | 23 / 365 -> 9 / 189 | 106 / 434 -> 17 / 198 |
| Grand Prix race, course 1, mirror (2100) | 63 / 1188 -> 32 / 635 | 191 / 1461 -> 65 / 720 |
| Grand Prix replay with blur (500) | 301 / 922 -> 196 / 828 | 395 / 1115 -> 214 / 874 |
| Car select and course menus (500) | 506 / 693 -> 486 / 669 | 640 / 844 -> 499 / 684 |
| Versus split screen race (2400) | 46 / 337 -> 12 / 144 | 208 / 412 -> 34 / 222 |
| Shooting Hoops, start (600) | 291 / 1063 -> 164 / 661 | 447 / 1316 -> 203 / 817 |
| Shooting Hoops, last lap (3800) | 245 / 7077 -> 53 / 1321 | 399 / 7650 -> 69 / 1330 |
| Shooting Hoops, finish, fireworks (600) | 1368 / 5157 -> 179 / 869 | 1775 / 5769 -> 203 / 901 |

Shooting Hoops is the heaviest course by far: its fireworks and lights
are hundreds of overlapping semi-transparent sprites a frame (the other
courses' races stay under 100 passes on average). Counting two barriers a
pass and four a copy, its finish took about 9,800 barriers a frame on
D3D12 (up to 33,000); now about 800-1,200 (the scaled target's passes
need none). With fetch (Metal) the race frames keep 2-3 passes; their
copies (CLUT loads) go from 85-164 to 8-35 a race frame (Grand Prix 127
-> 31, versus 164 -> 19, Shooting Hoops' last lap 96 -> 18).

With fetch (Metal) a race frame takes 2-3 passes; its copies are the CLUT
cache's palette loads (one copy each, 70-130 a race frame) and the
presentation's.

**Changes (PsyZ).**

1. *Levels.* A batch's segments (a run of opaque primitives, or one
   semi-transparent primitive or quad) get a level: the highest level of
   the earlier segments whose target-pixel boxes they overlap, plus one
   for a semi-transparent one. Level L is one render pass after one set of
   copies (the level's semi-transparent boxes, merged while the union
   costs at most 8x8 VRAM pixels more: a wider merge doubled the copied
   pixels at the finish of Shooting Hoops). Every pixel's primitives keep
   their order (overlapping segments never go to a lower level), and a
   semi-transparent primitive's copy holds exactly the earlier segments on
   its pixels. The first 512 segments of a window compare boxes; later
   ones read a height map (the highest level on each VRAM pixel, 16x16
   tiles for boxes over 1024 VRAM pixels), which can only raise a level.
   Ordering cost: mean 35-96 us a frame, at most 0.5 ms in races, 3-8 ms
   in nine menu transition frames of 12,600 (24,000 draws each).
   `PSYZ_GPU_LEVELS=0` (or `Draw_SetTargetLevels(0)`) draws in order.
2. *Scaled target without SAMPLER usage.* Only drawn into and copied
   from (the presentation and the VRAM debug view copy the shown region
   first), so D3D12 and Vulkan keep it in the render-target state.
3. *CLUT rows reused across commands.* A palette loaded again reuses its
   row while nothing wrote its VRAM pixels since (stamps of 64-pixel row
   blocks set by `MarkVramDirty`, which every native VRAM write goes
   through), from the newer half of the 2048-row ring; a batch is drawn
   before a load would come round to a row it reads. Before, only loads
   with no command recorded in between were shared.

4. *Lines drawn whole by the shader.* A line (GP0 40h-5Fh) was stepped
   on the CPU and drawn as a quad for every run of pixels on one row with
   one colour: a Gouraud line, one 4-vertex quad (672 bytes of vertices)
   a pixel. Shooting Hoops' final lap switches on two fireworks objects,
   510 Gouraud lines a frame, which took the race past the 131,072-vertex
   batch. Now, where exact polygons are drawn (no widescreen, no geometry
   correction, 1:1 draw grid; PsyZ shader revision 2), dithered (not high
   colour) and inside the VRAM, a line is one quad over its box: the
   fragment shader finds the major-axis step of its VRAM pixel, steps the
   minor axis in 32.32 and the colour in 12.12 like the CPU code, and
   keeps the pixel only where the line passes. With widescreen or
   geometry correction the runs stay (their squeezed or corrected quads
   cannot be matched pixel for pixel from the fragment position).
   `PSYZ_GPU_LINES=0` / `Draw_SetExactLines(0)` draws runs only. On Shooting
   Hoops' final lap at 2560x1440 without widescreen (fetch off), vertices
   a frame: lap 21,700 -> 7,500 (max 128,500 -> 18,300), finish 24,400 ->
   9,900; line quads 3,760 -> 210 and 4,060 -> 430.

**GPU and frame time, M5 Max** (other agents' runs shared the machine;
single runs, 5-10% run-to-run spread).

Unpaced (`PSYZ_AUDIO_CLOCK=video`, no limiter: each frame takes the CPU
or the GPU time, whichever is longer), the last 600 frames of Shooting
Hoops' finish (fields ~11,450-12,050), ms a frame mean / p95:

| Setting | Before | After |
| --- | ---: | ---: |
| Auto 2560x1440 (6x), widescreen, geometry correction, fetch off | 24.21 / 82.63 | 6.25 / 17.46 |
| Auto 2560x1440, no widescreen or geometry correction (exact lines), fetch off | 4.81 / 13.08 | 2.27 / 6.24 |
| Auto 3840x2160 + antialiasing (12x), fetch off | 49.18 / 164.74 | 9.34 / 35.97 |
| Auto 2560x1440, widescreen, fetch (Metal default) | 3.88 / 4.97 | 3.75 / 4.72 |
| Auto 2560x1440, no widescreen, fetch | 1.53 / 1.95 | 1.47 / 1.82 |
| Auto 3840x2160 + antialiasing, fetch | 3.95 / 4.84 | 3.97 / 5.13 |

GPU busy per 16.7 ms window (Metal System Trace, 5 s, paced wall-clock
runs, `measure.sh`-style attach at a field), mean ms:

| Scene, setting | Before | After |
| --- | ---: | ---: |
| GP race course 1, 1440p widescreen, fetch off | 3.37 | 2.31 |
| GP race course 1, 1440p widescreen, fetch | 2.96 | 2.01 |
| Shooting Hoops finish, 1440p widescreen, fetch off | 6.93 | 4.31 |
| Shooting Hoops finish, 1440p classic, fetch off | 8.28 | 5.20 |
| Shooting Hoops finish, 1440p widescreen, Vulkan (MoltenVK) | 8.17 | 3.31 |
| GP race course 1, 1440p widescreen, Vulkan (MoltenVK) | 4.21 | 2.83 |

(The Vulkan rows are a local experiment: PsyZ on macOS was built with
SPIR-V shaders and `SDL_GPU_DRIVER=vulkan` against MoltenVK; not part of
the change. They predate the exact lines, which widescreen does not use.)
At 4K with antialiasing and fetch off the paced traces disagree (7.68
before, 10.35 and 10.53 after): there the game drops frames, so the same
field shows different moments of the race in each build; the unpaced
table above compares the same frames.

**Image identity.** `PSYZ_AUDIO_CLOCK=video` runs, captures (rendered
frame and display) every 300 fields (150 on Shooting Hoops), 2,008 per
build: attract with the opening movie and menus (12,000 fields), Time
Attack (6,600), Grand Prix on course 1 with the mirror and results
(12,600), the same with the replay blur, versus split screen (7,000), and
Grand Prix on Shooting Hoops through its last lap and finish (12,450),
each at 1x with fetch off, Auto at 2560x1440 with fetch off and on, Auto
at 3840x2160 with antialiasing and fetch off (all with widescreen and
geometry correction), and Auto at 2560x1440 without them (where the
exact lines draw) with fetch off and on: 3,012 of 3,012 byte-identical
to main (the 1,004 without widescreen again after the last change to the
lines). Frames between captures were not compared; the
PsyZ test `target_copy_levels_keep_drawing_order` compares the whole
scaled VRAM, native VRAM and output of an overlap scene and a
600-particle batch with levels on and off at 1x/2x/4x (D3D12 WARP,
Metal with and without fetch, lavapipe, MoltenVK), and
`clut_cache_reload_sees_every_vram_write` reloads a palette after an
upload, a primitive and a copy over it; `exact_lines_match_pixel_runs`
draws 1,200 lines (all directions, Gouraud and flat, semi-transparent,
leaving the drawing area, the VRAM and the 11-bit range) both ways at
1x/2x/4x (D3D12 WARP with the CI-built DXIL, Metal, MoltenVK).

**Not done, measured or argued.**

- *Fixed-function blending for the PS1 modes.* Not byte-identical on the
  RGBA8 target: it stores 5-bit colour k as round(255k/31), and
  B+F (likewise B-F, B+F/4, B/2+F/2) of two such values is not a fixed
  linear function of the stored destination: for a given F the exact
  result's code differs from the destination's by two values one apart
  depending on B, so no source value (or dual-source factor) rounds to
  it for every B. It needs another target format (more memory traffic)
  or a different encoding the presentation would convert; not done.
- *Texture-page cache (CLUT textures decoded to RGBA).* Not done: the
  fragment shader is not where the time goes (Metal with fetch, all
  shading included: 2.0 ms a race frame at 6x).
- Fireworks remain chains: each particle overlaps the one before it,
  so the finish of Shooting Hoops still takes about 180 (up to 870) passes a frame
  without fetch.
- The full present copy, the antialiasing reduction chain and the
  per-batch upload copy pass are unchanged.
- No Windows or NVIDIA GPU was measured; the D3D12 path was run only on
  WARP in CI (correctness).

## Depth buffer (2026-11-24)

The depth buffer option (docs/ARCHITECTURE.md, off by default) adds a
depth texture the size of the scaled VRAM target (D32_FLOAT on Metal and on
D3D12 WARP: 32 MB at 4x, 128 MB at 8x, 512 MB at 16x), a float per vertex
(the vertex grows by 8 bytes), the depth attachment on the batch's render
passes and one more render pass a frame (median 2 -> 3 per race frame:
the view's clear is the first depth pass's load, and segments split where
the depth mode changes). With it on the course-occlusion check of the
extended draw distance is skipped.

Headless Grand Prix, course 1, extended draw distance, autodrive,
unpaced, race frames 7400-12300 (median of 2 runs each, M5 Max shared with
other agents' runs; run-to-run spread 5-10%):

| Setting | CPU work ms off -> on | p95 off -> on |
| --- | --- | --- |
| 4x, 1920x1080, geometry correction, filtering, widescreen | 5.17 -> 4.98 | 5.90 -> 5.68 |
| 9x (8x + AA, budget 2), 2992x1878 | 5.17 -> 5.15 | 5.81 -> 5.97 |
| 16x (8x + AA, no budget), 3840x2160, frames 7400-9700 | 5.47 / 6.03 -> 5.95 / 6.17 (interval) | 6.17 / 7.30 -> 6.95 / 9.79 |

The runs are CPU-bound up to 9x; the 4x saving is the occlusion check
(0.1-0.3 ms, as measured when it landed). At 16x the GPU shows through and
the depth buffer costs about 0.1-0.5 ms a frame, within the noise of two
runs. `PSYZ_GPU_TIME` cannot attribute it: the frame's drawing is
submitted in pieces at the descriptor rollovers, so the serialised
end-of-frame submission it times is near zero either way. Whole-run CPU
time (12,400 fields): 28.5 / 27.4 s off, 27.1 / 27.0 s on at 4x; 28.1 /
28.1 s off, 28.3 / 27.9 s on at 9x.

## Native VRAM drawn on demand; command buffer rollovers (2026-11-23)

**Rollover audit.** `ReserveDrawDescriptors` submits the pending command
buffer every 256 sampler descriptors, so a frame's drawing goes to the GPU
in several submissions. Checked for anything that could drop or reorder
draws: every render pass sets its viewport, scissor, uniforms, samplers
and pipeline after the reservation (nothing carries across a submission);
the batch's vertex upload and VRAM-to-sample copy are recorded before it,
in the earlier buffer; queued CLUT loads are recorded by `AcquireCmd` into
whichever buffer comes next, before the commands that read them; there is
no depth buffer (order is submission order on one queue); every pass loads
and stores (only the swapchain blit clears, on purpose). On the presented
path the swapchain image is acquired on the present's own command buffer
after the frame's drawing was flushed, and nothing between acquisition and
submission can roll over (blits only). The differences between the hidden
target and the swapchain are the target texture, the fence (`SubmitFrame`)
and the non-waiting acquire, which can skip showing a frame (an old image
stays) but cannot show a partial one. No bug found. Test
`command_buffer_rollovers_keep_every_draw` draws a frame of 400 overlapping
opaque/semi-transparent quads in four blend modes, lines, mask-bit changes,
drawing-area changes and a quad textured from its own display buffer at
1x/2x/4x with the budget at 4 descriptors (818-1637 rollovers in two
frames) and requires the scaled VRAM, the native VRAM and the presented
output to equal those drawn with no limit. The 120 Hz Metal pop-in report
is not explained by this path; it was not reproduced here.

**What reads the native VRAM in RR4** (traces of every flush, read and
write): the replay blur (overlay sprites read the previous frame's buffer,
1 frame old); 24-bit movie display (StoreImage of the shown buffer every
frame); menu palettes at rows 506-511 inside the interlaced 320x512 menu
buffer; palette/texture uploads into pages queued menu frames read. Race
frames: nothing (Grand Prix and Time Attack fill the back buffer each
frame).

**Deferred native draws** (PsyZ `NativeDefer`): a batch whose drawing areas
lie in recently shown display buffers keeps its 1x draw queued; it is drawn
in order when a read (texture or CLUT footprint, download) or a write
(upload, fill, move, an immediate 1x draw) touches its pixels or what it
read. A full overwrite drops it; an opaque untextured rectangle covering it
drops it. Its vertices stay in their ring generation (four own buffers
instead of SDL cycling) and its palette rows in the CLUT cache until reused;
there it is dropped and its pixels marked out of date, rebuilt from the
scaled picture if read. Per 6000-12000-field run: attract 6383 deferred /
4559 drawn later; GP 12308 / 3908 (4449 dropped, 3903 overwritten); Versus
7172 / 1761. Rebuilt pixels occur only in menus (a palette upload replays
the queued menu frames); every read of them was verified.

Image check (`PSYZ_AUDIO_CLOCK=video`, captures every 15 fields of the
rendered frame and the 1280x720 output, attract 12000 / TA 6600 / GP 12600
/ GP with the replay blur toggled 12600 / Versus 7000 fields, at 1x and 4x
with geometry correction + widescreen, 6x with antialiasing + filtering):
40,380 of 40,380 captures byte-identical to before. `PSYZ_NATIVE_VERIFY=1`
(a shadow 1x VRAM drawn at once, compared at every read that drew queued
draws or rebuilt pixels, every download and one read in 61): 0 differing
pixels in 297,456 reads (1.39 Gpx) at 4x and the same at 6x+AA.

Not landed: presenting without the copy of the shown region (when not
antialiasing). The nearest blit from the big VRAM texture rounds a texel
boundary differently: one 247-pixel row differed on the Time Attack loading
screen at 1x (1280x720). A scaled VRAM only as wide as the drawn/displayed
area (512 of 1024 columns in RR4) showed no measurable GPU difference on
the M5 Max (would halve memory again); not landed.

Throughput, unpaced Time Attack race frames (present-log ms per frame,
median of 3 runs, M5 Max shared with other agents' runs; spread between
runs 10-30%): 1920x1080 Auto 5x 2.10 -> 1.48, 1280x720 Auto 3x 1.51 ->
1.28, 3456x2234 Auto+AA (budget 2) 3.00 -> 2.57. With `PSYZ_NATIVE_DEFER=0`
the new build measured within noise of the deferred one (1.49 / 1.30 /
2.59), so these runs cannot attribute the difference; an earlier
serialised `PSYZ_GPU_TIME` comparison of the same race with the 1x pass
skipped outright gave 0.72 -> 0.60 ms GPU per frame at 1080p and 0.67 ->
0.56 at 720p, and 12% less main-thread CPU. CPU user time of the runs:
3.14 -> 2.53 s (1080p), 2.25 -> 2.23 s (720p), 3.01 -> 2.85 s (4K).

## Output-sized resolution, render budget, batched palette loads (2026-11-23)

Report: a Mac (Apple Silicon, 120 Hz, Metal) mostly at 59.8 FPS with dips
to 43-44; its log shows `internal resolution set to 16x (16384x8192)`:
the saved 8x with antialiasing doubled to a 16x scaled VRAM and a
4096x3840 picture for the window.

Changes (PsyZ unless noted):

1. **Auto resolution** (`PSYZ_INTERNAL_RES_AUTO`, RR4's new default
   "Auto (window)"): the whole scale that draws a 240-line picture at
   least at the output's height, the 16:9 box of the window's drawable size
   (1080p: 5x, 4K / 3456x2234: 9x, 720p: 3x). Chosen again 250 ms after the
   drawable size changes.
2. **Render budget** (`Psyz_VideoSetRenderBudget`, RR4 sets 2): the scale
   drawn at (antialiasing doubles the shown one) is lowered until the
   picture has at most twice the output's pixels, never below the
   output-matching scale and at most 12x. It applies to fixed scales too,
   so the reporter's saved 8x + antialiasing draws 11x at 3456x2234 and 6x
   at 1080p. Diagnostics' `RR4_INTERNAL_RES=n` draws exactly n (no budget);
   `RR4_INTERNAL_RES=auto` and `RR4_RENDER_BUDGET=x` reproduce the game.
3. **Palette (CLUT cache) loads**: each load was its own copy pass; a race
   frame loads a few dozen palettes some hundred times and on Metal every
   texture copy is a compute dispatch (1.5 ms of a race frame's GPU time,
   independent of the resolution). Loads are now queued and recorded in
   one copy pass before the next command, and a palette loaded again
   before any other command was recorded reuses the row loaded first.
   Captures of boot, menus and a Time Attack race at 4x are byte-identical
   to before (6 frames, deterministic `PSYZ_AUDIO_CLOCK=video` runs).

GPU time per frame, Metal System Trace (`xctrace`, template "Metal System
Trace", 5 s of a Time Attack race at Helter Skelter, union of the game's
GPU intervals per 16.72 ms window), M5 Max, hidden renderer
(`PSYZ_HEADLESS=gpu PSYZ_HEADLESS_PACED=1`, `PSYZ_HEADLESS_SIZE` for the
output), geometry correction, filtering, widescreen:

| Output | Setting | Drawn at | Before: mean / p99 / max ms | After: mean / p99 / max ms |
| --- | --- | ---: | ---: | ---: |
| 3456x2234 (reporter's fullscreen class) | 8x + AA | 16x -> 11x | 5.53 / 9.11 / 9.95 | 3.15 / 6.30 / 6.58 |
| 3456x2234 | Auto, no AA | 9x | - | 1.78 / 3.37 / 4.18 |
| 1920x1080 | 8x + AA | 16x -> 6x | 5.83 / 9.04 / 9.84 | 1.00 / 2.60 / 2.72 |
| 1280x720 (low end) | Auto, no AA | 3x | - | 0.32 / 0.96 / 1.05 |

(The 3456x2234 8x + AA "after" with the old palette copies measured 4.53
/ 8.65 / 9.09 ms: the palette batching alone is ~0.9 ms a frame there.)

Presented-image intervals, paced hidden runs of the Grand Prix script
(below), fields 0-10500, race = mode 1/47 from its 300th field:

| Run | Race: median / p99 / max ms, intervals > 20 ms | GPU per frame (PSYZ_GPU_TIME, serialised) median / p99 / max |
| --- | --- | --- |
| 1920x1080, 8x + AA, before | 16.72 / 33.90 / 184.5, 173 (with PSYZ_GPU_TIME) | 6.07 / 11.48 / 15.60 |
| 1920x1080, same settings, after (6x) | 16.72 / 17.27 / 17.45, 0 (with PSYZ_GPU_TIME) | 3.27 / 4.48 / 4.81 |
| 1280x720, Auto, after (3x) | 16.72 / 17.17 / 17.36, 0 | 1.80 / 2.70 / 2.99 |
| 3456x2234, 8x + AA, before, no GPU timing | 16.72 / 16.79 / 33.45, 2 | - |
| 3456x2234, same settings, after, no GPU timing | 16.72 / 16.79 / 33.42, 1 | - |

`PSYZ_GPU_TIME=file` waits for each submission (drawing at the blank,
then presentation), so its times include the wake-up latency and it
serialises the CPU and GPU: on its own the M5 Max keeps 60 FPS in the race
at every setting (last two rows); the serialised runs show how close a
slower GPU would come to the 16.7 ms field. The menus' and boot's longer
intervals (33/50 ms) are the 25 fps movies' 2-3 field pulldown and loading,
the same at every setting. CPU: the main thread's own work in the race is
a few ms a frame (`sample`); the rest is the limiter's wait and the
emulated-timing polls (VSync, kernel clock), which another change removes.

Not changed, measured: the presentation (copy of the shown region, the
antialiasing resolve and the reductions) costs 0.3-1.0 ms; each batch is
still drawn twice (native 1x VRAM for read-backs and texture feedback,
then the scaled target); the scaled VRAM is the whole 1024x512 VRAM at the
scale (two RGBA8 textures: 288 MiB each at 12x, 512 MiB each at the old
16x), not only the display buffers. Not measured: Windows/Linux GPUs (target-copy
semi-transparency instead of Metal framebuffer fetch).


## Frame interpolation removed (2026-11-22)

The 60 / 120 FPS presentation (in-between pictures guessed from the PS1 draw
lists) is gone; every game frame is presented once at its field. Paced
hidden runs (`PSYZ_HEADLESS=gpu PSYZ_HEADLESS_PACED=1 PSYZ_AUDIO_CLOCK=wall`,
8x, Original draw distance, geometry correction, filtering, widescreen),
`PSYZ_PRESENT_LOG` intervals; 0.3 with 60 FPS selected vs. this change.
Single runs on the M5 Max while the user was playing the game on it.

| Section | 0.3, 60 FPS | This change |
| --- | --- | --- |
| Opening movie (25 fps, fields 590-2000) | median 20 images/s; intervals 16-368 ms (33, 50, 83, 166, 200 ms: movie frames shown late or skipped) | 691 of 706 intervals 38-43 ms (every second field), 2 at 58 ms, 1 at 80 ms |
| Main menu (1/4) | 60 images/s | 49.7 images/s (every field), 1% low 41-49, max 20-32 ms |
| Time Attack race at 8x (1/53, 1619 frames) | - | every second field (25 fps PAL), 13 frames late (54-64 ms) |

The 0.3 60 FPS outputs of screens drawn without widescreen (menus, title,
garage, every 2D screen) were rendered into a texture of the buffer's
shape (320x256) and then shown at the display aspect, so they were
letterboxed twice: squashed vertically, with bars (in Classic too). Now they
are presented directly, as in Classic, pillarboxed at the display aspect;
the race view fills 16:9. Captures: `RR4_CAPTURE_DISPLAY=1`.

Still open: the 6 s without a presented image between the boot logo and the
opening movie (fields 280-290) is the movie's start waiting on the emulated
CD drive for its first frame (`ovl668_start_stream_playback` →
`StGetNext` → `Psyz_CdPollDrive`), as on the console; the window does not
process events meanwhile.

## Issue #5: 60 FPS at 4x with the extended draw distance (2026-11-22)

Report (0.2-alpha, Windows/D3D12, 4x, 60 FPS on a 60 Hz monitor): slower
than 0.1, Extended draw distance "breaks speeds", interpolation worse; the
log shows `midpoint=0.0` in nearly every window and "interpolation table
pairs full ... entries dropped".

Reproduced here on the hidden Metal renderer, 4x, geometry correction,
interpolation, 60 FPS presentation, the GP script below, race fields
8200-9400 (600 game frames), unpaced (`PSYZ_HEADLESS=gpu`):

| Build | Original (s) | Extended (s) | Midpoints in the slice | Game frames Orig / Ext |
| --- | ---: | ---: | ---: | ---: |
| 0.2 | 16.43 | crashed (bus error at field ~8800; ~4 s per 100 fields before) | 0 | 600 / - |
| main before | 6.41 | 15.52 | 597 / 591 | 600 / 594 |
| this change | 4.44 | 8.52 | 597 / 597 | 600 / 600 |

0.2 also reports pairs/tags full at 4x Extended; main before and after do
not (main had already replaced 0.2's midpoint presentation, which presented
no midpoints in this race at all: the reporter's `midpoint=0.0`).

Causes and fixes (PsyZ unless noted):

1. **Packet address hash** (`InterpHash`): `(n >> 2) ^ (n >> 16) ^ (n >> 30)`
   kept consecutive words in consecutive slots, so the 4 MiB packet arenas
   (16 table widths) and the GPU queue folded into long primary clusters.
   `GPU_Enqueue` looks up every queued word (`Psyz_GpuCopyInterpolationPacket`),
   mostly misses, and each miss walked a cluster: **56.7% of the main
   thread** at field 8400 with Extended. Fibonacci hashing of the word
   offset (still ASLR-independent) brings it to 5-6%.
2. **Junction search on worker threads**: `InterpSnapJunctions` looks up each
   paired vertex on its own (its midpoint vertex and motion-field entries
   are its alone), so the vertices run on a small pool (`PSYZ_INTERP_THREADS`,
   default logical cores - 2, at most 8; each worker has its own query
   cache). Outline placement (`InterpFinishLines`) runs there too. The
   ghost/new-face placement (`InterpFinishPending`) stays sequential: each
   one may follow one placed before it.
3. **Bounding-box rejection** in the cached junction scan: each packed edge
   reference keeps its pose-0 box grown by the widest reach; a vertex outside
   it is skipped before the edge record is touched (`PSYZ_INTERP_EDGE_BOX=0`
   disables). Exact (a vertex on an edge lies in its box grown by its
   distance); a small gain on its own.
4. **Game speed independent of the draw distance** (game + PsyZ): the CPU
   cost model already held the added views out, but the emulated GPU still
   timed their packets, and the mirror's views past the PS1's cut were
   charged as PS1 work. Extended lost 6 game frames in 1200 fields against
   Original. The held ranges now go to `Psyz_GpuTimingHostPackets` (no GPU
   time for them on the offline clock), and the mirror's extra views are host
   views. Original and Extended now have identical mode/frame traces over the
   whole 9601-field run. On the real-time clock (interactive play) the cost
   model is off anyway: there only the host's own speed matters.
5. **Headroom**: `INTERP_PAIRS` 16384 -> 32768, maps 65536 -> 131072 (0.2's
   "pairs full"; the ring search for a free pair also gets shorter). The
   "GPU queue full" info line (every overflowing list, hundreds in the
   reporter's log) is logged once; `RR4_PACKET_STATS` is read once.

Gates: ctest 17/17, `tools/port/check.sh` 0 failing (layout sweep skipped,
no decompilation tree next to the worktree), PsyZ ztest 431/0/18,
`regress.sh check` against 9668f273 all identical, all 35 `ds_compare`
scenarios byte-identical port traces and reports to 9668f273 (Original
settings are untouched: no presentation rate, no host views).

Exactness: a per-sync checksum of every midpoint vertex (temporary
instrumentation) is identical between main before and after (Original, 3623
syncs) and between threaded and single-threaded, box on/off (Extended, 3703
syncs). `PSYZ_INTERP_TREE_VERIFY=1 PSYZ_INTERP_QUERY_VERIFY=1` (every query
against the unfiltered scan, on the workers) completes 9400 Extended fields.
Captured 60 FPS midpoints are not comparable run to run (their sample
fractions follow the host clock, also on main before).

`PSYZ_PRESENT_LOG=file` writes one line per presented image (performance
counter ms, generated flag) to measure the output cadence, which the
field-rate frame log cannot show.

6. **Presentation polls in the long stretches** (PsyZ): 60 FPS outputs are
   presented when the main thread polls (the limiter's wait, the horizon
   submission, the junction search). A poll-gap trace (`POLLGAP`, temporary)
   at 4x Extended widescreen found 10-39 ms without a poll in `GPU_Enqueue`
   (the frame's whole list dispatch after VSync) and after the junction
   search (pending placement, the midpoint's draw): the due output went out
   late or its deadline was skipped. `GPU_Enqueue` now polls every 256 list
   nodes, `InterpFinishPending` every 64 primitives, and the midpoint sync
   once more before drawing (polls present only an output already complete,
   as the existing ones). No effect without a presentation rate (offline
   runs, regress).

Real-time measurement: `PSYZ_HEADLESS=gpu PSYZ_HEADLESS_PACED=1
PSYZ_AUDIO_CLOCK=wall` (the frame limiter and the wall clock of interactive
play, no window), 4x, geometry correction, interpolation, **widescreen**,
60 FPS, the same race fields 8200-9400 (24.1 s), intervals between
presented images from `PSYZ_PRESENT_LOG` (1% low: 1000 / mean of the worst
1% intervals; min: the longest interval):

| Draw distance / build | Presented/s | Median | 1% low | Min | Intervals > 25 ms | Game frames (600 = full speed) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Extended, main before | 59.53 | 60.0 | 26.7 | 18.8 | 22 | 597 |
| Extended, this change | 59.99 | 59.9 | 50.7 | 45.4 | 0 | 597 |
| Original, main before | 59.65 | 59.8 | 29.7 | 27.0 | 86 | 600 |
| Original, this change | 59.99 | 60.0 | 52.1 | 48.4 | 0 | 600 |

(Single runs, M5 Max, other agents' runs on the machine; "this change"
Original is the build with the presentation polls.) At real time the game
frames are the host's: Extended still loses 3 of 600 frames to field
overruns here (0.5%); a slower CPU loses more, which is the reporter's
"Extended breaks speeds" (the game, like the PS1, waits for the next field
when a frame overruns, so the race runs slower than real time).


Not measured: Windows/D3D12 (target-copy blending, its swapchain and
timer). The 60 FPS path does not use the "VSync forced ON/OFF" branches the
reporter's 0.2 log shows (`ConfigureVSync` returns early with a presentation
rate): with VSync Auto/On it presents VSYNC images paced by the limiter's
output deadlines, sleeping at most 1 ms at a time and polling presentation
in between, so a coarse Windows timer cannot delay a deadline by more than
its sleep. Remaining main-thread work in the heaviest Extended frames is the
game's horizon submission, PsyZ command dispatch and the sequential pending
placement.

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
