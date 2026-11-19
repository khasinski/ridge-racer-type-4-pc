#include "rr/geometry.h"
#include "rr/video_timing.h"
#include "game/race/racegrid.h"
#include "rr/frame_loop.h"
#include "game/data/resinit.h"
#include "game/math/rotmat.h"
#include "psyq/libgte/gtemat4.h"
#include "rr/frame_storage_types.h"
#include "psyq/libgte/gtemat3.h"
#include "psyq/libgte/gtemat.h"
/* RODATA 0x80010f00 0x1c */
#include "types.h"
#include "scratchpad.h"
#include "rr_asm.h"
#include "game/course/trkintp2.h"
#include "game/gfx/dispenv.h"
#include "psyq/libgte/gtemat7.h"
#include "game/gfx/gfxdraw.h"
#include "rr/audio_loop.h"
#include "rr/overlay662_distance.h"
#include "rr/fade_sprite.h"
#include "rr/trig.h"
#include "rr/scene_camera_state.h"
#include "rr/projection_state.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"

typedef struct { int x, y, z, w; } V4;

typedef struct { unsigned char a, b, c, d; } C4;

typedef struct { unsigned short a, b, c, d; } S4;

typedef struct {
    s32 unk00[4];
    s16 rotX10, rotY12, rotZ14, unk16;
    C4 c[3];
    S4 v[3];
    unsigned char b3C, b3D, b3E;
} A0;

typedef struct { s16 x, y, z, pad; } ShortVec4;

typedef struct {
    s32 unk00[2];
    u16 sxy[6];
    s32 unk14[4];
    ShortVec4 v[3];
    s32 unk3C[2];
    u8 c0[3];
    u8 pad3;
    u8 c1[3];
} GfxGteScratch;

typedef struct {
    GpuDmaTag tag;
    u8 r0, g0, b0, code;
    u16 x0, y0;
    u8 r1, g1, b1, pad0F;
    u16 x1, y1;
    u8 r2, g2, b2, pad17;
    u16 x2, y2;
} PolyG3Packet;

extern int ViewUsesCarTrackNode;
extern int SceneFogWorkVector[];
/* PsyZ libgte ScaleMatrix(MATRIX *, VECTOR *) and RotTransPers(SVECTOR *,
 * long *, long *, long *); the game types share those layouts. */
extern MatrixWords32 *ScaleMatrix(MatrixWords32 *matrix, V4 *scale);
extern V4 MenuCarDefaultScale;
extern int RotTransPers(const short *vector, int *screen, int *interpolation, int *flags);

void UpdateRaceCamera(int arg0, void *arg1);
void InitTrackStartPos(void);

/* The kind of camera the last call ran (its node's, for arg0 >= 2), for
 * the replay's charge of its time (rr/cpu_work.h RrSpendReplayFrame). */
int RaceCameraKind;

void UpdateRaceCamera(int arg0, void *arg1) {
    int sp10[4];
    int sp20[4];
    unsigned short sp30[4];
    unsigned short sp38[4];
    CamNode44 *s2;
    int a0;
    int a1;
    int a2;
    int t1;
    /* PS1 scratch words 2..4 = camera position, 6..8 = Euler angles. */
    s32 *position = ((ProjectionScratchPrefix *)SCRATCHPAD)->pose08.position00.words;
    s32 *angles = ((ProjectionScratchPrefix *)SCRATCHPAD)->pose08.angles10.words;

    s2 = RaceContext.cameraNodes550;
    a0 = FindNearestTrackEntry(arg1);
    if (arg0 >= 2) {
        s2 = RaceContext.cameraNodes550;
        s2 += a0;
        ViewUsesCarTrackNode = 0;
        arg0 = s2->f30 & 0xF;
    }
    RaceCameraKind = arg0;
    RaceContext.cameraNodeChanged1C = 0;
    a1 = RaceContext.cameraNodeFrames48 + 1;
    RaceContext.cameraNodeFrames48 = a1;
    if (a0 != RaceContext.cameraNodeIndex4C) {
        if (GameModeType == 5 || a1 >= RR_PAL_NTSC(0x1A, 0x1F)) {
            RaceContext.cameraNodeChanged1C = 1;
            RaceContext.cameraNodeIndex4C = a0;
            RaceContext.cameraNodeFrames48 = 0;
        }
        ResetCarSlotBuffers();
    }
    switch (arg0) {
    case 0:
        UpdateRaceChaseCamera(arg1);
        break;
    case 1:
        position[0] = s2->x * 4;
        position[1] = s2->y * 4;
        position[2] = s2->z * 4;
        RaceContext.cameraFollowOffset30.fields.x = 0;
        RaceContext.cameraFollowOffset30.fields.y = 0;
        RaceContext.cameraFollowOffset30.fields.z = -0x100;
        CarLocalToWorld(arg1, &RaceContext.cameraFollowOffset30, &RaceContext.cameraFollowPosition20.vector);
        sp10[0] = RaceContext.cameraFollowPosition20.vector.x - position[0];
        sp10[2] = RaceContext.cameraFollowPosition20.vector.z - position[2];
        sp10[1] = RaceContext.cameraFollowPosition20.vector.y - position[1];
        angles[1] = 0x400 - Atan2Table(sp10[0], sp10[2]);
        angles[0] = 0x400 - Atan2Table(sp10[1], CordicSqrt(sp10[0] * sp10[0] + sp10[2] * sp10[2]) >> 6);
        angles[2] = LerpAngleWrap4096(s2, RaceContext.cameraNodeFrames48);
        SetGeomScreen(s2->f28);
        break;
    case 2:
        RaceContext.cameraFollowOffset30.fields.x = 0;
        RaceContext.cameraFollowOffset30.fields.y = 0;
        RaceContext.cameraFollowOffset30.fields.z = -0x100;
        CarLocalToWorld(arg1, &RaceContext.cameraFollowOffset30, &RaceContext.cameraFollowPosition20.vector);
        a2 = ComputeCameraNodeDist(arg1, s2) >> 4;
        t1 = a2 / 4;
        if (a2 - t1 > (0x400000 / s2->f28) * 8) {
            t1 = a2 - (0x400000 / s2->f28) * 8;
        }
        sp10[0] = (RaceContext.cameraFollowPosition20.vector.x / 4) - s2->x;
        sp10[1] = (RaceContext.cameraFollowPosition20.vector.y / 4) - s2->y;
        sp10[2] = (RaceContext.cameraFollowPosition20.vector.z / 4) - s2->z;
        position[0] = RaceContext.cameraFollowPosition20.vector.x - ((sp10[0] * t1) / a2) * 4;
        position[1] = RaceContext.cameraFollowPosition20.vector.y - ((sp10[1] * t1) / a2) * 4;
        position[2] = RaceContext.cameraFollowPosition20.vector.z - ((sp10[2] * t1) / a2) * 4;
        a0 = (t1 * (s2->f28 << 4)) / 4096;
        if (a0 >= 0x401) {
            a0 = 0x400;
        } else if (a0 < 0x118) {
            a0 = 0x118;
        }
        SetGeomScreen(a0);
        sp10[0] = RaceContext.cameraFollowPosition20.vector.x - position[0];
        sp10[2] = RaceContext.cameraFollowPosition20.vector.z - position[2];
        sp10[1] = RaceContext.cameraFollowPosition20.vector.y - position[1];
        angles[1] = 0x400 - Atan2Table(sp10[0], sp10[2]);
        angles[0] = 0x400 - Atan2Table(sp10[1], CordicSqrt(sp10[0] * sp10[0] + sp10[2] * sp10[2]) >> 6);
        angles[2] = LerpAngleWrap4096(s2, RaceContext.cameraNodeFrames48);
        break;
    case 3:
        position[0] = s2->x * 4;
        position[1] = s2->y * 4;
        position[2] = s2->z * 4;
        angles[0] = s2->f10;
        angles[1] = s2->f14;
        angles[2] = LerpAngleWrap4096(s2, RaceContext.cameraNodeFrames48);
        SetGeomScreen(s2->f28);
        break;
    case 4:
        position[0] = s2->x * 4;
        position[1] = s2->y * 4;
        position[2] = s2->z * 4;
        RaceContext.cameraFollowOffset30.fields.x = 0;
        RaceContext.cameraFollowOffset30.fields.y = 0;
        RaceContext.cameraFollowOffset30.fields.z = -0x100;
        CarLocalToWorld(arg1, &RaceContext.cameraFollowOffset30, &RaceContext.cameraFollowPosition20.vector);
        sp30[0] = s2->f10;
        sp30[1] = s2->f14;
        sp30[2] = 0;
        RotateNegZByEulerZYX(sp30, s2->f18, sp20);
        sp20[0] += s2->x;
        sp20[1] += s2->y;
        sp20[2] += s2->z;
        RaceContext.cameraFollowPosition20.vector.x = ((((RaceContext.cameraFollowPosition20.vector.x / 4) - sp20[0]) * s2->f2C) / 1024 + sp20[0]) * 4;
        RaceContext.cameraFollowPosition20.vector.y = ((((RaceContext.cameraFollowPosition20.vector.y / 4) - sp20[1]) * s2->f2C) / 1024 + sp20[1]) * 4;
        RaceContext.cameraFollowPosition20.vector.z = ((((RaceContext.cameraFollowPosition20.vector.z / 4) - sp20[2]) * s2->f2C) / 1024 + sp20[2]) * 4;
        sp10[0] = RaceContext.cameraFollowPosition20.vector.x - position[0];
        sp10[2] = RaceContext.cameraFollowPosition20.vector.z - position[2];
        sp10[1] = RaceContext.cameraFollowPosition20.vector.y - position[1];
        angles[1] = 0x400 - Atan2Table(sp10[0], sp10[2]);
        angles[0] = 0x400 - Atan2Table(sp10[1], CordicSqrt(sp10[0] * sp10[0] + sp10[2] * sp10[2]) >> 6);
        angles[2] = LerpAngleWrap4096(s2, RaceContext.cameraNodeFrames48);
        SetGeomScreen(s2->f28);
        break;
    case 5:
        GetTrackLaneWorldPos(arg1, &RaceContext.cameraFollowPosition20.vector.x, s2);
        sp38[0] = s2->f10;
        sp38[1] = s2->f14;
        sp38[2] = s2->f18;
        TransformCameraOffset(sp38, s2->f2C, &position[0]);
        angles[2] = LerpAngleWrap4096(s2, RaceContext.cameraNodeFrames48);
        position[0] += RaceContext.cameraFollowPosition20.vector.x;
        position[1] += RaceContext.cameraFollowPosition20.vector.y;
        position[2] += RaceContext.cameraFollowPosition20.vector.z;
        SetGeomScreen(s2->f28);
        break;
    case 6:
        RaceContext.cameraFollowOffset30.fields.x = 0;
        RaceContext.cameraFollowOffset30.fields.y = 0;
        RaceContext.cameraFollowOffset30.fields.z = -0x100;
        CarLocalToWorld(arg1, &RaceContext.cameraFollowOffset30, &RaceContext.cameraFollowPosition20.vector);
        GetTrackLaneWorldPos(arg1, &position[0], s2);
        sp10[0] = RaceContext.cameraFollowPosition20.vector.x - position[0];
        sp10[2] = RaceContext.cameraFollowPosition20.vector.z - position[2];
        sp10[1] = RaceContext.cameraFollowPosition20.vector.y - position[1];
        angles[1] = 0x400 - Atan2Table(sp10[0], sp10[2]);
        angles[0] = 0x400 - Atan2Table(sp10[1], CordicSqrt(sp10[0] * sp10[0] + sp10[2] * sp10[2]) >> 6);
        angles[2] = LerpAngleWrap4096(s2, RaceContext.cameraNodeFrames48);
        SetGeomScreen(s2->f28);
        break;
    }
    BuildCameraMatrices();
}

void InitTrackStartPos(void) {
    SceneFogWorkVector[0] = TrackNodes[0].road.x;
    SceneFogWorkVector[1] = 0;
    SceneFogWorkVector[2] = TrackNodes[0].road.z;
    SceneFogWorkVector[4] = 0;
    SceneFogWorkVector[5] = 0;
    SceneFogWorkVector[6] = 0;
}

void BuildObjectLightAndRotMatrices(const void *object, MatrixWords32 *a1) {
    const A0 *a0 = object;
    struct {
        MatrixWords32 m10;
        MatrixWords32 m30;
        MatrixWords32 m50;
    } s;
    MatrixWords32 t70;
    MatrixWords32 t90;
    V4 b0;
    int a3;
    b0 = MenuCarDefaultScale;
    BuildRotMatrixY(&s.m50, -a0->rotY12);
    MulMatrix2((const MatrixWords32 *)SCRATCH_ROT_MATRIX, &s.m50);
    BuildRotMatrixX(&s.m10, -a0->rotX10);
    BuildRotMatrixZ(&s.m30, -a0->rotZ14);
    MulMatrix(&s.m10, &s.m30);
    MulMatrix2(&s.m50, &s.m10);
    BuildRotMatrixY(&s.m50, a0->rotY12 - 0x400);
    BuildRotMatrixX(&s.m30, a0->rotX10);
    MulMatrix(&s.m50, &s.m30);
    BuildRotMatrixZ(&s.m30, a0->rotZ14);
    MulMatrix2(&s.m50, &s.m30);
    a3 = 0;
    do {
        t70.fields.rotation00[a3][0] = a0->v[a3].a;
        t70.fields.rotation00[a3][1] = -a0->v[a3].b;
        t70.fields.rotation00[a3][2] = a0->v[a3].c;
        t90.fields.rotation00[0][a3] = a0->c[a3].a * 16;
        t90.fields.rotation00[1][a3] = a0->c[a3].b * 16;
        t90.fields.rotation00[2][a3] = a0->c[a3].c * 16;
        a3++;
    } while (a3 < 3);
    MulMatrix0(&t70, &s.m30, &s.m50);
    SetLightMatrix(&s.m50);
    SetColorMatrix(&t90);
    SetBackColor(a0->b3C, a0->b3D, a0->b3E);
    ScaleMatrix(&s.m10, &b0);
    *a1 = s.m10;
}

int RotTransPersOrigin(void) {
    short a[4]; /* SVECTOR: RotTransPers loads the pad halfword with vz */
    int b, c, d;
    a[0] = 0;
    a[1] = 0;
    a[2] = 0;
    return RotTransPers(a, &b, &c, &d);
}

/* PS1 0x800AC650 (BSS): every alias in its range is a named member. */
SceneRaceContext RaceContext;

void InitRaceContextBuffers(void) {
    int i;
    for (i = 0; i < 2; i++) {
        /* Showcase sub-ordering tables, [object][bank]: the PS1 map's
         * ViewportRenderEnvLeft (0x800AE660) and ViewportRenderEnvRight
         * (0x800D0DD8) are each bank's secondary ordering table, split into
         * two 0x15F-entry halves, so the split-screen views share them. */
        RaceContext.subOrdering50[i][0] = &DrawBuffers[0].buffer.secondaryOrderingB70.tags[i * 0x15F];
        RaceContext.subOrdering50[i][1] = &DrawBuffers[1].buffer.secondaryOrderingB70.tags[i * 0x15F];
    }
}

u8 *EmitSpotlightConePolys(s32 a0) {
    GfxGteScratch *g = (GfxGteScratch *)(SCRATCHPAD + 0x7C);
    PolyG3Packet *p = (PolyG3Packet *)RR_GPU_SCRATCH->cursor;
    GpuDmaTag *ot = &CurrentDrawBuf->buffer.ordering70.tags[a0];
    GpuCommandPacket *mode;
    s32 i;
    g->c0[0] = 0x50;
    g->c0[1] = 0x50;
    g->c0[2] = 0x50;
    g->c1[0] = 0;
    g->c1[1] = 0;
    g->c1[2] = 0;
    g->v[0].x = 0;
    g->v[0].y = 0;
    g->v[0].z = 0;
    i = 0;
    do {
        s32 a = i << 8;
        g->v[1].x = (rsin(a) * 800) / 4096;
        g->v[1].y = 0;
        g->v[1].z = (rcos(a) * 800) / 4096;
        i++;
        a = i << 8;
        g->v[2].x = (rsin(a) * 800) / 4096;
        g->v[2].y = 0;
        g->v[2].z = (rcos(a) * 800) / 4096;
        RR_GTE_LWC2_V012_3P(&g->v[0], &g->v[1], &g->v[2]);
        RR_GTE_RTPT_NOP2();
        RR_GTE_SWC2_SXY_3P(&g->sxy[0], &g->sxy[2], &g->sxy[4]);
        p->code = 0x33;
        RrSxyCopy(&p->x0, &g->sxy[0]);
        RrSxyCopy(&p->x1, &g->sxy[2]);
        RrSxyCopy(&p->x2, &g->sxy[4]);
        p->r0 = g->c0[0];
        p->g0 = g->c0[1];
        p->b0 = g->c0[2];
        p->r1 = g->c1[0];
        p->g1 = g->c1[1];
        p->b1 = g->c1[2];
        p->r2 = g->c1[0];
        p->g2 = g->c1[1];
        p->b2 = g->c1[2];
        RR_GPU_ADD_PRIM(ot, p, 6);
        p++;
    } while (i < 16);
    /* Draw-mode packet (E1 texture page / semi-transparency). */
    mode = (GpuCommandPacket *)p;
    mode->command = 0xE1000245;
    RR_GPU_ADD_PRIM(ot, mode, 1);
    RR_GPU_SCRATCH->cursor = (u8 *)(mode + 1);
    return RR_GPU_SCRATCH->cursor;
}

/* Initial values of the globals this module owns. */
V4 MenuCarDefaultScale = {.x = 409, .y = 409, .z = 409};
int SceneFogWorkVector[10];
