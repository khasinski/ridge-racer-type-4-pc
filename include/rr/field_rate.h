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
 * per-tick units so thresholds, tables and the HUD stay valid: the
 * opponents' driving, every car's stages and contact, the chase camera,
 * course objects and draw-time effects. Its sub-unit remainders live in
 * the per-car sidecar (CarFieldState), since RaceVehicle keeps its PS1
 * size (0x320).
 *
 * The player cars (CarFieldFast) run the original's whole tick at the
 * original rate on the tick's first field and are drawn half way from
 * the last tick's end to this one's there (StepRaceFrame, racemai2.c);
 * their drive has no field-rate code.
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
/* Non-zero on the fields that complete an original tick's fast-lane steps
 * (the field after the tick's; always when the race is not at field rate):
 * a half step on each field of the pair gives the tick's state after the
 * second. A decision the original takes on its tick's state (a threshold
 * crossed by up to one tick's step) takes it on this field. */
static inline int RaceTickEnd(void) { return !RaceFieldRateLive || !RaceTickField; }
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

/* x after one field of a filter that keeps num/den of it per tick (a
 * field keeps sqrt(num/den)), with the rounding carried in *rem (Q16). */
s32 RaceFieldKeep(s32 x, u32 num, u32 den, s32 *rem);

/* An add of `delta` per original tick into a value that a filter keeping
 * num/den of it per tick multiplies right after the add (before != 0:
 * `x = (x + delta) * num / den`) or right before it (`x = x * num / den +
 * delta`), per field, with the filter at RaceFieldKeep: the add scaled by
 * sqrt(k)/(1+sqrt(k)) (before) or 1/(1+sqrt(k)) (after), so the filtered
 * value settles where the original's does, and a decaying value summed
 * into a position (`pos += v; v = v * num / den`) moves it as far by each
 * tick. The rounding is carried in *rem (Q16). */
s32 RaceFieldFilterAdd(s32 delta, u32 num, u32 den, int before, s32 *rem);

/* The step this field of "x += tickStep" where tickStep closes part of
 * gap (gap >> 1, gap / d, ...), a follower of a moving target (the chase
 * camera). The game's own step for one tick gives the share r a tick
 * keeps, truncation and dead zones included (a gap / d that rounds to 0
 * keeps it all). A field keeps 2r / (1 + r): the same lag behind a target
 * moving steadily as a tick's (r / (1 - r) ticks); a still target is
 * closed a little faster than sqrt(r) would. At the original rate
 * tickStep. */
s32 RaceFieldApproach(s32 tickStep, s32 gap, s32 *rem);

/* A value made from a one-tick difference, measured per field: the
 * difference over the last two fields (prev2 holds the value of two
 * fields ago, prev1 of one). At the original rate the plain difference
 * now - *prev1. Updates the history. */
s32 RaceFieldDelta(s32 now, s32 *prev1, s32 *prev2);

/* x after one field of a per-tick update x -> next that is affine in x
 * with slope `slopeQ16` (Q16: 0 = next does not depend on x, a snap;
 * 65536 = an add; 94/100 of it = a filter keeping 94%): x + (next - x) /
 * (1 + sqrt(slope)), so two fields make the tick's update (exactly for an
 * affine one). The rounding is carried in *remQ16. At the original rate
 * `next`. */
s32 RaceFieldStep(s32 x, s32 next, u32 slopeQ16, s32 *remQ16);

/* ---- per-car sidecar ---- */

/* Fast-lane state of one car that RaceVehicle has no room for. Slots are
 * named by the code that owns them. A replay file holds the sidecars
 * whole: one of another size (an older layout) is not restored. */
enum {
    CAR_FIELD_REM_POS_X,      /* IntegrateCarMotion */
    CAR_FIELD_REM_POS_Z,
    /* the car stages and the contact (StepRaceFrame) */
    CAR_FIELD_REM_STEER_ROLL,     /* UpdateCarSteering: steerRoll1DE */
    CAR_FIELD_REM_STEER_ROLL_ADD,
    CAR_FIELD_REM_STEER_PITCH,    /* steerPitch1DC */
    CAR_FIELD_REM_STEER_PITCH_ADD,
    CAR_FIELD_REM_WHEEL_OFFSET,   /* wheelOffsetE4[4] */
    CAR_FIELD_REM_WHEEL_OFFSET_ADD = CAR_FIELD_REM_WHEEL_OFFSET + 4,
    CAR_FIELD_REM_SUSPENSION = CAR_FIELD_REM_WHEEL_OFFSET_ADD + 4, /* wheelSuspensionDC[4] */
    CAR_FIELD_REM_SUSPENSION_ADD = CAR_FIELD_REM_SUSPENSION + 4,
    CAR_FIELD_REM_WHEEL_WOBBLE = CAR_FIELD_REM_SUSPENSION_ADD + 4, /* wheelOffsetE4[2..3] */
    CAR_FIELD_REM_SHAKE_PUSH_X = CAR_FIELD_REM_WHEEL_WOBBLE + 2, /* DecayCarOffset */
    CAR_FIELD_REM_SHAKE_PUSH_Z,
    CAR_FIELD_REM_SHAKE_X,
    CAR_FIELD_REM_SHAKE_Z,
    CAR_FIELD_REM_SHAKE_HALF,     /* shakeTimer19E: 1 = half a tick past */
    CAR_FIELD_REM_STATE_HALF,     /* stateTimer1A5 (UpdateCarShake) */
    CAR_FIELD_REM_CONTACT_HALF,   /* contactTimer1A7 */
    CAR_FIELD_WALL_WINDOW,        /* StepRaceFrame's contact loss: its window's first field + 1 */
    CAR_FIELD_REM_KERB_INPUT,     /* the kerb / off-road loss */
    CAR_FIELD_REM_WHEEL_SPIN,     /* ClampAiCarLateralMotion */
    /* the chase camera's ease (UpdateRaceChaseCamera) */
    CAR_FIELD_REM_CAMERA_PITCH,
    CAR_FIELD_REM_CAMERA_YAW,
    /* the opponents' driving (UpdateComCars, carai.c and under it) */
    CAR_FIELD_REM_AI_REVS,       /* pre-start revs, rpm +- slideDivisor5E */
    CAR_FIELD_REM_AI_HEADING,    /* heading1D4 += steering8C (Q16) */
    CAR_FIELD_REM_AI_STEER_Q16,  /* filtered90 = (filtered90 + u) / 2 */
    CAR_FIELD_REM_AI_SPEED,      /* speed4C += accel50 */
    CAR_FIELD_REM_AI_DRAG_Q16,   /* speed4C * 94/100, 99/100 */
    CAR_FIELD_REM_AI_RPM_Q16,    /* UpdateEngineRpm */
    CAR_FIELD_REM_AI_LATERAL_Q16,/* corner turn mode 3, lateral24 3/4 */
    CAR_FIELD_REM_AI_ROT_Q16,    /* corner turn mode 3, rot54 to heading */
    CAR_FIELD_REM_AI_IMPACT_FIELD,/* RaceFieldCount at the last impact */
    CAR_FIELD_REM_COUNT
};
enum {
    CAR_FIELD_HIST_INPUT1,        /* UpdateCarSteering: input, a field ago */
    CAR_FIELD_HIST_INPUT2,        /* two fields ago */
    CAR_FIELD_HIST_AI_VEL_X,      /* IntegrateCarMotion: an opponent's */
    CAR_FIELD_HIST_AI_VEL_Z,      /* vel30/38 after their << 8 */
    CAR_FIELD_HIST_AI_ERROR1,     /* UpdateComCarDrive's error92, 1 field ago */
    CAR_FIELD_HIST_AI_ERROR2,     /* 2 fields ago */
    CAR_FIELD_HIST_AI_HEADING,    /* heading1D4 before the last field's step */
    CAR_FIELD_HIST_AI_INPUT,      /* input before the last field's step */
    CAR_FIELD_HIST_COUNT
};
/* UpdateWheelBumps' pitch / roll samples, one a field: the 10-tick moving
 * averages take 20 fields, the delayed one reaches 27 fields back. */
#define CAR_FIELD_BODY_RING 32
typedef struct {
    s32 rem[CAR_FIELD_REM_COUNT];
    s32 hist[CAR_FIELD_HIST_COUNT]; /* RaceFieldDelta histories */
    s16 bodyPitch[CAR_FIELD_BODY_RING], bodyRoll[CAR_FIELD_BODY_RING];
    s32 bodyRingPos;
} CarFieldState;

/* PlayerCars[0..1] are 0..1, ComCars[0..10] 2..12. */
#define CAR_FIELD_STATES 13
extern CarFieldState CarFieldStates[CAR_FIELD_STATES];
/* The car's sidecar (a dummy for a car outside both arrays). */
CarFieldState *CarFieldStateOf(const void *car);
/* A race at field rate and a car the player's model drives (PlayerCars,
 * flags1E4 & 0x10): its tick runs whole on the tick's first field and is
 * drawn half way there (StepRaceFrame); its chase camera eases every
 * field. The other cars step every field. */
int CarFieldFast(const void *car);
/* Cleared at race setup; SaveCarSnapshots / the replay's start copy the
 * cars' sidecars with the cars. */
void CarFieldStatesClear(void);
/* A player car's half way pose (StepRaceFrame, racemai2.c): keep != 0
 * puts the tick's end back (the race ends), 0 forgets it (the cars were
 * replaced: race setup, a replay's snapshot or file). */
void CarPoseSettle(int keep);
/* The sidecars SaveCarSnapshots took (CarSnapshots' order), for the
 * replay files. */
extern CarFieldState CarFieldSnapshots[8];
void CarFieldStatesSave(void);
void CarFieldStatesRestore(void);

#endif
