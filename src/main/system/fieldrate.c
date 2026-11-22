/* Race logic at the video field rate (rr/field_rate.h). */
#include "rr/field_rate.h"
#include "rr/frame_loop.h"
#include "rr/player_cars.h"
#include "rr/vehicle_table.h"
#include "game/system/pad.h"
#include <string.h>

/* RaceFieldRate: the host preferences own it (src/port/preferences.c). */
s32 RaceFieldRateLive;
s32 RaceTickField = 1;

CarFieldState CarFieldStates[CAR_FIELD_STATES];
static CarFieldState CarFieldDummy;
static CarFieldState CarFieldSnapshots[8];

/* The edges the slow lane has not seen yet, per pad. */
static struct {
    u16 pressed, repeat, released;
    s32 action[4];
} PadLatch[2];

/* One field, as the menus' 0x80, but a value of its own: a mode that sets
 * the menus' 0x80 (or the original 0x180) has left the race, which ends the
 * field rate wherever the game leaves its race (RaceFieldRateNextField). */
#define FIELD_RATE_THRESHOLD 0x81
s32 RaceFieldRateThreshold(void) { return RaceFieldRateLive ? FIELD_RATE_THRESHOLD : 0x180; }

static void ResetLane(void) {
    RaceTickField = 1;
    memset(PadLatch, 0, sizeof(PadLatch));
}

void RaceFieldRateBegin(s32 gameMode) {
    /* The link exchanges inputs per original frame in lockstep with a
     * peer that may run the original rate; the attract demo stays as the
     * console shows it. */
    RaceFieldRateLive = RaceFieldRate && gameMode != 4 && gameMode != 5;
    ResetLane();
    CarFieldStatesClear();
    VSyncThreshold = RaceFieldRateThreshold();
}

void RaceFieldRateResume(s32 live) {
    RaceFieldRateLive = live != 0;
    ResetLane();
    VSyncThreshold = RaceFieldRateThreshold();
}

void RaceFieldRateEnd(void) {
    RaceFieldRateLive = 0;
    ResetLane();
}

void RaceFieldRateTickNext(void) {
    if (RaceFieldRateLive)
        RaceTickField = 0;
}

void RaceFieldRateNextField(void) {
    int next;
    if (RaceFieldRateLive && VSyncThreshold != FIELD_RATE_THRESHOLD)
        RaceFieldRateEnd();
    if (!RaceFieldRateLive) {
        RaceTickField = 1;
        return;
    }
    next = !RaceTickField;
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

static u32 IsqrtU64(unsigned long long v) {
    unsigned long long r = 0, bit = 1ULL << 62;
    while (bit > v)
        bit >>= 2;
    while (bit) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (u32)r;
}

u32 RaceFieldKeepQ16(u32 num, u32 den) {
    /* sqrt(num/den) in Q16 = sqrt(num * 2^32 / den) */
    if (!den || num >= den)
        return num >= den ? 0x10000u : 0;
    return IsqrtU64(((unsigned long long)num << 32) / den);
}

s32 RaceFieldKeep(s32 x, u32 num, u32 den, s32 *rem) {
    long long v;
    if (!RaceFieldRateLive)
        return (s32)((long long)x * (s32)num / (s32)den);
    v = (long long)x * RaceFieldKeepQ16(num, den) + *rem;
    *rem = (s32)(v & 0xFFFF);
    return (s32)(v >> 16);
}

s32 RaceFieldDelta(s32 now, s32 *prev1, s32 *prev2) {
    s32 d;
    if (!RaceFieldRateLive) {
        d = now - *prev1;
        *prev2 = *prev1;
        *prev1 = now;
        return d;
    }
    d = now - *prev2;
    *prev2 = *prev1;
    *prev1 = now;
    return d;
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

void CarFieldStatesClear(void) {
    memset(CarFieldStates, 0, sizeof(CarFieldStates));
    memset(CarFieldSnapshots, 0, sizeof(CarFieldSnapshots));
}

void CarFieldStatesSave(void) {
    for (int i = 0; i < NumCars && i < 8; i++)
        CarFieldSnapshots[i] = *CarFieldStateOf(CarTable[i]);
}

void CarFieldStatesRestore(void) {
    for (int i = 0; i < NumCars && i < 8; i++)
        *CarFieldStateOf(CarTable[i]) = CarFieldSnapshots[i];
}
