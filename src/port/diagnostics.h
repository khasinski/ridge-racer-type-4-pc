#ifndef RR4_PORT_DIAGNOSTICS_H
#define RR4_PORT_DIAGNOSTICS_H

#include "preferences.h"

/* Test and diagnostic hooks for automated runs (the RR4_* variables listed
 * at the top of diagnostics.c): frame limits, traces, watches and pokes of
 * globals, scripted pads and the race autodriver, frame, VRAM, audio and
 * GPU command captures, and run-wide graphics / controller overrides.
 *
 * Built only with -DRR4_DIAGNOSTICS=ON (the default for development
 * builds; release builds turn it off, as RR4_POKE writes arbitrary
 * memory). main.c calls Rr4DiagnosticsInit once before the game starts and
 * Rr4DiagnosticsVSync at every VSync. */

/* Reads the variables. prefs is the run's configuration: the overrides
 * (RR4_INTERNAL_RES, RR4_GRAPHICS, RR4_PAD_TYPE) apply to PsyZ and are
 * kept there, so that the settings menu applies them again. interactive:
 * RR4_GRAPHICS only turns the named options on instead of setting every
 * option. */
void Rr4DiagnosticsInit(Rr4Prefs *prefs, int interactive);

/* VSync vsync (counted from 1) between game frames. quitting: the run ends
 * at this VSync anyway. Returns 1 when the run ends here (RR4_FRAME_LIMIT
 * reached or quitting), after writing the captures taken at exit. */
int Rr4DiagnosticsVSync(long vsync, int quitting);

#endif
