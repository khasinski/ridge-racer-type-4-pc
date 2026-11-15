#ifndef RR_GPU_LINE_H
#define RR_GPU_LINE_H
#include "rr/gpu.h"
typedef struct {
    GpuDmaTag tag;
    u8 r,g,b,code;
    s16 x0,y0,x1,y1;
} GpuLineF2Packet;
RR_ASSERT_SIZE(GpuLineF2Packet, RR_GPU_PACKET_SIZE(12));
RR_ASSERT_OFFSET(GpuLineF2Packet, x0, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(GpuLineF2Packet, x1, sizeof(GpuDmaTag) + 8);
GpuLineF2Packet *GameQueueTileTrans(GpuDmaTag *ordering, GpuLineF2Packet *packet,
    s32 x0, s32 y0, s32 x1, s32 y1, s32 red, s32 green, s32 blue);
GpuLineF2Packet *AddLineF2SemiTrans(GpuDmaTag *ordering, GpuLineF2Packet *packet,
    s32 x0, s32 y0, s32 x1, s32 y1, s32 red, s32 green, s32 blue);
#endif
