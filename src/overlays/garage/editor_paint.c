#include "rr/overlay_module.h"
#include "rr/overlay662_pixel_dispatch.h"
#include "rr/overlay662_image_stroke.h"
#include "game/system/pad.h"
#include "rr/overlay662_image_drag.h"
#include "rr/overlay662_color_pick.h"
#include "rr/overlay662_garage.h"
#include "rr/overlay662_image_scroll.h"
#include "rr/overlay662_pixels.h"
#include "rr/overlay662_tim.h"

void PlaySfx(s32 mode);

void ovl662_dispatch_canvas_tool(s32 actionCode)
{
    s32 i;
    u32 *clear;

    ovl662_update_canvas_hover();
    clear = ovl662_editor_workspace.scratch34;
    for (i = 511; i >= 0; i--) {
        *clear++ = 0;
    }
    if (ovl662_editor_workspace.config20 != 2) {
        switch (ovl662_editor_workspace.config1E) {
        case 0: ovl662_apply_pen_stroke(); break;
        case 1: ovl662_apply_line_stroke(); break;
        case 2: ovl662_apply_flood_fill(); break;
        case 3: ovl662_apply_primary_pen_stroke(); break;
        case 4: ovl662_apply_move_drag(); break;
        }
    }
    if (ovl662_editor_workspace.config20 != 1) {
        switch (ovl662_editor_workspace.config1F) {
        case 0: ovl662_apply_color_picker(); break;
        case 1: ovl662_apply_eraser_stroke(); break;
        case 2: ovl662_toggle_preview_side(); break;
        }
    }
}

void ovl662_apply_pen_stroke(void) {
    u32 *img;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_workspace.config20 = 0;
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(82, 71, 128, 256) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_workspace.config20 = 1;
        ovl662_editor_cursor_state.words[4] = 52;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_clip_rect(82, 71, 128, 256);
        ovl662_backup_layer_and_palette();
        ovl662_pen_stroke_prev_pt.x = ovl662_editor_workspace.reset08[0] + ovl662_editor_workspace.reset08[2];
        ovl662_pen_stroke_prev_pt.y = ovl662_editor_workspace.reset08[1] + ovl662_editor_workspace.reset08[3];
    }
    if (ovl662_editor_workspace.layer23 == 0) {
        img = ovl662_editor_workspace.image38[ovl662_editor_workspace.active21];
    } else {
        img = ovl662_editor_workspace.image40[ovl662_editor_workspace.active21];
    }
    ovl662_pen_stroke_curr_pt.x = ovl662_editor_workspace.reset08[0] + ovl662_editor_workspace.reset08[2];
    ovl662_pen_stroke_curr_pt.y = ovl662_editor_workspace.reset08[1] + ovl662_editor_workspace.reset08[3];
    ovl662_rasterize_brush_line(img, ovl662_pen_stroke_prev_pt.x, ovl662_pen_stroke_prev_pt.y, ovl662_pen_stroke_curr_pt.x, ovl662_pen_stroke_curr_pt.y, ovl662_editor_workspace.config22, ovl662_editor_workspace.config1D);
    ovl662_pen_stroke_prev_pt.x = ovl662_pen_stroke_curr_pt.x;
    ovl662_pen_stroke_prev_pt.y = ovl662_pen_stroke_curr_pt.y;
}

void ovl662_apply_line_stroke(void) {
    u32 *img;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_workspace.config20 = 0;
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            if (ovl662_editor_workspace.layer23 == 0) {
                img = ovl662_editor_workspace.image38[ovl662_editor_workspace.active21];
            } else {
                img = ovl662_editor_workspace.image40[ovl662_editor_workspace.active21];
            }
        } else {
            img = ovl662_editor_workspace.scratch34;
        }
    } else {
        if (ovl662_point_outside_rect(82, 71, 128, 256) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_workspace.config20 = 1;
        ovl662_editor_cursor_state.words[4] = 52;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_clip_rect(82, 71, 128, 256);
        ovl662_backup_layer_and_palette();
        img = ovl662_editor_workspace.scratch34;
        ovl662_line_stroke_start_pt.x = ovl662_editor_workspace.reset08[0] + ovl662_editor_workspace.reset08[2];
        ovl662_line_stroke_start_pt.y = ovl662_editor_workspace.reset08[1] + ovl662_editor_workspace.reset08[3];
    }
    ovl662_rasterize_brush_line(
        img,
        ovl662_line_stroke_start_pt.x,
        ovl662_line_stroke_start_pt.y,
        ovl662_editor_workspace.reset08[0] + ovl662_editor_workspace.reset08[2],
        ovl662_editor_workspace.reset08[1] + ovl662_editor_workspace.reset08[3],
        ovl662_editor_workspace.config22,
        ovl662_editor_workspace.config1D);
}

/* Four-way fill of a 64x64 4bpp image. Mark pixels when queued, so each
 * pixel occupies the work list at most once. The original recursive fill
 * allowed 8188 levels; a valid image has only 4096 pixels, so an explicit
 * 8 KiB work list covers every fill without consuming the host call stack. */
void FloodFill4bpp(u8 *pixel, u32 operation)
{
    const unsigned x = operation & 0xFF, y = (operation >> 8) & 0xFF;
    const unsigned previous = (operation >> 16) & 0xFF;
    const unsigned replacement = operation >> 24;
    if (!pixel || x >= 64 || y >= 64 || previous > 15 ||
        replacement > 15 || previous == replacement)
        return;
    u8 *image = pixel - (y * 32 + x / 2);
    u16 pending[64 * 64];
    unsigned count = 0;
    const unsigned first = y * 64 + x;
    const unsigned shift = (x & 1) * 4;
    if (((*pixel >> shift) & 15) != previous)
        return;
    *pixel = (u8)((*pixel & ~(15u << shift)) | (replacement << shift));
    pending[count++] = (u16)first;
    while (count) {
        const unsigned at = pending[--count];
        const unsigned px = at % 64, py = at / 64;
        unsigned neighbours[4], n = 0;
        if (py > 0) neighbours[n++] = at - 64;
        if (py < 63) neighbours[n++] = at + 64;
        if (px > 0) neighbours[n++] = at - 1;
        if (px < 63) neighbours[n++] = at + 1;
        for (unsigned i = 0; i < n; ++i) {
            const unsigned next = neighbours[i], bits = (next & 1) * 4;
            u8 *dest = &image[next / 2];
            if (((*dest >> bits) & 15) == previous) {
                *dest = (u8)((*dest & ~(15u << bits)) | (replacement << bits));
                pending[count++] = (u16)next;
            }
        }
    }
}

void ovl662_apply_flood_fill(void)
{
    GaragePixelEdit4 operation;
    u8 *pixel;
    GarageSelectionPrefix *state;
    u8 x0;
    u8 dx0;
    u8 y0;
    u8 dy0;
    s32 x;
    u32 y;
    u32 offset;
    s32 dx;
    u32 packed;

    if (ovl662_point_outside_rect(82, 71, 128, 256) == 0 &&
        (ovl662_editor_cursor_state.words[1] & 1)) {
        if (ovl662_editor_workspace.layer23 == 0) {
            pixel = (u8 *)ovl662_editor_workspace.image38[ovl662_editor_workspace.active21];
        } else {
            pixel = (u8 *)ovl662_editor_workspace.image40[ovl662_editor_workspace.active21];
        }
        state = &ovl662_editor_workspace;
        x0 = state->reset08[0];
        x = x0;
        dx0 = state->reset08[6];
        dx = dx0;
        y0 = state->reset08[1];
        dy0 = state->reset08[7];
        x += dx;
        y = y0 + dy0;
        operation.y = y;
        offset = y & 0xFF;
        operation.x = x;
        y = x & 0xFF;
        offset <<= 5;
        x = 0;
        packed = y >> 1;
        offset += packed;
        pixel += offset;
        y &= 1;
        if (y) {
            operation.previous = *pixel >> 4;
        } else {
            operation.previous = *pixel & 15;
        }
        if (PlayerControllers[0].input00.fields.type != 0x12 &&
            (PlayerControllers[0].input00.fields.buttons & 2) && (PlayerControllers[0].input00.fields.buttons & 1)) {
            operation.replacement = 0;
        } else {
            operation.replacement = ovl662_editor_workspace.config22;
        }
        if (operation.previous != operation.replacement) {
            ovl662_backup_layer_and_palette();
            packed = operation.y;
            dx = operation.x;
            packed = dx | (packed << 8);
            packed |= operation.previous << 16;
            FloodFill4bpp(pixel, packed | ((u32)operation.replacement << 24));
        }
    }
}

void ovl662_apply_primary_pen_stroke(void) {
    u32 *img;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_workspace.config20 = 0;
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(82, 71, 128, 256) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_workspace.config20 = 1;
        ovl662_editor_cursor_state.words[4] = 52;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_clip_rect(82, 71, 128, 256);
        ovl662_backup_layer_and_palette();
        ovl662_prim_pen_prev_pt.x = ovl662_editor_workspace.reset08[0] + ovl662_editor_workspace.reset08[2];
        ovl662_prim_pen_prev_pt.y = ovl662_editor_workspace.reset08[1] + ovl662_editor_workspace.reset08[3];
    }
    if (ovl662_editor_workspace.layer23 == 0) {
        img = ovl662_editor_workspace.image38[ovl662_editor_workspace.active21];
    } else {
        img = ovl662_editor_workspace.image40[ovl662_editor_workspace.active21];
    }
    ovl662_prim_pen_curr_pt.x = ovl662_editor_workspace.reset08[0] + ovl662_editor_workspace.reset08[2];
    ovl662_prim_pen_curr_pt.y = ovl662_editor_workspace.reset08[1] + ovl662_editor_workspace.reset08[3];
    ovl662_rasterize_brush_line(img, ovl662_prim_pen_prev_pt.x, ovl662_prim_pen_prev_pt.y, ovl662_prim_pen_curr_pt.x, ovl662_prim_pen_curr_pt.y, 0, ovl662_editor_workspace.config1D);
    ovl662_prim_pen_prev_pt.x = ovl662_prim_pen_curr_pt.x;
    ovl662_prim_pen_prev_pt.y = ovl662_prim_pen_curr_pt.y;
}

void ovl662_apply_move_drag(void) {
    u32 *src;
    u32 *dst;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 4) {
            ovl662_editor_workspace.config20 = 0;
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_editor_cursor_state.words[14] = 0;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(82, 71, 128, 256) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 1)) {
            return;
        }
        ovl662_editor_workspace.config20 = 1;
        ovl662_editor_cursor_state.words[4] = 52;
        ovl662_editor_cursor_state.words[14] = 1;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_clip_rect(82, 71, 128, 256);
        ovl662_backup_layer_and_palette();
        ovl662_move_drag_start_pt.x = ovl662_editor_workspace.reset08[0] + ovl662_editor_workspace.reset08[6];
        ovl662_move_drag_start_pt.y = ovl662_editor_workspace.reset08[1] + ovl662_editor_workspace.reset08[7];
    }
    if (ovl662_editor_workspace.layer23 == 0) {
        src = ovl662_editor_workspace.image38[!ovl662_editor_workspace.active21];
        dst = ovl662_editor_workspace.image38[ovl662_editor_workspace.active21];
    } else {
        src = ovl662_editor_workspace.image40[!ovl662_editor_workspace.active21];
        dst = ovl662_editor_workspace.image40[ovl662_editor_workspace.active21];
    }
    ovl662_scroll_layer_image(
        src,
        dst,
        ovl662_editor_workspace.reset08[0] + ovl662_editor_workspace.reset08[6] - ovl662_move_drag_start_pt.x,
        ovl662_editor_workspace.reset08[1] + ovl662_editor_workspace.reset08[7] - ovl662_move_drag_start_pt.y);
}

void ovl662_apply_color_picker(void) {
    s32 x, y;
    u8 *img0, *img1;
    u8 c;

    if (ovl662_point_outside_rect(82, 71, 128, 256) != 0) {
        return;
    }
    if (!(ovl662_editor_cursor_state.words[1] & 2)) {
        return;
    }
    img0 = (u8 *)(GarageImage512 *)ovl662_editor_workspace.image38[ovl662_editor_workspace.active21];
    img1 = (u8 *)(GarageImage512 *)ovl662_editor_workspace.image40[ovl662_editor_workspace.active21];
    x = ovl662_editor_workspace.reset08[0] + (ovl662_editor_cursor_state.halves[10] - 82) / ovl662_editor_workspace.config1C;
    y = ovl662_editor_workspace.reset08[1] + ((ovl662_editor_cursor_state.halves[11] - 71) / ovl662_editor_workspace.config1C) / 2;
    c = (img0 + y * 32)[x / 2];
    if (x & 1) {
        c >>= 4;
    } else {
        c &= 0xF;
    }
    if (c != 0) {
        ovl662_editor_workspace.config22 = c;
        PlaySfx(1);
    } else {
        c = (img1 + y * 32)[x / 2];
        if (x & 1) {
            c >>= 4;
        } else {
            c &= 0xF;
        }
        if (c != 0) {
            ovl662_editor_workspace.config22 = c;
            PlaySfx(1);
        } else {
            PlaySfx(4);
        }
    }
}

void ovl662_apply_eraser_stroke(void) {
    u32 *img;

    if (ovl662_editor_cursor_state.words[0] & 8) {
        if (ovl662_editor_cursor_state.words[1] & 8) {
            ovl662_editor_workspace.config20 = 0;
            ovl662_editor_cursor_state.words[0] &= ~8;
            ovl662_set_clip_rect(13, 63, 294, 346);
            return;
        }
    } else {
        if (ovl662_point_outside_rect(82, 71, 128, 256) != 0) {
            return;
        }
        if (!(ovl662_editor_cursor_state.words[1] & 2)) {
            return;
        }
        ovl662_editor_workspace.config20 = 2;
        ovl662_editor_cursor_state.words[4] = 52;
        ovl662_editor_cursor_state.words[0] |= 8;
        ovl662_set_clip_rect(82, 71, 128, 256);
        ovl662_backup_layer_and_palette();
        ovl662_eraser_stroke_prev_pt.x = ovl662_editor_workspace.reset08[0] + ovl662_editor_workspace.reset08[2];
        ovl662_eraser_stroke_prev_pt.y = ovl662_editor_workspace.reset08[1] + ovl662_editor_workspace.reset08[3];
    }
    if (ovl662_editor_workspace.layer23 == 0) {
        img = ovl662_editor_workspace.image38[ovl662_editor_workspace.active21];
    } else {
        img = ovl662_editor_workspace.image40[ovl662_editor_workspace.active21];
    }
    ovl662_eraser_stroke_curr_pt.x = ovl662_editor_workspace.reset08[0] + ovl662_editor_workspace.reset08[2];
    ovl662_eraser_stroke_curr_pt.y = ovl662_editor_workspace.reset08[1] + ovl662_editor_workspace.reset08[3];
    ovl662_rasterize_brush_line(img, ovl662_eraser_stroke_prev_pt.x, ovl662_eraser_stroke_prev_pt.y, ovl662_eraser_stroke_curr_pt.x, ovl662_eraser_stroke_curr_pt.y, 0, ovl662_editor_workspace.config1D);
    ovl662_eraser_stroke_prev_pt.x = ovl662_eraser_stroke_curr_pt.x;
    ovl662_eraser_stroke_prev_pt.y = ovl662_eraser_stroke_curr_pt.y;
}

void ovl662_toggle_preview_side(void)
{
    if(ovl662_editor_cursor_state.words[1]&2){
        PlaySfx(1);
        ovl662_editor_workspace.active21=!ovl662_editor_workspace.active21;
    }
}

/* Map the cursor inside the 128x256 image area (at 82,71) to image cells:
 * reset08[2..5] is the zoom window origin/extent centred on the cursor,
 * reset08[6..9] the single cell under the cursor. Both are clamped to the
 * 128/config1C grid. Outside the area everything is cleared. */
void ovl662_update_canvas_hover(void) {
#define SEL ovl662_editor_workspace
#define CUR ovl662_editor_cursor_state.points.cursor14
    GaragePoint p;
    s32 size, max, zoom, x, y;

    if (ovl662_point_outside_rect(82, 71, 128, 256) != 0) {
        SEL.reset08[2] = 0;
        SEL.reset08[3] = 0;
        SEL.reset08[4] = 0;
        SEL.reset08[5] = 0;
        SEL.reset08[6] = 0;
        SEL.reset08[7] = 0;
        SEL.reset08[8] = 0;
        SEL.reset08[9] = 0;
        return;
    }
    size = SEL.config1C;
    zoom = SEL.config1D;
    SEL.reset08[4] = SEL.reset08[5] = zoom;
    p.x = CUR.x - 82;
    p.y = (CUR.y - 71) >> 1;
    x = (p.x - (size * SEL.config1D - size) / 2) / size;
    max = 128 / SEL.config1C - zoom;
    y = (p.y - (size * SEL.config1D - size) / 2) / size;
    SEL.reset08[2] = x;
    SEL.reset08[3] = y;
    if (SEL.reset08[2] < 0) {
        SEL.reset08[2] = 0;
    } else if (max < SEL.reset08[2]) {
        SEL.reset08[2] = max;
    }
    if (SEL.reset08[3] < 0) {
        SEL.reset08[3] = 0;
    } else if (max < SEL.reset08[3]) {
        SEL.reset08[3] = max;
    }

    size = SEL.config1C;
    p.x = CUR.x - 82;
    p.y = (CUR.y - 71) >> 1;
    SEL.reset08[6] = p.x / size;
    SEL.reset08[7] = p.y / size;
    SEL.reset08[8] = SEL.reset08[9] = 1;
    max = 128 / SEL.config1C - 1;
    if (SEL.reset08[6] < 0) {
        SEL.reset08[6] = 0;
    } else if (max < SEL.reset08[6]) {
        SEL.reset08[6] = max;
    }
    if (SEL.reset08[7] < 0) {
        SEL.reset08[7] = 0;
    } else if (max < SEL.reset08[7]) {
        SEL.reset08[7] = max;
    }
#undef CUR
#undef SEL
}

/* Bresenham line from (x0, y0) to (x1, y1) with a square brush. */
void ovl662_rasterize_brush_line(u32 *image, s32 x0, s32 y0, s32 x1, s32 y1, u8 color, u8 size) {
    /* Address of the byte holding 4bpp pixel (x, y); rows are 32 bytes. */
#define PIXEL_BYTE(x, y) (&((u8 *)image + (y) * 32)[(x) / 2])
    /* Paint a size x size block of the color nibble at (x, y). Each instance
     * owns its block-local temporaries. */
#define PLOT_BLOCK(x, y)                                              \
    for (i = 0; i < size; i++) {                                      \
        for (j = 0; j < size; j++) {                                  \
            u8 *p = PIXEL_BYTE((x) + j, (y) + i);                     \
            u8 v = *p;                                                \
            u8 *q;                                                    \
            u8 w;                                                     \
            *p = (((x) + j) & 1) ? (v & 0x0F) : (v & 0xF0);           \
            q = PIXEL_BYTE((x) + j, (y) + i);                         \
            w = *q;                                                   \
            *q = w | ((((x) + j) & 1) ? (color << 4) : color);        \
        }                                                             \
    }
    s32 dx, dy, step, err;
    s32 i, j;

    if (x0 < x1) {
        dx = x1 - x0;
    } else {
        dx = x0 - x1;
    }
    if (y0 < y1) {
        dy = y1 - y0;
    } else {
        dy = y0 - y1;
    }
    if (dy < dx) {
        if (x1 < x0) {
            if (y1 < y0) {
                step = 1;
            } else {
                step = -1;
            }
            err = x0;
            x0 = x1;
            x1 = err;
            y0 = y1;
        } else {
            if (y0 < y1) {
                step = 1;
            } else {
                step = -1;
            }
        }
        PLOT_BLOCK(x0, y0);
        err = dx >> 1;
        for (x0++; x0 <= x1; x0++) {
            err -= dy;
            if (err < 0) {
                err += dx;
                y0 += step;
            }
            PLOT_BLOCK(x0, y0);
        }
    } else {
        if (y1 < y0) {
            if (x1 < x0) {
                step = 1;
            } else {
                step = -1;
            }
            err = y0;
            y0 = y1;
            y1 = err;
            x0 = x1;
        } else {
            if (x0 < x1) {
                step = 1;
            } else {
                step = -1;
            }
        }
        PLOT_BLOCK(x0, y0);
        err = dy >> 1;
        for (y0++; y0 <= y1; y0++) {
            err -= dx;
            if (err < 0) {
                err += dy;
                x0 += step;
            }
            PLOT_BLOCK(x0, y0);
        }
    }
#undef PLOT_BLOCK
#undef PIXEL_BYTE
}

void ovl662_scroll_layer_image(u32 *source,u32 *destination,s32 x,s32 y)
{
    u32 buffer[512];
    struct {s16 column,row;} cursor;
    s32 i,j,shift,left;
    u32 first,second;
    {
        u32 *input=source;
        u32 *temporary=buffer;
        intptr_t t1, t2;
        if(y>=0)cursor.row=y*8;
        else cursor.row=(y+64)*8;
        for(i=0;i<64;i++) {
            if(cursor.row>=505)cursor.row=0;
            for(j=0;j<8;) {
                u32 bytes=j++*4;
                s32 at=cursor.row;
                t1 = j | 1;
                t1 &= 1;
                temporary += t1 - 1;
                t2 = j | 1;
                t2 &= 1;
                temporary += t2 - 1;
                t1 = j | 1;
                t1 &= 1;
                input += t1 - 1;
                (temporary+at)[bytes/4]=*input++;
            }
            cursor.row+=8;
        }
    }
    {
        intptr_t t1, t2;
        union {
            u32 *ptr;
            intptr_t addr;
        } dest, out;
        u32 *temporary=buffer;
        u32 *output;
        dest.ptr = destination;
        out.addr = dest.addr | 2;
        out.addr &= dest.addr;
        output = out.ptr;
        x=-x;
        if(x<0)x+=64;
        t1 = dest.addr | 1;
        t1 &= dest.addr;
        t1 ^= dest.addr;
        x |= t1;
        shift=(x%8)*4;
        left=32-shift;
        for(i=0;i<64;i++) {
            cursor.column=x/8;
            first=(temporary+cursor.column)[i*8];
            t1 = (i * 32) | 1;
            t1 &= (i * 32);
            t1 ^= (i * 32);
            first |= t1;
            for(j=0;j<8;j++) {
                cursor.column++;
                if(cursor.column>=8)cursor.column=0;
                second=first;
                t1 = j | 1;
                t1 &= 1;
                temporary += t1 - 1;
                first=(temporary+cursor.column)[i*8];
                if(left==32)*output=second;
                else *output=(second>>shift)|(first<<left);
                output++;
            }
        }
    }
}

void ovl662_composite_emblem_layers(const u32 *source,u32 *destination)
{
    s32 i;
    for(i=0;i<512;i++){
        u32 value=*source;
        u32 mask=0xffffffffU;
        if(value&0xf0000000U)mask=0x0fffffffU;
        if(value&0x0f000000U)mask&=0xf0ffffffU;
        if(value&0x00f00000U)mask&=0xff0fffffU;
        if(value&0x000f0000U)mask&=0xfff0ffffU;
        if(value&0x0000f000U)mask&=0xffff0fffU;
        if(value&0x00000f00U)mask&=0xfffff0ffU;
        if(value&0x000000f0U)mask&=0xffffff0fU;
        if(value&0x0000000fU)mask&=0xfffffff0U;
        *destination &= mask;
        *destination |= *source;
        source++;destination++;
    }
}

void ovl662_pack_emblem_tims(void) {
    u32 *p;
    GarageTimBytes *src2C = ovl662_editor_workspace.tim2C[ovl662_editor_workspace.active21];
    u32 *img38 = ovl662_editor_workspace.image38[ovl662_editor_workspace.active21];
    PaletteBytes32 *pal48 = (PaletteBytes32 *)(Palette16 *)ovl662_editor_workspace.palette48[ovl662_editor_workspace.active21];

    p = CustomEmblemBuffers.images[0].words;
    *(GarageTimBytes *)p = *src2C;
    *p++ = 0x10;
    *p++ = 8;
    *p++ = 44;
    *p++ = 0x00400280;
    *p++ = 0x00010010;
    *(PaletteBytes32 *)p = *pal48;
    p += 8;
    *p++ = 0x80C;
    *p++ = 0x280;
    *p++ = 0x00400010;
    ovl662_composite_emblem_layers(img38, p);
}

/* Initial values of the overlay globals this module owns. */
GaragePoint ovl662_pen_stroke_prev_pt;
GaragePoint ovl662_pen_stroke_curr_pt;
GaragePoint ovl662_line_stroke_start_pt;
GaragePoint ovl662_prim_pen_prev_pt;
GaragePoint ovl662_prim_pen_curr_pt;
GaragePoint ovl662_move_drag_start_pt;
GaragePoint ovl662_eraser_stroke_prev_pt;
GaragePoint ovl662_eraser_stroke_curr_pt;

/* Writable state of this unit; ovl662_reset_module restores it. */
static const OverlayStateRegion ovl662_editor_paint_regions[] = {
    RR_OVERLAY_STATE(ovl662_eraser_stroke_curr_pt),
    RR_OVERLAY_STATE(ovl662_eraser_stroke_prev_pt),
    RR_OVERLAY_STATE(ovl662_line_stroke_start_pt),
    RR_OVERLAY_STATE(ovl662_move_drag_start_pt),
    RR_OVERLAY_STATE(ovl662_pen_stroke_curr_pt),
    RR_OVERLAY_STATE(ovl662_pen_stroke_prev_pt),
    RR_OVERLAY_STATE(ovl662_prim_pen_curr_pt),
    RR_OVERLAY_STATE(ovl662_prim_pen_prev_pt)};
const OverlayStateTable ovl662_editor_paint_state = {ovl662_editor_paint_regions, sizeof(ovl662_editor_paint_regions) / sizeof(ovl662_editor_paint_regions[0])};
