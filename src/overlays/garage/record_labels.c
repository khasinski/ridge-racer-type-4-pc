#include "rr/overlay_module.h"
#include "game/menu/menu8.h"
#include "game/gfx/sprefct.h"
#include "rr/overlay662_history_labels.h"
#include "rr/overlay662_character_grid.h"
#include "rr/overlay_extra_labels.h"
#include "rr/badge_rows.h"
#include "rr/overlay662_time.h"
#include "rr/string_tail_copy.h"
#include "rr/video_timing.h"

extern u8 ovl662_extra_record_col_dx[4];
extern u8 ovl662_extra_record_row_dy[4];
extern u8 ovl662_extra_record_label_glyphs[3];
/* The decal slots' edit times. The PS1 resident card code (scan, save,
 * load) and this panel share the one object at 0x801272F0. */
extern s32 GarageExtraDefaultTimes[3];

void FormatDecimalDigits(char *output, s32 value, s32 digitsMinusOne);

void *ovl662_draw_stage_history_label(GpuDmaTag *arg0, void *arg1, s32 arg2) {
    char buf[8];
    s32 i;
    s32 x;
    s32 y;
    s32 flag = 0;

    x = BadgeRows[arg2].x04 + 8;
    y = BadgeRows[arg2].y06 + 32;
    for (i = 0; i < 4; i++) {
        if (SelectionRows.choice.bytes[2] == SaveProgress.history6C[i].parameter &&
            SelectionRows.choice.bytes[0] == SaveProgress.history6C[i].selection &&
            SelectionRows.choice.bytes[1] == SaveProgress.history6C[i].variant) {
            flag = rsin((((ScreenFrame * 128U) - 682U)) & 0xFFE) < 0;
            break;
        }
    }
    for (i = 0; i < 8; i++) {
        buf[i] = 0;
    }
    for (i = 0; i < 3; i++) {
        arg1 = AddSpritePrim(arg0, arg1, x, y, &UiFontGlyphs[ovl662_stage_history_glyphs[i]], 0x7FF8, 0);
        x += UiFontGlyphs[ovl662_stage_history_glyphs[i]].w + 2;
        if (i == 2) {
            FormatDecimalDigits(buf, SelectionRows.choice.bytes[2] + 1, 1);
            arg1 = AddTextSprites(arg0, arg1, x, y, buf, flag + 0x7FF8, 0, 0);
        }
    }
    return AddDrawModePrim(arg0, arg1, 56);
}

void *ovl662_draw_team_car_counts_label(GpuDmaTag *order, void *cursor, s32 row)
{
    s32 i, x, y;
    s32 flag = 0;

    x = BadgeRows[row].x04 + 8;
    y = BadgeRows[row].y06 + 32;
    for (i = 0; i < 4; i++) {
        if (SelectionRows.choice.bytes[0] == SaveProgress.history6C[i].selection) {
            flag = rsin(
                (((ScreenFrame * 128U) - 682U)) & 0xFFE) < 0;
            break;
        }
    }
    for (i = 0; i < 7; i++) {
        if (ovl662_team_count_glyphs[i] == -1) {
            cursor = AddTextSprites(
                order, cursor, x, y,
                FormatCountOfTotal((s16)ovl662_team_unlocked_counts[SelectionRows.choice.bytes[0]], 80, 1),
                flag + 0x7FF8, 0, 0);
            x += 34;
            cursor = AddTilePrim(order, cursor, x, y - 2, 1, 20, 255, 255, 255);
            x += 8;
        } else if (ovl662_team_count_glyphs[i] == -2) {
            cursor = AddTextSprites(
                order, cursor, x, y,
                FormatCountOfTotal((s16)ovl662_total_unlocked_cars, 320, 2),
                flag + 0x7FF8, 0, 0);
        } else {
            cursor = AddSpritePrim(order, cursor, x, y,
                &UiFontGlyphs[ovl662_team_count_glyphs[i]], 0x7FF8, 0);
            x += UiFontGlyphs[ovl662_team_count_glyphs[i]].w + 2;
        }
    }
    return AddDrawModePrim(order, cursor, 56);
}

void *ovl662_draw_maker_car_count_label(GpuDmaTag *arg0, void *arg1, s32 arg2) {
    s32 i;
    s32 x;
    s32 y;
    s32 flag = 0;
    s32 t1;

    x = BadgeRows[arg2].x04 + 8;
    y = BadgeRows[arg2].y06 + 32;
    for (i = 0; i < 4; i++) {
        if (SelectionRows.choice.bytes[0] == SaveProgress.history6C[i].selection &&
            SelectionRows.choice.bytes[1] == SaveProgress.history6C[i].variant) {
            flag = rsin((((ScreenFrame * 128U) - 682U)) & 0xFFE) < 0;
            break;
        }
    }
    for (i = 0; i < 3; i++) {
        if (ovl662_maker_count_glyphs[i] == -1) {
            arg1 = AddTextSprites(
                arg0, arg1, x, y,
                FormatCountOfTotal(ovl662_team_maker_car_counts[SelectionRows.choice.bytes[0]][SelectionRows.choice.bytes[1]], 20, 1),
                flag + 0x7FF8, 0, 0);
            break;
        }
        arg1 = AddSpritePrim(arg0, arg1, x, y, &UiFontGlyphs[ovl662_maker_count_glyphs[i]], 0x7FF8, 0);
        t1 = i | 1;
        t1 &= 1;
        arg1 = (u8 *)arg1 + (t1 - 1);
        x += UiFontGlyphs[ovl662_maker_count_glyphs[i]].w + 2;
    }
    return AddDrawModePrim(arg0, arg1, 56);
}

void *ovl662_draw_car_number_label(GpuDmaTag *arg0, void *arg1, s32 arg2) {
    char buf[8];
    s32 i;
    s32 x;
    s32 y;

    x = BadgeRows[arg2].x04 + 8;
    y = BadgeRows[arg2].y06 + 32;
    for (i = 0; i < 8; i++) {
        buf[i] = 0;
    }
    for (i = 0; i < 3; i++) {
        arg1 = AddSpritePrim(arg0, arg1, x, y, &UiFontGlyphs[ovl662_car_number_glyphs[i]], 0x7FF8, 0);
        x += UiFontGlyphs[ovl662_car_number_glyphs[i]].w + 2;
        if (i == 2) {
            FormatDecimalDigits(buf, ovl662_selected_car_number, 1);
            arg1 = AddTextSprites(arg0, arg1, x, y, buf, 0x7FF8, 0, 0);
        }
    }
    return AddDrawModePrim(arg0, arg1, 56);
}

void *ovl662_draw_emblem_index_label(GpuDmaTag *arg0, void *arg1, s32 arg2) {
    s32 x;
    s32 y;
    s32 i = 0;
    s32 t1;
    x = BadgeRows[arg2].x04 + 8;
    y = BadgeRows[arg2].y06 + 32;
    for (i = 0; i < 4; i++) {
        if (ovl662_emblem_index_glyphs[i] == -1) {
            arg1 = AddTextSprites(arg0, arg1, x, y, FormatCountOfTotal(ovl662_selected_emblem_slot, 24, 1), 0x7FF8, 0, 0);
            break;
        }
        arg1 = AddSpritePrim(arg0, arg1, x, y, &UiFontGlyphs[ovl662_emblem_index_glyphs[i]], 0x7FF8, 0);
        t1 = x | 1;
        t1 &= 1;
        arg1 = (u8 *)arg1 + (t1 - 1);
        x += UiFontGlyphs[ovl662_emblem_index_glyphs[i]].w + 2;
    }
    return AddDrawModePrim(arg0, arg1, 56);
}

void *ovl662_draw_character_grid(GpuDmaTag *order, void *cursor, s32 row)
{
    char text[4];
    s32 i, x, y, dx, dy, clut, color;
    CharacterGridAddress clearAddress;

    x = BadgeRows[row].x04 + 36;
    y = BadgeRows[row].y06 + 56;
    /* Convert only the four in-bounds addresses to pointers. The last integer
     * decrement never constructs a C pointer before the buffer. */
    for (i = 3, clearAddress = (CharacterGridAddress)(text + 3); i >= 0; i--) {
        *(char *)clearAddress = 0;
        --clearAddress;
    }
    for (i = 0; i < 40; i++) {
        text[0] = NameCharset[i + 1];
        dx = (i % 10) * 24;
        dy = (i / 10) * 32;
        if (i == ovl662_name_grid_index) {
            color = rcos(((ScreenFrame * 128U) - 682U));
            if (color >= 0) {
                clut = 32760;
                color = ((color >> 6) + 100) * 2;
            } else {
                clut = 32761;
                color = 0;
            }
        } else {
            clut = 32760;
            color = 0;
        }
        cursor = AddTextSprites(order, cursor, x + dx, y + dy, text, clut, 0, color);
    }
    cursor = AddTilePrim(order, cursor, x - 16, y + 136, 256, 2, 255, 255, 255);
    y += 144;
    x -= 12;
    for (i = 0, dx = i; i < 4; i++) {
        cursor = AddTilePrim(order, cursor, x + dx, y, 1, 16, 255, 255, 255);
        dx += 4;
        if (i < 3) {
            if (i == (ovl662_name_grid_index - 40U)) {
                color = rcos(((ScreenFrame * 128U) - 682U));
                if (color >= 0) {
                    clut = 32760;
                    color = ((color >> 6) + 100) * 2;
                } else {
                    clut = 32761;
                    color = 0;
                }
            } else {
                clut = 32760;
                color = 0;
            }
            cursor = AddSpritePrim(order, cursor, x + dx, y,
                &UiFontGlyphs[ovl662_char_grid_cmd_glyphs[i]], clut, color);
            dx += UiFontGlyphs[ovl662_char_grid_cmd_glyphs[i]].w + 4;
        }
    }
    return AddDrawModePrim(order, cursor, 56);
}

void *ovl662_draw_name_entry_chars(GpuDmaTag *arg0, void *arg1, s32 arg2) {
    char buf[4];
    s32 i;
    s32 offset;
    s32 x;
    s32 y;
    s32 clut;
    s32 color;
    s32 t1;
    s32 x2;
    s32 c;

    x = BadgeRows[arg2].x04 + 8;
    y = BadgeRows[arg2].y06 + 72;
    for (i = 0; i < 4; i++) {
        buf[i] = 0;
    }
    for (i = 0, offset = i; i < 8; i++, offset += 12) {
        buf[0] = ovl662_name_entry_buffer[i];
        if (i == ovl662_name_entry_cursor) {
            clut = 0x7FF8;
            color = rcos(((ScreenFrame * 128U) - 682U));
            c = 0xFF;
            if (color < 0) {
                t1 = color | 1;
                t1 &= color;
                color &= t1;
                clut = 0x7FF9;
            }
            if (color < 0) {
                c = 8;
            }
            color = c;
        } else {
            clut = 0x7FF8;
            color = 0xFF;
        }
        x2 = x + offset;
        arg1 = AddTilePrim(arg0, arg1, x2, y, 8, 2, color, color, color);
        arg1 = AddTextSprites(arg0, arg1, x2 + 2, y - 20, buf, clut, 0, 0);
    }
    return AddDrawModePrim(arg0, arg1, 56);
}

void *ovl662_draw_extra_record_row(GpuDmaTag *ord, void *cur, s32 row)
{
    /* Only slot [1] is used; the array keeps the value in the frame at sp+44. */
    s32 time[2];
    s32 j;
    char text[8];
    s32 i;
    s32 width;
    char *p;
    s32 x, y;

    x = BadgeRows[row].x04;
    y = BadgeRows[row].y06;
    y += 32;
    width = BadgeRows[row].width0C;
    for (i = 0; i < 3; i++) {
        x += ovl662_extra_record_col_dx[i];
        cur = AddSpritePrim(ord, cur, x, y, &UiFontGlyphs[ovl662_extra_record_label_glyphs[i]], 0x7FF8, 0);
    }
    for (i = 7, p = text + 7; i >= 0; i--) {
        *p-- = 0;
    }
    y -= 8;
    for (i = 0; i < 3; i++) {
        x = BadgeRows[row].x04 + 8;
        FormatDecimalDigits(text, i + 1, 0);
        text[1] = 0;
        cur = AddTextSprites(ord, cur, x, y + ovl662_extra_record_row_dy[i + 1], text, 0x7FF8, 0, 0);
        time[1] = GarageExtraDefaultTimes[i];
        if (time[1] != -1) {
            x += ovl662_extra_record_col_dx[1];
            cur = AddSpritePrim(ord, cur, x, y + ovl662_extra_record_row_dy[i + 1], &UiFontGlyphs[42], 0x7FF8, 0);
            text[0] = i + 'A';
            text[1] = 0;
            cur = AddTextSprites(ord, cur, x + (UiFontGlyphs[42].w + 2), y + ovl662_extra_record_row_dy[i + 1], text, 0x7FF8, 0, 0);
            CopyStringClearTail(text, ovl662_format_record_time(time[1]));
            cur = AddTextSprites(ord, cur, x + ovl662_extra_record_col_dx[2], y + ovl662_extra_record_row_dy[i + 1], text, 0x7FF8, 1, 0);
        } else {
            cur = AddTilePrim(ord, cur, x + ovl662_extra_record_col_dx[1], y + ovl662_extra_record_row_dy[i + 1] + 8, 36, 2, 255, 255, 255);
        }
        if (i == GpSaveSlotCursor) {
            cur = AddTileSemiTransPrim(ord, cur, BadgeRows[row].x04, y + ovl662_extra_record_row_dy[i + 1] - 4, width, 24, 32, 32, 32);
        }
    }
    cur = AddDrawModePrim(ord, cur, 56);
    y += 28;
    x = BadgeRows[row].x04;
    cur = AddTilePrim(ord, cur, x, y, width, 2, 255, 255, 255);
    y -= 28;
    for (i = 0; i < 4; i++) {
        x = BadgeRows[row].x04;
        for (j = 0; j < 2; j++) {
            x += ovl662_extra_record_col_dx[j + 1];
            cur = AddTilePrim(ord, cur, x, y + ovl662_extra_record_row_dy[i], 2, 16, 255, 255, 255);
        }
    }
    return cur;
}

u8 *ovl662_format_record_time(s32 value){
 if(value>=0){
  /* Editing time in frames (50 or 60 a second): minutes, then hours. */
  s32 seconds=value/RR_PAL_NTSC(3000, 3600);
  s32 minutes=seconds/60;
  seconds%=60;
  ovl662_record_time_str_buf[0]=minutes/10+'0';
  ovl662_record_time_str_buf[1]=minutes%10+'0';
  ovl662_record_time_str_buf[3]=seconds/10+'0';
  ovl662_record_time_str_buf[4]=seconds%10+'0';
 }else{
  ovl662_record_time_str_buf[0]='-';ovl662_record_time_str_buf[1]='-';
  ovl662_record_time_str_buf[3]='-';ovl662_record_time_str_buf[4]='-';
 }
 return ovl662_record_time_str_buf;
}

/* Initial values of the overlay globals this module owns. */
s8 ovl662_stage_history_glyphs[3] = {64, 25, 123};
s8 ovl662_team_count_glyphs[7] = {23, 24, -1, 31, 25, 24, -2};
s8 ovl662_maker_count_glyphs[3] = {23, 24, -1};
s8 ovl662_car_number_glyphs[3] = {26, 25, 123};
s8 ovl662_emblem_index_glyphs[4] = {42, 25, 123, -1};
s8 ovl662_char_grid_cmd_glyphs[3] = {0, 1, 2};
u8 ovl662_extra_record_col_dx[4] = {8, 0x1C, 0x38, 0x30};
u8 ovl662_extra_record_row_dy[4] = {8, 0x30, 0x48, 0x60};
u8 ovl662_extra_record_label_glyphs[3] = {0x5D, 0x2A, 0x2B};
u8 ovl662_record_time_str_buf[6] = "00:00";

/* Writable state of this unit; ovl662_reset_module restores it. */
static const OverlayStateRegion ovl662_record_labels_regions[] = {
    RR_OVERLAY_STATE(ovl662_car_number_glyphs),
    RR_OVERLAY_STATE(ovl662_char_grid_cmd_glyphs),
    RR_OVERLAY_STATE(ovl662_emblem_index_glyphs),
    RR_OVERLAY_STATE(ovl662_extra_record_col_dx),
    RR_OVERLAY_STATE(ovl662_extra_record_label_glyphs),
    RR_OVERLAY_STATE(ovl662_extra_record_row_dy),
    RR_OVERLAY_STATE(ovl662_maker_count_glyphs),
    RR_OVERLAY_STATE(ovl662_record_time_str_buf),
    RR_OVERLAY_STATE(ovl662_stage_history_glyphs),
    RR_OVERLAY_STATE(ovl662_team_count_glyphs)};
const OverlayStateTable ovl662_record_labels_state = {ovl662_record_labels_regions, sizeof(ovl662_record_labels_regions) / sizeof(ovl662_record_labels_regions[0])};

/* Initial values of the globals this module owns. */
