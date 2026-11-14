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
    /* 1d28cb93: boot (DS fields 282-600) */
    RR_CYCLES_INIT_DEFAULT_TIME_RECORDS = 10428, // InitDefaultTimeRecords [rec]
    RR_CYCLES_INIT_TIM_TEMPLATE = 2119,          // InitTimTemplate [rec]
    /* InitCourseChoices: the PS1's bzero cleared 0x24 bytes more than the
     * host table holds, at the BIOS fill's 132 clocks a byte (PsyZ
     * BIOS_FILL_BYTE_CYCLES, which the recompiler confirms) */
    RR_CYCLES_BIOS_FILL_BYTE = 132,
    /* 00df23a8: loads with a save on the card. PadUpdate takes 2.0k or
     * 5.7k on the recompiler depending on its path (not its callees), so
     * the breakpoint value stays [call] */
    RR_CYCLES_PAD_UPDATE = 6248, // PadUpdate, with the routines it calls
    /* SoundUpdate with the routines it calls but the two below, with no
     * sound slot active and the listener off (attract, DS fields 2000-2200)
     * [rec]; in a race (Time Attack, DS fields 3266-3700) an active slot's
     * UpdateSoundSlot and the listener's car search add these [rec]. */
    RR_CYCLES_SOUND_UPDATE = 1105,
    RR_CYCLES_UPDATE_SOUND_SLOT = 3873,  // UpdateSoundSlot, a call
    RR_CYCLES_SOUND_LISTENER = 2695,     // SoundListenerUpdate, listener on
};

void Psyz_SpendCycles(unsigned cycles);
/* PS1 time in nanoseconds, for a cost measured as a span (psyz/system.h). */
void Psyz_SpendTime(unsigned ns);

#define RR_CPU_CYCLES(cycles) Psyz_SpendCycles(cycles)

#endif
