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
     the primitive (`PSYZ_GPU_FETCH=0` forces the copies on Metal). The
     probe for framebuffer fetch asks SDL's own Metal device, which on a Mac
     with two GPUs need not be the system default one.
   - **Fallback without SDL GPU:** when no SDL GPU device can be created
     (Linux without a Vulkan driver, Windows without a usable D3D12 device,
     or its resources fail; `PSYZ_GPU=off` forces it) the CPU rasterizer
     draws, as at 1x, and an SDL renderer (SDL's own choice, its software
     renderer last) presents the display area at the game's aspect. The
     GPU-only options (internal resolution, widescreen, antialiasing,
     texture filtering, geometry correction) are off
     and greyed out in the Escape menu with a note naming the renderer
     (`Psyz_VideoGetFeatures`); the Settings panel opens in a window of its
     own, as without the GPU overlay. Only when neither can start, or the
     GPU fails during the run, PsyZ calls the host's failure callback
     (`Psyz_VideoSetFailureCallback`): the game shows the message in a
     message box and quits (headless runs log it).
   - **GPU memory:** SDL keeps every buffer it cycles in (a buffer written
     while the GPU still reads it) and never frees them, so PsyZ cycles
     rarely. Batches go one after another into the vertex/index buffers of
     the ring's current generation and its transfer buffer (`RingReserve`),
     VRAM uploads into one upload buffer (`UploadMap`); a wrap moves to the
     next of four generations (own buffers, not SDL cycles, so a deferred
     native draw can still use its generation's vertices) and cycles the
     transfer buffer, and it submits the work with a fence and waits for the
     fence three wraps before. Presents keep
     two frames in flight (`SubmitFrame`). A waited fence is released after
     the next submit: SDL's Metal backend frees a finished command buffer
     (and its hold on the buffers it used) in a later submit, and a fence
     released before that was taken by the very next command buffer, so the
     old one never looked finished. A frame's command buffer is submitted
     every 256 descriptors on every backend (a test forces a submission
     before every render pass and compares the pictures). At 8x with
     antialiasing a race and its replay stay at about 2.6 GB (physical
     footprint, including GPU memory) instead of growing past 10 GB.
   - **Native VRAM on demand:** with a scaled target the native (1x) VRAM
     only serves what reads the VRAM back (textures and palettes the game
     drew itself, such as the replay's blur that draws the previous frame
     over the next; StoreImage, MoveImage, a 24-bit display, captures). A
     batch that draws only into recently shown display buffers keeps its
     native draw queued (`NativeDefer`: views, indices, compact vertices,
     ring generation) and draws it into the native VRAM, in order, only
     when a read or write needs those pixels (`NativeRead`/`NativeWrite`
     with the earlier queued draws they depend on); a fill or upload that
     covers a queued draw drops it. A race frame nobody reads is never
     drawn at 1x. Queued draws whose vertex buffers or palette rows are
     about to be reused are dropped (or drawn, when from the last two
     presents), their pixels marked out of date and rebuilt from the scaled
     picture if read later (logged). `PSYZ_NATIVE_DEFER=0` draws every
     batch at once; `PSYZ_NATIVE_VERIFY=1` draws a shadow native VRAM at
     once and compares it at every read that drew queued draws or rebuilt
     pixels, every download and one read in 61; `PSYZ_NATIVE_TRACE=1` logs
     the queue's work. With framebuffer fetch (Metal) the scaled target has
     no copy for semi-transparency (half the scaled VRAM memory).
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
     neighbour tile instead of leaving a hairline crack. Only where the
     truncated fraction projects under a pixel: close to the camera it spans
     several, and the exact vertex moved away from the whole-unit ones the
     neighbouring polygons are drawn from (the sky showed through a tunnel's
     ceiling at the top of the screen), so there it keeps the whole unit.
     With the option off
     the packets are unchanged: at Nx the original whole-pixel vertices
     leave the T-junction cracks between differently subdivided course tiles
     that fall between the PS1's samples at 1x (as in DuckStation upscaling
     without PGXP); geometry correction is the watertight mode. The precise
     vertices are kept in a set-associative cache by address (262,144
     entries, a race frame stores about 40,000); its sets are taken from the
     offset of the address in the executable's image, so which vertex a
     full set would drop does not change with ASLR from run to run (it
     did: a rare hairline seam on the car or the road, TECH_DEBT H16).
   - **Depth buffer (Escape > Graphics, `depth_buffer`,
     `RR4_DEPTH_BUFFER`; off by default):** the 3D world is depth-tested per
     pixel on the GPU instead of only drawn in ordering-table order
     (`include/rr/depth_buffer.h`, PsyZ `Psyz_VideoSetDepthBuffer`). Its
     depth comes from the same precise-vertex cache as geometry correction
     (kept active while the option is on, whether or not geometry
     correction draws from it): a polygon whose packet vertices all have a
     precise projection carries 1 / view depth per vertex (a vertex
     attribute of `psx.vert`; reversed: cleared to 0, `GREATER_OR_EQUAL`,
     32-bit float where the device has it). Opaque ones are tested and
     written, semi-transparent ones only tested; 2D, sprites, lines and
     everything without precise vertices keep the ordering table's order
     untested, exactly as before. The game marks each 3D view in its
     ordering table: `SetupViewport` links a host GP0 command (1Eh, handled
     by `DepthViewMark` in `camera/viewcam.c`) right after the view's
     drawing area, which clears the depth there and starts the view; the
     frame links an end command last in both ordering tables
     (`system/frmtask.c`). Only polygons drawn under the view's drawing
     area are tested (an off-screen sky render is not), and only the shown
     target has a depth texture (the scaled one at Nx; at 1x the GPU
     rasterizer draws, not the CPU one). The sky and backdrop
     (`DrawTrackScenery`, projected around the camera) are tagged
     `PSYZ_PGXP_DEPTH_NONE`. Coplanar layers keep the original's choice:
     every course face and horizon tile hands its ordering bias to its
     vertices (`Psyz_GteSetPgxpDepthLayer`, `gfx/submodel.c`), and PsyZ moves
     the face by half a depth unit per bias entry (horizon tiles: two), so a
     road marking (bias 5) stays on the subdivided road (15..30), whose
     whole-unit INTPL vertices are not planar with it; semi-transparent
     polygons (shadows) move a unit forward. Vertices whose projection
     saturates or is clamped (a tile reaching under the camera) carry no
     depth and leave their polygon in ordering-table order. A light trail's
     far edge is a sample projected in an earlier frame: its stored SZ
     gives it a depth (`Psyz_GteStorePgxpDepth`, `gfx/gtepoly1.c`), and a
     semi-transparent polygon with only some depths is tested at their
     average, so trails behind a wall are hidden with their car. With the
     option on, the course-occlusion check (`gfx/course_occlusion.c`) is
     skipped: the depth test hides what lies behind the course. With it
     off nothing is linked or classified differently and frames are
     unchanged. `PSYZ_DEPTH_DEBUG=1..5` tints written / tested polygons,
     the faces' biases and the 3D polygons left untested. What is left is
     listed in TECH_DEBT H2.
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

   - **Presentation:** every game frame is presented once, at the field
     blank it is shown from (paced presents, `sdl3_common.h`). The frame
     limiter always paces the fields by the wall clock at the console's
     field rate (59.82 Hz NTSC); the display never does
     (`field_pacing.h`). VSync Auto and On present in a VSync present
     mode (no tearing) on a display at about the field rate or faster, and
     the swapchain image is acquired without waiting
     (`SDL_AcquireGPUSwapchainTexture`, three frames in flight), so a full
     swapchain queue never stalls the game (that frame is not shown). In
     0.4 driver VSync paced the game on a 60 Hz display: the 60.000 Hz
     refreshes beat against the 59.82 Hz fields and the game ran at half
     speed for about three seconds in every 5.5. A backend without
     non-blocking synced presents (OpenGL) still waits for the display in
     its swap, but the wait for the field's blank counts that time
     (`Psyz_FieldWaitUntilNs`). 2D screens, menus and
     movies are presented at the display aspect in both Classic and
     Widescreen. The PS1-era frame interpolation (in-between frames guessed
     from the PS1 draw lists, 0.3 and before) was removed.

   - **Video timing (PAL / NTSC):** the PAL release is the NTSC game with
     every value that counts frames converted to 25 frames a second: frame
     counts to 5/6 (countdown, fades, timers, attract and intro scripts,
     camera keys, credits, music cue frames), frames-to-time conversions
     `/25` instead of `/30`, per-frame rates (engine RPM smoothing). Physics
     and AI are the NTSC per-frame code. The car records are the NTSC ones
     except 34 that the PAL release retuned (`docs/REGIONS.md`).
     `include/rr/video_timing.h` (`VideoTimingNtsc`, `RR_PAL_NTSC(pal,
     ntsc)`) keeps the PAL value and adds, at each such place, the value of
     the NTSC release (SLPS-01800, compared function by function with the
     PAL executable and overlays; every constant-only and timing difference
     `tools/port/region_diff.py` lists is behind `RR_PAL_NTSC` or
     `RR_PAL_JP`). NTSC timing is the default (Graphics > Game timing,
     preference `game_timing`, `RR4_VIDEO=pal|ntsc` for a run); the choice
     is made at startup. It runs `SetVideoMode(MODE_NTSC)`: 263-line
     fields at 59.82 Hz, races at 30 frames a second (60 with Race frame
     rate), the PAL picture (256 lines, layout, text) kept whole with
     `Psyz_GpuSetNtscPalPicture`, and the 34 retuned car records take the
     NTSC release's values as the car is loaded (`CarRecordUseNtscValues`,
     `src/main/data/bankrec.c`, by R4.BIN entry; both discs' values are in
     its table). With PAL every value is the original one.
     Checked against the Japanese disc on DuckStation, NTSC timing races
     match it field for field in their timing (race load 636 fields,
     countdown, two fields a frame, the frames-to-milliseconds lap time).
     With the NTSC car records a Time Attack race on the PAL disc and on
     the Japanese disc (both in the port, the same rand and autodrive
     inputs, field rate on) keep the player car's position and velocity
     identical for all 3298 race fields compared; with the PAL records they
     part at the 468th. The splash turns the display on at its second frame as the NTSC
     release does. Release differences that are not frame-rate conversions
     keep the PAL behaviour (TECH_DEBT F9): the splash's full-screen clear
     (black behind the logo), the menus' dialogue waits and demo fade
     values, the ranking tie rule and the race frame's mode flag. Movies
     keep their own rate (25 fps on the PAL disc) at either timing: each
     picture is shown at the first field at or after its time, 2-3-2-3-2
     fields at 59.82 Hz.

   - **Race frame rate (field rate):** with Graphics > Race frame rate on
     (default) a race runs its mode function every field, 50 Hz PAL /
     60 Hz NTSC, without interpolation (`include/rr/field_rate.h`,
     `src/main/system/fieldrate.c`). `RaceFieldRateBegin` (race setup)
     chooses the rate; the link race and the attract demo stay at the
     original rate, and a mode that sets the menus' `VSyncThreshold` ends
     it. The slow lane (`RaceSlowTick()`, every second field) runs the
     counters, timers and one-frame events at the original tick; the pad
     edges are latched between ticks. The cars are the released game's:
     `StepRaceFrame` (racemai2.c) runs the original tick of every car on
     the tick's first field with the rate off (`StepRaceTick`: the player
     cars' inputs and update, the opponents' driving `UpdateComCars` with
     its rubber band and start boost, every car stage, the car and wall /
     kerb contact, the opponents' track projections), in the original's
     order, and nothing for them on the second field. A race's every tick
     ends on the original's state, the opponents' included, and a replay
     (recorded and played once a tick, as released) reproduces it. What is
     drawn lies on one trajectory: the first field draws every car half
     way from the last tick's end to this one's (`CarPoseHalf` over
     `CarPoseFields`: position, angles, suspension, shakes and the drawn
     offsets the renderer and cameras read), the second the tick's end, and
     the pose goes back to the tick's end before the next field runs.
     The track position (`trackNodeF0`, `trackFraction170`) is race logic
     and stays the tick's end; what is placed by it and drawn with the car
     reads `CarDrawnTrackDistance` (half way with the pose): the replay
     track cameras (`UpdateRaceCamera` kinds 5 and 6) took the tick's end
     and led the car on one field of two, a shake at the field rate.
     `CarPoseSettle` forgets a half way pose when the cars are replaced (a
     race set up, a replay's snapshot restored, a replay file loaded) and
     puts the tick's end back when the race ends: a stale one moved the
     next race's cars off their grid slots and started replays where the
     race had finished. Presentation code converted to run every field
     (the fast lane) takes half steps (`RaceFieldHalf`) and the followers'
     per-field share (`RaceFieldApproach`: 2r/(1+r), a tick's lag behind a
     moving target), its remainders in the per-car sidecar
     `CarFieldState`. A replay file keeps the sidecars at its snapshot
     (block CarFieldStates; one of another size is not restored, an older
     file's FieldInputs block is not read). Original-rate replays play at
     the original rate.
     Presentation code that runs every field reads tick counters in field
     time (`RaceFieldTimeAfter/Before`: counter * 2 + the field's phase),
     splits a per-tick add over the tick's two fields (`RaceFieldSplit`),
     blends colour cycle tables half way between entries
     (`RaceFieldCycleColor`) and draws its flicker from a rand of its own
     (`RaceDrawRand`, PsyqRand at the original rate). Converted this way:
     the intro orbit, the replay/track cameras' cuts (per tick) and roll and
     lens lerps (per field), the Grand Prix fly-by; the course objects
     (`objsim.c`: f0 and every key, ease window and delay in fields, rand
     launches, light cycles and the blimp line's acceleration per tick); the
     draw-time light fades, beacons, fireworks (timeline per tick, motion
     split), light and brake trail rings (a pose per tick, the newest
     replaced between), glow ramps, sparks, scrolling textures, flashing
     lights, rank pulse and minimap dot; the sound (`SoundUpdate` per tick,
     `SoundUpdateField` between: listener, two-field Doppler, slot pitches
     and volumes, the master fade half way); the pad's auto-repeat counts
     ticks.

   - **Releases (PAL / Japanese):** the mounted disc's `SYSTEM.CNF` selects
     a release profile (`include/rr/release.h`): `RR_RELEASE_JP` and
     `RR_PAL_JP(pal, jp)` choose the Japanese release's (SLPS-01800) code and
     data next to the PAL code in the original modules, and `*Jp` tables sit
     next to the PAL tables they replace. The Japanese release runs at NTSC
     timing with its own 240-line picture, R4.BIN map, text, saves and
     passwords (`docs/REGIONS.md`). With the PAL disc every value is the
     original one.

   - **Time:** the port runs on the host's clock; nothing emulates the
     PS1's timing. Native code takes no emulated time, and PsyZ's devices
     complete as fast as the host does their work: CD commands complete at
     once (no seek, rotation or acknowledge times), a plain read (CdRead)
     hands its sectors to LIBCD's callback at the drive's next service (the
     interrupt thread, a VSync, a poll), the MDEC decodes a frame's slices
     as the game asks for them, SPU DMA, the GPU (DrawSync) and the memory
     card's requests and file I/O end inside their calls or as an interrupt
     right after. A CD read in CdlModeRT plays its XA audio as it goes, so
     its data sectors keep the disc's rate (150 a second at double speed)
     on the kernel's clock: the movies' frames come at their stream's rate
     (25 fps PAL, 30 the Japanese release) and StGetNext waits for the next
     sector instead of spinning, while the audio device pulls the XA sound
     from the same sectors. A frame's last sector is held to an even grid
     of the stream's period (the interleaved audio sectors would otherwise
     shift frames by a sector and alternate their pictures between one and
     three fields), and one due in the last 4 ms before a blank is taken
     early so it is decoded in time for it: each picture shows for two
     fields at 25 fps on 50 Hz, with a one-field picture where the stream's
     clock and the display's drift a field apart (about every 4 s). The vblank callbacks run once per field of the
     host's frame clock (VSync counts the fields of the wall clock; the
     game's VSync(0) presents and waits for the next), the root counters
     (the sequencer's tick) fire on the kernel's interrupt thread at their
     host times. Headless test runs use the offline clock
     (`PSYZ_AUDIO_CLOCK=video`, the default of `PSYZ_HEADLESS`): a
     deterministic field clock that stands still while the game works and
     moves only in its waits (VSync's for the next blank, one line per
     poll of the line counter `VSync(1)`, a stream's next sector), running
     the root counters and mixing the audio on the way, so a run's fields
     are the same every time. libpad (PadInitDirect, PadStartCom) is PSY-Q's channel
     state machine ported from the decompilation (PsyZ `libpad.c`), run a
     byte at a time against models of the controllers (`pad.c`, as
     DuckStation's digital pad, DUAL SHOCK, neGcon and JogCon answer):
     finding a DUAL SHOCK's modes and actuators takes 22 exchanges,
     PadSetActAlign four, a digital pad refuses 0x43 eleven times, and the
     motor bytes go out through libpad's actuator gates. It keeps a port's
     received packet until it next starts a transfer, so with two pads
     port 1's bytes reach the receive buffer at the vblank of the exchange
     and port 2's at the next, with one pad at the next; answers in
     configuration mode and failed exchanges leave the buffer as it was
     (an unplugged pad's bytes stay for eleven vblanks); test input
     (`RR4_PAD_SCRIPT`) drives the controllers before the exchange
     (`Psyz_SetPadsInputCb`).
   - **Sound:** PsyZ's software mixer (`src/psyz/psyz_spu.c`,
     `spu_voice.h`) plays the game's own sound data as the SPU did: VAG
     ADPCM voices with their pitch and Gaussian interpolation, ADSR
     envelopes, FM, noise, the reverb and the CD/XA audio, mixed to
     44100 Hz stereo. In play the audio device's thread pulls the samples
     (`sdl3_audio.c`); headless runs mix them on the offline field clock
     so their sound is deterministic. libspu's register writes and RAM
     transfers reach it directly (no DMA controller, interrupt or transfer
     timing; libspu's status waits end on their first check). It keeps the
     SPU's capture buffers in sound RAM and the key-on's five-sample delay:
     they cost little, are part of each note's timing, and PsyZ's tests
     observe a voice through the capture buffer as on the console.
   - **Link cable over the network:** the link battle's serial port (SIO1)
     is PsyZ libcomb (`src/psyz/libcomb.c`): Ridge Racer Type 4's libsio
     driver ported from the decompilation (`_sio_control`, the `sio:`
     device's synchronous and asynchronous transfers, the interrupt
     handler) over a model of the port's registers, whose other end is the
     other player's game over TCP (`link_net.c`, `psyz/link.h`;
     `src/port/link.c` sets it up from the Escape menu or `RR4_LINK`). Bytes
     take their character time at the programmed rate (20.8 us at the
     game's 529 kbit/s), the transmitter waits for CTS, the FIFO holds 8
     bytes; each pass of the driver's busy waits lasts what it did on the
     console (about a microsecond of host time), so the game's wait
     callbacks, which count passes, time out as on the console (about 24 ms
     in the race, 94 ms in the menus). The receiver
     announces each read (where it starts and ends, its packet size), so
     the DTR toggles that acknowledge the sender's packets and the RTS
     drop at the read's end follow at once from the sender's own bytes, as
     on the cable; everything else crosses the network's latency. Received
     bytes enter the FIFO at their arrival times on the kernel's clock and the RX
     interrupt drains each completed packet then, so a host thread that
     runs late cannot overrun it (an overrun loses a packet, and the race
     protocol then desyncs). The race protocol (`serial_link/sio.c`) steps
     the race only with both ends' inputs for a frame, one packet in flight
     each way, so a round trip longer than a frame slows the race (40 ms:
     half speed, 100 ms: a seventh) and seven missed frames end it with the
     game's link error; the menus' synchronous exchanges time out from
     about 90 ms. When one end falls a slot behind (a late packet, a host
     thread that ran late), the protocol steps both ends with the same
     earlier slot; the original compared those slots without wrapping them
     modulo 8 and desynced the race across the wrap, which a cable and
     aligned vblanks never let happen on the console (fixed in
     `ovl667_select_active_slot`). Two games on one machine race a whole
     race step for step identically (`RR4_LINK`, `PSYZ_HEADLESS=gpu`).

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
