#ifndef RR_MENU_SCREEN_EXIT_H
#define RR_MENU_SCREEN_EXIT_H
#include "game/data/menuvars.h"
#include "rr/frame_environment.h"
#include "rr/screen_tiles.h"
#include "game/race/raceset.h"
/* These scene exits draw through the containing frame's ordering table.
 * The first rectangle is the published screen context. */

void StopRaceAudioAndRumble(void);
void ovl660_exit_race(void);
void ovl661_exit_race(void);
#endif
