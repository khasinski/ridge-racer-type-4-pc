#include "game/car/carctrl.h"
#include "rr/player_cars.h"
#include "rr/scene_camera_state.h"
#include "scratchpad.h"
#include "rr/track_projection.h"
#include "rr/race.h"
#include "game/car/carspec3.h"
#include "game/race/racedist.h"
#include "game/car/carai.h"
#include "game/course/trkproj.h"
#include "rr/track_lateral.h"
#include "rr/vehicle_drive.h"
#include "game/car/cardrive.h"
#include "psyq/libc/rand.h"
#include "rr/field_rate.h"

extern int RaceState;
#include "rr/audio_state_types.h"

/* Standings rows hold vehicle pointers, so the table the PS1 built in the
 * scratchpad at +0x7C is an ordinary host object. */
static RaceStandingsTable ComCarStandings;

/* At field rate (rr/field_rate.h) this runs every field: the decisions
 * (lane changes, overtakes, corner plans, shifts) as the events they are,
 * the integrators with half a tick's step, the timers on the original
 * tick, every quantity kept in its per-tick units. */
void UpdateComCars(void) {
    int i;
    RaceVehicle *s0;
    RaceVehicleControl *s1;
    int r;
    const int tick = RaceSlowTick();

    RaceContext.standings = &ComCarStandings;
    UpdateRaceStandings();
    switch (RaceState) {
    case 0:
        SoundState.banks24.race.counter = 0;
        break;
    case 1:
        if (tick)
            SoundState.banks24.race.counter += 1U;
        break;
    }
    for (i = 0; i < 0xB; i++) {
        s0 = &ComCars[i];
        s1 = &s0->control;
        if ((short)s0->flags1E4 > 0) {
            CalcDistanceToPlayer(s0);
            if (RaceState < 2) {
                CarFieldState *fs = CarFieldStateOf(s0);
                /* the step histories start at the grid */
                fs->hist[CAR_FIELD_HIST_AI_HEADING] = s0->heading1D4;
                fs->hist[CAR_FIELD_HIST_AI_INPUT] = s0->input;
                if (RaceState == 0) {
                    /* its rand calls once per original tick */
                    if (!tick)
                        continue;
                    s1->slideTimer5C = PsyqRand() % 1000 + 0x1D4C;
                    r = PsyqRand();
                    s1->engine54.halves.accelerating = 1;
                    s1->engine54.halves.rpm = 0x400;
                    s1->latch60 = 0;
                    s1->slideDivisor5E = r % 100 + 0x190;
                } else if ((u16)s1->engine54.halves.accelerating == 1) {
                    s1->engine54.halves.rpm = s1->engine54.halves.rpm +
                        RaceFieldHalf((u16)s1->slideDivisor5E, &fs->rem[CAR_FIELD_REM_AI_REVS]);
                    if (s1->engine54.halves.rpm > s1->slideTimer5C) {
                        s1->engine54.halves.accelerating = 0;
                        s1->slideTimer5C = PsyqRand() % 1000 + 0x157C;
                        s1->slideDivisor5E = PsyqRand() % 100 + 0xC8;
                    }
                } else {
                    s1->engine54.halves.rpm = s1->engine54.halves.rpm -
                        RaceFieldHalf((u16)s1->slideDivisor5E, &fs->rem[CAR_FIELD_REM_AI_REVS]);
                    if (s1->engine54.halves.rpm < s1->slideTimer5C) {
                        s1->engine54.halves.accelerating = 1;
                        s1->slideTimer5C = PsyqRand() % 1000 + 0x1D4C;
                        s1->slideDivisor5E = PsyqRand() % 100 + 0x190;
                    }
                }
            } else {
                ApplyRubberBandSpec(s0);
                ComputeCarTrackSurfaceProj(s0);
                UpdateComCarOvertake(s0);
                UpdateComCarTargetPoint(s0);
                UpdateComCarDrive(s0);
                UpdateComCarLaneChange(s0);
                UpdateComCarSpeed(s0);
            }
        }
    }
}
