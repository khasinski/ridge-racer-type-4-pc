#include "rr/release.h"
#include "rr/cpu_time.h"
#include "rr/gpu_scratch_types.h"
#include "rr/frame_storage_types.h"
#include "rr/gpu.h"
#include "rr/animated_glyph.h"
#include "game/gfx/fontdraw.h"
#include "psyq/libgpu/primattr.h"
#include "psyq/libgpu/settrans.h"
#include "rr/player_status.h"

extern unsigned char SmallFontGlyphAdvances[];

void DrawAnimatedGlyph(s32 x, s32 y, s32 glyph, s32 clut, s16 *animation);

void DrawBigFontString(s32 a0, s32 a1, u8 *a2) {
    SpritePacket *s3, *s2;
    s32 c, a, v, row, col, u, vv;
    s3 = (SpritePacket *)RR_GPU_SCRATCH->cursor;
    s2 = s3;
    for (; *a2 != 0; a0 += 10) {
        a = *a2 - 0x20;
        a2++;
        if (a != 0) {
            a = FontAtlasTileCoords[a * 2] + (FontAtlasTileCoords[a * 2 + 1] << 5);
            row = a / 21;
            col = a % 21;
            u = col * 12;
            vv = row * 24 + 0x24;
            SetSprt(s3);
            s2->w = 12;
            s2->h = 24;
            SetShadeTex(s3, 1);
            SetSemiTrans(s3, 1);
            s2->x = a0;
            s2->y = a1;
            s2->u = u;
            s2->v = vv;
            s2->clut = 0x5B20;
            AddPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s2++);
            s3++;
        }
    }
    SetDrawMode(s3, 0, 1, 0x58, 0);
    AddPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], s3);
    RR_GPU_SCRATCH->cursor = (u8 *)((GpuDrawModePacket *)s3 + 1);
}

void *AddTextSprites(GpuDmaTag *ordering, void *cursor, s32 screenX, s32 screenY,
                             const char *text, s32 clut, s32 spacing, s32 color)
{
    int tileU;
    int tileV;
    int glyph;
    int brightness;
    u32 margin;
    SpritePacket *packet;
    SpritePacket *emitted;
    brightness = color >> 1;
    RR_CPU_CYCLES(RR_CYCLES_FONT_DRAW);
    if ((*text) != 0) {
        packet = cursor;
        do {
            glyph = (u8)*text - 0x20;
            text++;
            if (glyph != 0) {
                RR_CPU_CYCLES(RR_CYCLES_FONT_GLYPH);
                tileU = FontAtlasTileCoords[glyph * 2] << 3;
                tileV = FontAtlasTileCoords[(glyph * 2) + 1] << 4;
                SetSprt(cursor);
                if (brightness == 0) {
                    SetShadeTex(cursor, 1);
                } else {
                    packet->r = brightness;
                    packet->g = brightness;
                    packet->b = brightness;
                }
                SetSemiTrans(cursor, color & 1);
                packet->x = screenX;
                packet->y = screenY;
                packet->w = 8;
                packet->u = tileU;
                packet->v = tileV;
                packet->h = 0x10;
                packet->clut = clut;
                emitted = packet;
                packet++;
                cursor = (SpritePacket *)cursor + 1;
                AddPrim(ordering, emitted);
            }
            if (spacing == 1) {
                margin = 2;
                screenX = screenX + margin + SmallFontGlyphAdvances[((tileU / 8) + ((tileV / 16) * 32)) % 54];
            } else if (spacing == 2) {
                u32 advance = SmallFontGlyphAdvances[((tileU / 8) + ((tileV / 16) * 32)) % 54];
                screenX += advance;
            } else {
                screenX += 6U;
            }
        } while (*text != 0);
    }
    return cursor;
}

void DrawAnimatedGlyph(s32 x, s32 y, s32 glyph, s32 clut, s16 *animation) {
    s16 nextTimer;
    s32 tileU,shadowBrightness;
    s32 drawV;
    s32 yStep;
    s32 row;
    s32 curve;
    s16 displacement;
    s32 pass;
    s32 baseV;
    s16 tileV;
    SpritePacketSignedClut *sprites;
    u8 *cursor;
    SpritePacketSignedClut *linkPacket;

    cursor = RR_GPU_SCRATCH->cursor;
    /* The font page holds 32 glyphs 8 wide a row in the PAL release and 21
     * glyphs 12 wide in the Japanese one; the rows past the tenth start 16
     * lines lower. */
    s32 perRow = RR_PAL_JP(32, 21);
    s32 glyphW = RR_PAL_JP(8, 12);
    row = glyph/perRow;
    tileU=(glyph-row*perRow)*glyphW;
    baseV = row * 0x18;
    tileV = baseV;
    if (glyph >= perRow * 10) {
        tileV = baseV + 0x10;
    }
    displacement = 0;
    pass = 0;
    shadowBrightness = 8;
    sprites = (SpritePacketSignedClut *)cursor;
    /* The original retains a shadow branch, but runs only pass zero. */
    do {
        SetSprt(cursor);
        if (pass == 1) {
            sprites->r = shadowBrightness;
            sprites->g = shadowBrightness;
            sprites->b = shadowBrightness;
        } else {
            displacement = 0;
            if (*animation < 0x80) {
                sprites->r = (u8) *animation;
                sprites->g = (u8) *animation;
                sprites->b = (u8) *animation;
                nextTimer = (u16) *animation + 8;
                *animation = nextTimer;
                curve = *animation/8;
                curve *= curve;
                if(curve<0)curve+=7;
                curve >>= 3;
                if (*animation >= 0x31) {
                    displacement = curve - 4;
                } else {
                    displacement = -curve;
                }
            } else {
                s32 brightness = 128;
                sprites->r = brightness;
                sprites->g = brightness;
                sprites->b = brightness;
            }
        }
        linkPacket=sprites;
        if ((s16)displacement > 0) {
            displacement = 0;
        }
        cursor += sizeof(SpritePacketSignedClut);
        sprites->x = (s16)(displacement + ((u32)x + pass));
        yStep = pass * 2;
        sprites->u = (s8)tileU;
        sprites->v = tileV;
        sprites->y = (s16)((u32)y + yStep);
        sprites->w = glyphW;
        sprites->h = 0x18;
        sprites->clut = clut;
        sprites++;
        AddPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], linkPacket);
        pass++;
    } while (pass <= 0);
    drawV = (s16) tileV;
    if (drawV < 0) {
        drawV += 0xFF;
    }
    SetDrawMode(cursor, 0, 1, ((drawV & 0x100) >> 4) | 0x2F, 0);
    AddPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor);
    RR_GPU_SCRATCH->cursor = cursor + sizeof(GpuDrawModePacket);
}

void DrawAnimatedText(int arg0, int arg1, MenuTextState *p, int arg3, int arg4) {
    int s7 = 0, s4 = 0, s2 = 0, s3;
    unsigned s6;
    unsigned s0;
    unsigned short *s1;
    unsigned a2, v0; int v1;
    if (arg3 == 1) {
        p->s12 = p->bA - p->bB;
        s6 = p->bA - p->bB;
    } else {
        s6 = p->s12;
    }
    s1 = p->p;
    s0 = 0;
    if (s6 != 0) {
        do {
            a2 = *s1++;
            s3 = 0;
            if (a2 & 0x1000) {
                v0 = a2 & 0xFFF;
                if (v0 == 0) {
                    s2 = s3;
                    s4 += 0x18;
                } else {
                    a2 = v0;
                    s3 = 1;
                    goto L_call;
                }
            } else {
                v1 = a2;
                if ((a2 & 0xFE00) == 0x6000) {
                    s0 -= 1;
                    a2 = *s1++;
                    switch (v1 & 0xFFFF) {
                    default:
                        s0 += 1;
                        continue;
                    case 0x6005:
                        p->sE = a2 & 0xFF;
                        break;
                    case 0x6003:
                        p->b1C = a2;
                        break;
                    case 0x6004:
                        p->s14 = (signed char)a2;
                        if (s0 != s6 - 2) {
                            s0++;
                            continue;
                        }
                        s0 = s6 - 1;
                        break;
                    }
                } else {
                    if ((int)(a2 & 0xFE00) <= 0) {
                        if (a2 == 0) {
                            s2 += RR_PAL_JP(7, 11);
                        } else {
                        L_call:
                            DrawAnimatedGlyph(arg0 + (short)s2, arg1 + (short)s4, a2 - 1, arg4, p->animation28[s7].halves);
                            s2 += RR_PAL_JP(7, 11);
                            if (s3 != 0) {
                                s2 = 0;
                                s4 += 0x1A;
                            }
                            s7 += 1;
                        }
                    }
                }
            }
            s0 += 1;
        } while (s0 < s6);
    }
}

/* Initial values of the globals this module owns. */
u8 FontAtlasTileCoords[192] = {0, 0, 4, 1, 9, 1, 7, 1, 6, 1, 9, 1, 6, 1, 0xA, 1, 0xC, 1, 0xD, 1, 0xB, 1, 0xD, 1, 0xC, 1, 0xD, 1, 0xE, 1, 0xF, 1, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0, 0x10, 1, 0x12, 1, 0x13, 1, 0x17, 1, 0x14, 1, 5, 1, 7, 1, 0xA, 0, 0xB, 0, 0xC, 0, 0xD, 0, 0xE, 0, 0xF, 0, 0x10, 0, 0x11, 0, 0x12, 0, 0x13, 0, 0x14, 0, 0x15, 0, 0x16, 0, 0x17, 0, 0x18, 0, 0x19, 0, 0x1A, 0, 0x1B, 0, 0x1C, 0, 0x1D, 0, 0x1E, 0, 0x1F, 0, 0, 1, 1, 1, 2, 1, 3, 1, 0x15, 1, 0x1C, 1, 0x16, 1, 0x1E, 1, 0x1F, 1, 0x1A, 1, 0x10, 1, 0x11, 1, 0x12, 1, 0x1A, 1, 0xE, 0, 0xF, 0, 0x10, 0, 0x1E, 1, 0x1E, 1, 0x13, 0, 0x14, 0, 0x15, 0, 0x16, 0, 0x19, 1, 0x17, 1, 0x19, 0, 0x1A, 0, 0x1B, 1, 0x17, 1, 0x18, 1, 0x1E, 0, 0x1F, 0, 0, 1, 1, 1, 2, 1, 3, 1, 0x1B, 1, 0x1C, 1, 0x1D, 1, 0x1E, 1, 0x1F, 1};
unsigned char SmallFontGlyphAdvances[64] = {5, 3, 5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 3, 5, 6, 5, 7, 5, 5, 5, 5, 6, 5, 5, 5, 5, 8, 5, 5, 4, 2, 4, 6, 7, 4, 2, 3, 7, 7, 4, 4, 4, 2, 4, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
