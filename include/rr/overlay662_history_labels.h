#ifndef RR_OVERLAY662_HISTORY_LABELS_H
#define RR_OVERLAY662_HISTORY_LABELS_H
#include "rr/overlay662_progress_init.h"
#include "rr/menu_label_render.h"
#include "rr/save_progress.h"
#include "rr/menu_choice.h"
extern u32 ScreenFrame;
extern s8 ovl662_team_count_glyphs[7];
extern u16 ovl662_team_unlocked_counts[4],ovl662_total_unlocked_cars;
extern s8 ovl662_stage_history_glyphs[3],ovl662_maker_count_glyphs[3];
extern s8 ovl662_car_number_glyphs[3],ovl662_emblem_index_glyphs[4];
extern s16 ovl662_selected_car_number,ovl662_selected_emblem_slot;
s32 rsin(s32 angle);
/* Row index and nonnegative glyph keys name accessible table entries. Numeric
 * values fit two decimal digits. Services return full native arena pointers.
 * Ordering and cursor remain pointers throughout; each helper may change globals.
 * History matching captures the pulse flag before rendering the labels. */
void *ovl662_draw_stage_history_label(GpuDmaTag *,void *,s32);
/* Choice byte0 remains0..3 across services; live counts fit0..80/0..320.
 * Seven label keys are nonnegative UV indices or sentinels-1/-2. */
void *ovl662_draw_team_car_counts_label(GpuDmaTag *,void *,s32);
void *ovl662_draw_maker_car_count_label(GpuDmaTag *,void *,s32);
void *ovl662_draw_car_number_label(GpuDmaTag *,void *,s32);
void *ovl662_draw_emblem_index_label(GpuDmaTag *,void *,s32);
#endif
