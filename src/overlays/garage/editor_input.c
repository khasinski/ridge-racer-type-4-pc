#include "rr/overlay_module.h"
#include "game/system/pad.h"
#include "rr/overlay662_input.h"
#include "rr/overlay662_garage.h"
#include "rr/tables.h"
#include "rr/overlay662_garage_fade.h"
#include "rr/overlay662_distance.h"
#include "rr/overlay662_actions.h"
#include "rr/overlay662_cursor_control.h"
#include "rr/overlay662_action_loop.h"

extern void *PsyqMemset(void *, s32, u32);
s32 ovl662_map_controller_buttons(s32 mode);

void ovl662_read_analog_cursor_delta(GaragePoint *output)
{
    GaragePoint point;
    s32 step,delta;
    u32 axis;
    PsyqMemset(&point,0,sizeof(point));
    if (PlayerControllers[0].input00.fields.type == 0x73 && PlayerControllers[0].kind1C != 0 && PlayerControllers[0].flag40 == 0) {
        step=1;
        if (PlayerControllers[0].input00.fields.buttons & 8) step=4;
        axis=PlayerControllers[0].axes14.bytes[2];
        if (axis <112) {
            delta=(axis-112)*step;
            if (ovl662_analog_cursor_accum[0] <0) ovl662_analog_cursor_accum[0]+=delta;
            else ovl662_analog_cursor_accum[0]=delta;
            point.x=ovl662_analog_cursor_accum[0]/112;
            ovl662_analog_cursor_accum[0]-=point.x*112;
        } else if (axis>=145) {
            delta=(axis-143)*step;
            if (ovl662_analog_cursor_accum[0] >0) ovl662_analog_cursor_accum[0]+=delta;
            else ovl662_analog_cursor_accum[0]=delta;
            point.x=ovl662_analog_cursor_accum[0]/112;
            ovl662_analog_cursor_accum[0]-=point.x*112;
        } else ovl662_analog_cursor_accum[0]=0;
        axis=PlayerControllers[0].axes14.bytes[3];
        if (axis <112) {
            delta=(axis-112)*step;
            if (ovl662_analog_cursor_accum[1] <0) ovl662_analog_cursor_accum[1]+=delta;
            else ovl662_analog_cursor_accum[1]=delta;
            point.y=(s16)(ovl662_analog_cursor_accum[1]/112)*2;
            ovl662_analog_cursor_accum[1]-=point.y*112;
        } else if (axis>=145) {
            delta=(axis-143)*step;
            if (ovl662_analog_cursor_accum[1] >0) ovl662_analog_cursor_accum[1]+=delta;
            else ovl662_analog_cursor_accum[1]=delta;
            point.y=(s16)(ovl662_analog_cursor_accum[1]/112)*2;
            ovl662_analog_cursor_accum[1]-=point.y*112;
        } else ovl662_analog_cursor_accum[1]=0;
    }
    if ((u16)point.x | (u16)point.y) {output->x=point.x;output->y=point.y;}
}

void ovl662_read_mouse_cursor_delta(GaragePoint *output)
{
    GaragePoint point;
    s32 value;
    s32 y;
    PsyqMemset(&point,0,sizeof(point));
    if (PlayerControllers[1].input00.fields.type == 0x12) {
        value = (s32)(PlayerControllers[1].axes14.bytes[0] * 0x1000000U) >> 25;
        if (value < -3 || value > 3) value += value / 2;
        if (value) point.x=value;
        y = PlayerControllers[1].axes14.signedBytes[1];
        if (y < -7 || y > 7) y += (y + (s32)((u32)y >> 31)) >> 1;
        if (y) point.y=y;
    }
    if ((u16)point.x | (u16)point.y) {
        output->x=point.x;
        output->y=point.y;
    }
}

void ovl662_read_dpad_cursor_delta(GaragePoint *output)
{
    GaragePoint point;
    u32 buttons;
    s32 step;
    PsyqMemset(&point,0,sizeof(point));
    if (PlayerControllers[0].input00.fields.type != 0x12) {
        buttons = PlayerControllers[0].input00.fields.buttons;
        step=1;
        if (buttons & 8) step=4;
        if (buttons & 0x1000) point.y=-(step*2);
        if (buttons & 0x4000) point.y=step*2;
        if (buttons & 0x8000) point.x=-step;
        if (buttons & 0x2000) point.x=step;
    }
    if ((u16)point.x | (u16)point.y) {
        output->x=point.x;
        output->y=point.y;
    }
}

void ovl662_poll_stick_direction_repeat(u32 *output)
{
    u32 result=0;
    s32 axis;
    if (PlayerControllers[0].input00.fields.type == 0x73 && PlayerControllers[0].kind1C != 0 && PlayerControllers[0].flag40 == 0) {
        axis=PlayerControllers[0].axes14.bytes[3];
        if (axis < 32) {
            ovl662_stick_repeat_up++;
            if ((s32)ovl662_stick_repeat_up >= 47) ovl662_stick_repeat_up=40;
            if (ovl662_stick_repeat_up == 1 || ovl662_stick_repeat_up == 40) result |= 1;
            *output=result;
        } else {
            ovl662_stick_repeat_up=0;
        }
        if (axis >= 225) {
            ovl662_stick_repeat_down++;
            if ((s32)ovl662_stick_repeat_down >= 47) ovl662_stick_repeat_down=40;
            if (ovl662_stick_repeat_down == 1 || ovl662_stick_repeat_down == 40) result |= 2;
            *output=result;
        } else {
            ovl662_stick_repeat_down=0;
        }
        axis=PlayerControllers[0].axes14.bytes[2];
        if (axis < 32) {
            ovl662_stick_repeat_left++;
            if ((s32)ovl662_stick_repeat_left >= 47) ovl662_stick_repeat_left=40;
            if (ovl662_stick_repeat_left == 1 || ovl662_stick_repeat_left == 40) result |= 4;
            *output=result;
        } else {
            ovl662_stick_repeat_left=0;
        }
        if (axis >= 225) {
            ovl662_stick_repeat_right++;
            if ((s32)ovl662_stick_repeat_right >= 47) ovl662_stick_repeat_right=40;
            if (ovl662_stick_repeat_right == 1 || ovl662_stick_repeat_right == 40) result |= 8;
            *output=result;
        } else {
            ovl662_stick_repeat_right=0;
        }
    }
}

void ovl662_poll_dpad_direction_repeat(u32 *output)
{
    u32 result=0;
    if (PlayerControllers[0].input00.fields.type != 0x12) {
        if (PlayerControllers[0].input00.fields.buttons & 0x1000) {
            ovl662_dpad_repeat_up++;
            if ((s32)ovl662_dpad_repeat_up >= 47) ovl662_dpad_repeat_up=40;
            if (ovl662_dpad_repeat_up == 1 || ovl662_dpad_repeat_up == 40) result |= 1;
            *output=result;
        } else {
            ovl662_dpad_repeat_up=0;
        }
        if (PlayerControllers[0].input00.fields.buttons & 0x4000) {
            ovl662_dpad_repeat_down++;
            if ((s32)ovl662_dpad_repeat_down >= 47) ovl662_dpad_repeat_down=40;
            if (ovl662_dpad_repeat_down == 1 || ovl662_dpad_repeat_down == 40) result |= 2;
            *output=result;
        } else {
            ovl662_dpad_repeat_down=0;
        }
        if (PlayerControllers[0].input00.fields.buttons & 0x8000) {
            ovl662_dpad_repeat_left++;
            if ((s32)ovl662_dpad_repeat_left >= 47) ovl662_dpad_repeat_left=40;
            if (ovl662_dpad_repeat_left == 1 || ovl662_dpad_repeat_left == 40) result |= 4;
            *output=result;
        } else {
            ovl662_dpad_repeat_left=0;
        }
        if (PlayerControllers[0].input00.fields.buttons & 0x2000) {
            ovl662_dpad_repeat_right++;
            if ((s32)ovl662_dpad_repeat_right >= 47) ovl662_dpad_repeat_right=40;
            if (ovl662_dpad_repeat_right == 1 || ovl662_dpad_repeat_right == 40) result |= 8;
            *output=result;
        } else {
            ovl662_dpad_repeat_right=0;
        }
    }
}

void ovl662_step_panel_fade_timers(void)
{
    if ((ovl662_editor_cursor_state.words[0] & 2) ||
        (PlayerControllers[0].input00.fields.type != 0x12 && (PlayerControllers[0].input00.fields.buttons & 4))) {
        ovl662_editor_cursor_state.words[10] += ovl662_editor_cursor_state.words[11];
        if ((s32)ovl662_editor_cursor_state.words[10] > 4096) {
            do {
                do {
                    ovl662_editor_cursor_state.words[10] = 4096;
                } while (0);
            } while (0);
        }
    } else {
        if (ovl662_editor_cursor_state.words[10] == 4096) {
            PlaySfx(2);
        }
        ovl662_editor_cursor_state.words[10] -= ovl662_editor_cursor_state.words[11];
        if ((s32)ovl662_editor_cursor_state.words[10] < 0) {
            ovl662_editor_cursor_state.words[10] = 0;
            ovl662_editor_cursor_state.words[1] = 48;
            ovl662_editor_cursor_state.words[0] &= ~4;
        }
    }
    RR_GARAGE_GTE_LERP(ovl662_editor_cursor_state.words[10],
                       &ovl662_editor_cursor_state.words[8],
                       &ovl662_editor_cursor_state.words[5]);
}

s32 ovl662_calc_point_distance(const GaragePoint *a, const GaragePoint *b) {
    u32 dx = (b->x - a->x) * 2U;
    u32 dy = (b->y - a->y);

    return 4096 / (((CordicSqrt(((dx * dx) + (dy * dy))) >> 6) / 64) + 1);
}

GaragePoint ovl662_get_rect_center(s32 index)
{
    GaragePoint point;
    GarageRectangle *rectangle = ovl662_button_rect_lookup[index];
    u16 width = (*rectangle)[2];
    s32 x = (s16)(*rectangle)[0] + ((s16)width >> 1);
    s32 y = (s16)(*rectangle)[1] + ((s16)(*rectangle)[3] >> 1);
    point.x = x;
    point.y = y;
    return point;
}

void ovl662_handle_shortcut_buttons(u32 flags){
 if(flags)PlaySfx(0);
 if(flags&1)ovl662_action_lookup[ovl662_editor_workspace.current60]->callback(1);
 else if(flags&2)ovl662_action_lookup[ovl662_editor_workspace.current60]->callback(2);
 if(flags&4)ovl662_action_lookup[ovl662_editor_workspace.current60]->callback(4);
 else if(flags&8)ovl662_action_lookup[ovl662_editor_workspace.current60]->callback(8);
}

s32 ovl662_map_controller_buttons(s32 mode)
{
    if(mode){
        ovl662_editor_cursor_state.words[2]++;
        if(ovl662_editor_cursor_state.words[2]>=47)ovl662_editor_cursor_state.words[2]=40;
        if(ovl662_editor_cursor_state.words[2]==1 || ovl662_editor_cursor_state.words[2]==40)return 1;return 0;
    }else{
        ovl662_editor_cursor_state.words[3]++;
        if(ovl662_editor_cursor_state.words[3]>=47)ovl662_editor_cursor_state.words[3]=40;
        if(ovl662_editor_cursor_state.words[3]==1 || ovl662_editor_cursor_state.words[3]==40)return 1;return 0;
    }
}

/* Repeat-key handling for the two garage scroll directions. */
void ovl662_update_button_states(void)
{
    s32 first;
    s32 second;

    if (PlayerControllers[0].input00.fields.type == 0x23) {
        first = PlayerControllers[0].input00.fields.buttons & 0x840;
        second = PlayerControllers[0].input00.fields.buttons & 0x80;
    } else if (PlayerControllers[0].input00.fields.type != 0x12) {
        first = PlayerControllers[0].input00.fields.buttons & 0x820;
        second = PlayerControllers[0].input00.fields.buttons & 0x40;
    }
    if (PlayerControllers[1].input00.fields.type == 0x12) {
        first |= PlayerControllers[1].input00.fields.buttons & 8;
        second |= PlayerControllers[1].input00.fields.buttons & 4;
    }

    if (ovl662_editor_cursor_state.words[1] & 0x10) {
        if (first) {
            ovl662_editor_cursor_state.words[1] = (ovl662_editor_cursor_state.words[1] | 0x10) & ~5;
            if (ovl662_map_controller_buttons(1)) {
                ovl662_editor_cursor_state.words[1] |= 0x40;
            } else {
                ovl662_editor_cursor_state.words[1] &= ~0x40;
            }
        } else {
            ovl662_editor_cursor_state.words[1] = (ovl662_editor_cursor_state.words[1] & ~0x51) | 4;
        }
    } else {
        if (first) {
            ovl662_editor_cursor_state.words[1] = (ovl662_editor_cursor_state.words[1] | 0x11) & ~4;
            ovl662_editor_cursor_state.words[2] = 0;
            if (ovl662_map_controller_buttons(1)) {
                ovl662_editor_cursor_state.words[1] |= 0x40;
            } else {
                ovl662_editor_cursor_state.words[1] &= ~0x40;
            }
        } else {
            ovl662_editor_cursor_state.words[1] &= ~0x55;
        }
    }

    if (ovl662_editor_cursor_state.words[1] & 0x20) {
        if (second) {
            ovl662_editor_cursor_state.words[1] = (ovl662_editor_cursor_state.words[1] | 0x20) & ~0xA;
            if (ovl662_map_controller_buttons(0)) {
                ovl662_editor_cursor_state.words[1] |= 0x80;
            } else {
                ovl662_editor_cursor_state.words[1] &= ~0x80;
            }
        } else {
            ovl662_editor_cursor_state.words[1] = (ovl662_editor_cursor_state.words[1] & ~0xA2) | 8;
        }
    } else {
        if (second) {
            ovl662_editor_cursor_state.words[1] = (ovl662_editor_cursor_state.words[1] | 0x22) & ~8;
            ovl662_editor_cursor_state.words[3] = 0;
            if (ovl662_map_controller_buttons(0)) {
                ovl662_editor_cursor_state.words[1] |= 0x80;
            } else {
                ovl662_editor_cursor_state.words[1] &= ~0x80;
            }
        } else {
            ovl662_editor_cursor_state.words[1] &= ~0xAA;
        }
    }
}

void ovl662_update_editor_cursor(void) {
    GaragePoint movement;
    GaragePoint destination;
    u32 events;
    s32 flags;
    u16 ySum;
    u16 xSum;

    PsyqMemset(&movement, 0, sizeof(movement));
    events = 0;
    if (!(ovl662_editor_cursor_state.words[0] & 1)) {
        if (ovl662_editor_cursor_state.words[0] & 2) {
            ovl662_step_panel_fade_timers();
            if ((s32)ovl662_editor_cursor_state.points.word28 < 0x1000) {
                return;
            }
            ovl662_editor_cursor_state.points.cursor18 = ovl662_editor_cursor_state.points.cursor14;
            ovl662_editor_cursor_state.words[0] &= ~2U;
            ovl662_editor_cursor_state.words[0] &= ~4U;
        }
        if (ovl662_editor_cursor_state.words[0] & 4) {
            if (!(ovl662_editor_cursor_state.words[0] & 8)) {
                ovl662_step_panel_fade_timers();
                if ((s32)ovl662_editor_cursor_state.points.word28 < 0x1000) {
                    return;
                }
                ovl662_poll_stick_direction_repeat(&events);
                ovl662_poll_dpad_direction_repeat(&events);
                ovl662_handle_shortcut_buttons(events);
            }
        } else {
            ovl662_read_analog_cursor_delta(&movement);
            ovl662_read_mouse_cursor_delta(&movement);
            ovl662_read_dpad_cursor_delta(&movement);
            xSum = ovl662_editor_cursor_state.points.cursor18.x + movement.x;
            ySum = ovl662_editor_cursor_state.points.cursor18.y + movement.y;
            ovl662_editor_cursor_state.points.cursor18.x = xSum;
            ovl662_editor_cursor_state.points.cursor18.y = ySum;
            if ((s16) xSum < ovl662_editor_cursor_state.halves[24]) {
                ovl662_editor_cursor_state.points.cursor18.x = (u16) ovl662_editor_cursor_state.halves[24];
            }
            if (((ovl662_editor_cursor_state.halves[24] + ovl662_editor_cursor_state.halves[26]) - 1) < (s16) ovl662_editor_cursor_state.points.cursor18.x) {
                ovl662_editor_cursor_state.points.cursor18.x = (u16) (((u16) ovl662_editor_cursor_state.halves[24] + (u16) ovl662_editor_cursor_state.halves[26]) - 1);
            }
            if ((s16) ySum < ovl662_editor_cursor_state.halves[25]) {
                ovl662_editor_cursor_state.points.cursor18.y = (u16) ovl662_editor_cursor_state.halves[25];
            }
            if (((ovl662_editor_cursor_state.halves[25] + ovl662_editor_cursor_state.halves[27]) - 1) < (s16) ovl662_editor_cursor_state.points.cursor18.y) {
                ovl662_editor_cursor_state.points.cursor18.y = (u16) (((u16) ovl662_editor_cursor_state.halves[25] + (u16) ovl662_editor_cursor_state.halves[27]) - 1);
            }
            ovl662_editor_cursor_state.points.cursor14 = ovl662_editor_cursor_state.points.cursor18;
            flags = ovl662_editor_cursor_state.words[0];
            if (!(flags & 8) && (PlayerControllers[0].input00.fields.type != 0x12) && (PlayerControllers[0].edges08.fields.pressed & 4)) {
                ovl662_editor_cursor_state.words[0] = (flags | 4);
                ovl662_editor_cursor_state.words[1] = 0x30;
                ovl662_editor_cursor_state.points.first20 = ovl662_editor_cursor_state.points.cursor18;
                destination=ovl662_get_rect_center(ovl662_editor_workspace.saved64[0]);
                ovl662_editor_cursor_state.points.second24 = destination;
                ovl662_editor_cursor_state.points.word28 = 0;
                ovl662_editor_cursor_state.points.distance2C = ovl662_calc_point_distance(&ovl662_editor_cursor_state.points.first20,&ovl662_editor_cursor_state.points.second24);
                ovl662_editor_workspace.current60 = ovl662_editor_workspace.saved64[0];
                ovl662_editor_workspace.selectorsB8[0] = ovl662_editor_workspace.saved64[1];
                ovl662_editor_workspace.selectorsB8[1] = ovl662_editor_workspace.saved64[2];
                ovl662_editor_workspace.selectorsB8[2] = ovl662_editor_workspace.saved64[3];
                ovl662_editor_workspace.selectorsB8[3] = ovl662_editor_workspace.saved64[4];
                ovl662_action_lookup[((GarageButton *)ovl662_button_rect_lookup[ovl662_editor_workspace.saved64[0]])->action0A]->callback(-1);
                PlaySfx(3);
                return;
            }
        }
        ovl662_update_button_states();
    }
}

void ovl662_dispatch_editor_actions(s32 mode)
{
    s32 i;
    s16 selected;
    GarageActionRecord16 *a;
    GarageButton *p4, *p9, *p11, *p12, *p14;

    if (mode) {
        for (i = 0; i < 53; i++) {
            a = &ovl662_editor_action_records[i];
            ovl662_action_lookup[a->key] = &a->prefix;
        }
        for (i = 0; i < 46; i++) {
            ovl662_button_rect_lookup[ovl662_editor_buttons[i].key0C] = &ovl662_editor_buttons[i].rectangle;
        }
    } else {
        if (!(ovl662_editor_cursor_state.words[0] & 2)) {
            if (!((ovl662_editor_cursor_state.words[0] & 4) &&
                  (s32)ovl662_editor_cursor_state.words[10] < 4096)) {
                if (ovl662_editor_cursor_state.words[0] & 8) {
                    ovl662_action_lookup[ovl662_editor_cursor_state.words[4]]->callback(0);
                } else {
                    for (i = 0; i < 53; i++) {
                        ovl662_action_lookup[i]->callback(0);
                        if (ovl662_editor_cursor_state.words[0] & 8) {
                            break;
                        }
                    }
                }
            }
        }
        switch (ovl662_editor_workspace.config1E) {
        case 0: selected = 9; break;
        case 1: selected = 11; break;
        case 2: selected = 12; break;
        case 3: selected = 14; break;
        case 4: selected = 4; break;
        }
        p4 = (GarageButton *)ovl662_button_rect_lookup[4];
        p9 = (GarageButton *)ovl662_button_rect_lookup[9];
        p11 = (GarageButton *)ovl662_button_rect_lookup[11];
        p12 = (GarageButton *)ovl662_button_rect_lookup[12];
        p14 = (GarageButton *)ovl662_button_rect_lookup[14];
        p4->timer0E = 0;
        p14->timer0E = 0;
        p12->timer0E = 0;
        p11->timer0E = 0;
        p9->timer0E = 0;
        ((GarageButton *)ovl662_button_rect_lookup[selected])->timer0E = -1;
        if (!ovl662_editor_workspace.flagsD1) {
            for (i = 0; i < 53; i++) {
                /* Recover the complete owner from its initial prefix member. */
                a = (GarageActionRecord16 *)ovl662_action_lookup[i];
                a->render(a->argument);
            }
            ovl662_draw_rgb_sliders();
        }
    }
}

/* Initial values of the overlay globals this module owns. */
void ovl662_draw_canvas_grid(s32 index);
void ovl662_draw_zoomed_canvas(s32 renderArgument);
#include "rr/overlay662_pixel_dispatch.h"
#include "rr/overlay662_scroll_drag.h"
#include "rr/overlay662_palette_edit.h"
#include "rr/overlay662_preview_drag.h"
#include "rr/overlay662_scale_panel.h"
#include "rr/overlay662_palette_tile.h"
#include "rr/overlay662_scroll_panel.h"
#include "rr/overlay662_scroll_actions.h"
#include "rr/overlay662_tool_action.h"
#include "rr/overlay662_exit_dialog.h"
#include "rr/overlay662_confirm.h"
#include "rr/overlay662_mirror.h"
#include "rr/overlay662_rotate.h"
#include "rr/overlay662_save_dialog.h"
#include "rr/overlay662_modal.h"
s16 ovl662_analog_cursor_accum[2];
u32 ovl662_stick_repeat_up;
u32 ovl662_stick_repeat_down;
u32 ovl662_stick_repeat_left;
u32 ovl662_stick_repeat_right;
u32 ovl662_dpad_repeat_up;
u32 ovl662_dpad_repeat_down;
u32 ovl662_dpad_repeat_left;
u32 ovl662_dpad_repeat_right;
GarageActionRecord16 ovl662_editor_action_records[53] = {{.prefix = {.callback = (GarageStep)&ovl662_tool_undo_modal}, .render = (GarageStep)&ovl662_draw_button_highlight}, {.prefix = {.callback = (GarageStep)&ovl662_tool_rotate_layer}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 1, .key = 1}, {.prefix = {.callback = (GarageStep)&ovl662_tool_mirror_layer_h}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 2, .key = 2}, {.prefix = {.callback = (GarageStep)&ovl662_tool_flip_layer_v}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 3, .key = 3}, {.prefix = {.callback = (GarageStep)&ovl662_tool_center_cursor}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 4, .key = 4}, {.prefix = {.callback = (GarageStep)&ovl662_tool_clear_layer}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 5, .key = 5}, {.prefix = {.callback = (GarageStep)&ovl662_tool_load_card_dialog}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 6, .key = 6}, {.prefix = {.callback = (GarageStep)&ovl662_tool_cycle_preview_bg}, .render = (GarageStep)&ovl662_draw_tool_button, .argument = 7, .key = 7}, {.prefix = {.callback = (GarageStep)&ovl662_tool_cycle_zoom}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 8, .key = 8}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_pen}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 9, .key = 9}, {.prefix = {.callback = (GarageStep)&ovl662_tool_toggle_emblem_side}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 10, .key = 10}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_line}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 11, .key = 11}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_fill}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 12, .key = 12}, {.prefix = {.callback = (GarageStep)&ovl662_tool_cycle_brush_size}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 13, .key = 13}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_move}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 14, .key = 14}, {.prefix = {.callback = (GarageStep)&ovl662_tool_swap_emblem_sides}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 15, .key = 15}, {.prefix = {.callback = (GarageStep)&ovl662_tool_merge_layers}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 16, .key = 16}, {.prefix = {.callback = (GarageStep)&ovl662_tool_save_emblem_dialog}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 17, .key = 17}, {.prefix = {.callback = (GarageStep)&ovl662_tool_exit_editor_dialog}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 18, .key = 18}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_layer_0}, .render = (GarageStep)&ovl662_draw_canvas_grid, .argument = 19, .key = 19}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_layer_1}, .render = (GarageStep)&ovl662_draw_canvas_grid, .argument = 20, .key = 20}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_01}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 21, .key = 21}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_02}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 22, .key = 22}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_03}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 23, .key = 23}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_04}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 24, .key = 24}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_05}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 25, .key = 25}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_06}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 26, .key = 26}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_07}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 27, .key = 27}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_08}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 28, .key = 28}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_09}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 29, .key = 29}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_10}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 30, .key = 30}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_11}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 31, .key = 31}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_12}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 32, .key = 32}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_13}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 33, .key = 33}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_14}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 34, .key = 34}, {.prefix = {.callback = (GarageStep)&ovl662_tool_select_color_15}, .render = (GarageStep)&ovl662_draw_palette_swatch, .argument = 35, .key = 35}, {.prefix = {.callback = (GarageStep)&ovl662_scroll_canvas_up}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 36, .key = 36}, {.prefix = {.callback = (GarageStep)&ovl662_scroll_canvas_down}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 37, .key = 37}, {.prefix = {.callback = (GarageStep)&ovl662_scroll_canvas_left}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 38, .key = 38}, {.prefix = {.callback = (GarageStep)&ovl662_scroll_canvas_right}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 39, .key = 39}, {.prefix = {.callback = (GarageStep)&ovl662_step_palette_red_down}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 40, .key = 40}, {.prefix = {.callback = (GarageStep)&ovl662_step_palette_red_up}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 41, .key = 41}, {.prefix = {.callback = (GarageStep)&ovl662_step_palette_green_down}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 42, .key = 42}, {.prefix = {.callback = (GarageStep)&ovl662_step_palette_green_up}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 43, .key = 43}, {.prefix = {.callback = (GarageStep)&ovl662_step_palette_blue_down}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 44, .key = 44}, {.prefix = {.callback = (GarageStep)&ovl662_step_palette_blue_up}, .render = (GarageStep)&ovl662_draw_button_highlight, .argument = 45, .key = 45}, {.prefix = {.callback = (GarageStep)&ovl662_drag_h_scrollbar}, .render = (GarageStep)&ovl662_draw_scrollbars, .argument = 46, .key = 46}, {.prefix = {.callback = (GarageStep)&ovl662_drag_v_scrollbar}, .render = (GarageStep)&ovl662_draw_scrollbars, .argument = 47, .key = 47}, {.prefix = {.callback = (GarageStep)&ovl662_drag_palette_red_bar}, .render = (GarageStep)&ovl662_draw_scrollbars, .argument = 48, .key = 48}, {.prefix = {.callback = (GarageStep)&ovl662_drag_palette_green_bar}, .render = (GarageStep)&ovl662_draw_scrollbars, .argument = 49, .key = 49}, {.prefix = {.callback = (GarageStep)&ovl662_drag_palette_blue_bar}, .render = (GarageStep)&ovl662_draw_scrollbars, .argument = 50, .key = 50}, {.prefix = {.callback = (GarageStep)&ovl662_drag_preview_viewport}, .render = (GarageStep)&ovl662_draw_emblem_preview_box, .key = 51}, {.prefix = {.callback = (GarageStep)&ovl662_dispatch_canvas_tool}, .render = (GarageStep)&ovl662_draw_zoomed_canvas, .key = 52}};
GarageButton ovl662_editor_buttons[46] = {{.rectangle = {0x13, 0x45, 0x38, 0x10}, .sprite08 = 2}, {.rectangle = {0x13, 0x5D, 0x10, 0x20}, .sprite08 = 4, .action0A = 1, .key0C = 1}, {.rectangle = {0x27, 0x5D, 0x10, 0x20}, .sprite08 = 6, .action0A = 2, .key0C = 2}, {.rectangle = {0x3B, 0x5D, 0x10, 0x20}, .sprite08 = 8, .action0A = 3, .key0C = 3}, {.rectangle = {0x13, 0x85, 0x10, 0x20}, .sprite08 = 10, .action0A = 4, .key0C = 4}, {.rectangle = {0x27, 0x85, 0x10, 0x20}, .sprite08 = 12, .action0A = 5, .key0C = 5}, {.rectangle = {0x3B, 0x85, 0x10, 0x20}, .sprite08 = 14, .action0A = 6, .key0C = 6}, {.rectangle = {0x13, 0xAD, 0x24, 0x20}, .action0A = 7, .key0C = 7}, {.rectangle = {0x3B, 0xAD, 0x10, 0x20}, .sprite08 = 24, .action0A = 8, .key0C = 8}, {.rectangle = {0x17, 0xD5, 0x14, 0x28}, .sprite08 = 26, .action0A = 9, .key0C = 9}, {.rectangle = {0x33, 0xD5, 0x14, 0x28}, .sprite08 = 28, .action0A = 10, .key0C = 10}, {.rectangle = {0x17, 0x105, 0x14, 0x28}, .sprite08 = 30, .action0A = 11, .key0C = 11}, {.rectangle = {0x33, 0x105, 0x14, 0x28}, .sprite08 = 32, .action0A = 12, .key0C = 12}, {.rectangle = {0x17, 0x135, 0x14, 0x28}, .sprite08 = 34, .action0A = 13, .key0C = 13}, {.rectangle = {0x33, 0x135, 0x14, 0x28}, .sprite08 = 36, .action0A = 14, .key0C = 14}, {.rectangle = {0x11F, 0xE5, 0x10, 0x20}, .sprite08 = 38, .action0A = 15, .key0C = 15}, {.rectangle = {0x11F, 0x135, 0x10, 0x20}, .sprite08 = 40, .action0A = 16, .key0C = 16}, {.rectangle = {0xED, 0x175, 0x20, 0x20}, .sprite08 = 42, .action0A = 17, .key0C = 17}, {.rectangle = {0x10F, 0x175, 0x20, 0x20}, .sprite08 = 44, .action0A = 18, .key0C = 18}, {.rectangle = {0xEF, 0xD7, 0x20, 0x40}, .action0A = 19, .key0C = 19}, {.rectangle = {0xEF, 0x127, 0x20, 0x40}, .action0A = 20, .key0C = 20}, {.rectangle = {0x12, 0x169, 0xA, 0x10}, .action0A = 21, .key0C = 21}, {.rectangle = {0x1E, 0x169, 0xA, 0x10}, .action0A = 22, .key0C = 22}, {.rectangle = {0x2A, 0x169, 0xA, 0x10}, .action0A = 23, .key0C = 23}, {.rectangle = {0x36, 0x169, 0xA, 0x10}, .action0A = 24, .key0C = 24}, {.rectangle = {0x42, 0x169, 0xA, 0x10}, .action0A = 25, .key0C = 25}, {.rectangle = {0x4E, 0x169, 0xA, 0x10}, .action0A = 26, .key0C = 26}, {.rectangle = {0x5A, 0x169, 0xA, 0x10}, .action0A = 27, .key0C = 27}, {.rectangle = {0x66, 0x169, 0xA, 0x10}, .action0A = 28, .key0C = 28}, {.rectangle = {0x12, 0x17D, 0xA, 0x10}, .action0A = 29, .key0C = 29}, {.rectangle = {0x1E, 0x17D, 0xA, 0x10}, .action0A = 30, .key0C = 30}, {.rectangle = {0x2A, 0x17D, 0xA, 0x10}, .action0A = 31, .key0C = 31}, {.rectangle = {0x36, 0x17D, 0xA, 0x10}, .action0A = 32, .key0C = 32}, {.rectangle = {0x42, 0x17D, 0xA, 0x10}, .action0A = 33, .key0C = 33}, {.rectangle = {0x4E, 0x17D, 0xA, 0x10}, .action0A = 34, .key0C = 34}, {.rectangle = {0x5A, 0x17D, 0xA, 0x10}, .action0A = 35, .key0C = 35}, {.rectangle = {0xD7, 0x45, 6, 0x12}, .sprite08 = 47, .action0A = 36, .key0C = 36}, {.rectangle = {0xD7, 0x137, 6, 0x12}, .sprite08 = 49, .action0A = 37, .key0C = 37}, {.rectangle = {0x51, 0x14F, 9, 0xC}, .sprite08 = 51, .action0A = 38, .key0C = 38}, {.rectangle = {0xCA, 0x14F, 9, 0xC}, .sprite08 = 55, .action0A = 39, .key0C = 39}, {.rectangle = {0x8D, 0x163, 8, 0xC}, .sprite08 = 53, .action0A = 40, .key0C = 40}, {.rectangle = {0xD5, 0x163, 8, 0xC}, .sprite08 = 57, .action0A = 41, .key0C = 41}, {.rectangle = {0x8D, 0x175, 8, 0xC}, .sprite08 = 53, .action0A = 42, .key0C = 42}, {.rectangle = {0xD5, 0x175, 8, 0xC}, .sprite08 = 57, .action0A = 43, .key0C = 43}, {.rectangle = {0x8D, 0x187, 8, 0xC}, .sprite08 = 53, .action0A = 44, .key0C = 44}, {.rectangle = {0xD5, 0x187, 8, 0xC}, .sprite08 = 57, .action0A = 45, .key0C = 45}};
GarageActionPrefix *ovl662_action_lookup[54];

/* Writable state of this unit; ovl662_reset_module restores it. */
static const OverlayStateRegion ovl662_editor_input_regions[] = {
    RR_OVERLAY_STATE(ovl662_action_lookup),
    RR_OVERLAY_STATE(ovl662_analog_cursor_accum),
    RR_OVERLAY_STATE(ovl662_dpad_repeat_down),
    RR_OVERLAY_STATE(ovl662_dpad_repeat_left),
    RR_OVERLAY_STATE(ovl662_dpad_repeat_right),
    RR_OVERLAY_STATE(ovl662_dpad_repeat_up),
    RR_OVERLAY_STATE(ovl662_editor_action_records),
    RR_OVERLAY_STATE(ovl662_editor_buttons),
    RR_OVERLAY_STATE(ovl662_stick_repeat_down),
    RR_OVERLAY_STATE(ovl662_stick_repeat_left),
    RR_OVERLAY_STATE(ovl662_stick_repeat_right),
    RR_OVERLAY_STATE(ovl662_stick_repeat_up)};
const OverlayStateTable ovl662_editor_input_state = {ovl662_editor_input_regions, sizeof(ovl662_editor_input_regions) / sizeof(ovl662_editor_input_regions[0])};

/* Initial values of the globals this module owns. */
ControllerState5C PlayerControllers[2];
