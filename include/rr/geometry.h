#ifndef RR_GEOMETRY_H
#define RR_GEOMETRY_H

#include "rr/layout.h"

/* Small fixed-width vectors and table pairs shared by matching C units. */

typedef struct {
    s16 x, y;
} Vec2s;

/* GPU/GTE screen coordinates have signed halfword and packed-word views. */
typedef union {s32 word; Vec2s xy;} ScreenPosition;
RR_ASSERT_SIZE(ScreenPosition, 4);


typedef struct {s16 x, y, z, pad06;} Vec3sFields8;
/* Two word-aligned GTE input words, including the unused high Z halfword. */
typedef union {
    Vec3sFields8 fields;
    s16 halves[4];
    u32 words[2];
    s32 signedWords[2];
} Vec3s8;

/* Three word-sized components; GTE IR loads consume each word's signed low 16 bits. */
typedef struct {
    s32 x, y, z;
} Vec3i;

/* Full-word libgte vector layout. The fourth word is reserved by the SDK. */
typedef struct {
    s32 x, y, z, pad;
} Vector;

typedef union {
    Vector vector;
    Vec3i xyz;
    s32 words[4];
} VectorWords16;

/* SDK scalar arguments are long, including on LP64 hosts. */
void InitGeom(void);
void SetGeomOffset(long x, long y);
void SetGeomScreen(long distance);

/* SQR hardware helpers return MAC1..MAC3, with shift 12 and shift 0 respectively.
 * Input and output may overlap. GTE IR/MAC registers and FLAG are also modified. */
Vec3i *Square12(const Vec3i *input, Vec3i *output);
Vec3i *Square0(const Vec3i *input, Vec3i *output);

RR_ASSERT_SIZE(Vec3i, 12);
RR_ASSERT_SIZE(Vec3s8, 8);
RR_ASSERT_OFFSET(Vec3sFields8, z, 4);
RR_ASSERT_SIZE(Vector, 16);
RR_ASSERT_SIZE(VectorWords16, 16);
RR_ASSERT_OFFSET(Vector, z, 8);
RR_ASSERT_OFFSET(Vec3i, y, 4);
RR_ASSERT_OFFSET(Vec3i, z, 8);

typedef struct PairS32 {
    s32 a, b;
} PairS32;

typedef struct PairS16 {
    s16 a, b;
} PairS16;

RR_ASSERT_SIZE(Vec2s, 0x4);
RR_ASSERT_SIZE(PairS32, 0x8);
RR_ASSERT_SIZE(PairS16, 0x4);

#endif
