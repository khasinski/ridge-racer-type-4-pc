#ifndef RR_OVERLAY667_COURSE_MENU_H
#define RR_OVERLAY667_COURSE_MENU_H
#include "game/data/menuvars.h"
#include "game/system/pad.h"
#include "rr/menu_frame_state.h"
#include "rr/overlay_menu.h"
#include "rr/menu_selection.h"
#include "rr/frame_environment.h"
#include "rr/menu_arenas.h"
#include "rr/tables.h"
#include "rr/driver_control.h"
#include "game/race/racegrid.h"
extern s16 LinkState,LinkMenuSyncPhase;
extern s32 LinkAvailableCourseCount;
extern u32 ScreenFrame;
s32 ExitOnLinkLost(void);
s32 UpdateLinkHandshake(s32);
s32 LoadTrackNodesStep(void);
void ResetScreenCounters(void);
void DrawCourseMapPreview(s32,s32);
void DrawCourseInfoPanel(s32,s32);
void SetMenuFrameOptions(s32,s32,s32);
void PlaySfx(s32);
void SeqFadeVolume(s32,s32);
void SeqMuteVolume(void);
int DrawUiRow(s32);
#include "rr/screen_tiles.h"
void UpdateScreenFade(s32);
void ovl667_init_car_selection(s32);
/* At least25 complete UI rows are accessible. Menu selection is0..2 at
 * indexed accesses; the live highlight count is0..8. LinkHostFlag is0..1
 * before scene8 reset,so initializer161A8 receives a valid player0 or2. Services may replace
 * roots,selections,counts and resources. Both counters wrap32 and the timer
 * wraps signed16. Driver/loader results are compared as complete32-bit words. */
void ovl667_update_course_menu(void);
#endif
