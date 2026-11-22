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
 * voices, lap counting, HUD and sound timers, the code not yet converted)
 * runs once per original tick, on the fields where RaceSlowTick() is
 * non-zero (every second field). Pad edges (pressed, repeat, released, the
 * action words) are latched over the field between: the slow lane sees
 * every press once, on its tick; nothing sees an edge on the other field.
 *
 * Fast lane: code converted to run every field with half the original
 * step (RaceFieldHalf, RaceFieldKeep), its state kept in the original
 * per-tick units so thresholds, tables and the HUD stay valid. Its
 * sub-unit remainders live in the per-car sidecar (CarFieldState), since
 * RaceVehicle keeps its PS1 size (0x320).
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

/* Race setup (SetupRaceScene): the rate for this race by its game mode
 * (GameModeType 4, the link, and 5, the attract demo, stay at the original
 * rate). Sets VSyncThreshold. */
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
/* The VSyncThreshold of a race frame: one field (0x81) at field rate,
 * else 0x180. Code that tests VSyncThreshold == 0x80 for "one field a
 * frame" is menu code; race code uses RaceFieldRateLive. */
s32 RaceFieldRateThreshold(void);

/* ---- fast-lane helpers (identity at the original rate) ---- */

/* An add of `delta` per original tick, per field: half of it, the odd
 * part carried in *rem (one of the car's CarFieldState rem[] slots), so two
 * fields add exactly `delta`. */
static inline s32 RaceFieldHalf(s32 delta, s32 *rem) {
    s32 total;
    if (!RaceFieldRateLive)
        return delta;
    total = delta + *rem;
    *rem = total & 1;
    return total >> 1; /* floor: rem stays 0 or 1 */
}

/* A share `keep` (Q16, 65536 = all) of a value left after one original
 * tick, per field: sqrt(keep). RaceFieldKeepQ16(num, den) gives it for
 * the game's `x * num / den` filters. */
u32 RaceFieldKeepQ16(u32 num, u32 den);
/* x after one field of a filter that keeps num/den of it per tick, with
 * the rounding carried in *rem (Q16). */
s32 RaceFieldKeep(s32 x, u32 num, u32 den, s32 *rem);

/* A value made from a one-tick difference, measured per field: the
 * difference over the last two fields (prev2 holds the value of two
 * fields ago, prev1 of one). At the original rate the plain difference
 * now - *prev1. Updates the history. */
s32 RaceFieldDelta(s32 now, s32 *prev1, s32 *prev2);

/* ---- per-car sidecar ---- */

/* Fast-lane state of one car that RaceVehicle has no room for. Slots are
 * named by the code that owns them; add an enum entry, never reuse one. */
enum {
    CAR_FIELD_REM_POS_X,      /* IntegrateCarMotion */
    CAR_FIELD_REM_POS_Z,
    CAR_FIELD_REM_SLOTS = 48
};
typedef struct {
    s32 rem[CAR_FIELD_REM_SLOTS];
    s32 hist[16];             /* RaceFieldDelta histories */
} CarFieldState;

/* PlayerCars[0..1] are 0..1, ComCars[0..10] 2..12. */
#define CAR_FIELD_STATES 13
extern CarFieldState CarFieldStates[CAR_FIELD_STATES];
/* The car's sidecar (a dummy for a car outside both arrays). */
CarFieldState *CarFieldStateOf(const void *car);
/* Cleared at race setup; SaveCarSnapshots / the replay's start copy the
 * cars' sidecars with the cars. */
void CarFieldStatesClear(void);
void CarFieldStatesSave(void);
void CarFieldStatesRestore(void);

#endif
