# Changelog

## Unreleased

- **Fixed: on a 60 Hz display with VSync on (Auto or On), the game ran at
  half speed for about three seconds in every six** while the picture
  stayed at 60 frames a second (reported on Windows with NVIDIA GPUs; VSync
  Off avoided it). Since 0.4 a display at about the game's field rate let
  its VSync pace the game, while the game's field clock (and its VSync
  wait) followed the wall clock at the PlayStation's 59.82 Hz. A 60.000 Hz
  display's refreshes drift against those fields by one refresh every 5.5
  seconds: for half of that time each field's blank came while the game
  was still drawing its frame, so every frame waited two refreshes. Now the
  frame limiter always paces the game by the wall clock, in every VSync
  mode, and VSync only decides how frames reach the display: Auto and On
  present in the display's VSync present mode (no tearing) on any display
  at 57 Hz or faster, without the game waiting for a full swapchain (three
  frames in flight on Direct3D 12 and Vulkan, so a present at about the
  display's own rate finds a free image). On a 60.000 Hz display one field
  is shown for two refreshes every 5.5 s (the fields are 59.82 Hz); on 120,
  144 or 165 Hz displays the fields keep their 2/3-refresh cadence; with
  G-Sync / FreeSync every field is shown once. With OpenGL, whose synced
  swap waits for the display, the time the swap waited now counts towards
  the field's wait instead of adding to it. The log
  (`ridge-racer-type-4.log`) names the choice ("vsync auto, 60.00 Hz
  display: 59.817 Hz fields from the frame limiter, synced presents").
  Measured headless on a modelled 60.000 Hz display (`PSYZ_SIM_DISPLAY`,
  Time Attack, real GPU renderer): 0.4 alternated about 3 s at 60 and
  2.5 s at 30 game frames a second (44 to 47 on average over the race, at
  60 and 59.94 Hz, Auto and On); now 59.8 in every second at 60, 59.94,
  144 and 165 Hz and on a variable refresh display (144 and 165 Hz were
  not affected before either).
- Diagnostics for frame pacing in the log: `[Frame pacing] long frame`
  lines (at most one a second) when a frame took longer than a field, with
  the game's own work and the VSync wait, and `no_image=` in the
  `[Presentation FPS]` line for frames not shown because every swapchain
  image was queued. `PSYZ_SIM_DISPLAY=<hz>[,vrr][,images=n]` (with
  `PSYZ_HEADLESS=gpu`) models a display for headless pacing tests, with
  `PSYZ_SIM_VSYNC` and `PSYZ_SIM_DISPLAY_LOG`. PsyZ's `psyz_pacing_tests`
  run the frame limiter's decisions on a fake clock against fixed, slower,
  faster and variable refresh displays in every VSync mode.
- **Fixed: the camera shook very fast in some replay camera angles** (at
  the field rate, 0.4's default). Two kinds of replay track camera are
  placed by the car's distance along the course: one that runs alongside
  the car at a fixed angle, and one that rides the course beside it and
  turns to follow it. The car is drawn half way between two of the
  original's ticks on every other field, but its distance along the
  course is race logic and stayed at the tick's end, so those cameras
  jumped ahead of the car on one field and fell back on the next. They
  now take the distance where the car is drawn (`CarDrawnTrackDistance`).
  Measured per field on Grand Prix replays of all eight courses (the
  drawn camera's position, angles and lens, and the camera-to-car vector;
  sign-alternating second differences of 8 units or more): the
  alongside camera alternated on 99% of its fields (runs of up to 265
  fields in a row, steps up to 320 units), now on at most 2 isolated
  fields per course; the riding camera on 95 to 99.6% (runs up to 323,
  up to 390 units), now on 0.3 to 2.9% in runs of at most 6 and at most
  20 units, the integer rounding of the original's camera maths (a replay at
  the original rate: 9%, runs up to 4, up to 18). The other replay
  cameras (fixed, pivoting and zooming), the race's chase camera, the
  start orbit and the Grand Prix fly-by did not alternate and are
  unchanged. The race itself is untouched: every car's state per tick is
  the same as before.

- **Less GPU work on Direct3D 12 and Vulkan (Windows, Linux), most of all
  in Shooting Hoops.** Without Metal's framebuffer fetch, a
  semi-transparent primitive (smoke, shadows, light trails, glass, the
  fireworks over Shooting Hoops' finish) blends with a copy of the picture
  made before it, and every copy and every render pass is a set of
  whole-texture barriers on these APIs, where the GPU waits for all the
  work before it. The renderer ended a render pass and copied again at
  every semi-transparent primitive that overlapped anything drawn earlier
  in the pass: at the finish of Shooting Hoops about 1,400 passes and
  1,800 copies a frame on average (up to 5,000), 60 and 190 in a race on
  course 1, hundreds in menus. Now each batch is drawn in levels: a
  semi-transparent primitive goes one level above the highest thing it
  overlaps that was drawn before it, everything else into the level of
  what it overlaps, and each level is one render pass after one set of
  merged copies. Every pixel still sees its primitives in the original
  order, so the picture is the same (3,012 of 3,012 headless captures
  byte-identical: attract, menus, movie, Time Attack, Grand Prix with the
  mirror, replay blur and results, versus split screen, Shooting Hoops'
  last lap, at 1x, Auto at 1440p and Auto at 2160p with antialiasing, with
  and without widescreen, with and without framebuffer fetch). The scaled
  picture also stays in its render-target state between passes (it is no
  longer a sampler texture), and palettes are copied into the CLUT cache
  again only when their VRAM was written since (a race frame copied each
  of its palettes every few draws). Lines (the fireworks are 510 Gouraud
  lines a frame on the final lap) were stepped on the CPU and drawn as a
  quad for every pixel whose colour changes; without widescreen and
  geometry correction each line is now one quad, and the shader steps it
  per pixel with the same integer arithmetic (identical pixels; with
  widescreen or geometry correction the per-pixel quads stay). Headless at
  Shooting Hoops' finish with the copy path (as on Direct3D 12 and
  Vulkan): 24 -> 6 ms a frame at 1440p with widescreen, 49 -> 9 ms at 4K
  with antialiasing; a Grand Prix race's GPU time 3.4 -> 2.3 ms at 1440p,
  and with Metal's framebuffer fetch 3.0 -> 2.0 ms (fewer palette copies).
  Measured on a Mac (Metal with framebuffer fetch off, and Vulkan through
  MoltenVK); no Windows GPU was measured.

## 0.4-alpha

Highlights: races at 60 frames a second without interpolation, with the
original's driving, opponents and replays; no PlayStation timing emulation
(fast loads); resolution that follows the window; extended draw distance
without pop-in or cars through walls; an experimental GPU depth buffer.

- **Depth buffer (Escape > Graphics > Depth buffer; experimental, off by
  default).** The PlayStation draws the 3D world in ordering-table order: a
  whole polygon at the average depth of its corners, plus per-face biases
  the course data uses to keep markings on the road and walls behind what
  stands in front of them. Large or long polygons then sort through each
  other (trees over bridge pillars, cars over walls, crests and tunnel roofs
  at the extended draw distance). With the depth buffer on, every polygon
  of the 3D world is also hidden per pixel by what is in front of it, on the
  GPU: its depth comes from the precise projection of its vertices that
  geometry correction uses (with or without that option). Only the 3D world
  takes part: the HUD, text, menus, movies, sprites and the sky and
  backdrop are drawn exactly as before, in the original's order.
  Semi-transparent polygons (shadows, glass, smoke, lights) are depth-tested
  but do not hide what is drawn after them. The course's ordering biases
  still decide between faces on one surface (road markings stay on the
  road), and every view (race, mirror, split screen, replay, the garage and
  car select previews) starts with its own cleared depth. With it on, the
  course-occlusion check of the extended draw distance is not needed and is
  skipped. Off, every frame is drawn exactly as before (captures
  pixel-identical). `RR4_DEPTH_BUFFER=1` turns it on for a run;
  `PSYZ_DEPTH_DEBUG=1..4` tints what is depth-tested. SDL GPU renderer only
  (Metal, Vulkan, Direct3D 12); greyed out with the OpenGL or CPU renderer.
  Measured headless against the same frames with it off (Grand Prix on six
  courses, versus, Time Attack, menus, a race and its replay, 1x and 4x,
  with and without geometry correction): 0.0-0.4% of the pixels change,
  the cars and light trails behind walls, crests and tunnel walls, trees
  and posts behind bridge pillars, and the parts of cars the course
  occlusion check had left. Cost: no measurable frame time at 4x 1080p or
  8x with antialiasing; about 0.3 ms at 16x 2160p with antialiasing (within
  run-to-run noise there), one more render pass a frame, and a depth
  texture the size of the scaled VRAM (32 MB at 4x, 128 MB at 8x). Not yet
  free of artifacts, so off by default: far road markings lose a few edge
  pixels without geometry correction (most at 1x), and course objects
  modelled partly below the ground show as sunk (TECH_DEBT H2).

- **Extended draw distance: rivals no longer show through walls, crests,
  tunnel roofs and the road.** The course and the cars are layered by
  ordering-table slot: each course tile at the average depth of its corners
  plus a per-tile bias the course data uses to push walls, banks and
  roadside tiles behind what stands in front of them. With the extended
  view each slot is twice as deep, so those biases push tiles twice as far
  back (a wall tile with bias 20 by about 45 m instead of 22 m), and since
  opponents, their wheels and shadows are drawn to the end of the view, a
  car or its shadow behind a wall, a crest or a tunnel roof was drawn over
  it. Now, with Extended on, each view's drawn course tiles are also
  drawn, in the GPU's order, into a one-pixel buffer of the depth of the
  course where it shows (their corner depths from the GTE), leaving out
  tiles that are semi-transparent or whose texels include the transparent
  colour (fences, trees, cut-out shapes). An opponent's faces (body,
  wheels, shadow) that lie behind the visible course over their whole
  area are not drawn; when other faces of the car show over course in
  front of them, the whole car is linked behind that course in the
  ordering table, in its own order, if it stays in front of the course
  behind it, so the course covers the hidden parts (otherwise only its
  shadow moves, and the car is drawn whole as before). Cars nearer than
  about 25 m, the tail-light trails and cars behind track objects (signs,
  buildings) are drawn as before. The mirror and the split screen get the
  same test. Measured headless in Grand Prix races on courses 1 to 7 (a
  per-pixel check of what is drawn against the precise depths of geometry
  correction): car pixels drawn over nearer course beyond 25 m down by 82%
  (bodies 75%, wheels 63%, shadows 83%), beyond 175 m by 89%; visible car
  pixels lost by mistake under 0.01%. About 0.1 to 0.3 ms of CPU a frame.
  Original draw distance: no change (captures identical).

- **Opponents drive exactly as in the original at 60 Hz.** The opponents'
  driving, their car stages and the car-to-car contact ran every field with
  halved steps and square-rooted filters, an approximation of the original's
  tick: their positions drifted from the original's (thousands of units by
  the second lap of a Grand Prix), the contact met the player car at the end
  of its tick while the opponents were half way through theirs, and the car
  stages run once a tick outside the race loop (UpdateFinishingPlayerCar,
  after a Grand Prix's finish) moved a car half as far. Now every car runs
  the original's tick whole on the tick's first field, in the original's
  order, and is drawn half way between ticks as the player car already was:
  the rubber band, start boost, lane changes and contact are the released
  code, and a race at 60 Hz is the original rate's tick for tick. Headless
  NTSC Grand Prix with the reference stick driver: every car's position,
  speed and track distance at 60 Hz equal the original rate's at all 3,404
  tick ends compared (before: the opponents differed from the first tick, by
  up to 6,526 units of track by the second lap, their splits up to 13 ticks
  apart); the grid is the original's (the player 8th of 8, also in the race
  after a replay); replays equal their races at every tick end (Grand Prix
  at 60 Hz, Time Attack at both rates). The per-field opponent and car stage
  code is removed: the car modules are the released code again.

- **Replays show the race you drove, and the next race starts from your
  grid slot.** At the 60 Hz race frame rate the player car is drawn half
  way between two game ticks on every other frame, and that drawn pose was
  put back on the next frame from a copy of the previous tick's end. The
  copy outlived the race: when a replay began (the cars restored from the
  snapshot taken at the last lap), its first frame moved the player car to
  where the race had finished, keeping the snapshot's speed, so the replay
  drove a different line from the start (Grand Prix, 2,257 ticks compared:
  47,000 units off the race at the first tick); the next race (the
  second Grand Prix race, a retry, any race after a replay) put the player
  car at the position where the previous race or replay had ended instead
  of its grid slot (in a test, 4th of 8 on the grid instead of 8th, with
  the rank and the opponents' catch-up following that position). The
  half way pose is now forgotten whenever the cars are replaced (race
  setup, a replay's start, a loaded replay file) and settled when the race
  ends.

- **Extended draw distance: far opponents no longer show through walls, the
  road or your own car.** The renderer positions a car from the low 16 bits
  of its distance to the camera on each axis, which wraps 32768 units
  (about 165 m) away. The original drew no opponent that far; since
  opponents keep their model to the end of the extended view, a car past it
  was drawn about 330 m off its place: a small car on the road in front of
  you, over a wall or in the sky. Opponents and their wheels and shadows now
  take the whole distance, with the course's projection for points far off
  the view's axis; within 32768 units the result is the original's. The
  mirror's car depth bias keeps its original depth with the extended
  view's deeper ordering steps (it pulled far cars in front of walls twice
  as far). Original draw distance: no change in Time Attack and Grand Prix
  captures.

- **Geometry correction: no sky through tunnel ceilings at the top of the
  screen.** The subdivided course vertices keep their exact position (which
  closes far hairline cracks) only where the difference projects under a
  pixel; right above the camera it is several pixels, and the moved
  vertices left a sliver of sky between ceiling tiles at the top edge in
  tunnels and under bridges. The far road stays closed as before. PsyZ
  (GTE INTPL precision for geometry correction).

- **Extended draw distance: no more shading, shadow and car-detail pop-in a
  short way ahead.** The extended view only widened the course lists and the
  depth range; the original's near-detail steps stayed where they were, well
  inside it. Course tiles past a per-segment far depth (about 40-225 m
  ahead, typically 105 m) were drawn in one flat colour with a
  half-resolution texture, so baked shading such as the shadows on walls and
  under bridges appeared as the car closed in; opponents dropped their wheels
  past about 14 m, their shadow past about 65 m and vanished completely past
  about 175 m (in the mirror: no wheels, shadow to 52 m, car to 83 m),
  against a view of about 580 m. With
  Extended on, course tiles keep full detail at any distance and opponents
  keep their full model (the mirror its body and shadow) until the depth
  range culls them. Only the drawn models change: the cars' trail effects
  keep the original steps, so races, replays and the random sequence are
  unchanged (car-state traces identical before and after in Time Attack on
  four courses, Grand Prix at the field rate and the original rate, and
  Versus), and with Original draw distance every picture is byte-identical.
  The course's near subdivision levels are left as they are: forcing them
  further out opens seams between tiles. Cost at 8x, hidden Metal: game
  frame work +0.2 ms in a Grand Prix (4.7 to 4.9 ms mean), +0.04 ms in Time
  Attack.

- **The 1x copy of the VRAM is drawn only when the game reads it.** With a
  scaled picture every batch used to be drawn twice, at the scale and into
  the native 1x VRAM that serves read-backs (the replay blur's copy of the
  previous frame, 24-bit movie frames, palettes, StoreImage). A batch that
  draws only into the shown buffers now keeps its 1x draw queued and draws
  it, in order, only when something reads or overwrites those pixels; race
  frames nobody reads are never drawn at 1x. Captures of the attract mode,
  Time Attack, Grand Prix (with the replay blur switched on), Versus and
  their menus, movies and transitions at 1x, 4x and 6x + antialiasing are
  byte-identical to before (40,380 pictures), and a diagnostic shadow 1x
  VRAM (`PSYZ_NATIVE_VERIFY=1`) matched at every one of ~600,000 checked
  reads. On Metal the scaled target no longer has a second copy for
  semi-transparency (framebuffer fetch does not use it): half the scaled
  VRAM memory (242 MiB less at 11x). The GPU's command buffer rollovers
  were audited for lost or reordered draws (none found); a test now forces
  one before every render pass and compares the pictures.

- **Resolution follows the window; antialiasing and high scales stay
  affordable.** Internal resolution has a new default, Auto (window): the
  game draws at the whole multiple of the PlayStation picture that matches
  the window's drawable height (5x at 1080p, 9x at 4K or a Retina
  MacBook's fullscreen), chosen again after a resize. Whatever is selected,
  the picture drawn is held to about twice the window's pixels (at most
  12x): a saved 8x with antialiasing drew a 16x picture (4096x3840, in a
  16384x8192 scaled VRAM) even for a 1080p window; it now draws 6x there
  and 11x on a 3456x2234 display. Settings files keep their value
  (`internal_resolution = "auto"` is the new default; 1-8 stay valid).
  The GPU's time for a Time Attack race frame on an M5 Max (Metal System
  Trace): 8x + antialiasing 5.5 ms (p99 9.1) -> 3.2 ms (p99 6.3) at
  3456x2234, 5.8 ms (p99 9.0) -> 1.0 ms (p99 2.6) at 1920x1080; Auto
  without antialiasing at 1280x720 0.3 ms. docs/PERFORMANCE.md has the
  measurements.
- Palette loads are batched: a race frame loaded a few dozen palettes
  some hundred times, each a separate GPU copy (a compute dispatch on
  Metal); now a palette is copied once until the VRAM changes and the
  copies go to the GPU together. The frame's copy time went from 1.5 to
  0.4 ms on an M5 Max; captures are pixel-identical.

- Clean-up, no change in play: the race and car modules no longer carry
  the calls of the removed PS1 CPU time model (`RR_CPU_CYCLES`,
  `RR_WORK_CALL`, the course objects' per-segment clock table and their
  wrappers); `include/rr/cpu_work.h`, `cpu_time.h` and `ps1_sizes.h` are
  gone. PsyZ drops its unused DMA controller model (`Psyz_DmaWrite`), its
  sound mixer no longer copies the 512 KB sound RAM through the stack on a
  hot reset, and its log says "SPU mixer initialized". Headless runs of the
  attract mode, Time Attack, Grand Prix and Versus give the same state
  traces, pictures and mixed audio as before.
- Race frame rate: the player's car drives exactly as at 25/30 fps, and
  its motion is smooth at 60 fps. The car's game tick is computed whole on
  the tick's first field at the original rate. That field draws the car
  half way from the previous tick, and the second field draws the tick's
  result. Replaying the same stick inputs at both rates, the car's speed,
  revs, heading, pitch/roll and position match at every tick over three
  NTSC laps. With the closed-loop reference driver,
  Time Attack laps on three courses land within 0.03% of the original
  rate's (two courses identical to the millisecond). `RR4_AUTODRIVE=3`
  (diagnostics) is the reference driver used for these comparisons.
- Clean-up, no change in play: with the player car's tick computed whole at
  the original rate, the per-field player physics of the earlier field-rate
  work is gone (about 650 lines): the player's steering, engine, grip,
  drag, heading and normal / slide / spin-out code is the released code
  again, without the slide and tick look-ahead copies, the tick-end
  landing slots or the second field's take-back. `RR4_CAR_FIELD_STEP` is
  removed (at field rate the opponents' stages and contact always run
  every field). Replays no longer record the second field's inputs, which
  nothing reads: new replay files have no FieldInputs block and older
  builds report them incomplete; older field-rate files still load. Race
  frame rate off still runs the original rate. Headless NTSC runs of Time
  Attack on three courses (reference driver), a Grand Prix through its
  replay and results, Versus 2P and Extra Trial with its replay, at both
  race frame rates, give the same state of every car, camera and rand and
  the same picture on every field as before (two frames of the Grand Prix
  results screen also differ between two runs of the same build), and the
  open-loop stick log still matches the original rate at every tick end.
- **60 Hz by default: the game runs at the NTSC original's speed.** Game
  timing (Escape > Graphics) now defaults to NTSC: the European disc runs
  at 60 fields a second with the Japanese/US release's timing, so with Race
  frame rate on a race is 60 game frames a second, and the cars, timers,
  countdown, fades and attract scripts run as in the NTSC original (the
  PAL release ran 5/6 as fast at 50 Hz). At NTSC timing the 34 car records
  the PAL release retuned (gear record 4's speed, top speed, target
  frames; the first car of each maker, docs/REGIONS.md) now take the NTSC
  release's values, so those cars handle as in the NTSC original (earlier
  NTSC timing raced them with the PAL data); the splash turns the display
  on at its second frame as the NTSC release does. PAL stays available
  (Game timing: PAL, or `RR4_VIDEO=pal`). The settings file's
  `video_timing` key, which every earlier file wrote as "pal" (then the
  default), is dropped and replaced by `game_timing`: settings files from
  0.3 and before start at NTSC, and a player who wants PAL chooses it again.
  Replays saved with the earlier NTSC timing (PAL car data) are refused.
  Movies (25 frames a second on the PAL disc) keep their own clock on the
  60 Hz display: each picture is shown at the first field at or after its
  time, two or three fields (2-3-2-3-2, 24.99 fps over the opening movie),
  in step with their sound; the opening movie lasts 130.5 s at either
  timing.
- Fixed: with Race frame rate on, the Grand Prix crashed on the first
  fields of a race on course 7 (a rotating beacon light was drawn as the
  start banner with a missing model). A race now also always starts on a
  whole original frame, as on the console.
- **No more PlayStation timing emulation: fast loads and a responsive
  game.** The CD drive, movies, memory cards, sound transfers and the GPU
  no longer imitate the console's hardware delays, and the game's code no
  longer waits out the console's CPU time. The game reaches the title
  menu (opening movie skipped) in 6.9 s instead of 16.4 s; the 6-second
  black screen between the Namco logo and the opening movie is gone (the
  movie now starts 4.3 s after the first frame instead of 12 s); menu
  loads such as Time Attack's course select take about a second instead of
  four and a half. Saving and loading in Save and Load finish at once.
  Waits the game itself counts (logos, the race's opening banner) keep
  their length. Movies play at their own rate (25 frames a second, 30 on
  the Japanese disc) on the host's clock, in step with their sound, each
  picture for an even two fields (a one-field picture about every 4 s where
  the movie's 25 fps and the display's 49.9 Hz drift apart), and their
  pictures are decoded on the PC as they are needed.
- Headless test runs (`PSYZ_HEADLESS`) use a deterministic field clock:
  time passes only in the game's waits, so a run's fields are the same every
  time. `PSYZ_CD_TRACE=1` logs the drive's commands and deliveries; audio
  underruns are logged as warnings.
- Removed: `PSYZ_CD_TIMING`, `PSYZ_CARD_TIMING`, `PSYZ_SPU_TIMING` (the
  instant behaviour is the only one now), the `RR4_FUNC_MARKS` build and
  `RR4_MARK` diagnostics, and the DuckStation timing-fit tools
  (`ds_compare.py`, `ds_mark.py`, `ds_stall.py`).
- **Races at 60 frames a second, without interpolation.** Graphics >
  Race frame rate (default on; `race_field_rate` in the settings file,
  `RR4_RACE_FIELD_RATE`) shows a new race picture every video field. Every
  car, the player's and the opponents', runs the original's whole frame on
  the frame's first field and is drawn half way between frames, so the
  driving, the opponents, lap times and replays are the original's exactly;
  the start's orbit, the replay and fly-by cameras, the course's moving
  objects, fireworks, trails, flashing lights, scrolling textures, HUD
  pulses and the engine sound move every field at the original speed. The
  race's timers, countdown, voices and lap timing keep the original timing.
  The link race and the attract demo stay at the original rate.
- **Frame interpolation removed.** The 60 / 120 FPS frame rate setting
  guessed in-between pictures from the PlayStation's draw lists, which drew
  visible artifacts and made movies stutter. Every game frame is now
  presented once, at its own field. (Native 60 FPS will come from running the
  game at 60 frames a second, not from in-between pictures.) Settings files
  from 0.3 load as before: their `frame_rate` line is ignored.
- Fixed: with 60 FPS selected, menus, the title screen and other 2D screens
  were shown squashed (too wide, with bars above and below) in both
  Widescreen and Classic 4:3.
- Fixed: with 60 FPS selected, movies stuttered: their frames were shown
  late or skipped while the next one decoded.
- The game no longer waits for the display when the swapchain is full
  while the frame limiter paces it (VSync present mode on a display at about
  the game's rate, e.g. Windows/D3D12); that frame is skipped instead of
  stalling the game's timing.
- `RR4_CAPTURE_DISPLAY=1` (diagnostics) also captures the final presented
  1280x720 image of a hidden run at each `RR4_CAPTURE_AT` field.

## 0.3-alpha

- **Smooth 60 FPS at high resolution** (issue #5). With 60 FPS presentation
  at 4x, widescreen and either draw distance, races no longer drop frames on
  our test Mac (1% low ~50 FPS, was ~27; no frame over 25 ms, was dozens).
  The interpolation's lookup tables use a proper hash, its junction search
  runs on worker threads (`PSYZ_INTERP_THREADS`), and the presentation
  deadlines are checked during long draw-list work, so in-between frames are
  no longer late or skipped. The interpolation tables have twice the room.
- **Extended draw distance no longer changes the game's speed**, and no
  longer crashes late in a Grand Prix race at 4x as 0.2 could.
- `PSYZ_PRESENT_LOG=<file>` writes one line per presented image, to report
  frame pacing (Windows numbers are welcome).
- **Console-accurate timing.** The game now runs on the PlayStation's own
  timeline: CPU time of the race, menus and movies, the CD drive, the memory
  card driver, the pads, the sequencer and XA audio are timed as measured on
  the console (DuckStation in its exact interpreter mode is the reference).
  33 of the 35 compared scenarios — boot, menus, card paths, Time Attack,
  Grand Prix, Versus, Extra Trial, replays and results — make every
  decision on the same video field as the console. Random events seeded
  from timing (such as the race music picked on course 7) now come out as
  on the console.
- Fixed: a Grand Prix could hang when an XA music seek failed; long replays
  stalled after about nine minutes (a 64-bit clock overflow); the course 7
  load could stall for over two minutes; course 7 played a different music
  track than the console; opening a pulled-out memory card could succeed;
  the PocketStation probe and card file creation took the wrong time.
- The DualShock and digital pad follow libpad's real state machine,
  including the second pad's exchange and the DualShock's motors.
- XA music is decoded and resampled exactly as on the console.
- The original's null-pointer reads see the PS1's low memory, as on the
  console, instead of host memory.

## 0.2-alpha

- **Escape > Graphics > Draw distance** offers Original (default) or
  Extended (2x). The extended view draws additional course scenery and
  doubles the renderer's depth range, including mirrors and split screen.
  It costs more CPU and GPU time and is saved in the desktop preferences.
- The application icon now uses [amame-a's contribution](https://github.com/khasinski/ridge-racer-type-4-pc/issues/2),
  with matching macOS, Windows and Linux icons and the Orbitron font notice.

- The **Japanese release** (R4 - Ridge Racer Type 4, SLPS-01800, also the
  JogCon bundle) runs from its own disc: 60 Hz, its 240-line picture,
  Japanese text, R4.BIN layout, card saves (`BISLPSP01800RR4_*`) and record
  passwords. A Time Attack race matches the console (DuckStation) field for
  field; the other modes are ported from its executable but not yet
  compared (docs/REGIONS.md).
- **Escape > Controls** chooses the controller each pad emulates: digital pad
  (default, as before), DualShock (analog left stick steering), neGcon
  (stick twist, analog I/II from the triggers) or JogCon (stick turns the
  dial). The game recognises them as on the console, including its
  controller setup and neGcon calibration screens. Steering linearity and
  gamepad rumble for the DualShock's and JogCon's motors are in the same
  page; dead zone, saturation, linearity and scaling of the steering,
  throttle and brake axes are in the `[input]` section of the preferences
  (DuckStation's neGcon terms).

- **Escape > Replays** saves race replays to files and plays them from the
  title screen. Saved replays retain the race setup, textures and effects;
  files are checked for damage and build/video-timing compatibility.
- **Escape > Link cable** connects the original two-console battle mode over
  TCP, with host and join settings. Network play is experimental.
- PsyZ uses bounded GPU upload buffers, fixes GPU memory growth at high
  resolutions, and makes geometry correction and frame interpolation
  independent of address randomization and primitive batch boundaries.
- Systems without an SDL GPU device can draw at the original resolution
  through the CPU rasterizer; unsupported enhancements are disabled in settings.
- Startup checks the selected disc, recovers from missing or incompatible
  saved paths, and keeps memory cards and logs beside the preferences.
  Windows memory-card formatting and native MSVC builds are supported.
- Opponent initialization clears the complete native car array, including
  fields beyond the original PS1 allocation. Leaving a saved replay no
  longer runs physics against the restored but unloaded title-screen cars.
- Linux packages now include AppImages for x86-64 and ARM64. Release builds
  omit diagnostic hooks and include the source revision and license texts.

### Known limitations

- This remains an alpha. Windows and network play have less gameplay coverage.
- Japanese Time Attack has been compared against DuckStation; the remaining
  Japanese modes have not yet been compared field for field.
- A full VS replay comparison found four frames with differences of 6–23
  pixels. Time Attack and Grand Prix replay comparisons were pixel-identical.
- Automated wall-clock CD streaming can stall during the opening movie;
  network timing tests currently require an instant-CD diagnostic workaround.
  Interactive behavior on that path has not been verified.
- Frame interpolation can still produce artifacts on newly visible geometry;
  enhancements remain off by default. US discs and CHD images are unsupported.

## 0.1

First public build of the native port. It is an early alpha: expect bugs.

- The whole original game — resident program and every mode module (Grand
  Prix, Time Attack, VS battle, Extra Trial, garage and emblem editor,
  records, options, movie playback) — runs natively on 64-bit macOS, Linux
  and Windows on our PsyZ fork, from your own **European (PAL) CUE/BIN** disc
  image of *Ridge Racer Type 4* (`SCES-017.06`). Game data is not included.
- Every mode, the full Grand Prix season with its ending and unlocks, races,
  replays, VS split screen, the memory card screens and the attract demo have
  been checked against DuckStation reference runs. At 1x internal resolution
  frames are pixel-identical to the PS1 GPU; disc loads, the opening movie,
  memory card operations and the game's frame timing follow the console.
- Sequenced music, sound effects, CD audio and movie audio play through the
  emulated SPU.
- Memory cards are kept as `bu00`/`bu10` folders (in the working directory,
  or beside the preferences when the macOS app is started from Finder);
  saves are interchangeable with emulator memory cards.
- **Escape** opens a settings menu:
  - Graphics: internal resolution (1x to 8x), fullscreen and a Fullscreen
    50 Hz display mode, VSync, aspect ratio (Classic 4:3 or Widescreen 16:9
    for races, menus stay 4:3), dithering (off gives full colour),
    antialiasing, texture filtering, geometry correction (perspective-correct
    textures, no wobble or road gaps), frame interpolation (in-between frames
    in races) and video timing (PAL 25 fps races, or NTSC 30 fps races with
    the NTSC release's timings; applies on the next start).
  - Keyboard and gamepad bindings, master volume and the disc image.
  - Settings are saved to `ridge-racer-type-4.toml` (portable when that file
    sits beside the executable or the app).
- Without a disc argument the game starts the image chosen last time, or asks
  for the CUE file with a file dialog.
- Release packages: a signed and notarized macOS app (Apple Silicon), Linux
  x86-64 and ARM64 builds and an experimental Windows x86-64 build.

### Known limitations

- Serial link (two-console) play is not supported.
- Enhancements are off by default. Frame interpolation can still show small
  seams or holes in in-between frames (mostly in replays); turn it off for
  the original picture.
- Only the European (PAL) release is supported; other regions and CHD images
  are not.
- Windows is experimental and has had less testing than macOS and Linux.
