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
 * Every cost is in system clocks (33.8688 MHz), measured per call on
 * DuckStation (Ridge Racer Type 4 PAL) as the routine's entry-to-return
 * ticks with GDB breakpoints, without the routines it calls unless its
 * entry says so. Breakpoints slow DuckStation's CPU by about 1%, so these
 * may read up to about 1% high. Each entry names the commit whose
 * DuckStation comparison it was fitted in:
 *   d1dff76a  the idle attract cycle (loading screen, demo seed fc 4941)
 *   1d28cb93  the splash and opening movie, field for field
 *   00df23a8  loads with a save on the card, the VS movie, the Extra Trial
 *             race load
 * Changing a value changes on which field things happen: compare the
 * per-field game state with DuckStation before and after
 * (docs/ARCHITECTURE.md, PS1 time model). */
enum {
    /* d1dff76a: the loading screen and the menus */
    RR_CYCLES_INTRO_SCRIPT_STEP = 1282,      // IntroScriptStep
    RR_CYCLES_ADD_TILE_PRIM = 205,           // AddTilePrim, with its AddPrim
    RR_CYCLES_ADD_TILE_GRID_2X2 = 321,       // AddTileGrid2x2
    RR_CYCLES_ADD_VERTICAL_GRADIENT = 272,   // AddVerticalGradientRect
    RR_CYCLES_ADD_GOURAUD_RECT = 312,        // AddGouraudRect
    RR_CYCLES_ADD_LINE_F2 = 192,             // AddLineF2Prim
    RR_CYCLES_ADD_SLANTED_GRADIENT = 314,    // AddSlantedGradientRect
    RR_CYCLES_UPDATE_PARTICLES = 840,        // UpdateParticles
    RR_CYCLES_UPDATE_PARTICLE = 103,         // UpdateParticle
    RR_CYCLES_DRAW_PARTICLE = 2358,          // DrawParticle
    RR_CYCLES_DRAW_INTRO_RIPPLES = 2198,     // DrawIntroRippleSprites
    RR_CYCLES_UPDATE_MENU_SCRIPT = 506,      // UpdateMenuScript
    RR_CYCLES_UPDATE_FADE_TINT = 197,        // UpdateFadeTint
    RR_CYCLES_EMIT_FADE_SPRITE = 106,        // EmitFadeSprite
    RR_CYCLES_FONT_DRAW = 120,               // the font draw call
    RR_CYCLES_FONT_GLYPH = 192,              // and each glyph it draws
    /* 1d28cb93: the splash and the opening movie */
    RR_CYCLES_INIT_DEFAULT_TIME_RECORDS = 10589, // InitDefaultTimeRecords
    RR_CYCLES_INIT_TIM_TEMPLATE = 2129,          // InitTimTemplate
    /* InitCourseChoices: the PS1's bzero cleared 0x24 bytes more than the
     * host table holds, at the BIOS fill's 132 clocks a byte (PsyZ
     * BIOS_FILL_BYTE_CYCLES) */
    RR_CYCLES_BIOS_FILL_BYTE = 132,
    /* 00df23a8: loads with a save on the card */
    RR_CYCLES_PAD_UPDATE = 6248, // PadUpdate, with the routines it calls
    /* SoundUpdate with the routines it calls, measured outside races; it is
     * spent in races too, where it has not been measured */
    RR_CYCLES_SOUND_UPDATE = 1624,
};

void Psyz_SpendCycles(unsigned cycles);
/* PS1 time in nanoseconds, for a cost measured as a span (psyz/system.h). */
void Psyz_SpendTime(unsigned ns);

#define RR_CPU_CYCLES(cycles) Psyz_SpendCycles(cycles)

#endif
