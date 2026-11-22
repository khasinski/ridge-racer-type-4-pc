/* The opponents' driving at field rate (rr/field_rate.h): two fields of
 * each converted step make one original tick's. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rr/field_rate.h"
#include "rr/player_cars.h"
#include "rr/steering.h"
#include "rr/vehicle_rpm.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

static int field_step(void) {
    s32 rem = 0, x, tick;
    /* a filter keeping 94% a tick */
    RaceFieldRateLive = 0;
    tick = RaceFieldStep(100000, 100000 * 94 / 100, 0x10000u * 94 / 100, &rem);
    CHECK(tick == 94000);
    RaceFieldRateLive = 1;
    x = 100000;
    for (int i = 0; i < 2; i++)
        x = RaceFieldStep(x, x * 94 / 100, 0x10000u * 94 / 100, &rem);
    CHECK(abs(x - tick) <= 2);
    /* an add: exactly half and half */
    rem = 0;
    x = 1000;
    for (int i = 0; i < 2; i++)
        x = RaceFieldStep(x, x + 301, 0x10000u, &rem);
    CHECK(x == 1301);
    /* a snap: at once */
    rem = 0;
    CHECK(RaceFieldStep(1000, 7000, 0, &rem) == 7000);
    /* a long run of a filter keeping 1/25 of the gap to a target */
    rem = 0;
    x = 0;
    for (int i = 0; i < 40; i++)
        x = RaceFieldStep(x, x + (9000 - x) * 24 / 25, 0x10000u / 25, &rem);
    CHECK(abs(x - 9000) <= 25);
    RaceFieldRateLive = 0;
    return 0;
}

/* A corner turn's curve built in fields gives the same turn by real time. */
static int rate_curve(void) {
    Curve24 tickCurve, fieldCurve;
    s32 out, a, b, tickSum = 0, fieldSum = 0, worst = 0;
    const s32 disp = 1200, ticks = 0x30, rate = -9;
    BuildRateCurve(&tickCurve, disp, ticks, rate);
    BuildRateCurveQ12(&fieldCurve, disp, ticks * 2, rate * 2048);
    for (s32 t = 0; t < ticks; t++) {
        EvalRateCurve(&tickCurve, t, &out);
        tickSum += out;
        EvalRateCurve(&fieldCurve, 2 * t, &a);
        EvalRateCurve(&fieldCurve, 2 * t + 1, &b);
        fieldSum += a + b;
        if (abs(tickSum - fieldSum) > worst)
            worst = abs(tickSum - fieldSum);
    }
    /* the totals in 12-bit fixed point: within a fraction of a unit */
    CHECK(abs(tickSum - disp * 4096) < 4096);
    CHECK(abs(fieldSum - disp * 4096) < 4096);
    /* and the angle along the way within 2 units */
    CHECK(worst < 2 * 4096);
    return 0;
}

static void rpm_car(RaceVehicle *car) {
    memset(car, 0, sizeof(*car));
    car->control.gearCountFC = 5;
    for (int i = 0; i < 6; i++)
        car->control.shiftThresholds10C[i] = 300 + 200 * i;
    car->control.maxSpeedF8 = 1500 << 8;
    car->control.latch60 = 1;
}

/* The rpm of a top gear filter and of an upshift's hold. */
static int engine_rpm(void) {
    RaceVehicle tick, field;
    GameModeType = 1;
    for (int k = 0; k < 2; k++) {
        rpm_car(&tick);
        if (k == 0) {
            /* top gear, accelerating: the filter to the target */
            tick.control.gear68 = 4;
            tick.control.engine54.halves.accelerating = 1;
            tick.control.engine54.halves.rpm = 5000;
            tick.input = 1400;
            tick.control.steering8C = 6;
        } else {
            /* the upshift's hold: +300 a tick */
            tick.control.gear68 = 2;
            tick.control.holdFlag48 = 14;
            tick.control.shiftState64 = 1;
            tick.control.engine54.halves.rpm = 7000;
        }
        field = tick;
        memset(CarFieldStates, 0, sizeof(CarFieldStates));
        for (int n = 0; n < 6; n++) {
            RaceFieldRateLive = 0;
            UpdateEngineRpm(&tick);
            RaceFieldRateLive = 1;
            UpdateEngineRpm(&field);
            UpdateEngineRpm(&field);
            RaceFieldRateLive = 0;
            CHECK(abs((s32)tick.control.engine54.halves.rpm - (s32)field.control.engine54.halves.rpm) <= 8 + 2 * n);
        }
    }
    return 0;
}

int main(void) {
    if (field_step() || rate_curve() || engine_rpm())
        return 1;
    puts("field rate AI: ok");
    return 0;
}
