#ifndef RR_OVERLAY661_LAYOUT_H
#define RR_OVERLAY661_LAYOUT_H
#include "game/data/menuvars.h"
#include "rr/race.h"
#include "scratchpad.h"
#include "rr/load_cursor.h"
#include "rr/projection_state.h"
#include "rr/renderer_setup.h"
#include "rr/menu_arena_types.h"
#include "rr/menu_selection.h"
#include "rr/selection.h"
#include "rr/course_choice_types.h"
#include "rr/menu_glyph.h"
#include "rr/ui.h"
#include "rr/menu_arena_state.h"
#include "rr/cd_directory_buffer.h"
#include "rr/menu_arena_base.h"

extern u32 ScreenFrame;
extern s16 MenuCursor,MenuItemCount,ovl661_preview_confirmation;
extern s16 ovl661_available_car_counts[2][2],ovl661_selected_cars[2],ovl661_selected_groups[2];
extern CourseChoice4 ovl661_car_choices[2][2][9];
void ResetScreenCounters(void);
void InitMenuArenas(void);
void InitPreviewImageRows(void);
void ResetCarState(RaceVehicle *);
s32 CountChoiceGlyphs(const CourseChoice4 *);
/* Mode and fresh player selectors are0..1. Each group is0..1 and course0..8;
 * count helpers inspect all nine records. The full selection owner has32 bytes.
 * Setup callbacks may replace arenas and mutate every later selection. */
void ovl661_init_preview(s32 mode);
#endif
