#ifndef RR_RING_RENDERER_H
#define RR_RING_RENDERER_H

#include "rr/layout.h"
#include "rr/gpu.h"
#include "rr/matrix_words.h"
#include "rr/object_transform_types.h"

typedef struct { s16 x, y, z, pad; } RingVertex8;
typedef struct { s16 x, y; } RR_PACKED RingScreen4;

/* Geometry view of the scratch tail after the transform service has run. */
typedef struct {
    s16 origin[4];
    RingScreen4 screen[4];
    u8 reserved18[12];
    RingVertex8 vectors[4];
    u8 red, green, blue, reserved47;
} RingGeometry48;
/* The transform is prepared in this same tail before ring geometry reuses it. */
typedef union {
    ObjectTransform38 transform;
    RingGeometry48 geometry;
} RingScratch48;

/* Flat quad (POLY_F4) emitted per ring segment. */
typedef struct {
    GpuDmaTag tag;
    u8 red, green, blue, code;
    s16 xy[4][2];
} RingPacket24;

void DrawRing(s32 *position, s32 radius);

RR_ASSERT_SIZE(RingVertex8, 8);
RR_ASSERT_SIZE(RingScreen4, 4);
RR_ASSERT_SIZE(RingScratch48, 0x48);
RR_ASSERT_OFFSET(RingGeometry48, screen, 8);
RR_ASSERT_OFFSET(RingGeometry48, vectors, 0x24);
RR_ASSERT_OFFSET(RingGeometry48, red, 0x44);
RR_ASSERT_SIZE(RingPacket24, RR_GPU_PACKET_SIZE(20));
RR_ASSERT_OFFSET(RingPacket24, xy, sizeof(GpuDmaTag) + 4);

#endif
