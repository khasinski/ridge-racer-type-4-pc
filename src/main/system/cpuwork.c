/* The race frame's PS1 CPU time by the work its routines do (rr/cpu_work.h). */
#include <stdint.h>


#include "rr/cpu_work.h"
#include "rr/race_setup.h"
#include "rr/vehicle_table.h"
#include "rr/player_cars.h"

/* PsyZ (psyz/gte.h, which libgte.h comes with) */
unsigned long long Psyz_GteOpCount(unsigned op);

unsigned long long RrWorkCount[RR_WORK_UNITS];
void (*RrWorkPassHook)(int pass, const void *cursor);
int RrWorkFrame;
int RrWorkDepth;

/* GTE opcodes of the counted kinds */
static const unsigned char WorkOpcodes[RR_WORK_UNITS] = {
    [RR_WORK_RTPT] = 0x30, [RR_WORK_RTPS] = 0x01, [RR_WORK_NCLIP] = 0x06,
    [RR_WORK_DPCS] = 0x10, [RR_WORK_INTPL] = 0x11, [RR_WORK_MVMVA] = 0x12};

static int HoldDepth;
static unsigned long long Held[RR_WORK_UNITS], HoldStart[RR_WORK_UNITS];
static unsigned long long HeldBytes;
static const void *HoldCursor;

static void WorkRaw(unsigned long long raw[RR_WORK_UNITS]) {
    for (int k = RR_WORK_RTPT; k <= RR_WORK_MVMVA; k++)
        raw[k] = Psyz_GteOpCount(WorkOpcodes[k]);
    for (int k = RR_WORK_TESTED; k < RR_WORK_UNITS; k++)
        raw[k] = RrWorkCount[k];
    raw[RR_WORK_CALL] = raw[RR_WORK_BYTES] = 0;
}

void RrWorkBegin(RrWork *work, const void *cursor) {
    unsigned long long raw[RR_WORK_UNITS];
    RrWorkDepth++;
    WorkRaw(raw);
    for (int k = 0; k < RR_WORK_UNITS; k++)
        work->unit[k] = raw[k] - Held[k];
    work->cursor = cursor;
    work->heldBytes = HeldBytes;
}

void RrWorkSpend(const RrWork *work, const void *cursor, const RrWorkCost *cost) {
    unsigned long long raw[RR_WORK_UNITS], total = cost->per[RR_WORK_CALL];
    if (!work) { /* the call's own only */
        RR_CPU_CYCLES((unsigned)(total / 64));
        return;
    }
    if (RrWorkDepth > 0) RrWorkDepth--;
    WorkRaw(raw);
    for (int k = RR_WORK_RTPT; k < RR_WORK_UNITS; k++)
        total += (raw[k] - Held[k] - work->unit[k]) * cost->per[k];
    if (cursor && work->cursor && cursor > work->cursor) {
        unsigned long long bytes = (unsigned long long)((const unsigned char *)cursor -
                                                        (const unsigned char *)work->cursor);
        unsigned long long held = HeldBytes - work->heldBytes;
        total += (bytes > held ? bytes - held : 0) * cost->per[RR_WORK_BYTES];
    }
    RR_CPU_CYCLES((unsigned)(total / 64));
}

void RrWorkHoldBegin(const void *cursor) {
    if (HoldDepth++ != 0)
        return;
    WorkRaw(HoldStart);
    HoldCursor = cursor;
}

void RrWorkHoldEnd(const void *cursor) {
    unsigned long long raw[RR_WORK_UNITS];
    if (HoldDepth <= 0 || --HoldDepth != 0)
        return;
    WorkRaw(raw);
    for (int k = 0; k < RR_WORK_UNITS; k++)
        Held[k] += raw[k] - HoldStart[k];
    if (cursor && HoldCursor && cursor > HoldCursor)
        HeldBytes += (unsigned long long)((const unsigned char *)cursor -
                                          (const unsigned char *)HoldCursor);
}

/* The race frame's routines: DuckStation's entry-to-return clocks of each
 * call, its interrupts' time taken out (DSF_MARKS name:ret, DSF_EXC) and the
 * CPU's stalls for the list DMA, which the port models apart (PsyZ
 * gpu_timing.c; PSYZ_STALL_TRACE in the same call of the same frame: 21k of
 * the Grand Prix's StepRaceFrame and 28k of Time Attack's course view, which
 * the earlier fit took as work), fitted to the port's counts of those frames
 * (RR4_MARK with RR4_MARK_WORK in an RR4_FUNC_MARKS build) over Time
 * Attack's race on courses 0 and 7, Versus's and the Grand Prix's four races
 * (4715 frames). Per unit in 1/64 clocks; the comments give the fit's mean,
 * the median and 90th percentile of its residual in clocks. [mark] */
#define W(call, bytes, rtpt, rtps, nclip, dpcs, intpl, mvmva)                  \
    {{(call), (bytes), (rtpt), (rtps), (nclip), (dpcs), (intpl), (mvmva), 0, 0, 0, 0}}
const RrWorkCost RrCostReadCarInput = W(12955, 0, 0, 0, 0, 0, 0, 0);       /* 225; 57, 163 */
const RrWorkCost RrCostUpdatePlayerCar = W(159747, 0, 0, 0, 0, 0, 0, 0);   /* 2705; 614, 2319 */
const RrWorkCost RrCostUpdateComCars = W(437660, 0, 0, 0, 0, 0, 0, 532731); /* 35772; 3266, 9908 */
/* ClampAiCarLateralMotion, UpdateCarSteering, DecayCarOffset,
 * IntegrateCarMotion, UpdateCarTrackNode, ComputeWheelOffsets,
 * ConstrainCarToTrack: a car each */
const RrWorkCost RrCostCarStages[7] = {
    W(3299, 0, 0, 0, 0, 0, 0, 0),        /* 230; 9, 36 */
    W(70214, 0, 0, 0, 0, 0, 0, 0),       /* 4722; 105, 333 */
    W(1559, 0, 0, 0, 0, 0, 0, 0),        /* 111; 5, 20 */
    W(9334, 0, 0, 0, 0, 0, 0, 28003),    /* 6414; 171, 550 */
    W(1063, 0, 0, 0, 10030, 0, 0, 0),    /* 1972; 15, 134 */
    W(0, 0, 0, 0, 0, 0, 0, 8607),        /* 2543; 43, 317 */
    W(75292, 0, 0, 0, 0, 0, 0, 0),       /* 5243; 118, 504 */
};
const RrWorkCost RrCostUpdateTrackObjects = W(232214, 0, 0, 0, 7507, 0, 0, 0); /* 4877; 954, 1927 */
const RrWorkCost RrCostChaseCamera = W(0, 0, 0, 0, 0, 0, 0, 20413);   /* UpdateRaceChaseCamera 1070; 24, 115 */
const RrWorkCost RrCostCameraMatrices = W(0, 0, 0, 0, 0, 0, 0, 5252); /* BuildCameraMatrices 2510; 45, 218 */
/* DrawPlayerCar and DrawOpponentCar (the replays' cars too), and DrawCars'
 * own (339-919 for 1-8 cars: 340 a call) */
const RrWorkCost RrCostDrawPlayerCar = W(0, 104, 7420, 1665, 1478, 34955, 0, 1163);       /* 68405; 814, 2662 */
const RrWorkCost RrCostDrawOpponentCar = W(36201, 156, 12292, 0, 0, 27437, 0, 3);        /* 39076; 487, 1704 */
const RrWorkCost RrCostDrawCars = W(21760, 0, 0, 0, 0, 0, 0, 0);
/* SetupPlayerView's (and the other course views') parts besides
 * SubmitCourseHorizon (submodel.c): BuildViewSegmentObjectList,
 * ApplySegmentVisibilityMask and the rest, 2600 a call */
const RrWorkCost RrCostViewSegmentList = W(32115, 0, 0, 0, 8546, 0, 0, 0);    /* 737; 48, 117 */
const RrWorkCost RrCostSegmentVisibility = W(11806, 0, 0, 0, 8619, 0, 0, 0);  /* 365; 15, 58 */
const RrWorkCost RrCostCourseView = W(166400, 0, 0, 0, 0, 0, 0, 0);
const RrWorkCost RrCostDrawTrackObjects = W(109112, 95, 1113, 0, 5783, 11882, 0, 15362); /* 10573; 593, 1064 */
const RrWorkCost RrCostDrawTrackScenery = W(51155, 140, 6782, 897, 7749, 897, 11231, 0);  /* 35007; 128, 413 */
/* SubmitModel where nothing above includes it (the menus' models: the car
 * selects' cars, 49k clocks a frame in the Grand Prix's), from its calls in
 * the race frames above (2981 frames) */
const RrWorkCost RrCostSubmitModel = W(6090, 223, 8037, 1700, 0, 0, 0, 1685); /* 28987; 184, 978 */
#undef W

/* StepRaceFrame less the calls above, without the list DMA's stalls: 5016
 * with one car (Time Attack), 6126 with two (Versus), 37.3k-39.1k with
 * eight, seven of them the AI's (the Grand Prix: their track projections
 * and the collisions among them), medians: 3906, 1110 a car and 3602 an AI
 * car the frame drives. */
void RrSpendStepRaceFrameOwn(int comCars) {
    int cars = 0, ai = 0;
    for (int i = 0; i < NumCars; i++)
        cars += CarTable[i]->half1E8 != -1;
    if (comCars)
        for (int i = 0; i < 11; i++)
            ai += (short)ComCars[i].flags1E4 > 0;
    if (cars > 0)
        RR_CPU_CYCLES((unsigned)(3906 + 1110 * cars + 3602 * ai));
}

/* The race modes' frame functions less the routines above and the stalls,
 * by RaceState (0 the start, 1 the countdown, 2-3 racing, from 4 the
 * finish); the Grand Prix's start includes its grid's fly-by. Medians of
 * 75-786 frames (Time Attack's on course 7 3.3k longer racing). */
void RrSpendRaceModeOwn(int mode) {
    static const unsigned own[3][4] = {
        {3537, 14403, 12447, 1065},
        {63, 20165, 16189, 2271},
        {75000, 15000, 12500, 1600},
    };
    int state = RaceState < 0 ? 0 : RaceState >= 4 ? 3 : RaceState == 3 ? 2 : RaceState;
    if (mode >= 0 && mode < 3)
        RR_CPU_CYCLES(own[mode][state]);
}
