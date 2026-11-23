# Technical debt and known risks

This is the register of known bugs, likely bugs, fidelity gaps and technical
debt in the port and its PsyZ fork. It was compiled on 2026-11-12 from a
read-only audit of `src/main`, `src/overlays`, `include`, `src/port`, `tools`,
CI and `third_party/psyz`, plus the open items left by the DuckStation
comparison work.

Each entry has an ID, a severity and a location, and says how to fix it. When
you fix something, delete its entry in the same commit. When you find a bug,
check whether the same pattern occurs elsewhere and record that too.

Severity:

- **High**: can visibly break the game, lose data or stop it from starting
  for some players.
- **Medium**: wrong behaviour in a narrow case, undefined behaviour that
  happens to work today, or debt that is actively hiding bugs.
- **Low**: cleanup, robustness or a diagnostic-only problem.

Entries marked *(speculative)* are plausible from reading the code but have
not been reproduced.

## 1. Bugs in the game modules

None open.

## 2. Bugs in the host layer, tooling and PsyZ

| ID | Sev | Where | Problem | Fix |
| --- | --- | --- | --- | --- |
| H1 | Medium | PsyZ `frame_interp_impl.h`, `sdl3_gpu.c` (Windows) | Issue #5: 60 FPS at 4x with the extended draw distance is measured only on the hidden Metal renderer (docs/PERFORMANCE.md, 2026-11-22). D3D12's target-copy blending, its VSYNC swapchain acquire (`SDL_WaitAndAcquireGPUSwapchainTexture` blocks the main thread when two frames are in flight) and the timer are unmeasured; `InterpFinishPending` (ghost/new-face placement) is still sequential on the main thread. | A Windows tester's `PSYZ_PRESENT_LOG` / `PSYZ_FRAME_STATS=100` run at 4x Extended; if the swapchain acquire shows up, acquire without waiting at the 60 FPS deadlines and keep the output for the next poll. |

## 3. Fidelity gaps against the console

The port has no PS1 timing (docs/ARCHITECTURE.md, Time): loads, card
I/O and the CD drive take the host's time, so the per-field comparisons
with DuckStation (`tools/port/ds_frames.py`, `ds_script.py`) no longer line
up from the boot on and are not a measure of fidelity. Game logic and
rendering are compared by what the game shows and does.

| ID | Where | Gap |
| --- | --- | --- |
- On DuckStation's exact timing every card scenario but the two Versus ones matches in every field: `save-load` and `card-pulled` (they differed from 4414) and `unformatted` (from 4313) now too. The BIOS's file creation is the driver's four turns of the port (`Psyz_CardFileCreate`: directory frame 0 read at the port's next turn, a probe, the file's directory frame written at the third, a probe; DUCKSTATION_CARD_TRACE: Save and Load 4406-4412, the Garage 5335-5341, the data's first frame at the turn after) and returns 12.5k clocks after the last probe's SIO interrupt, 18.1k after that turn's vblank exception (`BIOS_CARD_CREATE_END_CYCLES`; a card wait now resumes once the exception that ended the request has returned). The BIOS file layer's CPU time on a card file is charged (`BIOS_CARD_*`, `ds_frames.py` `DSF_MARKS` on the kernel's B(32h)-B(36h) at 0x2958-0x2e00): an open for reading 26.2k, for writing 21.2k, an FASYNC read's or write's start 18.7k / 18.2k, a close 0.8k-4.1k. The create as four turns alone (the earlier attempt) matched Save and Load but put the Garage's save a field late from 5343: the Garage's display is interlaced, and libetc's VSync(0) waits until GPUSTAT's line bit differs from its value at the call; on DuckStation the frame after the create calls it 135.6k clocks after the blank (44k of the card menu's own drawing after CardWriteFile returns), past the display range's start, where the port, without the BIOS's and the menu's CPU time, called it in the blank, read the other parity and waited a field more. The card menu frame's routines are charged now (F13), and with them a VSync(1) query takes its 99 clocks instead of a scanline, which had put the Garage history screen's last frame (DuckStation: VSync(0) 0.7k clocks before the blank) past the blank at 3393. Since the boot's loop model and the MDEC model (F6) `vs-slot2-only` matches in every field and decision and `vs-two-cards` differs from 3518 (the Versus movie's ReadS 67 clocks early, blank 1336; F6). Before that: `vs-slot2-only` differed in one field's submode at 7601 (from 3901: the Versus movie's frames start on DuckStation's clock again after a stream sector in the vblank's exception, F14) and `vs-two-cards` differed in two fields from 7296 (a blocking card access a frame early; it matches in every field since the root counter exception's exit, F14): the first decision differences are a blank 576 clocks into a stream sector's exception that DuckStation's libpad misses (`vs-slot2-only` 871; the port's sector 111 clocks later, F14) and, in `vs-two-cards`, card driver turns at 469 and 1057, 1141, 1329 and 1349 whose differences cancel out in the driver's port alternation (sharing the timer's tick with a stream sector's exception, held back, puts 1057, 1141 and 1349 on DuckStation's side and leaves 1329, so the PocketStation probe at 3502 comes a field early: 3518). |
| F8 | JogCon | Out of scope by the user's decision (2026-11-18): the JogCon's fidelity against the console (its motor commands, their echo in the packet, the force-feedback pattern) is not pursued. It goes through the same libpad state machine as the other controllers, with a byte-level model of DuckStation's `jogcon.cpp` answers; it is detected and steers, and a Time Attack run's motor byte matched DuckStation's in the fields compared, but nothing more was checked. |
| F9 | NTSC mode | NTSC timing (the default, `RR4_VIDEO=ntsc`) is the PAL game at 60 Hz with the NTSC release's frame-rate values and car records, not the Japanese release (which the port runs from its own disc, `docs/REGIONS.md`). Left PAL: release changes that are not frame-rate conversions (`docs/ARCHITECTURE.md`): the splash's full-screen clear (black behind the logo, nothing visible), the menus' dialogue waits (ModeSelect/NewCar 501 frames, JP 301), the demo fade values (206/140, JP 104/64), the ranking tie rule, and `StepRaceFrame`/`ResolveCarCollisions` testing a flag where the Japanese code tests `GameModeType` (opponent projections, link knock-back; effect per mode not worked out). |
| F10 | Frame interpolation | A face that is new in a frame is placed by the motion of the paired vertices next to it, which can shear it: in the attract sweep along a car (frame 2740, 1x and 4x) the rear wing coming into view is drawn as a skewed plate for one field. The same sweep scores 4–6% unexplained pixels, most of it the car's texture moving farther than the metric's reach. Outlines of a wall entering the view (no pose in the frame before) float a pixel or two above its edge (race frame 6842, also in HEAD). Thin lines that remain in the seam scan are thin geometry (poles, gantries) and a light texel column at a near wall's edge (chase frame 3292, also in HEAD). Cuts, level-of-detail switches, near-plane crossings and newly visible faces are handled by holding the previous frame or by snapping to paired neighbours, not by a true midpoint. The midpoint no longer depends on how a frame is split into batches: its geometry is kept for the frame and placed once the frame is complete (a race frame at 8x queues up to about 89,000 vertices, a Time Attack replay frame up to about 165,000; the store holds 262,144). It is still split, with a warning and a `PSYZ_INTERP_STATS` count, when the store or the CLUT cache fills or a VRAM write reaches what it samples or draws over; the 8× all-effects GP run reproduces CLUT exhaustion, with its first split warning at game frame 7558. Earlier forced-small-batch checks used 16 primitives (`PSYZ_BATCH_PRIMITIVES`, 1,500–2,300 a frame) or 4,096 vertices (`PSYZ_BATCH_VERTICES`); they do not establish that the full-settings run stays unsplit. The fixed tables count what they drop (`PSYZ_INTERP_STATS`, a warning the first time): the old pair-ring allocator (`INTERP_PAIRS`) lost 1–20 live pairs a run (most in VS split screen and replays), which then started as new; the identity hash probes 64 slots. The 8× all-effects audit reproduced pair and ghost-history vertex-store warnings. Pair allocation now searches the whole ring and protects queued tags; ghost banks grow to six vertices per pair. Both defects have failing-old/passing-new regressions, and the full GP capture run reaches 46923 retained vertices without pair/store/order overflow. The full-settings TA/replay/retry and VS runs also complete without interpolation table overflows; reset/allocation-failure boundaries and other modes still need broader coverage. Packet address maps now probe to an empty slot and compact stale entries: a randomized churn test retains all 6000 live tags, and full game captures match across two packet-arena layouts (see `STABILITY_AUDIT.md`). The same scripted run now draws the same frames and midpoints every time; before, three runs of one build gave three different sets of midpoints, and a rare frame showed a hairline seam of wrong texels, which is what made 16-primitive batches (`PSYZ_BATCH_PRIMITIVES=16`) look different from normal ones. With 16-primitive batches the attract run at 2x (402 captures) and the race frames checked (3840–3860) are identical to normal batches. The newer independent 60/120 FPS path presents to a hidden 1280×720 GPU target in headless GPU runs and reports actual/generated output rates. Older headless measurements included midpoint drawing but omitted final presentation; they are not directly comparable to this path. |
| F11 | Widescreen | The squeeze also applies to native VRAM. StoreImage/MoveImage read-backs of the display buffers therefore see squeezed pixels, and 1x loses horizontal resolution. The double buffers are found by a VRAM layout rule. Widescreen exists only on SDL_GPU. |
| F12 | Japanese release | Verified against DuckStation for the Time Attack race and the front end up to it; the front end reaches each mode 1 to 6 fields early (a splash frame and the menu loads, `docs/REGIONS.md`). Grand Prix (dialogue, results, epilogue credits), Extra Trial, VS, garage, records, options and card saves are ported from the Japanese executable but not yet compared field for field. |
| F15 | Other controllers, `ds_script.py` | `ds_script.py` numbers its lines like `ds_frames.py` (DuckStation frame - 295; it used 225, which with the port's two cards against DuckStation's one happened to line up the boot) and sets a press one field ahead for every controller (it set a DUAL SHOCK's two ahead, which put its presses a field early once the pause was known to come before the blank). Its stick steps (`lx`/`ly`/`rx`/`ry`) bind DuckStation's half axes so that the byte matches the port's linear mapping (DuckStation scales by its analog sensitivity, 1.33). With these the digital pad and the DUAL SHOCK match from the boot on (F6). The neGcon and the JogCon (F8) were not compared again. |

## 4. Structural debt

| ID | Sev | Where | Debt | Fix |
| --- | --- | --- | --- | --- |
| D8 | Medium | PsyZ history | The fork has 108 non-merge commits (+51.5k/−4.4k lines) since upstream 6d9c573, with Parasite Eve, PE2 and Crash branches mixed in. Port changes sit in the decomp `libsnd`/`libspu`/`libgpu`/`libcd` sources under `#ifdef __psyz`, and the public headers grew (`video.h` +319). Rebasing on upstream is expensive. | Keep RR4 changes in topic commits. Upstream generic pieces (drive model, libcard, controller types) separately. Write down the upstream base. |
| D11 | Low | `include/rr_asm.h:89-101` | 12 of the 13 GTE `.word` string macros are unused. `RR_NOP_DEP*` are empty. The macros pun through `*(u32 *)` on unaligned data. | Delete the unused ones; turn the rest into `static inline` with `memcpy`. |
| D13 | Low | game code | 478 uses of 168 `D_80xxxxxx` names and 44 `unk*` fields; worst in `sound/sndque.c`, `demo/opening.c`, `ui/hudmap2.c`, `menu/mnpulse.c`, `menu/mnrows.c`. | Rename as the code is understood. |
| D14 | Low | `src/overlays/*` | About 50 resident globals are defined in overlay files (`PlayerCars`, `Options`, `RaceRecords`, …) and are deliberately not reset with the overlay. | Move them to `src/main`. |
| D15 | Low | `drawlist.c:74` (~950 lines), `objdraw.c:65` (690), `ovl665_update_controller_options` (547), `gtepoly1.c:840` (473); duplication between overlays (`serial_link/car_selection.c` vs `versus/preview.c` ~200 lines, `time_attack/preview.c` vs `extra_trial/preview.c` ~137, `grand_prix/race.c` vs `extra_trial/race.c` ~110) | Long functions and copied code. The copies exist in the original game; under the porting contract this is a reading aid, not a merge target. | Leave them as they are; split a function only when it is being fixed anyway. |
| D16 | Low | `include/rr/memory.h:14` | `_copy_memcard_patch` is declared but never defined. | Delete it. |
| D18 | Low | `include/rr/runtime_settings.h:22` (`Options.storyBranch3C`) | Named and commented as an "epilogue story branch 0..4", but it is the PAL language: `ModeSelectMenuStep` steps it through 5 values, `LoadDialogueStep` reads a fifth of each dialogue entry by it, `trkld8.c` and the credits pick one of 5×4 team sets, and the Japanese release has none of these (`docs/REGIONS.md`). | Rename it (`language3C`) and fix the comment and the users. |

## 5. Tests, CI and releases

Automated game runs (scans, captures, frame-time and memory runs) go
through `tools/port/run_capped.sh`, headless (`PSYZ_HEADLESS=1` or `gpu`,
`SDL_AUDIODRIVER=dummy`). At most `RR4_RUN_SLOTS` (default 2) capped runs
execute at once machine-wide; the others wait. It kills the game when its
physical footprint, which on macOS includes GPU memory that `ps` and RSS do
not show, passes `RR4_MEM_LIMIT_MB` (default 6000), and `RR4_MEM_LOG`
records the footprint over the run. A GPU memory leak once took runs past
100 GB; if a run is killed, find the leak instead of raising the limit.

| ID | Sev | Where | Debt | Fix |
| --- | --- | --- | --- | --- |
| P2 | Medium | CI | No game-level test runs in CI, because they need a disc (`tools/port/regress.sh` runs them locally against a reference commit). The host layer's parsers, the disc identity check and the startup's handling of images that fail are unit-tested without one (`tests/port/startup_test.c`, `diagnostics_test.c`); the message boxes and the file dialog themselves are not. | Run `regress.sh` before every release; a redistributable test image would let CI boot the game. |
