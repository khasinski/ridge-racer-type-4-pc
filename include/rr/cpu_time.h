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
    RR_CYCLES_ADD_TILE_GRID_2X2 = 321,     // AddTileGrid2x2 [call]
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
    RR_CYCLES_BIOS_FILL = 328, // and the call (PsyZ BIOS_FILL_CYCLES)
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
    RR_CYCLES_UPDATE_SOUND_SLOT = 3873,  // UpdateSoundSlot, a call
    RR_CYCLES_SOUND_LISTENER = 2695,     // SoundListenerUpdate, listener on
    /* The boot, measured [mark]: entry-to-entry clocks on DuckStation's
     * recompiler, from a stub at each function entry that writes root
     * counter 2 (unused before SsStart), logged with DUCKSTATION_CD_TRACE
     * (tools/port/DUCKSTATION.md). The BIOS (fast boot, SCPH-1001) starts
     * the executable 10.7 ms into a field; its crt0 reaches MainLoop
     * 6.484 ms after the next vblank, and libetc's first ResetCallback
     * takes 62.3k clocks: SystemInit starts 8.324 ms after that vblank.
     * The port starts its fields there and spends both in MainLoop. */
    RR_CYCLES_BOOT_TO_MAIN_LOOP = 219610,
    RR_CYCLES_FIRST_RESET_CALLBACK = 62308,
    /* LoadR4Directory after its read: the directory's entries to sector
     * positions (CdIntToPos, 76 clocks, and the loop), 80.0k for 950 */
    RR_CYCLES_R4_DIRECTORY_ENTRY = 84,
    /* LoadR4Directory from its CdReadSync loop's end to the parse: on
     * DuckStation the read's Pause completion comes 2 fields and 34871
     * clocks before CdReset's first acknowledge (the boot's Getstat), the
     * work between them as charged [exc] */
    RR_CYCLES_R4_DIRECTORY_READ_END = 251,
    /* The attract demo's race frame, AttractDemoRaceUpdate up to the
     * frame's DrawSync: the least of 169 frames (502k-614k, median 614k)
     * [mark], so a frame that starts on time stays in its field. Its logic
     * (StepRaceFrame and UpdateTrackObjects: 97.5k in the demo's first
     * frame and the least of 517) comes before the cars are drawn, the
     * rest after them (DuckStation: the cars' wheel effects draw rand()
     * 111k-258k after the frame starts, on the console's side of a vblank
     * the whole frame's time charged first put them past) [mark] */
    RR_CYCLES_ATTRACT_DEMO_LOGIC = 97489,
    RR_CYCLES_ATTRACT_DEMO_DRAW = 502220 - 97489,
};

void Psyz_SpendCycles(unsigned cycles);
/* PS1 time in nanoseconds, for a cost measured as a span (psyz/system.h). */
void Psyz_SpendTime(unsigned ns);

#define RR_CPU_CYCLES(cycles) Psyz_SpendCycles(cycles)

#endif
