#include "rr/release.h"
/* RODATA 0x800103c0 0x1c */
#include "rr/widescreen.h"
#include "scratchpad.h"
#include "rr/frame_storage_types.h"
#include "rr/gpu.h"
#include "psyq/libgpu/primattr.h"
#include "psyq/libgpu/settrans.h"
#include "rr/race_setup.h"
#include "rr/hud_number.h"
#include "psyq/libgpu/gpudisp.h"
#include "rr/overlay662_draw_area.h"
#include "rr/animated_glyph.h"
#include "game/ui/badge1.h"

#include "game/ui/window1.h"
#include "rr/frame_loop.h"
static const HudSpriteDef14 Rect16RowDefaultPresetsJp[12];
static const HudSpriteDef14 BadgeRowPresetTableAJp[7];

extern HudSpriteDef14 BadgeRowPresetTableA[];
extern HudSpriteDef14 BadgeRowPresetTableB[];
extern HudSpriteDef14 Rect16RowDefaultPresets[];

/* The prebuilt race HUD sprites' own x (widescreen moves them per frame). */
static s16 HudSpriteBaseX[12];

void InitHudSpritePacket(SpritePacket *a0, const HudSpriteDef14 *a1) {
    a0->tag.length = 4;
    a0->code = 0x64;
    a0->x = a1->x00;
    a0->y = a1->y02;
    a0->w = a1->w04;
    a0->h = a1->h06;
    a0->u = a1->u08;
    a0->v = a1->v0A;
    a0->clut = a1->clut0C;
    if (a1->semiTrans10 != 0) {
        a0->code |= 2;
    } else {
        a0->code &= 0xFD;
    }
    a0->code |= 1;
}

void InitRaceHudPackets(int a0)
{
    int n, i, j;
    const HudSpriteDef14 *tab;
    SpritePacket *sprites;
    switch (a0) {
    case 0:
    case 5:
    default:
        n = 4;
        tab = RR_PAL_JP(BadgeRowPresetTableA, BadgeRowPresetTableAJp);
        break;
    case 1:
        n = 8;
        tab = BadgeRowPresetTableB;
        break;
    case 2:
    case 3:
    case 4:
    case 6:
        if ((LinkPlayerCount == 2 && a0 == 4) || a0 == 6) {
            n = 6;
        } else {
            n = 12;
        }
        tab = RR_PAL_JP(Rect16RowDefaultPresets, Rect16RowDefaultPresetsJp);
        break;
    }
    for (i = 0; i < n; i++) {
        for (j = 0; j < 2; j++) {
            sprites = DrawBuffers[j].buffer.hudSprites;
            InitHudSpritePacket(&sprites[i], &tab[i]);
            HudSpriteBaseX[i] = tab[i].x00;
            /* Chain each sprite after its predecessor. */
            if (i != 0) RR_GPU_LINK(&sprites[i - 1].tag, &sprites[i]);
        }
    }
}

void SubmitRaceHudPackets(int a0) {
    GpuRectangleFields area;
    GpuDmaTag *s0;
    int a1;
    SpritePacket *sprites;
    GpuDrawModePacket *s1;
    switch (a0) {
    case 0:
    default:
        a1 = 4;
        s0 = &CurrentDrawBuf->buffer.ordering70.tags[0];
        break;
    case 1:
        a1 = 8;
        s0 = &CurrentDrawBuf->buffer.ordering70.tags[0];
        break;
    case 2:
    case 3:
    case 4:
        if ((LinkPlayerCount == 2 && a0 == 3) || a0 == 4) {
            a1 = 6;
            s0 = &CurrentDrawBuf->buffer.ordering70.tags[0];
        } else {
            a1 = 0xC;
            s0 = &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0];
        }
        break;
    }
    /* Splice the prebuilt chain hudSprites[0..a1-1] in at the head. */
    sprites = CurrentDrawBuf->buffer.hudSprites;
    /* Widescreen: the labels on the left move to the 16:9 view's left edge,
     * the others to its right edge. */
    for (int i = 0; i < a1; i++)
        sprites[i].x = (s16)(HudSpriteBaseX[i] + WideHudShift(HudSpriteBaseX[i] < 0xA0 ? -1 : 1));
    sprites[a1 - 1].tag.address = s0->address;
    s0->address = (uintptr_t)&sprites[0];
    s1 = AddDrawModePrim(s0, (GpuDrawModePacket *)RR_GPU_SCRATCH->cursor, 5);
    RR_GPU_SCRATCH->cursor = (u8 *)s1;
    if (a0 >= 2) {
        area.x = 0;
        area.y = DrawBufIndex << 8;
        area.w = 0x140;
        area.h = 0x100;
        SetDrawArea(s1, &area);
        RR_GPU_LINK(s0, s1);
        RR_GPU_SCRATCH->cursor = (u8 *)(s1 + 1);
    }
}

void DrawDigitString12(s32 a0, s32 a1, const char *text, s32 a3) {
    SpritePacketSignedClut *p = (SpritePacketSignedClut *)RR_GPU_SCRATCH->cursor;
    const u8 *a2 = (const u8 *)text;
    GpuDrawModePacket *mode;
    s32 t;
    s32 kb;
    s16 c;
    kb = 0xA8;
    if (*a2 != 0) {
        do {
            s32 ch = *a2;
            a2++;
            p->tag.length = 4;
            p->code = 0x64;
            p->x = a0;
            t = ch - 0x30;
            p->y = a1;
            if (t >= 0xD) {
                c = ch * 8 - 0x14C;
                p->w = 4;
                p->h = 8;
                if (t == 0xF)
                    a0 += 5;
                else
                    a0 += 4;
            } else {
                c = t * 12;
                a0 += 10;
                p->w = 12;
                p->h = 12;
            }
            p->u = c;
            p->v = kb;
            p->code |= 1;
            p->clut = a3;
            RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], p);
            p++;
        } while (*a2 != 0);
    }
    mode = (GpuDrawModePacket *)p;
    SetDrawMode(mode, 0, 1, 5, 0);
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], mode);
    RR_GPU_SCRATCH->cursor = (u8 *)(mode + 1);
}


void DrawDigitString8(GpuDmaTag *a0, s32 a1, s32 a2, const char *text, s32 a4) {
    SpritePacket *p = (SpritePacket *)RR_GPU_SCRATCH->cursor;
    const u8 *a3 = (const u8 *)text;
    s32 c, u;
    s32 kb;
    kb = 0xB4;
    if (*a3 != 0) {
        do {
            c = *a3;
            a3++;
            p->tag.length = 4;
            p->code = 0x64;
            p->x = a1;
            p->y = a2;
            p->w = 8;
            p->h = 8;
            c -= 0x30;
            u = c * 8 + 0x6C;
            if (c >= 10) a1 += 3; else a1 += 7;
            p->u = u;
            p->v = kb;
            p->code |= 1;
            p->clut = a4;
            RR_GPU_LINK(a0, p);
            p++;
        } while (*a3 != 0);
    }
    RR_GPU_SCRATCH->cursor = AddDrawModePrim(a0, (GpuDrawModePacket *)p, 5);
}


void DrawDigitString16(GpuDmaTag *a0, s32 a1, s32 a2, const char *text) {
    SpritePacket *p = (SpritePacket *)RR_GPU_SCRATCH->cursor;
    const u8 *a3 = (const u8 *)text;
    int c, u, kh;
    if (*a3 != 0) {
        kh = 0x10;
        do {
            c = *a3;
            a3++;
            p->tag.length = 4;
            p->code = 0x64;
            p->x = (short)a1;
            p->y = (short)a2;
            c -= 0x30;
            u = c * 8;
            if (c < 10) {
                a1 += 5;
                p->w = 8;
                p->h = kh;
            } else {
                a1 += 3;
                p->w = 4;
                p->h = kh;
            }
            SetShadeTex(p, 1);
            SetSemiTrans(p, 1);
            p->u = u;
            p->v = 0x58;
            p->clut = 0x7FA4;
            RR_GPU_LINK(a0, p);
            p++;
        } while (*a3 != 0);
    }
    RR_GPU_SCRATCH->cursor = AddDrawModePrim(a0, (GpuDrawModePacket *)p, 0x59);
}

/* Initial values of the globals this module owns. */
HudSpriteDef14 BadgeRowPresetTableB[8] = {{.x00 = 0xC, .y02 = 0xE, .w04 = 0x38, .h06 = 0xC, .v0A = 0xCC, .clut0C = 0x7EF8}, {.x00 = 0xE6, .y02 = 0xE, .w04 = 0x54, .h06 = 0xC, .u08 = 0x38, .v0A = 0xCC, .clut0C = 0x7EF8}, {.x00 = 0xC, .y02 = 0x1A, .w04 = 0x18, .h06 = 0x18, .v0A = 0xD8, .clut0C = 0x7EF8}, {.x00 = 0x20, .y02 = 0x1A, .w04 = 8, .h06 = 0x18, .u08 = 0xB8, .v0A = 0xC0, .clut0C = 0x7EF8}, {.x00 = 0x2C, .y02 = 0x20, .w04 = 0x10, .h06 = 0x10, .u08 = 0xC0, .v0A = 0xC8, .clut0C = 0x7EF8}, {.x00 = 0xE8, .y02 = 0x50, .w04 = 0x2C, .h06 = 8, .u08 = 0x40, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0xE7, .y02 = 0x1C, .w04 = 0xC, .h06 = 0xC, .u08 = 0x90, .v0A = 0xA8, .clut0C = 0x7EF8}, {.x00 = 0x40, .y02 = 0xE, .w04 = 0x14, .h06 = 0xC, .v0A = 0xB4, .clut0C = 0x7EF8}};
HudSpriteDef14 BadgeRowPresetTableA[7] = {{.x00 = 6, .y02 = 0xE, .w04 = 0x64, .h06 = 0xC, .v0A = 0xC0, .clut0C = 0x7EF8}, {.x00 = 0xE6, .y02 = 0xE, .w04 = 0x54, .h06 = 0xC, .u08 = 0x38, .v0A = 0xCC, .clut0C = 0x7EF8}, {.x00 = 0xE7, .y02 = 0x1C, .w04 = 0xC, .h06 = 0xC, .u08 = 0x90, .v0A = 0xA8, .clut0C = 0x7EF8}, {.x00 = 0xA, .y02 = 0x1C, .w04 = 0xC, .h06 = 0xC, .u08 = 0x90, .v0A = 0xA8, .clut0C = 0x7EF8}, {.x00 = 0xF4, .y02 = 0x70, .w04 = 0x48, .h06 = 8, .u08 = 0xA0, .v0A = 0x30, .clut0C = 0x4A7}, {.x00 = 8, .y02 = 0x20, .w04 = 8, .h06 = 8, .u08 = 0x50, .v0A = 0x40, .clut0C = 0x4E0}, {.x00 = 0x78, .y02 = 0x50, .w04 = 8, .h06 = 8, .u08 = 0x78, .v0A = 8, .clut0C = 0x4E0}};
/* The Japanese release's values (SLPS-01800). */
static const HudSpriteDef14 BadgeRowPresetTableAJp[7] = {{.x00 = 6, .y02 = 0xE, .w04 = 0x64, .h06 = 0xC, .v0A = 0xC0, .clut0C = 0x7EF8}, {.x00 = 0xE6, .y02 = 0xE, .w04 = 0x54, .h06 = 0xC, .u08 = 0x38, .v0A = 0xCC, .clut0C = 0x7EF8}, {.x00 = 0xE7, .y02 = 0x1C, .w04 = 0xC, .h06 = 0xC, .u08 = 0x90, .v0A = 0xA8, .clut0C = 0x7EF8}, {.x00 = 0xA, .y02 = 0x1C, .w04 = 0xC, .h06 = 0xC, .u08 = 0x90, .v0A = 0xA8, .clut0C = 0x7EF8}, {.x00 = 0xF4, .y02 = 0x70, .w04 = 0x48, .h06 = 8, .u08 = 0xA0, .v0A = 0x30, .clut0C = 0x7893}, {.x00 = 8, .y02 = 0x20, .w04 = 8, .h06 = 8, .u08 = 0x50, .v0A = 0x40, .clut0C = 0x78CC}, {.x00 = 0x78, .y02 = 0x50, .w04 = 8, .h06 = 8, .u08 = 0x78, .v0A = 8, .clut0C = 0x78CC}};
HudSpriteDef14 Rect16RowDefaultPresets[12] = {{.x00 = 0xC, .y02 = 0xE, .w04 = 0x38, .h06 = 0xC, .v0A = 0xCC, .clut0C = 0x7EF8}, {.x00 = 0x40, .y02 = 0xE, .w04 = 0x14, .h06 = 0xC, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0xC, .y02 = 0x1A, .w04 = 0x18, .h06 = 0x18, .v0A = 0xD8, .clut0C = 0x7EF8}, {.x00 = 0x20, .y02 = 0x1A, .w04 = 8, .h06 = 0x18, .u08 = 0xB8, .v0A = 0xC0, .clut0C = 0x7EF8}, {.x00 = 0x2C, .y02 = 0x20, .w04 = 0x10, .h06 = 0x10, .u08 = 0xC0, .v0A = 0xC8, .clut0C = 0x7EF8}, {.x00 = 0xE8, .y02 = 0x18, .w04 = 0x2C, .h06 = 8, .u08 = 0x40, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0xC, .y02 = 0x8E, .w04 = 0x38, .h06 = 0xC, .v0A = 0xCC, .clut0C = 0x7EF8}, {.x00 = 0x40, .y02 = 0x8E, .w04 = 0x14, .h06 = 0xC, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0xC, .y02 = 0x9A, .w04 = 0x18, .h06 = 0x18, .v0A = 0xD8, .clut0C = 0x7EF8}, {.x00 = 0x20, .y02 = 0x9A, .w04 = 8, .h06 = 0x18, .u08 = 0xB8, .v0A = 0xC0, .clut0C = 0x7EF8}, {.x00 = 0x2C, .y02 = 0xA0, .w04 = 0x10, .h06 = 0x10, .u08 = 0xC0, .v0A = 0xC8, .clut0C = 0x7EF8}, {.x00 = 0xE8, .y02 = 0x98, .w04 = 0x2C, .h06 = 8, .u08 = 0x40, .v0A = 0xB4, .clut0C = 0x7EF8}};
/* The Japanese release's values (SLPS-01800). */
static const HudSpriteDef14 Rect16RowDefaultPresetsJp[12] = {{.x00 = 0xC, .y02 = 0xE, .w04 = 0x38, .h06 = 0xC, .v0A = 0xCC, .clut0C = 0x7EF8}, {.x00 = 0x40, .y02 = 0xE, .w04 = 0x14, .h06 = 0xC, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0xC, .y02 = 0x1A, .w04 = 0x18, .h06 = 0x18, .v0A = 0xD8, .clut0C = 0x7EF8}, {.x00 = 0x20, .y02 = 0x1A, .w04 = 8, .h06 = 0x18, .u08 = 0xB8, .v0A = 0xC0, .clut0C = 0x7EF8}, {.x00 = 0x2C, .y02 = 0x20, .w04 = 0x10, .h06 = 0x10, .u08 = 0xC0, .v0A = 0xC8, .clut0C = 0x7EF8}, {.x00 = 0xE8, .y02 = 0x18, .w04 = 0x2C, .h06 = 8, .u08 = 0x40, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0xC, .y02 = 0x86, .w04 = 0x38, .h06 = 0xC, .v0A = 0xCC, .clut0C = 0x7EF8}, {.x00 = 0x40, .y02 = 0x86, .w04 = 0x14, .h06 = 0xC, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0xC, .y02 = 0x92, .w04 = 0x18, .h06 = 0x18, .v0A = 0xD8, .clut0C = 0x7EF8}, {.x00 = 0x20, .y02 = 0x92, .w04 = 8, .h06 = 0x18, .u08 = 0xB8, .v0A = 0xC0, .clut0C = 0x7EF8}, {.x00 = 0x2C, .y02 = 0x98, .w04 = 0x10, .h06 = 0x10, .u08 = 0xC0, .v0A = 0xC8, .clut0C = 0x7EF8}, {.x00 = 0xE8, .y02 = 0x90, .w04 = 0x2C, .h06 = 8, .u08 = 0x40, .v0A = 0xB4, .clut0C = 0x7EF8}};
