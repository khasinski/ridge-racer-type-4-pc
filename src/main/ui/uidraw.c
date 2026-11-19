/* RODATA 0x80010884 0x54 */
#include "rr/ps1_sizes.h"
#include "rr/cpu_time.h"
#include "game/ui/hudmap.h"
#include "rr/frame_storage_types.h"
#include "game/gfx/gfxmtx.h"
#include "game/menu/mnrows.h"
#include "game/ui/uidraw.h"
#include "rr/bios_devices.h"
#include "rr/audio_cd_controller.h"
#include "rr/badge_rows.h"
#include "rr/menu_state_types.h"
#include "rr/ui_panel.h"
#include "rr/trig.h"






void UpdateBadgeRowAnim(int index) {
    RR_CPU_CYCLES(RR_CYCLES_UPDATE_BADGE_ROW_ANIM);
    int tick;
    s32 scale;
    if (index < BadgeRowCount) {
        tick = BadgeRows[index].animationTick12;
        switch (BadgeRows[index].state10) {
        case BADGE_ANIM_INITIALIZE:
            BadgeRows[index].h1A = 2;
            BadgeRows[index].state10 = BADGE_ANIM_WAIT_FOR_ENABLE;
            BadgeRows[index].w18 = 0;
            BadgeRows[index].animationTick12 = 0;
            BadgeRows[index].x04 = BadgeRows[index].x08;
            BadgeRows[index].y06 = (BadgeRows[index].y0A + BadgeRows[index].extent0E) - 2;
            BadgeRows[index].x14 = BadgeRows[index].x08;
            BadgeRows[index].y16 = (BadgeRows[index].y0A + BadgeRows[index].extent0E) - 2;
            break;
        case BADGE_ANIM_REVEAL_WIDTH:
            scale = rsin((tick * 1024) / 15);
            BadgeRows[index].w18 = ((BadgeRows[index].width0C + 2) * scale) / 4096;
            if (tick < 0xF) {
                BadgeRows[index].animationTick12++;
            } else {
                BadgeRows[index].state10 = BADGE_ANIM_REVEAL_HEIGHT;
                BadgeRows[index].animationTick12 = 0;
            }
            break;
        case BADGE_ANIM_REVEAL_HEIGHT:
            scale = rsin((tick * 1024) / 15);
            BadgeRows[index].h1A = (BadgeRows[index].extent0E * scale) / 4096;
            BadgeRows[index].y16 = (BadgeRows[index].y0A + BadgeRows[index].extent0E) - BadgeRows[index].h1A;
            BadgeRows[index].y06 = BadgeRows[index].y16;
            if (tick < 0xF) {
                BadgeRows[index].animationTick12++;
            } else {
                BadgeRows[index].state10 = BADGE_ANIM_VISIBLE;
                BadgeRows[index].animationTick12 = 0;
            }
            break;
        case BADGE_ANIM_VISIBLE:
            BadgeRows[index].x04 = BadgeRows[index].x08;
            BadgeRows[index].y06 = BadgeRows[index].y0A;
            BadgeRows[index].x14 = BadgeRows[index].x08;
            BadgeRows[index].y16 = BadgeRows[index].y0A;
            BadgeRows[index].w18 = BadgeRows[index].width0C + 2;
            BadgeRows[index].h1A = BadgeRows[index].extent0E;
            break;
        case BADGE_ANIM_HIDE_HEIGHT:
            scale = rcos((tick * 1024) / 15);
            BadgeRows[index].h1A = (BadgeRows[index].extent0E * scale) / 4096;
            BadgeRows[index].y16 = BadgeRows[index].y0A + (BadgeRows[index].extent0E - BadgeRows[index].h1A);
            BadgeRows[index].y06 = BadgeRows[index].y16;
            if (tick < 0xF) {
                BadgeRows[index].animationTick12++;
            } else {
                BadgeRows[index].state10 = BADGE_ANIM_HIDE_WIDTH;
                BadgeRows[index].animationTick12 = 0;
            }
            break;
        case BADGE_ANIM_HIDE_WIDTH:
            scale = rcos((tick * 1024) / 15);
            BadgeRows[index].w18 = ((BadgeRows[index].width0C + 2) * scale) / 4096;
            if (tick < 0xF) {
                BadgeRows[index].animationTick12++;
            } else {
                BadgeRows[index].state10 = BADGE_ANIM_HIDDEN;
                BadgeRows[index].animationTick12 = 0;
            }
            break;
        case BADGE_ANIM_WAIT_FOR_ENABLE:
            if (BadgeRows[index].half02 != 0) {
                BadgeRows[index].state10 = BADGE_ANIM_REVEAL_WIDTH;
            }
            break;
        case BADGE_ANIM_REVEAL_WIDTH_FULL_HEIGHT:
            scale = rsin((tick * 1024) / 15);
            BadgeRows[index].w18 = ((BadgeRows[index].width0C + 2) * scale) / 4096;
            BadgeRows[index].h1A = BadgeRows[index].extent0E;
            BadgeRows[index].x14 = BadgeRows[index].x08;
            BadgeRows[index].y16 = BadgeRows[index].y0A;
            if (tick < 0xF) {
                BadgeRows[index].animationTick12++;
            } else {
                BadgeRows[index].state10 = BADGE_ANIM_VISIBLE;
                BadgeRows[index].animationTick12 = 0;
            }
            break;
        case 6:
        case 9:
        case BADGE_ANIM_HIDDEN:
            break;
        case BADGE_ANIM_REVEAL_HEIGHT_CENTERED:
            BadgeRows[index].w18 = BadgeRows[index].width0C + 2;
            scale = rsin((tick * 1024) / 15);
            BadgeRows[index].h1A = (BadgeRows[index].extent0E * scale) / 4096;
            BadgeRows[index].x14 = BadgeRows[index].x08;
            BadgeRows[index].y16 = BadgeRows[index].y0A + ((BadgeRows[index].extent0E / 2) - (BadgeRows[index].h1A / 2));
            if (tick < 0xF) {
                BadgeRows[index].animationTick12++;
            } else {
                BadgeRows[index].state10 = BADGE_ANIM_VISIBLE;
                BadgeRows[index].animationTick12 = 0;
            }
            break;
        }
        DrawBadgeRow(index);
    }
}

int AllocRect16Rows(void *a0) {
    int n = Rect16RowCount * (int)sizeof(*Rect16Rows);
    Rect16Rows = a0;
    PsyqBzero(a0, n);
    DebugPrintStub(StrSizeEqLdFmt, n);
    return n;
}

void InitRect16Row(s32 index, s16 x, s16 y, s16 width, s16 height, s16 phase, s16 parameter) {
    if (index < Rect16RowCount) {
        Rect16Rows[index].parameter00 = parameter;
        Rect16Rows[index].restart02 = 0;
        Rect16Rows[index].x = x;
        Rect16Rows[index].y = y;
        Rect16Rows[index].width = width;
        Rect16Rows[index].height = height;
        Rect16Rows[index].phase = phase;
    }
}

int UpdateRect16Row(int index) {
    MenuRectCorners corners;
    int intensity[4];
    int complete;
    int fullIntensity;
    int result;
    int line;
    int phase, tick;
    const MenuRectEdgeCorners *edge;
    const int *x, *y;
    const unsigned char *levels;

    result = index < Rect16RowCount;
    if (result != 0) {
        complete = 0;
        phase = Rect16Rows[index].phase;
        tick = Rect16Rows[index].tick;
        line = 3;
        do {
            intensity[line] = 0;
            line--;
        } while (line >= 0);
        corners.x[0] = Rect16Rows[index].x;
        corners.y[0] = Rect16Rows[index].y;
        corners.x[1] = Rect16Rows[index].x + Rect16Rows[index].width - 1;
        corners.y[1] = Rect16Rows[index].y;
        corners.x[2] = Rect16Rows[index].x;
        corners.y[2] = Rect16Rows[index].y + Rect16Rows[index].height - 1;
        corners.x[3] = Rect16Rows[index].x + Rect16Rows[index].width - 1;
        corners.y[3] = Rect16Rows[index].y + Rect16Rows[index].height - 1;
        switch (phase) {
        case MENU_RECT_INITIALIZE:
            Rect16Rows[index].phase = MENU_RECT_VERTICAL_LINES;
            Rect16Rows[index].tick = 0;
            break;
        case MENU_RECT_COMPLETE:
            complete = 1;
            fullIntensity = 0x100;
            line = 3;
            do {
                intensity[line] = fullIntensity;
                line--;
            } while (line >= 0);
            break;
        case MENU_RECT_SECOND_DIAGONAL:
            intensity[2] = 0x100;
        case MENU_RECT_FIRST_DIAGONAL:
            intensity[1] = 0x100;
        case MENU_RECT_HORIZONTAL_LINES:
            intensity[0] = 0x100;
        case MENU_RECT_VERTICAL_LINES:
            intensity[phase - 1] = (tick * 256) / 6;
            if (tick < 6) {
                Rect16Rows[index].tick = Rect16Rows[index].tick + 1;
            } else {
                Rect16Rows[index].tick = 0;
                Rect16Rows[index].phase = Rect16Rows[index].phase + 1;
            }
            break;
        case MENU_RECT_WAIT_FOR_RESTART:
            if (Rect16Rows[index].restart02 != 0) {
                Rect16Rows[index].phase = MENU_RECT_VERTICAL_LINES;
            }
            break;
        default:
            break;
        }
        line = 0;
        x = corners.x;
        y = corners.y;
        levels = MenuRectLineIntensitySlots;
        edge = MenuRectLineCorners;
    loop:
        AddPartialLine(&CurrentDrawBuf->buffer.ordering70.tags[0],
                       x[edge->first], y[edge->first],
                       x[edge->last], y[edge->last],
                       intensity[levels[line]],
                       1, 8, 8, 8, -1);
        edge++;
        line++;
        if (line < 6) goto loop;
        result = complete;
    }
    return result;
}

int AllocGlyphRows(void *buffer) {
    int bytes = GlyphRowCount * (s32)sizeof(*GlyphRows);
    GlyphRows = buffer;
    PsyqBzeroPs1(buffer, (u32)bytes, GlyphRowCount * RR_PS1_MENU_GLYPH_ROW_BYTES);
    DebugPrintStub(SizeLdFmt, bytes);
    return bytes;
}
void InitGlyphRow(int index, int control, int glyph, int presetIndex) {
    MenuGlyphPreset14 *preset;
    if (index < GlyphRowCount) {
        GlyphRows[index].control02 = control;
        GlyphRows[index].glyph07 = glyph;
        preset = &AnimatedGlyphPresets[presetIndex];
        GlyphRows[index].x08 = preset->x00;
        GlyphRows[index].y0A = preset->y02;
        GlyphRows[index].w0C = preset->w04;
        GlyphRows[index].h0E = preset->h06;
        GlyphRows[index].frameCount01 = preset->frameCount0E;
        GlyphRows[index].frameDuration05 = preset->frameDuration0F;
        GlyphRows[index].imageSectors06 = preset->imageSectors10;
        GlyphRows[index].texturePage10 = preset->page08;
        GlyphRows[index].clut12 = preset->clut0A;
        GlyphRows[index].u14 = preset->u0C;
        GlyphRows[index].v15 = preset->v0D;
    }
}

/* Initial values of the globals this module owns. */
char SizeLdFmt[56] = {115, 105, 122, 101, 32, 61, 32, 37, 108, 100, 10};
MenuRectEdgeCorners MenuRectLineCorners[6] = {{.first = 2}, {.first = 1, .last = 3}, {.last = 1}, {.first = 3, .last = 2}, {.first = 1, .last = 2}, {.last = 3}};
u8 MenuRectLineIntensitySlots[6] = {0, 0, 1, 1, 2, 3};
MenuGlyphPreset14 AnimatedGlyphPresets[2] = {{.x00 = 120, .y02 = 124, .w04 = 80, .h06 = 160, .page08 = 0xBD, .clut0A = 0x3FE8, .u0C = 0x30, .frameCount0E = 3, .frameDuration0F = 0x3C, .imageSectors10 = 8}, {.x00 = 120, .y02 = 124, .w04 = 80, .h06 = 160, .page08 = 0xBD, .clut0A = 0x3FE8, .u0C = 0x30, .frameCount0E = 1, .frameDuration0F = 0x3C, .imageSectors10 = 8}};
int Rect16RowCount;
MenuRectRow10 *Rect16Rows;
BadgeRow1C *BadgeRows;
int GlyphRowCount;
