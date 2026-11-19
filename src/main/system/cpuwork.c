/* The race frame's PS1 CPU time by the work its routines do (rr/cpu_work.h). */
#include <stdint.h>


#include "rr/cpu_work.h"
#include "rr/race_setup.h"
#include "rr/vehicle_table.h"

/* PsyZ (psyz/gte.h, which libgte.h comes with) */
unsigned long long Psyz_GteOpCount(unsigned op);

unsigned long long RrWorkCount[RR_WORK_UNITS];
int RrWorkFrame;

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
    RrWorkFrame = 1;
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
 * call (DSF_MARKS name:ret, interrupts' time taken out with DSF_EXC; Time
 * Attack's race 2971-4025, Versus's 3619-4773, the Grand Prix's first
 * 7843-8896: 527-577 frames each) fitted to the port's counts of those
 * frames (RR4_MARK with RR4_MARK_WORK in an RR4_FUNC_MARKS build). Per
 * unit in 1/64 clocks; the comments give the fit's mean, the median and
 * 90th percentile of its residual in clocks. [mark] */
#define W(call, bytes, rtpt, rtps, nclip, dpcs, intpl, mvmva)                  \
    {{(call), (bytes), (rtpt), (rtps), (nclip), (dpcs), (intpl), (mvmva), 0, 0, 0, 0}}
const RrWorkCost RrCostReadCarInput = W(13470, 0, 0, 0, 0, 0, 0, 0);       /* 280; 56, 171 */
const RrWorkCost RrCostUpdatePlayerCar = W(166330, 0, 0, 0, 0, 0, 0, 0);   /* 3261; 523, 2422 */
const RrWorkCost RrCostUpdateComCars = W(444142, 0, 0, 0, 0, 0, 0, 518606); /* 33917; 3328, 8040 */
/* ClampAiCarLateralMotion, UpdateCarSteering, DecayCarOffset,
 * IntegrateCarMotion, UpdateCarTrackNode, ComputeWheelOffsets,
 * ConstrainCarToTrack: a car each */
const RrWorkCost RrCostCarStages[7] = {
    W(3255, 0, 0, 0, 0, 0, 0, 0),        /* 186; 8, 31 */
    W(62460, 0, 0, 0, 0, 0, 0, 0),       /* 3850; 36, 96 */
    W(1531, 0, 0, 0, 0, 0, 0, 0),        /* 91; 4, 19 */
    W(0, 0, 0, 0, 0, 0, 0, 30134),       /* 5394; 201, 819 */
    W(32, 0, 0, 0, 10652, 0, 0, 0),      /* 1593; 18, 147 */
    W(0, 0, 0, 0, 0, 0, 0, 8771),        /* 2142; 60, 318 */
    W(54197, 0, 0, 0, 0, 0, 0, 21277),   /* 4442; 139, 580 */
};
const RrWorkCost RrCostUpdateTrackObjects = W(254267, 0, 0, 0, 4685, 0, 0, 0); /* 6038; 728, 1847 */
const RrWorkCost RrCostChaseCamera = W(0, 0, 0, 0, 0, 0, 0, 22575);   /* UpdateRaceChaseCamera 1468; 78, 164 */
const RrWorkCost RrCostCameraMatrices = W(0, 0, 0, 0, 0, 0, 0, 5672); /* BuildCameraMatrices 3166; 140, 322 */
/* DrawPlayerCar (Time Attack's 5.0k short, Versus's 2.8k long) and
 * DrawOpponentCar (the replays' cars too: their frames 4027-4405,
 * 4775-4971 and 8898-9096 in the fit), and DrawCars' own (339-986 for 1-8
 * cars: 340 a call) */
const RrWorkCost RrCostDrawPlayerCar = W(0, 0, 307, 9399, 2241, 0, 0, 3228);          /* 87514; 2433, 5580 */
const RrWorkCost RrCostDrawOpponentCar = W(24881, 29, 12742, 0, 0, 10163, 12703, 3168); /* 40527; 678, 1807 */
const RrWorkCost RrCostDrawCars = W(21760, 0, 0, 0, 0, 0, 0, 0);
/* SetupPlayerView's (and the other course views') parts besides
 * SubmitCourseHorizon (submodel.c): BuildViewSegmentObjectList,
 * ApplySegmentVisibilityMask and the rest, 2620 a call */
const RrWorkCost RrCostViewSegmentList = W(31689, 0, 0, 0, 8840, 0, 0, 0);    /* 784; 41, 122 */
const RrWorkCost RrCostSegmentVisibility = W(11791, 0, 0, 0, 8376, 0, 0, 0);  /* 357; 15, 59 */
const RrWorkCost RrCostCourseView = W(167680, 0, 0, 0, 0, 0, 0, 0);
const RrWorkCost RrCostDrawTrackObjects = W(151930, 104, 7592, 2989, 975, 0, 0, 11009); /* 6744; 104, 224 */
const RrWorkCost RrCostDrawTrackScenery = W(28, 136, 7018, 0, 7470, 0, 12041, 0);       /* 43325; 151, 316 */
#undef W

/* StepRaceFrame less the calls above: 5264 with one car (Time Attack),
 * 13464 with two (Versus), 55622 with eight (the Grand Prix), medians:
 * 7060 a car less 500. */
void RrSpendStepRaceFrameOwn(void) {
    int cars = 0;
    for (int i = 0; i < NumCars; i++)
        cars += CarTable[i]->half1E8 != -1;
    if (cars > 0)
        RR_CPU_CYCLES((unsigned)(7060 * cars - 500));
}

/* The race modes' frame functions less the routines above, by RaceState
 * (0 the start, 1 the countdown, 2-3 racing, from 4 the finish); the
 * Grand Prix's start includes its grid's fly-by. Medians of 75-325 frames
 * (the racing frames' 10th to 90th percentiles 10.6k-14.7k in Time
 * Attack, 15.6k-23.3k in Versus, 11.7k-14.7k in the Grand Prix). */
void RrSpendRaceModeOwn(int mode) {
    static const unsigned own[3][4] = {
        {3557, 14643, 12624, 1065},
        {6320, 23282, 17299, 2335},
        {82337, 15211, 11908, 1645},
    };
    int state = RaceState < 0 ? 0 : RaceState >= 4 ? 3 : RaceState == 3 ? 2 : RaceState;
    if (mode >= 0 && mode < 3)
        RR_CPU_CYCLES(own[mode][state]);
}
