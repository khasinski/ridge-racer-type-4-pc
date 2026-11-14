#include "psyq/libgte/gtemat5.h"
#include "rr/vehicle_table.h"
#include "psyq/libgte/gtemat4.h"
#include "psyq/libgte/gtemat3.h"
#include <stddef.h>
#include "types.h"
#include "rr/draw_distance.h"
#include "game/car/bounds.h"
#include "game/car/carwheel.h"
#include "rr/race.h"
#include "game/gfx/gtepoly1.h"
#include "game/race/raceinit.h"
#include "game/car/carbound.h"
#include "rr/audio_loop.h"
#include "rr/race_setup.h"
#include "rr/bios_devices.h"
#include "rr/matrix_workspace.h"
#include "game/race/racesamp.h"
#include "game/data/selection.h"
#include "game/math/rotmat.h"

extern short LinkHostFlag;

void SetCarBounds(RaceVehicle *a);

void DrawCars(s32 a0) {
    int z; 
    int i, s2;
    RaceVehicle **p;
    ApplyCourseDrawDistance();
    CarViewportBankOffset = 0;
    switch (a0) {
    case 0:
        z = 0;
        if (!(CarTable[0]->flags1E4 & 0x100)) {
            DrawPlayerCar(CarTable[0], z);
        } else {
            /* Hidden car: advance trail state only (no position, as carwheel.c). */
            DrawWheelTrailEffects(CarTable[0], NULL, NULL, NULL, 0, 0, 0);
        }
    case 1:
        if (GameModeType != 4) {
            for (i = 1; i < NumCars; i++) {
                if (CarTable[i]->half1E8 != -1) DrawOpponentCar(CarTable[i], a0);
            }
        } else {
            if (LinkPlayerCount == 2) {
                DrawOpponentCar(CarTable[LinkHostFlag * 2], a0);
            }
        }
        break;
    case 3:
    case 5:
        CarViewportBankOffset = 1;
    case 2:
    case 4:
        s2 = 3;
        if (GameModeType == 4) s2 = 4;
        if (LinkPlayerCount == 2) s2 = 0;
        if (NumCars > 0) {
            i = 0;
            p = CarTable;
            do {
                if (p[0]->half1E8 != -1) {
                    if (a0 - 2 == i) {
                        z = 0;
                        if (!(p[0]->flags1E4 & 0x100)) {
                            DrawPlayerCar(p[0], z);
                        } else {
                            DrawWheelTrailEffects(p[0], NULL, NULL, NULL, p[0]->slotIndex1E6 + CarViewportBankOffset, 0, 0);
                        }
                    } else {
                        DrawOpponentCar(p[0], s2);
                    }
                }
                p++;
                i++;
            } while (i < NumCars);
        }
        break;
    case 6:
        if (NumCars > 0) {
            i = 0;
            do {
                if (CarTable[i]->half1E8 != -1) DrawOpponentCar(CarTable[i], 2);
                i++;
            } while (i < NumCars);
        }
        break;
    }
}

void SetCarBounds(RaceVehicle *vehicle)
{
    int boundsIndex;
    CarBounds **boundsSlot;
    CarBounds **boundsTable;
    if (LinkHostFlag != 0) {
        boundsIndex = vehicle != PlayerCars;
    } else {
        boundsIndex = 3;
        if (vehicle == PlayerCars) {
            boundsIndex = 2;
        }
    }
    vehicle->slotIndex1E6 = boundsIndex * 2;
    boundsTable = CarBoundsTable;
    boundsSlot = boundsTable + boundsIndex;
    vehicle->height108 = (*boundsSlot)->bodyHeight * 4;
    vehicle->width10C = ((*boundsSlot)->halfWidth + 5) * 2;
    vehicle->halfSpan10E = 0x28 - (*boundsSlot)->halfLength;
    vehicle->footprint110.fields.corners[0].x = -(((u16)(*boundsSlot)->halfWidth + 5) * 4);
    vehicle->footprint110.fields.corners[0].y = -(((*boundsSlot)->halfLength - 0x14) * 4);
    vehicle->footprint110.fields.corners[1].x = ((u16)(*boundsSlot)->halfWidth + 5) * 4;
    vehicle->footprint110.fields.corners[1].y = -(((*boundsSlot)->halfLength - 0x14) * 4);
    vehicle->footprint110.fields.corners[2].x = -(((u16)(*boundsSlot)->halfWidth + 5) * 4);
    vehicle->footprint110.fields.corners[2].y = -0x50;
    vehicle->footprint110.fields.corners[3].x = ((u16)(*boundsSlot)->halfWidth + 5) * 4;
    vehicle->footprint110.fields.corners[3].y = -0x50;
}

void ResetCarState(RaceVehicle *a)
{
    PsyqBzero(a, sizeof(RaceVehicle));
    SetCarBounds(a);
    a->ang80 = (unsigned short)a->rot50;
    a->ang82 = (unsigned short)a->rot54;
    a->ang84 = (unsigned short)a->rot58;
    *(Vector *)a->ir70 = *(Vector *)a->position20.words;
    a->posScaled00[0] = a->position20.words[0] * 256;
    a->posScaled00[1] = a->position20.words[1] * 256;
    a->posScaled00[2] = a->position20.words[2] * 256;
    a->bodyPosition = *(Vector *)&a->rot50;
}

void UpdateCarWorldTransform(RaceVehicle *a0) {
    struct {
        MatrixWords32 m1;
        MatrixWords32 m2;
        short v[3];
    } L;
    int t1, t2;
    unsigned short u1, u2, u3;
    a0->worldPosition10.words[0] = a0->position20.words[0] * 4;
    a0->worldPosition10.words[1] = a0->position20.words[1] * 4;
    a0->worldPosition10.words[2] = a0->position20.words[2] * 4;
    a0->ang80 = (unsigned short)a0->rot50;
    a0->ang82 = (unsigned short)a0->rot54;
    a0->ang84 = (unsigned short)a0->rot58;
    BuildRotMatrixY(&L.m1, 0x800 - a0->rot54);
    BuildRotMatrixX(&L.m2, a0->rot50);
    MulMatrix(&L.m1, &L.m2);
    BuildRotMatrixZ(&L.m2, a0->rot58);
    MulMatrix2(&L.m1, &L.m2);
    L.v[0] = 0;
    L.v[1] = -(unsigned short)a0->height108;
    L.v[2] = (a0->halfSpan10E * 4) / 3;
    ApplyMatrix(&L.m2, (const Vec3s8 *)(L.v), (Vec3i *)a0->ir40);
    BuildRotMatrixY(&L.m1, 0x800 - a0->ang82);
    BuildRotMatrixX(&L.m2, a0->ang80);
    MulMatrix(&L.m1, &L.m2);
    BuildRotMatrixZ(&L.m2, a0->ang84);
    MulMatrix2(&L.m1, &L.m2);
    L.v[0] = 0;
    L.v[1] = (a0->wheelOffsetE4[2] + a0->wheelOffsetE4[3]) >> 7;
    L.v[2] = 0;
    ApplyMatrix(&L.m2, (const Vec3s8 *)(L.v), (Vec3i *)a0->ir70);
    L.v[0] = 0;
    L.v[1] = -((a0->wheelSuspensionDC[2] + a0->wheelSuspensionDC[3]) >> 3);
    L.v[2] = 0;
    ApplyMatrix(&L.m2, (const Vec3s8 *)(L.v), (Vec3i *)a0->ir60);
    a0->ir70[0] = a0->ir70[0] + a0->worldPosition10.words[0];
    a0->ir70[1] = a0->ir70[1] + a0->worldPosition10.words[1];
    a0->ir70[2] = a0->ir70[2] + a0->worldPosition10.words[2];
    a0->ang80 = (unsigned short)a0->rot50;
    a0->ang82 = (unsigned short)a0->rot54;
    a0->ang84 = (unsigned short)a0->rot58;
    a0->worldPosition10.words[1] = a0->worldPosition10.words[1] + a0->ir40[1];
    a0->worldPosition10.words[0] = a0->worldPosition10.words[0] + a0->ir40[0];
    a0->ang80 = (unsigned short)a0->ang80 + (unsigned short)a0->renderAngleX1AC;
    a0->ang84 = (unsigned short)a0->ang84 + (unsigned short)a0->renderAngleZ1AE;
    a0->ir70[1] = a0->ir70[1] + a0->ir40[1];
    a0->ir70[0] = a0->ir70[0] + a0->ir40[0];
    a0->worldPosition10.words[2] = a0->worldPosition10.words[2] + a0->ir40[2];
    a0->ir70[2] = a0->ir70[2] + a0->ir40[2];
    a0->bodyPosition = a0->worldPosition10.vector;
}

void ScaleMatrixRow1(s32 scale, MatrixWords32 *matrix) {
    u32 factor = scale;

    matrix->fields.rotation00[1][0] = (s16)(matrix->fields.rotation00[1][0] * factor);
    matrix->fields.rotation00[1][1] = (s16)(matrix->fields.rotation00[1][1] * factor);
    matrix->fields.rotation00[1][2] = (s16)(matrix->fields.rotation00[1][2] * factor);
}

void AssignCarModelSlot(RaceVehicle *p) {
    unsigned char *a2;
    int t;
    int b;
    int c;
    int r = 0;
    if (GameModeType == 1) {
        a2 = SelectionRows.bytes;
        t = p->flags1E4 & 7;
        c = a2[0];
        r = (t < 4) ^ 1;
        b = a2[t * 4];
        if (c == b) {
            p->slotIndex1E6 = r;
        } else if (a2[0x14] == b) {
            p->slotIndex1E6 = r | 2;
        } else if (a2[0x18] == b) {
            p->slotIndex1E6 = r | 4;
        } else if (a2[0x1C] == b) {
            p->slotIndex1E6 = r | 6;
        }
    } else if (GameModeType == 5) {
        int u = p->flags1E4;
        if ((u & 7) >= 5) r = 1;
        p->slotIndex1E6 = (((u - 1) & 3) << 1) | r;
    } else if (GameModeType == 6) {
        p->slotIndex1E6 = 2;
    } else {
        int w = (short)p->flags1E4 - 1;
        p->slotIndex1E6 = (w % 2 + 2) << 1;
    }
    p->control.maxSpeedF8 = 0x100;
    PlaceCarOnGrid(p, ((short)p->flags1E4 - 1) & 7);
}

/* Initial values of the globals this module owns. */
int CarViewportBankOffset;
s16 LinkPlayerCount;
