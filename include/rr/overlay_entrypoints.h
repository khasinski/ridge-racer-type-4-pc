#ifndef RR_OVERLAY_ENTRYPOINTS_H
#define RR_OVERLAY_ENTRYPOINTS_H
#include "types.h"

/* Resident-to-overlay calls. Keep these interfaces independent of each
 * overlay's private state views so callers and definitions share prototypes. */
void ovl659_init_results(void);
void ovl660_init_menu(void);
void ovl660_init_results(void);
void ovl661_init_preview(s32 mode);
void ovl661_init_results(void);
void ovl662_init_team_car_layout(void);
void ovl666_init_results(void);
void ovl667_init_car_selection(s32 player);
void ovl667_init_results(void);
#endif
