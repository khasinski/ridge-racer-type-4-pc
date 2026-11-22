#include "types.h"
#include "rr/vehicle_drive.h"
#include "rr_asm.h"
#include "rr/menu_overlay_runtime.h"
#include "rr/track_projection.h"
#include "rr/field_rate.h"


typedef struct {
    MatrixWords32 m;
    Vec3sFields8 sv;
    Vec3sFields8 out;
    Vec3sFields8 rot;
} DriveGteScratch;

void UpdateComCarSpeed(RaceVehicle *a0);

void UpdateComCarDrive(RaceVehicle *arg0) {
    s32 sp18;
    s32 sp1C;
    s32 sp20;
    s16 temp_a0_3;
    s16 temp_v0_3;
    s32 var_s0;
    s32 heading;
    s16 var_v1_2;
    s32 temp_a0_2;
    s32 temp_a1;
    s32 temp_a1_2;
    s32 temp_v0_4;
    s32 temp_v0_5;
    s32 temp_v1_3;
    s32 var_a0_2;
    s32 var_a0_3;
    u16 temp_a0;
    u16 temp_v1;
    RaceVehicleControl *temp_s1;

    CarFieldState *fs = CarFieldStateOf(arg0);

    temp_s1 = &arg0->control;
    /* At field rate prevHeading1D6 stays the heading of one original tick
     * ago (two fields), as the body roll reads it (UpdateCarSteering). */
    if (RaceFieldRateLive) {
        arg0->prevHeading1D6 = (u16) fs->hist[CAR_FIELD_HIST_AI_HEADING];
        fs->hist[CAR_FIELD_HIST_AI_HEADING] = arg0->heading1D4;
    } else {
        arg0->prevHeading1D6 = (u16) arg0->heading1D4;
    }
    temp_a1 = ratan2(arg0->control.pathDeltaX30, arg0->control.pathDeltaZ34);
    temp_v1 = *(u16 *)&arg0->heading1D4;
    temp_a0 = temp_v1 & 0xFFF;
    arg0->heading1D4 = temp_a0;
    if (temp_v1 & 0x800) {
        arg0->heading1D4 = (u16) (temp_a0 | 0xF000);
    }
    heading = arg0->heading1D4;
    var_s0 = heading - temp_a1;
    var_s0 &= 0xFFF;
    if (var_s0 & 0x800) {
        var_s0 |= ~0xFFF;
    }
    {
        s32 value;
        s32 channel;
        s32 steering;
        s32 negEighth;
        value = CalcSteerLimit(arg0);
        steering = var_s0;
        temp_s1->limit6E = value;
        channel = temp_s1->error92;
        value = (value * 0x10000U);
        var_s0 = value >> 4;
        temp_s1->error92 = steering;
        value = steering - channel;
        /* The D term is the error's change over one original tick: over
         * the last two fields at field rate. */
        if (RaceFieldRateLive)
            value = RaceFieldDelta(steering, &fs->hist[CAR_FIELD_HIST_AI_ERROR1],
                                   &fs->hist[CAR_FIELD_HIST_AI_ERROR2]);
        if (steering < 0) steering += 7;
        channel = value;
        value = steering >> 3;
        negEighth = -value;
        if (channel < 0) channel += 3;
        value = channel >> 2;
        temp_v0_3 = negEighth - value;
    }
    var_v1_2 = temp_v0_3;
    if (temp_v0_3 > 0) {
        if (temp_s1->limit6E < temp_v0_3) {
            var_v1_2 = (s16) (u16) temp_s1->limit6E;
        }
    } else if (temp_v0_3 < -temp_s1->limit6E) {
        var_v1_2 = -(s16) (u16) temp_s1->limit6E;
    }
    var_a0_2 = -0xF000;
    temp_v0_4 = (temp_s1->filtered90 + var_v1_2) / 2;
    /* the filter keeps half per tick */
    temp_v0_4 = RaceFieldStep(temp_s1->filtered90, temp_v0_4, 0x8000,
                              &fs->rem[CAR_FIELD_REM_AI_STEER_Q16]);
    temp_s1->filtered90 = (s16) temp_v0_4;
    temp_s1->steering8C = (u16) temp_v0_4;
    temp_v1_3 = -(s16) temp_v0_4 * 0x90;
    arg0->lateralMotion178 = temp_v1_3;
    if ((temp_v1_3 < -0xF000) || (var_a0_2 = 0xF000, ((temp_v1_3 > 0xF000) != 0))) {
        arg0->lateralMotion178 = var_a0_2;
    }
    if (arg0->input > 0) {
        if (arg0->steeringLock1B8 == 0) {
            /* The tick turns by the filter's value at its end. Two fields
             * of half the field's value lag that; a lead of
             * (3 - 2 sqrt 2) of the filter's remaining gap to the output
             * makes them turn as the tick does. */
            s32 turn = temp_s1->steering8C;
            if (RaceFieldRateLive) {
                /* half of it a field, in Q16 with the remainder carried */
                s32 *rem = &fs->rem[CAR_FIELD_REM_AI_HEADING];
                long long q = ((long long)(s16)temp_s1->steering8C << 16) +
                              (long long)(var_v1_2 - (s16)temp_s1->steering8C) * 11244;
                q = (q >> 1) + *rem;
                *rem = (s32)(q & 0xFFFF);
                turn = (s32)(q >> 16);
            }
            arg0->heading1D4 = (u16) (arg0->heading1D4 + turn);
        }
    }
    temp_a1_2 = ScanCurveAhead(arg0, temp_s1->laneTimer38, &sp18, &sp1C, &sp20);
    temp_s1->dragFlag46 = 0;
    if (temp_s1->shiftDir62 != 0) {
        temp_s1->gear68 = (u16) (temp_s1->gear68 + (u16) temp_s1->shiftDir62);
        if (temp_s1->shiftDir62 > 0) {
            temp_s1->holdFlag48 = 16;
        } else {
            temp_s1->holdFlag48 = 5;
        }

    }
    if (temp_s1->holdFlag48 > 0) {
        /* The shift's hold counts original ticks: a tick is a slow-lane
         * field and the next (RaceTickEnd), and a shift starts on the
         * slow-lane field after its decision, so the hold counts down on
         * the second, keeping the original's 2 x hold fields without
         * drive. */
        if (RaceTickEnd())
            temp_s1->holdFlag48 = (s16) ((u16) temp_s1->holdFlag48 - 1);
        temp_s1->accel50 = 0;
    } else if (temp_a1_2 < var_s0) {
        temp_s1->accel50 = (u16) temp_s1->gearRatios100[temp_s1->gear68];
    } else if (arg0->input != 0) {
        s32 half_limit = var_s0 >> 1;
        temp_a0_2 = ((u32)temp_a1_2 - var_s0);
        temp_s1->accel50 = 0;
        if (half_limit < temp_a0_2) {
            temp_s1->dragFlag46 = 1;
            heading = (var_s0 * 2) / 3;
            if ((heading < temp_a0_2) && (temp_s1->steeringEnabled124 != 0) && (temp_s1->dynamics94.steering.mode00 == 0)) {
                PlanComCarCornerTurn(arg0);
            }
        }
    } else {
        temp_s1->accel50 = (u16) temp_s1->gearRatios100[0];
    }
    if (temp_s1->accel50 > 0) {
        var_a0_3 = arg0->rot50;
        if (var_a0_3 < -0xC8) {
            var_a0_3 = -0xC8;
        } else if (var_a0_3 >= 0x65) {
            var_a0_3 = 0x64;
        }
        temp_v0_5 = (s32)((u32)temp_s1->accel50 * (var_a0_3 + 0x3E8)) / 1000;
        temp_s1->accel50 = temp_v0_5;
        if (temp_v0_5 < 0) {
            temp_s1->accel50 = 0;
        }
    }
    temp_a0_3 = temp_s1->attenuation4A;
    if (temp_a0_3 > 0) {
        temp_s1->accel50 = (s32)((u32)temp_s1->accel50 * (0x10-temp_a0_3)) / 16;
    }
    if (temp_s1->accel50 == 0) {
        temp_s1->engine54.halves.accelerating = 0;
    } else {
        temp_s1->engine54.halves.accelerating = 1;
    }
    UpdateEngineRpm(arg0);
}

void UpdateComCarSpeed(RaceVehicle *a0) {
    DriveGteScratch f;
    RaceVehicle *s1 = a0;
    RaceVehicleControl *s0 = &s1->control;
    s32 v0, v1, a1;
    s32 a;
    CarFieldState *fs = CarFieldStateOf(a0);
    const int tick = RaceSlowTick();
    s32 *speedRem = &fs->rem[CAR_FIELD_REM_AI_SPEED];
    a1 = s0->accel50;
    if (a1 != 0) {
        v1 = s0->boostTimer44;
        if (v1 > 0) {
            a = s0->divisor11C;
            v0 = a;
            if (a < 0) v0 = -v0;
            v1 = v0 - v1;
            v1 = (a1 * v1) / v0;
            if (a > 0) {
                v0 = s0->factor11E;
                v0 = (v1 * v0) / 1024;
                v1 = s0->speed4C;
                v1 += RaceFieldHalf(a1 + v0, speedRem);
                s0->speed4C = v1;
            } else {
                s0->speed4C = s0->speed4C + RaceFieldHalf(v1, speedRem);
            }
            /* the start boost lasts its original ticks */
            if (tick)
                s0->boostTimer44 = (u16)s0->boostTimer44 - 1;
        } else {
            s0->speed4C = s0->speed4C + RaceFieldHalf(a1, speedRem);
        }
        s1->control.dragFlag46 = 0;
        v0 = s0->speed4C;
        v1 = s0->maxSpeedF8;
        if (v1 < v0) s0->speed4C = v1;
    } else {
        if (s0->holdFlag48 == 0) {
            /* per-tick shares: their square roots per field */
            if (s0->dragFlag46 != 0) {
                s0->speed4C = RaceFieldStep(s0->speed4C, (s0->speed4C * 94) / 100,
                                            0x10000u * 94 / 100, &fs->rem[CAR_FIELD_REM_AI_DRAG_Q16]);
            } else {
                s0->speed4C = RaceFieldStep(s0->speed4C, (s0->speed4C * 99) / 100,
                                            0x10000u * 99 / 100, &fs->rem[CAR_FIELD_REM_AI_DRAG_Q16]);
            }
        }
    }
    /* prevInput1DA: the speed one original tick ago (two fields), as the
     * body pitch and the contact sounds read it */
    if (RaceFieldRateLive) {
        s1->prevInput1DA = (u16)fs->hist[CAR_FIELD_HIST_AI_INPUT];
        fs->hist[CAR_FIELD_HIST_AI_INPUT] = s1->input;
    } else {
        s1->prevInput1DA = (u16)s1->input;
    }
    s1->input = s0->speed4C / 256;
    f.rot.x = 0;
    f.rot.y = (u16)s1->heading1D4;
    f.rot.z = 0;
    RotMatrix(&f.rot, &f.m);
    f.sv.x = 0;
    f.sv.y = 0;
    f.sv.z = s1->input / 16;
    RR_GTE_SETROT(f.m.w);
    RR_GTE_MVMVA_ROT_V0_NOCV(&f.sv);
    RR_GTE_STORE_IR_S16(&f.out);
    v0 = s0->gearCountFC;
    v1 = s0->gear68;
    s0->shiftDir62 = 0;
    /* The shift points are the original tick's: the speed crosses a
     * threshold by up to a tick's step there (the next field shifts). */
    if (!RaceTickEnd()) {
    } else if (v1 < v0 - 1) {
        if (s1->input > s0->shiftThresholds10C[v1]) {
            s0->shiftDir62 = 1;
            s0->shiftState64 = 1;
        }
    }
    v0 = s0->gear68;
    if (v0 > 0 && RaceTickEnd()) {
        v0 = v0 - 1;
        v1 = s0->shiftThresholds10C[v0] - 0x24;
        if (v1 < 0) v1 = 0;
        if (s1->input < v1) {
            v1 = s0->engine54.halves.rpm;
            s0->shiftDir62 = -1;
            s0->shiftState64 = -1;
            v0 = (0x2800 - v1) / 4;
            s0->shiftDelay66 = v0;
            if ((s16)v0 < 0) s0->shiftDelay66 = 0;
        }
    }
    s1->vel30 = f.out.x;
    s1->vel38 = f.out.z;
    UpdateCarTrackProjection(s1);
}
