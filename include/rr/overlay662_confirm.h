#ifndef RR_OVERLAY662_CONFIRM_H
#define RR_OVERLAY662_CONFIRM_H
#include "game/menu/menutra21.h"
#include "rr/overlay662_input.h"
#include "rr/overlay662_tool_action.h"
#include "rr/menu_text_state.h"
#include "rr/menu_transition.h"

/* Active confirmation reads first badge.state10 and text status. Sound may
 * replace text/badge roots; acceptance writes the fresh owners afterward. */
void ovl662_tool_load_card_dialog(s32 mode);
#endif
