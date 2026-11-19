/* The race frame's PS1 CPU time by the work its routines do (rr/cpu_work.h). */
#include <stdint.h>


#include "rr/cpu_work.h"
#include "rr/race_setup.h"
#include "rr/vehicle_table.h"
#include "rr/player_cars.h"
#include "game/data/menuvars.h"
#include "rr/scene_camera_state.h"

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
 * call, its interrupts' time taken out (DSF_MARKS name:ret, DSF_EXC) and
 * the CPU's stalls for the list DMA in that call (DuckStation's own: the
 * regtest's DUCKSTATION_DMA_TRACE slices of the GPU channel outside the
 * exceptions; the port models them apart, PsyZ gpu_timing.c), fitted to
 * the port's counts of those frames (RR4_MARK with RR4_MARK_WORK in an
 * RR4_FUNC_MARKS build) over Time Attack's race on courses 0 and 7,
 * Versus's and the Grand Prix's four races (4715 frames). The port's own
 * stalls stood in for DuckStation's before: their total in a frame is the
 * same (30.8k clocks in the Grand Prix's first race), but 4.9k of it comes
 * inside DuckStation's exceptions there, which the fit took off the
 * routines' time, and the port puts a routine's stalls where it charges
 * its time, not in the call. Per unit in 1/64 clocks; the comments give
 * the fit's mean, the median and 90th percentile of its residual in
 * clocks. [mark] */
#define W(call, bytes, rtpt, rtps, nclip, dpcs, intpl, mvmva)                  \
    {{(call), (bytes), (rtpt), (rtps), (nclip), (dpcs), (intpl), (mvmva), 0, 0, 0, 0}}
const RrWorkCost RrCostReadCarInput = W(12622, 0, 0, 0, 0, 0, 0, 0);       /* 218; 42, 158 */
const RrWorkCost RrCostUpdatePlayerCar = W(156324, 0, 0, 0, 0, 0, 0, 0);   /* 2637; 608, 2266 */
const RrWorkCost RrCostUpdateComCars = W(426793, 0, 0, 0, 0, 0, 0, 438764); /* 30520; 3137, 8654 */
/* ClampAiCarLateralMotion, UpdateCarSteering, DecayCarOffset,
 * IntegrateCarMotion, UpdateCarTrackNode, ComputeWheelOffsets,
 * ConstrainCarToTrack: a car each */
const RrWorkCost RrCostCarStages[7] = {
    W(3299, 0, 0, 0, 0, 0, 0, 0),        /* 230; 9, 36 */
    W(61719, 0, 0, 0, 0, 0, 0, 0),       /* 4259; 34, 91 */
    W(1559, 0, 0, 0, 0, 0, 0, 0),        /* 111; 5, 20 */
    W(0, 0, 0, 0, 0, 0, 0, 27488),       /* 5669; 5, 19 */
    W(3296, 0, 0, 0, 8432, 0, 0, 0),     /* 1721; 0, 4 */
    W(1870, 0, 0, 0, 0, 0, 0, 7480),     /* 2185; 3, 14 */
    W(59759, 0, 0, 0, 0, 0, 0, 7422),    /* 4618; 12, 50 */
};
const RrWorkCost RrCostUpdateTrackObjects = W(233755, 0, 0, 0, 7280, 0, 0, 0); /* 4973; 763, 1659 */
const RrWorkCost RrCostChaseCamera = W(6130, 0, 0, 0, 0, 0, 0, 18389); /* UpdateRaceChaseCamera 1094; 1, 2 */
const RrWorkCost RrCostCameraMatrices = W(0, 0, 0, 0, 0, 0, 0, 5225);  /* BuildCameraMatrices 2571; 2, 9 */
/* DrawPlayerCar and DrawOpponentCar (the replays' cars too), and DrawCars'
 * own (339-987 for 1-8 cars: 340 a call) */
const RrWorkCost RrCostDrawPlayerCar = W(0, 92, 7573, 113, 2102, 34377, 0, 1467);         /* 67622; 306, 736 */
const RrWorkCost RrCostDrawOpponentCar = W(34328, 124, 12038, 482, 0, 27338, 0, 616);     /* 40003; 513, 1453 */
const RrWorkCost RrCostDrawCars = W(21760, 0, 0, 0, 0, 0, 0, 0);
/* SetupPlayerView's (and the other course views') parts besides
 * SubmitCourseHorizon (submodel.c): BuildViewSegmentObjectList,
 * ApplySegmentVisibilityMask and the rest, 2600 a call */
const RrWorkCost RrCostViewSegmentList = W(30149, 0, 0, 0, 8467, 0, 0, 0);    /* 682; 44, 96 */
const RrWorkCost RrCostSegmentVisibility = W(11036, 0, 0, 0, 8440, 0, 0, 0);  /* 336; 3, 55 */
const RrWorkCost RrCostCourseView = W(166400, 0, 0, 0, 0, 0, 0, 0);
const RrWorkCost RrCostDrawTrackObjects = W(109263, 95, 1102, 0, 5813, 11679, 0, 15349); /* 10563; 597, 1061 */
const RrWorkCost RrCostDrawTrackScenery = W(50928, 140, 6787, 915, 7702, 915, 11285, 0);  /* 34989; 125, 413 */
/* SubmitModel where nothing above includes it (the menus' models: the car
 * selects' cars, 49k clocks a frame in the Grand Prix's), from its calls in
 * the race frames above (2981 frames) */
const RrWorkCost RrCostSubmitModel = W(6975, 174, 3466, 3943, 3466, 0, 0, 1993); /* 26916; 28, 133 */
#undef W

/* StepRaceFrame less the calls above, without its stalls: 5114 with one
 * car (Time Attack), 10488 with two (Versus), 47.1k-48.6k with eight, seven
 * of them the AI's (the Grand Prix), medians; a replay's 450 a car and 520
 * an AI car more (5.6k-5.8k, 11.2k, 54.9k). */
void RrSpendStepRaceFrameOwn(int comCars) {
    int cars = 0, ai = 0;
    for (int i = 0; i < NumCars; i++)
        cars += CarTable[i]->half1E8 != -1;
    if (comCars)
        for (int i = 0; i < 11; i++)
            ai += (short)ComCars[i].flags1E4 > 0;
    if (cars > 0) {
        int own = 5374 * cars + 706 * ai - 260;
        if (ModeGroup == 1 && ModeIndex == 22) /* ReplayModeUpdate */
            own += 450 * cars + 520 * ai;
        RR_CPU_CYCLES((unsigned)own);
    }
}

/* The race modes' frame functions less the routines above, their stalls
 * and the lap list (DrawLapTimeList, charged by its laps: rr/cpu_time.h),
 * by RaceState (0 the start, 1 the countdown, 2-3 racing, from 4 the
 * finish); the Grand Prix's start includes its grid's fly-by. Medians of
 * 75-786 frames (the Grand Prix's four races 8.4k-8.5k racing, its third
 * 10.6k; Time Attack's on course 7 1.9k longer racing: its overlay
 * sprites and lap timing). */
void RrSpendRaceModeOwn(int mode) {
    static const unsigned own[3][4] = {
        {3439, 10293, 8214, 1020},
        {6061, 18649, 13965, 1944},
        {78000, 11550, 8475, 1590},
    };
    int state = RaceState < 0 ? 0 : RaceState >= 4 ? 3 : RaceState == 3 ? 2 : RaceState;
    if (mode >= 0 && mode < 3)
        RR_CPU_CYCLES(own[mode][state]);
}

/* ReplayModeUpdate's own, 1.2k-1.8k, and UpdateRaceCamera's for the node
 * camera of the replay (FindNearestTrackEntry over the course's camera
 * nodes, about 1.0k-1.1k, then the node's kind: 2.0k fixed (kind 3), 3.2k
 * with the car's point and the angles (CarLocalToWorld, CordicSqrt: kinds
 * 1, 4, 6), 3.5k following the car's distance (2), ResetCarSlotBuffers'
 * 142 when the node changes), besides the chase camera and
 * BuildCameraMatrices, which charge themselves: DuckStation's exact timing
 * over the Grand Prix's four replays (325 frames, DSF_MARKS, DSF_EXC, the
 * list DMA's stalls taken out), which the port did not charge. [mark] */
extern int RaceCameraKind;
void RrSpendReplayFrame(void) {
    static const unsigned kind[7] = {1300, 3163, 3478, 2023, 3163, 3163, 3163};
    unsigned c = 1400 + kind[RaceCameraKind >= 0 && RaceCameraKind < 7 ? RaceCameraKind : 3];
    if (RaceContext.cameraNodeChanged1C)
        c += 142;
    RR_CPU_CYCLES(c);
}
