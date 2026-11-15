#ifndef RR_OVERLAY662_COURSE_CONTROLLER_H
#define RR_OVERLAY662_COURSE_CONTROLLER_H
#include "rr/overlay662_available_cars.h"
#include "game/data/menuvars.h"
#include "rr/screen_tiles.h"
#include "game/system/pad.h"
#include "rr/race.h"
#include "game/car/carwheel.h"
#include "rr/load_cursor.h"
#include "rr/menu_transition.h"
#include "rr/menu_selection.h"
#include "rr/ui.h"
#include "rr/badge_rows.h"
#include "rr/tables.h"
#include "rr/selection_types.h"
#include "rr/course_choice_types.h"
#include "rr/menu_arena_types.h"
#include "rr/preview_rgb.h"
#include "rr/frame_storage_types.h"
extern CourseChoice4 CourseChoiceTable[4][9];
#include "game/data/selection.h"
#include "rr/menu_arena_state.h"
#include "rr/image_rows.h"
extern s32 ActivePlayerIndex;
extern s16 MenuCursor;
extern s16 ovl662_menu_substate,ovl662_available_car_count,ovl662_selected_car_slot;

void SetupMenuCarCamera(s32);void SetupViewport(s32);s32 LoadCarRangeStep(s32,s32);
void SetCarBounds(RaceVehicle*);void UpdateCarWorldTransform(char*);
void SetMenuFrameOptions(s32,s32,s32);void StepMenuValue(s32,s32,char*,s32,s32);
void UnpackChoiceGlyph(s8*,s32);s32 PackChoiceGlyph(void*);
void ResetScreenCounters(void);void SetModelTexturePage(s32);void PlaySfx(s32);
int DrawUiRow(s32);void DrawSelectionIconGrid(s32);
void UpdateBadgeRowAnim(s32);s32 UpdateImageRow(s32);
void DrawMenuStepArrows(s32,s32,s32,s32,s32,s32,s32);void UpdateScreenFade(s32);
void ovl662_select_garage_car(void);
/* UI selection is0..3 and available-car selection0..8. Dynamic close counts
 * require initialized rows. Services may replace full UI, badge, frame, mode
 * and arena roots; the complete mode allocation and32-byte selection owner
 * remain live. Readiness is a full signed word; RGB has three defined bytes. */
void ovl662_update_team_car_select(void);
#endif
