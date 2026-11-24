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

int main(void) {
    CHECK(!first_lap());
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
