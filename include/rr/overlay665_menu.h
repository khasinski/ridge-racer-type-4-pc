#ifndef RR_OVERLAY665_MENU_H
#define RR_OVERLAY665_MENU_H
#include "game/data/menuvars.h"
#include "rr/menu_arenas.h"
#include "rr/menu_setup.h"
#include "rr/menu_text_state.h"
#include "rr/renderer_setup.h"
#include "rr/overlay665_visual_types.h"
#include "rr/overlay_menu.h"
#include "rr/menu_pulse.h"
#include "rr/frame_environment.h"
#include "rr/scene_camera_state.h"

#include "rr/menu_frame_state.h"
extern s32 ovl665_saved_stereo_mode;
void RaceLoadReset(void);
void SeqStartTrack(s32);
#include "rr/screen_tiles.h"
void SetPresentationDrawEnabled(s32);
void SeqFadeVolume(s32,s32);
int DrawUiRow(s32);
void UpdateScreenFade(s32);
void ovl665_init_controller_options(void);
void ovl665_init_menu(void);
void ovl665_update_menu(void);
/* Helpers may replace the full text-state root. */
RR_ASSERT_OFFSET(Overlay665VisualState,selection08,8);
RR_ASSERT_OFFSET(Overlay665VisualState,exit34,0x34);
#endif
