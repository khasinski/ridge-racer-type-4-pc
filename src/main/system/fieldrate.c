/* Race logic at the video field rate (rr/field_rate.h). */
#include "rr/field_rate.h"
#include "rr/frame_loop.h"
#include "rr/player_cars.h"
#include "rr/vehicle_table.h"
#include "game/system/pad.h"
#include "game/data/menuvars.h"
#include <stdlib.h>
#include <string.h>

/* RaceFieldRate: the host preferences own it (src/port/preferences.c). */
s32 RaceFieldRateLive;
s32 RaceTickField = 1;
s32 RaceFieldCarStep;
u32 RaceFieldCount;

/* The car stages every field unless RR4_CAR_FIELD_STEP=0 (rr/field_rate.h). */
static int CarStepWanted(void) {
    static int wanted = -1;
    if (wanted < 0) {
        const char *e = getenv("RR4_CAR_FIELD_STEP");
        wanted = !(e && *e == '0');
    }
    return wanted;
}

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
    RaceFieldCarStep = RaceFieldRateLive && CarStepWanted();
    ResetLane();
    CarFieldStatesClear();
    VSyncThreshold = RaceFieldRateThreshold();
}

void RaceFieldRateResume(s32 live) {
    RaceFieldRateLive = live != 0;
    RaceFieldCarStep = RaceFieldRateLive && CarStepWanted();
    ResetLane();
    VSyncThreshold = RaceFieldRateThreshold();
}

void RaceFieldRateEnd(void) {
    RaceFieldRateLive = 0;
    RaceFieldCarStep = 0;
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
    int neg = x < 0;
    if (!RaceFieldRateLive)
        return (s32)((long long)x * (s32)num / (s32)den);
    /* on the magnitude, as the game's divisions truncate toward zero: a
     * small value decays to 0 from either side */
    v = (long long)(neg ? -(long long)x : x) * RaceFieldKeepQ16(num, den) + (*rem & 0xFFFF);
    *rem = (s32)(v & 0xFFFF);
    v >>= 16;
    return (s32)(neg ? -v : v);
}

u32 RaceFieldDecayKeepQ16(s32 tickResult, s32 x) {
    long long r = tickResult, v = x;
    if (v == 0)
        return 0x10000u; /* nothing to take */
    if (r != 0 && ((r < 0) != (v < 0)))
        return 0;
    if (r < 0) {
        r = -r;
        v = -v;
    }
    if (r >= v)
        return 0x10000u; /* this filter keeps all of it (or grows) */
    return IsqrtU64(((unsigned long long)r << 32) / (unsigned long long)v);
}

s32 RaceFieldDecay(s32 tickResult, s32 x, s32 *rem) {
    long long v;
    int neg = x < 0;
    if (!RaceFieldRateLive)
        return tickResult;
    if ((long long)tickResult * (neg ? -1 : 1) > (long long)x * (neg ? -1 : 1))
        return tickResult; /* growth: not a decay (none in the converted code) */
    v = (long long)(neg ? -(long long)x : x) * RaceFieldDecayKeepQ16(tickResult, x) + (*rem & 0xFFFF);
    *rem = (s32)(v & 0xFFFF);
    v >>= 16;
    return (s32)(neg ? -v : v);
}

s32 RaceFieldApproach(s32 tickStep, s32 gap, s32 *rem) {
    unsigned long long r, k, v;
    int neg = gap < 0;
    if (!RaceFieldRateLive)
        return tickStep;
    {
        /* r: the share a tick keeps (Q16), as RaceFieldDecay finds it */
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

s32 RaceFieldFilterAdd(s32 delta, u32 num, u32 den, int before, s32 *rem) {
    long long v;
    u32 k, f;
    if (!RaceFieldRateLive)
        return delta;
    k = RaceFieldKeepQ16(num, den);
    /* sqrt(k)/(1+sqrt(k)) or 1/(1+sqrt(k)), Q16 */
    f = (u32)(((unsigned long long)(before ? k : 0x10000u) << 16) / (0x10000u + k));
    v = (long long)delta * f + *rem;
    *rem = (s32)(v & 0xFFFF);
    return (s32)(v >> 16);
}

int RaceFieldOncePerTick(s32 *last) {
    if (!RaceFieldRateLive)
        return 1;
    /* *last = the field it fired on + 1 */
    if (*last != 0 && RaceFieldCount - (u32)(*last - 1) < 2u)
        return 0;
    *last = (s32)(RaceFieldCount + 1);
    return 1;
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

s32 RaceFieldStep(s32 x, s32 next, u32 slopeQ16, s32 *remQ16) {
    long long v;
    u32 root;
    if (!RaceFieldRateLive)
        return next;
    if (slopeQ16 > 0x10000u)
        slopeQ16 = 0x10000u;
    /* sqrt(slope) in Q16 = sqrt(slope << 16) */
    root = IsqrtU64((unsigned long long)slopeQ16 << 16);
    /* (next - x) / (1 + root), in Q16 with the rounding carried */
    v = ((long long)(next - x) << 32) / (0x10000 + root) + *remQ16;
    *remQ16 = (s32)(v & 0xFFFF);
    return x + (s32)(v >> 16);
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
    for (int i = 0; i < NumCars && i < 8; i++)
        *CarFieldStateOf(CarTable[i]) = CarFieldSnapshots[i];
}
