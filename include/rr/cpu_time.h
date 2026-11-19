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
    RR_CYCLES_INTRO_SCRIPT_STEP = 1282,    // IntroScriptStep [call]
    RR_CYCLES_ADD_TILE_PRIM = 184,         // AddTilePrim [rec]
    RR_CYCLES_ADD_TILE_GRID_2X2 = 277,     // AddTileGrid2x2 [mark], besides its AddTilePrims (321 [call])
    RR_CYCLES_ADD_VERTICAL_GRADIENT = 270, // AddVerticalGradientRect [rec]
    RR_CYCLES_ADD_GOURAUD_RECT = 284,      // AddGouraudRect [rec]
    RR_CYCLES_ADD_LINE_F2 = 125,           // AddLineF2Prim [rec]
    RR_CYCLES_ADD_SLANTED_GRADIENT = 290,  // AddSlantedGradientRect [rec]
    RR_CYCLES_UPDATE_PARTICLES = 840,      // UpdateParticles [call]
    RR_CYCLES_UPDATE_PARTICLE = 53,        // UpdateParticle [rec]
    RR_CYCLES_DRAW_PARTICLE = 2110,        // DrawParticle [rec ratio] 0.895
    RR_CYCLES_DRAW_INTRO_RIPPLES = 2064,   // DrawIntroRippleSprites [rec ratio] 0.939
    RR_CYCLES_UPDATE_FADE_TINT = 112,      // UpdateFadeTint [rec]
    RR_CYCLES_EMIT_FADE_SPRITE = 81,       // EmitFadeSprite [rec]
    RR_CYCLES_FONT_DRAW = 116,             // AddTextSprites [rec ratio] 0.969,
    RR_CYCLES_FONT_GLYPH = 186,            // and each glyph it draws
    /* the menus of a Time Attack run (DS fields 283-3266) */
    RR_CYCLES_UPDATE_MENU_SCRIPT = 433,    // UpdateMenuScript [rec ratio] 0.855
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
     * previous DrawOTag's list DMA, which PsyZ posts at the DMA's time. */
    RR_CYCLES_PAD_UPDATE = 1930,
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
     * TOC position before its CdSync(1), 100 from a completed CdSync to the
     * next GetlocP (CdLastCom, CdPosToInt), and the XA player's step 45 to
     * its CdSync. [mark] */
    RR_CYCLES_SOUND_UPDATE_HEAD = 38,
    RR_CYCLES_SOUND_UPDATE_FADES = 68,
    RR_CYCLES_CD_AUDIO_UPDATE = 39,
    RR_CYCLES_CD_AUDIO_TICK = 206,
    RR_CYCLES_CD_AUDIO_POSITION = 100,
    RR_CYCLES_CD_AUDIO_PLAY_STEP = 45,
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
    RR_CYCLES_SOUND_LISTENER = 2695,     // SoundListenerUpdate, listener on
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
     * frames) */
    RR_CYCLES_CD_FILE_STEP = 23,
    RR_CYCLES_CD_FILE_STEP_IDLE = 39,
    RR_CYCLES_CD_FILE_STEP_BUSY = 77,
    RR_CYCLES_CD_FILE_STEP_DISPATCH = 45,
    RR_CYCLES_CD_FILE_START_READ = 15,
    RR_CYCLES_CD_FILE_STEP_RETURN = 51,
    /* LoadR4Directory's CdReadSync(1) loop around the call: its turns 709
     * clocks apart on DuckStation with the call's two marks' stubs (about
     * 17), the call 680 [mark] */
    RR_CYCLES_CD_READ_SYNC_LOOP = 12,
    /* LoadR4Directory after its read: the directory's entries to sector
     * positions (CdIntToPos, 76 clocks, and the loop), 80.0k for 950 */
    RR_CYCLES_R4_DIRECTORY_ENTRY = 84,
    /* LoadR4Directory from its CdReadSync loop's end to the parse: on
     * DuckStation the read's Pause completion comes 2 fields and 34871
     * clocks before CdReset's first acknowledge (the boot's Getstat), the
     * work between them as charged [exc] */
    RR_CYCLES_R4_DIRECTORY_READ_END = 251,
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
     * 3340) [mark] */
    RR_CYCLES_MAP_BITMAP_ISO = 77100,
    RR_CYCLES_TRACK_NODE_MAP = 435,
    RR_CYCLES_TRACK_NODE_MAP_NODE_X100 = 6886,
    RR_CYCLES_COURSE_INFO_PANEL = 2927,
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
    RR_CYCLES_ADD_SPRITE_PRIM = 212,          // AddSpritePrim, brightness set
    RR_CYCLES_ADD_SPRITE_PRIM_SHADE_TEX = 14, // its SetShadeTex call instead
    RR_CYCLES_ADD_DRAW_AREA_PRIM = 320,       // AddDrawAreaPrim
    RR_CYCLES_ADD_DRAW_MODE_PRIM = 140,       // AddDrawModePrim
    RR_CYCLES_DRAW_UI_ROW = 628,              // DrawUiRow (535-900 by its state)
    RR_CYCLES_DRAW_UI_BOX = 2677,             // DrawUiBox, besides its AddTilePrim
    RR_CYCLES_DRAW_UI_WINDOW_ROW_TEXT = 663,  // DrawUiWindowRowText
    RR_CYCLES_DRAW_MENU_FRAME = 2182,         // DrawMenuFrame
    RR_CYCLES_UPDATE_SCREEN_FADE = 247,       // UpdateScreenFade
    RR_CYCLES_STEP_CAMERA_SCRIPT = 400,       // StepCameraScript (217 idle)
    RR_CYCLES_UPDATE_BADGE_ROW_ANIM = 173,    // UpdateBadgeRowAnim
    RR_CYCLES_DRAW_BADGE_ROW = 2304,          // DrawBadgeRow
    RR_CYCLES_UPDATE_DIALOGUE = 130,          // UpdateDialogue
    /* DrawAnimatedText: 123, 69 for each glyph it draws, 25 for each other
     * entry; DrawAnimatedGlyph 345 */
    RR_CYCLES_DRAW_ANIMATED_TEXT = 123,
    RR_CYCLES_DRAW_ANIMATED_TEXT_GLYPH = 69,
    RR_CYCLES_DRAW_ANIMATED_TEXT_ENTRY = 25,
    RR_CYCLES_DRAW_ANIMATED_GLYPH = 345,
    RR_CYCLES_CARD_MENU_MODE_UPDATE = 138,    // CardMenuModeUpdate
    /* StepCardMenuStateMachine 356, 294 more with its card poll;
     * DrawCardMenuStatusText 203, 79 more for a status lookup */
    RR_CYCLES_STEP_CARD_MENU = 356,
    RR_CYCLES_STEP_CARD_MENU_POLL = 294,
    RR_CYCLES_DRAW_CARD_MENU_STATUS = 203,
    RR_CYCLES_DRAW_CARD_MENU_STATUS_LOOKUP = 79,
    RR_CYCLES_POLL_CARD_STATUS = 472,         // PollCardStatus
    RR_CYCLES_UPDATE_CARD_SLOT_ROWS = 193,    // UpdateCardSlotUiRows
    RR_CYCLES_DRAW_CARD_MENU_COMMANDS = 269,  // DrawCardMenuCommandRows
    /* InitAnimatedTrackObjects (course 7's fireworks pools: 2 x 5 x 255
     * line packets set up and linked, then 18 key pointers): 74230 on
     * DuckStation [mark], of which its SetLineG2 calls charge 2550 x 5 */
    RR_CYCLES_INIT_ANIMATED_TRACK_OBJECTS = 74230 - 2550 * 5,
    RR_CYCLES_LOAD_TIM_IMAGE = 201, // LoadTimImage, besides its LoadImage and DrawSync calls
};

void Psyz_SpendCycles(unsigned cycles);
/* PS1 time in nanoseconds, for a cost measured as a span (psyz/system.h). */
void Psyz_SpendTime(unsigned ns);

#define RR_CPU_CYCLES(cycles) Psyz_SpendCycles(cycles)

#endif
