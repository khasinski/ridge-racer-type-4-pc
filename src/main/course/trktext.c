#include "rr/cpu_time.h"
#include "game/system/pad.h"
#include "game/car/carstat.h"
#include "types.h"
#include "rr/frame_storage_types.h"
#include "scratchpad.h"
#include "rr/track_lateral.h"
#include "rr/text_sprites.h"
#include "rr/gpu.h"
#include "rr/tables.h"
#include "game/course/trktext.h"
#include "rr/bios_devices.h"
#include "rr/controller_hud.h"
#include "rr/vehicle_drive.h"
#include "rr/vab_data.h"
#include "game/data/trkld1.h"

typedef struct {
    u32 otIndex00;
    u32 shadeTex04;
    u32 semiTrans08;
    u16 tpageMode0C;
    u16 unk0E;
    u8 r10, g11, b12, unk13;
    u16 x14, y16, w18, h1A;
    u8 u1C, unk1D, v1E, unk1F;
    u8 uw20, unk21, vh22, unk23;
    u16 tpageX24, tpageY26;
    u16 clutX28, clutY2A;
} TexturedQuadDesc2C;

typedef struct {
    u16 x, y, w, h;
} RectU16;

typedef struct {
    u8 r, g, b;
} ColorRGB8;

extern unsigned char FontGlyphMap[96];
extern unsigned short PadDefaultButtonPresets[8][8];
extern s32 PadDeadzoneTable[4];
extern int CourseAssetGroupIds[8];
extern int CourseAssetBaseFileIndex;
extern unsigned short SavedCourseIndex;
extern s32 D_800A027C[8];
extern short D_800A02C4[2], D_800A02C8[2], D_800A02CC[2], D_800A02D0[2], D_800A02D4[4];

/* The PS1 renderer reads glyphs as FontAtlasRowTable[c] (0x800A015C, i.e.
 * FontGlyphMap - 0x20) and widths as FontGlyphWidths[glyph] with no range
 * check, so control characters, bytes >= 0x80 and unmapped glyph values
 * read whatever the linker placed around the font tables. Those neighbours
 * belong to other modules here; this window maps every reachable PS1 offset
 * (from 0x800A015C) back onto its owner so such reads see the same bytes,
 * including the live race-loader state words. */
typedef struct {
    u16 offset;
    u16 size;
    const void *data;
} FontDataSpan;

static const FontDataSpan FontDataWindow[] = {
    {0x000, 0x10, PadDefaultButtonPresets[7]}, /* 0x800A015C */
    {0x010, 0x10, PadDeadzoneTable},           /* 0x800A016C */
    {0x020, 0x60, FontGlyphMap},               /* 0x800A017C */
    {0x080, 0x3C, FontGlyphWidths},            /* 0x800A01DC */
    {0x0BC, 4, &RaceSoundTrackLoadState},      /* 0x800A0218 */
    {0x0C0, 4, &RaceLoadStepIndex},
    {0x0C4, 4, &RaceLoadState},
    {0x0C8, 4, &RaceLoadSubState},
    {0x0CC, 0x54, CourseVabLoadConfigs},       /* 0x800A0228 */
    {0x120, 0x20, D_800A027C},
    {0x140, 0x20, CourseAssetGroupIds},        /* 0x800A029C */
    {0x160, 4, &CourseAssetBaseFileIndex},
    {0x164, 2, &SavedCourseIndex},             /* 0x800A02C0, then 2 pad */
    {0x168, 4, D_800A02C4},
    {0x16C, 4, D_800A02C8},
    {0x170, 4, D_800A02CC},
    {0x174, 4, D_800A02D0},
    {0x178, 8, D_800A02D4},                    /* ends 0x800A02DC */
};

static u8 FontDataByte(u32 offset) {
    u32 i;
    for (i = 0; i < sizeof(FontDataWindow) / sizeof(FontDataWindow[0]); i++) {
        const FontDataSpan *span = &FontDataWindow[i];
        if (offset >= span->offset && offset < (u32)span->offset + span->size) {
            return ((const u8 *)span->data)[offset - span->offset];
        }
    }
    return 0;
}

/* FontAtlasRowTable[c] / FontGlyphMap[c - 0x20] for any byte c. */
static u8 FontGlyphForChar(u8 c) {
    return c >= 0x20 && c < 0x80 ? FontGlyphMap[c - 0x20] : FontDataByte(c);
}

/* FontGlyphWidths[glyph] for any glyph byte. */
static u8 FontGlyphWidth(u8 glyph) {
    return glyph < 0x3C ? FontGlyphWidths[glyph] : FontDataByte(0x80 + glyph);
}

s32 GetTrackNodeHeading(s32 node, s32 lane);
void DrawFontString(s32 x, s32 y, const u8 *text, s32 color, s32 bank);
int DrawFontStringCentered(int a0, int a1, unsigned char *s, int a3, int a4);
int DrawFontStringCenteredOffset(int a0, int a1, unsigned char *s, int a3, int a4);
void DrawFontStringRight(int a0, int a1, unsigned char *s, int a3, int a4);
void *EmitTexturedQuadPrim(TexturedQuadDesc2C *a0);
void InitTexturedQuadDesc(TexturedQuadDesc2C *p);
void AddHGradientRectPrim(RectU16 *arg0, ColorRGB8 *arg1, ColorRGB8 *arg2, s32 arg3);
void AddSlantedGradientRect(unsigned short *a0, int a1, unsigned char *a2, unsigned char *a3, int a4);

int GetMappedPadButtons(int i) {
    ControllerState5C *t = &PlayerControllers[i];
    switch (t->input00.fields.type) {
    case 0x41:
    case 0x73:
    default:
        return PlayerControllers[i].edges08.fields.pressed & PadButtonMap[i].standard[6];
    case 0x23:
        return t->edges08.fields.pressed & PadButtonMap[i].calibrated[6];
    case 0xE3:
        return t->edges08.fields.pressed & PadButtonMap[i].wheel[6];
    }
}

s32 GetTrackNodeHeading(s32 node, s32 lane)
{
    int delta[6];
    int previous;
    TrackLaneOffset8 *currentOffset, *previousOffset;
    TrackSample3C *previousSample, *currentSample;
    previous = (node - 1) % TrackNodeCount;
    if (previous < 0) previous += TrackNodeCount;
    switch (lane) {
    case 0:
        currentOffset = RaceContext.laneOffsets54C + node;
        previousOffset = RaceContext.laneOffsets54C + previous;
        previousSample = TrackNodes + previous;
        currentSample = TrackNodes + node;
        delta[2] = previousSample->road.x + previousOffset->x - (currentSample->road.x + currentOffset->x);
        delta[3] = previousSample->road.z + previousOffset->z - (currentSample->road.z + currentOffset->z);
        break;
    case 10:
        delta[2] = TrackNodes[previous].road.x - TrackNodes[node].road.x;
        delta[3] = TrackNodes[previous].road.z - TrackNodes[node].road.z;
        break;
    }
    return (s16)ratan2(delta[3], delta[2]);
}

/* One 12x12 sprite per glyph from the font atlas (21 glyphs per row at
 * v=188); color 255 draws raw texture. Unmapped characters advance 4. */
void DrawFontString(s32 x, s32 y, const u8 *text, s32 color, s32 bank) {
    SpritePacket *packet = (SpritePacket *)RR_GPU_SCRATCH->cursor;
    u32 current_x = x;
    GpuDmaTag *ordering = &CurrentDrawBuf->buffer.ordering70.tags[bank];

    while (*text != 0) {
        s32 glyph = FontGlyphForChar(*text);
        text++;
        if (glyph != 255) {
            s32 width = FontGlyphWidth(glyph);
            if (color == 255) {
                packet->code = 0x65;
            } else {
                packet->code = 0x66;
                packet->r = color;
                packet->g = color;
                packet->b = color;
            }
            packet->x = current_x;
            packet->y = y;
            packet->u = (glyph % 21) * 12;
            packet->v = (glyph / 21) * 12 + 188;
            packet->clut = 0x7F78;
            packet->w = width;
            packet->h = 12;
            RR_GPU_ADD_PRIM(ordering, packet, 4);
            packet++;
            current_x += width;
        } else {
            current_x += 4U;
        }
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
}

int DrawFontStringCentered(int a0, int a1, unsigned char *s, int a3, int a4) {
    int sum = 0;
    unsigned char *s0 = s;
    if (*s0 != 0) {
        do {
            unsigned char c = FontGlyphForChar(*s);
            s++;
            if (c != 0xFF) {
                sum += FontGlyphWidth(c);
            } else {
                sum += 4;
            }
        } while (*s != 0);
    }
    DrawFontString(0xA0 - sum / 2, a1, s0, a3, a4);
    return sum;
}

int DrawFontStringCenteredOffset(int a0, int a1, unsigned char *s, int a3, int a4) {
    int sum = 0;
    unsigned char *s0 = s;
    if (*s0 != 0) {
        do {
            unsigned char c = FontGlyphForChar(*s);
            s++;
            if (c != 0xFF) {
                sum += FontGlyphWidth(c);
            } else {
                sum += 4;
            }
        } while (*s != 0);
    }
    DrawFontString(a0 + (0xA0 - sum / 2), a1, s0, a3, a4);
    return sum;
}

void DrawFontStringRight(int a0, int a1, unsigned char *s, int a3, int a4) {
    int sum = 0;
    unsigned char *s0 = s;
    if (*s0 != 0) {
        do {
            unsigned char c = FontGlyphForChar(*s);
            s++;
            if (c != 0xFF) {
                sum += FontGlyphWidth(c);
            } else {
                sum += 4;
            }
        } while (*s != 0);
    }
    DrawFontString(a0 - sum, a1, s0, a3, a4);
}

void AddSetTexpagePrim(int a0) {
    GpuCommandPacket *p = (GpuCommandPacket *)RR_GPU_SCRATCH->cursor;
    p->command = 0xE100023E;
    RR_GPU_ADD_PRIM(&CurrentDrawBuf->buffer.ordering70.tags[a0], p, 1);
    p++;
    RR_GPU_SCRATCH->cursor = (u8 *)p;
}

void *EmitTexturedQuadPrim(TexturedQuadDesc2C *a0)
{
    u16 *new_var;
    TexturedQuadDesc2C *new_var2;
    int new_var3;
    GpuPolyFT4Packet *p = (GpuPolyFT4Packet *)RR_GPU_SCRATCH->cursor;
    u16 a1;
    p->code = 0x2C;
    if (a0->semiTrans08 != 0) {
        p->code |= 2;
    } else {
        p->code &= ~2;
    }
    if (a0->shadeTex04 != 0) {
        p->code |= 1;
    } else {
        p->code &= ~1;
    }
    p->r0 = a0->r10;
    p->g0 = a0->g11;
    p->b0 = a0->b12;
    p->x0 = a0->x14;
    new_var2 = a0;
    new_var3 = 1;
    p->y0 = a0->y16;
    p->x1 = (u16)((new_var2->x14 + new_var3) + a0->w18);
    p->y1 = a0->y16;
    p->x2 = a0->x14;
    a1 = 1;
    p->y2 = (u16)((a0->y16 + a1) + a0->h1A);
    p->x3 = (u16)(((short)a0->x14 + 1) + a0->w18);
    new_var = &a0->y16;
    p->y3 = (u16)((*new_var + a1) + a0->h1A);
    p->u0 = a0->u1C;
    p->v0 = a0->v1E;
    p->u1 = a0->u1C + a0->uw20;
    p->v1 = a0->v1E;
    p->u2 = a0->u1C;
    p->v2 = a0->v1E + a0->vh22;
    p->u3 = a0->u1C + a0->uw20;
    p->v3 = a0->v1E + a0->vh22;
    a1 = a0->tpageY26;
    p->tpage = (((a0->tpageMode0C & 3) << 7) | (((a1 & 0x100) << 16) >> 20) | ((a0->tpageX24 & 0x3FF) >> 6)) | ((a1 & 0x200) * 4);
    p->clut = (a0->clutY2A << 6) | ((a0->clutX28 >> 4) & 0x3F);
    RR_GPU_ADD_PRIM(&CurrentDrawBuf->buffer.ordering70.tags[a0->otIndex00], p, 9);
    p++;
    RR_GPU_SCRATCH->cursor = (u8 *)p;
    return p;
}

void InitTexturedQuadDesc(TexturedQuadDesc2C *p) {
    PsyqBzero((char *)p, sizeof(*p));
    p->shadeTex04 = 1;
    p->r10 = 0x80;
    p->g11 = 0x80;
    p->b12 = 0x80;
}

void AddHGradientRectPrim(RectU16 *arg0, ColorRGB8 *arg1, ColorRGB8 *arg2, s32 arg3) {
    PolyG4Packet *temp_t0;

    temp_t0 = (PolyG4Packet *)RR_GPU_SCRATCH->cursor;
    temp_t0->code = 0x38;
    temp_t0->x0 = arg0->x;
    temp_t0->y0 = arg0->y;
    temp_t0->x1 = arg0->x + arg0->w;
    temp_t0->y1 = arg0->y;
    temp_t0->x2 = arg0->x;
    temp_t0->y2 = arg0->y + arg0->h;
    temp_t0->x3 = arg0->x + arg0->w;
    temp_t0->y3 = arg0->y + arg0->h;
    temp_t0->r0 = arg1->r;
    temp_t0->g0 = arg1->g;
    temp_t0->b0 = arg1->b;
    temp_t0->r1 = arg2->r;
    temp_t0->g1 = arg2->g;
    temp_t0->b1 = arg2->b;
    temp_t0->r2 = arg1->r;
    temp_t0->g2 = arg1->g;
    temp_t0->b2 = arg1->b;
    temp_t0->r3 = arg2->r;
    temp_t0->g3 = arg2->g;
    temp_t0->b3 = arg2->b;
    RR_GPU_ADD_PRIM(&CurrentDrawBuf->buffer.ordering70.tags[arg3], temp_t0, 8);
    RR_GPU_SCRATCH->cursor = (u8 *)(temp_t0 + 1);
}

void AddSlantedGradientRect(unsigned short *a0, int a1, unsigned char *a2, unsigned char *a3, int a4) {
    RR_CPU_CYCLES(RR_CYCLES_ADD_SLANTED_GRADIENT);
    PolyG4Packet *p = (PolyG4Packet *)RR_GPU_SCRATCH->cursor;
    p->code = 0x3A;
    if (a1 == 0) {
        p->x0 = a0[0];
        p->y0 = a0[1];
        p->x1 = a0[0] + a0[2];
        p->y1 = a0[1] + (short)a0[3] / 2;
        p->x2 = a0[0];
        p->y2 = a0[1] + a0[3];
        p->x3 = a0[0] + a0[2];
        p->y3 = a0[1] + a0[3];
    } else {
        p->x0 = a0[0];
        p->y0 = a0[1];
        p->x1 = a0[0] + a0[2];
        p->y1 = a0[1];
        p->x2 = a0[0];
        p->y2 = a0[1] + a0[3];
        p->x3 = a0[0] + a0[2];
        p->y3 = a0[1] + (short)a0[3] / 2;
    }
    p->r0 = a2[0];
    p->g0 = a2[1];
    p->b0 = a2[2];
    p->r1 = a2[0];
    p->g1 = a2[1];
    p->b1 = a2[2];
    p->r2 = a3[0];
    p->g2 = a3[1];
    p->b2 = a3[2];
    p->r3 = a3[0];
    p->g3 = a3[1];
    p->b3 = a3[2];
    RR_GPU_ADD_PRIM(&CurrentDrawBuf->buffer.ordering70.tags[a4], p, 8);
    p++;
    RR_GPU_SCRATCH->cursor = (u8 *)p;
}

/* Initial values of the globals this module owns. */
unsigned char FontGlyphMap[96] = {0xFF, 0x24, 0x29, 0xFF, 0xFF, 0xFF, 0x26, 0x28, 0x2C, 0x2D, 0xFF, 0xFF, 0xFF, 0x2B, 0xFF, 0x3B, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xFF, 0xFF, 0x2C, 0xFF, 0x2D, 0x25, 0x27, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
u8 FontGlyphWidths[60] = {7, 6, 8, 8, 8, 8, 8, 8, 7, 8, 9, 8, 8, 8, 7, 7, 8, 8, 5, 8, 9, 7, 0xA, 8, 8, 8, 9, 8, 8, 8, 8, 8, 0xB, 8, 9, 8, 5, 7, 0xA, 0xA, 4, 4, 5, 6, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6};
