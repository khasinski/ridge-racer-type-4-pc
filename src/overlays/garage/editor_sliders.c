#include "rr/overlay_module.h"
#include "rr/overlay662_scroll_actions.h"
#include "rr/overlay662_palette_edit.h"
#include "rr/overlay662_tool_action.h"
#include "rr/overlay662_scroll_drag.h"
#include "rr/overlay662_preview_drag.h"

void ovl662_scroll_canvas_up(void) {
    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(215, 69, 6, 18) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 36;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_button_clip_rect(36);
    }
    if (ovl662_editor_cursor_state.words[1] & 0x40) {
        ((GarageButton *)ovl662_button_rect_lookup[36])->timer0E = 7;
        if (--ovl662_editor_workspace.reset08[1] < 0) {
            ovl662_editor_workspace.reset08[1] = 0;
            PlaySfx(4);
        } else {
            PlaySfx(0);
        }
    }
}

void ovl662_scroll_canvas_down(void) {
    s32 limit = 64 - 64 / (ovl662_editor_zoom_shift_alias >> 1);

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(215, 311, 6, 18) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 37;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_button_clip_rect(37);
    }
    if (ovl662_editor_cursor_state.words[1] & 0x40) {
        ((GarageButton *)ovl662_button_rect_lookup[37])->timer0E = 7;
        if (++ovl662_editor_workspace.reset08[1] > limit) {
            ovl662_editor_workspace.reset08[1] = limit;
            PlaySfx(4);
        } else {
            PlaySfx(0);
        }
    }
}

void ovl662_scroll_canvas_left(void) {
    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(81, 335, 9, 12) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 38;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_button_clip_rect(38);
    }
    if (ovl662_editor_cursor_state.words[1] & 0x40) {
        ((GarageButton *)ovl662_button_rect_lookup[38])->timer0E = 7;
        if (--ovl662_editor_workspace.reset08[0] < 0) {
            ovl662_editor_workspace.reset08[0] = 0;
            PlaySfx(4);
        } else {
            PlaySfx(0);
        }
    }
}

void ovl662_scroll_canvas_right(void) {
    s32 limit = 64 - 64 / (ovl662_editor_zoom_shift_alias >> 1);

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(202, 335, 9, 12) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 39;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_button_clip_rect(39);
    }
    if (ovl662_editor_cursor_state.words[1] & 0x40) {
        ((GarageButton *)ovl662_button_rect_lookup[39])->timer0E = 7;
        if (++ovl662_editor_workspace.reset08[0] > limit) {
            ovl662_editor_workspace.reset08[0] = limit;
            PlaySfx(4);
        } else {
            PlaySfx(0);
        }
    }
}

void ovl662_step_palette_red_down(void) {
    u16 *pal = (u16 *)((PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]+(ovl662_editor_workspace.config22*2));
    s32 val = *pal & 0x1F;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(141, 355, 8, 12) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 40;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_button_clip_rect(40);
        ovl662_backup_layer_and_palette();
    }
    if (ovl662_editor_cursor_state.words[1] & 0x40) {
        val--;
        ((GarageButton *)ovl662_button_rect_lookup[40])->timer0E = 7;
        if (val < 0) {
            val = 0;
            PlaySfx(4);
        } else {
            PlaySfx(0);
        }
        *pal = (*pal & 0xFFE0) | val;
    }
}

void ovl662_step_palette_red_up(void) {
    u16 *pal = (u16 *)((PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]+(ovl662_editor_workspace.config22*2));
    u16 val = *pal & 0x1F;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(213, 355, 8, 12) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 41;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_button_clip_rect(41);
        ovl662_backup_layer_and_palette();
    }
    if (ovl662_editor_cursor_state.words[1] & 0x40) {
        val++;
        ((GarageButton *)ovl662_button_rect_lookup[41])->timer0E = 7;
        if (val >= 32) {
            val = 31;
            PlaySfx(4);
        } else {
            PlaySfx(0);
        }
        *pal = (*pal & 0xFFE0) | val;
    }
}

void ovl662_step_palette_green_down(void) {
    u16 *pal = (u16 *)((PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]+(ovl662_editor_workspace.config22*2));
    s32 val = (*pal >> 5) & 0x1F;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(141, 373, 8, 12) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 42;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_button_clip_rect(42);
        ovl662_backup_layer_and_palette();
    }
    if (ovl662_editor_cursor_state.words[1] & 0x40) {
        val--;
        ((GarageButton *)ovl662_button_rect_lookup[42])->timer0E = 7;
        if (val < 0) {
            val = 0;
            PlaySfx(4);
        } else {
            PlaySfx(0);
        }
        *pal = (*pal & 0xFC1F) | (val << 5);
    }
}

void ovl662_step_palette_green_up(void) {
    u16 *pal = (u16 *)((PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]+(ovl662_editor_workspace.config22*2));
    u16 val = (*pal >> 5) & 0x1F;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(213, 373, 8, 12) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 43;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_button_clip_rect(43);
        ovl662_backup_layer_and_palette();
    }
    if (ovl662_editor_cursor_state.words[1] & 0x40) {
        val++;
        ((GarageButton *)ovl662_button_rect_lookup[43])->timer0E = 7;
        if (val >= 32) {
            val = 31;
            PlaySfx(4);
        } else {
            PlaySfx(0);
        }
        *pal = (*pal & 0xFC1F) | (val << 5);
    }
}

void ovl662_step_palette_blue_down(void) {
    u16 *pal = (u16 *)((PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]+(ovl662_editor_workspace.config22*2));
    s32 val = (*pal >> 10) & 0x1F;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(141, 391, 8, 12) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 44;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_button_clip_rect(44);
        ovl662_backup_layer_and_palette();
    }
    if (ovl662_editor_cursor_state.words[1] & 0x40) {
        val--;
        ((GarageButton *)ovl662_button_rect_lookup[44])->timer0E = 7;
        if (val < 0) {
            val = 0;
            PlaySfx(4);
        } else {
            PlaySfx(0);
        }
        *pal = (*pal & 0x83FF) | (val << 10);
    }
}

void ovl662_step_palette_blue_up(void) {
    u16 *pal = (u16 *)((PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]+(ovl662_editor_workspace.config22*2));
    u16 val = (*pal >> 10) & 0x1F;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(213, 391, 8, 12) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 45;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_button_clip_rect(45);
        ovl662_backup_layer_and_palette();
    }
    if (ovl662_editor_cursor_state.words[1] & 0x40) {
        val++;
        ((GarageButton *)ovl662_button_rect_lookup[45])->timer0E = 7;
        if (val >= 32) {
            val = 31;
            PlaySfx(4);
        } else {
            PlaySfx(0);
        }
        *pal = (*pal & 0x83FF) | (val << 10);
    }
}

void ovl662_drag_h_scrollbar(void) {
    s32 imageTravel;
    s32 trackTravel;
    s32 limit;
    s32 imageThumb;
    u32 trackThumb;
    s32 projected;
    s32 relative;
    u16 narrowed;
    s32 coordinate;
    GarageSelectionPrefix *selection;
    GarageSelectionPrefix *selectionOwner;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] = (ovl662_editor_cursor_state.words[0] & ~8);
            ovl662_set_clip_rect(0xD, 0x3F, 0x126, 0x15A);
            return;
        }
    } else if (((u8) ovl662_editor_workspace.config1C >= 4U) && (ovl662_point_outside_rect(0x5A, 0x151, 0x70, 8) == 0) && (ovl662_editor_cursor_state.words[1] & 1)) {
        ovl662_editor_cursor_state.words[4] = 0x2E;
        ovl662_editor_cursor_state.words[0] = (ovl662_editor_cursor_state.words[0] | 8);
        ovl662_set_clip_rect(0x5A, 0x151, 0x70, 8);
        imageThumb = 0x40 / ((u8) ovl662_editor_workspace.config1C >> 1);
        selectionOwner = &ovl662_editor_workspace;
        trackThumb = 0x70 / ((u8) selectionOwner->config1C >> 1);
        imageTravel = 0x40 - imageThumb;
        trackTravel = 0x70 - trackThumb;
        projected = ((ovl662_editor_cursor_state.points.cursor14.x - 0x5A) * imageTravel) / trackTravel;
        ovl662_hscroll_image_thumb = imageThumb;
        ovl662_hscroll_image_travel = imageTravel;
        ovl662_hscroll_track_thumb = trackThumb;
        ovl662_hscroll_track_travel = trackTravel;
        relative = projected - (s16) ovl662_editor_workspace.reset08[0];
        if ((relative < 0) || (relative >= imageThumb)) {
            ovl662_hscroll_grab_offset = (u16) ((s32) ((trackThumb >> 1) * imageTravel) / trackTravel);
        } else {
            ovl662_hscroll_grab_offset = projected - ovl662_editor_workspace.reset08[0];
        }
    } else {
        return;
    }
    {
        coordinate=ovl662_editor_cursor_x_alias-0x5A;
        limit = ovl662_hscroll_image_travel;
        selection=&ovl662_editor_workspace;
        narrowed = ((coordinate * limit) / ovl662_hscroll_track_travel) - ovl662_hscroll_grab_offset;
        selection->reset08[0] = narrowed;
        if ((s16) narrowed < 0) {
            selection->reset08[0] = 0U;
            return;
        }
        if (limit < (s16) narrowed) {
            selection->reset08[0] = (u16) ovl662_hscroll_image_travel;
        }
    }
}

void ovl662_drag_v_scrollbar(void) {
    s32 imageTravel;
    s32 trackTravel;
    s32 limit;
    s32 imageThumb;
    u32 trackThumb;
    s32 projected;
    s32 relative;
    u16 narrowed;
    GarageSelectionPrefix *selection;
    s32 coordinate;
    GarageSelectionPrefix *selectionOwner;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] = (ovl662_editor_cursor_state.words[0] & ~8);
            ovl662_set_clip_rect(0xD, 0x3F, 0x126, 0x15A);
            return;
        }
    } else if (((u8) ovl662_editor_workspace.config1C >= 4U) && (ovl662_point_outside_rect(0xD8, 0x57, 4, 0xE0) == 0) && (ovl662_editor_cursor_state.words[1] & 1)) {
        ovl662_editor_cursor_state.words[4] = 0x2F;
        ovl662_editor_cursor_state.words[0] = (ovl662_editor_cursor_state.words[0] | 8);
        ovl662_set_clip_rect(0xD8, 0x57, 4, 0xE0);
        imageThumb = 0x40 / ((u8) ovl662_editor_workspace.config1C >> 1);
        selectionOwner = &ovl662_editor_workspace;
        trackThumb = 0xE0 / ((u8) selectionOwner->config1C >> 1);
        imageTravel = 0x40 - imageThumb;
        trackTravel = 0xE0 - trackThumb;
        projected = ((ovl662_editor_cursor_state.points.cursor14.y - 0x57) * imageTravel) / trackTravel;
        ovl662_vscroll_image_thumb = imageThumb;
        ovl662_vscroll_image_travel = imageTravel;
        ovl662_vscroll_track_thumb = trackThumb;
        ovl662_vscroll_track_travel = trackTravel;
        relative = projected - (s16) ovl662_editor_workspace.reset08[1];
        if ((relative < 0) || (relative >= imageThumb)) {
            ovl662_vscroll_grab_offset = (u16) ((s32) ((trackThumb >> 1) * imageTravel) / trackTravel);
        } else {
            ovl662_vscroll_grab_offset = projected - ovl662_editor_workspace.reset08[1];
        }
    } else {
        return;
    }
    {
        coordinate=ovl662_editor_cursor_y_alias-0x57;
        limit = ovl662_vscroll_image_travel;
        selection=&ovl662_editor_workspace;
        narrowed = ((coordinate * limit) / ovl662_vscroll_track_travel) - ovl662_vscroll_grab_offset;
        selection->reset08[1] = narrowed;
        if ((s16) narrowed < 0) {
            selection->reset08[1] = 0U;
            return;
        }
        if (limit < (s16) narrowed) {
            selection->reset08[1] = (u16) ovl662_vscroll_image_travel;
        }
    }
}

void ovl662_drag_palette_red_bar(void) {
    u16 val;
    u16 tmp;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(149, 355, 65, 13) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 48;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_clip_rect(149, 355, 64, 12);
        ovl662_red_bar_dragging = 0;
        ovl662_red_bar_saved_color = *(u16 *)((ovl662_editor_workspace.config22*2)+(PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]) & 0x1F;
    }
    val = (ovl662_editor_cursor_state.halves[10] - 149) / 2;
    if (ovl662_red_bar_saved_color != val) {
        if (ovl662_red_bar_dragging == 0) {
            ovl662_red_bar_dragging = 1;
            ovl662_backup_layer_and_palette();
        }
    }
    tmp = *(u16 *)((ovl662_editor_workspace.config22*2)+(PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]) & 0xFFE0;
    *(u16 *)((ovl662_editor_workspace.config22*2)+(PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]) = tmp | val;
}

void ovl662_drag_palette_green_bar(void) {
    u16 val;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(149, 373, 65, 13) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 49;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_clip_rect(149, 373, 64, 12);
        ovl662_green_bar_dragging = 0;
        ovl662_green_bar_saved_color = (*(u16 *)((ovl662_editor_workspace.config22*2)+(PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]) >> 5) & 0x1F;
    }
    val = (ovl662_editor_cursor_state.halves[10] - 149) / 2;
    if (ovl662_green_bar_saved_color != val) {
        if (ovl662_green_bar_dragging == 0) {
            ovl662_green_bar_dragging = 1;
            ovl662_backup_layer_and_palette();
        }
    }
    *(u16 *)((ovl662_editor_workspace.config22*2)+(PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]) =
        (*(u16 *)((ovl662_editor_workspace.config22*2)+(PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]) & 0xFC1F) | (val << 5);
}

void ovl662_drag_palette_blue_bar(void) {
    u16 val;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(149, 391, 65, 13) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_cursor_state.words[4] = 50;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_clip_rect(149, 391, 64, 12);
        ovl662_blue_bar_dragging = 0;
        ovl662_blue_bar_saved_color = (*(u16 *)((ovl662_editor_workspace.config22*2)+(PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]) >> 10) & 0x1F;
    }
    val = (ovl662_editor_cursor_state.halves[10] - 149) / 2;
    if (ovl662_blue_bar_saved_color != val) {
        if (ovl662_blue_bar_dragging == 0) {
            ovl662_blue_bar_dragging = 1;
            ovl662_backup_layer_and_palette();
        }
    }
    *(u16 *)((ovl662_editor_workspace.config22*2)+(PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]) =
        (*(u16 *)((ovl662_editor_workspace.config22*2)+(PaletteAddress)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21]) & 0x83FF) | (val << 10);
}

void ovl662_drag_preview_viewport(void) {
    u16 *grab;
    s32 offsetX;
    s32 thumb;
    s32 center;
    u16 nextX;
    u32 deltaY;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_cursor_state.words[0] = (ovl662_editor_cursor_state.words[0] & ~8);
            ovl662_set_clip_rect(0xD, 0x3F, 0x126, 0x15A);
            return;
        }
    } else if (((u8) ovl662_editor_workspace.config1C >= 4U) && (ovl662_point_outside_rect(0xED, 0x47, 0x40, 0x80) == 0) && (ovl662_editor_cursor_state.words[1] & 1)) {
        ovl662_editor_cursor_state.words[4] = 0x33;
        ovl662_editor_cursor_state.words[0] = (ovl662_editor_cursor_state.words[0] | 8);
        ovl662_set_clip_rect(0xED, 0x47, 0x40, 0x80);
        thumb = 0x40 / ((u8) ovl662_editor_workspace.config1C >> 1);
        ovl662_preview_drag_thumb = thumb;
        ovl662_preview_drag_limit = 0x40 - thumb;
        if (ovl662_point_outside_rect((s16) (ovl662_editor_workspace.reset08[0] + 0xED), (s16) ((ovl662_editor_workspace.reset08[1] * 2) + 0x47), (s16) ovl662_preview_drag_thumb, thumb * 2) != 0) {
            grab=ovl662_preview_drag_grab;
            center = ovl662_preview_drag_thumb / 2;
            grab[1] = (u16) center;
            ovl662_preview_drag_grab[0] = (u16) center;
        } else {
            ovl662_preview_drag_grab[0] = (u16) ((ovl662_editor_cursor_state.points.cursor14.x - 0xED) - ovl662_editor_workspace.reset08[0]);
            ovl662_preview_drag_grab[1] = (u16) ((((s16) ovl662_editor_cursor_state.points.cursor14.y / 2) - 0x23) - (u16) ovl662_editor_workspace.reset08[1]);
        }
    } else {
        return;
    }
    {
        offsetX = ovl662_preview_drag_grab[0] + 0xED;

        nextX = ovl662_editor_cursor_state.points.cursor14.x - offsetX;
        ovl662_editor_workspace.reset08[0] = nextX;
        deltaY = (s16) ovl662_editor_cursor_state.points.cursor14.y - 0x47;
        ovl662_editor_workspace.reset08[1] = (s16) (((s32) deltaY / 2) - ovl662_preview_drag_grab[1]);
        if ((s16) nextX < 0) {
            ovl662_editor_workspace.reset08[0] = 0U;
        } else if (ovl662_preview_drag_limit < (s16) nextX) {
            ovl662_editor_workspace.reset08[0] = (u16) ovl662_preview_drag_limit;
        }
        if (ovl662_editor_workspace.reset08[1] < 0) {
            ovl662_editor_workspace.reset08[1] = 0;
            return;
        }
        if (ovl662_preview_drag_limit < ovl662_editor_workspace.reset08[1]) {
            ovl662_editor_workspace.reset08[1] = (s16) (u16) ovl662_preview_drag_limit;
        }
    }
}

/* Initial values of the overlay globals this module owns. */
u16 ovl662_hscroll_grab_offset;
s32 ovl662_hscroll_image_thumb;
s32 ovl662_hscroll_image_travel;
s32 ovl662_hscroll_track_thumb;
s32 ovl662_hscroll_track_travel;
u16 ovl662_vscroll_grab_offset;
s32 ovl662_vscroll_image_thumb;
s32 ovl662_vscroll_image_travel;
s32 ovl662_vscroll_track_thumb;
s32 ovl662_vscroll_track_travel;
GaragePreviewGrab ovl662_preview_drag_grab;
s32 ovl662_preview_drag_thumb;
s32 ovl662_preview_drag_limit;
GarageRectangle *ovl662_button_rect_lookup[46];
GarageWorkingStorage ovl662_editor_cursor_state;
s32 ovl662_red_bar_dragging;
u16 ovl662_red_bar_saved_color;
s32 ovl662_green_bar_dragging;
u16 ovl662_green_bar_saved_color;
s32 ovl662_blue_bar_dragging;
u16 ovl662_blue_bar_saved_color;

/* Writable state of this unit; ovl662_reset_module restores it. */
static const OverlayStateRegion ovl662_editor_sliders_regions[] = {
    RR_OVERLAY_STATE(ovl662_blue_bar_dragging),
    RR_OVERLAY_STATE(ovl662_blue_bar_saved_color),
    RR_OVERLAY_STATE(ovl662_button_rect_lookup),
    RR_OVERLAY_STATE(ovl662_editor_cursor_state),
    RR_OVERLAY_STATE(ovl662_green_bar_dragging),
    RR_OVERLAY_STATE(ovl662_green_bar_saved_color),
    RR_OVERLAY_STATE(ovl662_hscroll_grab_offset),
    RR_OVERLAY_STATE(ovl662_hscroll_image_thumb),
    RR_OVERLAY_STATE(ovl662_hscroll_image_travel),
    RR_OVERLAY_STATE(ovl662_hscroll_track_thumb),
    RR_OVERLAY_STATE(ovl662_hscroll_track_travel),
    RR_OVERLAY_STATE(ovl662_preview_drag_grab),
    RR_OVERLAY_STATE(ovl662_preview_drag_limit),
    RR_OVERLAY_STATE(ovl662_preview_drag_thumb),
    RR_OVERLAY_STATE(ovl662_red_bar_dragging),
    RR_OVERLAY_STATE(ovl662_red_bar_saved_color),
    RR_OVERLAY_STATE(ovl662_vscroll_grab_offset),
    RR_OVERLAY_STATE(ovl662_vscroll_image_thumb),
    RR_OVERLAY_STATE(ovl662_vscroll_image_travel),
    RR_OVERLAY_STATE(ovl662_vscroll_track_thumb),
    RR_OVERLAY_STATE(ovl662_vscroll_track_travel)};
const OverlayStateTable ovl662_editor_sliders_state = {ovl662_editor_sliders_regions, sizeof(ovl662_editor_sliders_regions) / sizeof(ovl662_editor_sliders_regions[0])};
