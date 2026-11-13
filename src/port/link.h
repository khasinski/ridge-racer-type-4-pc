#ifndef RR4_PORT_LINK_H
#define RR4_PORT_LINK_H

#include <stddef.h>

#include "preferences.h"

/* The game's link cable (two consoles, the "PLAYER 2" link battle of the
 * main menu) over the network: PsyZ's serial port talks to the other
 * player's game through TCP (psyz/link.h). The Escape menu's Link cable
 * page and the [link] preferences set it up; RR4_LINK overrides them:
 *   RR4_LINK=host[:port]          wait for the other player (port 47400)
 *   RR4_LINK=join:address[:port]  connect to the host ([v6 address]:port)
 *   RR4_LINK=off
 *   RR4_LINK_DELAY=ms             test hook: ms of added one-way latency
 * A headless run with RR4_LINK keeps the host clock and paces its fields
 * (PSYZ_AUDIO_CLOCK=wall, PSYZ_HEADLESS_PACED=1) unless they are given:
 * the two games share real time, as two consoles do. Only the hidden GPU
 * renderer (PSYZ_HEADLESS=gpu) paces; PSYZ_HEADLESS=1 runs unpaced. Such
 * runs also need PSYZ_CD_TIMING=instant for now (TECH_DEBT H19). */

/* First thing in main: the environment defaults above. */
void Rr4LinkPrepareEnv(void);
/* Once the release and the video timing are known (both players need the
 * same, and the same build): connects as RR4_LINK or the preferences say.
 * release names the disc (its product code). */
void Rr4LinkStart(const Rr4Prefs *prefs, int ntsc, const char *release);
/* The preferences changed (Rr4PrefsApply): reconnects as they say, unless
 * RR4_LINK overrides them. Before Rr4LinkStart it only remembers them. */
void Rr4LinkApply(const Rr4Prefs *prefs);

/* Parses an RR4_LINK value into mode (Rr4LinkPref), address and port.
 * Returns 0 for a malformed one. */
int Rr4LinkParse(const char *spec, int *mode, char *address, size_t size, unsigned *port);

/* The connection for the settings page: a short value and the whole line. */
void Rr4LinkStatus(char *brief, size_t brief_size, char *text, size_t size);

#endif
