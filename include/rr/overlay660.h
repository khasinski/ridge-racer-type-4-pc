#ifndef RR_OVERLAY660_H
#define RR_OVERLAY660_H

#include "game/menu/menutra21.h"
#include "game/data/menuvars.h"
#include "rr/menu_overlay_runtime.h"
#include "rr/overlay_race_state.h"
#include "rr/race_frame_scratch.h"

extern const char ovl660_title[];
extern char ovl660_new_record_label[];
extern s32 ovl660_result_labels[55];
extern u8 ovl660_preview_row_map[2][4];
extern s16 ovl660_available_car_counts[2];
extern s32 ovl660_available_course_count;
extern s32 ovl660_race_record_rank;
extern s32 ovl660_name_cursor;
extern s16 ovl660_result_kind;
extern char ovl660_record_name[12];
extern s16 ovl660_preview_confirmation;
extern s32 ovl660_result_transition;
extern s16 ovl660_preview_selection;
extern s32 ovl660_lap_record_rank;
extern s16 ovl660_preview_bank;

#include "rr/runtime_settings.h"

void CopyStringClearTail(char *destination, char *source);
void StepNameChar(char *text);
void DrawRaceOverlaySprites(s32 paused);
s32 CountChoiceGlyphs(const CourseChoice4 *choices);
s32 EncodeRecordPassword(s32 course, s32 selection, s32 car, s32 mode, s32 value, u8 *output);

void ovl660_select_scene(void);
void ovl660_init_menu(void);
void ovl660_init_scene(void);
void ovl660_update_menu(void);
void ovl660_init_results(void);
/* The results controller explicitly passes zero in a0. */
s32 ovl660_update_retry_choice(s32 reserved);
void ovl660_update_results(void);
s32 ovl660_insert_record(s32 mode, s32 time);
void ovl660_rank_standings(void);
void ovl660_init_preview(void);
void ovl660_update_preview(void);
void ovl660_exit_race(void);
void ovl660_update_race(void);

#include "scratchpad.h"
typedef RaceFrameScratch Rr660FrameScratch;
#ifndef RR_660_FRAME_SCRATCH
#define RR_660_FRAME_SCRATCH ((RaceFrameScratch *)SCRATCHPAD)
#endif

#endif
