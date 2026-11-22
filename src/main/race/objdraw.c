#include "game/data/menuvars.h"
#include "psyq/libgte/rotmat.h"
#include "rr/player_cars.h"
#include "rr/menu_selection.h"
#include "rr/rro_model.h"
#include "rr/memory.h"
#include "psyq/libgpu/gpudisp.h"
#include "rr/animated_glyph.h"
#include "rr/projection_state.h"
#include "game/gfx/model.h"
#include "psyq/libgte/gtemat5.h"
#include "rr/camera_matrices.h"
#include "rr/track_lighting.h"
#include "psyq/libgte/gtemat4.h"
#include "psyq/libgte/gtemat.h"
#include "rr/matrix_workspace.h"
#include "types.h"
#include "scratchpad.h"
#include "rr_asm.h"
#include "game/car/aipath.h"
#include "game/gfx/gtemodel.h"
#include "game/gfx/poly1.h"
#include "game/gfx/tilerect.h"
#include "game/sound/sndbgm3.h"
#include "game/sound/sndrst2.h"
#include "game/sound/sndvab.h"
#include "psyq/libgte/gtemat3.h"
#include "psyq/libgte/gtemat7.h"
#include "psyq/libgte/gtemath2.h"
#include "game/race/objrec.h"
#include "game/race/objclr.h"
#include "game/race/objsim.h"
#include "game/race/objdraw.h"
#include "game/gfx/polytri.h"
#include "rr/track_sample_types.h"
#include "psyq/libgte/gtecord.h"
#include "psyq/libc/rand.h"
#include "rr/audio_loop.h"
#include "rr/bios_devices.h"
#include "game/math/rotmat.h"
#include "rr/trig.h"
#include "rr/vehicle_drive.h"
#include "rr/cpu_work.h"
#ifdef RR4_OBJDBG_PRINT
#include <stdio.h>
#include <stdlib.h>
#include <psyz/system.h>
void RrFuncMarkWork(void);
#endif

typedef SemiTransSVec SVec;
typedef SemiTransCVec CVec;
typedef ObjectTrailPoint16 Vec16;


typedef SemiTransTri38 Poly38;
typedef SemiTransQuad48 Poly48;
extern void SubmitCourseModel2(void *, s32);
extern s32 OpeningSunbeamPolys[];
extern s32 OpeningCreditTextTable[];
extern SVec CourseLightColors[];
extern CVec OpeningSkipHoldTimer[];
extern s32 ReplayState;
extern s8 RacePaused;
extern s32 RaceState;
extern s32 TrackObjCullDistance;
extern s32 TrackObjVisibleFrame[];
extern Vec16 TrackObjWorldOrigin;


static RrWork *TrackObjectsWork;

/* A course object's SubmitModel by its own cost (rr/cpu_work.h), not the
 * course objects' per-operation weights: DuckStation's four calls in the
 * Grand Prix's countdown on course 7 take 57k-74k clocks of its 125k-143k
 * DrawTrackObjects, which the objects' weights charged 15k short (the
 * fly-by's frames, which make none, are unchanged). [mark] */
static void TrackObjectSubmitModel(RroSceneScratchPrefix *scene, s32 modelIndex) {
    static const RrWorkCost none;
    RrWork work;
    if (TrackObjectsWork) {
        RrWorkSpendPart(TrackObjectsWork, RR_GPU_SCRATCH->cursor, &RrCostDrawTrackObjects);
        RrWorkBegin(&work, RR_GPU_SCRATCH->cursor);
    }
    if (TrackObjectsWork)
        RrWorkArmEnd(&work, &RrCostSubmitModel);
    SubmitModel(scene, modelIndex, 0, 0);
    if (TrackObjectsWork)
        RrWorkSpendPart(TrackObjectsWork, RR_GPU_SCRATCH->cursor, &none);
}

/* The work so far charged before a course object's rand call, its sound
 * writes and its models, so they come after its own time (rr/cpu_work.h) */
static void TrackObjectsSpend(void) {
    if (TrackObjectsWork)
        RrWorkSpendPart(TrackObjectsWork, RR_GPU_SCRATCH->cursor, &RrCostDrawTrackObjects);
}

static s32 TrackObjectRand(void) {
    TrackObjectsSpend();
    return PsyqRand();
}

/* The course objects' own code by where it runs: each point below charges
 * the code from it to the next of the object's routines (rr/cpu_work.h),
 * fitted to DuckStation's clocks between those routines' marks. */
enum {
    DTO_SEG_TOP,
    DTO_SEG_Q,
    DTO_SEG_C_0,
    DTO_SEG_BOM_1,
    DTO_SEG_C_1,
    DTO_SEG_BOM_2,
    DTO_SEG_SM_1,
    DTO_SEG_C_2,
    DTO_SEG_C_3,
    DTO_SEG_C_4,
    DTO_SEG_C_5,
    DTO_SEG_C_6,
    DTO_SEG_F8A_1_6_8_9,
    DTO_SEG_F8_1_14,
    DTO_SEG_BOM_3,
    DTO_SEG_SM_2,
    DTO_SEG_F8_2,
    DTO_SEG_BOM_4,
    DTO_SEG_F8_3,
    DTO_SEG_BOM_5,
    DTO_SEG_F8_4_7,
    DTO_SEG_BOM_6,
    DTO_SEG_F8_5_6,
    DTO_SEG_F8_8,
    DTO_SEG_F8_9,
    DTO_SEG_F8_11,
    DTO_SEG_F8_12,
    DTO_SEG_F8_15,
    DTO_SEG_F8_10_13,
    DTO_SEG_FC_1,
    DTO_SEG_RAND_1,
    DTO_SEG_BOM_7,
    DTO_SEG_G3_1,
    DTO_SEG_G4_1,
    DTO_SEG_FC_2_5,
    DTO_SEG_BOM_8,
    DTO_SEG_G4_2,
    DTO_SEG_G4_3,
    DTO_SEG_FC_3,
    DTO_SEG_BOM_9,
    DTO_SEG_G4_4,
    DTO_SEG_FC_4,
    DTO_SEG_RAND_2,
    DTO_SEG_BOM_10,
    DTO_SEG_G3_2,
    DTO_SEG_RAND_3,
    DTO_SEG_BOM_11,
    DTO_SEG_G3_3,
    DTO_SEG_RAND_4,
    DTO_SEG_BOM_12,
    DTO_SEG_G3_4,
    DTO_SEG_COUNT
};
static const short DtoSegCycles[DTO_SEG_COUNT] = {
    [DTO_SEG_BOM_1] = 373,
    [DTO_SEG_BOM_10] = 52,
    [DTO_SEG_BOM_11] = 54,
    [DTO_SEG_BOM_12] = 160,
    [DTO_SEG_BOM_2] = 392,
    [DTO_SEG_BOM_4] = 252,
    [DTO_SEG_BOM_5] = 38,
    [DTO_SEG_BOM_7] = 63,
    [DTO_SEG_C_0] = -339,
    [DTO_SEG_C_1] = 31,
    [DTO_SEG_C_2] = 144,
    [DTO_SEG_C_3] = 9,
    [DTO_SEG_C_4] = 6,
    [DTO_SEG_C_6] = 206,
    [DTO_SEG_F8A_1_6_8_9] = 402,
    [DTO_SEG_F8_11] = 250,
    [DTO_SEG_F8_12] = 60,
    [DTO_SEG_F8_1_14] = 68,
    [DTO_SEG_F8_2] = -10,
    [DTO_SEG_F8_3] = 691,
    [DTO_SEG_FC_1] = 1383,
    [DTO_SEG_FC_4] = 226,
    [DTO_SEG_G3_1] = 1161,
    [DTO_SEG_G3_2] = 427,
    [DTO_SEG_G3_3] = 362,
    [DTO_SEG_G3_4] = 277,
    [DTO_SEG_G4_1] = 210,
    [DTO_SEG_Q] = 230,
    [DTO_SEG_RAND_1] = 1986,
    [DTO_SEG_RAND_2] = 962,
    [DTO_SEG_RAND_3] = 346,
    [DTO_SEG_RAND_4] = 211,
    [DTO_SEG_SM_1] = 68,
    [DTO_SEG_SM_2] = 590,
    [DTO_SEG_TOP] = -83,
};

#ifdef RR4_OBJDBG_PRINT
static int TrackObjectsDebug(void) {
    static int dbg = -1;
    if (dbg < 0) dbg = getenv("RR4_OBJDBG") != NULL;
    return dbg;
}
#endif

/* A point's charge; a negative one (the port's own charges there larger
 * than DuckStation's code) comes off the next */
static int TrackObjectsOwed;
static void TrackObjectsCharge(int cycles) {
    cycles += TrackObjectsOwed;
    TrackObjectsOwed = cycles < 0 ? cycles : 0;
    if (cycles > 0)
        RR_CPU_CYCLES((unsigned)cycles);
}

static void DtoSeg(int seg) {
    if (!TrackObjectsWork)
        return;
#ifdef RR4_OBJDBG_PRINT
    if (TrackObjectsDebug()) {
        fprintf(stderr, "XMARK SEG %llu %d\n", Psyz_EmulatedClocks(), seg);
        RrFuncMarkWork();
    }
#endif
    TrackObjectsCharge(DtoSegCycles[seg]);
}

/* The work since the last charge left out (a routine that charged its own) */
static void TrackObjectsDrop(void) {
    static const RrWorkCost none;
    if (TrackObjectsWork)
        RrWorkSpendPart(TrackObjectsWork, RR_GPU_SCRATCH->cursor, &none);
}

/* The objects' models by their own costs, their work out of the objects' */
#define TRACK_OBJECT_SUB(call, cost)                                           \
    do {                                                                       \
        RrWork subWork_;                                                       \
        TrackObjectsSpend();                                                   \
        if (TrackObjectsWork)                                                  \
            RrWorkBegin(&subWork_, RR_GPU_SCRATCH->cursor);                    \
        call;                                                                  \
        if (TrackObjectsWork) {                                                \
            RrWorkSpend(&subWork_, RR_GPU_SCRATCH->cursor, (cost));            \
            TrackObjectsDrop();                                                \
        }                                                                      \
    } while (0)

/* EmitSemiTransG3List and EmitSemiTransG4List by their own costs */
static void TrackObjectEmit(int quads, void *list, s32 count, s32 mode) {
    RrWork work;
    TrackObjectsSpend();
    if (TrackObjectsWork) {
        RrWorkBegin(&work, RR_GPU_SCRATCH->cursor);
        RrWorkArmEnd(&work, quads ? &RrCostEmitSemiTransG4 : &RrCostEmitSemiTransG3);
        if (count > 0) RrWorkCount[RR_WORK_EMIT_ITEMS] += (unsigned)count;
    }
    if (quads)
        EmitSemiTransG4List(list, count, mode);
    else
        EmitSemiTransG3List(list, count, mode);
    if (TrackObjectsWork)
        TrackObjectsDrop();
}
#define TrackObjectEmitG3(list, count, mode) TrackObjectEmit(0, (list), (count), (mode))
#define TrackObjectEmitG4(list, count, mode) TrackObjectEmit(1, (list), (count), (mode))

static void DrawTrackObjectsWork(s32 arg0) {
    MatrixWords32 sp18;
    MatrixWords32 sp38;
    MatrixWords32 sp58;
    MatrixWords32 sp78;
    SVec sp98;
    SVec spA0[15];
    Vec3sFields8 sp118;
    SVec sp120;
    CVec sp128[40];
    Vec16 sp1C8;
    Poly38 sp1D8[20];
    Poly48 sp638[80];
    s32 sp1CB8;
    s32 *s3;
    RroSceneScratchPrefix *scene;
    s32 i;
    s32 j;
    s32 k;
    s32 v;
    s32 dist;
    s32 pan;
    s32 q;

    PsyqMemset(&sp98, 0, sizeof(sp98));
    s3 = (s32 *)((u8 *)SCRATCHPAD + sizeof(RroSceneScratchPrefix));
    scene = (RroSceneScratchPrefix *)SCRATCHPAD;
    for (i = 0; i < TrackObjectCount; i++) {
        if (TrackObjectStates[i].f24 != 1) {
            continue;
        }
        if (arg0 == 2 && (TrackObjectDefs[i].f4 / 10) != 1) {
            continue;
        }
#ifdef RR4_OBJDBG_PRINT
        if (TrackObjectsWork && TrackObjectsDebug()) {
            fprintf(stderr, "XMARK OBJ %llu %d %d %d %d\n", Psyz_EmulatedClocks(), (int)i,
                    (int)(TrackObjectDefs[i].f4 % 10), (int)TrackObjectDefs[i].f8, (int)(TrackObjectDefs[i].fC % 10));
            RrFuncMarkWork();
        }
#endif
        DtoSeg(DTO_SEG_TOP);
        if (TrackObjectsWork) {
            RrWorkSpendPart(TrackObjectsWork, RR_GPU_SCRATCH->cursor, &RrCostDrawTrackObjects);
        }

        switch (TrackObjectDefs[i].f4 % 10) {
        case 0:
            DtoSeg(DTO_SEG_C_0);
            BuildObjectMatrix(i, (const s16 *)&TrackObjectStates[i].f1C, &sp18, arg0);
            DtoSeg(DTO_SEG_BOM_1);
            SetTrackObjectTransform(i, &sp18, arg0);
            if ((TrackObjectStates[i].f8 / 10) == 1) {
                s3[0x00 / 4] = TrackObjectStates[i].f8 + (CourseIndex % 4) * 3 - 11;
                s3[0x04 / 4] = (TrackObjectStates[i].f0 / 6) % OpeningSunbeamPolys[s3[0x00 / 4]];
                s3[0x08 / 4] = OpeningCreditTextTable[s3[0x00 / 4] * 12 + s3[0x04 / 4]];
                TRACK_OBJECT_SUB(SubmitCourseModel2((scene), TrackObjectDefs[i].f0 + s3[0x08 / 4]), &RrCostTrackObjectCourseModel);
                continue;
            }
            if (((TrackObjectStates[i].f4 / 100) - 1U) < 2U) {
                TRACK_OBJECT_SUB(SubmitCourseModel2((scene), TrackObjectDefs[i].f0 + (TrackObjectStates[i].f4 % 100)), &RrCostTrackObjectCourseModel);
            } else {
                TRACK_OBJECT_SUB(SubmitCourseModel2((scene), TrackObjectDefs[i].f0), &RrCostTrackObjectCourseModel);
            }
            break;

        case 1:
            DtoSeg(DTO_SEG_C_1);
            BuildObjectMatrix(i, (const s16 *)&TrackObjectStates[i].f1C, &sp18, 0);
            DtoSeg(DTO_SEG_BOM_2);
            sp38 = LightMatrix.matrix;
            if (CourseIndex == 7 || CourseIndex == 3 || CourseIndex == 2) {
                for (j = 0; j < 3; j++) {
                    LightMatrix.matrix.fields.rotation00[j][0] = TrackLightTable->lighting04.matrix.fields.vectors[j].x;
                    LightMatrix.matrix.fields.rotation00[j][1] = -TrackLightTable->lighting04.matrix.fields.vectors[j].y;
                    LightMatrix.matrix.fields.rotation00[j][2] = TrackLightTable->lighting04.matrix.fields.vectors[j].z;
                    sp78.fields.rotation00[0][j] = CourseLightColors[CourseIndex * 3 + j].vx;
                    sp78.fields.rotation00[1][j] = CourseLightColors[CourseIndex * 3 + j].vy;
                    sp78.fields.rotation00[2][j] = CourseLightColors[CourseIndex * 3 + j].vz;
                }
            } else {
                for (j = 0; j < 3; j++) {
                    LightMatrix.matrix.fields.rotation00[j][0] = TrackLightTable->lighting04.matrix.fields.vectors[j].x;
                    LightMatrix.matrix.fields.rotation00[j][1] = TrackLightTable->lighting04.matrix.fields.vectors[j].y;
                    LightMatrix.matrix.fields.rotation00[j][2] = TrackLightTable->lighting04.matrix.fields.vectors[j].z;
                    sp78.fields.rotation00[0][j] = CourseLightColors[CourseIndex * 3 + j].vx;
                    sp78.fields.rotation00[1][j] = CourseLightColors[CourseIndex * 3 + j].vy;
                    sp78.fields.rotation00[2][j] = CourseLightColors[CourseIndex * 3 + j].vz;
                }
            }
            SetBackColor(
                TrackLightTable->lighting04.ambient[0],
                TrackLightTable->lighting04.ambient[1],
                TrackLightTable->lighting04.ambient[2]);
            SetColorMatrix(&sp78);
            MulMatrix0(&LightMatrix.matrix, &sp18, &sp58);
            SetLightMatrix(&sp58);
            SetTrackObjectTransform(i, &sp18, 0);
            SelectModelBank(4);
            TrackObjectSubmitModel(scene, TrackObjectDefs[i].f0);
            DtoSeg(DTO_SEG_SM_1);
            break;

        case 2:
            DtoSeg(DTO_SEG_C_2);
            TRACK_OBJECT_SUB(DrawTrackObjectLitModel(TrackObjectDefs[i].f8, TrackObjectDefs[i].f0), &RrCostTrackObjectLit);
            continue;

        case 3:
            DtoSeg(DTO_SEG_C_3);
            TRACK_OBJECT_SUB(DrawTrackObjectBillboard(TrackObjectDefs[i].f0), &RrCostTrackObjectBillboard);
            continue;

        case 4:
            DtoSeg(DTO_SEG_C_4);
            TRACK_OBJECT_SUB(DrawTrackObjectFlashQuads(TrackObjectDefs[i].f0), &RrCostTrackObjectFlashQuads);
            continue;

        case 5:
            DtoSeg(DTO_SEG_C_5);
            if (ReplayState == 2 && GameModeType == 1 && PlayerCars[0].control.dynamics94.steering.count22 < 6 /* RaceFinishBannerTimer */) {
                continue;
            }
            if (TrackObjectStates[i].f8 != 0) {
                if (TrackObjectStates[i].f4 + 100 == TrackObjectStates[i].f0) {
                    if (RacePaused == 0 && GameModeType != 5) {
                        TrackObjectsSpend();
                        PlaySfxVolPan(0x27, 0x7F, 0x40, -1);
                    }
                }
                TRACK_OBJECT_SUB(DrawAnimatedTrackObject((AnimatedTrackScratch *)(scene), TrackObjectDefs[i].f0, RacePaused == 0), &RrCostTrackObjectAnimated);
                SetDrawMode(RR_GPU_SCRATCH->cursor, 0, 1, 0x2A, 0);
                AddPrim(&RR_GPU_SCRATCH->order[0x900 / 4], RR_GPU_SCRATCH->cursor);
                RR_GPU_SCRATCH->cursor += sizeof(GpuDrawModePacket);
            }
            continue;

        case 6:
            DtoSeg(DTO_SEG_C_6);
            break;
        }

        switch (TrackObjectDefs[i].f8) {
        case 1:
        case 6:
        case 8:
        case 9:
            DtoSeg(DTO_SEG_F8A_1_6_8_9);
            s3[0x00 / 4] = (TrackObjectStates[i].fC - (scene->view.projection.pose08.position00.xyz.x)) >> 4;
            s3[0x04 / 4] = (TrackObjectStates[i].f10 - (scene->view.projection.pose08.position00.xyz.y)) >> 4;
            s3[0x08 / 4] = (TrackObjectStates[i].f14 - (scene->view.projection.pose08.position00.xyz.z)) >> 4;
            dist = CordicSqrt((s3[0x00 / 4] * s3[0x00 / 4] + s3[0x04 / 4] * s3[0x04 / 4] + s3[0x08 / 4] * s3[0x08 / 4]) >> 12);
            if (dist < 0) {
                dist = -dist;
            }
            break;
        }

        switch (TrackObjectDefs[i].f8) {
        case 1:
        case 14:
            DtoSeg(DTO_SEG_F8_1_14);
            sp118.x = TrackObjectStates[i].f1C;
            sp118.y = (TrackObjectStates[i].f0 * 0x188) % 0x1000;
            sp118.z = TrackObjectStates[i].f20;
            BuildObjectMatrix(i, (const s16 *)&sp118, &sp18, 0);
            DtoSeg(DTO_SEG_BOM_3);
            MulMatrix0(&sp38, &sp18, &sp58);
            SetLightMatrix(&sp58);
            SetTrackObjectTransform(i, &sp18, 0);
            SelectModelBank(4);
            TrackObjectSubmitModel(scene, 1);
            DtoSeg(DTO_SEG_SM_2);
            if (TrackObjectDefs[i].f8 != 14) {
                v = (0x898 - dist) / 20;
                if (dist >= 0x899) {
                    v = 0;
                }
                s3[0x00 / 4] = TrackObjWorldOrigin.vx - (scene->view.projection.pose08.position00.xyz.x);
                s3[0x04 / 4] = TrackObjWorldOrigin.vy - (scene->view.projection.pose08.position00.xyz.y);
                s3[0x08 / 4] = TrackObjWorldOrigin.vz - (scene->view.projection.pose08.position00.xyz.z);
                s3[0x10 / 4] = CordicSqrt((s3[0x00 / 4] * s3[0x00 / 4] + s3[0x04 / 4] * s3[0x04 / 4] + s3[0x08 / 4] * s3[0x08 / 4]) >> 10) >> 2;
                if (dist < TrackObjCullDistance) {
                    s3[0x10 / 4] = -s3[0x10 / 4];
                }
                {
                    s32 t = ((0x4C8 - (s3[0x10 / 4] / 3)) * 0x1E00) / 1224;
                    if (s3[0x10 / 4] > -800 && s3[0x10 / 4] < 800) {
                        SoundLoopSet(5, t, v, 0x40);
                    }
                }
                TrackObjWorldOrigin = *(Vec16 *)&(scene->view.projection.pose08.position00.xyz.x);
                TrackObjCullDistance = dist;
            }
            break;

        case 2:
            DtoSeg(DTO_SEG_F8_2);
            sp118.x = 0;
            sp118.y = 0x400;
            sp118.z = 0;
            BuildObjectMatrix(i, (const s16 *)&sp118, &sp18, arg0);
            DtoSeg(DTO_SEG_BOM_4);
            SetTrackObjectTransform(i, &sp18, arg0);
            TRACK_OBJECT_SUB(SubmitCourseModel2((scene), 2), &RrCostTrackObjectCourseModel);
            break;

        case 3:
            DtoSeg(DTO_SEG_F8_3);
            sp118.x = sp118.y = sp118.z = 0;
            BuildObjectMatrix(i, (const s16 *)&sp118, &sp18, arg0);
            DtoSeg(DTO_SEG_BOM_5);
            SetTrackObjectTransform(i, &sp18, arg0);
            TRACK_OBJECT_SUB(SubmitCourseModel2((scene), 2), &RrCostTrackObjectCourseModel);
            break;

        case 4:
        case 7:
            DtoSeg(DTO_SEG_F8_4_7);
            sp118.x = sp118.y = sp118.z = 0;
            BuildObjectMatrix(i, (const s16 *)&sp118, &sp18, 0);
            DtoSeg(DTO_SEG_BOM_6);
            SetTrackObjectTransform(i, &sp18, 0);
            if (TrackObjectDefs[i].f8 == 4) {
                for (j = 2; j < 6; j++) {
                    TRACK_OBJECT_SUB(SubmitCourseModel2((scene), j), &RrCostTrackObjectCourseModel);
                }
            } else {
                for (j = 0x2F; j < 0x33; j++) {
                    TRACK_OBJECT_SUB(SubmitCourseModel2((scene), j), &RrCostTrackObjectCourseModel);
                }
            }
            break;

        case 5:
        case 6:
            DtoSeg(DTO_SEG_F8_5_6);
            RR_GTE_COPY_ROT_TR_TO_LLM_BK();
            {
                s32 one = 0x1000;
                RR_GTE_SET_IDENTITY_ROT_ZERO_TR(one);
            }
            TRACK_OBJECT_SUB(DrawTrackObjectModel(TrackObjectDefs[i].f0 + 1, 1), &RrCostTrackObjectModel);
            TRACK_OBJECT_SUB(DrawTrackObjectModel(TrackObjectDefs[i].f0 + 2, 0), &RrCostTrackObjectModel);
            if (TrackObjectDefs[i].f8 != 5) {
                v = (0xDB6 - dist) / 13;
                if (dist >= 0xDB7 || RacePaused != 0) {
                    SoundLoopSet(6, 0x1E00, 0, 0x40);
                    v = 0;
                    if (TrackObjectStates[i].f8 == 1) {
                        TrackObjectStates[i].f8 = 2;
                    }
                }
                if ((v >= 0x74 && TrackObjectStates[i].f8 == 0) || TrackObjectStates[i].f8 == 1) {
                    TrackObjectStates[i].f8 = 1;
                    SoundLoopSet(6, 0x1E00, v, 0x40);
                }
            }
            break;

        case 8:
            DtoSeg(DTO_SEG_F8_8);
            v = (0xC80 - dist) / 25;
            if (v >= 0x3D && TrackObjectStates[i].f8 == 0) {
                TrackObjectStates[i].f8 = 1;
                TrackObjectsSpend();
                PlaySfxVolPan(0x26, 0x7F, 0x40, -1);
            }
            break;

        case 9:
            DtoSeg(DTO_SEG_F8_9);
            v = (0x119E - dist) / 13;
            if (dist >= 0x119F || RacePaused != 0) {
                SoundLoopSet(6, 0x1E00, 0, 0x40);
                v = 0;
                if (TrackObjectStates[i].f8 == 1) {
                    TrackObjectStates[i].f8 = 2;
                }
            }
            if ((v >= 0x22 && TrackObjectStates[i].f8 == 0) || TrackObjectStates[i].f8 == 1) {
                TrackObjectStates[i].f8 = 1;
                SoundLoopSet(6, 0x1E00, v, 0x40);
            }
            break;

        case 11:
            DtoSeg(DTO_SEG_F8_11);
            if (RaceState == 0) {
                continue;
            }
            sp18.fields.translation14[0] = sp18.fields.translation14[1] = sp18.fields.translation14[2] = 0;
            sp118.x = ((s16)scene->view.projection.pose08.angles10.xyz.x);
            sp118.y = -((s16)scene->view.projection.pose08.angles10.xyz.y);
            sp118.z = ((s16)scene->view.projection.pose08.angles10.xyz.z);
            RotMatrix(&sp118, &sp18);
            RR_GTE_SETROT_TR(&sp18);
            if (((u16)CourseIndex - 2U) < 2U && (scene->view.projection.pose08.position00.xyz.x) <= 0x21EF7) {
                s3[0x2C / 4] = 2;
            } else {
                s3[0x2C / 4] = 0;
            }
            s3[0x14 / 4] = (scene->view.projection.pose08.position00.xyz.x) - TrackObjectKeyframes[TrackObjectDefs[i].f24 + s3[0x2C / 4]].f0C;
            s3[0x18 / 4] = (scene->view.projection.pose08.position00.xyz.x) - TrackObjectKeyframes[TrackObjectDefs[i].f24 + s3[0x2C / 4] + 1].f0C;
            if (s3[0x14 / 4] > 0) {
                sp1C8.vx = s3[0x14 / 4] >> 4;
                v = (0x2A30 - s3[0x14 / 4]) / 100;
            } else if (s3[0x18 / 4] < 0) {
                sp1C8.vx = s3[0x18 / 4] >> 4;
                v = (0x2A30 + s3[0x18 / 4]) / 100;
            } else {
                sp1C8.vx = 0;
                v = 0x6C;
            }
            if (v < 0) {
                v = 0;
            }
            sp1C8.vy = 0;
            sp1C8.vz = ((scene->view.projection.pose08.position00.xyz.z) - TrackObjectKeyframes[TrackObjectDefs[i].f24 + s3[0x2C / 4]].f14) >> 4;
            sp120.vx = sp1C8.vx;
            sp120.vy = sp1C8.vy;
            sp120.vz = sp1C8.vz;
            RotTrans((s32 *)&sp120, (s32 *)&sp1C8, &sp1CB8);
            s3[0x1C / 4] = ratan2(-sp1C8.vz, sp1C8.vx);
            pan = (rcos(s3[0x1C / 4]) / 64) + 0x40;
            if (pan >= 0x80) {
                pan = 0x7F;
            }
            if (((u16)GameModeType - 2U) < 3U) {
                if (TrackObjVisibleFrame[i] == ModeFrame) {
                    SetSoundChannelVolLR(1, (s16)((v * pan) >> 7), (s16)((v * (0x80 - pan)) >> 7));
                } else {
                    SetSoundChannelVolLR(0, (s16)((v * pan) >> 7), (s16)((v * (0x80 - pan)) >> 7));
                    SetSoundChannelVolLR(1, 0, 0);
                }
                TrackObjVisibleFrame[i] = ModeFrame;
            } else {
                SetSoundChannelVolLR(0, (s16)((v * pan) >> 7), (s16)((v * (0x80 - pan)) >> 7));
            }
            break;

        case 12:
            DtoSeg(DTO_SEG_F8_12);
            if (RaceState == 0) {
                continue;
            }
            s3[0x14 / 4] = (scene->view.projection.pose08.position00.xyz.x) - TrackObjectKeyframes[TrackObjectDefs[i].f24].f0C;
            pan = 0x40;
            s3[0x18 / 4] = (scene->view.projection.pose08.position00.xyz.x) - TrackObjectKeyframes[TrackObjectDefs[i].f24 + 1].f0C;
            if (s3[0x14 / 4] > 0) {
                v = (0x2A30 - s3[0x14 / 4]) / 100;
            } else if (s3[0x18 / 4] < 0) {
                v = (0x2A30 + s3[0x18 / 4]) / 100;
            } else {
                v = 0x6C;
            }
            if (v < 0) {
                v = 0;
            }
            if (((u16)GameModeType - 2U) < 3U) {
                if (TrackObjVisibleFrame[i] == ModeFrame) {
                    SetSoundChannelVolLR(1, (s16)((v * pan) >> 7), (s16)((v * (0x80 - pan)) >> 7));
                } else {
                    SetSoundChannelVolLR(0, (s16)((v * pan) >> 7), (s16)((v * (0x80 - pan)) >> 7));
                    SetSoundChannelVolLR(1, 0, 0);
                }
                TrackObjVisibleFrame[i] = ModeFrame;
            } else {
                SetSoundChannelVolLR(0, (s16)((v * pan) >> 7), (s16)((v * (0x80 - pan)) >> 7));
            }
            break;

        case 15:
            DtoSeg(DTO_SEG_F8_15);
            SetSoundChannelVolLR(0, 0, 0);
            SetSoundChannelVolLR(1, 0, 0);
            break;

        case 10:
        case 13:
            DtoSeg(DTO_SEG_F8_10_13);
            break;
        }

        q = TrackObjectStates[i].f4 / 100;
        DtoSeg(DTO_SEG_Q);
        if (q == 3) {
            TrackObjectStates[i].f4 = (TrackObjectStates[i].f4 - 300 < 50) ? TrackObjectStates[i].f4 + 10 : TrackObjectStates[i].f4;
        } else if (q == 0) {
            TrackObjectStates[i].f4 = (TrackObjectStates[i].f4 > 0) ? TrackObjectStates[i].f4 - 5 : TrackObjectStates[i].f4;
        }

        if ((TrackObjectStates[i].f4 % 100) == 0) {
            continue;
        }

        switch (TrackObjectDefs[i].fC % 10) {
        case 1:
            DtoSeg(DTO_SEG_FC_1);
            if (arg0 == 2) {
                s3[0x20 / 4] = -1;
            } else {
                s3[0x20 / 4] = 1;
            }
            sp1CB8 = FindTrackNodeAtXZ(TrackObjectStates[i].fC, TrackObjectStates[i].f14);
            for (j = 0; j < 8; j++) {
                spA0[j].vy = (TrackNodes[sp1CB8].road.height - TrackObjectStates[i].f10) >> 2;
                spA0[j].vx = (((spA0[j].vy >> 1)) * rcos(TrackObjectStates[i].f0 * 57 + j * 512)) / 4096;
                spA0[j].vz = (((spA0[j].vy >> 1)) * rsin(TrackObjectStates[i].f0 * 57 + j * 512)) / 4096 + (((spA0[j].vy >> 1)) + 250) * s3[0x20 / 4];
                sp128[j].b = 0;
                sp128[j].g = 0;
                sp128[j].r = 0;
            }
            {
                k = (TrackObjectRand() % 10) + (TrackObjectStates[i].f4 % 100);
                DtoSeg(DTO_SEG_RAND_1);
                if (RacePaused != 0) {
                    k = TrackObjectStates[i].f4 % 100;
                }
                for (j = 0; j < 4; j++) {
                    sp1D8[j].v0.vy = 20;
                    sp1D8[j].v0.vx = 0;
                    sp1D8[j].v0.vz = s3[0x20 / 4] * 50;
                    sp1D8[j].c0.r = sp1D8[j].c0.g = k;
                    sp1D8[j].c0.b = (k * 3) / 4;
                    sp1D8[j].v1.vx = (spA0[j].vx >> 1);
                    sp1D8[j].v1.vy = (spA0[j].vy >> 1);
                    sp1D8[j].v1.vz = (spA0[j].vz >> 1);
                    sp1D8[j].v2.vx = (spA0[(j + 4) % 8].vx >> 1);
                    sp1D8[j].v2.vy = (spA0[(j + 4) % 8].vy >> 1);
                    sp1D8[j].v2.vz = (spA0[(j + 4) % 8].vz >> 1);
                    sp1D8[j].c1 = sp128[j];
                    sp1D8[j].c2 = sp128[(j + 4) % 8];
                }
                for (j = 0; j < 8; j++) {
                    sp1D8[j + 4].v0.vx = 0;
                    sp1D8[j + 4].v0.vy = (TrackNodes[sp1CB8].road.height - TrackObjectStates[i].f10) >> 2;
                    sp1D8[j + 4].v0.vz = ((spA0[j].vy / 4) + 250) * s3[0x20 / 4];
                    sp1D8[j + 4].v1.vx = (spA0[j].vx >> 1);
                    sp1D8[j + 4].v1.vy = spA0[j].vy;
                    sp1D8[j + 4].v1.vz = ((spA0[j].vz >> 1)) + 180;
                    sp1D8[j + 4].v2.vx = (spA0[(j + 1) % 8].vx >> 1);
                    sp1D8[j + 4].v2.vy = spA0[(j + 1) % 8].vy;
                    sp1D8[j + 4].v2.vz = ((spA0[(j + 1) % 8].vz >> 1)) + 180;
                    sp1D8[j + 4].c0.r = sp1D8[j + 4].c0.g = k + 5;
                    sp1D8[j + 4].c0.b = (k * 3) / 4;
                    sp1D8[j + 4].c1.r = sp1D8[j + 4].c1.g = (k + 5) >> 1;
                    sp1D8[j + 4].c1.b = (k * 3) >> 3;
                    sp1D8[j + 4].c2.r = sp1D8[j + 4].c2.g = (k + 5) >> 1;
                    sp1D8[j + 4].c2.b = (k * 3) >> 3;
                }
                BuildObjectMatrix(i, (const s16 *)&TrackObjectStates[i].f1C, &sp18, arg0);
                DtoSeg(DTO_SEG_BOM_7);
                SetRotMatrix(sp18.w);
                TrackObjectEmitG3(sp1D8, 12, 1);
                DtoSeg(DTO_SEG_G3_1);
                for (j = 0; j < 8; j++) {
                    sp638[j].v0.vx = (spA0[j].vx >> 1);
                    sp638[j].v0.vy = spA0[j].vy;
                    sp638[j].v0.vz = ((spA0[j].vz >> 1)) + 180;
                    sp638[j].v1.vx = (spA0[(j + 1) % 8].vx >> 1);
                    sp638[j].v1.vy = spA0[(j + 1) % 8].vy;
                    sp638[j].v1.vz = ((spA0[(j + 1) % 8].vz >> 1)) + 180;
                    sp638[j].v2.vx = spA0[j].vx;
                    sp638[j].v2.vy = spA0[j].vy;
                    sp638[j].v2.vz = spA0[j].vz;
                    sp638[j].v3.vx = spA0[(j + 1) % 8].vx;
                    sp638[j].v3.vy = spA0[(j + 1) % 8].vy;
                    sp638[j].v3.vz = spA0[(j + 1) % 8].vz;
                    sp638[j].c0.r = sp638[j].c0.g = (k + 5) >> 1;
                    sp638[j].c0.b = (k * 3) >> 3;
                    sp638[j].c1.r = sp638[j].c1.g = (k + 5) >> 1;
                    sp638[j].c1.b = (k * 3) >> 3;
                    sp638[j].c2.r = sp638[j].c2.g = sp638[j].c2.b = 0;
                    sp638[j].c3.r = sp638[j].c3.g = sp638[j].c3.b = 0;
                }
                TrackObjectEmitG4(sp638, 8, 1);
                DtoSeg(DTO_SEG_G4_1);
            }
            break;

        case 2:
        case 5:
            DtoSeg(DTO_SEG_FC_2_5);
            s3[0x28 / 4] = TrackObjectDefs[i].fC / 10;
            for (k = 0; k < 18; k++) {
                if (s3[0x28 / 4] == 0) {
                    if (CourseIndex == 0) {
                        sp128[k].r = (k < 9) ? 20 : 20 - (k - 8) * 2;
                        sp128[k].g = (k < 9) ? 20 : 20 - (k - 8) * 2;
                        sp128[k].b = (k < 9) ? 10 : 18 - k;
                    } else {
                        sp128[k].r = (k < 9) ? 30 : 30 - (k - 8) * 3;
                        sp128[k].g = (k < 9) ? 30 : 30 - (k - 8) * 3;
                        sp128[k].b = (k < 9) ? 20 : 20 - (k - 8) * 2;
                    }
                } else if (s3[0x28 / 4] == 1) {
                    sp128[k].r = (k < 9) ? 70 : 70 - (k - 8) * 7;
                    sp128[k].g = (k < 9) ? 20 : 20 - (k - 8) * 2;
                    sp128[k].b = 0;
                } else if (s3[0x28 / 4] == 2) {
                    sp128[k].r = 0;
                    sp128[k].g = (k < 9) ? 20 : 20 - (k - 8) * 2;
                    sp128[k].b = (k < 9) ? 30 : 30 - (k - 8) * 3;
                }
                s3[0x2C / 4] = (k == 0) ? 5 : 10;
                for (j = 0; j < 2; j++) {
                    s3[0x30 / 4] = k * 2 + j;
                    s3[0x34 / 4] = s3[0x28 / 4] * 16 + s3[0x28 / 4] * 4 + k + 1;
                    sp638[s3[0x30 / 4]].v0.vx = TrackObjectTrailPoints[s3[0x34 / 4]].vx;
                    sp638[s3[0x30 / 4]].v0.vy = TrackObjectTrailPoints[s3[0x34 / 4]].vy - s3[0x2C / 4] * j;
                    sp638[s3[0x30 / 4]].v0.vz = TrackObjectTrailPoints[s3[0x34 / 4]].vz - s3[0x2C / 4] * (1 - j);
                    sp638[s3[0x30 / 4]].v1.vx = TrackObjectTrailPoints[s3[0x34 / 4]].vx;
                    sp638[s3[0x30 / 4]].v1.vy = TrackObjectTrailPoints[s3[0x34 / 4]].vy + s3[0x2C / 4] * j;
                    sp638[s3[0x30 / 4]].v1.vz = TrackObjectTrailPoints[s3[0x34 / 4]].vz + s3[0x2C / 4] * (1 - j);
                    sp638[s3[0x30 / 4]].v2.vx = TrackObjectTrailPoints[s3[0x34 / 4] + 1].vx;
                    sp638[s3[0x30 / 4]].v2.vy = TrackObjectTrailPoints[s3[0x34 / 4] + 1].vy - j * 10;
                    sp638[s3[0x30 / 4]].v2.vz = TrackObjectTrailPoints[s3[0x34 / 4] + 1].vz - (1 - j) * 10;
                    sp638[s3[0x30 / 4]].v3.vx = TrackObjectTrailPoints[s3[0x34 / 4] + 1].vx;
                    sp638[s3[0x30 / 4]].v3.vy = TrackObjectTrailPoints[s3[0x34 / 4] + 1].vy + j * 10;
                    sp638[s3[0x30 / 4]].v3.vz = TrackObjectTrailPoints[s3[0x34 / 4] + 1].vz + (1 - j) * 10;
                    sp638[s3[0x30 / 4]].c0 = sp638[s3[0x30 / 4]].c1 = (k == 0) ? sp128[0] : sp128[k - 1];
                    sp638[s3[0x30 / 4]].c2 = sp638[s3[0x30 / 4]].c3 = sp128[k];
                }
            }
            BuildObjectMatrix(i, (const s16 *)&sp98, &sp18, arg0);
            DtoSeg(DTO_SEG_BOM_8);
            sp120.vx = -(scene->view.projection.pose08.position00.xyz.x) >> 3;
            sp120.vy = -(scene->view.projection.pose08.position00.xyz.y) >> 3;
            sp120.vz = -(scene->view.projection.pose08.position00.xyz.z) >> 3;
            ApplyMatrix((MatrixWords32 *)SCRATCH_ROT_MATRIX, (const Vec3s8 *)(&sp120), (Vec3i *)(&sp1C8));
            sp38.fields.translation14[0] = sp1C8.vx;
            sp38.fields.translation14[1] = sp1C8.vy;
            sp38.fields.translation14[2] = sp1C8.vz;
            RR_GTE_SETROT(&sp18);
            RR_GTE_SETTR_AT20(&sp38);
            if ((TrackObjectDefs[i].fC % 10) == 5) {
                TrackObjectEmitG4(sp638, 36, 2);
                DtoSeg(DTO_SEG_G4_2);
            } else {
                TrackObjectEmitG4(sp638, 36, 0);
                DtoSeg(DTO_SEG_G4_3);
            }
            break;

        case 3:
            DtoSeg(DTO_SEG_FC_3);
            for (k = 0; k < 38; k++) {
                sp128[k].r = (k < 21) ? OpeningSkipHoldTimer[CourseIndex - 4].r : OpeningSkipHoldTimer[CourseIndex - 4].r - (OpeningSkipHoldTimer[CourseIndex - 4].r * (k - 20)) / 17;
                sp128[k].g = (k < 21) ? OpeningSkipHoldTimer[CourseIndex - 4].g : OpeningSkipHoldTimer[CourseIndex - 4].g - (OpeningSkipHoldTimer[CourseIndex - 4].g * (k - 20)) / 17;
                sp128[k].b = (k < 21) ? OpeningSkipHoldTimer[CourseIndex - 4].b : OpeningSkipHoldTimer[CourseIndex - 4].b - (OpeningSkipHoldTimer[CourseIndex - 4].b * (k - 20)) / 17;
                for (j = 0; j < 2; j++) {
                    s3[0x38 / 4] = k * 2 + j;
                    sp638[s3[0x38 / 4]].v0.vx = TrackObjectTrailPoints[k + 1].vx;
                    sp638[s3[0x38 / 4]].v0.vy = TrackObjectTrailPoints[k + 1].vy;
                    sp638[s3[0x38 / 4]].v0.vz = TrackObjectTrailPoints[k + 1].vz;
                    sp638[s3[0x38 / 4]].v1.vx = TrackObjectTrailPoints[k + 1].vx;
                    sp638[s3[0x38 / 4]].v1.vy = TrackObjectTrailPoints[k + 1].vy - (k * 2 + 25) + (k * 4 + 50) * (1 - j);
                    sp638[s3[0x38 / 4]].v1.vz = TrackObjectTrailPoints[k + 1].vz;
                    sp638[s3[0x38 / 4]].v2.vx = TrackObjectTrailPoints[k + 2].vx;
                    sp638[s3[0x38 / 4]].v2.vy = TrackObjectTrailPoints[k + 2].vy;
                    sp638[s3[0x38 / 4]].v2.vz = TrackObjectTrailPoints[k + 2].vz;
                    sp638[s3[0x38 / 4]].v3.vx = TrackObjectTrailPoints[k + 2].vx;
                    sp638[s3[0x38 / 4]].v3.vy = TrackObjectTrailPoints[k + 2].vy - (k * 2 + 25) + (k * 4 + 50) * (1 - j);
                    sp638[s3[0x38 / 4]].v3.vz = TrackObjectTrailPoints[k + 2].vz;
                    sp638[s3[0x38 / 4]].c0 = sp638[s3[0x38 / 4]].c2 = (k == 0) ? sp128[0] : sp128[k - 1];
                    sp638[s3[0x38 / 4]].c1 = sp638[s3[0x38 / 4]].c3 = sp128[37];
                }
            }
            for (j = 0; j < 2; j++) {
                sp638[j + 76].v0.vx = TrackObjectStates[i].fC >> 3;
                sp638[j + 76].v0.vy = TrackObjectStates[i].f10 >> 3;
                sp638[j + 76].v0.vz = TrackObjectStates[i].f14 >> 3;
                sp638[j + 76].v1.vx = TrackObjectStates[i].fC >> 3;
                sp638[j + 76].v1.vy = TrackObjectStates[i].f10 >> 3;
                sp638[j + 76].v1.vz = TrackObjectStates[i].f14 >> 3;
                sp638[j + 76].v2.vx = TrackObjectTrailPoints[1].vx;
                sp638[j + 76].v2.vy = TrackObjectTrailPoints[1].vy;
                sp638[j + 76].v2.vz = TrackObjectTrailPoints[1].vz;
                sp638[j + 76].v3.vx = TrackObjectTrailPoints[1].vx;
                sp638[j + 76].v3.vy = TrackObjectTrailPoints[1].vy - 25 + 50 * (1 - j);
                sp638[j + 76].v3.vz = TrackObjectTrailPoints[1].vz;
                sp638[j + 76].c0 = sp638[j + 76].c2 = sp128[1];
                sp638[j + 76].c1 = sp638[j + 76].c3 = sp128[37];
            }
            BuildObjectMatrix(i, (const s16 *)&sp98, &sp18, arg0);
            DtoSeg(DTO_SEG_BOM_9);
            sp120.vx = -(scene->view.projection.pose08.position00.xyz.x) >> 3;
            sp120.vy = -(scene->view.projection.pose08.position00.xyz.y) >> 3;
            sp120.vz = -(scene->view.projection.pose08.position00.xyz.z) >> 3;
            ApplyMatrix((MatrixWords32 *)SCRATCH_ROT_MATRIX, (const Vec3s8 *)(&sp120), (Vec3i *)(&sp1C8));
            sp38.fields.translation14[0] = sp1C8.vx;
            sp38.fields.translation14[1] = sp1C8.vy;
            sp38.fields.translation14[2] = sp1C8.vz;
            RR_GTE_SETROT(&sp18);
            RR_GTE_SETTR_AT20(&sp38);
            TrackObjectEmitG4(sp638, 78, 0);
            DtoSeg(DTO_SEG_G4_4);
            break;

        case 4:
            DtoSeg(DTO_SEG_FC_4);
            {
                s32 dx = (TrackObjectStates[i].fC - (scene->view.projection.pose08.position00.xyz.x)) >> 4;
                s32 dz = (TrackObjectStates[i].f14 - (scene->view.projection.pose08.position00.xyz.z)) >> 4;
                s3[0x00 / 4] = CordicSqrt((dx * dx + dz * dz) >> 12);
            }
            if (((u16)GameModeType - 2U) < 3U) {
                if (TrackObjVisibleFrame[i] != ModeFrame && RacePaused == 0) {
                    TrackObjectStates[i].f8 = (TrackObjectStates[i].f8 + (((i & 1) == 0) ? 0x20 : 0xFE0)) % 0x1000;
                }
                TrackObjVisibleFrame[i] = ModeFrame;
            } else if (RacePaused == 0) {
                TrackObjectStates[i].f8 = (TrackObjectStates[i].f8 + (((i & 1) == 0) ? 0x20 : 0xFE0)) % 0x1000;
            }
            if (s3[0x00 / 4] < 900) {
                for (j = 0; j < 2; j++) {
                    spA0[j].vx = 0;
                    spA0[j].vy = -1000 - s3[0x00 / 4];
                    { s32 t = ((((s3[0x00 / 4] - 500) * 3 >> 2) + 900) * rsin(TrackObjectStates[i].f8) >> 12) - 400; spA0[j].vz = j * (((s3[0x00 / 4] - 500) / 7) + 400) + t; }
                    sp128[j].b = 0;
                    sp128[j].g = 0;
                    sp128[j].r = 0;
                }
                {
                    k = (TrackObjectRand() % 10) + (TrackObjectStates[i].f4 % 100);
                    DtoSeg(DTO_SEG_RAND_2);
                if (RacePaused != 0) {
                    k = TrackObjectStates[i].f4 % 100;
                }
                    for (j = 0; j < 3; j++) {
                        sp1D8[j].v0.vy = 20;
                        sp1D8[j].v0.vz = 50;
                        sp1D8[j].v0.vx = 0;
                        sp1D8[j].c0.r = k >> 1;
                        sp1D8[j].c0.g = sp1D8[j].c0.b = (k * 2) / 3;
                        sp1D8[j].v1 = spA0[0];
                        sp1D8[j].v2 = spA0[1];
                        sp1D8[j].c1 = sp128[0];
                        sp1D8[j].c2 = sp128[1];
                        switch (j) {
                        case 0:
                            sp1D8[0].v1 = spA0[0];
                            sp1D8[0].v2 = spA0[1];
                            break;
                        case 1:
                        case 2:
                            s3[0x20 / 4] = 200 - (s3[0x00 / 4] - 500) / 14;
                            s3[0x24 / 4] = (s3[0x20 / 4] * 17) / 10;
                            sp1D8[j].v2.vx = -(u16)s3[0x24 / 4];
                            sp1D8[j].v2.vy = spA0[0].vy;
                            sp1D8[j].v2.vz = spA0[0].vz + (u16)s3[0x20 / 4];
                            sp1D8[j].v1 = spA0[j - 1];
                            break;
                        }
                    }
                }
                BuildObjectMatrix(i, (const s16 *)&TrackObjectStates[i].f1C, &sp18, arg0);
                DtoSeg(DTO_SEG_BOM_10);
                RR_GTE_SETROT(&sp18);
                TrackObjectEmitG3(sp1D8, 3, 0);
                DtoSeg(DTO_SEG_G3_2);
            } else if (s3[0x00 / 4] < 3300) {
                for (j = 0; j < 2; j++) {
                    spA0[j].vx = 0;
                    spA0[j].vy = -1000 - s3[0x00 / 4];
                    { s32 t = ((((s3[0x00 / 4] - 500) * 3 >> 2) + 900) * rsin(TrackObjectStates[i].f8) >> 12) - 400; spA0[j].vz = j * (((s3[0x00 / 4] - 500) / 7) + 400) + t; }
                    sp128[j].b = 0;
                    sp128[j].g = 0;
                    sp128[j].r = 0;
                }
                {
                    k = (TrackObjectRand() % 10) + (TrackObjectStates[i].f4 % 100);
                    DtoSeg(DTO_SEG_RAND_3);
                if (RacePaused != 0) {
                    k = TrackObjectStates[i].f4 % 100;
                }
                    sp1D8[0].v0.vy = 20;
                    sp1D8[0].v0.vx = 0;
                    sp1D8[0].v0.vz = 50;
                    sp1D8[0].c0.r = k;
                    sp1D8[0].c0.g = sp1D8[0].c0.b = (k * 4) / 3;
                    sp1D8[0].v1 = spA0[0];
                    sp1D8[0].v2 = spA0[1];
                    sp1D8[0].c1 = sp128[0];
                    sp1D8[0].c2 = sp128[1];
                    sp1D8[0].v1 = spA0[0];
                    sp1D8[0].v2 = spA0[1];
                }
                BuildObjectMatrix(i, (const s16 *)&TrackObjectStates[i].f1C, &sp18, arg0);
                DtoSeg(DTO_SEG_BOM_11);
                RR_GTE_SETROT(&sp18);
                TrackObjectEmitG3(sp1D8, 1, 0);
                DtoSeg(DTO_SEG_G3_3);
            } else {
                for (j = 0; j < 2; j++) {
                    spA0[j].vx = 0;
                    spA0[j].vy = -350 - (s3[0x00 / 4] >> 1);
                    { s32 t = ((rsin(TrackObjectStates[i].f8) * 3000) >> 12) - 400; spA0[j].vz = j * 800 + t; }
                    sp128[j].b = 0;
                    sp128[j].g = 0;
                    sp128[j].r = 0;
                }
                {
                    k = (TrackObjectRand() % 10) + (TrackObjectStates[i].f4 % 100);
                    DtoSeg(DTO_SEG_RAND_4);
                if (RacePaused != 0) {
                    k = TrackObjectStates[i].f4 % 100;
                }
                    if (s3[0x00 / 4] >= 5801) {
                        k = (k * 500) / (s3[0x00 / 4] - 5300);
                    }
                    {
                        s32 x, y, z;
                        x = TrackObjectStates[i].fC >> 3;
                        sp1D8[0].v0.vx = x;
                        y = TrackObjectStates[i].f10 >> 3;
                        sp1D8[0].v0.vy = y;
                        z = TrackObjectStates[i].f14 >> 3;
                        sp1D8[0].v1.vx = spA0[0].vx + x;
                        sp1D8[0].v1.vy = spA0[0].vy + y;
                        sp1D8[0].c0.r = k;
                        sp1D8[0].v2.vx = spA0[1].vx + x;
                        sp1D8[0].v2.vy = spA0[1].vy + y;
                        sp1D8[0].v0.vz = z;
                        sp1D8[0].v1.vz = z + (spA0[0].vz >> 1);
                        sp1D8[0].v2.vz = z + (spA0[1].vz >> 1);
                    }
                    sp1D8[0].c0.g = sp1D8[0].c0.b = (k * 4) / 3;
                    sp1D8[0].c1 = sp128[0];
                    sp1D8[0].c2 = sp128[1];
                }
                BuildObjectMatrix(i, (const s16 *)&sp98, &sp18, arg0);
                DtoSeg(DTO_SEG_BOM_12);
                sp120.vx = -(scene->view.projection.pose08.position00.xyz.x) >> 3;
                sp120.vy = -(scene->view.projection.pose08.position00.xyz.y) >> 3;
                sp120.vz = -(scene->view.projection.pose08.position00.xyz.z) >> 3;
                ApplyMatrix((MatrixWords32 *)SCRATCH_ROT_MATRIX, (const Vec3s8 *)(&sp120), (Vec3i *)(&sp1C8));
                sp38.fields.translation14[0] = sp1C8.vx;
                sp38.fields.translation14[1] = sp1C8.vy;
                sp38.fields.translation14[2] = sp1C8.vz;
                RR_GTE_SETROT(&sp18);
                RR_GTE_SETTR_AT20(&sp38);
                TrackObjectEmitG3(sp1D8, 1, 2);
                DtoSeg(DTO_SEG_G3_4);
            }
            break;
        }
    }
}

/* With its PS1 time (rr/cpu_work.h), object by object: the course objects'
 * rand calls and sound writes come where the objects before them put them
 * (DuckStation: the Grand Prix fly-by's first 4.2k-4.6k clocks into the
 * call, the port's at 1.4k with the whole call's time at its end). */
void DrawTrackObjects(s32 arg0) {
    RrWork work;
    RR_CPU_CYCLES(RR_CYCLES_DRAW_TRACK_OBJECTS_START);
    RrWorkBegin(&work, RR_GPU_SCRATCH->cursor);
    RrWorkPush(&work, 1, &RrCostDrawTrackObjects);
    TrackObjectsWork = &work;
    TrackObjectsOwed = 0;
    DrawTrackObjectsWork(arg0);
    TrackObjectsWork = NULL;
    RrWorkPop();
    RrWorkSpend(&work, RR_GPU_SCRATCH->cursor, &RrCostDrawTrackObjects);
}

void BuildObjectMatrix(int unused, const s16 *p, MatrixWords32 *out, int flag) {
    MatrixWords32 m10, m30, m50, m70;
    MatrixWords32 m90;
    MatrixWords32 *q = &m70;
    (void)unused;
    TrackObjectsSpend();
    BuildRotMatrixZ(q, -p[2]);
    BuildRotMatrixX(&m30, -p[0]);
    if (flag == 2) {
        m90 = (*(MatrixWords32 *)SCRATCH_ROT_MATRIX);
        BuildRotMatrixY(&m50, -p[1] - 0x800);
        m90.halves[6] = -m90.halves[6];
        m90.halves[7] = -m90.halves[7];
        m90.halves[8] = -m90.halves[8];
        MulMatrix2(&m90, q);
    } else {
        BuildRotMatrixY(&m50, -p[1]);
        MulMatrix2((MatrixWords32 *)SCRATCH_ROT_MATRIX, q);
    }
    MulMatrix0(&m30, &m50, &m10);
    MulMatrix2(&m70, &m10);
    *out = m10;
    if (TrackObjectsWork) {
        TrackObjectsDrop();
        RR_CPU_CYCLES(RR_CYCLES_BUILD_OBJECT_MATRIX);
    }
}

void SetTrackObjectTransform(int index, MatrixWords32 *matrix, int mode)
{
    Vec3s8 relative;
    VectorWords16 transformed;
    RroSceneScratchPrefix *scene = (RroSceneScratchPrefix *)SCRATCHPAD;
    int x;
    RR_GTE_SETROT(CameraViewMatrix.w);
    scene->draw74.draw.counter = 0;
    relative.fields.x = (s32)((u32)TrackObjectStates[index].fC - scene->view.projection.pose08.position00.xyz.x) >> 2;
    relative.fields.y = (s32)((u32)TrackObjectStates[index].f10 - scene->view.projection.pose08.position00.xyz.y) >> 2;
    relative.fields.z = (s32)((u32)TrackObjectStates[index].f14 - scene->view.projection.pose08.position00.xyz.z) >> 2;
    ApplyRotMatrix(&relative, &transformed.xyz);
    if (mode == 2) x = -transformed.xyz.x;
    else x = transformed.xyz.x;
    matrix->w[5] = x * 4U;
    matrix->w[6] = transformed.xyz.y * 4U;
    matrix->w[7] = transformed.xyz.z * 4U;
    RR_GTE_SETTR_AT20(matrix);
    RR_GTE_SETROT(matrix);
}
s32 LoadTrackObjects(void *data, S_ECB4 *states) {
    int count = ((TrackObjectFile *)data)->count;
    int bytes;
    data = (u8 *)data + RR_FIELD_OFFSET(TrackObjectFile,definitions);
    TrackObjMeshTablePtr = (S_ECB0 *)data;
    TrackObjectDefs = (S_ECB0 *)data;
    TrackObjInstanceStates = states;
    TrackObjectStates = states;
    data = (u8 *)data + count * sizeof(S_ECB0);
    TrackObjectCount = count;
    TrackObjSplineNodes = (Node24 *)data;
    bytes = count * sizeof(*states);
    TrackObjectKeyframes = (Node24 *)data;
    PsyqBzero(states, bytes);
    return bytes;
}

/* Initial values of the globals this module owns. */
s32 OpeningSunbeamPolys[15] = {10, 8, 10, 10, 10, 10, 10, 10, 10, 12, 12, 12, 12, 12, 4};
s32 OpeningCreditTextTable[172] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 10, 11, 12, 13, 14, 15, 16, 17, 0, 0, 0, 0, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 0, 0, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 0, 0, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 12, 13, 13, 14, 14, 18, 18, 14, 14, 18, 18, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 15, 15, 13, 13, 16, 16, 18, 18, 16, 16, 18, 18, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 17, 17, 18, 18};
SVec CourseLightColors[24] = {{.vx = 375, .vy = 369, .vz = 283}, {.vx = 375, .vy = 369, .vz = 283}, {.vx = 162, .vy = 157, .vz = 159}, {.vx = 230, .vy = 187, .vz = 255}, {.vx = 230, .vy = 187, .vz = 255}, {0}, {.vx = 180, .vy = 320, .vz = 300}, {.vx = 180, .vy = 320, .vz = 300}, {0}, {.vx = 384, .vy = 360, .vz = 220}, {.vx = 386, .vy = 338, .vz = 202}, {0}, {.vx = 200, .vy = 110, .vz = 30}, {.vx = 200, .vy = 110, .vz = 30}, {0}, {.vx = 300, .vy = 300, .vz = 192}, {.vx = 300, .vy = 300, .vz = 192}, {0}, {.vx = 255, .vy = 255, .vz = 180}, {.vx = 255, .vy = 255, .vz = 155}, {0}, {.vx = 206, .vy = 510, .vz = 510}, {.vx = 206, .vy = 510, .vz = 510}};
CVec OpeningSkipHoldTimer[2] = {{.r = 0x3C, .g = 0x28, .b = 0x1E}, {.r = 0x1E, .g = 0x1E, .b = 0x14}};
s16 CourseIndex;
s8 RacePaused;
TrackLighting2C *TrackLightTable;
s32 TrackObjCullDistance;
s32 TrackObjVisibleFrame[32];
Vec16 TrackObjWorldOrigin;
Node24 *TrackObjSplineNodes;
S_ECB0 *TrackObjMeshTablePtr;
S_ECB4 *TrackObjInstanceStates;
ObjectTrailPoint16 TrackObjectTrailPoints[60];
int TrackObjectCount;
