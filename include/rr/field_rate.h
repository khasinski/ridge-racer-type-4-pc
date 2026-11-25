#ifndef RR_FIELD_RATE_H
#define RR_FIELD_RATE_H

#include "types.h"

/* Race logic at the video field rate (Escape > Graphics > Race frame rate).
 *
 * The PS1 game runs a race at one frame every two fields: 25 Hz PAL, 30 Hz
 * NTSC (VSyncThreshold 0x180). With RaceFieldRate on (the default) a race
 * runs its mode function every field, 50 / 60 Hz (VSyncThreshold 0x80),
 * without interpolation. The game's per-frame quantities fall in two lanes:
 *
 * Slow lane: everything that counts frames or acts on one frame's event
 * (mode counters ModeFrame / ScreenFrame, fades, the countdown and its
 * voices, lap counting, HUD and sound timers) runs once per original
 * tick, on the fields where RaceSlowTick() is non-zero (every second
 * field). Pad edges (pressed, repeat, released, the action words) are
 * latched over the field between: the slow lane sees every press once, on
 * its tick; nothing sees an edge on the other field.
 *
 * The cars are the original's: StepRaceFrame (racemai2.c) runs the
 * original tick of every car (the player cars' inputs and update, the
 * opponents' driving, every car stage, the contact) on the tick's first
 * field with the rate off, so a race, its rubber band and its replay are
 * the released game's tick for tick, and draws each car half way from the
 * last tick's end to this one's there; the second field runs no car and
 * draws the tick's end.
 *
 * Fast lane: presentation converted to run every field with half the
 * original step (RaceFieldHalf, RaceFieldApproach), its state kept in the
 * original per-tick units: the chase camera's ease, the replay and track
 * cameras, course objects and draw-time effects, the sound.
 *
 * The rate is chosen when a race is set up (RaceFieldRateBegin) and holds
 * until the race and its replay end. The serial link race, the attract
 * demo and a replay recorded at the original rate run at the original
 * rate. With the rate off, every helper below is the identity: the game
 * runs as released. */

/* The option: non-zero = races at field rate (from the next race). */
extern s32 RaceFieldRate;
/* This race runs at field rate. */
extern s32 RaceFieldRateLive;
/* Non-zero on the fields where the original tick runs (always when the
 * race is not at field rate). */
extern s32 RaceTickField;

static inline int RaceSlowTick(void) { return !RaceFieldRateLive || RaceTickField; }
/* Fields per original tick: 2 at field rate, else 1. */
static inline int RaceFieldsPerTick(void) { return RaceFieldRateLive ? 2 : 1; }

/* Fields since start-up (RaceFieldRateNextField counts them). */
extern u32 RaceFieldCount;

/* Race setup (SetupRaceScene): the rate for this race by its game mode
 * (GameModeType 4, the link, and 5, the attract demo, stay at the original
 * rate). Sets VSyncThreshold: one field (0x81) at field rate, else 0x180.
 * Code that tests VSyncThreshold == 0x80 for "one field a frame" is menu
 * code; race code uses RaceFieldRateLive. */
void RaceFieldRateBegin(s32 gameMode);
/* Back to the original rate (the race's and its replay's end). */
void RaceFieldRateEnd(void);
/* A replay's rate (the rate it was recorded at); sets VSyncThreshold. */
void RaceFieldRateResume(s32 live);
/* The next field is a tick (a replay's first frame follows its
 * snapshot's tick). */
void RaceFieldRateTickNext(void);
/* Frame loop, after PadUpdate: the next field's lane and the pad edges'
 * latch. */
void RaceFieldRateNextField(void);

/* ---- field time (cameras, course objects, draw-time effects) ---- */

/* 1 on the field between two ticks (half a tick after the last one). */
static inline int RaceFieldPhase(void) { return RaceFieldRateLive && !RaceTickField; }
/* The time in fields of a tick counter, for code that runs every field:
 * After for a counter the tick advanced before the code reads it (ModeFrame
 * at the top of a mode function), Before for one it advances after. */
static inline s32 RaceFieldTimeAfter(s32 frame) { return frame * RaceFieldsPerTick() + RaceFieldPhase(); }
static inline s32 RaceFieldTimeBefore(s32 frame) { return frame * RaceFieldsPerTick() - RaceFieldPhase(); }
/* A per-tick add `d` of code that runs once per field: the tick's field
 * takes d - d/2, the other d/2, so a tick's two fields add exactly d. */
static inline s32 RaceFieldSplit(s32 d) {
    if (!RaceFieldRateLive)
        return d;
    return RaceTickField ? d - d / 2 : d / 2;
}
/* A colour cycle table (GPU colour words: rgb in the low bytes, the
 * command byte on top) read at a tick counter `index` (ScreenFrame plus a
 * phase, the table `mask + 1` long): on the field between ticks the colour
 * half way to the next entry. */
static inline u32 RaceFieldCycleColor(const s32 *table, u32 index, u32 mask) {
    u32 a = (u32)table[index & mask], b, out;
    if (!RaceFieldPhase())
        return a;
    b = (u32)table[(index + 1) & mask];
    out = a & 0xFF000000u;
    for (int k = 0; k < 24; k += 8)
        out |= ((((a >> k) & 0xFF) + ((b >> k) & 0xFF)) >> 1) << k;
    return out;
}
/* rand() for draw-time flicker: PsyqRand at the original rate (as
 * released); at field rate a generator of its own, so drawing every field
 * leaves the game's rand sequence (AI, spin-outs) to the race logic. */
s32 RaceDrawRand(void);

/* ---- fast-lane helpers (identity at the original rate) ---- */

/* An add of `delta` per original tick, per field: half of it, the odd
 * part carried in *rem, so two
 * fields add exactly `delta`. */
static inline s32 RaceFieldHalf(s32 delta, s32 *rem) {
    s32 total;
    if (!RaceFieldRateLive)
        return delta;
    total = delta + *rem;
    *rem = total & 1;
    return total >> 1; /* floor: rem stays 0 or 1 */
}

/* The step this field of "x += tickStep" where tickStep closes part of
 * gap (gap >> 1, gap / d, ...), a follower of a moving target (the chase
 * camera). The game's own step for one tick gives the share r a tick
 * keeps, truncation and dead zones included (a gap / d that rounds to 0
 * keeps it all). A field keeps 2r / (1 + r): the same lag behind a target
 * moving steadily as a tick's (r / (1 - r) ticks); a still target is
 * closed a little faster than sqrt(r) would. At the original rate
 * tickStep. */
s32 RaceFieldApproach(s32 tickStep, s32 gap, s32 *rem);

/* ---- per-car sidecar ---- */

/* Fast-lane state of one car that RaceVehicle has no room for (the
 * chase camera's ease remainders). A replay file holds the sidecars whole:
 * one of another size (an older layout) is not restored. */
enum {
    CAR_FIELD_REM_CAMERA_PITCH, /* UpdateRaceChaseCamera */
    CAR_FIELD_REM_CAMERA_YAW,
    CAR_FIELD_REM_COUNT
};
typedef struct {
    s32 rem[CAR_FIELD_REM_COUNT];
} CarFieldState;

/* PlayerCars[0..1] are 0..1, ComCars[0..10] 2..12. */
#define CAR_FIELD_STATES 13
extern CarFieldState CarFieldStates[CAR_FIELD_STATES];
/* The car's sidecar (a dummy for a car outside both arrays). */
CarFieldState *CarFieldStateOf(const void *car);
/* A race at field rate and a car the player's model drives (PlayerCars,
 * flags1E4 & 0x10): its chase camera eases every field. */
int CarFieldFast(const void *car);
/* Cleared at race setup; SaveCarSnapshots / the replay's start copy the
 * cars' sidecars with the cars. */
void CarFieldStatesClear(void);
/* The cars' half way pose (StepRaceFrame, racemai2.c): keep != 0
 * puts the tick's end back (the race ends), 0 forgets it (the cars were
 * replaced: race setup, a replay's snapshot or file). */
void CarPoseSettle(int keep);
/* The car's distance along the course (trackNodeF0 << 10 plus
 * trackFraction170) where it is drawn this field: half way on the first
 * field of a tick, as its pose (racemai2.c), for what is placed by it. */
struct RaceVehicle;
s32 CarDrawnTrackDistance(const struct RaceVehicle *car);
/* The sidecars SaveCarSnapshots took (CarSnapshots' order), for the
 * replay files. */
extern CarFieldState CarFieldSnapshots[8];
void CarFieldStatesSave(void);
void CarFieldStatesRestore(void);

#endif
