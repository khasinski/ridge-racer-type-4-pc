#include "rr/release.h"
#include "rr/frame_interp.h"
/* RODATA 0x800100e4 0x20 */
#include "rr/widescreen.h"
#include "scratchpad.h"
#include "rr_asm.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "psyq/libgpu/gpudisp.h"
#include "game/data/ldscn5.h"
#include "game/gfx/sky.h"
#include "rr/audio_cd_controller.h"
#include "rr/menu_overlay_runtime.h"

/* Sky and scenery mesh records. Vertex fields are indices into an array of
 * eight-byte SVECTORs; colors and UV words are GPU payload words. The last
 * byte of each record biases the ordering-table entry. */
typedef struct {
    int v01;
    short v2;
    char b6;
    signed char b7;
    int c0;
    int c1;
    int c2;
} SkyTri;

typedef struct {
    int v01;
    int v23;
    int c0;
    int c1;
    int c2;
    int c3;
    char b18;
    signed char b19;
    short pad1A;
} SkyQuad17528;

typedef struct {
    int v01;
    int v23;
    int c0;
    int uv0;
    int uv1;
    int uv23;
    char b18;
    signed char b19;
    short pad1A;
} SkyQuad177E4;

typedef struct {
    int v01;
    short v2;
    short pad06;
    int c0;
    int c1;
    int c2;
    int uv0;
    int uv1;
    union {
        int uv2;
        struct {
            char unk0[3];
            signed char b1F;
        } b;
    } u1C;
} SkyTri17A90;

typedef struct {
    unsigned short v0, v1, v2;
    unsigned short n0, n1, n2;
    int unk0C;
    unsigned char b10;
    signed char b11;
    short unk12;
} SkyLitTri14;

typedef struct {
    unsigned short v0, v1, v2, v3;
    unsigned short n0, n1, n2, n3;
    int rgb10;
    unsigned char b14;
    signed char b15;
    short unk16;
} SkyLitQuad18;

typedef struct {
    unsigned short v0, v1, v2, v3;
    unsigned short n0, n1, n2, n3;
    int rgb10;
    int uv0;
    int uv1;
    union {
        int word;
        struct { short lo, hi; } h;
    } uv2;
    unsigned char b20;
    signed char b21;
    short unk22;
} SkyLitTexQuad24;

/* Scenery work state. On the PS1 this lived at scratchpad+0x7C; it holds
 * vertex pointers, so the host keeps it in an ordinary static. */
typedef struct {
    int ir0;
    int otz;
    const u8 *verts;
    const u32 *v0;
    const u32 *v1;
    const u32 *v2;
    const u32 *v3;
    int v3_0;
    int v3_1;
    union {
        struct {
            int sxy0;
            int sxy1;
            int sxy2;
            int sxy3;
        } w;
        struct {
            short sx0, sy0;
            short sx1, sy1;
            short sx2, sy2;
            short sx3, sy3;
        } s;
    } u;
} SkyCtx;

/* Lit sky work state (PS1 scratchpad+0x7C, a separate use of that area). */
typedef struct {
    const u8 *verts70;
    int countE4;
    int otzE8;
    int mac0EC;
    int mac0F0;
    int rgbF4;
    int pageF8;
    int clutFC;
    struct { int x, y, z; } ir100;
    struct { int x, y, z; } ir10C;
    struct { int x, y, z; } ir118;
    struct { int x, y, z; } ir124;
    union {
        struct {
            int sxy0;
            int sxy1;
            int sxy2;
            int sxy3;
        } w;
        struct {
            short sx0, sy0;
            short sx1, sy1;
            short sx2, sy2;
            short sx3, sy3;
        } s;
    } sxy140;
} SkyLitCtx;

/* Offset tables (TrackSkyTable, SkyCommandListPtr): a count word followed by
 * 12-byte records whose first and third words are signed 32-bit offsets from
 * the table start to the vertex array and to the command stream. */
typedef struct {
    s32 verts;
    s32 unk04;
    s32 commands;
} SkyOffsetRecord;

/* Host GPU packets (tag followed by the PS1 payload words). */
typedef struct {
    GpuDmaTag tag;
    u32 color0, xy0, color1, xy1, color2, xy2;
} SkyPolyG3;
typedef struct {
    GpuDmaTag tag;
    u32 color0, xy0, color1, xy1, color2, xy2, color3, xy3;
} SkyPolyG4;
typedef struct {
    GpuDmaTag tag;
    u32 color, xy0, uv0, xy1, uv1, xy2, uv2, xy3, uv3;
} SkyPolyFT4;
typedef struct {
    GpuDmaTag tag;
    u32 color0, xy0, uv0, color1, xy1, uv1, color2, xy2, uv2;
} SkyPolyGT3;
typedef struct {
    GpuDmaTag tag;
    u32 color0, xy0, uv0, color1, xy1, uv1, color2, xy2, uv2, color3, xy3, uv3;
} SkyPolyGT4;
/* E1 draw mode, merged-packet NOP and a textured sprite. */
typedef struct {
    GpuDmaTag tag;
    u32 drawMode;
    u32 nop;
    u32 color;
    u32 position;
    u32 uvClut;
    u32 size;
} SkySpritePacket;

#define SKY_SCENERY_OT_BASE 702 /* PS1 byte offset 0xAF8 */
/* Bottom of the screen for "some vertex on screen" tests: 256 lines in the
 * PAL release, 240 in the Japanese one (the lit sky works in half lines). */
#define SKY_Y_LIMIT RR_SCREEN_BOTTOM
#define SKY_LIT_Y_LIMIT RR_PAL_JP(0x200, 0x1E0)
#define SKY_LIT_OT_BASE 0x80    /* PS1 byte offset 0x200 */
#define SKY_VERTEX(verts, index) ((const u32 *)((verts) + ((index) << 3)))
#define SKY_DEPTH_SHIFT (*(s32 *)(SCRATCHPAD + 0x60))

extern char UnknownSkyModeFmt[];
extern short *CourseSkyKeyframePtrs[];
extern short *CourseSceneryRangePtrs[];
extern char SkyDefaultRotMatrix[];
void SetGeomOffset(s32 x, s32 y);

static SkyCtx SkySceneryWork;
static SkyLitCtx SkyLitWork;

static GpuDmaTag *SkyOrderEntry(int index) {
    return RR_GPU_SCRATCH->order + index;
}

static const SkyOffsetRecord *SkyRecord(const u8 *table, int index) {
    return (const SkyOffsetRecord *)(table + 4 + index * (int)sizeof(SkyOffsetRecord));
}

static void SkyLoadVector(unsigned base, const u32 *v) {
    Psyz_GteDataWrite(base, v[0]);
    Psyz_GteDataWrite(base + 1, v[1]);
}

static void SkyLoadTriangle(const SkyCtx *c) {
    SkyLoadVector(0, c->v0);
    SkyLoadVector(2, c->v1);
    SkyLoadVector(4, c->v2);
}

/* DPCT over three colors at depth IR0; results in RGB0..RGB2. */
static void SkyDepthCue3(const SkyCtx *c, int c0, int c1, int c2) {
    Psyz_GteDataWrite(8, (u32)c->ir0);
    Psyz_GteDataWrite(6, (u32)c0);
    Psyz_GteDataWrite(20, (u32)c0);
    Psyz_GteDataWrite(21, (u32)c1);
    Psyz_GteDataWrite(22, (u32)c2);
    Psyz_GteCommand(0x0F8002A);
}

/* DPCS of one color; result in RGB2. IR0 must already be loaded. */
static void SkyDepthCue1(int color) {
    Psyz_GteDataWrite(6, (u32)color);
    Psyz_GteCommand(0x0780010);
}

static SkyTri *SkyEmitTriFT3(SkyTri *a0, SkyCtx *a1, int a2) {
    int v0 = a0->v01;
    int a3 = a0->v2;
    const u8 *verts = a1->verts;
    int v1 = v0 >> 16;
    SkyPolyG3 *pkt;

    v0 &= 0xFFFF;
    a1->v0 = SKY_VERTEX(verts, v0);
    a1->v1 = SKY_VERTEX(verts, v1);
    a1->v2 = SKY_VERTEX(verts, a3);
    pkt = (SkyPolyG3 *)RR_GPU_SCRATCH->cursor;
    do {
        SkyLoadTriangle(a1);
        v0 = a0[1].v01;
        RR_GTE_CMD_1NOP(0x4A280030);
        a3 = a0[1].v2;
        v1 = v0 >> 16;
        v0 &= 0xFFFF;
        a1->v0 = SKY_VERTEX(verts, v0);
        a1->v1 = SKY_VERTEX(verts, v1);
        a1->v2 = SKY_VERTEX(verts, a3);
        RR_GTE_OP(0x4B58002D);
        RR_GTE14_SWC2_OTZ(&a1->otz);
        if (a1->otz > 0) {
            RR_GTE_SWC2_SXYZ(&a1->u.w.sxy0);
            RR_SXY_COPY(pkt->xy0, a1->u.w.sxy0);
            RR_SXY_COPY(pkt->xy1, a1->u.w.sxy1);
            RR_SXY_COPY(pkt->xy2, a1->u.w.sxy2);
            v0 = a1->u.s.sx0;
            v1 = a1->u.s.sx1;
            a3 = a1->u.s.sx2;
            if ((v0 >= WIDE_SCREEN_LEFT || v1 >= WIDE_SCREEN_LEFT || a3 >= WIDE_SCREEN_LEFT) &&
                (v0 < WIDE_SCREEN_RIGHT || v1 < WIDE_SCREEN_RIGHT || a3 < WIDE_SCREEN_RIGHT)) {
                v0 = a1->u.s.sy0;
                v1 = a1->u.s.sy1;
                a3 = a1->u.s.sy2;
                if ((v0 >= 0 || v1 >= 0 || a3 >= 0) &&
                    (v0 < SKY_Y_LIMIT || v1 < SKY_Y_LIMIT || a3 < SKY_Y_LIMIT)) {
                    SkyDepthCue3(a1, a0->c0, a0->c1, a0->c2);
                    pkt->color0 = Psyz_GteDataRead(20);
                    pkt->color1 = Psyz_GteDataRead(21);
                    pkt->color2 = Psyz_GteDataRead(22);
                    RR_GPU_ADD_PRIM(SkyOrderEntry(SKY_SCENERY_OT_BASE + a0->b7), pkt, 6);
                    INTERP_TAG(pkt, a0, 0);
                    pkt++;
                }
            }
        }
        a0++;
        a2--;
    } while (a2 > 0);
    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
    return a0;
}

/* RTPT of the first three quad vertices; keep a copy of the fourth. */
static void SkyProjectQuadFirstThree(SkyCtx *a1) {
    SkyLoadTriangle(a1);
    Psyz_GteCommand(0x0280030);
    a1->v3_0 = (int)a1->v3[0];
    a1->v3_1 = (int)a1->v3[1];
}

static void SkySetQuadVertices(SkyCtx *a1, const u8 *verts, int v01, int v23) {
    a1->v0 = SKY_VERTEX(verts, v01 & 0xFFFF);
    a1->v1 = SKY_VERTEX(verts, v01 >> 16);
    a1->v2 = SKY_VERTEX(verts, v23 & 0xFFFF);
    a1->v3 = SKY_VERTEX(verts, v23 >> 16);
}

static SkyQuad17528 *SkyEmitQuadG4(SkyQuad17528 *a0, SkyCtx *a1, int a2) {
    const u8 *verts = a1->verts;
    int v1, t0, a3, t1;
    SkyPolyG4 *pkt;

    SkySetQuadVertices(a1, verts, a0->v01, a0->v23);
    pkt = (SkyPolyG4 *)RR_GPU_SCRATCH->cursor;
    do {
        SkyProjectQuadFirstThree(a1);
        SkySetQuadVertices(a1, verts, a0[1].v01, a0[1].v23);
        RR_GTE_OP(0x4B58002D);
        RR_GTE14_SWC2_OTZ(&a1->otz);
        if (a1->otz > 0) {
            RR_GTE_SWC2_SXYZ(&a1->u.w.sxy0);
            RR_GTE_LWC2_PAIR_AT(0, 0, 1, 4, &a1->v3_0);
            RR_GTE14_RTPS();
            RR_SXY_COPY(pkt->xy0, a1->u.w.sxy0);
            RR_SXY_COPY(pkt->xy1, a1->u.w.sxy1);
            RR_SXY_COPY(pkt->xy2, a1->u.w.sxy2);
            RR_GTE_SWC2_SXY2(&a1->u.w.sxy3);
            RR_GTE_SWC2_SXY2(&pkt->xy3);
            v1 = a1->u.s.sx0;
            t0 = a1->u.s.sx1;
            a3 = a1->u.s.sx2;
            t1 = a1->u.s.sx3;
            if ((v1 >= WIDE_SCREEN_LEFT || t0 >= WIDE_SCREEN_LEFT || a3 >= WIDE_SCREEN_LEFT || t1 >= WIDE_SCREEN_LEFT) &&
                (v1 < WIDE_SCREEN_RIGHT || t0 < WIDE_SCREEN_RIGHT || a3 < WIDE_SCREEN_RIGHT || t1 < WIDE_SCREEN_RIGHT)) {
                v1 = a1->u.s.sy0;
                t0 = a1->u.s.sy1;
                a3 = a1->u.s.sy2;
                t1 = a1->u.s.sy3;
                if ((v1 >= 0 || t0 >= 0 || a3 >= 0 || t1 >= 0) &&
                    (v1 < SKY_Y_LIMIT || t0 < SKY_Y_LIMIT || a3 < SKY_Y_LIMIT || t1 < SKY_Y_LIMIT)) {
                    SkyDepthCue3(a1, a0->c0, a0->c1, a0->c2);
                    pkt->color0 = Psyz_GteDataRead(20);
                    pkt->color1 = Psyz_GteDataRead(21);
                    pkt->color2 = Psyz_GteDataRead(22);
                    SkyDepthCue1(a0->c3);
                    pkt->color3 = Psyz_GteDataRead(22);
                    RR_GPU_ADD_PRIM(SkyOrderEntry(SKY_SCENERY_OT_BASE + a0->b19), pkt, 8);
                    INTERP_TAG(pkt, a0, 0);
                    pkt++;
                }
            }
        }
        a0++;
        a2--;
    } while (a2 > 0);
    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
    return a0;
}

static SkyQuad177E4 *SkyEmitQuadFT4(SkyQuad177E4 *a0, SkyCtx *a1, int a2) {
    const u8 *verts = a1->verts;
    int v1, t0, a3, t1;
    SkyPolyFT4 *pkt;

    SkySetQuadVertices(a1, verts, a0->v01, a0->v23);
    pkt = (SkyPolyFT4 *)RR_GPU_SCRATCH->cursor;
    do {
        SkyProjectQuadFirstThree(a1);
        SkySetQuadVertices(a1, verts, a0[1].v01, a0[1].v23);
        RR_GTE_OP(0x4B58002D);
        RR_GTE14_SWC2_OTZ(&a1->otz);
        if (a1->otz > 0) {
            RR_GTE_SWC2_SXYZ(&a1->u.w.sxy0);
            RR_GTE_LWC2_PAIR_AT(0, 0, 1, 4, &a1->v3_0);
            a3 = a0->uv23;
            RR_GTE_CMD_1NOP(0x4A180001);
            pkt->uv0 = a0->uv0;
            pkt->uv1 = a0->uv1;
            pkt->uv2 = a3;
            pkt->uv3 = a3 >> 16;
            RR_SXY_COPY(pkt->xy0, a1->u.w.sxy0);
            RR_SXY_COPY(pkt->xy1, a1->u.w.sxy1);
            RR_SXY_COPY(pkt->xy2, a1->u.w.sxy2);
            RR_GTE_SWC2_SXY2(&a1->u.w.sxy3);
            RR_GTE_SWC2_SXY2(&pkt->xy3);
            v1 = a1->u.s.sx0;
            t0 = a1->u.s.sx1;
            a3 = a1->u.s.sx2;
            t1 = a1->u.s.sx3;
            if ((v1 >= WIDE_SCREEN_LEFT || t0 >= WIDE_SCREEN_LEFT || a3 >= WIDE_SCREEN_LEFT || t1 >= WIDE_SCREEN_LEFT) &&
                (v1 < WIDE_SCREEN_RIGHT || t0 < WIDE_SCREEN_RIGHT || a3 < WIDE_SCREEN_RIGHT || t1 < WIDE_SCREEN_RIGHT)) {
                v1 = a1->u.s.sy0;
                t0 = a1->u.s.sy1;
                a3 = a1->u.s.sy2;
                t1 = a1->u.s.sy3;
                if ((v1 >= 0 || t0 >= 0 || a3 >= 0 || t1 >= 0) &&
                    (v1 < SKY_Y_LIMIT || t0 < SKY_Y_LIMIT || a3 < SKY_Y_LIMIT || t1 < SKY_Y_LIMIT)) {
                    Psyz_GteDataWrite(8, (u32)a1->ir0);
                    SkyDepthCue1(a0->c0);
                    pkt->color = Psyz_GteDataRead(22);
                    RR_GPU_ADD_PRIM(SkyOrderEntry(SKY_SCENERY_OT_BASE + a0->b19), pkt, 9);
                    INTERP_TAG(pkt, a0, 0);
                    pkt++;
                }
            }
        }
        a0++;
        a2--;
    } while (a2 > 0);
    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
    return a0;
}

static SkyTri17A90 *EmitSkyDepthCuedGT3(SkyTri17A90 *a0, SkyCtx *a1, int a2) {
    int v0 = a0->v01;
    int a3 = a0->v2;
    const u8 *verts = a1->verts;
    int v1 = v0 >> 16;
    SkyPolyGT3 *pkt;

    v0 &= 0xFFFF;
    a1->v0 = SKY_VERTEX(verts, v0);
    a1->v1 = SKY_VERTEX(verts, v1);
    a1->v2 = SKY_VERTEX(verts, a3);
    pkt = (SkyPolyGT3 *)RR_GPU_SCRATCH->cursor;
    do {
        SkyLoadTriangle(a1);
        v0 = a0[1].v01;
        RR_GTE_CMD_1NOP(0x4A280030);
        a3 = a0[1].v2;
        v1 = v0 >> 16;
        v0 &= 0xFFFF;
        a1->v0 = SKY_VERTEX(verts, v0);
        a1->v1 = SKY_VERTEX(verts, v1);
        a1->v2 = SKY_VERTEX(verts, a3);
        pkt->uv0 = a0->uv0;
        pkt->uv1 = a0->uv1;
        pkt->uv2 = a0->u1C.uv2;
        RR_GTE_OP(0x4B58002D);
        RR_GTE14_SWC2_OTZ(&a1->otz);
        if (a1->otz > 0) {
            RR_GTE_SWC2_SXYZ(&a1->u.w.sxy0);
            RR_SXY_COPY(pkt->xy0, a1->u.w.sxy0);
            RR_SXY_COPY(pkt->xy1, a1->u.w.sxy1);
            RR_SXY_COPY(pkt->xy2, a1->u.w.sxy2);
            v0 = a1->u.s.sx0;
            v1 = a1->u.s.sx1;
            a3 = a1->u.s.sx2;
            if ((v0 >= WIDE_SCREEN_LEFT || v1 >= WIDE_SCREEN_LEFT || a3 >= WIDE_SCREEN_LEFT) &&
                (v0 < WIDE_SCREEN_RIGHT || v1 < WIDE_SCREEN_RIGHT || a3 < WIDE_SCREEN_RIGHT)) {
                v0 = a1->u.s.sy0;
                v1 = a1->u.s.sy1;
                a3 = a1->u.s.sy2;
                if ((v0 >= 0 || v1 >= 0 || a3 >= 0) &&
                    (v0 < SKY_Y_LIMIT || v1 < SKY_Y_LIMIT || a3 < SKY_Y_LIMIT)) {
                    SkyDepthCue3(a1, a0->c0, a0->c1, a0->c2);
                    pkt->color0 = Psyz_GteDataRead(20);
                    pkt->color1 = Psyz_GteDataRead(21);
                    pkt->color2 = Psyz_GteDataRead(22);
                    RR_GPU_ADD_PRIM(SkyOrderEntry(SKY_SCENERY_OT_BASE + a0->u1C.b.b1F), pkt, 9);
                    INTERP_TAG(pkt, a0, 0);
                    pkt++;
                }
            }
        }
        a0++;
        a2--;
    } while (a2 > 0);
    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
    return a0;
}

static void DrawTrackSceneryRecords(int a0);

void DrawTrackScenery(int a0) {
    u32 interp = InterpBegin(INTERP_SCENERY, 0);
    DrawTrackSceneryRecords(a0);
    InterpEnd(interp);
}

static void DrawTrackSceneryRecords(int a0) {
    const u8 *table = (const u8 *)TrackSkyTable;
    const SkyOffsetRecord *record;
    const u8 *r;
    SkyCtx *s0 = &SkySceneryWork;
    short *a1;
    int s1;
    int s3;
    int w;
    int k;

    RR_GTE_SETROT((int *)SCRATCH_ROT_MATRIX);
    RR_GTE_CLRTR();
    a1 = CourseSkyKeyframePtrs[CourseIndex];
    if (a1 != 0) {
        while (a1[0] < a0) a1 += 2;
        {
            int pv = a1[-1];
            int a3 = a1[1];
            int sum = pv + a3;
            if (sum == 0) {
                s0->ir0 = 0;
            } else if (sum == 0x2000) {
                s0->ir0 = 0x1000;
            } else {
                int a2 = a1[0];
                if (a0 == a2) {
                    s0->ir0 = a3;
                } else {
                    int d = ((a2 - a0) << 12) / (a2 - a1[-2]);
                    RR_GTE_MTC2_IR0(d);
                    RR_GTE_MTC2_IR1((unsigned short)a1[1]);
                    RR_GTE_CTC2_FCR((unsigned short)a1[-1]);
                    RR_GTE_INTPL();
                    RR_GTE_SWC2_IR1(&s0->ir0);
                }
            }
        }
    } else {
        s0->ir0 = 0;
    }
    k = 0;
    RR_GTE_SETFC(k, k, k);
    a1 = CourseSceneryRangePtrs[CourseIndex];
    if (a1 != 0) {
        while (a1[0] < a0) a1 += 2;
        s3 = a1[1];
    } else {
        s3 = *(const s32 *)table;
    }
    for (s1 = 0; s1 < s3; s1++) {
        record = SkyRecord(table, s1);
        s0->verts = table + record->verts;
        r = table + record->commands;
        while (w = *(const s32 *)r, r += 4, w != 0) {
            k = w & 0xFFFF;
            w = w >> 16;
            switch (k) {
            case 7: r = (const u8 *)SkyEmitTriFT3((SkyTri *)r, s0, w); break;
            case 8: r = (const u8 *)SkyEmitQuadG4((SkyQuad17528 *)r, s0, w); break;
            case 1: r = (const u8 *)SkyEmitQuadFT4((SkyQuad177E4 *)r, s0, w); break;
            case 11: r = (const u8 *)EmitSkyDepthCuedGT3((SkyTri17A90 *)r, s0, w); break;
            default: DebugPrintStub(UnknownSkyModeFmt, k, w); return;
            }
        }
    }
}

/* Environment-map UV from a light-space normal, clamped to 0..0x7F. */
static int SkyEnvUV(int x, int y) {
    int u = (x >> 6) + 0x40;
    int v = (y >> 7) + 0x40;
    if (u < 0) {
        u = 0;
    } else if (u >= 0x80) {
        u = 0x7F;
    }
    if (v < 0) {
        v = 0;
    } else if (v >= 0x80) {
        v = 0x7F;
    }
    return (v << 8) | u;
}

static int SkyLitOnScreen3(const SkyLitCtx *a1) {
    return (a1->sxy140.s.sx0 >= 0 || a1->sxy140.s.sx1 >= 0 || a1->sxy140.s.sx2 >= 0) &&
           (a1->sxy140.s.sx0 < 0x140 || a1->sxy140.s.sx1 < 0x140 || a1->sxy140.s.sx2 < 0x140) &&
           (a1->sxy140.s.sy0 >= 0 || a1->sxy140.s.sy1 >= 0 || a1->sxy140.s.sy2 >= 0) &&
           (a1->sxy140.s.sy0 < SKY_LIT_Y_LIMIT || a1->sxy140.s.sy1 < SKY_LIT_Y_LIMIT || a1->sxy140.s.sy2 < SKY_LIT_Y_LIMIT);
}

static int SkyLitOnScreen4(const SkyLitCtx *a1) {
    return (a1->sxy140.s.sx0 >= 0 || a1->sxy140.s.sx1 >= 0 || a1->sxy140.s.sx2 >= 0 || a1->sxy140.s.sx3 >= 0) &&
           (a1->sxy140.s.sx0 < 0x140 || a1->sxy140.s.sx1 < 0x140 || a1->sxy140.s.sx2 < 0x140 || a1->sxy140.s.sx3 < 0x140) &&
           (a1->sxy140.s.sy0 >= 0 || a1->sxy140.s.sy1 >= 0 || a1->sxy140.s.sy2 >= 0 || a1->sxy140.s.sy3 >= 0) &&
           (a1->sxy140.s.sy0 < SKY_LIT_Y_LIMIT || a1->sxy140.s.sy1 < SKY_LIT_Y_LIMIT || a1->sxy140.s.sy2 < SKY_LIT_Y_LIMIT || a1->sxy140.s.sy3 < SKY_LIT_Y_LIMIT);
}

static SkyLitTri14 *SkyEmitLitTriG3(SkyLitTri14 *a0, SkyLitCtx *a1) {
    int n = a1->countE4;
    SkyPolyGT3 *pkt = (SkyPolyGT3 *)RR_GPU_SCRATCH->cursor;
    int shift = SKY_DEPTH_SHIFT;
    const u8 *verts = a1->verts70;

    a1->rgbF4 = 0x34808080;
    RR_GTE14_LWC2_RGB(&a1->rgbF4);

    while (n-- > 0) {
        RR_GTE_LWC2_V012_3P(verts + (a0->v0 << 3), verts + (a0->v1 << 3), verts + (a0->v2 << 3)); RR_GTE_RTPT_NOP2();
        RR_GTE14_NCLIP();
        RR_GTE14_SWC2_MAC0(&a1->mac0EC);

        if (a1->mac0EC > 0) {
            RR_GTE14_SWC2_SXYZ(&a1->sxy140.w.sxy0);
            RR_SXY_COPY(pkt->xy0, a1->sxy140.w.sxy0);
            RR_SXY_COPY(pkt->xy1, a1->sxy140.w.sxy1);
            RR_SXY_COPY(pkt->xy2, a1->sxy140.w.sxy2);
            RR_GTE_OP(0x4B58002D);

            if (SkyLitOnScreen3(a1)) {
                int z;

                RR_GTE14_SWC2_OTZ(&a1->otzE8);
                RR_GTE14_LWC2_V0_V1_V2(verts + (a0->n0 << 3), verts + (a0->n1 << 3), verts + (a0->n2 << 3));
                RR_GTE14_LIGHT0();
                RR_GTE14_LIGHT1();
                RR_GTE14_LIGHT2();
                pkt->color0 = Psyz_GteDataRead(20);
                pkt->color1 = Psyz_GteDataRead(21);
                pkt->color2 = Psyz_GteDataRead(22);
                RR_GTE_OP(0x4A486012);
                RR_GTE_STORE_IR_MEM(&a1->ir100);
                RR_GTE_OP(0x4A48E012);
                pkt->uv0 = SkyEnvUV(a1->ir100.x, a1->ir100.y) | a1->clutFC;

                RR_GTE_STORE_IR_MEM(&a1->ir10C);
                RR_GTE_OP(0x4A496012);
                pkt->uv1 = SkyEnvUV(a1->ir10C.x, a1->ir10C.y) | a1->pageF8;

                RR_GTE_STORE_IR_MEM(&a1->ir118);
                pkt->uv2 = SkyEnvUV(a1->ir118.x, a1->ir118.y);

                z = a1->otzE8 >> shift;
                if (z > 0 && z < 0x1C0) {
                    RR_GPU_ADD_PRIM(SkyOrderEntry(SKY_LIT_OT_BASE + z + a0->b11), pkt, 9);
                    INTERP_TAG(pkt, a0, 0);
                    pkt++;
                }
            }
        }
        a0++;
    }

    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
    return a0;
}

static SkyLitQuad18 *SkyEmitLitTexQuadGT4(SkyLitQuad18 *a0, SkyLitCtx *a1) {
    int n = a1->countE4;
    SkyPolyGT4 *pkt = (SkyPolyGT4 *)RR_GPU_SCRATCH->cursor;
    int shift = SKY_DEPTH_SHIFT;
    const u8 *verts = a1->verts70;

    a1->rgbF4 = 0x3C808080;
    RR_GTE14_LWC2_RGB(&a1->rgbF4);

    while (n-- > 0) {
        RR_GTE_LWC2_V012_3P(verts + (a0->v0 << 3), verts + (a0->v1 << 3), verts + (a0->v2 << 3)); RR_GTE_RTPT_NOP2();
        RR_GTE14_NCLIP();
        RR_GTE14_SWC2_MAC0(&a1->mac0EC);
        RR_GTE14_SWC2_SXYZ(&a1->sxy140.w.sxy0);
        RR_GTE_RTPS_LDV(verts + (a0->v3 << 3));
        RR_SXY_COPY(pkt->xy0, a1->sxy140.w.sxy0);
        RR_SXY_COPY(pkt->xy1, a1->sxy140.w.sxy1);
        RR_SXY_COPY(pkt->xy2, a1->sxy140.w.sxy2);
        RR_GTE14_NCLIP();
        RR_GTE14_SWC2_SXY2(&a1->sxy140.w.sxy3);
        RR_GTE14_SWC2_MAC0(&a1->mac0F0);

        if (a1->mac0EC > 0 || a1->mac0F0 < 0) {
            RR_GTE14_AVSZ4();
            RR_SXY_COPY(pkt->xy3, a1->sxy140.w.sxy3);

            if (SkyLitOnScreen4(a1)) {
                int z;

                RR_GTE14_SWC2_OTZ(&a1->otzE8);
                RR_GTE14_LWC2_V0_V1_V2(verts + (a0->n0 << 3), verts + (a0->n1 << 3), verts + (a0->n2 << 3));
                RR_GTE14_LIGHT0();
                RR_GTE14_LIGHT1();
                RR_GTE14_LIGHT2();
                pkt->color0 = Psyz_GteDataRead(20);
                pkt->color1 = Psyz_GteDataRead(21);
                pkt->color2 = Psyz_GteDataRead(22);
                RR_GTE_OP(0x4A486012);
                RR_GTE_STORE_IR_MEM(&a1->ir100);
                RR_GTE_OP(0x4A48E012);
                pkt->uv0 = SkyEnvUV(a1->ir100.x, a1->ir100.y) | a1->clutFC;

                RR_GTE_STORE_IR_MEM(&a1->ir10C);
                RR_GTE_OP(0x4A496012);
                pkt->uv1 = SkyEnvUV(a1->ir10C.x, a1->ir10C.y) | a1->pageF8;

                RR_GTE_STORE_IR_MEM(&a1->ir118);
                RR_GTE14_LWC2_V0(verts + (a0->n3 << 3));
                RR_GTE14_LIGHT0();
                RR_GTE14_SWC2_RGB2(&pkt->color3);
                RR_GTE_OP(0x4A486012);
                pkt->uv2 = SkyEnvUV(a1->ir118.x, a1->ir118.y);

                RR_GTE_STORE_IR_MEM(&a1->ir124);
                pkt->uv3 = SkyEnvUV(a1->ir124.x, a1->ir124.y);

                z = a1->otzE8 >> shift;
                if (z > 0 && z < 0x1C0) {
                    RR_GPU_ADD_PRIM(SkyOrderEntry(SKY_LIT_OT_BASE + z + a0->b15), pkt, 12);
                    INTERP_TAG(pkt, a0, 0);
                    pkt++;
                }
            }
        }
        a0++;
    }

    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
    return a0;
}

static SkyLitQuad18 *SkyEmitLitQuadG4(SkyLitQuad18 *a0, SkyLitCtx *a1) {
    int n = a1->countE4;
    SkyPolyG4 *pkt = (SkyPolyG4 *)RR_GPU_SCRATCH->cursor;
    int shift = SKY_DEPTH_SHIFT;
    const u8 *verts = a1->verts70;

    while (n-- > 0) {
        RR_GTE_LWC2_V012_3P(verts + (a0->v0 << 3), verts + (a0->v1 << 3), verts + (a0->v2 << 3)); RR_GTE_RTPT_NOP2();
        RR_GTE14_NCLIP();
        RR_GTE14_SWC2_MAC0(&a1->mac0EC);
        RR_GTE_OP(0x4B58002D);
        RR_GTE14_SWC2_OTZ(&a1->otzE8);
        if (a1->otzE8 > 0) {
            RR_GTE14_SWC2_SXYZ(&a1->sxy140.w.sxy0);
            RR_GTE_RTPS_LDV(verts + (a0->v3 << 3));
            RR_GTE14_NCLIP();
            RR_GTE14_SWC2_MAC0(&a1->mac0F0);
            if (a1->mac0EC > 0 || a1->mac0F0 < 0) {
                RR_GTE14_SWC2_SXY2(&a1->sxy140.w.sxy3);
                RR_SXY_COPY(pkt->xy0, a1->sxy140.w.sxy0);
                RR_SXY_COPY(pkt->xy1, a1->sxy140.w.sxy1);
                RR_SXY_COPY(pkt->xy2, a1->sxy140.w.sxy2);
                RR_SXY_COPY(pkt->xy3, a1->sxy140.w.sxy3);
                if (SkyLitOnScreen4(a1)) {
                    int z = a1->otzE8 >> shift;
                    if (z > 0 && z < 0x1C0) {
                        RR_GTE14_LWC2_RGB(&a0->rgb10);
                        RR_GTE14_LWC2_V0_V1_V2(verts + (a0->n0 << 3), verts + (a0->n1 << 3), verts + (a0->n2 << 3));
                        RR_GTE_OP(0x4B18043F);
                        pkt->color0 = Psyz_GteDataRead(20);
                        pkt->color1 = Psyz_GteDataRead(21);
                        pkt->color2 = Psyz_GteDataRead(22);
                        RR_GTE14_LWC2_V0(verts + (a0->n3 << 3));
                        RR_GTE_OP(0x4B08041B);
                        RR_GTE14_SWC2_RGB2(&pkt->color3);
                        RR_GPU_ADD_PRIM(SkyOrderEntry(SKY_LIT_OT_BASE + z + a0->b15), pkt, 8);
                        INTERP_TAG(pkt, a0, 0);
                        pkt++;
                    }
                }
            }
        }
        a0++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
    return a0;
}

/* Specular lighting: CC after squaring each normal-light product twice. */
static void SkySpecularLight(u32 mvmva) {
    RR_GTE_OP(mvmva);
    RR_GTE_SQR12();
    RR_GTE_SQR12();
    RR_GTE_OP(0x4B38041C);
}

static SkyLitTexQuad24 *EmitSkyLitGT4(SkyLitTexQuad24 *a0, SkyLitCtx *a1) {
    int n = a1->countE4;
    SkyPolyGT4 *pkt = (SkyPolyGT4 *)RR_GPU_SCRATCH->cursor;
    int shift = SKY_DEPTH_SHIFT;
    const u8 *verts = a1->verts70;
    int z;

    while (n-- > 0) {
        RR_GTE_LWC2_V012_3P(verts + (a0->v0 << 3), verts + (a0->v1 << 3), verts + (a0->v2 << 3)); RR_GTE_RTPT_NOP2();
        RR_GTE14_NCLIP();
        RR_GTE14_SWC2_MAC0(&a1->mac0EC);
        RR_GTE_OP(0x4B58002D);
        RR_GTE14_SWC2_OTZ(&a1->otzE8);
        if (a1->otzE8 > 0) {
            RR_GTE14_SWC2_SXYZ(&a1->sxy140.w.sxy0);
            RR_GTE_RTPS_LDV(verts + (a0->v3 << 3));
            RR_GTE14_NCLIP();
            RR_GTE14_SWC2_MAC0(&a1->mac0F0);
            if (a1->mac0EC > 0 || a1->mac0F0 < 0) {
                RR_GTE14_SWC2_SXY2(&a1->sxy140.w.sxy3);
                RR_SXY_COPY(pkt->xy0, a1->sxy140.w.sxy0);
                RR_SXY_COPY(pkt->xy1, a1->sxy140.w.sxy1);
                RR_SXY_COPY(pkt->xy2, a1->sxy140.w.sxy2);
                RR_SXY_COPY(pkt->xy3, a1->sxy140.w.sxy3);
                if (SkyLitOnScreen4(a1)) {
                    pkt->uv0 = a0->uv0;
                    pkt->uv1 = a0->uv1;
                    pkt->uv2 = a0->uv2.word;
                    pkt->uv3 = a0->uv2.h.hi;
                    z = a1->otzE8 >> shift;
                    if (z > 0 && z < 0x1C0) {
                        RR_GTE14_LWC2_RGB(&a0->rgb10);
                        RR_GTE14_LWC2_V0_V1_V2(verts + (a0->n0 << 3), verts + (a0->n1 << 3), verts + (a0->n2 << 3));
                        SkySpecularLight(0x4A4A6412);
                        SkySpecularLight(0x4A4AE412);
                        SkySpecularLight(0x4A4B6412);
                        pkt->color0 = Psyz_GteDataRead(20);
                        pkt->color1 = Psyz_GteDataRead(21);
                        pkt->color2 = Psyz_GteDataRead(22);
                        RR_GTE14_LWC2_V0(verts + (a0->n3 << 3));
                        SkySpecularLight(0x4A4A6412);
                        RR_GTE14_SWC2_RGB2(&pkt->color3);
                        RR_GPU_ADD_PRIM(SkyOrderEntry(SKY_LIT_OT_BASE + z + a0->b21), pkt, 12);
                        INTERP_TAG(pkt, a0, 0);
                        pkt++;
                    }
                }
            }
        }
        a0++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
    return a0;
}

static SkyLitTexQuad24 *SkyEmitSpecTexQuadGT4(SkyLitTexQuad24 *a0, SkyLitCtx *a1) {
    int n = a1->countE4;
    SkyPolyFT4 *pkt = (SkyPolyFT4 *)RR_GPU_SCRATCH->cursor;
    int shift = SKY_DEPTH_SHIFT;
    const u8 *verts = a1->verts70;

    while (n-- > 0) {
        RR_GTE_LWC2_V012_3P(verts + (a0->v0 << 3), verts + (a0->v1 << 3), verts + (a0->v2 << 3)); RR_GTE_RTPT_NOP2();
        RR_GTE14_NCLIP();
        RR_GTE14_SWC2_MAC0(&a1->mac0EC);
        RR_GTE_OP(0x4B58002D);
        RR_GTE14_SWC2_OTZ(&a1->otzE8);
        if (a1->otzE8 > 0) {
            RR_GTE14_SWC2_SXYZ(&a1->sxy140.w.sxy0);
            RR_GTE_RTPS_LDV(verts + (a0->v3 << 3));
            RR_GTE14_NCLIP();
            RR_GTE14_SWC2_MAC0(&a1->mac0F0);
            if (a1->mac0EC > 0 || a1->mac0F0 < 0) {
                RR_GTE14_SWC2_SXY2(&a1->sxy140.w.sxy3);
                RR_SXY_COPY(pkt->xy0, a1->sxy140.w.sxy0);
                RR_SXY_COPY(pkt->xy1, a1->sxy140.w.sxy1);
                RR_SXY_COPY(pkt->xy2, a1->sxy140.w.sxy2);
                RR_SXY_COPY(pkt->xy3, a1->sxy140.w.sxy3);
                if (SkyLitOnScreen4(a1)) {
                    int z;
                    pkt->uv0 = a0->uv0;
                    pkt->uv1 = a0->uv1;
                    pkt->uv2 = a0->uv2.word;
                    pkt->uv3 = a0->uv2.h.hi;
                    z = a1->otzE8 >> shift;
                    if (z > 0 && z < 0x1C0) {
                        RR_GTE14_LWC2_RGB(&a0->rgb10);
                        RR_GTE14_LWC2_V0(verts + (a0->n0 << 3));
                        RR_GTE14_LIGHT0();
                        RR_GTE14_SWC2_RGB2(&pkt->color);
                        RR_GPU_ADD_PRIM(SkyOrderEntry(SKY_LIT_OT_BASE + z + a0->b21), pkt, 9);
                        INTERP_TAG(pkt, a0, 0);
                        pkt++;
                    }
                }
            }
        }
        a0++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
    return a0;
}

void RunSkyCommandList(const u8 *table, int index)
{
    SkyLitCtx *s = &SkyLitWork;
    const SkyOffsetRecord *record = SkyRecord(table, index);
    const u8 *p;
    int w;

    s->clutFC = 0x20180000;
    s->pageF8 = 0x60000;
    s->verts70 = table + record->verts;
    p = table + record->commands;
    w = *(const s32 *)p;
    p += 4;
    while (w) {
        s->countE4 = w >> 16;
        switch (w & 0xFFFF) {
        case 9:
            p = (const u8 *)SkyEmitLitTriG3((SkyLitTri14 *)p, s);
            break;
        case 10:
            p = (const u8 *)SkyEmitLitTexQuadGT4((SkyLitQuad18 *)p, s);
            break;
        case 13:
            p = (const u8 *)SkyEmitLitQuadG4((SkyLitQuad18 *)p, s);
            break;
        case 15:
            p = (const u8 *)EmitSkyLitGT4((SkyLitTexQuad24 *)p, s);
            break;
        case 16:
            p = (const u8 *)SkyEmitSpecTexQuadGT4((SkyLitTexQuad24 *)p, s);
            break;
        }
        w = *(const s32 *)p;
        p += 4;
    }
}

void SetupSkyLightMatrices(char *a0, unsigned short *a1_in)
{
    volatile unsigned short *p = (unsigned short *)(SCRATCHPAD + 0x7C), *a1 = a1_in;
    RR_GTE_SETROT(SkyDefaultRotMatrix);
    RR_GTE_MV3_COL(a1);
    RR_GTE_ST3_COL(p);
    RR_GTE_MV3_COL(a1 + 1);
    RR_GTE_ST3_COL((unsigned short *)(SCRATCHPAD + 0x7E));
    RR_GTE_MV3_COL(a1 + 2);
    RR_GTE_ST3_COL((unsigned short *)SCRATCH_CAR_WORK);
    RR_GTE_SETLLM(p);
    p[0] = a1[0];
    p[1] = a1[1];
    p[2] = a1[2];
    p[3] = a1[3] << 1;
    p[4] = a1[4] << 1;
    p[5] = a1[5] << 1;
    p[6] = a1[6];
    p[7] = a1[7];
    p[8] = a1[8];
    RR_GTE_SETROT(p);
    RR_GTE_SETTR(a0);
    RunSkyCommandList(SkyCommandListPtr, 0);
}

static SkySpritePacket *SkyEmitSprite(SkySpritePacket *packet, u32 drawMode, u32 color,
                                      u32 position, u32 size, int order) {
    packet->drawMode = drawMode;
    packet->nop = RR_GPU_MERGED_PRIM_NOP;
    packet->color = color;
    packet->position = position;
    packet->uvClut = 0;
    packet->size = size;
    RR_GPU_ADD_PRIM(SkyOrderEntry(order), packet, 6);
    return packet + 1;
}

static void SkyEmitDrawArea(GpuRectangleFields *rectangle, int order) {
    GpuDrawModePacket *packet = (GpuDrawModePacket *)RR_GPU_SCRATCH->cursor;
    SetDrawArea(packet, rectangle);
    RR_GPU_ADD_PRIM(SkyOrderEntry(order), packet, 2);
    packet++;
    SetDrawOffset(packet, rectangle);
    RR_GPU_ADD_PRIM(SkyOrderEntry(order), packet, 2);
    packet++;
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
}

/* Renders the sky into an off-screen rectangle (rect x/y/w/h) and copies it
 * back with two blended sprites of the given intensity. */
void RenderSkyToRect(const void *translation, const void *rotation, const GpuRectangleFields *a2, int a3) {
    GpuRectangleFields rectangle;
    SkySpritePacket *sprite;
    GpuTileCommandWords *tile;
    u32 c;

    sprite = (SkySpritePacket *)RR_GPU_SCRATCH->cursor;
    sprite = SkyEmitSprite(sprite, 0xE1000307, 0x65000000,
                           ((u32)a2->y << 16) | (u32)a2->x,
                           (u32)a2->w | 0x01000000, 4);
    if (a2->h >= 0x101) {
        sprite = SkyEmitSprite(sprite, 0xE1000317, 0x65000000,
                               ((u32)(a2->y + 0x100) << 16) | (u32)a2->x,
                               ((u32)(a2->h - 0x100) << 16) | (u32)a2->w, 4);
    }
    RR_GPU_SCRATCH->cursor = (u8 *)sprite;
    rectangle.w = 0x140;
    rectangle.x = 0;
    rectangle.y = 0;
    rectangle.h = 0x1E0;
    SkyEmitDrawArea(&rectangle, 4);
    SetGeomOffset((short)(unsigned short)a2->w / 2, (short)(unsigned short)a2->h / 2);
    SetupSkyLightMatrices((char *)translation, (unsigned short *)rotation);
    c = ((u32)a3 << 16) | ((u32)a3 << 8) | 0x66000000 | (u32)a3;
    sprite = (SkySpritePacket *)RR_GPU_SCRATCH->cursor;
    sprite = SkyEmitSprite(sprite, 0xE1000307, c, 0xFFFF0000, (u32)a2->w | 0x01000000, 4);
    sprite = SkyEmitSprite(sprite, 0xE1000317, c, 0xFF0000,
                           ((u32)(a2->h - 0x100) << 16) | (u32)a2->w, 4);
    tile = (GpuTileCommandWords *)sprite;
    tile->color.word = 0x60000000;
    tile->position = 0;
    tile->size = ((u32)a2->h << 16) | (u32)a2->w;
    RR_GPU_ADD_PRIM(SkyOrderEntry(700), tile, 3);
    RR_GPU_SCRATCH->cursor = (u8 *)(tile + 1);
    rectangle.x = 0x1C0;
    rectangle.y = 0;
    rectangle.w = a2->w;
    rectangle.h = a2->h;
    SkyEmitDrawArea(&rectangle, 700);
}

/* Initial values of the globals this module owns. */
/* Sky keyframe lists (time, weight pairs) of courses 0, 1 and 6, one owner
 * from 0x8009E168: DrawTrackScenery reads the pair before a list's first
 * entry (a1[-1], a1[-2]) when the current time equals it. For the first
 * list that is the zero pad word ending SpotlightConeRingOuter. */
short CourseSkyKeyframes[2 + 20 + 20 + 12] = {
    0, 0,
    0, 0, 226, 0, 230, 4096, 317, 4096, 321, 0, 470, 0, 474, 4096, 480, 4096, 484, 0, 4096, 0, /* D_8009E16C */
    0, 0, 209, 0, 213, 4096, 242, 4096, 246, 0, 501, 0, 505, 4096, 512, 4096, 516, 0, 4096, 0, /* D_8009E194 */
    0, 0, 232, 0, 236, 4096, 273, 4096, 277, 0, 4096, 0, /* D_8009E1BC */
};
short D_8009E1F8[6] = {228, 3, 508, 2, 4096, 3};
short D_8009E204[6] = {94, 2, 436, 3, 4096, 2};
short D_8009E210[10] = {90, 2, 233, 3, 400, 2, 456, 3, 4096, 2};
char UnknownSkyModeFmt[52] = {117, 110, 107, 110, 111, 119, 110, 32, 109, 111, 100, 101, 32, 37, 120, 10};
short *CourseSkyKeyframePtrs[9] = {&CourseSkyKeyframes[2], &CourseSkyKeyframes[22], 0, 0, 0, 0, &CourseSkyKeyframes[42]};
short *CourseSceneryRangePtrs[9] = {0, (short *)D_8009E1F8, (short *)D_8009E204, (short *)D_8009E210};
char SkyDefaultRotMatrix[32] = {0, 12, 0, -12, 0, -12, 103, -10, 103, -10, 103, -10, 0, 0, 0, 0, 0, -16};
