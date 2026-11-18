/* RODATA 0x8001129c 0x30 */
#include "rr/scene_camera_state.h"
#include "types.h"
#include "game/math/shuffle.h"
#include "rr/presentation.h"
#include "game/gfx/motion1.h"
#include "rr/bios_devices.h"
#include "rr/textured_panel.h"


void InitMotionPanel(s32 arg0) {
    PanelTrack50 *panel = &MotionControllerView()->panel;
    PsyqBzero(panel, sizeof(*panel));
    panel->mode = arg0;
}

void UpdateMotionPanel(MotionClock20 *clock) {
    s32 images[2][3] = {{1, 2, 4}, {5, 6, 4}};
    PresentationController *controller = MotionControllerView();
    PanelTrack50 *panel = &controller->panel;
    int mode = panel->mode;
    u32 duration;
    s32 image, variant;
    if (mode < 0) return;
    switch (mode) {
    case 0:
    case 1:
    {
        switch (controller->panel.state) {
        case 0:
            if (clock->active == 0) return;
            duration = clock->tempo / 50 * 2;
            duration = duration / 3;
            panel->duration4A = duration - ((u16)MotionControllerView()->scale4C + 1);
            image = images[panel->mode][0];
            panel->elapsed48 = 0;
            panel->state = panel->state + 1;
            panel->image44 = image;
            break;
        case 1:
            panel->elapsed48 = panel->elapsed48 + 1;
            if ((short)panel->elapsed48 < panel->duration4A) return;
            variant = panel->mode;
            image = images[variant][1];
            panel->elapsed48 = 0;
            panel->state = panel->state + 1;
            panel->image44 = image;
            break;
        case 2:
            panel->elapsed48 = panel->elapsed48 + 1;
            if ((short)panel->elapsed48 < panel->duration4A) return;
            variant = panel->mode;
            image = images[variant][2];
            panel->elapsed48 = 0;
            panel->state = 3;
            panel->image44 = image;
            break;
        case 4:
            panel->elapsed48 = panel->elapsed48 + 1;
            if ((short)panel->elapsed48 < panel->duration4A) return;
            panel->duration4A = (clock->tempo / 100) - ((u16)MotionControllerView()->scale4C + 1);
            panel->image44 = 5;
            panel->elapsed48 = 0;
            panel->state = panel->state + 1;
            break;
        case 5:
            panel->elapsed48 = panel->elapsed48 + 1;
            if ((short)panel->elapsed48 < panel->duration4A) return;
            panel->image44 = 6;
            panel->elapsed48 = 0;
            panel->state = panel->state + 1;
            break;
        }
    }
    break;
    case 2:
    {
        switch (controller->panel.state) {
        case 0:
            if (clock->active == 0) return;
            panel->image44 = 3;
            panel->elapsed48 = 0;
            controller->panel.state = controller->panel.state + 1;
            break;
        case 1:
            if (clock->active == 0) return;
            panel->image44 = 0;
            panel->elapsed48 = 0;
            controller->panel.state = 0;
            break;
        }
    }
    }
}

void DrawMotionControllerPanel(void) {
    s32 temp_a0;

    temp_a0 = MotionControllerView()->panel.image44;
    if (temp_a0 > 0) {
        EmitTexturedPanel(temp_a0 - 1);
    }
}

void ResetSlotOrder(void) {
    SlotOrder.pending00 = -1;
}
