#ifndef RR_OVERLAY662_SAVE_DIALOG_H
#define RR_OVERLAY662_SAVE_DIALOG_H
#include "game/menu/menutra21.h"
#include "rr/overlay662_modal.h"
#include "rr/overlay662_tim.h"

/* Modes1/2/4/8 configure navigation through the shared option mapper.
 * Other nonzero modes preserve state. Mode0 operates two live UI rows and
 * complete text/badge owners; sound and TIM services may replace the roots.
 * Choice4 rechecks the fresh text status, captures a TIM through80126044,
 * then updates fresh UI rows and fade state. Its active side is0..1 and
 * TIM/image/palette owners meet that helper's backing-storage contract. */
void ovl662_tool_save_emblem_dialog(s32 mode);
#endif
