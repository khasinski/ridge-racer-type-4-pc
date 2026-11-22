#include "rr/cpu_time.h"
#include "rr/cpu_work.h"
#include "rr/vehicle_table.h"
#include "game/race/racegrid.h"
#include "rr/player_cars.h"
#include "rr/frame_storage_types.h"
#include "game/data/bankrec.h"
#include "types.h"
#include "game/car/car.h"
#include "game/car/grid.h"
#include "scratchpad.h"
#include "rr/gpu_scratch_types.h"
#include "rr/track_sample_types.h"
#include "game/car/carmove1.h"
#include "game/menu/menu8.h"
#include "game/course/trkintp.h"
#include "game/ui/uipart1.h"
#include "game/car/carparam.h"
#include "game/car/carsetup.h"
#include "rr/audio_loop.h"
#include "game/car/aipath2.h"
#include "game/car/bounds.h"
#include "game/data/selection.h"
#include "game/race/racesamp.h"
#include "game/ui/uitext.h"
#include "rr/race_setup.h"
#include "game/ui/badge1.h"
#include "rr/menu_label_render.h"
#include "game/course/trkclip.h"
#include "game/math/rotmat.h"
#include "game/ui/uibar.h"
#include "game/ui/uidraw.h"
#include "rr/scene_camera_state.h"

typedef struct {
    int a, b, c, d;
} Vec4;

extern s32 MaskBitSetting;
extern unsigned char ReplayCameraViewWeights[];
extern unsigned char RaceCarStatParam1[];
extern unsigned char RaceCarStatParam3[];
extern unsigned char RaceCarStatParam4[];
extern unsigned char RaceCarStatParam5[];
extern RaceVehicle PlayerCars[];
/* The PS1 map names two windows of the computer car array: DuelRaceVehicles
 * (ComCars - 2) and GrandPrixRaceVehicles (ComCars - 1), so that car slot i
 * of a race maps onto ComCars without a gap. */
extern RaceVehicle ComCars[];
extern short ReverseCourse;

void InitPreviewImageRows(void) {
    SetModelTexturePage(0x19);
    MaskBitSetting = 0;
    InitBadgeRow(0, 0x10, 0x158, 0x120, 0x3C, 0, 0x103);
    InitRect16Row(0, 0x10, 0x48, 0x120, 0x120, 0, 0);
    InitImageRow(0, -1, 0, 1);
}

void DrawSelectionIconGrid(int arg0) {
    RR_CPU_CYCLES(RR_CYCLES_SELECTION_ICON_GRID);
    int v[9];
    UiUvRect buf;
    int s0;
    void *a1;
    int b;
    unsigned char *t7 = ReplayCameraViewWeights;
    SelectionRow *q = SelectionRows.entries;
    SelectionRow *p = &q[arg0];
    v[0] = t7[p->variant];
    v[1] = RaceCarStatParam1[p->variant];
    v[2] = CarRecordPtrs[arg0]->handling1C8.steeringMode0A > 0 ? 0x18 : 0x19;
    v[3] = RaceCarStatParam3[p->selection];
    v[4] = RaceCarStatParam4[p->selection];
    v[5] = RaceCarStatParam5[p->selection];
    v[6] = CarRecordPtrs[arg0]->stage1F3 + 0x14;
    b = p->option;
    v[7] = b + 0x1A;
    v[8] = b + 0x1C;
    if (p->parameter == 0x27) {
        v[5] = 0x1E;
        v[4] = 0x1E;
        v[3] = 0x1E;
        v[1] = 0x1E;
        v[0] = 0x1E;
    }
    if (p->option == 2) {
        v[8] = 0x1E;
        v[7] = 0x1E;
    }
    s0 = 0;
    a1 = RR_GPU_SCRATCH->cursor;
    buf.w = 0x10;
    buf.h = 0x20;
    do {
        int t = v[s0];
        buf.u = (t % 16) * 16;
        buf.v = (t / 16) << 5;
        a1 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, (s0 % 3) * 0x14 + 0xF0, (s0 / 3) * 0x28 + 0x58, &buf, 0x7FA4, 1);
        s0++;
    } while (s0 < 9);
    RR_GPU_SCRATCH->cursor = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, 0x39);
}

void InitCarTable(int a) {
    int i;
    int n;
    int v;
    RaceVehicle **p;
    RaceVehicle *q;
    switch (a) {
    case 1:
        NumCars = 8;
        i = 0;
        for (; i < NumCars; i++) {
            if (i == 0) {
                CarTable[0] = PlayerCars;
            } else {
                CarTable[i] = ComCars + (i - 1);
            }
        }
        break;
    case 2:
        NumCars = 2;
        CarTable[0] = PlayerCars;
        CarTable[1] = PlayerCars + 1;
        break;
    case 3:
        i = 2;
        CarTable[0] = PlayerCars;
        CarTable[1] = PlayerCars + 1;
        v = NumCars;
        if (i < v) {
            n = v;
            do {
                CarTable[i] = ComCars + (i - 2);
                i++;
            } while (i < n);
        }
        break;
    case 4:
        NumCars = 4;
        if (LinkHostFlag == 1) {
            CarTable[0] = PlayerCars;
            CarTable[1] = PlayerCars + 1;
            CarTable[2] = ComCars;
            CarTable[3] = ComCars + 1;
        } else {
            CarTable[2] = PlayerCars;
            CarTable[3] = PlayerCars + 1;
            CarTable[0] = ComCars;
            CarTable[1] = ComCars + 1;
        }
        break;
    case 5:
        NumCars = 7;
        i = 0;
        n = 7;
        for (; i < n; i++) {
            CarTable[i] = ComCars + i;
        }
        break;
    default:
    case 0:
        CarTable[0] = PlayerCars;
        NumCars = 1;
        break;
    case 6:
        CarTable[0] = PlayerCars;
        CarTable[1] = ComCars;
        NumCars = 2;
        break;
    }
    ResetRankEntriesForCars();
}

void PlaceCarOnGrid(RaceVehicle *p, int arg1) {
    int i;
    int t;
    int *g = (int *)(SCRATCHPAD + 0x7C);

    if (!RrWorkFrame && RrWorkDepth == 0)
        RR_CPU_CYCLES(RR_CYCLES_PLACE_CAR_ON_GRID);
    ResetVehicleShake(p);
    if (GameModeType == 0 && ReverseCourse == 1) {
        p->trackNodeF0 = RaceContext.startGrid53C->f0;
        t = TrackNodeCount - 5;
        p->position20.vector.x = TrackNodes[(p->trackNodeF0 + t) % TrackNodeCount].road.x;
        p->position20.vector.y = 0;
        p->position20.vector.z = TrackNodes[(p->trackNodeF0 + t) % TrackNodeCount].road.z;
    } else {
        p->position20.vector.x = RaceContext.startGrid53C->pos[arg1].x;
        p->position20.vector.y = 0;
        p->position20.vector.z = RaceContext.startGrid53C->pos[arg1].z;
        p->trackNodeF0 = RaceContext.startGrid53C->f0;
    }
    p->posScaled00[0] = p->position20.vector.x << 8;
    p->posScaled00[1] = 0;
    p->posScaled00[2] = p->position20.vector.z << 8;
    p->trackNodeF0 = FindTrackNode(&p->position20.vector.x, p->trackNodeF0, TrackNodeCount + 1);
    p->wheelNodesF8[0] = p->wheelNodesF8[1] = p->wheelNodesF8[2] = p->wheelNodesF8[3] = p->trackNodeF0;
    p->rot50 = 0;
    p->rot54 = ((0x800 - TrackNodes[p->trackNodeF0].road.heading) - (ReverseCourse << 11)) & 0xFFF;
    p->rot58 = 0;
    p->renderAngleZ1AE = 0;
    p->heading1D4 = p->prevHeading1D6 = p->rot54;
    if (*(s16 *)&p->flags1E4 >= 0x10) {
        p->control.dynamics94.player.steeringHeading08 = p->heading1D4;
    }
    i = p->slotIndex1E6 >> 1;
    p->height108 = CarBoundsTable[i]->bodyHeight * 4;
    p->width10C = ((short)*(u16 *)&CarBoundsTable[i]->halfWidth + 5) * 2;
    p->halfSpan10E = 0x28 - CarBoundsTable[i]->halfLength;
    p->footprint110.fields.corners[0].x = -((*(u16 *)&CarBoundsTable[i]->halfWidth + 5) * 4);
    p->footprint110.fields.corners[0].y = -((CarBoundsTable[i]->halfLength - 0x14) * 4);
    p->footprint110.fields.corners[1].x = (*(u16 *)&CarBoundsTable[i]->halfWidth + 5) * 4;
    p->footprint110.fields.corners[1].y = -((CarBoundsTable[i]->halfLength - 0x14) * 4);
    p->footprint110.fields.corners[2].x = -((*(u16 *)&CarBoundsTable[i]->halfWidth + 5) * 4);
    p->footprint110.fields.corners[2].y = -0x50;
    p->footprint110.fields.corners[3].x = (*(u16 *)&CarBoundsTable[i]->halfWidth + 5) * 4;
    p->footprint110.fields.corners[3].y = -0x50;
    p->footprint110.fields.heading10 = 0x400 - Atan2Table(p->footprint110.fields.corners[1].x * 2, p->footprint110.fields.corners[1].y + 0x50);
    p->footprint110.fields.heading12 = 0x400 - Atan2Table(p->footprint110.fields.corners[3].x * 2, p->footprint110.fields.corners[3].y - p->footprint110.fields.corners[1].y);
    p->sampleCount16C = 0x10;
    p->wheelSuspensionDC[0] = p->wheelSuspensionDC[1] = 0;
    p->wheelSuspensionDC[2] = p->wheelSuspensionDC[3] = 0;
    p->wheelOffsetE4[0] = p->wheelOffsetE4[1] = p->height108 << 6;
    p->wheelOffsetE4[2] = p->wheelOffsetE4[3] = p->height108 << 6;
    p->ang80 = p->rot50;
    p->ang82 = p->rot54;
    p->ang84 = p->rot58;
    *(Vec4 *)p->ir70 = *(Vec4 *)&p->position20;
    p->renderAngleX1AC = p->wheelRotX1A8 = p->bumpPitch1B4 = p->shakePitch1B0 = 0;
    p->renderAngleZ1AE = p->wheelRotZ1AA = p->shakeRollOffset1B6 = p->shakeRoll1B2 = 0;
    p->pitchAverage164 = p->rollAverage166 = 0;
    for (i = 0; i < 10; i++) {
        p->pitchAverage164 = (u16)p->pitchAverage164 + p->rot50;
        p->rollAverage166 = (u16)p->rollAverage166 + p->rot58;
    }
    p->pitchAverage168 = p->rollAverage16A = 0;
    for (i = 0; i < 10; i++) {
        p->pitchAverage168 = (u16)p->pitchAverage168 + p->rot50;
        p->rollAverage16A = (u16)p->rollAverage16A + p->rot58;
    }
    for (i = 0; i < 16; i++) {
        p->xHistory124[i] = p->rot50;
        p->zHistory144[i] = p->rot58;
    }
    ResetCarBodyFieldRing(p);
    for (i = 0; i < 4; i++) {
        p->wheelPositions88[i].y = p->position20.vector.y;
        p->wheelBumpState1BC[i] = 0;
        p->wheelBumpVel1C4[i] = 0;
    }
    ComputeStartDistance(p, 0);
    p->word190 = p->word18C;
    IntegrateCarMotion(p);
    ComputeWheelOffsets(p);
    ConstrainCarToTrack(p);
    *g = 0;
    FindWheelTrackNodes(p);
    UpdateWheelBumps(p);
}

/* Initial values of the globals this module owns. */
unsigned char ReplayCameraViewWeights[4] = {2, 3, 1};
unsigned char RaceCarStatParam1[4] = {0xA, 0xB, 9, 8};
unsigned char RaceCarStatParam3[4] = {7, 4, 5, 6};
unsigned char RaceCarStatParam4[4] = {0xB, 8, 9, 0xA};
unsigned char RaceCarStatParam5[4] = {0x13, 0x10, 0x11, 0x12};
s16 NumCars;
