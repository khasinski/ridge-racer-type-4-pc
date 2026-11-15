#ifndef RR_OVERLAY665_PANELS_H
#define RR_OVERLAY665_PANELS_H
#include "rr/overlay665_menu.h"
#include "scratchpad.h"
#include "rr/menu_label_render.h"
#include "rr/projection_state.h"
extern UiUvRect ovl665_brightness_swatch_uv_rect;
void ovl665_init_brightness_panel(void);
void ovl665_update_brightness_panel(void);
void DrawHudDigit(s32,s32,s32,s32);
void UpdateBadgeRowAnim(s32);
/* Writable aligned packet arenas and a full frame ordering context. Device
 * links serialize low24 address bits; software cursors keep native width. */
#endif
