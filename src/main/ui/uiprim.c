#include "scratchpad.h"
#include "rr/frame_storage_types.h"
#include "rr/gpu_line.h"
#include "rr/gpu_scratch_types.h"
#include "rr/frame_environment.h"
#include "rr/ui_primitives.h"
#include "rr/fade_sprite.h"

/* Host POLY_G4 packet (len 8): four color/XY pairs after the tag. */
typedef struct {
    GpuDmaTag tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 r1, g1, b1, pad1;
    s16 x1, y1;
    u8 r2, g2, b2, pad2;
    s16 x2, y2;
    u8 r3, g3, b3, pad3;
    s16 x3, y3;
} UiGouraudQuadPacket;

static UiGouraudQuadPacket *BeginUiGouraudQuad(const GpuRectangleFields *rect)
{
    UiGouraudQuadPacket *primitive = (UiGouraudQuadPacket *)RR_GPU_SCRATCH->cursor;
    primitive->tag.length = 8;
    primitive->code = 0x3A;
    primitive->x0 = rect->x;
    primitive->y0 = rect->y;
    primitive->x1 = rect->x + rect->w;
    primitive->y1 = rect->y;
    primitive->x2 = rect->x;
    primitive->y2 = rect->y + rect->h;
    primitive->x3 = rect->x + rect->w;
    primitive->y3 = rect->y + rect->h;
    return primitive;
}

static void EndUiGouraudQuad(UiGouraudQuadPacket *primitive, s32 depth)
{
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[depth], primitive);
    RR_GPU_SCRATCH->cursor = (u8 *)(primitive + 1);
}

void AddVerticalGradientRect(GpuRectangleFields *rect, GpuColorCommand *top, GpuColorCommand *bottom, s32 depth)
{
    UiGouraudQuadPacket *primitive = BeginUiGouraudQuad(rect);
    primitive->r0 = top->r;
    primitive->g0 = top->g;
    primitive->b0 = top->b;
    primitive->r1 = top->r;
    primitive->g1 = top->g;
    primitive->b1 = top->b;
    primitive->r2 = bottom->r;
    primitive->g2 = bottom->g;
    primitive->b2 = bottom->b;
    primitive->r3 = bottom->r;
    primitive->g3 = bottom->g;
    primitive->b3 = bottom->b;
    EndUiGouraudQuad(primitive, depth);
}

void AddGouraudRect(GpuRectangleFields *rect, GpuColorCommand *color0, GpuColorCommand *color1,
    GpuColorCommand *color2, GpuColorCommand *color3, s32 depth)
{
    UiGouraudQuadPacket *primitive = BeginUiGouraudQuad(rect);
    primitive->r0 = color0->r;
    primitive->g0 = color0->g;
    primitive->b0 = color0->b;
    primitive->r1 = color1->r;
    primitive->g1 = color1->g;
    primitive->b1 = color1->b;
    primitive->r2 = color2->r;
    primitive->g2 = color2->g;
    primitive->b2 = color2->b;
    primitive->r3 = color3->r;
    primitive->g3 = color3->g;
    primitive->b3 = color3->b;
    EndUiGouraudQuad(primitive, depth);
}

void AddLineF2Prim(const s16 coordinates[4], const u8 rgb[3], s32 blend, s32 depth)
{
    GpuLineF2Packet *primitive = (GpuLineF2Packet *)RR_GPU_SCRATCH->cursor;
    primitive->tag.length = 3;
    primitive->code = blend ? 0x42 : 0x40;
    primitive->r = rgb[0];
    primitive->g = rgb[1];
    primitive->b = rgb[2];
    primitive->x0 = coordinates[0];
    primitive->y0 = coordinates[1];
    primitive->x1 = coordinates[2];
    primitive->y1 = coordinates[3];
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[depth], primitive);
    primitive++;
    RR_GPU_SCRATCH->cursor = (u8 *)primitive;
}
