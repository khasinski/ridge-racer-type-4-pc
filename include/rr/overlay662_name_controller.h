#ifndef RR_OVERLAY662_NAME_CONTROLLER_H
#define RR_OVERLAY662_NAME_CONTROLLER_H
#include "game/menu/menutra21.h"
#include "game/data/menuvars.h"
#include "rr/screen_tiles.h"
#include "game/system/pad.h"
#include "rr/overlay662_name_state.h"
#include "rr/menu_transition.h"
#include "rr/menu_choice.h"
#include "rr/save_progress.h"
#include "rr/string_tail_copy.h"
#include "rr/tables.h"

extern u32 ScreenFrame;
extern u8 NameCharset[44];

void UpdateBadgeRowAnim(s32);void UpdateScreenFade(s32);
/* Captures the unsigned scene phase, then advances both wrapping counters.
 * Grid reads require row0..4 and column0..9 before one-step navigation.
 * Sound/layout services may mutate later input, selection and the full frame
 * root. The cursor remains0..8; glyph access uses selected0..42 after the
 * insertion sound. Both name owners retain a NUL within their twelve bytes.
 * Keys40/41/42 mean space/backspace/commit; larger keys are ignored.
 * Fade uses the signed low16 of the decremented timer and rereads it after
 * the drawing service. Every phase finishes with both row renders and title. */
void ovl662_update_name_entry(void);
#endif
