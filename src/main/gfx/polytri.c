#include "rr/cpu_work.h"
#include "rr/frame_interp.h"
#include "scratchpad.h"
#include "rr/menu_selection.h"
#include "game/race/objrec.h"
#include "game/car/aipath.h"
#include "psyq/libgte/gtemath4.h"
#include "psyq/libgte/gtemath5.h"
#include "game/race/objclr.h"
#include "game/gfx/polytri.h"
#include "rr/animated_glyph.h"
#include "rr/audio_loop.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "rr/rro_model.h"
#include "rr_asm.h"
/* GTE staging at scratchpad+0x7C: projected XY words and short vectors. */
typedef struct {
    s32 unk00[0x78 / 4];
    s32 sxy0, sxy1, sxy2;
    s32 unk84[(0xB4 - 0x84) / 4];
    unsigned short v0[6];
    unsigned short v1[6];
    unsigned short v2[6];
} Sp7C;

typedef struct {
    s32 unk00[0x78 / 4];
    s32 sxy0, sxy1, sxy2, sxy3;
    s32 unk88[(0xB4 - 0x88) / 4];
    unsigned short v0[6];
    unsigned short v1[6];
    unsigned short v2[6];
    unsigned short v3[6];
} Sp7C_1;

/* POLY_G4 with the host packet tag. */
typedef struct {
    GpuDmaTag tag;
    s32 c0, sxy0, c1, sxy1, c2, sxy2, c3, sxy3;
} PolyG4;

#include "game/car/car.h"
/* size 0x320 */
typedef struct {
    int f0;
    int f4;
    int f8;
} S_SP7C;
extern RaceVehicle PlayerCars[];
extern int ReplayState;
extern short ReverseCourse;
extern int TrackLength;

void EmitSemiTransG3List(SemiTransTri38 *a0, int a1, int a2) {
    int sp20;
    int sp24;
    int i;
    int otz;
    GpuDmaTag *ot;
    s32 depthShift = ((RroSceneScratchPrefix *)SCRATCHPAD)->depthShift60;
    GpuPolyG3 *pkt = (GpuPolyG3 *)RR_GPU_SCRATCH->cursor;
    GpuCommandPacket *mode;
    Sp7C *sp = (Sp7C *)(SCRATCHPAD + 0x7C);

    for (i = 0; i < a1; i++, a0++) {
        sp->v0[0] = (u16)a0->v0.vx;
        sp->v0[1] = (u16)a0->v0.vy;
        sp->v0[2] = (u16)a0->v0.vz;
        sp->v1[0] = (u16)a0->v1.vx;
        sp->v1[1] = (u16)a0->v1.vy;
        sp->v1[2] = (u16)a0->v1.vz;
        sp->v2[0] = (u16)a0->v2.vx;
        sp->v2[1] = (u16)a0->v2.vy;
        sp->v2[2] = (u16)a0->v2.vz;
        otz = RotAverage3((int *)sp->v0, (int *)sp->v1, (int *)sp->v2, &sp->sxy0, &sp->sxy1, &sp->sxy2, &sp20, &sp24) >> (depthShift + a2);
        if (otz > 0 && otz < 0x1C0) {
            if (a2 == 2) {
                otz = 0x1BF;
            }
            RR_SXY_COPY(pkt->xy0, sp->sxy0);
            RR_SXY_COPY(pkt->xy1, sp->sxy1);
            RR_SXY_COPY(pkt->xy2, sp->sxy2);
            pkt->color0 = (*(s32 *)&a0->c0 & 0xFFFFFF) | 0x32000000;
            pkt->color1 = *(s32 *)&a0->c1;
            pkt->color2 = *(s32 *)&a0->c2;
            ot = RR_GPU_SCRATCH->order + (otz + 0x80);
            RR_GPU_ADD_PRIM(ot, pkt, 6);
            /* the list entry of the object's list */
            INTERP_TAG(pkt, (u32)i | (u32)a1 << 8 | (u32)a2 << 16, 3);
            mode = (GpuCommandPacket *)(pkt + 1);
            mode->command = 0xE1000220;
            RR_GPU_ADD_PRIM(ot, mode, 1);
            pkt = (GpuPolyG3 *)(mode + 1);
        }
    }
    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
    RrWorkEnd(RR_GPU_SCRATCH->cursor); /* a caller's count (rr/cpu_work.h) */
}

void EmitSemiTransG4List(SemiTransQuad48 *a0, int a1, int a2) {
    int sp28;
    int sp2C;
    int i;
    int otz;
    GpuDmaTag *ot;
    s32 depthShift = ((RroSceneScratchPrefix *)SCRATCHPAD)->depthShift60;
    PolyG4 *pkt = (PolyG4 *)RR_GPU_SCRATCH->cursor;
    GpuCommandPacket *mode;
    Sp7C_1 *sp = (Sp7C_1 *)(SCRATCHPAD + 0x7C);

    for (i = 0; i < a1; i++, a0++) {
        sp->v0[0] = (u16)a0->v0.vx;
        sp->v0[1] = (u16)a0->v0.vy;
        sp->v0[2] = (u16)a0->v0.vz;
        sp->v1[0] = (u16)a0->v1.vx;
        sp->v1[1] = (u16)a0->v1.vy;
        sp->v1[2] = (u16)a0->v1.vz;
        sp->v2[0] = (u16)a0->v2.vx;
        sp->v2[1] = (u16)a0->v2.vy;
        sp->v2[2] = (u16)a0->v2.vz;
        sp->v3[0] = (u16)a0->v3.vx;
        sp->v3[1] = (u16)a0->v3.vy;
        sp->v3[2] = (u16)a0->v3.vz;
        otz = RotAverage4((int *)sp->v0, (int *)sp->v1, (int *)sp->v2, (int *)sp->v3, &sp->sxy0, &sp->sxy1, &sp->sxy2, &sp->sxy3, &sp28, &sp2C) >> (depthShift + a2);
        if (otz > 0 && otz < 0x1C0) {
            if (a2 == 2) {
                otz = 0x1BF;
            }
            RR_SXY_COPY(pkt->sxy0, sp->sxy0);
            RR_SXY_COPY(pkt->sxy1, sp->sxy1);
            RR_SXY_COPY(pkt->sxy2, sp->sxy2);
            RR_SXY_COPY(pkt->sxy3, sp->sxy3);
            pkt->c0 = (*(s32 *)&a0->c0 & 0xFFFFFF) | 0x3A000000;
            pkt->c1 = *(s32 *)&a0->c1;
            pkt->c2 = *(s32 *)&a0->c2;
            pkt->c3 = *(s32 *)&a0->c3;
            ot = RR_GPU_SCRATCH->order + (otz + 0x80);
            RR_GPU_ADD_PRIM(ot, pkt, 8);
            INTERP_TAG(pkt, (u32)i | (u32)a1 << 8 | (u32)a2 << 16, 4);
            mode = (GpuCommandPacket *)(pkt + 1);
            mode->command = 0xE1000220;
            RR_GPU_ADD_PRIM(ot, mode, 1);
            pkt = (PolyG4 *)(mode + 1);
        }
    }
    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
    RrWorkEnd(RR_GPU_SCRATCH->cursor); /* a caller's count (rr/cpu_work.h) */
}

void UpdateTrackObjectActivation(void) {
    int i;
    S_SP7C *sp = (S_SP7C *)(SCRATCHPAD + 0x7C);
    const ProjectionScratchPrefix *view = (const ProjectionScratchPrefix *)SCRATCHPAD;

#define b0 (&TrackObjectDefs[i])
#define b4 (&TrackObjectStates[i])

    sp->f0 = FindTrackNodeAtXZ(view->pose08.position00.vector.x, view->pose08.position00.vector.z);
    for (i = 0; i < TrackObjectCount; i++) {
        if (b0->f18 >= 0 && b0->f18 != PlayerCars[0].control.dynamics94.steering.count22) {
            if (b0->f10 > 0) {
                b4->f24 = 0;
            }
            continue;
        }
        if (b0->f2C != 0x3F) {
            if (GameModeType == 1 && ReplayState < 2) {
                if (!(b0->f2C & 1)) {
                    continue;
                }
            } else if (GameModeType == 1 && ReplayState == 2) {
                if (!(b0->f2C & 2)) {
                    continue;
                }
            } else if (GameModeType == 0 && ReplayState < 2) {
                if (!(b0->f2C & 4)) {
                    continue;
                }
            } else if (GameModeType == 0 && ReplayState == 2) {
                if (!(b0->f2C & 8)) {
                    continue;
                }
            } else if (GameModeType == 6) {
                if (b0->f2C == 0xA) {
                    if (ReplayState != 2) {
                        continue;
                    }
                } else if (!(b0->f2C & 0x10)) {
                    continue;
                }
            } else if (GameModeType == 5) {
                if (b0->f2C != 0x20) {
                    continue;
                }
            } else {
                if (b0->f2C != 0xA || ReplayState != 2) {
                    continue;
                }
            }
        }
        sp->f4 = b0->f1C;
        sp->f8 = b0->f20;
        if (b0->f10 > 0) {
            if (b0->f18 == -3) {
                if ((PlayerCars[0].control.dynamics94.steering.count22 < 6 && (TrackLength * 4 + ((TrackLength * 59) >> 6)) >= PlayerCars[0].first) ||
                    (sp->f4 < sp->f0 && sp->f0 <= sp->f8)) {
                    b4->f24 = 0;
                    b4->f8 = 0;
                } else {
                    b4->f24 = 1;
                }
                continue;
            }
            if ((sp->f4 >= sp->f8 ? (sp->f4 >= sp->f0 && sp->f0 >= sp->f8) : (sp->f4 >= sp->f0 || sp->f0 > sp->f8)) || b0->f18 == -2) {
                b4->f24 = 1;
            } else {
                b4->f24 = 0;
                continue;
            }
        } else if (b0->f14 > 0 && b4->f24 == 0 && sp->f4 == sp->f0 && PlayerCars[0].half1EA == 0 && ReverseCourse == 0) {
            b4->f24 = 1;
        }
        if (GameModeType == 2 || GameModeType == 3) {
            if (b0->f0 == 0 && (b0->f4 % 10) == 0) {
                if (PlayerCars[0].control.dynamics94.steering.count22 == 0) {
                    if (PlayerCars[1].control.dynamics94.steering.count22 == 0) {
                        b4->f24 = 1;
                    } else {
                        b4->f24 = 0;
                    }
                } else {
                    b4->f24 = 0;
                }
            } else if (b0->f0 == 1 && (b0->f4 % 10) == 0) {
                if (((NumLaps - 1) * TrackLength + ((TrackLength * 29) >> 5)) < PlayerCars[0].first ||
                    ((NumLaps - 1) * TrackLength + ((TrackLength * 29) >> 5)) < PlayerCars[1].first) {
                    b4->f24 = 1;
                } else {
                    b4->f24 = 0;
                }
            }
        } else if ((GameModeType == 4 || GameModeType == 5) &&
                   (((b0->f4 % 10) == 0 && (b0->f0 == 0 || b0->f0 == 1)) ||
                    (b0->f8 == 0xA && b0->f4 == 0) ||
                    b0->fC == 4)) {
            b4->f24 = 0;
        } else if (b0->f0 == 1 && (b0->f4 % 10) == 0) {
            if (((NumLaps - 1) * TrackLength + ((TrackLength * 29) >> 5)) > PlayerCars[0].first) {
                b4->f24 = 0;
            }
        }
    }
}

/* Initial values of the globals this module owns. */
int ReplayState;
int TrackLength;
S_ECB0 *TrackObjectDefs;
