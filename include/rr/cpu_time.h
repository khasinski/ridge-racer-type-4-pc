#ifndef RR_CPU_TIME_H
#define RR_CPU_TIME_H

/* PS1 CPU time of a game routine. Native code runs in no emulated time, so
 * the routines whose work decides whether a frame fits in its field (the
 * loading screen's particles and text, a load's bank setup) spend what they
 * took on the PS1. SDK and BIOS calls take theirs in PsyZ
 * (third_party/psyz/psyz/src/timing_profile.h); the movie player's VLC
 * decode and frame wait spend theirs with Psyz_SpendTime
 * (src/overlays/movie_playback/stream.c). On a real-time clock none of it
 * is spent.
 *
 * Every cost is in system clocks (33.8688 MHz), measured on DuckStation
 * (Ridge Racer Type 4 PAL). How, per entry:
 *   [rec]  the routine's entry-to-return clocks on DuckStation's recompiler,
 *          the CPU mode of the reference runs, with tools/port/ds_calltime.py
 *          (a timing stub patched into RAM, no breakpoints); the median of
 *          its calls in the context named. A routine whose cost leaves out
 *          the routines it calls is measured with them, so these are
 *          inclusive unless the entry says otherwise.
 *   [rec ratio]  a cost without its callees, fitted with breakpoints: the
 *          breakpoint value scaled by recompiler / interpreter clocks of the
 *          whole routine in the same context (ds_calltime.py, DUMMYBP=1).
 *   [call] entry-to-return clocks measured with GDB breakpoints, which run
 *          DuckStation's interpreter: per routine -45% to +20% off the
 *          recompiler, kept where no recompiler value is better.
 * Each value was fitted in, and is checked with, the per-field DuckStation
 * comparison of a commit:
 *   d1dff76a  the idle attract cycle (loading screen, demo seed fc 4941)
 *   1d28cb93  the splash and opening movie, field for field
 *   00df23a8  loads with a save on the card, the VS movie, the Extra Trial
 *             race load
 * Changing a value changes on which field things happen: compare the
 * per-field game state with DuckStation before and after
 * (docs/ARCHITECTURE.md, PS1 time model). */
enum {
    /* d1dff76a: the attract cycle's loading screen (DS fields 8400-9100) */
    /* IntroScriptStep's own code besides its callees (DuckStation's exact
     * timing, DSF_MARKS on it and every callee, the main menu: 826-870 with
     * the marks' stubs taken out, below; the [call] value 1282 had its
     * callees) [mark] */
    RR_CYCLES_INTRO_SCRIPT_STEP = 848,
    RR_CYCLES_ADD_TILE_PRIM = 184,         // AddTilePrim [rec]
    /* ... outside a race frame (whose own costs were fitted with 184):
     * 171 on DuckStation, SetTile's 5 charged by PsyZ (the main menu, the
     * marks' stubs taken out) [mark] */
    RR_CYCLES_ADD_TILE_PRIM_MENU = 171 - 5,
    RR_CYCLES_ADD_TILE_GRID_2X2 = 222,     // AddTileGrid2x2 [mark], besides its AddTilePrims (321 [call])
    RR_CYCLES_ADD_VERTICAL_GRADIENT = 270, // AddVerticalGradientRect [rec]
    RR_CYCLES_ADD_GOURAUD_RECT = 284,      // AddGouraudRect [rec]
    RR_CYCLES_ADD_LINE_F2 = 125,           // AddLineF2Prim [rec], in a race frame
    RR_CYCLES_ADD_LINE_F2_MENU = 112,      // ... elsewhere [mark] (the [rec] value had a stub)
    RR_CYCLES_ADD_SLANTED_GRADIENT = 290,  // AddSlantedGradientRect [rec]
    RR_CYCLES_UPDATE_PARTICLES = 683,      // UpdateParticles [mark] (840 [call])
    RR_CYCLES_UPDATE_PARTICLE = 40,        // UpdateParticle [mark] (53 [rec])
    RR_CYCLES_DRAW_PARTICLE = 2110 - 171,  // DrawParticle [rec ratio] 0.895, 171 long [mark] (main menu)
    /* DrawIntroRippleSprites: 1756 on DuckStation with the SDK calls PsyZ
     * charges (the port's took 2930 with the [rec ratio] 2064) [mark] */
    RR_CYCLES_DRAW_INTRO_RIPPLES = 2064 - (2930 - 1756),
    RR_CYCLES_UPDATE_FADE_TINT = 99,       // UpdateFadeTint [mark] (112 [rec])
    RR_CYCLES_EMIT_FADE_SPRITE = 158,      // EmitFadeSprite besides its AddSpritePrim [mark] (main menu)
    /* AddTextSprites, and each glyph it draws: 186 a glyph on DuckStation
     * with its SetSprt, SetSemiTrans and AddPrim, which PsyZ charges (56;
     * the main menu's title and fade-out text: 1708 and 2289 against the
     * port's 2294 and 3020) [mark] */
    RR_CYCLES_FONT_DRAW = 105,
    RR_CYCLES_FONT_GLYPH = 186 - 56,
    /* the menus of a Time Attack run (DS fields 283-3266) */
    RR_CYCLES_UPDATE_MENU_SCRIPT = 199,    // UpdateMenuScript [mark] (433 [rec ratio] with its callees)
    /* SetMenuTextRow, entry to return [ret] (tools/port/ds_frames.py
     * DSF_MARKS name:ret, Time Attack's results setup, 55 calls): 159 and
     * 44 a template character (6 to 22: 423 to 1127), the row's format on
     * top (digits and a race time 235, a race time 195, a record row 985,
     * digits 70, a CLUT row 30), an empty row 505 */
    RR_CYCLES_MENU_TEXT_ROW = 159,
    RR_CYCLES_MENU_TEXT_ROW_CHAR = 44,
    RR_CYCLES_MENU_TEXT_ROW_DIGITS_TIME = 235,
    RR_CYCLES_MENU_TEXT_ROW_TIME = 195,
    RR_CYCLES_MENU_TEXT_ROW_RECORD = 985,
    RR_CYCLES_MENU_TEXT_ROW_DIGITS = 70,
    RR_CYCLES_MENU_TEXT_ROW_CLUT = 30,
    RR_CYCLES_MENU_TEXT_ROW_CLEAR = 505,
    /* Time Attack's results setup (InitPostRaceScreen) [ret]/[mark]:
     * ovl660_rank_standings and each ovl660_insert_record */
    RR_CYCLES_TA_RANK_STANDINGS = 209,
    RR_CYCLES_TA_INSERT_RECORD = 1290,
    /* SoundCloseVabsAndReset's steps, entry to the next call [mark]: the
     * SDK calls of ReverbReset (SsUtReverbOff, SsUtSetReverbType,
     * SsUtReverbOn, SsUtSetReverbDepth) and SoundStopAll's own code and
     * calls besides SsUtAllKeyOff (before it, and after it to the return) */
    RR_CYCLES_SOUND_MASTER_VOLUME_RESET = 19,
    RR_CYCLES_RESET_PLAYER_VOICES = 96, // with InitLoopVoices
    RR_CYCLES_RESET_SOUND_CHANNEL_VOL = 30,
    RR_CYCLES_SOUND_QUEUE_CLEAR = 51,
    RR_CYCLES_SOUND_LOOP_RESET = 17,
    RR_CYCLES_REVERB_RESET = 2293,
    RR_CYCLES_SOUND_STOP_ALL = 558,
    RR_CYCLES_SOUND_STOP_ALL_REST = 2245,
    /* 1d28cb93: boot (DS fields 282-600) */
    RR_CYCLES_INIT_DEFAULT_TIME_RECORDS = 10428, // InitDefaultTimeRecords [rec]
    RR_CYCLES_INIT_TIM_TEMPLATE = 2119,          // InitTimTemplate [rec]
    /* InitCourseChoices: the PS1's bzero cleared 0x24 bytes more than the
     * host table holds, at the BIOS fill's 132 clocks a byte (PsyZ
     * BIOS_FILL_BYTE_CYCLES, which the recompiler confirms) */
    RR_CYCLES_BIOS_FILL_BYTE = 132,
    /* SoundSystemInit's bzero of AudioState and RaceContext: their PS1
     * sizes (the executable's arguments) */
    RR_PS1_AUDIO_STATE_BYTES = 0xA8D0,
    RR_PS1_RACE_CONTEXT_BYTES = 1404,
    /* PadUpdate, with the routines it calls [ret]: 1961 in 3771 of 8216
     * calls over the Grand Prix scenario's first 9600 fields (the stubs'
     * clocks included); the calls of 6.6k (loads, the movie) take the
     * previous DrawOTag's list DMA, which PsyZ posts at the DMA's time.
     * Its mark's stub out, 1950: a digital pad in port 1, port 2 empty,
     * both ports' buttons as in the previous call, no repeat, no pulse and
     * no rumble. With a DualShock (libpad's extended controller, in its
     * digital mode too) 48 more: the frame loop from PadUpdate's entry to
     * the first ClearOTagR is 2047 clocks on DuckStation with a DualShock
     * against 1999 with a digital pad in the same menus, the loads and the
     * movies (ds_compare's Time Attack and Grand Prix scenarios, entry
     * marks on the frame loop's calls, no exception stubs, 7 clocks a
     * stub; the port's 1986 matched the digital pad's) [mark]. */
    RR_CYCLES_PAD_UPDATE = 1950,
    RR_CYCLES_PAD_UPDATE_EXTENDED = 48,
    RR_CYCLES_PAD_UPDATE_ANALOG = 0,
    /* ... and its paths, from entry marks on PadUpdate, PadStepPulse,
     * PadUpdateRumble, PadPollPort and ClearOTagR on DuckStation against
     * the port's path in the same call (every field of extra-trial,
     * gp-rules, options, vs-picks and ta-course4 matches; 40k calls, each
     * path's clocks constant to 1). A port with a controller takes 157
     * more in PadUpdate's own loop than an empty one and 75 more in its
     * PadPollPort (Versus's second pad: 232; the 1950 has one); a port
     * whose buttons did not change 27 more than one whose did (the
     * repeat count; the 1950 has both), and the count's wrap (from 34
     * back to 30: every fourth call of a port held still) 20 more; 1999 clocks to ClearOTagR in a menu frame, 2019 and 2039 with
     * one or both ports' wrap. [mark] */
    RR_CYCLES_PAD_UPDATE_PORT = 157,
    RR_CYCLES_PAD_POLL_PORT = 75,
    RR_CYCLES_PAD_UPDATE_HELD = 27,
    RR_CYCLES_PAD_UPDATE_REPEAT = 20,
    RR_CYCLES_PAD_UPDATE_BASE = RR_CYCLES_PAD_UPDATE - RR_CYCLES_PAD_UPDATE_PORT -
                                RR_CYCLES_PAD_POLL_PORT - 2 * RR_CYCLES_PAD_UPDATE_HELD,
    /* PadStepPulse's paths, a port's (the force feedback pattern a race
     * plays through PadStartPulse: kerb, impact, spin): a repeating
     * pattern 59 clocks with a digital pad (output19 from one column) and
     * 66 with a DualShock, a one-shot 61 and 68; a pattern's next row 20
     * more, the repeating one's wrap 3 and the one-shot's end 13 more
     * again (PadStepPulse to PadUpdateRumble 101 with none; the race's
     * 160-277, as the port's paths sum them, in every call of
     * extra-trial's and ta-course4's races) [mark] */
    RR_CYCLES_PAD_PULSE_REPEAT = 59,
    RR_CYCLES_PAD_PULSE_REPEAT_EXTENDED = 66,
    RR_CYCLES_PAD_PULSE_ONCE = 61,
    RR_CYCLES_PAD_PULSE_ONCE_EXTENDED = 68,
    RR_CYCLES_PAD_PULSE_STEP = 20,
    RR_CYCLES_PAD_PULSE_WRAP = 3,
    RR_CYCLES_PAD_PULSE_END = 13,
    /* PadUpdateRumble's paths, a port's: a rumble on 13 more, 39 when its
     * direction stays within the wheel's range (42 with a level), and an
     * actuator bit set from the previous call 40 (PadUpdateRumble to
     * PadPollPort 247 with none, 260-353 in the races) [mark] */
    RR_CYCLES_PAD_RUMBLE_ACTIVE = 13,
    RR_CYCLES_PAD_RUMBLE_DIRECTION = 26,
    RR_CYCLES_PAD_RUMBLE_DIRECTION_LEVEL = 3,
    RR_CYCLES_PAD_RUMBLE_ACTUATOR = 40,
    /* SetupRace's car placement outside a race frame (the race's load, the
     * retry's): PlaceCarOnGrid's own 1490 (the grid, bounds and history
     * stores), FindTrackNode 266 and 250 a node it steps over, the
     * starting distance (ComputeStartDistance) 53 and 23 a node, and the
     * physics stages it runs once: IntegrateCarMotion 1278,
     * ComputeWheelOffsets 487, ConstrainCarToTrack 1035, FindWheelTrackNodes
     * 2663, UpdateWheelBumps 1414. ApplyOptionsPadButtonMaps 1030,
     * InitCourseViewState's lighting and view setup 330, and course 7's
     * animated objects' SetAnimatedTrackObjectState 220 more than their
     * VSync(1) and srand (DSF_MARKS name:ret, the stubs out: Time Attack on
     * course 7 field 2969, the Grand Prix's first race's eight cars, fields
     * 7725-7728 of gp-team2; FindTrackNode 13-65 nodes, the starting
     * distance 8-34, each within 10 clocks). The port charged only the car's
     * bzero there, 12.5k short a car: the objects' rand seed, VSync(1)'s
     * line count (SetAnimatedTrackObjectState), came 6 lines early and
     * course 7's random race music (SelectCourseBgmTrack's rand % 14) was
     * another track than DuckStation's. [mark] */
    RR_CYCLES_PLACE_CAR_ON_GRID = 1490 + 1278 + 487 + 1035 + 2663 + 1414,
    RR_CYCLES_FIND_TRACK_NODE = 266,
    RR_CYCLES_FIND_TRACK_NODE_STEP = 250,
    RR_CYCLES_START_DISTANCE = 53,
    RR_CYCLES_START_DISTANCE_STEP = 23,
    RR_CYCLES_APPLY_PAD_BUTTON_MAPS = 1030,
    RR_CYCLES_INIT_COURSE_VIEW = 330,
    RR_CYCLES_ANIMATED_OBJECT_STATE = 220,
    /* SoundUpdate with the routines it calls but the two below, with no
     * sound slot active and the listener off (attract, DS fields 2000-2200)
     * [rec]; in a race (Time Attack, DS fields 3266-3700) an active slot's
     * UpdateSoundSlot and the listener's car search add these [rec]. */
    RR_CYCLES_SOUND_UPDATE = 1105,
    /* ... split where it runs on DuckStation (DSF_MARKS SoundUpdate:ret,
     * CdAudioUpdate:ret, CdSync:ret, CdControlF:ret and SoundUpdate's
     * callees, Time Attack race fields 3324-8006 and the attract demo, the
     * stubs' 12.5 clocks taken off): 38 before CdAudioUpdate, whose CD
     * command (the XA player's ReadS, its GetlocP every frame) comes with
     * its own path's time, 68 for the fades after it and the rest after the
     * voices' and the queue's updates at its end, so the command's
     * acknowledge and the stream's sectors come at DuckStation's clocks
     * (the whole charge first put them 700-1100 clocks late). CdAudioUpdate
     * on the playing stream: 39 to its switch, 206 for the tick from the
     * TOC position before its CdSync(1), 142 from a completed CdSync to the
     * next GetlocP (CdLastCom, CdPosToInt), and the XA player's step 29 to
     * its CdSync. [mark] (With PadUpdate's paths and the CD path's refit,
     * the race frames' CdFileStep entry to the GetlocP's acknowledge was
     * 42 clocks short of DuckStation's unmarked one in every frame, 1774
     * of extra-trial's and extra-trial-race's, and to the XA player's
     * ReadS 16 long, 4 of 4: 100 and 45 before) [trace] */
    RR_CYCLES_SOUND_UPDATE_HEAD = 38,
    RR_CYCLES_SOUND_UPDATE_FADES = 68,
    RR_CYCLES_CD_AUDIO_UPDATE = 39,
    RR_CYCLES_CD_AUDIO_TICK = 206,
    RR_CYCLES_CD_AUDIO_POSITION = 142,
    RR_CYCLES_CD_AUDIO_PLAY_STEP = 29,
    /* CdAudioReadyCallback in a sector's exception (the XA stream's TOC
     * sector, 32 words fetched): on DuckStation 230 clocks of a 3029-clock
     * exception [mark] (PsyZ takes CdRead's callback out of that sector's
     * exception, CD_READ_CALLBACK_CLOCKS) */
    RR_CYCLES_CD_AUDIO_READY = 230,
    /* UpdateSoundSlot by its work: its own code for each of the slot's
     * voices, SlotInterpCurve twice a voice (72 and 9 for each key it looks
     * at: 81 at a curve's start, 153 past its last key), the utility calls
     * in PsyZ (SsUtChangePitch, SsUtSetVVol, SsUtKeyOnV: timing_profile.h).
     * On DuckStation the player's slot (4 voices) takes 2770-2839 in the
     * menus and the attract demo (its voices not keyed on), 4398 in a race
     * (the median; 4044-4568), 7637 when its program changes (two key-ons),
     * an opponent's (3 voices) 2150 (Time Attack fields 2700-8000, the
     * attract demo 7700-9300, DSF_MARKS UpdateSoundSlot and its callees
     * against the port's slot and voice counts); the flat 3873 of a race's
     * call was 1.6k short there and 1.1k long in the menus. [mark] */
    RR_CYCLES_UPDATE_SOUND_SLOT_VOICE = 410,
    RR_CYCLES_SLOT_INTERP_CURVE = 72,
    RR_CYCLES_SLOT_INTERP_KEY = 9,
    /* SoundListenerUpdate, listener on, with SoundUpdateComSlots: 2695 in
     * Time Attack's race, 3325 with the Grand Prix's eight cars, 7021 in its
     * replays, whose listener follows the camera (DSF_MARKS, DuckStation's
     * exact timing; a flat 2695 before) [mark] */
    RR_CYCLES_SOUND_LISTENER = 2605,
    RR_CYCLES_SOUND_LISTENER_CAR = 90,
    RR_CYCLES_SOUND_LISTENER_CAMERA = 3696,
    /* The boot, measured [mark]: entry-to-entry clocks on DuckStation's
     * recompiler, from a stub at each function entry that writes root
     * counter 2 (unused before SsStart), logged with DUCKSTATION_CD_TRACE
     * (tools/port/DUCKSTATION.md). The BIOS (fast boot, SCPH-1001) starts
     * the executable 10.7 ms into a field; its crt0 reaches MainLoop
     * 6.484 ms after the next vblank, and libetc's first ResetCallback
     * takes 62.3k clocks: SystemInit starts 8.324 ms after that vblank.
     * The port starts its fields there and spends both in MainLoop. On
     * DuckStation's exact timing (tools/port/DUCKSTATION.md) SystemInit
     * starts 717 clocks sooner, 281198 clocks after the vblank (the BIOS's
     * boot, its CD waits, ends sooner where events run at their clock). */
    RR_CYCLES_BOOT_TO_MAIN_LOOP = 219610 - 717,
    RR_CYCLES_FIRST_RESET_CALLBACK = 62308,
    /* CdFileStep, the frame loop's CD file state machine, around its
     * CdSync(1) poll and state handler: to the poll 23, an idle step (no
     * file) 39, after a poll that finds the drive busy 77, the handler's
     * dispatch 45 (CdFileStateStartRead's CdRead call 15 more) and the
     * return after it 51 [mark] (DSF_MARKS CdFileStep, CdSync:ret,
     * CdControlF, CdRead, CdReadSync, SoundUpdate: Save and Load, 1391
     * frames). Without the stubs CdFileStep's entry to a Setloc's CD_cw
     * is 481 clocks on DuckStation (one entry mark each, DSF_EXC=0, their
     * stubs' 7 out; ta-course4's menus and course load), 503 here, and to
     * a ReadN's 966 against 1010; taking them off puts the card scenarios
     * and the loads off (unformatted from 2632): not yet (docs/TECH_DEBT.md
     * F13) [mark] */
    /* The frame loop's own code (MainLoop) between its calls after the
     * VSync: from PutDispEnv to PutDrawEnv and from it to DrawOTag (25 each
     * on DuckStation, 14 without the marks' stubs), to PadUpdate (29), from
     * PadUpdate's return to the first ClearOTagR (the frame counter and the
     * buffers' set-up, 56), between the ClearOTagRs (26) and to CdFileStep
     * (18) [mark] (DSF_MARKS on each, the main menu's 391 frames); not
     * charged, so with the SDK calls' charges fitted with the stubs in the
     * frames started about right in most menus and 120-350 early where
     * the GPU's list was still going */
    RR_CYCLES_FRAME_LOOP_ENV = 14,
    RR_CYCLES_FRAME_LOOP_PAD = 18,
    RR_CYCLES_FRAME_LOOP_TOP = 45 - 9, // (the movies' frames after their DrawOTag's wait kept where they were)
    RR_CYCLES_FRAME_LOOP_CLEARS = 15,
    RR_CYCLES_FRAME_LOOP_STEP = 7,
    RR_CYCLES_CD_FILE_STEP = 23,
    RR_CYCLES_CD_FILE_STEP_IDLE = 39,
    RR_CYCLES_CD_FILE_STEP_BUSY = 77,
    /* (a load's CdFileStep entry to its command's CD_cw 481 clocks on
     * DuckStation's unmarked runs, not 503: 22 of the dispatch's after
     * the command, with the CD path's refit, PsyZ timing_profile.h) */
    RR_CYCLES_CD_FILE_STEP_DISPATCH = 45 - 22,
    RR_CYCLES_CD_FILE_START_READ = 15,
    RR_CYCLES_CD_FILE_STEP_RETURN = 51 + 22,
    /* LoadR4Directory's CdReadSync(1) loop around the call: its turns 709
     * clocks apart on DuckStation with the call's two marks' stubs (about
     * 17), the call 680 [mark] */
    RR_CYCLES_CD_READ_SYNC_LOOP = 12,
    /* LoadR4Directory after its read: the directory's entries to sector
     * positions (CdIntToPos, 76 clocks, and the loop), 80.0k for 950 */
    RR_CYCLES_R4_DIRECTORY_ENTRY = 84,
    /* LoadR4Directory from its CdReadSync loop's end to the parse: on
     * DuckStation's exact timing SsInit starts 82719 clocks after the
     * read's Pause completion (DSF_MARKS SsInit, DUCKSTATION_DMA_TRACE),
     * the work between them as charged [mark] */
    RR_CYCLES_R4_DIRECTORY_READ_END = 171,
    /* The attract demo's race frame, AttractDemoRaceUpdate less the
     * routines that spend their time by their work (rr/cpu_work.h:
     * StepRaceFrame, UpdateTrackObjects, DrawCars, SetupPlayerView,
     * DrawTrackObjects, DrawTrackSceneryForPlayer), interrupts' time taken
     * out: the median of 1618 frames (10th-90th percentile 7957-9183)
     * [mark] (it was the whole frame's least, 502k, fixed) */
    RR_CYCLES_ATTRACT_DEMO_OWN = 8574,
    /* The course menus' map preview (DrawCourseMapPreview: Time Attack's,
     * the Grand Prix's, Versus's): DrawMapBitmapIso's 24x24 sprites of the
     * height map without DrawTrackNodeMap, 74.0k-79.3k by the cells the
     * window shows (courses 0 and 7, 584 frames; the median), and
     * DrawTrackNodeMap's tile a track node (1063 on course 0: 74.1k, 809 on
     * course 7: 56.6k) besides the primitives the port charges itself;
     * DrawCourseInfoPanel's own code past its primitives' (6267 against
     * 3340) [mark]. DuckStation's clocks of DrawMapBitmapIso hold the
     * previous frame's list DMA, which the GPU still takes then: 28.3k of
     * 3319 slices in the port, which stalls the CPU there too, so its own
     * code is 79.4k less those (the port's call took 105.4k); the course
     * menus have half the track nodes counted above (529 on course 0, 563
     * on course 1: 74.0k and 79.0k on DuckStation, 139 a node; the port
     * charged 36.9k) */
    RR_CYCLES_MAP_BITMAP_ISO = 79400 - 28300,
    RR_CYCLES_TRACK_NODE_MAP = 435,
    RR_CYCLES_TRACK_NODE_MAP_NODE_X100 = 13900,
    RR_CYCLES_COURSE_INFO_PANEL = 725,
    /* The Time Attack course menu (mode 1/50), each routine's own code
     * besides its callees charged themselves (DSF_MARKS on each and its
     * callees, DuckStation's exact timing, ta-course1 fields 1893-2193):
     * DrawCourseInfoPanel 725 (its 2927 had held its digit strings and
     * times); DrawDigitString16 118 and 100 a character (218, 518, 1016 for
     * 1, 4 and 9), 26 of them the SetShadeTex and SetSemiTrans PsyZ
     * charges; FormatMillisecondsTime; DrawMenuStepArrows (its two
     * triangles, besides EmitPolyF3's PsyZ calls: 852 against 112);
     * ovl660_update_menu's own (161, 816 with the selection's step);
     * DrawCourseMapPreview's own, DecodeCourseIndex, LoadTrackNodesStep
     * once loaded, SetMenuFrameOptions [mark] (the stubs out). Not charged
     * before: the course menu's frames ended 9k early. */
    RR_CYCLES_DIGIT_STRING16 = 118,
    RR_CYCLES_DIGIT_STRING16_CHAR = 100 - 13 - 13,
    RR_CYCLES_FORMAT_MS_TIME = 79,
    RR_CYCLES_MENU_STEP_ARROWS = 489,
    RR_CYCLES_TA_COURSE_MENU = 161,
    RR_CYCLES_TA_COURSE_MENU_SELECT = 816 - 161,
    RR_CYCLES_COURSE_MAP_PREVIEW = 108,
    RR_CYCLES_DECODE_COURSE_INDEX = 15,
    RR_CYCLES_LOAD_TRACK_NODES_IDLE = 45,
    RR_CYCLES_SET_MENU_FRAME_OPTIONS = 4,
    /* Time Attack's car select (1/51), the same way: EmitPolyF3 besides its
     * SetPolyF3, SetSemiTrans and AddPrim (179 against 56: DrawMenuStepArrows
     * 478-500 besides its triangles), DrawSelectionIconGrid (900-1050),
     * DrawCarSpecBadgePanel (2234-2579), AddScreenFadeTiles (662 against
     * 353), UpdateImageRow's own, ovl660_update_preview's own (2989 against
     * 1997) [mark] */
    RR_CYCLES_EMIT_POLY_F3 = 179 - 56,
    RR_CYCLES_SELECTION_ICON_GRID = 900 - 129,
    RR_CYCLES_CAR_SPEC_BADGE_PANEL = 2234 - 129,
    RR_CYCLES_SCREEN_FADE_TILES = 662 - 353,
    RR_CYCLES_UPDATE_IMAGE_ROW = 91,
    RR_CYCLES_TA_CAR_SELECT = 2989 - 1997,
    /* ... its car's set-up outside a race frame: SetCarBounds (195),
     * SetupViewport (434 against 38), SetupMenuCarCamera (2097 against
     * 1959) [mark] */
    RR_CYCLES_SET_CAR_BOUNDS_MENU = 195,
    RR_CYCLES_SETUP_VIEWPORT_MENU = 434 - 38,
    RR_CYCLES_MENU_CAR_CAMERA = 2097 - 1959,
    /* Versus's car select (1/56) the same: ovl661_update_preview's own (3097
     * against 1997), EmitPolyFT3 besides its SDK calls (325 against 69),
     * UpdateRect16Row (2431 against 224), FormatDecimalDigits (22) [mark] */
    RR_CYCLES_VS_CAR_SELECT = 3097 - 1997,
    RR_CYCLES_EMIT_POLY_FT3 = 325 - 69,
    RR_CYCLES_UPDATE_RECT16_ROW = 2431 - 224,
    RR_CYCLES_FORMAT_DECIMAL_DIGITS = 22,
    /* The garage's name entry (1/28): ovl662_draw_character_grid's own
     * (4313 against the port's 1629 for its callees' charges),
     * ovl662_draw_name_entry_chars' (2114 against 1497),
     * ovl662_update_name_entry's (4457 against 3716) [mark] */
    RR_CYCLES_NAME_ENTRY_GRID = 4313 - 1629,
    RR_CYCLES_NAME_ENTRY_CHARS = 2114 - 1497,
    RR_CYCLES_NAME_ENTRY = 4457 - 3716,
    /* ... and its progress menu (1/40: ovl662_update_progress_menu 12518
     * against 7995) and history (1/66: ovl662_draw_history_arrows 647
     * against 171, ovl662_draw_team_car_counts_label 1954 against 1375,
     * FormatCountOfTotal 153 against 44) [mark] */
    RR_CYCLES_GARAGE_PROGRESS_MENU = 12518 - 7995,
    RR_CYCLES_HISTORY_ARROWS = 647 - 171,
    RR_CYCLES_TEAM_CAR_COUNTS_LABEL = 1954 - 1375,
    RR_CYCLES_FORMAT_COUNT_OF_TOTAL = 153 - 44,
    /* The Grand Prix's screens: the car standings (1/16: AddBadgeSpriteStrip
     * 409, DrawCarStatBars 385, DrawStepProgressBar 282,
     * EmitBadgePatternSprites 2629 against 1562, DrawMenuCarPreviewScene 1547
     * against 927, CarStandingsModeUpdate 392 against 222), the course
     * preview's ovl659_update_preview (285), the garage's car preview
     * (13056 against 11456), the new car (1608 against 1438), the results
     * (DrawResultsRankIcons' 242 cells 24185 less its two AddDrawModePrim,
     * UpdateMenuTextRow 191, DrawResultsStandingsOverlay 487,
     * ovl659_update_results 370), the dialogues (DialogueModeUpdate 168,
     * UpdateDialogueBox 205) [mark] */
    RR_CYCLES_BADGE_SPRITE_STRIP = 409,
    RR_CYCLES_CAR_STAT_BARS = 385,
    RR_CYCLES_STEP_PROGRESS_BAR = 282,
    RR_CYCLES_BADGE_PATTERN_SPRITES = 2629 - 1562,
    RR_CYCLES_MENU_CAR_PREVIEW_SCENE = 1547 - 927,
    RR_CYCLES_CAR_STANDINGS = 392 - 222,
    RR_CYCLES_GP_PREVIEW = 285,
    RR_CYCLES_GARAGE_CAR_PREVIEW = 13056 - 11456,
    RR_CYCLES_NEW_CAR_MODE = 1608 - 1438,
    RR_CYCLES_RESULTS_RANK_ICONS = 24185 - 2 * 129,
    RR_CYCLES_UPDATE_MENU_TEXT_ROW = 191,
    RR_CYCLES_RESULTS_STANDINGS_OVERLAY = 487,
    RR_CYCLES_GP_RESULTS = 370,
    RR_CYCLES_DIALOGUE_MODE = 168,
    RR_CYCLES_DIALOGUE_BOX = 205,
    RR_CYCLES_JITTER_SPRITE = 321 - 56, // AddJitterSprites a sprite, its SDK calls out
    /* The Grand Prix's intro scenes (0/2, StepIntroScene) outside a race
     * frame: StepIntroScene's own (1533 against 38), the title model's
     * DrawTransformedTitleModel (4899 against 3731) and
     * DrawRotatingTitleModel (1893 against 1230), SetLightColorsFromAngle
     * (148), RampValueTo4096 and RampValueToLimit (18), and the scene's
     * rarer steps: DrawMenuGridMesh (73187), the sky's RunSkyCommandList
     * (64395), RenderSkyToRect (959), SetupSkyLightMatrices (322),
     * DrawRotatingSky (266), EmitTitleMaskSprites (127)
     * [mark]; none was charged but by the callees */
    RR_CYCLES_INTRO_SCENE = 1533 - 38,
    RR_CYCLES_TRANSFORMED_TITLE_MODEL = 4899 - 3731,
    RR_CYCLES_ROTATING_TITLE_MODEL = 1893 - 1230,
    RR_CYCLES_LIGHT_COLORS_FROM_ANGLE = 148,
    RR_CYCLES_RAMP_VALUE = 18,
    RR_CYCLES_MENU_GRID_MESH = 73187,
    RR_CYCLES_SKY_COMMAND_LIST = 64395,
    RR_CYCLES_RENDER_SKY_TO_RECT = 959,
    RR_CYCLES_SKY_LIGHT_MATRICES = 322,
    RR_CYCLES_ROTATING_SKY = 266,
    RR_CYCLES_TITLE_MASK_SPRITES = 127,
    /* The results' text rows (DrawMenuTextRow, besides its AddDrawModePrim):
     * 166 and by its characters, a glyph shown 355 (its brightness's
     * rsin/rcos and the divisions), a glyph not yet shown 120, a space 30,
     * a glyph of the two-sprite styles 466 (6501 calls on DuckStation in
     * Time Attack's results, the counts from the port's same calls: a row
     * 1.6k-11k, 71k a frame, which the port did not charge) [mark] */
    RR_CYCLES_MENU_TEXT_ROW_DRAW = 166,
    RR_CYCLES_MENU_TEXT_ROW_GLYPH = 355,
    RR_CYCLES_MENU_TEXT_ROW_HIDDEN = 120,
    RR_CYCLES_MENU_TEXT_ROW_SPACE = 30,
    RR_CYCLES_MENU_TEXT_ROW_GLYPH2 = 466,
    RR_CYCLES_TILE_SEMI_TRANS = 78,
    RR_CYCLES_TA_RESULTS = 350,         // ovl660_update_results' own [mark]     // AddTileSemiTransPrim outside a race frame, its own [mark]
    /* The menus' presentation (StepCameraScript's): DrawPresentationTrackModel's
     * own (its 16 x 16 banner grid and its markers' projections: 18420 on
     * DuckStation in the records' menu), each marker's
     * EmitProjectedMarkerSprt (58), SetupObjectLightMatrices (2233),
     * DrawRingEffects' own (93); none was charged [mark] */
    RR_CYCLES_PRESENTATION_TRACK_MODEL = 18420 - 1900, // its projections' GTE time PsyZ charges
    RR_CYCLES_PROJECTED_MARKER = 58,
    RR_CYCLES_OBJECT_LIGHT_MATRICES = 2233,
    RR_CYCLES_RING_EFFECTS = 93,
    /* The records (1/74, 1/75): ovl663_update_selection's own (187, 763 as
     * it steps the course), ovl663_update_menu's (264), a records table's
     * (ovl663_draw_lap_records, ovl663_draw_race_records: its five rows'
     * blink test and columns, 1642 against the port's 258 for its two
     * AddDrawModePrim); its times as FormatMillisecondsTime [mark] */
    RR_CYCLES_RECORDS_SELECTION = 187,
    RR_CYCLES_RECORDS_SELECTION_STEP = 763,
    RR_CYCLES_RECORDS_MENU = 264,
    RR_CYCLES_RECORDS_TABLE = 1642 - 258,
    /* The options (2/2): DrawHudDigit outside a race (114), the controller
     * page's DrawControllerConfigPanel (551 against 344) and callout strokes
     * (3230 against 1680) [mark] */
    RR_CYCLES_HUD_DIGIT_MENU = 114,
    RR_CYCLES_CONTROLLER_CONFIG_PANEL = 551 - 344,
    RR_CYCLES_CALLOUT_STROKES = 3230 - 1680,
    /* The card menu and a save (Save and Load, 1/72): the save's
     * BuildSaveData (45648) and BuildCardHeaderStatic (8181), the card
     * events' polls (ClearHwCardEvents, GetHwCardEvent, PollHwCardEvents),
     * CardSaveStep, RunCardSaveOrLoadStep, CardUpdate: not charged [mark] */
    RR_CYCLES_BUILD_SAVE_DATA = 45648,
    RR_CYCLES_BUILD_CARD_HEADER = 8181,
    RR_CYCLES_CLEAR_HW_CARD_EVENTS = 238,
    RR_CYCLES_GET_HW_CARD_EVENT = 252,
    RR_CYCLES_POLL_HW_CARD_EVENTS = 285,
    RR_CYCLES_CARD_SAVE_STEP = 71,
    RR_CYCLES_CARD_SAVE_OR_LOAD_STEP = 105,
    RR_CYCLES_CARD_UPDATE = 156,
    /* ... and the card menu's close into Versus's car select (CountChoiceGlyphs
     * 143, UnpackChoiceGlyph 17, CloseCardMenuUiRows 7232 against 6481)
     * [mark] */
    RR_CYCLES_COUNT_CHOICE_GLYPHS = 143,
    RR_CYCLES_UNPACK_CHOICE_GLYPH = 17,
    RR_CYCLES_CLOSE_CARD_MENU_ROWS = 7232 - 6481,
};

/* The race HUD's lap list (DrawLapTimeList): its own and a lap's row with
 * its time (DrawRaceTime) and sprite, DuckStation's exact timing (DSF_MARKS,
 * DSF_EXC, the list DMA's stalls taken out): 4245 a frame on Time Attack's
 * three laps, 7359-7485 on course 7's six. The race modes' own costs
 * (cpuwork.c) included it for the courses they were fitted on, so course
 * 7's frames came 3.2k early and the Grand Prix's two-lap races 1k late.
 * [mark] */
/* The Grand Prix's opening fly-by in a race frame (DrawLensFlareAndSun,
 * RaceState 0): its own besides the calls charged themselves (the slots'
 * pause, the channel volume), the opening overlay of its first 12 frames,
 * the camera track's key search and interpolation (FindCameraTrackTrigger,
 * ApplyCameraState besides BuildCameraMatrices), and the team's showcase
 * objects (RenderSubOtObject: one, two on course 7): an object's light and
 * rotation matrices, transform and projected origin, when it is in front
 * its sub-ordering table's clear, transform again and model bank, its
 * SubmitModel by its work (rr/cpu_work.h, held in a race frame otherwise)
 * and its spotlight cone. DuckStation's exact timing, gp-ending's first and
 * eighth races (DSF_MARKS, DSF_EXC, the list DMA's stalls taken out): a
 * frame's fly-by 5.6k-142k clocks; the Grand Prix's race mode own cost had
 * a flat 75k for it in every start frame. [mark] */
enum {
    RR_CYCLES_LENS_FLARE = 471,
    RR_CYCLES_RACE_OPENING_OVERLAY = 11300,
    RR_CYCLES_CAMERA_TRACK_TRIGGER = 1016,
    RR_CYCLES_SHOWCASE_OBJECT = 2550,
    RR_CYCLES_SHOWCASE_DRAW = 1500,
    RR_CYCLES_SPOTLIGHT_CONE = 4908,
};

/* The race opening overlay's routines outside a race frame (a race load's
 * waiting frames, UpdateRaceOpeningOverlay from UpdateMenuScript), each by
 * its work: DrawFontString 51 and 115 a glyph, 21 a character with none;
 * the centring routines' measuring loop 177 and 21 a character;
 * DrawGlyphList 84 (DrawGlyphListSloped 97) and 189 a glyph; the icon row,
 * AddBrightnessSprite and AddSetTexpagePrim; the overlay's BIOS sprintf of
 * the track number and its own code (the race banner, the mode banner).
 * DuckStation's exact timing, DSF_MARKS name:ret on each and DSF_EXC, the
 * Extra Trial's load and the Grand Prix's first (the marks' stubs, 25
 * clocks a call, taken out: the overlay 7180 and 11267 clocks a frame
 * against 7205 and 11292 with its own mark only). Not charged before, so
 * those frames ended 7-11k early. [mark] */
enum {
    RR_CYCLES_FONT_STRING = 51,
    RR_CYCLES_FONT_STRING_GLYPH = 115,
    RR_CYCLES_FONT_STRING_SKIP = 21,
    RR_CYCLES_FONT_STRING_MEASURE = 177,
    RR_CYCLES_FONT_STRING_MEASURE_CHAR = 21,
    RR_CYCLES_GLYPH_LIST = 84,
    RR_CYCLES_GLYPH_LIST_SLOPED = 97,
    RR_CYCLES_GLYPH_LIST_GLYPH = 189,
    RR_CYCLES_ICON_SPRITE_ROW = 105,
    RR_CYCLES_ADD_BRIGHTNESS_SPRITE = 122,
    RR_CYCLES_ADD_SET_TEXPAGE = 36,
    RR_CYCLES_OPENING_TRACK_NUMBER = 463,
    RR_CYCLES_OPENING_OVERLAY_BANNER = 914,
    RR_CYCLES_OPENING_OVERLAY_MODE = 831,
};

enum {
    RR_CYCLES_DRAW_LAP_TIME_LIST = 1089,
    RR_CYCLES_DRAW_LAP_TIME_ROW = 1052,
};

/* The menus' row, box and dialogue drawing and the card menu's frame, each
 * routine's own code without the routines it calls that are charged
 * themselves (DSF_MARKS name:ret on every routine below and its callees,
 * DuckStation's exact timing, Save and Load's card menu over port fields
 * 4104-4444: 329 frames, the stubs' clocks included) [mark]. A card menu
 * frame (CardMenuModeUpdate) takes 87.9k on DuckStation, 12k before these;
 * the frame after a save's file creation starts its VSync on DuckStation
 * 44k (the Garage's) to 60k (Save and Load's) after CardWriteFile returns,
 * so the game's VSync(0) reads the interlaced field's line parity after
 * the display range has started and the next frame runs a field sooner
 * than the port's, which read it in the blank. */
enum {
    /* AddSpritePrim, brightness set: 201 on DuckStation with SetSprt and
     * SetSemiTrans, which PsyZ charges (PRIM_SETTER_CYCLES,
     * PRIM_SET_ATTR_CYCLES: the port's call took 230); its SetShadeTex call
     * instead 14 more (PsyZ's 13). Here and below a mark's stub is taken
     * out of DuckStation's clocks: 11 clocks of a :ret mark fall between a
     * routine's entry and return marks and 11 in its caller's own (each
     * marked callee lengthened TitleMenuUpdate by 22-25 clocks); the menus'
     * charges fitted with the stubs in were that much long, which put the
     * main menu's frames 1.1k late with all else matched */
    RR_CYCLES_ADD_SPRITE_PRIM = 201 - 5 - 13,
    RR_CYCLES_ADD_SPRITE_PRIM_SHADE_TEX = 14 - 13,
    RR_CYCLES_ADD_DRAW_AREA_PRIM = 309,       // AddDrawAreaPrim
    RR_CYCLES_ADD_DRAW_MODE_PRIM = 129,       // AddDrawModePrim
    /* AddSpriteWithDrawMode besides its two AddDrawModePrim; DrawImageRow's
     * own a cell of its 18 x 4 (a car select's image row 56.3k a frame on
     * DuckStation: 72 of them, 41.4k, and 18 GetClut); UpdateCarWorldTransform
     * outside the race frame's routines (Time Attack's car select, DSF_MARKS,
     * DuckStation's exact timing, the list DMA's stalls taken out; less the
     * marks' own stubs, about 17 clocks a call: the row's 50.5k without its
     * callees marked) [mark] */
    RR_CYCLES_ADD_SPRITE_WITH_DRAW_MODE = 245 - 14,
    RR_CYCLES_DRAW_IMAGE_ROW_CELL = 189, // 13635 a row on DuckStation (its GetClut calls in), the stubs out
    RR_CYCLES_DRAW_IMAGE_ROW_CELL_FT3 = 456, // its triangles' path (the Versus select's close: 32846)
    RR_CYCLES_UPDATE_CAR_WORLD_TRANSFORM = 1895,
    RR_CYCLES_DRAW_UI_ROW = 550,              // DrawUiRow (535-900 by its state, the stubs in)
    /* DrawUiBox besides its AddTilePrim: 2655 on DuckStation with its four
     * AddPartialLine's eight line packets' SetLineF2, SetShadeTex and
     * AddPrim, which PsyZ charges (56 a line: the port's call took 3125) */
    RR_CYCLES_DRAW_UI_BOX = 2655 - 8 * 56,
    /* DrawUiWindowRowText: 300 and 60 a cell (one 360 in the main menu's
     * rows, two 376-410 in the car selects, more 795 in the new car's) */
    RR_CYCLES_DRAW_UI_WINDOW_ROW_TEXT = 300,
    RR_CYCLES_DRAW_UI_WINDOW_ROW_TEXT_CELL = 60,
    RR_CYCLES_DRAW_UI_WINDOW_ROW_TEXT_TILE = 321, // a row of no cells (its one tile: the card menu's)
    RR_CYCLES_DRAW_MENU_FRAME = 1320,         // DrawMenuFrame's own, each callee marked (Time Attack's results)
    RR_CYCLES_UPDATE_SCREEN_FADE = 247,       // UpdateScreenFade
    RR_CYCLES_STEP_CAMERA_SCRIPT = 260,       // StepCameraScript (215-389, the menus' frames: 251-273 median)
    RR_CYCLES_UPDATE_BADGE_ROW_ANIM = 151,    // UpdateBadgeRowAnim
    /* DrawBadgeRow's own and its title bar's (DrawBadgeTitleBar besides the
     * SetSprt, SetShadeTex and AddPrim of its GameQueueSprite calls, which
     * PsyZ charges): 2368 on DuckStation in the main menu with the title
     * bar, 1830-2036 the bar alone in Time Attack's car select, 1232-1424
     * the row without it (its AddDrawAreaPrim and AddDrawModePrim in) */
    RR_CYCLES_DRAW_BADGE_ROW = 346,           // 346 in the dialogues (its own alone)
    RR_CYCLES_BADGE_TITLE_BAR = 1400,
    RR_CYCLES_UPDATE_DIALOGUE = 130,          // UpdateDialogue
    /* The main menu (mode 1/4), each routine's own code besides the
     * routines it calls that are charged themselves (the BIOS's
     * _sio_control calls included): TitleMenuUpdate by its submode (the
     * title, the mode select, its fade out; 221 in the unlock notices),
     * RunIntroScriptStep, ModeSelectMenuStep, StepMenuValue (a short value,
     * an int 8 more), DrawModeSelectRows (DSF_MARKS on each and its
     * callees, DuckStation's exact timing, ta-course1 fields 1037-1667)
     * [mark]. Not charged before: the main menu's frames ended 13.5k late
     * with the routines that were charged too long (DrawUiBox,
     * DrawUiWindowRowText, AddSpritePrim, IntroScriptStep, DrawParticle) */
    RR_CYCLES_TITLE_MENU_NOTICE = 200,
    RR_CYCLES_TITLE_MENU_TITLE = 206,
    RR_CYCLES_TITLE_MENU_SELECT = 271,
    RR_CYCLES_TITLE_MENU_FADE = 102,
    RR_CYCLES_RUN_INTRO_SCRIPT_STEP = 17,
    RR_CYCLES_MODE_SELECT_MENU_STEP = 253,
    RR_CYCLES_STEP_MENU_VALUE = 115,
    RR_CYCLES_STEP_MENU_VALUE_INT = 8,
    RR_CYCLES_DRAW_MODE_SELECT_ROWS = 277,
    /* DrawAnimatedText: 123, 69 for each glyph it draws, 25 for each other
     * entry; DrawAnimatedGlyph 345 */
    RR_CYCLES_DRAW_ANIMATED_TEXT = 123,
    RR_CYCLES_DRAW_ANIMATED_TEXT_GLYPH = 69,
    RR_CYCLES_DRAW_ANIMATED_TEXT_ENTRY = 25,
    RR_CYCLES_DRAW_ANIMATED_GLYPH = 345 - 92, // (334 on DuckStation, the port's 426 with its SDK calls)
    RR_CYCLES_CARD_MENU_MODE_UPDATE = 40,     // CardMenuModeUpdate (its frame's 2276 against the port's 2471)
    /* StepCardMenuStateMachine 356, 294 more with its card poll;
     * DrawCardMenuStatusText 203, 79 more for a status lookup */
    RR_CYCLES_STEP_CARD_MENU = 221,          // (356 with its callees' marks' stubs)
    RR_CYCLES_STEP_CARD_MENU_POLL = 294,
    RR_CYCLES_DRAW_CARD_MENU_STATUS = 203 - 105,
    RR_CYCLES_DRAW_CARD_MENU_STATUS_LOOKUP = 79,
    RR_CYCLES_POLL_CARD_STATUS = 257,         // PollCardStatus (472 with its callees' marks)
    RR_CYCLES_UPDATE_CARD_SLOT_ROWS = 90,     // UpdateCardSlotUiRows (193 with the stubs)
    RR_CYCLES_DRAW_CARD_MENU_COMMANDS = 151,  // DrawCardMenuCommandRows (269 with the stubs)
    /* InitAnimatedTrackObjects (course 7's fireworks pools: 2 x 5 x 255
     * line packets set up and linked, then 18 key pointers): 74230 on
     * DuckStation [mark], of which its SetLineG2 calls charge 2550 x 5 */
    RR_CYCLES_INIT_ANIMATED_TRACK_OBJECTS = 74230 - 2550 * 5,
    RR_CYCLES_LOAD_TIM_IMAGE = 201, // LoadTimImage, besides its LoadImage and DrawSync calls
    /* A course load's relocations by their work (DSF_MARKS name:ret, DSF_EXC:
     * course 7 and course 4's loads, 80 and 134 segments, 396 and 666 track
     * nodes, 28 and 8 models; 7 of each base the mark's stub):
     * RelocateRelativeFile 14300 and 23804 (176 a segment's 8 view slots),
     * SetTrackNodeData 8000 and 13400 (20 a node's length), RelocateModelTable
     * 671 and 211 (23 an entry) [mark] */
    RR_CYCLES_RELOCATE_RELATIVE_FILE = 220 - 7,
    RR_CYCLES_RELOCATE_RELATIVE_SEGMENT = 176,
    RR_CYCLES_SET_TRACK_NODE_DATA = 80 - 7,
    RR_CYCLES_SET_TRACK_NODE = 20,
    RR_CYCLES_RELOCATE_MODEL_TABLE = 27 - 7,
    RR_CYCLES_RELOCATE_MODEL_ENTRY = 23,
    /* RegisterModelBank (a model bank's vertex, normal and stream offsets
     * made addresses): 86 clocks on DuckStation with two streams, both
     * courses' loads above, 7 of them the mark's stub [mark] */
    RR_CYCLES_REGISTER_MODEL_BANK = 86 - 7,
};

void Psyz_SpendCycles(unsigned cycles);
/* PS1 time in nanoseconds, for a cost measured as a span (psyz/system.h). */
void Psyz_SpendTime(unsigned ns);

#define RR_CPU_CYCLES(cycles) Psyz_SpendCycles(cycles)

#endif
