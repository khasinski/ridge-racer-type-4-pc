#ifndef RR_OVERLAY662_FRAME_STEP_H
#define RR_OVERLAY662_FRAME_STEP_H
#include "rr/overlay662_init.h"
#include "rr/overlay662_garage.h"
#include "rr/emblem_images.h"

extern u32 ScreenFrame;
void ovl662_update_editor_cursor(void);
void ovl662_dispatch_editor_actions(s32 mode);
void ovl662_draw_editor_cursor(void);
void ovl662_draw_editor_panel_fades(void);
void ovl662_init_editor_draw_env(void);
void ovl662_update_editor_overlay(void);
void ovl662_upload_editor_textures(void);
/* Increment pulse with PAL word wrap; clock wraps before its upper clamp.
 * After eight services, publish the fresh active0/1 buffer and TIM pointers. */
void ovl662_step_editor_frame(void);
#endif
