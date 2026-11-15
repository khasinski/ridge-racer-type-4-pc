#ifndef RR_OVERLAY662_PROGRESS_CONTROLLER_H
#define RR_OVERLAY662_PROGRESS_CONTROLLER_H
#include "rr/overlay662_progress_init.h"
#include "game/data/menuvars.h"
#include "rr/screen_tiles.h"
#include "game/system/pad.h"
#include "rr/overlay662_setup.h"
#include "rr/menu_transition.h"
#include "rr/frame_environment.h"
#include "rr/tables.h"
#include "rr/overlay_menu.h"

extern u16 ovl662_team_unlocked_counts[4];
s32 LoadGlyphRowsStep(void);
void ovl662_step_selected_team(void);
void ovl662_step_selected_maker(void);
void ovl662_init_available_cars(void);
void ovl662_init_team_car_layout(void);

int DrawUiRow(s32);
void UpdateBadgeRowAnim(s32);
void UpdateScreenFade(s32);
/* Entry phase must be0,1,2,3,4 or8. PAL table slots5..7 and its default
 * consume an unspecified saved register and are not controller states.
 * Choice bytes0/1 remain0..3 across callbacks. Published roots cover a complete
 * UI row, badge, four glyph rows and an enclosing frame ordering owner.
 * All services may change globals; the badge state is captured before input.
 * Nonzero glyph-step completion triggers navigation. Countdown wraps signed16. */
void ovl662_update_history_screen(void);
#endif
