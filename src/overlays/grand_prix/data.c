#include "rr/overlay_module.h"
#include "rr/overlay659.h"

u32 ovl659_module_id = 10;

u8 ovl659_finish_rank_limits[4]
    = {2, 1, 0, 0};

s32 ovl659_result_labels[34] = {
    7701, 0,   4378, 3,    4,    -1, 269,  525, 781,  20, 1038, 7701, 0,  4378, 18, -1,   269,
    525,  781, 20,   1038, 7701, 5,  7701, 17,  7701, -1, 6,    24,   20, 4107, -1, 7701, -1};

s32 ovl659_preview_timer = 0;

/* Writable state of this unit; ovl659_reset_module restores it. */
static const OverlayStateRegion ovl659_data_regions[] = {
    RR_OVERLAY_STATE(ovl659_finish_rank_limits),
    RR_OVERLAY_STATE(ovl659_module_id),
    RR_OVERLAY_STATE(ovl659_preview_timer),
    RR_OVERLAY_STATE(ovl659_result_labels)};
const OverlayStateTable ovl659_data_state = {ovl659_data_regions, sizeof(ovl659_data_regions) / sizeof(ovl659_data_regions[0])};

/* Module reset: the state a fresh load of the grand_prix image gave
 * (initialized data and zeroed BSS); see rr/overlay_module.h. */
extern const OverlayStateTable ovl659_data_state;
static const OverlayStateTable *const ovl659_state_tables[] = {&ovl659_data_state};
static OverlayStateImage ovl659_state_image = {ovl659_state_tables, sizeof(ovl659_state_tables) / sizeof(ovl659_state_tables[0]), NULL};

void ovl659_capture_module(void) {
    OverlayStateCapture(&ovl659_state_image);
}

void ovl659_reset_module(void) {
    OverlayStateRestore(&ovl659_state_image);
}
