#include "game/car/car.h"
#include "game/car/bounds.h"
#include "game/car/carsteer.h"
#include "rr/menu_overlay_runtime.h"
#include "game/course/trkintp.h"
#include "game/math/rotmat.h"


void UpdateCarSteering(RaceVehicle *arg0) {
    s32 s1;
    s32 s2;
    s32 s3;
    s32 v0;
    s32 i;

    s3 = 0;
    s2 = (arg0->prevInput1DA - arg0->input) * 4;
    if (arg0->flags1E4 & 0x10) {
        s1 = AngleDiffSigned(arg0->heading1D4, arg0->rot54) * 4;
        if (arg0->control.handling1C->steeringMode0A != 0) {
            s3 = (rsin((arg0->control.engine54.word << 5) & 0xFE0) * arg0->steerRoll1DE) / 8128;
        }
    } else {
        s1 = AngleDiffSigned(arg0->prevHeading1D6, arg0->rot54) * 4;
    }

    if ((v0 = arg0->input < 1000)) {
        s1 = (s1 * v0) / 1800;
    } else {
        s1 = (s1 * 1000) / 1800;
    }

    /* Note: increase s2 ref count so global.c allocates $s2 before $s3 */
    if (s2 < -0x10) {
        arg0->steerRoll1DE -= 0x10;
    } else if (s2 > 0x10) {
        arg0->steerRoll1DE += 0x10;
    }
    arg0->steerRoll1DE = (arg0->steerRoll1DE * 26) / 32;

    if (s1 < -0x20) {
        arg0->steerPitch1DC -= 0x20;
    } else if (s1 > 0x20) {
        arg0->steerPitch1DC += 0x20;
    }
    arg0->steerPitch1DC = (arg0->steerPitch1DC * 24) / 32;

    for (i = 0; i < 4; i++) {
        if (arg0->wheelBumpState1BC[i] == 0) {
            arg0->wheelOffsetE4[i] += (i & 1) ? -arg0->steerPitch1DC : arg0->steerPitch1DC;
            arg0->wheelOffsetE4[i] += (i >= 2) ? -arg0->steerRoll1DE : arg0->steerRoll1DE;
        } else if (arg0->wheelBumpState1BC[i] > 0) {
            arg0->wheelOffsetE4[i] -= 8;
        }

        if (arg0->wheelOffsetE4[i] > 0x100) {
            arg0->wheelOffsetE4[i] = 0x100;
        } else if (arg0->wheelOffsetE4[i] < -0x300) {
            arg0->wheelOffsetE4[i] = -0x300;
        }
        arg0->wheelOffsetE4[i] = (arg0->wheelOffsetE4[i] * 24) / 32;
    }

    if (arg0->steerRoll1DE > 0) {
        arg0->wheelOffsetE4[2] += s3;
        arg0->wheelOffsetE4[3] += s3;
    }

    s1 = 4 - CarBoundsTable[(s16)arg0->slotIndex1E6 >> 1]->reserved0B;
    if (s1 <= 0) {
        s1 = 4;
    }

    arg0->renderAngleX1AC = Atan2Table(arg0->halfSpan10E, ((arg0->wheelOffsetE4[0] + arg0->wheelOffsetE4[1]) - (arg0->wheelOffsetE4[2] + arg0->wheelOffsetE4[3])) >> 4) >> 3;
    arg0->renderAngleZ1AE = (s16)(Atan2Table(arg0->width10C, ((arg0->wheelOffsetE4[0] + arg0->wheelOffsetE4[2]) - (arg0->wheelOffsetE4[1] + arg0->wheelOffsetE4[3])) >> 4) >> 2) / s1;

    if (arg0->renderAngleZ1AE > 0x20) {
        arg0->wheelSuspensionDC[0] += arg0->steerPitch1DC / 20;
        arg0->wheelSuspensionDC[2] += arg0->steerPitch1DC / 16;
    } else if (arg0->renderAngleZ1AE < -0x20) {
        arg0->wheelSuspensionDC[1] += (-arg0->steerPitch1DC) / 20;
        arg0->wheelSuspensionDC[3] += (-arg0->steerPitch1DC) / 16;
    }

    for (i = 0; i < 4; i++) {
        arg0->wheelSuspensionDC[i] = (arg0->wheelSuspensionDC[i] * 22) / 32;
        if (arg0->wheelSuspensionDC[i] <= 0) {
            arg0->wheelSuspensionDC[i] = 0;
        } else if (arg0->wheelSuspensionDC[i] >= (arg0->height108 << 3)) {
            arg0->wheelSuspensionDC[i] = arg0->height108 << 3;
        }
    }

    arg0->wheelRotX1A8 = Atan2Table(arg0->halfSpan10E, ((arg0->wheelSuspensionDC[0] + arg0->wheelSuspensionDC[1]) - (arg0->wheelSuspensionDC[2] + arg0->wheelSuspensionDC[3])) >> 5);
    arg0->wheelRotZ1AA = Atan2Table(arg0->width10C, ((arg0->wheelSuspensionDC[0] + arg0->wheelSuspensionDC[2]) - (arg0->wheelSuspensionDC[1] + arg0->wheelSuspensionDC[3])) >> 5);
}
