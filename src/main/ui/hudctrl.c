#include "rr/release.h"
#include "rr/widescreen.h"
#include "game/system/pad.h"
#include "scratchpad.h"
#include "rr/frame_storage_types.h"
#include "rr/controller_hud.h"
#include "game/ui/hudctrl.h"


void DrawRaceMinimap(s32 arg0) {
    s32 var_s2;
    s32 temp_hi;
    void *var_a1;
    s32 var_s0;
    s32 var_s0_2;
    GpuDmaTag *var_s3;
    s32 var_s4;
    s32 var_v0;
    RaceVehicle *temp_a0;

    var_s4 = 0;
    if (arg0 == 0) {
        var_s3 = CurrentDrawBuf->buffer.ordering70.tags;
        if (LinkHostFlag >= 0) {
            var_s2 = (s16)((u16)LinkHostFlag ^ 1);
        } else {
            var_s2 = 0;
        }
    } else {
        var_s3 = CurrentDrawBuf->buffer.secondaryOrderingB70.tags;
        if (arg0 == 1) {
            var_s4 = RR_PAL_JP(-0x80, -0x78);
        }
        if (LinkHostFlag >= 0) {
            {s32 step=((((u16)LinkHostFlag^1)<<16)>>15);step=step-1;var_s2=arg0+step;}
        } else {
            var_s2 = arg0 - 1;
        }
    }
    var_a1 = RR_GPU_SCRATCH->cursor;
    var_s0 = 0;
    if (NumCars > 0) {
        s32 yBase;UiUvRect *const glyph=&HudCheckeredFlagUv;
        do {
            temp_hi = (var_s2 + var_s0) % (s16) NumCars;
            temp_a0 = CarTable[temp_hi];
            if ((u16)temp_a0->flags1E4 & 0x10) {
                u16 clut=(temp_hi<<6)+0x1A16;
                s32 yNumerator=temp_a0->position20.words[2]; s32 xNumerator=temp_a0->position20.words[0];
                s32 xValue;
                xValue=WIDE_HUD_LEFT(xNumerator/1150+4);
                yBase=var_s4+RR_PAL_JP(242, 226);
                var_v0=yBase-yNumerator/1150;
                var_a1 = AddSpritePrim(var_s3, var_a1, xValue, var_v0-4, glyph, clut, 0);
            }
            var_s0 += 1;
        } while (var_s0 < NumCars);
    }
    var_s0_2 = 0;
    if ((arg0 + 1) > 0) {
        s16 originX = (s16)WIDE_HUD_LEFT(8);
        do {
            s32 x = originX;
            var_v0 = var_s4 + RR_PAL_JP(0x3E, 0x36);
            if (var_s0_2 <= 0) {
                var_v0 = var_s4 + RR_PAL_JP(0xBE, 0xAE);
            }
            var_a1 = GameQueueSprite(var_s3, var_a1, x, var_v0, 0x34, 0x34, 0x4C, 0x6C, 0x1BD6);
            var_s0_2 += 1;
        } while (var_s0_2 < (arg0 + 1));
    }
    RR_GPU_SCRATCH->cursor = AddDrawModePrim(var_s3, var_a1, 0x25);
}

void DrawSteeringIndicator(GpuDmaTag *ordering, s32 layout) {
    HudTriangle triangle;
    s16 *outputPoint;
    s32 status;

    void *modeCursor;
    void *spriteCursor;
    s32 parity;
    void *cursor;
    s32 shapeOffset;
    s32 i;
    s32 tickX;
    s32 player;
    s32 baselineY;
    s32 offsetX;
    s32 widthBase,heightBase,color;
    u16 *toggle;
    u8 *shapeY;
    u8 *shapeX;
    u8 encodedX;
    ControllerState5C *input;
    ControllerState5C *inputBase;
    u8 *shapeBase;

    player = 0;
    baselineY = RR_PAL_JP(0x70, 0x68);
    if (layout != 1) {
        baselineY = RR_PAL_JP(0xF0, 0xE0);
        player = layout == 2;
    }
    inputBase = PlayerControllers;

    input = inputBase + player;
    status = input->protocol10;
    if (status == 2) {
        if (input->edges08.fields.pressed & 0x100) {
            toggle = (u16 *)&HudLapFlashToggles[player];
            *toggle ^= 1;
        }
        i = GetControllerWarning(player);
        if (i > 0) {
            if ((ScreenFrame & 3) >= 2U || i == status) {
                modeCursor = AddDrawModePrim(ordering, (GpuDrawModePacket *)RR_GPU_SCRATCH->cursor, 0x25);
                RR_GPU_SCRATCH->cursor = modeCursor;
                spriteCursor = GameQueueSprite(ordering, (SpritePacket *)RR_GPU_SCRATCH->cursor, 0x80, baselineY - 0xA, 0x44, 8, 0x80, 0x40, 0x1B16);
                RR_GPU_SCRATCH->cursor = spriteCursor;
                RR_GPU_SCRATCH->cursor = AddDrawModePrim(ordering, spriteCursor, 0x25);
            }
        }
        i = 0;
        if (HudLapFlashToggles[player] != 0) {
            cursor = RR_GPU_SCRATCH->cursor;
            widthBase = 2;
            heightBase = 6;
            color = 255;
            tickX = 0x63;
            do {
                parity = i & 1;
                offsetX = widthBase - parity;
                cursor = AddTilePrim(ordering, cursor, tickX, baselineY | (parity * 2), offsetX, heightBase - (parity * 4), color, color, color);
                i += 1;
                tickX += 0xF;
            } while (i < 9);
            offsetX = (PlayerControllers[player].steering3C * 0x3C) / 127;
            if (offsetX >= 0x3D) {
                offsetX = 0x3C;
                goto drawNeedle;
            }
            i = 0;
            if (offsetX < -0x3C) {
                offsetX = -0x3C;
drawNeedle:
                i = 0;
            }
            shapeBase = HudRotatingQuadVerts;
            shapeOffset = 1;
            outputPoint = triangle.half;
            shapeX = shapeBase;
            do {
                shapeY = shapeBase + shapeOffset;
                shapeOffset += 2;
                encodedX = *shapeX;
                shapeX += 2;
                outputPoint[0] = (s16) (offsetX + (s16)((s8) encodedX + 0xA0));
                i += 1;
                outputPoint[1] = (s16) ((s8) *shapeY + baselineY + 5);
                outputPoint += 2;
            } while (i < 3);
            RR_GPU_SCRATCH->cursor = (u8 *)EmitPolyF3(ordering, cursor, triangle.word[0], triangle.word[1], triangle.word[2], HudIndicatorTriColor, 0);
        }
    }
}

void DrawSplitScreenDivider(int a0)
{
    GpuTilePacket *p;
    p = (GpuTilePacket *)RR_GPU_SCRATCH->cursor;
    p->tag.length = 3;
    p->code = 0x60;
    if (a0 == 0) {
        p->y = RR_PAL_JP(0x7E, 0x76);
        p->w = 0x140;
        p->x = 0;
        p->h = 4;
    } else {
        p->x = 0x9E;
        p->w = 4;
        p->y = 0;
        p->h = RR_PAL_JP(0x100, 0xF0);
    }
    p->r = 8;
    p->g = 8;
    p->b = 8;
    RR_GPU_LINK(&CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0], p);
    p++;
    RR_GPU_SCRATCH->cursor = (u8 *)p;
}

/* Initial values of the globals this module owns. */
UiUvRect HudCheckeredFlagUv = {.u = 0xE8, .v = 0x68, .w = 8, .h = 8};
u8 HudRotatingQuadVerts[6] = {0xFC, 0, 0, 0xFC, 4};
u8 HudIndicatorTriColor[3] = {0xFF, 0xFF, 0xFF};
s16 HudLapFlashToggles[2] = {1, 1};
