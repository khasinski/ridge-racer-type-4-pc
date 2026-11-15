#ifndef RR_OVERLAY667_COURSE_INITIALIZER_H
#define RR_OVERLAY667_COURSE_INITIALIZER_H
#include "game/data/menuvars.h"
#include "rr/layout.h"
#include "rr/menu_selection.h"
#include "rr/course_choice_types.h"
#include "rr/menu_choice_types.h"
#include "rr/selection.h"
#include "rr/player_status.h"
#include "rr/course_choices.h"
extern s16 LinkPlayerWinCounts[4],LinkCarChoiceIndex[2],LinkCarChoiceGroup[2];
#include "game/data/selection.h"
extern CourseChoice4 LinkCarChoiceTable[2][2][9];
extern CourseChoice4 CourseChoiceTable[4][9];
extern char LinkPlayerNames[4][12];
extern char *ovl667_default_player_names[4];
extern s32 ActivePlayerIndex,LinkAvailableCourseCount;
extern s16 LinkMenuSyncPhase,LinkClientRoleState;

void SetDispMask(s32);
void SetupDrawDispEnvs(s32,s32,s16,u32);
s32 UpdateLinkHandshake(s32);
void ClearScreenOverlayState(void);
void CopyStringClearTail(char*,char*);
void ovl667_init_car_selection(s32);
void UnpackChoiceGlyph(s8*,s32);
void SeqStartTrack(s32);
void SetMenuFrameOptions(s32,s32,s32);
/* Four complete selection prefixes occupy bytes 0..15 of the 32-byte owner.
 * Each player base freshly read after a service is 0 or 2; mode is 0 or 1.
 * Name lookups are four native pointers to strings fitting twelve bytes.
 * Copies cover the full 144-byte two-snapshot/two-player/nine-course owner. */
void ovl667_init_session_menus(void);
#endif
