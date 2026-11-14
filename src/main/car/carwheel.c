#include "rr/frame_interp.h"
#include "rr/rro_model.h"
#include "game/gfx/model.h"
#include "rr/track_lighting.h"
#include "psyq/libgte/gtemat4.h"
#include "psyq/libgte/gtemat.h"
#include "rr/matrix_workspace.h"
#include "game/camera/viewcam.h"
#include "types.h"
#include "game/car/bounds.h"
#include "scratchpad.h"
#include "rr/lookup.h"
#include "rr_asm.h"
#include "game/data/cdfile2.h"
#include "game/gfx/gfxmtx.h"
#include "game/gfx/gtepoly1.h"
#include "psyq/libgte/gtemat3.h"
#include "game/race/raceinit.h"
#include "game/car/carwheel.h"
#include "rr/race_setup.h"
#include "rr/player_cars.h"
#include "game/race/racemai2.h"
#include "game/math/rotmat.h"

#define G ((Scratch2D784 *)SCRATCHPAD)



typedef struct {
    s16 d0, d2, d4;
} LodDist6;

typedef struct {
    s32 unk00[2];
    s32 x08;
    s32 y0C;
    s32 z10;
    s32 unk14[25];
    s32 col78;
} Scratch2D784;
extern u8 CarPartModelIndices[];
extern u8 CarPartRenderModes[];
extern u8 CarPartTransformIndices[];
extern u8 CarPartMatrixIndices[];
extern LodDist6 CarModelLodDistances[];

int TriggerWheelBump(RaceVehicle *vehicle, int wheel)
{
    int event;
    if (vehicle->input < 0x320 ||
        (event = LookupTrackEvent(vehicle->half1EA, vehicle->prevWheelNodes100[wheel])) == -1) {
        return 0;
    }
    vehicle->wheelBumpVel1C4[wheel] = -((s32)((u32)event * vehicle->input) / 2800);
    vehicle->wheelBumpState1BC[wheel] = 1;
    vehicle->steeringLock1B8 ^= 1 << wheel;
    return 1;
}

static void DrawPreviewCarParts(RaceVehicle *arg0, PreviewRgb3 color, s32 arg2, s32 flags) {
    MatrixWords32 sp10;
    MatrixWords32 sp30;
    MatrixWords32 sp50;
    MatrixWords32 sp70;
    s16 sp90[4];
    MatrixWords32 sp98[6];
    Vector sp158[5];
    s32 slot_half;
    s32 s4_val;
    s32 i;
    u8 a1_val;

    slot_half = arg0->slotIndex1E6 >> 1;
    SelectModelBank(slot_half);
    SetLightColorsScaled(arg2);

    BuildRotMatrixY(&sp70, -arg0->rot54);
    BuildRotMatrixX(&sp30, -arg0->rot50);
    BuildRotMatrixZ(&sp50, -arg0->rot58);
    MulMatrix(&sp30, &sp50);
    MulMatrix(&sp70, &sp30);
    MulMatrix0(&LightMatrix.matrix, &sp70, &sp50);
    RR_GTE_SETLLM(&sp50);

    BuildRotMatrixY(&sp10, -arg0->rot54);
    MulMatrix2((MatrixWords32 *)SCRATCH_ROT_MATRIX, &sp10);
    BuildRotMatrixX(&sp98[0], arg0->wheelRotX1A8 - arg0->rot50);
    BuildRotMatrixZ(&sp50, arg0->wheelRotZ1AA - arg0->rot58);
    MulMatrix(&sp98[0], &sp50);
    MulMatrix2(&sp10, &sp98[0]);

    sp158[0] = arg0->bodyPosition;
    sp158[1].x = arg0->ir70[0] + arg0->ir60[0];
    sp158[1].y = arg0->ir70[1] + arg0->ir60[1];
    sp158[1].z = arg0->ir70[2] + arg0->ir60[2];

    BuildRotMatrixX(&sp98[1], -arg0->ang80);
    BuildRotMatrixZ(&sp50, -arg0->ang84);
    MulMatrix(&sp98[1], &sp50);
    MulMatrix2(&sp10, &sp98[1]);

    sp158[2].x = arg0->worldPosition10.words[0] + arg0->ir60[0];
    sp158[2].y = arg0->worldPosition10.words[1] + arg0->ir60[1];
    sp158[2].z = arg0->worldPosition10.words[2] + arg0->ir60[2];

    if (arg0->wheelSteering17C & 0x1000) {
        s4_val = -arg0->wheelSteering17C;
    } else {
        s4_val = arg0->wheelSteering17C;
    }
    if (CarBoundsTable[slot_half]->wheelRotationAxis == 1) {
        BuildRotMatrixY(&sp98[3], s4_val);
    } else {
        BuildRotMatrixX(&sp98[3], s4_val);
    }

    sp98[2] = sp30;
    MulMatrix2(&sp10, &sp98[2]);
    MulMatrix2(&sp98[2], &sp98[3]);
    BuildRotMatrixY(&sp98[4], arg0->lateralMotion178 / 12);
    BuildRotMatrixX(&sp98[5], s4_val);
    MulMatrix2(&sp98[2], &sp98[4]);
    MulMatrix2(&sp98[4], &sp98[5]);

    RR_GTE_SETROT(&sp70);
    for (i = 0; i < 2; i++) {
        s32 v0_w = CarBoundsTable[slot_half]->halfWidth;
        if (i & 1) {
            v0_w = -v0_w;
        }
        sp90[0] = v0_w * 4;
        sp90[1] = CarBoundsTable[slot_half]->wheelOffsetY * 4;
        sp90[2] = -CarBoundsTable[slot_half]->halfLength * 4;
        RR_GTE_MVMVA_ROT_V0_NOCV(sp90);
        RR_GTE_STORE_IR(&sp158[i + 3]);
        sp158[i + 3].x += sp158[2].x;
        sp158[i + 3].y += sp158[2].y;
        sp158[i + 3].z += sp158[2].z;
    }

    for (i = 0; i < 8; i++) {
        u8 s0_mode = CarPartRenderModes[i];
        InterpPart(i);
        if (s0_mode != 1 || DrawRroModel(&sp158[CarPartTransformIndices[i]], &sp98[CarPartMatrixIndices[i]], CarPartModelIndices[i], 0) != s0_mode) {
            SetGteObjectTransform(
                (ObjectTransform38 *)(SCRATCHPAD + 0x7C),
                (const s16 *)&sp158[CarPartTransformIndices[i]],
                sp98[CarPartMatrixIndices[i]].w);
            G->col78 = 0;
            a1_val = 1;
            if (CarPartModelIndices[i] < ModelBankFormat) {
                a1_val = CarPartModelIndices[i];
            }
            SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1_val, i != 0, 0);
        }
    }
}


static void DrawPlayerCarParts(RaceVehicle *arg0, s32 viewMode) {
    MatrixWords32 sp20;
    MatrixWords32 sp40;
    MatrixWords32 sp60;
    MatrixWords32 sp80;
    s16 spA0[4];
    MatrixWords32 spA8[6];
    Vector sp168[5];
    s32 slot_half;
    s32 slot_idx;
    s32 s5_val;
    s32 v0_drag;
    s32 s4_val;
    s32 i;
    u8 a1_val;

    slot_half = arg0->slotIndex1E6 >> 1;
    SelectModelBank(slot_half);
    s5_val = ComputeCarSurfaceLighting((void *)arg0, GetCarTrackLightIndex(arg0));
    BuildRotMatrixY(&sp80, -arg0->rot54);
    BuildRotMatrixX(&sp40, -arg0->rot50);
    BuildRotMatrixZ(&sp60, -arg0->rot58);
    MulMatrix(&sp40, &sp60);
    MulMatrix(&sp80, &sp40);
    MulMatrix0(&LightMatrix.matrix, &sp80, &sp60);
    RR_GTE_SETLLM(&sp60);
    BuildRotMatrixY(&sp20, -arg0->rot54);
    MulMatrix2((MatrixWords32 *)SCRATCH_ROT_MATRIX, &sp20);
    BuildRotMatrixX(&spA8[0], arg0->wheelRotX1A8 - arg0->rot50);
    BuildRotMatrixZ(&sp60, arg0->wheelRotZ1AA - arg0->rot58);
    MulMatrix(&spA8[0], &sp60);
    MulMatrix2(&sp20, &spA8[0]);
    sp168[0] = arg0->bodyPosition;
    sp168[1].x = arg0->ir70[0] + arg0->ir60[0];
    sp168[1].y = arg0->ir70[1] + arg0->ir60[1];
    sp168[1].z = arg0->ir70[2] + arg0->ir60[2];
    BuildRotMatrixX(&spA8[1], -arg0->ang80);
    BuildRotMatrixZ(&sp60, -arg0->ang84);
    MulMatrix(&spA8[1], &sp60);
    MulMatrix2(&sp20, &spA8[1]);
    sp168[2].x = arg0->worldPosition10.words[0] + arg0->ir60[0];
    sp168[2].y = arg0->worldPosition10.words[1] + arg0->ir60[1];
    sp168[2].z = arg0->worldPosition10.words[2] + arg0->ir60[2];
    if (arg0->wheelSteering17C & 0x1000) {
        s4_val = -arg0->wheelSteering17C;
    } else {
        s4_val = arg0->wheelSteering17C;
    }
    if (CarBoundsTable[slot_half]->wheelRotationAxis == 1) {
        BuildRotMatrixY(&spA8[3], s4_val);
    } else {
        BuildRotMatrixX(&spA8[3], s4_val);
    }
    spA8[2] = sp40;
    MulMatrix2(&sp20, &spA8[2]);
    MulMatrix2(&spA8[2], &spA8[3]);
    BuildRotMatrixY(&spA8[4], arg0->lateralMotion178 / 12);
    BuildRotMatrixX(&spA8[5], s4_val);
    MulMatrix2(&spA8[2], &spA8[4]);
    MulMatrix2(&spA8[4], &spA8[5]);
    RR_GTE_SETROT(&sp80);
    for (i = 0; i < 2; i++) {
        s32 v0_w = CarBoundsTable[slot_half]->halfWidth;
        if (i & 1) {
            v0_w = -v0_w;
        }
        spA0[0] = v0_w * 4;
        spA0[1] = CarBoundsTable[slot_half]->wheelOffsetY * 4;
        spA0[2] = -CarBoundsTable[slot_half]->halfLength * 4;
        RR_GTE_MVMVA_ROT_V0_NOCV(spA0);
        RR_GTE_STORE_IR(&sp168[i + 3]);
        sp168[i + 3].x += sp168[2].x;
        sp168[i + 3].y += sp168[2].y;
        sp168[i + 3].z += sp168[2].z;
    }
    s5_val = (s16)(TrackLightTable[s5_val].flags02 | ForceTailLightTrails);
    slot_idx = arg0->slotIndex1E6 + CarViewportBankOffset;
    if (arg0->flags1E4 & 0x10) {
        v0_drag = arg0->control.dynamics94.steering.target18.halves.high;
    } else {
        v0_drag = arg0->control.dragFlag46;
    }
    if (v0_drag > 0) {
        s5_val |= 2;
    }
    InterpPart(0x80);
    DrawWheelTrailEffects(arg0, &sp168[1].x, &spA8[1], &sp80, slot_idx, s5_val, 0);
    for (i = 0; i < 8; i++) {
        u8 s0_mode = CarPartRenderModes[i];
        InterpPart(i);
        if (s0_mode != 1 || DrawRroModel(&sp168[CarPartTransformIndices[i]], &spA8[CarPartMatrixIndices[i]], CarPartModelIndices[i], 0) != s0_mode) {
            SetGteObjectTransform(
                (ObjectTransform38 *)(SCRATCHPAD + 0x7C),
                (const s16 *)&sp168[CarPartTransformIndices[i]],
                spA8[CarPartMatrixIndices[i]].w);
            G->col78 = 0;
            a1_val = 1;
            if (CarPartModelIndices[i] < ModelBankFormat) {
                a1_val = CarPartModelIndices[i];
            }
            SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1_val, i != 0, 0);
        }
    }
}



static void DrawOpponentCarParts(RaceVehicle *arg0, s32 arg1) {
    MatrixWords32 sp20;
    MatrixWords32 sp40;
    MatrixWords32 sp60;
    MatrixWords32 sp80;
    s16 spA0[4];
    MatrixWords32 spA8[6];
    Vector sp168[5];
    s32 slot_half;
    s32 slot_idx;
    s32 fp_val;
    s32 s7_val;
    s32 s5_val;
    s32 v0_drag;
    s32 s4_val;
    s32 i;
    u8 a1_val;

    slot_half = arg0->slotIndex1E6 >> 1;
    fp_val = (arg0->slotIndex1E6 & 1) * 0x05000000;
    slot_idx = arg0->slotIndex1E6 + CarViewportBankOffset;
    SelectModelBank(slot_half);

    sp168[0].x = (arg0->worldPosition10.words[0] - G->x08) >> 2;
    sp168[0].y = 0;
    sp168[0].z = (arg0->worldPosition10.words[2] - G->z10) >> 2;
    ApplyMatrixLV((const MatrixWords32 *)(SCRATCH_ROT_MATRIX), (const Vec3i *)(&sp168[0]), (Vec3i *)(&sp168[1]));
    if (sp168[0].x < 0) {
        sp168[0].x = -sp168[0].x;
    }
    if (sp168[0].z < 0) {
        sp168[0].z = -sp168[0].z;
    }
    i = sp168[0].x + sp168[0].z;
    if (sp168[1].z >= 0) {
        s7_val = 0;
        if (arg1 == 1) {
            s7_val = -0x5A;
        }
        s5_val = ComputeCarSurfaceLighting((void *)arg0, GetCarTrackLightIndex(arg0));
        s5_val = (s16)(TrackLightTable[s5_val].flags02 | ForceTailLightTrails);
        if (arg0->flags1E4 & 0x10) {
            if (arg0->control.dynamics94.steering.target18.halves.high > 0) {
                s5_val |= 2;
            }
        } else {
            if (arg0->control.dragFlag46 > 0) {
                s5_val |= 2;
            }
        }
        s5_val &= 3;
        BuildRotMatrixY(&sp80, -arg0->rot54);
        BuildRotMatrixX(&sp40, -arg0->rot50);
        BuildRotMatrixZ(&sp60, -arg0->rot58);
        MulMatrix(&sp40, &sp60);
        MulMatrix(&sp80, &sp40);
        MulMatrix0(&LightMatrix.matrix, &sp80, &sp60);
        RR_GTE_SETLLM(&sp60);
        if (i < CarModelLodDistances[arg1].d0) {
            BuildRotMatrixY(&sp20, -arg0->rot54);
            MulMatrix2((MatrixWords32 *)SCRATCH_ROT_MATRIX, &sp20);
            BuildRotMatrixX(&spA8[0], arg0->wheelRotX1A8 - arg0->rot50);
            BuildRotMatrixZ(&sp60, arg0->wheelRotZ1AA - arg0->rot58);
            MulMatrix(&spA8[0], &sp60);
            MulMatrix2(&sp20, &spA8[0]);
            sp168[0] = arg0->bodyPosition;
            sp168[1].x = arg0->ir70[0] + arg0->ir60[0];
            sp168[1].y = arg0->ir70[1] + arg0->ir60[1];
            sp168[1].z = arg0->ir70[2] + arg0->ir60[2];
            BuildRotMatrixX(&spA8[1], -arg0->ang80);
            BuildRotMatrixZ(&sp60, -arg0->ang84);
            MulMatrix(&spA8[1], &sp60);
            MulMatrix2(&sp20, &spA8[1]);
            sp168[2].x = arg0->worldPosition10.words[0] + arg0->ir60[0];
            sp168[2].y = arg0->worldPosition10.words[1] + arg0->ir60[1];
            sp168[2].z = arg0->worldPosition10.words[2] + arg0->ir60[2];
            if (arg0->wheelSteering17C & 0x1000) {
                s4_val = -arg0->wheelSteering17C;
            } else {
                s4_val = arg0->wheelSteering17C;
            }
            if (CarBoundsTable[slot_half]->wheelRotationAxis == 1) {
                BuildRotMatrixY(&spA8[3], s4_val);
            } else {
                BuildRotMatrixX(&spA8[3], s4_val);
            }
            spA8[2] = sp40;
            MulMatrix2(&sp20, &spA8[2]);
            MulMatrix2(&spA8[2], &spA8[3]);
            BuildRotMatrixY(&spA8[4], arg0->lateralMotion178 / 12);
            BuildRotMatrixX(&spA8[5], s4_val);
            MulMatrix2(&spA8[2], &spA8[4]);
            MulMatrix2(&spA8[4], &spA8[5]);
            RR_GTE_SETROT(&sp80);
            for (i = 0; i < 2; i++) {
                s32 v0_w = CarBoundsTable[slot_half]->halfWidth;
                if (i & 1) {
                    v0_w = -v0_w;
                }
                spA0[0] = v0_w * 4;
                spA0[1] = CarBoundsTable[slot_half]->wheelOffsetY * 4;
                spA0[2] = -CarBoundsTable[slot_half]->halfLength * 4;
                RR_GTE_MVMVA_ROT_V0_NOCV(spA0);
                RR_GTE_STORE_IR(&sp168[i + 3]);
                sp168[i + 3].x += sp168[2].x;
                sp168[i + 3].y += sp168[2].y;
                sp168[i + 3].z += sp168[2].z;
            }
            InterpPart(0x80);
            DrawWheelTrailEffects(arg0, &sp168[1].x, &spA8[1], &sp80, slot_idx, s5_val, 0);
            for (i = 0; i < 8; i++) {
                u8 s0_mode = CarPartRenderModes[i];
                InterpPart(i);
                if (s0_mode != 1 || DrawRroModel(&sp168[CarPartTransformIndices[i]], &spA8[CarPartMatrixIndices[i]], CarPartModelIndices[i], fp_val) != s0_mode) {
                    SetGteObjectTransform(
                        (ObjectTransform38 *)(SCRATCHPAD + 0x7C),
                        (const s16 *)&sp168[CarPartTransformIndices[i]],
                        spA8[CarPartMatrixIndices[i]].w);
                    G->col78 = fp_val;
                    a1_val = 1;
                    if (CarPartModelIndices[i] < ModelBankFormat) {
                        a1_val = CarPartModelIndices[i];
                    }
                    SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1_val, i != 0, 0);
                }
            }
            return;
        } else if (i < CarModelLodDistances[arg1].d2) {
            BuildRotMatrixY(&sp20, -arg0->rot54);
            MulMatrix2((MatrixWords32 *)SCRATCH_ROT_MATRIX, &sp20);
            BuildRotMatrixX(&spA8[0], arg0->wheelRotX1A8 - arg0->rot50);
            BuildRotMatrixZ(&sp60, arg0->wheelRotZ1AA - arg0->rot58);
            MulMatrix(&spA8[0], &sp60);
            MulMatrix2(&sp20, &spA8[0]);
            sp168[0] = arg0->bodyPosition;
            sp168[1].x = arg0->ir70[0] + arg0->ir60[0];
            sp168[1].y = arg0->ir70[1] + arg0->ir60[1];
            sp168[1].z = arg0->ir70[2] + arg0->ir60[2];
            BuildRotMatrixX(&spA8[1], -arg0->ang80);
            BuildRotMatrixZ(&sp60, -arg0->ang84);
            MulMatrix(&spA8[1], &sp60);
            MulMatrix2(&sp20, &spA8[1]);
            InterpPart(0x80);
            DrawWheelTrailEffects(arg0, &sp168[1].x, &spA8[1], &sp80, slot_idx, s5_val, s7_val);
            SetGteObjectTransform((ObjectTransform38 *)(SCRATCHPAD + 0x7C), (const s16 *)&sp168[0], spA8[0].w);
            G->col78 = 0;
            a1_val = 1;
            if (ModelBankFormat >= 4) {
                a1_val = 3;
            }
            InterpPart(0x10);
            SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1_val, 0, s7_val);
            InterpPart(0x11);
            if (DrawRroModel(&sp168[1], &spA8[1], 1, fp_val) == 0) {
                SetGteObjectTransform(
                    (ObjectTransform38 *)(SCRATCHPAD + 0x7C),
                    (const s16 *)&sp168[1],
                    spA8[1].w);
                G->col78 = fp_val;
                SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, 1, 1, s7_val);
            }
            return;
        } else if (i < CarModelLodDistances[arg1].d4) {
            MulMatrix0((const MatrixWords32 *)((void *)SCRATCH_ROT_MATRIX), &sp80, &spA8[0]);
            sp168[0].x = arg0->worldPosition10.words[0];
            sp168[0].y = arg0->worldPosition10.words[1];
            sp168[0].z = arg0->worldPosition10.words[2];
            InterpPart(0x80);
            DrawWheelTrailEffects(arg0, &sp168[0].x, &spA8[0], &sp80, slot_idx, s5_val, s7_val);
            SetGteObjectTransform((ObjectTransform38 *)(SCRATCHPAD + 0x7C), (const s16 *)&sp168[0], spA8[0].w);
            G->col78 = fp_val;
            a1_val = 1;
            if (ModelBankFormat >= 3) {
                a1_val = 2;
            }
            InterpPart(0x20);
            SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1_val, 1, s7_val);
            return;
        }
    }
    InterpPart(0x80);
    DrawWheelTrailEffects(arg0, 0, 0, 0, slot_idx, 0, 0);
}

/* Frame interpolation: each car is one object, its parts are pieces. The
 * identity is the car's record in PlayerCars or ComCars (another car, as a
 * preview's, by its offset from PlayerCars), never its host address: ASLR
 * moves that between runs, and the midpoint's identity table then drops
 * and keeps other identities each run. */
static u32 InterpCarId(const RaceVehicle *car) {
    const uintptr_t at = (uintptr_t)car, players = (uintptr_t)PlayerCars, coms = (uintptr_t)ComCars;
    u32 index;

    if (at >= players && at < players + sizeof(PlayerCars))
        index = (u32)((at - players) / sizeof(*car));
    else if (at >= coms && at < coms + sizeof(ComCars))
        index = 2 + (u32)((at - coms) / sizeof(*car));
    else
        index = 13 + (u32)(((at - players) >> 4) % 0xFFF0u);
    return index << 8;
}

void DrawPreviewCar(RaceVehicle *arg0, PreviewRgb3 color, s32 arg2, s32 flags) {
    u32 interp = InterpBegin(INTERP_CAR, InterpCarId(arg0));
    DrawPreviewCarParts(arg0, color, arg2, flags);
    InterpEnd(interp);
}

void DrawPlayerCar(RaceVehicle *arg0, s32 viewMode) {
    u32 interp = InterpBegin(INTERP_CAR, InterpCarId(arg0));
    DrawPlayerCarParts(arg0, viewMode);
    InterpEnd(interp);
}

void DrawOpponentCar(RaceVehicle *arg0, s32 arg1) {
    u32 interp = InterpBegin(INTERP_CAR, InterpCarId(arg0));
    DrawOpponentCarParts(arg0, arg1);
    InterpEnd(interp);
}

/* Initial values of the globals this module owns. */
u8 CarPartModelIndices[8] = {3, 0, 9, 6, 8, 7, 5, 4};
u8 CarPartRenderModes[8] = {1, 1, 0, 1, 0, 0, 1, 1};
u8 CarPartTransformIndices[8] = {0, 1, 2, 2, 3, 4, 3, 4};
u8 CarPartMatrixIndices[8] = {0, 1, 2, 3, 4, 4, 5, 5};
LodDist6 CarModelLodDistances[5] = {{.d0 = 672, .d2 = 3200, .d4 = 8704}, {.d0 = -1, .d2 = 2560, .d4 = 4096}, {.d0 = 4096, .d2 = 8192, .d4 = 8704}, {.d0 = 320, .d2 = 3200, .d4 = 8704}, {.d0 = 160, .d2 = 2560, .d4 = 8704}};
s16 ForceTailLightTrails;
/* A slot reads its bounds before the first car model is loaded into it
 * (e.g. the VS preview resets both cars on entry); the PS1 table holds 0
 * there and the read lands in kernel RAM at address 0, whose first twelve
 * bytes are constant after boot. Unloaded slots point at those bytes so
 * the reset computes the same values; the loader replaces the pointer. */
static CarBounds CarBoundsUnloaded = {
    .halfWidth = 0x0003, .wheelOffsetY = 0x0000, .halfLength = 0x0C80,
    .bodyHeight = 0x275A, .unk08 = 0x0008, .wheelRotationAxis = 0x40,
    .reserved0B = 0x03,
};
CarBounds *CarBoundsTable[4] = {
    &CarBoundsUnloaded, &CarBoundsUnloaded,
    &CarBoundsUnloaded, &CarBoundsUnloaded,
};
