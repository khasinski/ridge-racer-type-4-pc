#include "rr/release.h"
/* RODATA 0x800103e0 0x1c */
#include "rr/widescreen.h"
#include "types.h"
#include "rr/frame_storage_types.h"
#include "scratchpad.h"
#include "rr/hud_rotating_quad.h"
#include "rr/trig.h"
#include "rr/hud_number.h"
#include "game/ui/window2.h"
#include "game/ui/window1.h"
#include "rr/race_setup.h"
#include "game/ui/badge1.h"
#include "rr/frame_loop.h"
static const HudSpriteDef14 GlyphRowDefaultPresetsJp[4];
static const HudSpriteDef14 UiRowDefaultPresetsJp[3];

/* The bank-0/bank-1 ordering tables the HUD number targets point at:
 * full-screen layouts use each bank's primary table, split-screen layouts
 * the secondary one. (PS1 names: HudPrimaryDigitBuffer =
 * DrawBuffers[0]+0x70, ViewportRenderEnvLeft = DrawBuffers[0]+0xB70, ...) */
extern HudSpriteDef14 UiRowDefaultPresets[];
extern HudSpriteDef14 GlyphRowDefaultPresets[];

void InitHudNumberTargets(int a0) {
    NumberTarget *targets = HudNumberTargets;
    switch (a0) {
    case 0: case 1: case 5: break;
    case 2: case 3: case 4: case 6:
        if (a0 == 4 && LinkPlayerCount == 2) {
        } else if (a0 != 6) {
            goto splitScreen;
        }
    }
    targets[0].heads[0] = DrawBuffers[0].buffer.ordering70.tags;
    targets[0].heads[1] = DrawBuffers[1].buffer.ordering70.tags;
    targets[0].number3.x = 0x106;
    targets[0].number3.y = RR_PAL_JP(0xE6, 0xD6);
    targets[0].digit.x = 0x122;
    targets[0].digit.y = RR_PAL_JP(0xDC, 0xCC);
    targets[0].x = 0x112;
    targets[0].y = RR_PAL_JP(0xCE, 0xBE);
    return;
splitScreen:
    targets[0].heads[0] = DrawBuffers[0].buffer.secondaryOrderingB70.tags;
    targets[0].heads[1] = DrawBuffers[1].buffer.secondaryOrderingB70.tags;
    targets[0].number3.x = 0xFC;
    targets[0].number3.y = RR_PAL_JP(0x58, 0x50);
    targets[0].digit.x = 0x128;
    targets[0].digit.y = RR_PAL_JP(0x4A, 0x42);
    targets[0].x = 0xFC;
    targets[0].y = RR_PAL_JP(0x68, 0x60);
    targets[1].heads[0] = DrawBuffers[0].buffer.secondaryOrderingB70.tags;
    targets[1].heads[1] = DrawBuffers[1].buffer.secondaryOrderingB70.tags;
    targets[1].number3.x = 0xFC;
    targets[1].number3.y = RR_PAL_JP(0xD8, 0xC8);
    targets[1].digit.x = 0x122;
    targets[1].digit.y = RR_PAL_JP(0xCA, 0xBA);
    targets[1].x = 0xFC;
    targets[1].y = RR_PAL_JP(0xE8, 0xD8);
}

/* Split layouts (a0 >= 2) draw four (two for a0 == 3) border sprites into
 * the secondary table, full-screen layouts three into the primary table. */
void DrawViewportBorderFrames(int a0) {
    SpritePacket *s0;
    GpuDmaTag *s2;
    const HudSpriteDef14 *preset;
    int s3, s5;

    s0 = (SpritePacket *)RR_GPU_SCRATCH->cursor;
    if (a0 >= 2) {
        s2 = &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0];
        s5 = 4;
        if (a0 == 3) s5 = 2;
        preset = RR_PAL_JP(GlyphRowDefaultPresets, GlyphRowDefaultPresetsJp);
        for (s3 = 0; s3 < s5; s3++) {
            InitHudSpritePacket(s0, &preset[s3]);
            s0->x = (s16)WIDE_HUD_RIGHT(s0->x);
            RR_GPU_LINK(s2, s0);
            s0++;
        }
    } else {
        s2 = &CurrentDrawBuf->buffer.ordering70.tags[0];
        preset = RR_PAL_JP(UiRowDefaultPresets, UiRowDefaultPresetsJp);
        for (s3 = 0; s3 < 3; s3++) {
            InitHudSpritePacket(s0, &preset[s3]);
            /* Original compares against 3, which this branch never sees. */
            if (a0 == 3) s0->y -= RR_PAL_JP(0x80, 0x78);
            s0->x = (s16)WIDE_HUD_RIGHT(s0->x);
            RR_GPU_LINK(s2, s0);
            s0++;
        }
    }
    RR_GPU_SCRATCH->cursor = AddDrawModePrim(s2, (GpuDrawModePacket *)s0, 5);
}

void DrawHudRotatingQuad(s32 phase,s32 index) {
    HudQuadColor colors[4];
    u32 sine;
    s32 angle;
    u32 cosine;
    s32 i;
    HudQuadCorner *corner;
    NumberTarget *target;
    GpuDmaTag *head;
    HudQuadEmission *packet;
    u32 x;
    u32 y;
    colors[0].rgb.r=0xD0;
    colors[0].rgb.g=24;
    colors[0].rgb.b=0x30;
    colors[1].rgb.r=0x68;
    colors[1].rgb.g=12;
    colors[1].rgb.b=24;
    colors[2].rgb.r=0x8A;
    colors[2].rgb.g=0x12;
    colors[2].rgb.b=0x24;
    colors[3].rgb.r=0x34;
    colors[3].rgb.g=6;
    colors[3].rgb.b=12;
    angle=(s32)(phase*0xA00U)/10000+0xE00;
    sine=rsin(angle);
    cosine=rcos(angle);
    corner=UiBoxCornerOffsets;
    target=&HudNumberTargets[index];
    packet=(HudQuadEmission *)RR_GPU_SCRATCH->cursor;
    x=(u16)WIDE_HUD_RIGHT(target->x);y=(u16)target->y;
    head=target->heads[DrawBufIndex];
    packet->r=0xB0;packet->g=0xB0;packet->b=0;
    for(i=0;i<4;i++,corner++) {
        packet->vertices[i].x=(s16)(x+((s32)(cosine*corner->x-sine*corner->y)/4096));
        packet->vertices[i].y=(s16)(y+((s32)(sine*corner->x+cosine*corner->y)/4096));
        /* Vertex i's color word is the color of vertex i+1 (G4 layout). */
        packet->vertices[i].color=colors[i];
    }
    packet->code=0x38;
    RR_GPU_ADD_PRIM(head,packet,8);
    RR_GPU_SCRATCH->cursor=(u8 *)packet+RR_FIELD_OFFSET(HudQuadEmission,vertices[3].color);
}

/* 12x16 HUD digit sprite (texture row V 0xF0, U glyph*12) linked at the head
 * of ordering; returns the next cursor. */
void *EmitHudGlyph12(GpuDmaTag *arg0, void *arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5) {
    SpritePacket *p = arg1;
    p->tag.length = 4;
    p->code = 0x65;
    p->w = 0xC;
    p->h = 0x10;
    p->x = arg2;
    p->y = arg3;
    p->v = 0xF0;
    p->u = (u8)(arg4 * 0xC);
    p->clut = (u16)arg5;
    RR_GPU_LINK(arg0, p);
    return p + 1;
}

/* Same as EmitHudGlyph12 from the second glyph set (U offset 0x78). */
void *EmitHudGlyph12Alt(GpuDmaTag *arg0, void *arg1, s16 arg2, s16 arg3, s32 arg4, s32 arg5) {
    SpritePacket *p = arg1;
    p->tag.length = 4;
    p->code = 0x65;
    p->w = 0xC;
    p->h = 0x10;
    p->x = arg2;
    p->y = arg3;
    p->v = 0xF0;
    p->u = (u8)(arg4 * 0xC + 0x78);
    p->clut = (u16)arg5;
    RR_GPU_LINK(arg0, p);
    return p + 1;
}

void DrawHudNumber3(int a0, int a1)
{
    NumberTarget *target = &HudNumberTargets[a1];
    short x = (short)WIDE_HUD_RIGHT(target->number3.x);
    short y = target->number3.y;
    GpuDmaTag *s1 = target->heads[DrawBufIndex];
    int h, tn, on;
    void *cursor;
    h = a0 / 100;
    tn = (a0 / 10) - h * 10;
    on = a0 % 10;
    cursor = EmitHudGlyph12(s1, RR_GPU_SCRATCH->cursor, x, y, h, 0x7EFA);
    cursor = EmitHudGlyph12(s1, cursor, x + 8, y, tn, 0x7EFA);
    cursor = EmitHudGlyph12(s1, cursor, x + 0x10, y, on, 0x7EFA);
    RR_GPU_SCRATCH->cursor = AddDrawModePrim(s1, cursor, 5);
}

/* Initial values of the globals this module owns. */
HudSpriteDef14 UiRowDefaultPresets[3] = {{.x00 = 0x120, .y02 = 0xEA, .w04 = 0x18, .h06 = 0xC, .u08 = 0x28, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0x125, .y02 = 0xB6, .w04 = 0x14, .h06 = 0x20, .v0A = 0x48, .clut0C = 0x7F3B}, {.x00 = 0xEC, .y02 = 0xAC, .w04 = 0x4C, .h06 = 0x3C, .v0A = 0x6C, .clut0C = 0x7E78}};
/* The Japanese release's values (SLPS-01800). */
static const HudSpriteDef14 UiRowDefaultPresetsJp[3] = {{.x00 = 0x120, .y02 = 0xDA, .w04 = 0x18, .h06 = 0xC, .u08 = 0x28, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0x125, .y02 = 0xA6, .w04 = 0x14, .h06 = 0x20, .v0A = 0x48, .clut0C = 0x7F3B}, {.x00 = 0xEC, .y02 = 0x9C, .w04 = 0x4C, .h06 = 0x3C, .v0A = 0x6C, .clut0C = 0x7E78}};
HudSpriteDef14 GlyphRowDefaultPresets[4] = {{.x00 = 0x115, .y02 = 0x5C, .w04 = 0x18, .h06 = 0xC, .u08 = 0x28, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0x11E, .y02 = 0x6C, .w04 = 0x14, .h06 = 0xC, .u08 = 0x14, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0x115, .y02 = 0xDC, .w04 = 0x18, .h06 = 0xC, .u08 = 0x28, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0x11E, .y02 = 0xEC, .w04 = 0x14, .h06 = 0xC, .u08 = 0x14, .v0A = 0xB4, .clut0C = 0x7EF8}};
/* The Japanese release's values (SLPS-01800). */
static const HudSpriteDef14 GlyphRowDefaultPresetsJp[4] = {{.x00 = 0x115, .y02 = 0x54, .w04 = 0x18, .h06 = 0xC, .u08 = 0x28, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0x11E, .y02 = 0x64, .w04 = 0x14, .h06 = 0xC, .u08 = 0x14, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0x115, .y02 = 0xCC, .w04 = 0x18, .h06 = 0xC, .u08 = 0x28, .v0A = 0xB4, .clut0C = 0x7EF8}, {.x00 = 0x11E, .y02 = 0xDC, .w04 = 0x14, .h06 = 0xC, .u08 = 0x14, .v0A = 0xB4, .clut0C = 0x7EF8}};
HudQuadCorner UiBoxCornerOffsets[4] = {{.x = -32, .y = 1}, {.x = -32, .y = -1}, {.x = 2, .y = 2}, {.x = 2, .y = -2}};
NumberTarget HudNumberTargets[2];
