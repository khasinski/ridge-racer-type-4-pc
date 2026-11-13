# Architecture

The native target compiles and runs the original C modules from
`ridge-racer-type-4-decomp`, repaired in place under `src/main/` and
`src/overlays/`, using CMake and PsyZ (`third_party/psyz`). No parallel game
engine, historical PS1 compiler, splat step, PS1 linker script or retail
checksum check is used.

## Native build and module integration

```
src/main/ + src/overlays/ -> host C11 compiler -> original game + PsyZ
```

1. **Resident executable (`src/main/`):**
   - `system/` — main loop, mode table dispatch (`ModeFuncTable`), frame task
     scheduler, pad polling and dual-actuator rumble.
   - `data/` — `R4.BIN` archive directory, asynchronous CD/sector streaming,
     relative file (`RelocateRelativeFile`) unpacking, memory card state
     machine and save data serialization.
   - `gfx/` — double-buffered `DRAWENV`/`DISPENV`, ordering tables, GTE course
     and car model projection (`model.c`, `gtemodel.c`, `gtepoly*.c`), skybox,
     billboards, rear-view mirror and screen fades.
   - `course/` — track node splines, surface projection, collision bounds,
     visibility octants and animated track objects.
   - `car/` — player and AI vehicle physics, tire grip, slipstream, collisions,
     replay recording and articulated wheel/body transforms.
   - `camera/` — race chase, cockpit, intro, finish and replay camera splines.
   - `race/` — race state machine, standings, lap/split timing and HUD.
   - `menu/` & `ui/` — mode select, Grand Prix story/dialogue panels, save UI,
     badge rows, glyph lists and UI windows.
   - `sound/` — VAB bank loader, engine voice pitch/pan synthesis, sequence
     queue and CD-XA / stream audio control.
   - `demo/` & `effect/` — opening/attract sequences, lens flare and spark
     particles.
2. **Statically linked overlays (`src/overlays/`):**
   - In retail PS1, `R4.BIN` entries `659..668` (copies `669..672`) are read
     on demand into `CdDirectoryBuffer`, linked for `0x80114878`. In the
     native port the nine modules (`grand_prix`, `time_attack`, `versus`,
     `garage`, `records`, `options`, `extra_trial`, `serial_link`,
     `movie_playback`) are linked statically; no code is read from disc.
   - **Dispatch:** `ModeFuncTable` / `ModeGroup0..4FuncTable`
     (`src/main/system/modeinit.c`) hold real function pointers; each entry
     the PS1 resolved into the window names the `ovlNNN_` function that
     image held at that address. Resident code calls other overlay entry
     points directly by name.
   - **Binding and reset:** the five loader steps that read an overlay image
     (`ldscn1.c`, `ldscn4.c`, `trkld8.c`) first call
     `OverlayModuleBind(entry)` (`modeinit.c`, `rr/overlay_module.h`). For an
     executable entry it runs `ovlNNN_reset_module()` and the step completes
     without reading or allocating; other entries (`664`, `673`, assets) are
     read as before. Each unit that defines module state lists it in an
     `ovlNNN_<unit>_state` table; the reset captures those objects on the
     first bind (their linked values = a fresh load: initialized data, zero
     BSS) and copies them back on every later bind.
   - Overlay data is defined in C with its original values.
3. **PsyZ runtime (`third_party/psyz/`):**
   - Provides `libgpu`, `libgte`, `libspu`, `libetc`, `libpad`, `libcd`,
     `libpress` (MDEC) and SDL3 window/audio/input integration.
   - **Rasterizers:** at 1x the CPU rasterizer draws; higher internal
     resolutions, the Escape menu enhancements (geometry correction,
     dithering off, texture filtering, antialiasing) and
     `RR4_RASTERIZER=gpu` use the SDL GPU rasterizer. Both
     follow the PS1 GPU bit for bit at 1x (checked with GPUDUMP frames
     replayed in DuckStation): fill rule, 12.12 attribute stepping, line
     stepping, dithering, blending, mask bit, CLUT cache, texture windows,
     wrapping and culling. At Nx the GPU rasterizer evaluates the same rules
     at the target pixel (i, j) as the PS1 sample (i / N, j / N), so the first
     pixel of every NxN block equals the 1x pixel (`psyz/tests`:
     `sdl_gpu_rasterizer_matches_cpu_rasterizer_at_every_scale`). Only the
     other pixels of a block add detail: polygon edges and the UV/colour
     steps inside polygons (each still dithered per VRAM pixel). Sprites,
     rectangles, lines, fills and transfers cover whole blocks; read-backs
     and texture sampling use the native VRAM, which is the 1x result.
     Semi-transparent primitives blend in the shader with their destination
     pixel: on Apple GPUs (Metal) read from the colour attachment
     (framebuffer fetch), elsewhere from a copy of the target made before
     the primitive (`PSYZ_GPU_FETCH=0` forces the copies on Metal).
   - **GPU memory:** SDL keeps every buffer it cycles in (a buffer written
     while the GPU still reads it) and never frees them, so PsyZ cycles
     rarely. Batches go one after another into one vertex/index buffer and
     its transfer buffer (`RingReserve`), VRAM uploads into one upload
     buffer (`UploadMap`); only a wrap cycles them, and it submits the work
     with a fence and waits for the fence three wraps before. Presents keep
     two frames in flight (`SubmitFrame`). A waited fence is released after
     the next submit: SDL's Metal backend frees a finished command buffer
     (and its hold on the buffers it used) in a later submit, and a fence
     released before that was taken by the very next command buffer, so the
     old one never looked finished. A frame's command buffer is submitted
     every 256 descriptors on every backend. At 8x with antialiasing a race
     and its replay stay at about 2.6 GB (physical footprint, including GPU
     memory) instead of growing past 10 GB.
   - **Geometry correction:** the renderers store every projected vertex
     through `RR_GTE_STORE_SXY` / `RR_SXY_COPY` (`include/rr_asm.h`), so
     PsyZ finds the precise projection of each packet vertex by its address
     and draws it at subpixel precision with perspective-correct texturing
     (PGXP-like); the main loop starts a new generation of them per frame
     (`Psyz_PgxpNextFrame`). Culling follows the precise vertices too: NCLIP
     takes their facing (`Psyz_GteSetPgxpNclip`), so the far road's strips
     that snap to zero height are drawn instead of opening gaps, and a vertex
     the course renderer subdivides with INTPL keeps its exact position
     (`Psyz_GteSetPgxpIntpl`), so it stays on the edge of a less subdivided
     neighbour tile instead of leaving a hairline crack. With the option off
     the packets are unchanged: at Nx the original whole-pixel vertices
     leave the T-junction cracks between differently subdivided course tiles
     that fall between the PS1's samples at 1x (as in DuckStation upscaling
     without PGXP); geometry correction is the watertight mode.
   - **Widescreen 16:9 (hor+, square pixels):** the projection is
     unchanged; a full-width race view (`include/rr/widescreen.h`, set up in
     `camera/viewcam.c`) culls `WideMargin()` pixels past both screen sides,
     merges the course list of the neighbouring view octant
     (`camera/viewini.c`), anchors edge HUD elements to the 16:9 view and
     tells PsyZ per buffer that it holds a 16:9 picture
     (`Psyz_VideoSetWidescreenPicture`). PsyZ's squeeze is fitted to the
     display mode (`Psyz_VideoSetWidescreenSqueeze(0)`): the 320x256 buffer
     holds exactly the 455.1-pixel 16:9 view at square pixels (squeeze
     45/64, margin 68, HUD shift 67), so nothing is stretched. Every other frame is marked
     `Psyz_VideoSetWidescreenNative`: drawn and shown exactly as in Classic
     4:3. The 16:9 view's square pixels come from `PSYZ_ASPECT_SQUARE`, so
     those frames (and 24-bit movies) are presented at the display aspect
     instead (`Psyz_VideoSetWidescreenNativeAspect(PSYZ_ASPECT_DISPLAY)`):
     same picture aspect and scale as Classic, with bars at the sides of a
     16:9 window, and bit-identical captures. In Classic 4:3 every value is
     the original one.

   - **Frame interpolation:** a race frame takes two fields (25 fps PAL);
     with the option on PsyZ presents a midpoint between two frames at the
     blank in between (`Psyz_GpuPrepareInterpolatedBlank` in `MainLoop`),
     drawn into a separate target from the same packets with averaged
     vertices. The packets come from a linear arena, so the 3D renderers tag
     each one with a stable identity (`include/rr/frame_interp.h`): the view,
     the object (`InterpObject`: course, scenery, car and part, track object
     placement) and a key from the renderer (the face record, the contents of
     a course tile record with its grid cell and subdivision levels). A
     packet pairs with the same identity in the previous frame; untagged
     packets (HUD, 2D) keep their pose. Culling reaches past the view while
     interpolating (`InterpMargin`), tiles nearer than the projection plane
     and the previous level of detail of a re-subdivided tile or course model
     face are drawn into the midpoint only, the new level into the frame
     only, and the midpoint target starts as the previous frame so nothing
     uncovered shows an older picture. Tiles (and grid pieces of near tiles)
     that the facing test drops while a corner is nearer than the projection
     plane are passing under the camera and are drawn into the midpoint only
     too, tagged `INTERP_NO_POSE`: their coordinates are no projection, so
     PsyZ continues the last motion of the pose the frame before drew.
     Other midpoint-only geometry pairs only with a pose the frame before
     drew. A camera jump (`InterpViewCamera`) or a mode
     change starts new identities, also for the level-of-detail history
     (`InterpViewGeneration`). PsyZ places a paired vertex at the projection
     of the midpoint of its two view-space positions (the screen positions
     weighted by their PGXP depths), so subdivided pieces stay on their
     neighbours' edges and near geometry moves as the camera does. An
     identity the frame before drew and this frame leaves out (a tile
     leaving the drawn range, a model switching to another representation,
     a culled object) is drawn into the midpoint as a ghost, right after
     the primitive drawn before it in that frame; a new identity where
     ghosts go stays out of the midpoint. Geometry with one pose only (a
     ghost, a tile passing under the camera, a newly visible face) moves as
     the paired vertices next to it at a like depth did, joined to them
     where it shares their vertex: its own continued motion does not know
     how the view moved this frame (a camera jolt against a wall), and a
     new face's pose in this frame is not where its neighbours are in the
     midpoint. The midpoint keeps its geometry for the whole frame and
     places these vertices, the T-junctions and the outlines below once
     the frame is complete (`InterpMidpointSync`, at
     `Psyz_GpuPrepareInterpolatedBlank`), then draws it, so how the
     frame's primitives were split into batches (drawing-state changes,
     texture feedback, a full buffer) makes no difference; it draws
     earlier only when something would change what it samples or draws
     over (a VRAM write to its texture pages or drawing area, a full CLUT
     cache or store), and `PSYZ_INTERP_STATS` counts such splits. A
     vertex of a finer subdivision that lies on an edge of a coarser
     neighbour in both frames, to the precision of the projection (a
     T-junction), is put on
     that edge's midpoint, so the midpoint is as watertight as the frames.
     An outline (a line the course renderer draws along a tile's edges, at
     whole pixels and behind the tile) is drawn into the midpoint as a quad
     per segment whose points go where the polygon vertices they lie at
     went, also when its identity is new (an outline's key counts its place
     in the tile); a new outline where ghosts go stays out. Car light
     trails are tagged by the trail sample they reach back to, since their
     ring advances every frame. A triangle the GPU skips for its size in
     this frame (it
     passes close to the view) that the frame before drew is still drawn
     into the midpoint. A midpoint that is no plausible in-between (most
     identities new: a cut; much of the picture without a pose in the frame
     before; or an area where it shows something much farther than both
     frames show there, a hole or the background through a gap, measured on
     coarse depth grids of the opaque primitives) is not shown: the frame
     before stays for that field. Off, nothing is tagged and every value is
     the original one.

   - **Video timing (PAL / NTSC):** the PAL release is the NTSC game with
     every value that counts frames converted to 25 frames a second: frame
     counts to 5/6 (countdown, fades, timers, attract and intro scripts,
     camera keys, credits, music cue frames), frames-to-time conversions
     `/25` instead of `/30`, per-frame rates (engine RPM smoothing). Physics
     and AI are mostly the NTSC per-frame code, and so are the car records,
     except 34 that the PAL release retuned, which NTSC timing keeps
     (`docs/REGIONS.md` lists the exceptions). `include/rr/video_timing.h`
     (`VideoTimingNtsc`, `RR_PAL_NTSC(pal, ntsc)`) keeps the PAL value and
     adds, at each such place, the value of the NTSC release (SLPS-01800,
     compared function by function with the PAL executable and overlays).
     NTSC timing runs `SetVideoMode(MODE_NTSC)`: 263-line fields at 59.82
     Hz, races at 30 frames a second, and the PAL picture (256 lines,
     layout, text) is kept whole with `Psyz_GpuSetNtscPalPicture`. The
     choice is made at startup (preference or `RR4_VIDEO`); with PAL every
     value is the original one.
     Checked against the Japanese disc on DuckStation, NTSC timing races
     match it field for field in their timing (race load 636 fields,
     countdown, two fields a frame, the frames-to-milliseconds lap time);
     with the same random state and input the car still differs by one
     position unit from the eleventh race frame, which is the release's
     code or data, not timing. Release differences that are not frame-rate
     conversions keep the PAL behaviour: the splash turns the display on at
     its third frame (the NTSC release at its second) after a full-screen
     clear the NTSC release does not draw, and its loads read the PAL disc
     (larger menu sound banks, other file positions and seeks), so the
     NTSC-timing splash lasts 314 fields against the Japanese release's
     291.

   - **Releases (PAL / Japanese):** the mounted disc's `SYSTEM.CNF` selects
     a release profile (`include/rr/release.h`): `RR_RELEASE_JP` and
     `RR_PAL_JP(pal, jp)` choose the Japanese release's (SLPS-01800) code and
     data next to the PAL code in the original modules, and `*Jp` tables sit
     next to the PAL tables they replace. The Japanese release runs at NTSC
     timing with its own 240-line picture, R4.BIN map, text, saves and
     passwords (`docs/REGIONS.md`). With the PAL disc every value is the
     original one.

   - **PS1 time model:** the game and PsyZ spend the PS1 time of the work
     that decides on which field something happens (CPU routines, BIOS and
     SDK calls, CD, card, SPU, GPU and interrupt handlers), measured on
     DuckStation; mode entries, `FrameCounter` and `rand` then match it
     field for field (splash, opening movie, title, attract, menus, Grand
     Prix, Time Attack, Extra Trial). The costs are in two tables, PsyZ's
     `src/timing_profile.h` (BIOS, SDK, devices, interrupts) and the game's
     `include/rr/cpu_time.h` (`RR_CPU_CYCLES`), each value with how it was
     fitted and the commit of its run. Fit costs to runs without GDB
     breakpoints: any breakpoint slows DuckStation's CPU by about 1%
     (`tools/port/duckstation.py` stepping and RAM reads do not). The
     memory card driver serves the ports on alternate vblanks, a sector
     transfer ends 266k clocks after its vblank, and file I/O takes a turn
     per frame; the interlace field flips at line 0 and the displayed field
     is latched at the vblank, as in DuckStation's CRTC, so every other
     VSync(0) in 480-line interlace returns late on the console's fields.
     Busy waits that DuckStation measures with interrupts in them are fitted
     as CPU time with the interrupts on top (StGetNext), so a second pad's
     libpad exchange (4.8k clocks a vblank) lengthens them; DrawOTag waits
     for the list DMA over the table's empty head (61 clocks a node).
     A LIBCD polling wait (CdSync(0), CdReady(0)) takes the interrupt
     handlers that come meanwhile within the wait, so a CdControlB(CdlPause)
     returns as the drive finishes; the wait CD_cw makes for the previous
     command keeps them owed, as DuckStation does. Still open: libpad's
     store order and missed vblanks with two pads (VS, `docs/TECH_DEBT.md`
     F1), and the sequencer timer's phase, which the boot timeline before
     SsStart sets (F6).

## Repository layout

| Path | Contents |
| --- | --- |
| `src/main/<subsystem>/` | resident game modules |
| `src/main/psyq/lib*/` | legacy PsyQ helpers being replaced by PsyZ or kept as portable C |
| `src/overlays/<module>/` | the nine mode/screen overlay modules |
| `src/port/` | native entry point and host integration helpers |
| `include/` | shared types, asset layouts and subsystem headers |
| `third_party/psyz/` | PsyZ host runtime and SDL3 dependency |
| `docs/` | native development guidance, ABI notes and format reference |
