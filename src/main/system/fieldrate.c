/* Race logic at the video field rate (rr/field_rate.h). */
#include "rr/field_rate.h"
#include "rr/frame_loop.h"
#include "rr/player_cars.h"
#include "rr/vehicle_table.h"
#include "game/system/pad.h"
#include "game/data/menuvars.h"
#include <string.h>

/* RaceFieldRate: the host preferences own it (src/port/preferences.c). */
s32 RaceFieldRateLive;
s32 RaceTickField = 1;
u32 RaceFieldCount;

CarFieldState CarFieldStates[CAR_FIELD_STATES];
static CarFieldState CarFieldDummy;
CarFieldState CarFieldSnapshots[8];

/* The edges the slow lane has not seen yet, per pad. */
static struct {
    u16 pressed, repeat, released;
    s32 action[4];
} PadLatch[2];

/* One field, as the menus' 0x80, but a value of its own: a mode that sets
 * the menus' 0x80 (or the original 0x180) has left the race, which ends the
 * field rate wherever the game leaves its race (RaceFieldRateNextField). */
#define FIELD_RATE_THRESHOLD 0x81
static s32 RaceFieldRateThreshold(void) { return RaceFieldRateLive ? FIELD_RATE_THRESHOLD : 0x180; }

static void ResetLane(void) {
    RaceTickField = 1;
    memset(PadLatch, 0, sizeof(PadLatch));
}

void RaceFieldRateBegin(s32 gameMode) {
    /* The link exchanges inputs per original frame in lockstep with a
     * peer that may run the original rate; the attract demo stays as the
     * console shows it. */
    RaceFieldRateLive = RaceFieldRate && gameMode != 4 && gameMode != 5;
    CarPoseSettle(0);
    ResetLane();
    CarFieldStatesClear();
    VSyncThreshold = RaceFieldRateThreshold();
}

void RaceFieldRateResume(s32 live) {
    RaceFieldRateLive = live != 0;
    CarPoseSettle(0);
    ResetLane();
    VSyncThreshold = RaceFieldRateThreshold();
}

void RaceFieldRateEnd(void) {
    CarPoseSettle(1);
    RaceFieldRateLive = 0;
    ResetLane();
}

void RaceFieldRateTickNext(void) {
    if (RaceFieldRateLive)
        RaceTickField = 0;
}

void RaceFieldRateNextField(void) {
    int next;
    RaceFieldCount++;
    if (RaceFieldRateLive && VSyncThreshold != FIELD_RATE_THRESHOLD)
        RaceFieldRateEnd();
    if (!RaceFieldRateLive) {
        RaceTickField = 1;
        return;
    }
    next = !RaceTickField;
    /* A mode's first field is a tick, as a mode's first frame is a whole
     * frame on the PS1: its counters, setup and events run before anything
     * that runs every field reads them. The race is set up (and the lane
     * reset) before its load screens, whose length decides the phase the
     * race mode would otherwise start on: half of the races began on the
     * field between ticks, drawn with ModeFrame 0 before the race's first
     * tick (its ranking, step and camera setup) had run. */
    {
        static s32 laneMode = -1;
        s32 mode = (s32)(((u32)ModeGroup << 8) | (ModeIndex & 0xFF));
        if (mode != laneMode)
            next = 1;
        laneMode = mode;
    }
    for (int i = 0; i < 2; i++) {
        ControllerState5C *pad = &PlayerControllers[i];
        s32 *action[4] = {&pad->action28, &pad->action2C, &pad->action30, &pad->action34};
        PadLatch[i].pressed |= pad->edges08.fields.pressed;
        PadLatch[i].repeat |= pad->edges08.fields.repeat;
        PadLatch[i].released |= pad->released06;
        for (int k = 0; k < 4; k++)
            PadLatch[i].action[k] |= *action[k];
        if (next) {
            /* the tick: every edge since the last one, once */
            pad->edges08.fields.pressed = PadLatch[i].pressed;
            pad->edges08.fields.repeat = PadLatch[i].repeat;
            pad->released06 = PadLatch[i].released;
            for (int k = 0; k < 4; k++)
                *action[k] = PadLatch[i].action[k];
            memset(&PadLatch[i], 0, sizeof(PadLatch[i]));
        } else {
            pad->edges08.fields.pressed = 0;
            pad->edges08.fields.repeat = 0;
            pad->released06 = 0;
            for (int k = 0; k < 4; k++)
                *action[k] = 0;
        }
    }
    RaceTickField = next;
}

/* ---- draw-time rand ---- */

extern s32 PsyqRand(void);
static u32 DrawRandState = 0x24681357u;

s32 RaceDrawRand(void) {
    if (!RaceFieldRateLive)
        return PsyqRand();
    /* the BIOS rand()'s LCG and 15-bit result */
    DrawRandState = DrawRandState * 1103515245u + 12345u;
    return (s32)((DrawRandState >> 16) & 0x7FFF);
}

/* ---- fast-lane helpers ---- */

s32 RaceFieldApproach(s32 tickStep, s32 gap, s32 *rem) {
    unsigned long long r, k, v;
    int neg = gap < 0;
    if (!RaceFieldRateLive)
        return tickStep;
    {
        /* r: the share a tick keeps (Q16), from the tick's own step */
        long long left = (long long)gap - tickStep, g = gap;
        if (g == 0)
            return 0;
        if (left != 0 && ((left < 0) != (g < 0)))
            r = 0; /* overshoots: a tick lands past the target */
        else {
            if (left < 0) {
                left = -left;
                g = -g;
            }
            r = left >= g ? 0x10000ull : ((unsigned long long)left << 16) / (unsigned long long)g;
        }
    }
    k = (2 * r << 16) / (0x10000ull + r);
    v = (unsigned long long)(neg ? -(long long)gap : gap) * k + (unsigned)(*rem & 0xFFFF);
    *rem = (s32)(v & 0xFFFF);
    v >>= 16; /* what the field leaves of the gap */
    return neg ? gap + (s32)v : gap - (s32)v;
}

/* ---- per-car sidecar ---- */

CarFieldState *CarFieldStateOf(const void *car) {
    const RaceVehicle *v = car;
    if (v >= PlayerCars && v < PlayerCars + 2)
        return &CarFieldStates[v - PlayerCars];
    if (v >= ComCars && v < ComCars + 11)
        return &CarFieldStates[2 + (v - ComCars)];
    return &CarFieldDummy;
}

int CarFieldFast(const void *car) {
    const RaceVehicle *v = car;
    return RaceFieldRateLive && v >= PlayerCars && v < PlayerCars + 2 && (v->flags1E4 & 0x10);
}

void CarFieldStatesClear(void) {
    memset(CarFieldStates, 0, sizeof(CarFieldStates));
    memset(CarFieldSnapshots, 0, sizeof(CarFieldSnapshots));
}

void CarFieldStatesSave(void) {
    for (int i = 0; i < NumCars && i < 8; i++)
        CarFieldSnapshots[i] = *CarFieldStateOf(CarTable[i]);
}

void CarFieldStatesRestore(void) {
    CarPoseSettle(0);
    for (int i = 0; i < NumCars && i < 8; i++)
        *CarFieldStateOf(CarTable[i]) = CarFieldSnapshots[i];
}
