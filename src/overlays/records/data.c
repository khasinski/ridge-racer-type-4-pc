#include "rr/overlay_module.h"
#include "rr/overlay663.h"

u32 ovl663_module_id = 13;

const char ovl663_scene_name[16] = "recode0\n";

u8 ovl663_course_order[9]
    = {1, 0, 2, 3, 8, 4, 6, 7, 5};

u8 ovl663_time_text[10]
    = {48, 48, 39, 48, 48, 34, 48, 48, 48, 0};

UiUvRect ovl663_record_badges[2]
    = {{0, 216, 16, 32}, {16, 216, 16, 32}};

u8 ovl663_race_column_widths[4]
    = {20, 8, 60, 8};

u8 ovl663_lap_column_widths[4]
    = {20, 8, 60, 8};

UiUvRect ovl663_record_label = {176, 16, 48, 16};

s32 ovl663_selected_record = 0;

s32 ovl663_course_count = 0;

s32 ovl663_available_courses[9]
    = {0, 0, 0, 0, 0, 0, 0, 0, 0};

s32 ovl663_extra_courses_enabled = 0;

s32 ovl663_record_kind = 0;

s32 ovl663_available_course_count = 0;

s32 ovl663_course_unlock_levels[9]
    = {0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Writable state of this unit; ovl663_reset_module restores it. */
static const OverlayStateRegion ovl663_data_regions[] = {
    RR_OVERLAY_STATE(ovl663_available_course_count),
    RR_OVERLAY_STATE(ovl663_available_courses),
    RR_OVERLAY_STATE(ovl663_course_count),
    RR_OVERLAY_STATE(ovl663_course_order),
    RR_OVERLAY_STATE(ovl663_course_unlock_levels),
    RR_OVERLAY_STATE(ovl663_extra_courses_enabled),
    RR_OVERLAY_STATE(ovl663_lap_column_widths),
    RR_OVERLAY_STATE(ovl663_module_id),
    RR_OVERLAY_STATE(ovl663_race_column_widths),
    RR_OVERLAY_STATE(ovl663_record_badges),
    RR_OVERLAY_STATE(ovl663_record_kind),
    RR_OVERLAY_STATE(ovl663_record_label),
    RR_OVERLAY_STATE(ovl663_selected_record),
    RR_OVERLAY_STATE(ovl663_time_text)};
const OverlayStateTable ovl663_data_state = {ovl663_data_regions, sizeof(ovl663_data_regions) / sizeof(ovl663_data_regions[0])};

/* Module reset: the state a fresh load of the records image gave
 * (initialized data and zeroed BSS); see rr/overlay_module.h. */
extern const OverlayStateTable ovl663_data_state;
static const OverlayStateTable *const ovl663_state_tables[] = {&ovl663_data_state};
static OverlayStateImage ovl663_state_image = {ovl663_state_tables, sizeof(ovl663_state_tables) / sizeof(ovl663_state_tables[0]), NULL};

void ovl663_capture_module(void) {
    OverlayStateCapture(&ovl663_state_image);
}

void ovl663_reset_module(void) {
    OverlayStateRestore(&ovl663_state_image);
}
