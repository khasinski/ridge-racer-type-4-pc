#ifndef RR_OVERLAY662_ACTION_LOOP_H
#define RR_OVERLAY662_ACTION_LOOP_H
#include "rr/overlay662_actions.h"
#include "rr/overlay662_buttons.h"
/* The existing callback-only view is the first member of each complete record.
 * A pointer to it can be converted back to its genuine containing record. */
typedef struct {
    GarageActionPrefix prefix;
    GarageStep render;
    s32 argument,key;
} GarageActionRecord16;
extern GarageActionRecord16 ovl662_editor_action_records[53];
void ovl662_draw_rgb_sliders(void);
/* Nonzero mode installs53 action and46 button pointers by their bounded keys.
 * Mode0 requires all53 entries to address complete records with both callbacks.
 * A dispatched single action requires working slot4 to be0..52.
 * After step callbacks, config1E
 * must be0..4; table slots4/9/11/12/14 point at complete button owners.
 * Callbacks may replace later entries and change working flags or render mode.
 * The render-enable byte is checked once, before all53 render callbacks. */
void ovl662_dispatch_editor_actions(s32 mode);
RR_ASSERT_SIZE(GarageActionRecord16, 2 * sizeof(GarageStep) + 8);
RR_ASSERT_OFFSET(GarageActionRecord16,prefix,0);
RR_ASSERT_OFFSET(GarageActionRecord16, render, sizeof(GarageStep));
RR_ASSERT_OFFSET(GarageActionRecord16, argument, 2 * sizeof(GarageStep));
RR_ASSERT_OFFSET(GarageActionRecord16, key, 2 * sizeof(GarageStep) + 4);
#endif
