#include "rr/overlay_module.h"
#include "rr/overlay666.h"

u32 ovl666_module_id = 15;

const char ovl666_scene_name[24] =
    "job_ex_crs_info0\n";

UiUvRect ovl666_selection_sprite
    = {0, 120, 40, 80};

s32 ovl666_result_labels[33] = {
    7702, 0,  -1, 3,  5,    23,   -1, -1,   -1, 7702, -1, 7702, 0,  -1,   19, -1, 28,
    29,   30, 31, -1, 7702, 7702, 17, 7702, -1, 6,    8,  9,    -1, 7702, -1, -1};

u16 ovl666_course_dispatch[4][4] = {
    {6, 1, 6, 6}, {0, 0, 0, 3}, {4, 4, 7, 4}, {5, 2, 2, 2}};

s16 ovl666_course_counts[4] = {0, 0, 0, 0};

s32 ovl666_preview_phase = 0;

s32 ovl666_result_kind = 0;

MenuSelectionWord32 ovl666_selected_course
    = {0};

CourseChoice4 ovl666_available_courses[4][9]
    = {0};

/* Writable state of this unit; ovl666_reset_module restores it. */
static const OverlayStateRegion ovl666_data_regions[] = {
    RR_OVERLAY_STATE(ovl666_available_courses),
    RR_OVERLAY_STATE(ovl666_course_counts),
    RR_OVERLAY_STATE(ovl666_course_dispatch),
    RR_OVERLAY_STATE(ovl666_module_id),
    RR_OVERLAY_STATE(ovl666_preview_phase),
    RR_OVERLAY_STATE(ovl666_result_kind),
    RR_OVERLAY_STATE(ovl666_result_labels),
    RR_OVERLAY_STATE(ovl666_selected_course),
    RR_OVERLAY_STATE(ovl666_selection_sprite)};
const OverlayStateTable ovl666_data_state = {ovl666_data_regions, sizeof(ovl666_data_regions) / sizeof(ovl666_data_regions[0])};

/* Module reset: the state a fresh load of the extra_trial image gave
 * (initialized data and zeroed BSS); see rr/overlay_module.h. */
extern const OverlayStateTable ovl666_data_state;
static const OverlayStateTable *const ovl666_state_tables[] = {&ovl666_data_state};
static OverlayStateImage ovl666_state_image = {ovl666_state_tables, sizeof(ovl666_state_tables) / sizeof(ovl666_state_tables[0]), NULL};

void ovl666_capture_module(void) {
    OverlayStateCapture(&ovl666_state_image);
}

void ovl666_reset_module(void) {
    OverlayStateRestore(&ovl666_state_image);
}
