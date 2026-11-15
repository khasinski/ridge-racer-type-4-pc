#ifndef RR_OVERLAY662_AVAILABLE_CARS_H
#define RR_OVERLAY662_AVAILABLE_CARS_H
#include "game/data/menuvars.h"
#include "game/data/cdfile2.h"
#include "rr/menu_arenas.h"
#include "rr/menu_setup.h"
#include "rr/menu_selection.h"
#include "rr/save_progress.h"
#include "rr/menu_glyph.h"
#include "rr/course_choice_types.h"
/* Four-byte available-car slots retain their unused upper halfword. Twenty
 * slots occupy27348..27398;the following storage is the image-fade phase. */
typedef struct {s16 value;u16 reserved;} GarageAvailableCar;
extern CourseChoice4 CourseChoiceTable[4][9];
extern s16 ovl662_menu_substate,ovl662_available_car_count,ovl662_selected_car_slot;
extern s16 MenuCursor,MenuItemCount;
extern s16 ovl662_car_slot_statuses[20];
extern GarageAvailableCar ovl662_available_car_slots[20];
extern s16 ovl662_saved_car_choice_idx;
void InitPreviewImageRows(void);s32 PackChoiceGlyph(void *);
/* Resource base owns at least0x2D000 bytes for the initial arena offset.
 * Services may replace full arena/UI pointers;progress group bytes remain
 *0..3 whenever the next progress word is selected. Each of20 low mask bits
 * determines a status and appends a car. Eight readable choices in group1
 * identify saved cars after byte2 values below39 reduce modulo20.
 * The final glyph decode reads slot0 even when no cars were appended,so it
 * remains a readable initialized record. Count is0..20. Final UI and arena
 * first pointer both capture the fresh screen root after the last decoder. */
void ovl662_init_available_cars(void);
RR_ASSERT_SIZE(GarageAvailableCar,4);
RR_ASSERT_OFFSET(GarageAvailableCar,reserved,2);
#endif
