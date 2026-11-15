#ifndef RR_TEXTURED_PANEL_H
#define RR_TEXTURED_PANEL_H
#include "rr/animated_sprite_grid.h"

/* POLY_FT4: flat-color textured quadrilateral with four UV coordinate pairs. */
typedef struct {
    GpuDmaTag tag;
    u8 r,g,b,code;
    s16 x0,y0;
    u8 u0,v0;
    u16 clut;
    s16 x1,y1;
    u8 u1,v1;
    u16 tpage;
    s16 x2,y2;
    u8 u2,v2;
    u16 pad1E;
    s16 x3,y3;
    u8 u3,v3;
    u16 pad26;
} PolyFT4Packet;

/* Emits a raw-texture panel at (32,112)..(288,368). Signed index division
 * selects texture-page/vertical coordinates. Requires a four-byte-aligned destination
 * with sizeof(PolyFT4Packet) writable bytes; links at ordering entry 1. RGB bytes and
 * the last two UV padding halfwords retain their old contents. */
void EmitTexturedPanel(s32 index);
RR_ASSERT_SIZE(PolyFT4Packet, RR_GPU_PACKET_SIZE(36));
RR_ASSERT_OFFSET(PolyFT4Packet, code, sizeof(GpuDmaTag) + 3);
RR_ASSERT_OFFSET(PolyFT4Packet, x0, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(PolyFT4Packet, clut, sizeof(GpuDmaTag) + 10);
RR_ASSERT_OFFSET(PolyFT4Packet, x1, sizeof(GpuDmaTag) + 12);
RR_ASSERT_OFFSET(PolyFT4Packet, tpage, sizeof(GpuDmaTag) + 18);
RR_ASSERT_OFFSET(PolyFT4Packet, x2, sizeof(GpuDmaTag) + 20);
RR_ASSERT_OFFSET(PolyFT4Packet, pad1E, sizeof(GpuDmaTag) + 26);
RR_ASSERT_OFFSET(PolyFT4Packet, x3, sizeof(GpuDmaTag) + 28);
RR_ASSERT_OFFSET(PolyFT4Packet, pad26, sizeof(GpuDmaTag) + 34);
#endif
