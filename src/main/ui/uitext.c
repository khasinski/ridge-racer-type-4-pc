#include "rr/release.h"
#include "game/gfx/sprefct.h"
#include "rr/save_progress.h"
#include "rr/vehicle_table.h"
#include "rr/menu_selection.h"
#include "game/gfx/fontdraw.h"
#include "rr/sprite_emit.h"
#include "rr/badge_rows.h"
#include "rr/gpu_scratch_types.h"
/* RODATA 0x80010938 0x24c */
#include "types.h"
#include "rr/frame_storage_types.h"
#include "scratchpad.h"
#include "game/ui/hudmap.h"
#include "game/ui/uiwin.h"
#include "psyq/libgpu/addprim.h"
#include "game/menu/mnrows.h"
#include "game/ui/uitext.h"
#include "rr/ui.h"
#include "rr/ui_bar.h"
#include "rr/bios_devices.h"
#include "rr/trig.h"
#include "game/ui/badgespr.h"
#include "game/ui/uidraw.h"
#include "rr/overlay665_audio_options.h"
static const s8 SaveSlotStageTrophyMapJp[26][4];

typedef struct { unsigned short s[6]; unsigned char b0, b1, reserved[2]; } ImagePreset10;

extern const char StrDigitOne[];
extern const char StrDigitTwo[];
extern const char StrPleaseWait[];
extern s8 SaveSlotStageTrophyMap[][4];
extern s8 UiScrollStepOffsets[][8];
extern UiUvRect UiCursorArrowUvs[];
extern s8 UiTimeTextScratch[];
extern s16 ComDifficultyLevel;
extern s16 ReverseCourse;
extern s16 SelectedMakerIndex;
extern ImagePreset10 PreviewImageRowPresets[];


s32 DrawUiRow(s32 arg0) {
    RectS16 sp28;
    s32 sp30;
    s32 sp34;
    s32 sp38;
    s32 s0;
    s32 s1;
    s32 s2;
    void *s3;
    void *s3_init;
    s32 s4;
    s32 s5_1;
    s32 s5_2;
    s32 s6;
    s32 s7;
    u16 fp;
    u32 a0_byte;
    GpuDmaTag *a0_arg;
    const UiUvRect *glyph;

    fp = 0;
    s3_init = RR_GPU_SCRATCH->cursor;
    sp38 = UiRows[arg0].half04;
    s2 = 0;
    sp30 = UiRows[arg0].byte14;
    s4 = sp38;
    a0_byte = UiRows[arg0].reserved00;
    s7 = UiRows[arg0].half06;
    s6 = UiRows[arg0].byte15;
    sp34 = UiRows[arg0].byte16;

    switch (sp30) {
    case 0:
        UiRows[arg0].byte14 = 1;
        UiRows[arg0].rect18.w = 0;
        UiRows[arg0].rect18.x = UiRows[arg0].half04;
        UiRows[arg0].rect18.y = UiRows[arg0].half06;
        UiRows[arg0].rect18.h = UiRows[arg0].half0A;
        UiRows[arg0].reserved00 = 0;
        UiRows[arg0].cells[0] = 0;
        break;
    case 1:
        UiRows[arg0].rect18.w = (rsin((u16)a0_byte << 6) * UiRows[arg0].half08) / 4096;
        if ((u16)a0_byte >= 16) {
            UiRows[arg0].reserved00 = 0;
            UiRows[arg0].byte14 = 2;
        } else {
            UiRows[arg0].reserved00++;
        }
        break;
    case 2:
        if (UiRows[arg0].byte02 == 1) {
            if (((s8 *)UiRows[arg0].cells)[0] & 2) {
                s2 = rcos((UiRows[arg0].byte17 << 4) - 0x2AA);
                fp = (u32)s2 >> 31;
                if (s2 >= 0) {
                    s2 = ((s2 >> 6) + 100) << 1;
                } else {
                    s2 = 0;
                }
            }
            if (UiRows[arg0].half20 == 1) {
                fp = 2;
            }
        }
        break;
    case 3:
        UiRows[arg0].rect18.w = (rcos((u16)a0_byte << 6) * UiRows[arg0].half08) / 4096;
        if ((u16)a0_byte >= 16) {
            UiRows[arg0].reserved00 = 0;
            UiRows[arg0].byte14 = 4;
        } else {
            UiRows[arg0].reserved00++;
        }
        break;
    case 4:
    case 7:
        if (a0_byte >= 2) {
            UiRows[arg0].reserved00 = 0;
            if (sp30 == 7) {
                UiRows[arg0].byte14 = 8;
            } else if (UiRows[arg0].byte01 == 0) {
                UiRows[arg0].byte14 = 5;
            } else {
                UiRows[arg0].byte14 = 6;
            }
        } else {
            UiRows[arg0].reserved00++;
        }
        break;
    case 5:
    case 6:
        break;
    case 8:
        if (a0_byte >= 16) {
            UiRows[arg0].reserved00 = 0;
            UiRows[arg0].byte14 = 9;
        } else {
            UiRows[arg0].reserved00++;
        }
        /* fallthrough */
    case 9:
        if (UiRows[arg0].byte02 == 1) {
            fp = (((s8 *)UiRows[arg0].cells)[0] != 1) << 1;
        }
        break;
    }

    sp28 = UiRows[arg0].rect18;
    s3 = AddDrawAreaPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3_init, 0, 0, 0x140, RR_PAL_JP(0x200, 0x1E0));
    s4 += 7;

    switch (s6) {
    case 5:
    case 6:
    case 7:
        if (s6 == 5) {
            s0 = NumLaps;
        }
        if (s6 == 6) {
            s0 = NumCars - 2;
        }
        fp = 0;
        if (s6 == 7) {
            s0 = ComDifficultyLevel + 1;
        }
        s2 = 0;
        /* fallthrough */
    case 0:
        {
            const UiUvRect *base_0 = UiFontGlyphs;
            s3 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4, s7 + 8, &base_0[sp34], fp | 0x7FF8, s2);
            if (s6 != 0) {
                for (s1 = 0; s1 < 2; s1++) {
                    s3 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4 + 0x1E + s1 * 0x16, s7 + 8, &SaveSlotTrophyUvs[s1], 0x7FF8, 0);
                    if (s1 == 0) {
                        UiTimeTextScratch[0] = s0 / 10;
                    } else {
                        UiTimeTextScratch[0] = (s0 % 10) * 8;
                    }
                    /* UiTimeTextScratch is a UV record whose U selects the digit. */
                    s3 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4 + 0x26 + s1 * 6, s7 + 8, (const UiUvRect *)UiTimeTextScratch, 0x7FF8, 0);
                }
            }
        }
        break;
    case 1:
    case 4:
    case 8:
    case 9:
        for (s1 = 0; s1 < 4; s1++) {
            s0 = RR_RELEASE_JP ? SaveSlotStageTrophyMapJp[sp34][s1] : SaveSlotStageTrophyMap[sp34][s1];
            if (s0 == -1) {
                break;
            }
            s5_1 = fp | 0x7FF8;
            s3 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4, s7 + 8, &UiFontGlyphs[s0], s5_1, s2);
            s4 += 2 + UiFontGlyphs[s0].w;
        }
        if (s6 == 4) {
            if (ReverseCourse == 1) {
                s0 = 0xD;
            } else {
                s0 = 0xC;
            }
            s3 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4, s7 + 8, &UiFontGlyphs[s0], 0x7FF8, 0);
        } else if ((s6 - 8) < 2U) {
            s3 = AddTextSprites(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4, s7 + 8, (s6 == 8) ? StrDigitOne : StrDigitTwo, fp | 0x7FF8, 0, s2);
        }
        break;
    case 2:
        for (s1 = 0; s1 < 7; s1++) {
            s0 = UiScrollStepOffsets[sp34][s1];
            if (s0 == -1) {
                break;
            }
            a0_arg = &CurrentDrawBuf->buffer.ordering70.tags[0];
            glyph = &UiFontGlyphs[s0];
            s5_2 = fp | 0x7FF8;
            s3 = AddSpritePrim(a0_arg, s3, s4, s7 + 8, glyph, s5_2, 0);
            s4 += 2 + UiFontGlyphs[s0].w;
        }
        s3 = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, sp38 + 0xA9, s7 + 6, 1, 0x14, 0xFF, 0xFF, 0xFF);
        s4 = sp38 + 0xB0;
        for (s1 = 0; s1 < 2; s1++) {
            if (s1 == 0) {
                s0 = 0x45;
            } else {
                s0 = UiScrollStepOffsets[sp34][7];
            }
            s3 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4, s7 + 8, &UiFontGlyphs[s0], 0x7FF8, 0);
            s4 += 2 + UiFontGlyphs[s0].w;
        }
        break;
    case 3:
        for (s1 = 0; s1 < 7; s1++) {
            s0 = UiScrollStepOffsets[sp34][s1];
            if (s0 == -1) {
                break;
            }
            s3 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4, s7 + 8, &UiFontGlyphs[s0], 0x7FF8, 0);
            s4 += 2 + UiFontGlyphs[s0].w;
            if (s1 == 3) {
                s3 = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, sp38 + 0x7E, s7 + 6, 1, 0x14, 0xFF, 0xFF, 0xFF);
                s4 = sp38 + 0x86;
            }
        }
        break;
    case 10:
    case 11:
        s3 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4, s7 + 8, &SaveSlotStatusUvs[0], 0x7FF8, 0);
        s3 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4 + 0x44, s7 + 8, &SaveSlotStatusUvs[1], 0x7FF8, 0);
        /* The PS1 reads the words at 0x80119754 / 0x80119758, in the overlay
         * window: the options overlay's saved music and effect volumes (the
         * only rows of type 10 / 11 are its volume sliders). */
        if (s6 == 10) {
            s0 = ovl665_saved_bgm_volume;
        } else {
            s0 = ovl665_saved_sfx_volume;
        }
        for (s1 = 0; s1 < s0; s1++) {
            s3 = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4 + 0x12 + s1 * 3, s7 + 8, 2, 0x10, 0xFF, 0xFF, 0xFF);
        }
        break;
    case 12:
        if (SaveProgress.prefix.flags5C[SelectedMakerIndex] == 1) {
            s2 = 8;
        } else {
            s2 = 7;
        }
        if (s2 != 0) {
            s1 = 0;
            do {
                s0 = UiScrollStepOffsets[sp34][s1];
                s3 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4, s7 + 8, &UiFontGlyphs[s0], 0x7FF8, 0);
                s4 += 2 + UiFontGlyphs[s0].w;
                s1++;
            } while (s1 < s2);
        }
        break;
    case 0x7E:
        s3 = AddTextSprites(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4, s7 + 8, StrPleaseWait, 0x7FF8, 2, 0);
        break;
    case 0x7F:
        s3 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, s4, s7 + 8, &UiCursorArrowUvs[sp34], fp | 0x4820, s2);
        break;
    }

    RR_GPU_SCRATCH->cursor = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3, 0x38);
    DrawUiBox(sp38, s7, UiRows[arg0].half08, UiRows[arg0].half0A, 0x4A, 0x4A, 0x4A);
    RR_GPU_SCRATCH->cursor = AddDrawAreaPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], RR_GPU_SCRATCH->cursor, sp28.x, sp28.y, sp28.w, sp28.h);
    sp38 += UiRows[arg0].rect18.w;
    if ((sp30 - 4) >= 3U) {
        DrawUiWindowRowText(arg0, sp38, s7);
    }
    return (u32)~sp30 >> 31;
}

void DrawUiBox(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6)
{
    int *px;
    int *py;
    GpuDmaTag *t;
    int k;
    int j;
    int x[4];
    void *g;
    int y[4];
    int i;

    x[0] = x[3] = a0;
    x[1] = x[2] = a0 + (u32)a2;
    y[0] = y[1] = a1;
    y[2] = y[3] = a1 + (u32)a3 - 2U;
    i = 0;
    do {
        t = &CurrentDrawBuf->buffer.ordering70.tags[0];
        px = x + i;
        py = y + i;
        j = i + 1;
        k = j;
        if (k < 0) k = i + 4;
        k >>= 2;
        k <<= 2;
        k = j - k;
        AddPartialLine(t, *px, *py, x[k], y[k], 0x100, 1, 0xFF, 0xFF, 0xFF, -1);
        i++;
    } while (i < 4);
    x[0] = x[1] - (u32)x[3];
    y[0] = y[3] - (u32)y[1];
    g = RR_GPU_SCRATCH->cursor;
    RR_GPU_SCRATCH->cursor = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], g, a0, a1, x[0], y[0], a4, a5, a6);
}

int AllocImageRows(void *a) {
    int sz = ImageRowCount * 28;
    ImageRows = (ImageRow20 *)a;
    PsyqBzero(a, sz);
    return sz;
}

void InitImageRow(int i, int a, int b, int j) {
    if (i < ImageRowCount) {
        ImagePreset10 *q;
        ImageRow20 *p;
        ImageRows[i].f1 = a;
        ImageRows[i].f3 = b;
        p = &ImageRows[i];
        q = &PreviewImageRowPresets[j];
        p->geom.s[0] = q->s[0];
        p->geom.s[1] = q->s[1];
        p->geom.s[2] = q->s[2];
        p->geom.s[3] = q->s[3];
        p->geom.s[4] = q->s[4];
        p->geom.s[5] = q->s[5];
        p->b0 = q->b0;
        ImageRows[i].b1 = q->b1;
    }
}

int UpdateImageRow(int a0) {
    int s1;
    int s2;
    int state;
    unsigned short val;
    s32 sp[16];

    s2 = 0;
    state = ImageRows[a0].f1;
    s1 = ImageRows[a0].f2;
    switch (state) {
    case -1:
        if (UpdateRect16Row(ImageRows[a0].f3) == 1) {
            ImageRows[a0].f1 = 0;
        }
        break;
    case 0:
        ImageRows[a0].f2 = 0;
        ImageRows[a0].f1 = 1;
    case 1:
        s2 = 1;
        val = (s1 << 7) / 30;
        DrawImageRow(a0, 0, val, 0);
        if (s1 >= 30) {
            ImageRows[a0].f2 = 0;
            ImageRows[a0].f1 = 2;
        } else {
            ImageRows[a0].f2++;
        }
        break;
    case 2:
    case 4:
    default:
        s2 = 2;
        DrawImageRow(a0, 0, 0x80, 0);
        break;
    case 3:
        val = rcos((s1 << 10) / 30) / 32;
        DrawImageRow(a0, 1, val, 0);
        if (s1 >= 30) {
            ImageRows[a0].f1 = 5;
            ImageRows[a0].f2 = 0;
        } else {
            ImageRows[a0].f2++;
        }
        break;
    case 5:
        break;
    case 6:
        s2 = 1;
        val = ((30 - s1) << 7) / 30;
        DrawImageRow(a0, 0, val, 0);
        if (s1 >= 30) {
            ImageRows[a0].f2 = 0;
            ImageRows[a0].f1 = 7;
        } else {
            ImageRows[a0].f2++;
        }
        break;
    case 7:
        s2 = 1;
        DrawImageRow(a0, 0, 0, 0);
        break;
    }
    return s2;
}

void DrawImageRow(int a0, int a1, int a2, int a3) {
    int i, j, k;
    int tx, ty;
    int x[4], y[4], u[4], v[4];
    unsigned short tpage, clut;
    void *cursor;

    cursor = RR_GPU_SCRATCH->cursor;
    for (i = 0; i < 18; i++) {
        tx = ImageRows[a0].geom.coords.u + (i % 9) * 24;
        ty = ImageRows[a0].geom.coords.v + (i / 9) * 96;
        tpage = ((ty & 0x100) >> 4) | 0x80 | ((tx & 0x3FF) >> 6);
        clut = GetClut(ImageRows[a0].geom.coords.cx, ImageRows[a0].geom.coords.cy + i / 2);
        for (j = 0; j < 4; j++) {
            if (a1 == 0) {
                u[0] = (tx % 64) * 2 + (j & 1) * 24;
                v[0] = (i / 9) * 96;
                if (j >= 2) {
                    v[0] += 48;
                }
                x[0] = ImageRows[a0].geom.coords.x + (i % 6) * 48 + (j & 1) * 24;
                y[0] = ImageRows[a0].geom.coords.y + (i / 6) * 96;
                if (j >= 2) {
                    y[0] += 48;
                }
                cursor = AddSpriteWithDrawMode(&CurrentDrawBuf->buffer.ordering70.tags[694], cursor, x[0], y[0], 24, 48, u[0], v[0], tpage, clut, a3, a2);
            } else if (a1 == 1) {
                for (k = 0; k < 4; k++) {
                    x[k] = ImageRows[a0].geom.coords.x + (i % 6) * 48 + (k & 1) * 48;
                    y[k] = ImageRows[a0].geom.coords.y + (i / 6) * 96 + j * 24;
                    if (k >= 2) {
                        y[k] += 24;
                    }
                    u[k] = (tx % 64) * 2 + (k & 1) * 48;
                    v[k] = (i / 9) * 96 + j * 24;
                    if (k >= 2) {
                        v[k] += 24;
                    }
                }
                y[0] = -(a2 * 12) / 128 + y[0] + 12;
                y[1] = -(a2 * 12) / 128 + y[1] + 12;
                y[2] = (a2 * 12) / 128 + y[2] - 12;
                y[3] = (a2 * 12) / 128 + y[3] - 12;
                cursor = EmitPolyFT3(&CurrentDrawBuf->buffer.ordering70.tags[694], cursor, &x[0], &y[0], &u[0], &v[0], tpage, clut, 0, a2);
                cursor = EmitPolyFT3(&CurrentDrawBuf->buffer.ordering70.tags[694], cursor, &x[1], &y[1], &u[1], &v[1], tpage, clut, 0, a2);
            }
        }
    }
    RR_GPU_SCRATCH->cursor = cursor;
}

/* Initial values of the globals this module owns. */
const char StrDigitOne[4] = {49};
const char StrDigitTwo[4] = {50};
const char StrPleaseWait[600] = {80, 76, 69, 65, 83, 69, 32, 87, 65, 73, 84};
ImagePreset10 PreviewImageRowPresets[2] = {{.s = {0x10, 0x48, 0x280, 0, 0x280, 0xEA}}, {.s = {0x10, 0x48, 0x280, 0x100, 0x280, 0xF4}}};
/* The whole glyph table, including the ranges reached as RecordsHeaderSpriteUvs,
 * SaveSlotStatusUvs, MenuStepArrowUv and SaveSlotTrophyUvs (rr/ui_bar.h). */
UiUvRect UiFontGlyphsPal[126] = {{0x78, 0x20, 0x16, 0x10}, {0x90, 0x20, 0x12, 0x10}, {0xA4, 0x20, 0xD, 0x10}, {0xB4, 0x20, 0x12, 0x10}, {0xC8, 0x20, 0x10, 0x10}, {0xDC, 0x20, 0xD, 0x10}, {0xEC, 0x20, 0x13, 0x10}, {0x0, 0x30, 0x10, 0x10}, {0x14, 0x30, 0x10, 0x10}, {0x28, 0x30, 0x3, 0x10}, {0x30, 0x30, 0x26, 0x10}, {0x58, 0x30, 0x24, 0x10}, {0x94, 0x30, 0x21, 0x10}, {0xB8, 0x30, 0x24, 0x10}, {0x0, 0x40, 0x2C, 0x10}, {0x30, 0x40, 0x14, 0x10}, {0x48, 0x40, 0x12, 0x10}, {0x5C, 0x40, 0x11, 0x10}, {0x70, 0x40, 0xF, 0x10}, {0x80, 0x40, 0x1A, 0x10}, {0x9C, 0x40, 0x18, 0x10}, {0xB8, 0x40, 0x2A, 0x10}, {0xE4, 0x40, 0x19, 0x10}, {0x0, 0x50, 0x1C, 0x10}, {0x20, 0x50, 0x1E, 0x10}, {0x40, 0x50, 0x1C, 0x10}, {0x60, 0x50, 0x13, 0x10}, {0x74, 0x50, 0xF, 0x10}, {0x84, 0x50, 0x10, 0x10}, {0x98, 0x50, 0x1A, 0x10}, {0xB4, 0x50, 0xD, 0x10}, {0xC4, 0x50, 0x13, 0x10}, {0xD8, 0x50, 0x1E, 0x10}, {0x0, 0x60, 0xC, 0x10}, {0x10, 0x60, 0xE, 0x10}, {0x20, 0x60, 0xC, 0x10}, {0x30, 0x60, 0xC, 0x10}, {0x40, 0x60, 0xC, 0x10}, {0x50, 0x60, 0x15, 0x10}, {0x68, 0x60, 0x12, 0x10}, {0x7C, 0x60, 0x25, 0x10}, {0xA4, 0x60, 0x12, 0x10}, {0xB8, 0x60, 0x15, 0x10}, {0xD8, 0x60, 0x13, 0x10}, {0xEC, 0x60, 0x12, 0x10}, {0x0, 0x70, 0x31, 0x10}, {0x34, 0x70, 0x27, 0x10}, {0x5C, 0x70, 0x40, 0x10}, {0xA0, 0x70, 0x28, 0x10}, {0xCC, 0x70, 0x2B, 0x10}, {0x0, 0x80, 0x35, 0x10}, {0x38, 0x80, 0x3A, 0x10}, {0x74, 0x80, 0x3A, 0x10}, {0xB0, 0x80, 0x18, 0x10}, {0xCC, 0x80, 0x1A, 0x10}, {0xE8, 0x80, 0xA, 0x10}, {0xF4, 0x80, 0xC, 0x10}, {0x0, 0x90, 0x55, 0x10}, {0x58, 0x90, 0xD, 0x10}, {0x68, 0x90, 0x19, 0x10}, {0x84, 0x90, 0x11, 0x10}, {0x98, 0x90, 0x13, 0x10}, {0xB0, 0x90, 0x1F, 0x10}, {0xD4, 0x90, 0xC, 0x10}, {0xE4, 0x90, 0xD, 0x10}, {0x0, 0xA0, 0x18, 0x10}, {0x1C, 0xA0, 0x13, 0x10}, {0x30, 0xA0, 0x13, 0x10}, {0x44, 0xA0, 0x14, 0x10}, {0x5C, 0xA0, 0x1C, 0x10}, {0x7C, 0xA0, 0x12, 0x10}, {0x90, 0xA0, 0x1B, 0x10}, {0xB0, 0xA0, 0x11, 0x10}, {0xC4, 0xA0, 0x19, 0x10}, {0xE0, 0xA0, 0x15, 0x10}, {0x0, 0xB0, 0x1F, 0x10}, {0x20, 0xB0, 0x19, 0x10}, {0x3C, 0xB0, 0x21, 0x10}, {0x60, 0xB0, 0x17, 0x10}, {0x78, 0xB0, 0x16, 0x10}, {0x90, 0xB0, 0x15, 0x10}, {0xA8, 0xB0, 0x13, 0x10}, {0xBC, 0xB0, 0x17, 0x10}, {0xD4, 0xB0, 0xF, 0x10}, {0xE8, 0xB0, 0x13, 0x10}, {0x0, 0xC0, 0xF, 0x10}, {0x10, 0xC0, 0x17, 0x10}, {0x28, 0xC0, 0xE, 0x10}, {0x38, 0xC0, 0x1A, 0x10}, {.u = 0x54, .v = 0xC0, .w = 0xE, .h = 0x10}, {.u = 0x64, .v = 0xC0, .w = 0x10, .h = 0x10}, {.u = 0x78, .v = 0xC0, .w = 0x19, .h = 0x10}, {.u = 0x94, .v = 0xC0, .w = 0x14, .h = 0x10}, {.u = 0xAC, .v = 0xC0, .w = 0xD, .h = 0x10}, {.u = 0xBC, .v = 0xC0, .w = 0x14, .h = 0x10}, {.u = 0xD4, .v = 0xC0, .w = 0x17, .h = 0x10}, {.u = 0xEC, .v = 0xC0, .w = 0xA, .h = 0x10}, {.v = 0xD0, .w = 0x3D, .h = 0x10}, {.u = 0x48, .v = 0xD0, .w = 0x1A, .h = 0x10}, {.u = 0x64, .v = 0xD0, .w = 0x1A, .h = 0x10}, {.u = 0x80, .v = 0xD0, .w = 0x34, .h = 0x10}, {.u = 0xB8, .v = 0xD0, .w = 0x11, .h = 0x10}, {.u = 0xCC, .v = 0xD0, .w = 0x13, .h = 0x10}, {.u = 0xDC, .v = 0x30, .w = 0x24, .h = 0x10}, {.u = 0xE0, .v = 0xD0, .w = 0xE, .h = 0x10}, {.u = 0xF0, .v = 0xD0, .w = 0xA, .h = 0x10}, {.v = 0xE0, .w = 0x22, .h = 0x10}, {.u = 0x24, .v = 0xE0, .w = 0xA, .h = 0x10}, {.u = 0x30, .v = 0xE0, .w = 0xB, .h = 0x10}, {.u = 0x40, .v = 0xE0, .w = 0x31, .h = 0x10}, {.u = 0x74, .v = 0xE0, .w = 0x15, .h = 0x10}, {.u = 0x8C, .v = 0xE0, .w = 0x30, .h = 0x10}, {.u = 0xC0, .v = 0xE0, .w = 0x3B, .h = 0x10}, {.v = 0xF0, .w = 0x1F, .h = 0x10}, {.u = 0x20, .v = 0xF0, .w = 0x29, .h = 0x10}, {.u = 0x4C, .v = 0xF0, .w = 0x19, .h = 0x10}, {0x68, 0xF0, 0x2D, 0x10}, {0x98, 0xF0, 0x12, 0x10}, {0xAC, 0xF0, 0x15, 0x10}, {0xC4, 0xF0, 0xA, 0x10}, {0xD0, 0xF0, 0x16, 0x10}, {0xE8, 0xF0, 0x17, 0x10}, {0xE0, 0x10, 0x20, 0x10}, {0x80, 0x10, 0x4, 0x10}, {.u = 0x64, .v = 0x20, .w = 4, .h = 0x10}, {.u = 0x70, .v = 0x20, .w = 4, .h = 0x10}};
/* The Japanese release's values (SLPS-01800). */
const UiUvRect UiFontGlyphsJp[126] = {{.u = 0x78, .v = 0x20, .w = 0x16, .h = 0x10}, {.u = 0x90, .v = 0x20, .w = 0x12, .h = 0x10}, {.u = 0xA4, .v = 0x20, .w = 0xD, .h = 0x10}, {.u = 0xB4, .v = 0x20, .w = 0x12, .h = 0x10}, {.u = 0xC8, .v = 0x20, .w = 0x10, .h = 0x10}, {.u = 0xDC, .v = 0x20, .w = 0xD, .h = 0x10}, {.u = 0xEC, .v = 0x20, .w = 0x12, .h = 0x10}, {.v = 0x30, .w = 0x10, .h = 0x10}, {.u = 0x14, .v = 0x30, .w = 0x10, .h = 0x10}, {.u = 0x28, .v = 0x30, .w = 3, .h = 0x10}, {.u = 0x30, .v = 0x30, .w = 0x42, .h = 0x10}, {.u = 0x74, .v = 0x30, .w = 0x1C, .h = 0x10}, {.u = 0x94, .v = 0x30, .w = 0x21, .h = 0x10}, {.u = 0xB8, .v = 0x30, .w = 0x24, .h = 0x10}, {.v = 0x40, .w = 0x2C, .h = 0x10}, {.u = 0x30, .v = 0x40, .w = 0x14, .h = 0x10}, {.u = 0x48, .v = 0x40, .w = 0x12, .h = 0x10}, {.u = 0x5C, .v = 0x40, .w = 0x11, .h = 0x10}, {.u = 0x70, .v = 0x40, .w = 0xF, .h = 0x10}, {.u = 0x80, .v = 0x40, .w = 0x1A, .h = 0x10}, {.u = 0x9C, .v = 0x40, .w = 0x18, .h = 0x10}, {.u = 0xB8, .v = 0x40, .w = 0x2A, .h = 0x10}, {.u = 0xE4, .v = 0x40, .w = 0x19, .h = 0x10}, {.v = 0x50, .w = 0x1C, .h = 0x10}, {.u = 0x20, .v = 0x50, .w = 0x1E, .h = 0x10}, {.u = 0x40, .v = 0x50, .w = 0x1C, .h = 0x10}, {.u = 0x60, .v = 0x50, .w = 0x13, .h = 0x10}, {.u = 0x74, .v = 0x50, .w = 0xF, .h = 0x10}, {.u = 0x84, .v = 0x50, .w = 0x10, .h = 0x10}, {.u = 0x98, .v = 0x50, .w = 0x1A, .h = 0x10}, {.u = 0xB4, .v = 0x50, .w = 0xD, .h = 0x10}, {.u = 0xC4, .v = 0x50, .w = 0x13, .h = 0x10}, {.u = 0xD8, .v = 0x50, .w = 0x1E, .h = 0x10}, {.v = 0x60, .w = 0xC, .h = 0x10}, {.u = 0x10, .v = 0x60, .w = 0xE, .h = 0x10}, {.u = 0x20, .v = 0x60, .w = 0xC, .h = 0x10}, {.u = 0x30, .v = 0x60, .w = 0xC, .h = 0x10}, {.u = 0x40, .v = 0x60, .w = 0xC, .h = 0x10}, {.u = 0x50, .v = 0x60, .w = 0x15, .h = 0x10}, {.u = 0x68, .v = 0x60, .w = 0x12, .h = 0x10}, {.u = 0x7C, .v = 0x60, .w = 0x25, .h = 0x10}, {.u = 0xA4, .v = 0x60, .w = 0x12, .h = 0x10}, {.u = 0xB8, .v = 0x60, .w = 0x1C, .h = 0x10}, {.u = 0xD8, .v = 0x60, .w = 0x13, .h = 0x10}, {.u = 0xEC, .v = 0x60, .w = 0x12, .h = 0x10}, {.v = 0x70, .w = 0x31, .h = 0x10}, {.u = 0x34, .v = 0x70, .w = 0x27, .h = 0x10}, {.u = 0x5C, .v = 0x70, .w = 0x40, .h = 0x10}, {.u = 0xA0, .v = 0x70, .w = 0x28, .h = 0x10}, {.u = 0xCC, .v = 0x70, .w = 0x2B, .h = 0x10}, {.v = 0x80, .w = 0x35, .h = 0x10}, {.u = 0x38, .v = 0x80, .w = 0x3A, .h = 0x10}, {.u = 0x74, .v = 0x80, .w = 0x3A, .h = 0x10}, {.u = 0xB0, .v = 0x80, .w = 0x18, .h = 0x10}, {.u = 0xCC, .v = 0x80, .w = 0x1A, .h = 0x10}, {.u = 0xE8, .v = 0x80, .w = 0xA, .h = 0x10}, {.u = 0xF4, .v = 0x80, .w = 0xC, .h = 0x10}, {.v = 0x90, .w = 0x55, .h = 0x10}, {.u = 0x58, .v = 0x90, .w = 0xD, .h = 0x10}, {.u = 0x68, .v = 0x90, .w = 0x19, .h = 0x10}, {.u = 0x84, .v = 0x90, .w = 0x11, .h = 0x10}, {.u = 0x98, .v = 0x90, .w = 0x13, .h = 0x10}, {.u = 0xB0, .v = 0x90, .w = 0x1F, .h = 0x10}, {.u = 0xD4, .v = 0x90, .w = 0xC, .h = 0x10}, {.u = 0xE4, .v = 0x90, .w = 0xD, .h = 0x10}, {.v = 0xA0, .w = 0x18, .h = 0x10}, {.u = 0x1C, .v = 0xA0, .w = 0x13, .h = 0x10}, {.u = 0x30, .v = 0xA0, .w = 0x13, .h = 0x10}, {.u = 0x44, .v = 0xA0, .w = 0x14, .h = 0x10}, {.u = 0x5C, .v = 0xA0, .w = 0x1C, .h = 0x10}, {.u = 0x7C, .v = 0xA0, .w = 0x12, .h = 0x10}, {.u = 0x90, .v = 0xA0, .w = 0x1B, .h = 0x10}, {.u = 0xB0, .v = 0xA0, .w = 0x11, .h = 0x10}, {.u = 0xC4, .v = 0xA0, .w = 0x19, .h = 0x10}, {.u = 0xE0, .v = 0xA0, .w = 0x15, .h = 0x10}, {.v = 0xB0, .w = 0x1F, .h = 0x10}, {.u = 0x20, .v = 0xB0, .w = 0x19, .h = 0x10}, {.u = 0x3C, .v = 0xB0, .w = 0x21, .h = 0x10}, {.u = 0x60, .v = 0xB0, .w = 0x17, .h = 0x10}, {.u = 0x78, .v = 0xB0, .w = 0x16, .h = 0x10}, {.u = 0x90, .v = 0xB0, .w = 0x15, .h = 0x10}, {.u = 0xA8, .v = 0xB0, .w = 0x13, .h = 0x10}, {.u = 0xBC, .v = 0xB0, .w = 0x17, .h = 0x10}, {.u = 0xD4, .v = 0xB0, .w = 0xF, .h = 0x10}, {.u = 0xE8, .v = 0xB0, .w = 0x13, .h = 0x10}, {.v = 0xC0, .w = 0xF, .h = 0x10}, {.u = 0x10, .v = 0xC0, .w = 0x17, .h = 0x10}, {.u = 0x28, .v = 0xC0, .w = 0xE, .h = 0x10}, {.u = 0x38, .v = 0xC0, .w = 0x1A, .h = 0x10}, {.u = 0x54, .v = 0xC0, .w = 0xE, .h = 0x10}, {.u = 0x64, .v = 0xC0, .w = 0x10, .h = 0x10}, {.u = 0x78, .v = 0xC0, .w = 0x19, .h = 0x10}, {.u = 0x94, .v = 0xC0, .w = 0x14, .h = 0x10}, {.u = 0xAC, .v = 0xC0, .w = 0xD, .h = 0x10}, {.u = 0xBC, .v = 0xC0, .w = 0x14, .h = 0x10}, {.u = 0xD4, .v = 0xC0, .w = 0x17, .h = 0x10}, {.u = 0xEC, .v = 0xC0, .w = 0xA, .h = 0x10}, {.v = 0xD0, .w = 0x43, .h = 0x10}, {.u = 0x48, .v = 0xD0, .w = 0x1A, .h = 0x10}, {.u = 0x64, .v = 0xD0, .w = 0x1A, .h = 0x10}, {.u = 0x80, .v = 0xD0, .w = 0x34, .h = 0x10}, {.u = 0xB8, .v = 0xD0, .w = 0x11, .h = 0x10}, {.u = 0xCC, .v = 0xD0, .w = 0x13, .h = 0x10}, {.u = 0xDC, .v = 0x30, .w = 0x24, .h = 0x10}, {.u = 0xE0, .v = 0xD0, .w = 0xE, .h = 0x10}, {.u = 0xF0, .v = 0xD0, .w = 0xA, .h = 0x10}, {.v = 0xE0, .w = 0x22, .h = 0x10}, {.u = 0x24, .v = 0xE0, .w = 0xA, .h = 0x10}, {.u = 0x30, .v = 0xE0, .w = 0xB, .h = 0x10}, {.u = 0x40, .v = 0xE0, .w = 0x31, .h = 0x10}, {.u = 0x74, .v = 0xE0, .w = 0x15, .h = 0x10}, {.u = 0x8C, .v = 0xE0, .w = 0x30, .h = 0x10}, {.u = 0xC0, .v = 0xE0, .w = 0x3B, .h = 0x10}, {.v = 0xF0, .w = 0x1F, .h = 0x10}, {.u = 0x20, .v = 0xF0, .w = 0x29, .h = 0x10}, {.u = 0x4C, .v = 0xF0, .w = 0x19, .h = 0x10}, {.u = 0x68, .v = 0xF0, .w = 0x2D, .h = 0x10}, {.u = 0x98, .v = 0xF0, .w = 0x12, .h = 0x10}, {.u = 0xAC, .v = 0xF0, .w = 0x15, .h = 0x10}, {.u = 0xC4, .v = 0xF0, .w = 0xA, .h = 0x10}, {.u = 0xD0, .v = 0xF0, .w = 0x16, .h = 0x10}, {.u = 0xE8, .v = 0xF0, .w = 0x17, .h = 0x10}, {.u = 0xE0, .v = 0x10, .w = 0x20, .h = 0x10}, {.u = 0x80, .v = 0x10, .w = 4, .h = 0x10}, {.u = 0x64, .v = 0x20, .w = 4, .h = 0x10}, {.u = 0x70, .v = 0x20, .w = 4, .h = 0x10}};
s8 SaveSlotStageTrophyMap[26][4] = {{19, 20, 45, 80}, {19, 20, 46, 80}, {19, 20, 47, 82}, {19, 20, 48, 80}, {19, 20, 49, 80}, {19, 20, 50, 82}, {19, 20, 51, 80}, {19, 20, 52, 82}, {61, 20, 68, 82}, {61, 20, 65, 79}, {61, 20, 66, 80}, {61, 20, 67, 81}, {64, 28, -1, -1}, {4, 5, -1, -1}, {4, 6, -1, -1}, {42, 27, -1, -1}, {30, 32, -1, -1}, {31, 32, -1, -1}, {120, 121, -1, -1}, {97, 121, -1, -1}, {98, 99, -1, -1}, {98, 114, -1, -1}, {117, 119, -1, -1}, {117, 118, -1, -1}, {64, 28, -1, -1}, {98, 112, -1, -1}};
/* The Japanese release's values (SLPS-01800). */
static const s8 SaveSlotStageTrophyMapJp[26][4] = {{0x13, 0x14, 0x2D, 0x50}, {0x13, 0x14, 0x2E, 0x50}, {0x13, 0x14, 0x2F, 0x52}, {0x13, 0x14, 0x30, 0x50}, {0x13, 0x14, 0x31, 0x50}, {0x13, 0x14, 0x32, 0x52}, {0x13, 0x14, 0x33, 0x50}, {0x13, 0x14, 0x34, 0x52}, {0x3D, 0x14, 0x44, 0x52}, {0x3D, 0x14, 0x41, 0x4F}, {0x3D, 0x14, 0x42, 0x50}, {0x3D, 0x14, 0x43, 0x51}, {0x40, 0x1C, -1, -1}, {4, 5, -1, -1}, {4, 6, -1, -1}, {0x2A, 0x1B, -1, -1}, {0x1E, 0x20, -1, -1}, {0x1F, 0x20, -1, -1}, {0x78, 0x79, -1, -1}, {0x61, 0x79, -1, -1}, {0x62, 0x63, -1, -1}, {0x72, 0x73, -1, -1}, {0x75, 0x77, -1, -1}, {0x75, 0x76, -1, -1}, {0x40, 0x1C, -1, -1}, {0x3D, 0x14, 0x3F, 0x3B}};
s8 UiScrollStepOffsets[12][8] = {{61, 20, 63, 59, 61, 82, -1, 73}, {61, 20, 57, 79, -1, 0, 0, 70}, {61, 20, 58, 59, 60, 80, -1, 71}, {61, 20, 59, 61, 62, 81, -1, 72}, {95, 20, 77, 81, 16, 123, 17, -1}, {95, 20, 78, 82, 16, 123, 17, -1}, {95, 20, 76, 80, 16, 123, 18, -1}, {95, 20, 75, 79, 16, 123, 18, -1}, {7, 20, 77, 8, 9, 77, 10, 11}, {7, 20, 78, 8, 9, 78, 10, 11}, {7, 20, 76, 8, 9, 76, 10, 11}, {7, 20, 75, 8, 9, 75, 10, 11}};
UiUvRect UiCursorArrowUvs[9] = {{.w = 0x28, .h = 0x10}, {.u = 0x78, .w = 0x30, .h = 0x10}, {.u = 0x28, .w = 0x24, .h = 0x10}, {.u = 0xA8, .w = 0x1C, .h = 0x10}, {.u = 0x4C, .v = 0x10, .w = 0x20, .h = 0x10}, {.u = 0xC4, .w = 0x38, .h = 0x10}, {.v = 0x10, .w = 0x20, .h = 0x10}, {.u = 0x20, .v = 0x10, .w = 0x28, .h = 0x10}, {.u = 0x4C, .w = 0x28, .h = 0x10}};
s8 UiTimeTextScratch[4] = {0, 0, 8, 16};
int ImageRowCount;
ImageRow20 *ImageRows;
UiRow24 *UiRows;
