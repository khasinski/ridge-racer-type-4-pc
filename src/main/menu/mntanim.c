/* RODATA 0x80010ea0 0x2c */
#include "scratchpad.h"
#include "rr/gpu.h"

#include "rr/menu_text_rows.h"
#include "rr/frame_storage_types.h"
#include "rr/gpu_scratch_types.h"
#include "game/menu/mntrow.h"
#include "rr/menu_overlay_runtime.h"
#include "game/ui/badge1.h"

#include "rr/animated_glyph.h"

#define RR_TEXT_GPU_SCRATCH RR_GPU_SCRATCH


void UpdateMenuTextRow(s32 row) {
    s32 x[4];
    s32 y[4];
    s32 u[4];
    s32 v[4];
    s32 tick;
    s32 mode;
    s32 brightness;
    s32 glyph;
    SpritePacket *cursor;
    SpritePacket *sprite;

    if (row < MenuTextRowCount) {
        mode = MenuTextRows[row].rendering.phase;
        tick = MenuTextRows[row].rendering.tick;
        x[0] = MenuTextRows[row].rendering.x;
        y[0] = MenuTextRows[row].rendering.y;
        switch (mode) {
        case -1:
            break;
        case 0:
            MenuTextRows[row].rendering.tick = 0;
            MenuTextRows[row].rendering.phase = 1;
            break;
        case 1:
        case 8:
            u[0] = u[2] = 0xF4;
            u[1] = u[3] = 0xFF;
            v[0] = v[1] = 0x68;
            v[2] = v[3] = 0x7F;
            y[2] = y[0] + 0x18;
            if (mode == 1) {
                brightness = rsin((tick << 10) / 30);
            } else {
                brightness = rcos((tick << 10) / 30);
            }
            brightness /= 32;
            cursor = (SpritePacket *)RR_TEXT_GPU_SCRATCH->cursor;
            sprite = cursor;
            for (glyph = 0; glyph < 0x16; sprite++) {
                SetSprt(cursor);
                cursor++;
                sprite->x = x[0];
                sprite->y = y[0];
                sprite->w = 0xC;
                sprite->h = 0x18;
                sprite->u = 0xF4;
                sprite->v = 0x68;
                sprite->r = brightness;
                sprite->g = brightness;
                sprite->b = brightness;
                sprite->clut = 0x2678;
                AddPrim(&CurrentDrawBuf->ordering.ordering70.tags[0], sprite);
                glyph++;
                x[0] += 0xD;
            }
            RR_TEXT_GPU_SCRATCH->cursor = AddDrawModePrim(&CurrentDrawBuf->ordering.ordering70.tags[0], (GpuDrawModePacket *)cursor, 0x2E);
            if (tick >= 0x1E) {
                MenuTextRows[row].rendering.phase++;
                MenuTextRows[row].rendering.tick = 0;
            } else {
                MenuTextRows[row].rendering.tick++;
            }
            break;
        case 2:
            MenuTextRows[row].rendering.visible = 0;
            for (glyph = 0; glyph < 0x16; glyph++) {
                MenuTextRows[row].rendering.revealed[glyph] = 0;
            }
            if ((u16)(MenuTextRows[row].rendering.flags - 1) < 0xFF) {
                MenuTextRows[row].rendering.phase = 4;
            } else if ((s16)MenuTextRows[row].rendering.flags >= 0) {
                MenuTextRows[row].rendering.phase = 3;
            } else {
                MenuTextRows[row].rendering.phase = -1;
            }
        case 3:
            if (tick >= 2) {
                if (MenuTextRows[row].rendering.visible < 0x16) {
                    MenuTextRows[row].rendering.visible++;
                    MenuTextRows[row].rendering.revealed[MenuTextRows[row].rendering.visible] = 0;
                    MenuTextRows[row].rendering.tick = 0;
                } else if (tick >= 0x40) {
                    MenuTextRows[row].rendering.tick = 0;
                } else {
                    MenuTextRows[row].rendering.tick++;
                }
            } else {
                MenuTextRows[row].rendering.tick++;
            }
            DrawMenuTextRow(row);
            break;
        case 4:
            MenuTextRows[row].rendering.visible = 0x16;
            DrawMenuTextRow(row);
            break;
        case 6:
            MenuTextRows[row].rendering.visible = 1;
            DrawMenuTextRow(row);
            break;
        case 9:
            break;
        }
    }
}
