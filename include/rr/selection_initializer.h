#ifndef RR_OVERLAY667_SELECTION_INIT_H
#define RR_OVERLAY667_SELECTION_INIT_H
#include "game/data/menuvars.h"
#include "game/data/cdfile2.h"
#include "rr/ui.h"
#include "rr/load_cursor.h"
#include "rr/race_types.h"
#include "rr/menu_arena_types.h"
#include "rr/menu_selection.h"
#include "rr/selection_types.h"
#include "rr/course_choice_types.h"
#include "rr/projection_state.h"

extern u32 ScreenFrame;
extern s16 MenuCursor,MenuItemCount,LinkMenuSyncPhase;
#include "rr/cd_directory_buffer.h"
#include "rr/menu_arena_state.h"
#include "rr/menu_arena_base.h"
#include "rr/player_cars.h"
#include "game/data/selection.h"
extern CourseChoice4 LinkCarChoiceTable[2][2][9];
extern s16 LinkCarChoiceCounts[2][2],LinkCarChoiceGroup[2],LinkCarChoiceIndex[2];
void ResetScreenCounters(void);
void InitMenuArenas(void);

void InitPreviewImageRows(void);
void ResetCarState(RaceVehicle *);
s32 CountChoiceGlyphs(const CourseChoice4 *);
void UnpackChoiceGlyph(s8 *,s32);
/* Selection initializer: original player0..3 selects the group reset. Service
 * callbacks may change the live player0..3, arena roots and choice state;
 * every subsequent publication uses that live state. Group/index must stay
 * within2 groups and9 choices. Resource base owns at least0x2D000 bytes.
 * Pointer fields retain native widths; only the target has PAL pointer layout. */
RR_ASSERT_SIZE(CourseChoice4,4);
RR_ASSERT_SIZE(SelectionRows32,32);
void ovl667_init_car_selection(s32 player);
#endif
