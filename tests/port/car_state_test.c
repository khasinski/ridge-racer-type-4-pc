/* Regression for native car records growing past the PS1 array size.
 * Entering a mode with no opponents must clear every byte of every slot,
 * including the last car and the tail of its pointer-bearing control. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rr/player_cars.h"
#include "rr/scene_camera_state.h"
#include "rr/menu_setup.h"
#include "rr/race_setup.h"
#include "game/car/carlane.h"
#include "game/race/raceset.h"
#include "game/car/carstate.h"
#include "game/car/cartimer.h"
#include "rr/field_rate.h"

extern s32 LapTimes[10], NumLaps;

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

static int first_lap(void) {
    RaceVehicle car = {0};
    GameModeType = 0;
    NumLaps = 9;
    TrackLength = 1000;
    CourseIndex = 0;
    BestLapTime = 60000;
    SplitIndex = 0;
    SplitTimes[0] = 123;
    BestSplitTimes[0] = 456;
    memset(LapFrameCounts, 0, sizeof(LapFrameCounts));
    for (unsigned i = 0; i < 10; ++i) LapTimes[i] = -1;
    UpdateLapTiming(&car);
    CHECK(car.control.dynamics94.steering.count22 == 1);
    CHECK(BestLapTime == 60000);
    CHECK(LatchedSplitTime == 123 && LatchedBestSplitTime == 456);
    for (unsigned i = 0; i < 10; ++i) CHECK(LapTimes[i] == -1 && LapFrameCounts[i] == 0);
    /* Crossing after a full lap still records a new best time. */
    car.first = TrackLength;
    LapFrameCounts[0] = 100;
    UpdateLapTiming(&car);
    CHECK(car.control.dynamics94.steering.count22 == 2);
    CHECK(LapFrameCounts[0] == 101);
    CHECK(BestLapTime == LapTimes[0] && BestLapTime > 0 && BestLapTime < 60000);
    for (unsigned i = 1; i < 10; ++i) CHECK(LapTimes[i] == -1 && LapFrameCounts[i] == 0);
    return 0;
}

/* The car stages every field (rr/field_rate.h): one tick of the original
 * against two fields, sampled at the ticks. */
static void set_rate(int field) {
    RaceFieldRateLive = field;
    CarFieldStatesClear();
}

static int field_filters(void) {
    /* x = (x + 16) * 26 / 32 a tick settles where its field form does
     * (the original's truncation settles anywhere in 65..69) */
    s32 x = 0, y = 0, rem = 0, remAdd = 0;
    set_rate(0);
    for (int t = 0; t < 200; t++)
        x = ((x + 16) * 26) / 32;
    set_rate(1);
    for (int f = 0; f < 400; f++) {
        y += RaceFieldFilterAdd(16, 26, 32, 1, &remAdd);
        y = RaceFieldKeep(y, 26, 32, &rem);
    }
    CHECK(x >= 65 && x <= 69 && y >= 65 && y <= 69);
    return 0;
}

static int field_shake(void) {
    /* A wall hit's roll shake (UpdateCarShake, state 2): the same waveform
     * at every tick, over the same 30 ticks. */
    static s16 tick[32];
    RaceVehicle *car = &ComCars[0];
    int t, f, end = -1;
    for (int field = 0; field < 2; field++) {
        set_rate(field);
        memset(car, 0, sizeof *car);
        car->input = 0x400;
        car->trackAngle1EC = 0x200;
        SetCarImpactState(2, car);
        CHECK(car->stateTimer1A5 == 30 && car->steerAngle1A2 != 0);
        for (t = 0; t < 32 * (field + 1); t++) {
            car->renderAngleZ1AE = 0;
            UpdateCarShake(car);
            if (!field) {
                tick[t] = car->shakeRollOffset1B6;
                if (!car->state1A4 && end < 0) end = t;
            } else if (t & 1) {
                CHECK(abs(car->shakeRollOffset1B6 - tick[t / 2]) <= 1);
                if (!car->state1A4) CHECK(t / 2 >= end);
            } else if (t / 2 < end) {
                CHECK(car->state1A4 == 2);
            }
        }
        CHECK(!car->state1A4);
    }
    /* the kerb's contact shake (contactTimer1A7): 10 ticks, 20 fields */
    for (int field = 0; field < 2; field++) {
        set_rate(field);
        memset(car, 0, sizeof *car);
        car->input = 0x400;
        car->contactTimer1A7 = 10;
        for (f = 0; car->contactTimer1A7 && f < 100; f++)
            UpdateCarShake(car);
        CHECK(f == 10 * (field + 1));
    }
    return 0;
}

static int field_offset(void) {
    /* A push off a car (DecayCarOffset): as far by every tick. */
    static s32 pos[16];
    RaceVehicle *car = &ComCars[0];
    for (int field = 0; field < 2; field++) {
        set_rate(field);
        memset(car, 0, sizeof *car);
        car->shakeActive19C = 2;
        car->shakeTimer19E = 15;
        car->shakeX198 = 3000;
        car->shakeY19A = -1800;
        for (int t = 0; t < 16; t++) {
            DecayCarOffset(car);
            if (field)
                DecayCarOffset(car);
            if (!field)
                pos[t] = car->posScaled00[0];
            else
                CHECK(abs(car->posScaled00[0] - pos[t]) <= 30);
        }
        CHECK(!car->shakeActive19C);
    }
    return 0;
}

int main(void) {
    CHECK(!first_lap());
    CHECK(!field_filters());
    CHECK(!field_shake());
    CHECK(!field_offset());
    set_rate(0);
    static RaceSpecEntry30 specs[11];
    RaceContext.carSpecs540 = specs;
    GameModeType = 0;
    SelectedTeamIndex = 0;
    for (int pass = 0; pass < 2; pass++) {
        memset(ComCars, pass ? 0x5A : 0xA5, sizeof ComCars);
        InitComCars(0);
        const unsigned char *bytes = (const unsigned char *)ComCars;
        for (size_t i = 0; i < sizeof ComCars; i++) {
            if (bytes[i]) {
                fprintf(stderr, "Opponent state survived reset at byte %zu of %zu\n", i, sizeof ComCars);
                return 1;
            }
        }
    }
    return 0;
}
