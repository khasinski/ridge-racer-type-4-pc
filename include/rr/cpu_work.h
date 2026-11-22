#ifndef RR_CPU_WORK_H
#define RR_CPU_WORK_H

#include "rr/cpu_time.h"
#include "rr/layout.h"
#include "rr/gpu_scratch_types.h"

/* PS1 CPU time of the race frame's routines (logic, physics, AI, camera,
 * the course's and the cars' GTE geometry, the ordering table), modelled by
 * the work each call does: a routine begins a count (RrWorkBegin) and
 * spends, at its end, its own clocks for the call and so many for each unit
 * of work it did meanwhile (RrWorkSpend): the GTE commands of the kinds
 * below (PsyZ's Psyz_GteOpCount), the packet bytes it added and, in the
 * course renderer, the tiles it tested for facing, those facing the view,
 * the grids it subdivided and their cells. The costs (rr/cpu_time.h
 * RR_WORK_*) are fitted with non-negative least squares to DuckStation's
 * entry-to-return clocks of each routine, its interrupts' time taken out
 * (tools/port/ds_frames.py DSF_MARKS name:ret and DSF_EXC; Time Attack's,
 * Versus's and the Grand Prix's four races), against the port's counts of
 * the same frames (an RR4_FUNC_MARKS build).
 *
 * Work the PS1 does not do, the host's widescreen and draw distance views
 * past the original list and frame interpolation's second grid, is held
 * out of the counts (RrWorkHoldBegin/End), so the emulated time does not
 * depend on those options. */
enum {
    RR_WORK_CALL,   /* the call itself */
    RR_WORK_BYTES,  /* packet bytes added */
    RR_WORK_RTPT,
    RR_WORK_RTPS,
    RR_WORK_NCLIP,
    RR_WORK_DPCS,
    RR_WORK_INTPL,
    RR_WORK_MVMVA,
    RR_WORK_TESTED, /* course tiles tested for facing */
    RR_WORK_FACED,  /* ... and facing the view */
    RR_WORK_GRIDS,  /* tiles drawn as a subdivided grid */
    RR_WORK_CELLS,  /* the grids' cells */
    RR_WORK_CURVE_STEPS, /* the AI's track nodes looked at ahead (ScanCurveAhead) */
    RR_WORK_COLLIDE_CARS, /* the other cars ResolveCarCollisions looked at */
    RR_WORK_WHEEL_AXLES,  /* FindWheelTrackNodes: an axle's node set up */
    RR_WORK_WHEEL_CURVED, /* ... a wheel on a curved node */
    RR_WORK_WHEEL_SHOULDER, /* ... a wheel past the road's edge */
    RR_WORK_WHEEL_SHOULDER_PAST, /* ... and past the shoulder */
    RR_WORK_BUMP_NODES,   /* UpdateWheelBumps: a wheel on another node */
    RR_WORK_OBJECTS,      /* UpdateTrackObjects: the course objects it looked at */
    RR_WORK_OBJECTS_ACTIVE, /* ... the active ones */
    RR_WORK_OBJECT_KEYS,  /* ... their keyframe interpolations */
    RR_WORK_CAR_NEAR,     /* an opponent car drawn by its near model's parts */
    RR_WORK_CAR_MID,      /* ... its middle model */
    RR_WORK_CAR_FAR,      /* ... its far model */
    RR_WORK_CAR_TRAILS,   /* DrawWheelTrailEffects with the car's points */
    RR_WORK_CAR_TRAILS_OFF, /* ... without (the car not drawn) */
    RR_WORK_CAR_LIGHTS,   /* ... its lights on (the night courses) */
    RR_WORK_CAR_BRAKE_LIGHTS, /* ... its brake lights */
    RR_WORK_SQR,          /* the GTE's SQR and CC (the cars' trails and lights) */
    RR_WORK_CC,
    RR_WORK_VIEWS,        /* a course pass's turns over the visible views */
    RR_WORK_EMIT_ITEMS,   /* EmitSemiTransG3List/G4List: the list's entries */
    RR_WORK_NCC,          /* the GTE's NCCS and NCCT (the player's and the menus' car) */
    RR_WORK_UNITS
};

/* 1/64 clocks per unit of each kind (RR_WORK_CALL: the call's own). */
typedef struct {
    u32 per[RR_WORK_UNITS];
} RrWorkCost;

typedef struct {
    unsigned long long unit[RR_WORK_UNITS];
    const void *cursor;
    unsigned long long heldBytes;
} RrWork;

/* The course renderer's own counts (RR_WORK_TESTED .. RR_WORK_CELLS) and
 * the AI's (RR_WORK_CURVE_STEPS). */
extern unsigned long long RrWorkCount[RR_WORK_UNITS];
void RrWorkNet(unsigned long long out[RR_WORK_UNITS], unsigned long long *heldBytes);

void RrWorkBegin(RrWork *work, const void *cursor);
/* The PS1 time of the work since RrWorkBegin; cursor: the packet cursor
 * now (the one RrWorkBegin was given, moved on). A null work spends the
 * cost's RR_WORK_CALL only. */
void RrWorkSpend(const RrWork *work, const void *cursor, const RrWorkCost *cost);
/* The work since RrWorkBegin (or the previous part) without the call's own,
 * spent now: a routine that charges by object spends each object's time
 * before the next one, so what an object does (a rand call, a sound
 * register write) comes where its time puts it on the PS1, not at the
 * routine's start. */
void RrWorkSpendPart(RrWork *work, const void *cursor, const RrWorkCost *cost);
/* Host-only work between them is not counted (nested holds count once). */
void RrWorkHoldBegin(const void *cursor);
void RrWorkHoldEnd(const void *cursor);

/* The routines being counted (RR_WORK_CALL, RR_WORK_DRAW): a rand call
 * (PsyqRand) charges the innermost one's work so far first, so the call
 * comes after the time of the work before it as on the PS1, not at the
 * routine's start with all of its time at its end. */
void RrWorkPush(RrWork *work, int packets, const RrWorkCost *cost);
void RrWorkPop(void);
void RrWorkFlush(void);
/* A count begun by a caller and charged at the callee's end (before its
 * return), for routines whose return comes right before other events:
 * RrWorkArmEnd, then the callee's RrWorkEnd. */
void RrWorkArmEnd(RrWork *work, const RrWorkCost *cost);
void RrWorkEnd(const void *cursor);

/* A call and its PS1 time by the work it did (no packets counted). */
#define RR_WORK_CALL(call, cost)                                               \
    do {                                                                       \
        RrWork rrWork_;                                                        \
        RrWorkBegin(&rrWork_, 0);                                              \
        RrWorkPush(&rrWork_, 0, (cost));                                       \
        call;                                                                  \
        RrWorkPop();                                                           \
        RrWorkSpend(&rrWork_, 0, (cost));                                      \
    } while (0)

/* The same counting the packets it adds at the frame's packet cursor. */
#define RR_WORK_DRAW(call, cost)                                               \
    do {                                                                       \
        RrWork rrWork_;                                                        \
        RrWorkBegin(&rrWork_, RR_GPU_SCRATCH->cursor);                         \
        RrWorkPush(&rrWork_, 1, (cost));                                       \
        call;                                                                  \
        RrWorkPop();                                                           \
        RrWorkSpend(&rrWork_, RR_GPU_SCRATCH->cursor, (cost));                 \
    } while (0)

/* Non-zero once StepRaceFrame has run in this frame (a race, a replay, the
 * attract demo; the frame loop clears it): the race modes' own costs
 * (RrSpendRaceModeOwn, fitted to DuckStation's clocks) include the HUD's
 * primitives and the models drawn outside the routines above, so the
 * primitive setters' own charges (AddSpritePrim, AddDrawAreaPrim,
 * AddDrawModePrim: the menus') and SubmitModel's are held then. The menus
 * that use the race frame's routines (the car selects' BuildCameraMatrices)
 * are not race frames: RrWorkBegin set it before, which held their
 * primitives' time. */
extern int RrWorkFrame;
/* The routines of the model running (RrWorkBegin to RrWorkSpend). */
extern int RrWorkDepth;

/* Diagnostics (RR4_FUNC_MARKS builds, RR4_MARK_WORK): called before each
 * course pass with its number and after it with number | 0x100, the
 * packet cursor then. */
extern void (*RrWorkPassHook)(int pass, const void *cursor);

/* The race frame's routines (cpuwork.c has the fits). */
extern const RrWorkCost RrCostReadCarInput;
/* by the race's state (cpuwork.c) */
const RrWorkCost *RrCostUpdatePlayerCar(void);
const RrWorkCost *RrCostUpdateComCars(void);
extern const RrWorkCost RrCostCarStages[11];
extern const RrWorkCost RrCostUpdateTrackObjects, RrCostChaseCamera, RrCostCameraMatrices;
extern const RrWorkCost RrCostDrawPlayerCar, RrCostDrawOpponentCar, RrCostDrawCars;
extern const RrWorkCost RrCostViewSegmentList, RrCostSegmentVisibility, RrCostCourseView;
extern const RrWorkCost RrCostDrawTrackObjects, RrCostDrawTrackScenery, RrCostSubmitModel;
extern const RrWorkCost RrCostSubmitModelTwoTri, RrCostMeshPrimList;
/* the course objects' EmitSemiTransG3List and EmitSemiTransG4List */
extern const RrWorkCost RrCostEmitSemiTransG3, RrCostEmitSemiTransG4;
/* ... and their models: DrawTrackObjectLitModel, DrawTrackObjectBillboard,
 * DrawTrackObjectFlashQuads, SubmitCourseModel2, DrawAnimatedTrackObject,
 * DrawTrackObjectModel */
extern const RrWorkCost RrCostTrackObjectLit, RrCostTrackObjectBillboard, RrCostTrackObjectFlashQuads;
extern const RrWorkCost RrCostTrackObjectCourseModel, RrCostTrackObjectAnimated, RrCostTrackObjectModel;
/* StepRaceFrame's own work besides its calls: a stage loop's end, the
 * impact loop by what it did, the rest at its end. */
void RrSpendStepRaceFrameLoop(void);
void RrSpendStepRaceFrameImpacts(int cars, int impacts, int contactCars, int contacts);
void RrSpendStepRaceFrameOwn(int comCars);
/* A race mode's own work besides its routines (cpuwork.c): mode 0 Time
 * Attack, 1 Versus, 2 the Grand Prix. */
void RrSpendRaceModeOwn(int mode);
/* A replay frame's own (ReplayModeUpdate) and its node camera's
 * (UpdateRaceCamera besides the calls charged themselves), after the
 * camera's call. */
void RrSpendReplayFrame(void);
/* ... the node camera's, in UpdateRaceCamera before its BuildCameraMatrices
 * (DuckStation's: the camera's 3.2k there, not after the call) */
void RrSpendReplayCamera(void);

#endif
