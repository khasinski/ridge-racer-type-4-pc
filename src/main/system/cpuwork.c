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
    raw[RR_WORK_SQR] = Psyz_GteOpCount(0x28);
    raw[RR_WORK_CC] = Psyz_GteOpCount(0x1C);
    raw[RR_WORK_NCC] = Psyz_GteOpCount(0x1B) + Psyz_GteOpCount(0x3F);
    raw[RR_WORK_CALL] = raw[RR_WORK_BYTES] = 0;
}

/* The counts so far less the host-only work held out (the fitting tools'
 * view of the PS1's work, RR4_MARK_WORK's XHZN) */
void RrWorkNet(unsigned long long out[RR_WORK_UNITS], unsigned long long *heldBytes) {
    WorkRaw(out);
    for (int k = 0; k < RR_WORK_UNITS; k++)
        out[k] -= Held[k];
    *heldBytes = HeldBytes;
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

void RrWorkSpendPart(RrWork *work, const void *cursor, const RrWorkCost *cost) {
    RrWorkCost part = *cost;
    part.per[RR_WORK_CALL] = 0;
    RrWorkSpend(work, cursor, &part);
    RrWorkBegin(work, cursor);
}

enum { WORK_STACK = 16 };
static struct {
    RrWork *work;
    const RrWorkCost *cost;
    int packets;
} WorkStack[WORK_STACK];
static int WorkTop;

void RrWorkPush(RrWork *work, int packets, const RrWorkCost *cost) {
    if (WorkTop < WORK_STACK) {
        WorkStack[WorkTop].work = work;
        WorkStack[WorkTop].cost = cost;
        WorkStack[WorkTop].packets = packets;
    }
    WorkTop++;
}

void RrWorkPop(void) {
    if (WorkTop > 0)
        WorkTop--;
}

void RrWorkFlush(void) {
    if (WorkTop <= 0 || WorkTop > WORK_STACK)
        return;
    RrWorkSpendPart(WorkStack[WorkTop - 1].work,
                    WorkStack[WorkTop - 1].packets ? RR_GPU_SCRATCH->cursor : 0,
                    WorkStack[WorkTop - 1].cost);
}

static RrWork *EndWork;
static const RrWorkCost *EndCost;

void RrWorkArmEnd(RrWork *work, const RrWorkCost *cost) {
    EndWork = work;
    EndCost = cost;
}

void RrWorkEnd(const void *cursor) {
    if (EndWork) {
        RrWork *work = EndWork;
        EndWork = NULL;
        RrWorkSpend(work, cursor, EndCost);
    }
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
    {{(call), (bytes), (rtpt), (rtps), (nclip), (dpcs), (intpl), (mvmva)}}
const RrWorkCost RrCostReadCarInput = W(12622, 0, 0, 0, 0, 0, 0, 0);       /* 218; 42, 158 */
/* UpdatePlayerCar and UpdateComCars by the race's state, their paths:
 * before the start (RaceState 0) the player's car 177 a call and the AI
 * 2520 besides its BIOS rand calls (two an AI car, 433 each, which charge
 * themselves: DuckStation's call 8.7k with seven, the Grand Prix's fly-by
 * on course 7, where 8538 counted them twice and its frames ran 4.5k long),
 * in the countdown (1) 2440 and 2784 besides the rand calls of the AI's
 * engine revving (DuckStation 3.5k a call with them, 0.7k of them a frame:
 * the first race's countdown), racing 3060 and 3530 + 7716 an AI car it
 * drives (its projection, the one MVMVA each; 57k-59k with seven).
 * One cost for every state had the AI 3k short racing, 3k long in the
 * countdown and the player's car 2.3k long before the start. Racing, the
 * AI's time goes with the track nodes ScanCurveAhead looks at (its curve
 * scans for the overtake's score and the drive, the course's node
 * lengths and the car's speed): 4569 an AI car and 232.5 a node besides
 * the call's 3530 (DuckStation's exact timing, its list DMA stalls taken
 * out, the Grand Prix's second, third and course 7's final race, 193
 * frames: residual median 245, 90th percentile 778), where the MVMVA's
 * 7716 had course 7's final race 10k long (47.7k on DuckStation with 38-64
 * nodes a frame against the other races' 57k with 94). [mark] */
static const RrWorkCost CostUpdatePlayerCar[3] = {
    W(11328, 0, 0, 0, 0, 0, 0, 0), W(156160, 0, 0, 0, 0, 0, 0, 0), W(195840, 0, 0, 0, 0, 0, 0, 0)};
static const RrWorkCost CostUpdateComCars[3] = {
    W(161280, 0, 0, 0, 0, 0, 0, 493824), W(178176, 0, 0, 0, 0, 0, 0, 493824),
    {{[RR_WORK_CALL] = 225920, [RR_WORK_MVMVA] = 292416, [RR_WORK_CURVE_STEPS] = 14880}}};
const RrWorkCost *RrCostUpdatePlayerCar(void) {
    return &CostUpdatePlayerCar[RaceState <= 0 ? 0 : RaceState == 1 ? 1 : 2];
}
const RrWorkCost *RrCostUpdateComCars(void) {
    return &CostUpdateComCars[RaceState <= 0 ? 0 : RaceState == 1 ? 1 : 2];
}
/* The eleven stages a car each (CarUpdateStages), from a call's entry to
 * the next call's: ClampAiCarLateralMotion, UpdateCarSteering,
 * DecayCarOffset, IntegrateCarMotion (by its MVMVAs), UpdateCarTrackNode
 * (its NCLIPs), ComputeWheelOffsets, ConstrainCarToTrack; then
 * ResolveCarCollisions 141 and 166.8 an other car it looks at (none in
 * Time Attack), UpdateCarShake, FindWheelTrackNodes by its axle search
 * (FindAxleTrackNode's NCLIPs), the axles set up, the wheels on a curved
 * node and past the road's edge, UpdateWheelBumps 1210 and 141 a wheel on
 * another node. DuckStation's exact timing, the list DMA's stalls taken
 * out: the Grand Prix's race on course 0 and its final on course 7 and
 * Time Attack on course 4 (gp-rules, gp-ending, ta-course4; 4300 calls a
 * stage, residual medians 4-18, FindWheelTrackNodes' 4: 2707 a call on
 * course 0 and 3242 on course 7 as DuckStation's). The later four were
 * charged with StepRaceFrame's own time by the cars, 2.7k long a frame on
 * course 7 and at the frame's end. [mark] */
#define S(call, ...) {{[RR_WORK_CALL] = (call) * 64, __VA_ARGS__}}
const RrWorkCost RrCostCarStages[11] = {
    S(84),
    S(1000),
    S(58),
    S(133, [RR_WORK_MVMVA] = 25485),
    S(93, [RR_WORK_NCLIP] = 8416),
    S(32, [RR_WORK_MVMVA] = 8090),
    S(1085),
    S(141, [RR_WORK_COLLIDE_CARS] = 10675),
    S(107),
    S(1850, [RR_WORK_NCLIP] = 8250, [RR_WORK_WHEEL_AXLES] = 22131, [RR_WORK_WHEEL_CURVED] = 2074,
      [RR_WORK_WHEEL_SHOULDER] = 7718, [RR_WORK_WHEEL_SHOULDER_PAST] = 4352),
    S(1210, [RR_WORK_BUMP_NODES] = 9018),
};
#undef S
/* UpdateTrackObjects by the course objects it looks at, the active ones and
 * their keyframe interpolations, and its NCLIPs (the camera's node search):
 * 608, 55, 244, 1027 and 125.5, DuckStation's exact timing over the Grand
 * Prix's race on course 0, its final on course 7 and Time Attack on
 * course 4 (1191 frames, residual median 48; 8100 clocks a frame on course
 * 7, its fireworks, where the call and NCLIP costs had it 3.8k short and
 * course 4's 2.2k long) [mark] */
const RrWorkCost RrCostUpdateTrackObjects = {{[RR_WORK_CALL] = 608 * 64, [RR_WORK_NCLIP] = 8032,
    [RR_WORK_OBJECTS] = 3539, [RR_WORK_OBJECTS_ACTIVE] = 15635, [RR_WORK_OBJECT_KEYS] = 65715}};
const RrWorkCost RrCostChaseCamera = W(6130, 0, 0, 0, 0, 0, 0, 18389); /* UpdateRaceChaseCamera 1094; 1, 2 */
const RrWorkCost RrCostCameraMatrices = W(0, 0, 0, 0, 0, 0, 0, 5225);  /* BuildCameraMatrices 2571; 2, 9 */
/* DrawPlayerCar and the menus' car (DrawPreviewCar: the car selects, the
 * garage) by one cost, fitted jointly on DuckStation's calls (exceptions
 * out, the list DMA's stalls and the inner marks taken off): Time Attack's
 * car select and race on course 1 and a Grand Prix race on course 0, 385
 * calls, residual median 70, 90th percentile 219, the menus' car 33 and the
 * race's -13 and 42 in the median; its lit polygons' NCCS and NCCT (the
 * GTE's, RR_WORK_NCC) and DPCS, INTPL, its RTPTs and NCLIPs. The race's
 * cost alone had the menus' car 1.1k short. [mark] */
const RrWorkCost RrCostDrawPlayerCar = {{[RR_WORK_BYTES] = 75, [RR_WORK_RTPT] = 8303, [RR_WORK_NCLIP] = 1997,
    [RR_WORK_DPCS] = 8110, [RR_WORK_INTPL] = 10137, [RR_WORK_NCC] = 38728, [RR_WORK_CC] = 620, [RR_WORK_SQR] = 1222}};
/* DrawOpponentCar by its model's distance (near, middle: its parts, the
 * body and wheels), its trails' points with the lights on (the night
 * courses) and the GTE's SQR and CC for its lights: DuckStation's exact
 * timing, the Grand Prix's races on courses 0 and 7 (gp-rules, gp-ending:
 * 15078 calls without an exception inside on either side, the list DMA's
 * stalls out, residual median 28, 90th percentile 226; a frame's cars
 * within -387..464 on course 0 and -1.8k..1.2k on course 7, where the
 * calls' cost before had them 2.2k long on course 0 and up to 4.1k short
 * on course 7's race, its lights not counted) [mark] */
const RrWorkCost RrCostDrawOpponentCar = {{[RR_WORK_CALL] = 36191, [RR_WORK_BYTES] = 51, [RR_WORK_RTPT] = 7510,
    [RR_WORK_RTPS] = 2730, [RR_WORK_DPCS] = 26336, [RR_WORK_SQR] = 1665, [RR_WORK_CC] = 833,
    [RR_WORK_CAR_NEAR] = 264066, [RR_WORK_CAR_MID] = 88685, [RR_WORK_CAR_TRAILS] = 55971,
    [RR_WORK_CAR_LIGHTS] = 60931}};
/* DrawCars' own (339-987 for 1-8 cars: 340 a call) */
const RrWorkCost RrCostDrawCars = W(21760, 0, 0, 0, 0, 0, 0, 0);
/* SetupPlayerView's (and the other course views') parts besides
 * SubmitCourseHorizon (submodel.c): BuildViewSegmentObjectList,
 * ApplySegmentVisibilityMask and the rest, 2600 a call */
const RrWorkCost RrCostViewSegmentList = W(30149, 0, 0, 0, 8467, 0, 0, 0);    /* 682; 44, 96 */
const RrWorkCost RrCostSegmentVisibility = W(11036, 0, 0, 0, 8440, 0, 0, 0);  /* 336; 3, 55 */
const RrWorkCost RrCostCourseView = W(166400, 0, 0, 0, 0, 0, 0, 0);
const RrWorkCost RrCostDrawTrackObjects = W(11053, 0, 0, 0, 6586, 0, 0, 11347);
/* EmitSemiTransG3List and EmitSemiTransG4List by the list's entries and the
 * primitives they emit (32 and 72 bytes): 97 + 161 an entry + 71 a
 * triangle, 28 + 224.6 an entry + 168 a quad, to the clock on DuckStation
 * (the course objects' lists in gp-ending, gp-rules, extra-trial-race and
 * Time Attack on courses 1, 4 and 7, every list length the courses have),
 * where the bytes alone had a list with one of its triangles drawn 139
 * clocks long and the quads' list with none 1.8k short [mark] */
const RrWorkCost RrCostEmitSemiTransG3 = {{[RR_WORK_CALL] = 6208, [RR_WORK_BYTES] = 142, [RR_WORK_EMIT_ITEMS] = 10304}};
const RrWorkCost RrCostEmitSemiTransG4 = {{[RR_WORK_CALL] = 1798, [RR_WORK_BYTES] = 149, [RR_WORK_EMIT_ITEMS] = 14376}};
const RrWorkCost RrCostTrackObjectLit = W(9210, 58, 7398, 0, 0, 0, 0, 1984);
const RrWorkCost RrCostTrackObjectBillboard = W(550, 19, 5389, 5389, 0, 0, 0, 6054);
const RrWorkCost RrCostTrackObjectFlashQuads = W(384, 32, 5638, 5638, 5638, 0, 0, 0);
const RrWorkCost RrCostTrackObjectCourseModel = W(37600, 141, 6982, 832, 1459, 0, 0, 0);
const RrWorkCost RrCostTrackObjectAnimated = W(0, 95, 1102, 0, 5813, 11679, 0, 15349);
const RrWorkCost RrCostTrackObjectModel = W(0, 95, 1102, 0, 5813, 11679, 0, 15349);
const RrWorkCost RrCostDrawTrackScenery = W(50928, 140, 6787, 915, 7702, 915, 11285, 0);  /* 34989; 125, 413 */
/* SubmitModel where nothing above includes it (the menus' models: the car
 * selects' cars, 49k clocks a frame in the Grand Prix's), from its calls in
 * the race frames above (2981 frames) */
const RrWorkCost RrCostSubmitModel = W(6975, 174, 3466, 3943, 3466, 0, 0, 1993); /* 26916; 28, 133 */
/* SubmitModelTwoTri and DrawMeshPrimList outside a race frame (the menus'
 * presentation models and meshes, the options' controller): SubmitModel's
 * costs scaled to DuckStation's clocks for the records' menu's motion model
 * (34.6k against 40.0k, the median of 138 calls) and its meshes (3161
 * against 4102, 156 calls), DSF_MARKS, the stubs out [mark] */
const RrWorkCost RrCostSubmitModelTwoTri = W(6033, 151, 2998, 3411, 2998, 0, 0, 1724);
const RrWorkCost RrCostMeshPrimList = W(5375, 134, 2671, 3039, 2671, 0, 0, 1536);
#undef W

/* A stage loop's end (18 clocks past a call within it) and the impact loop
 * between the seventh stage and the eighth: 23, 68 a car, 4870 a car with
 * an impact (SetCarImpactState, StartCollisionShake, the speed's scaling),
 * 107 a car it tests for kerb contact and 51 a wheel in contact
 * (DuckStation, as the stages above: residual median 5) [mark] */
void RrSpendStepRaceFrameLoop(void) {
    RR_CPU_CYCLES(18);
}
void RrSpendStepRaceFrameImpacts(int cars, int impacts, int contactCars, int contacts) {
    RR_CPU_CYCLES((unsigned)(23 + 68 * cars + 4870 * impacts + 107 * contactCars + 51 * contacts));
}
/* The rest of StepRaceFrame: its set-up before the first call and its end,
 * UpdateComCarTrackProjections' 446 where it ran (comCars). [mark] */
void RrSpendStepRaceFrameOwn(int comCars) {
    RR_CPU_CYCLES(RR_CYCLES_STEP_RACE_FRAME_OWN + (comCars ? 446 : 0));
}

/* The race modes' frame functions less the routines above, their stalls
 * and the HUD's routines (rr/cpu_time.h RR_CYCLES_UPDATE_LAP_TIMING ...,
 * DrawLapTimeList by its laps), by RaceState (0 the start, 1 the
 * countdown, 2-3 racing, from 4 the finish). The Grand Prix and Time
 * Attack draw their HUD before the race step and have no more there on
 * DuckStation (106-275 clocks from the last HUD routine to the step,
 * their own 213 from RaceState 4); their lumps of 1.0k-11.6k were the HUD
 * routines' time, charged after them. Versus draws its HUD after the
 * step: its lump, less those routines' charges, comes after the HUD
 * (DuckStation: 247 from UpdateRaceRanking to the step). */
void RrSpendRaceModeOwn(int mode) {
    static const unsigned own[3][4] = {
        {0, 0, 0, 90},
        {5668, 13111, 9072, 1928},
        {0, 0, 0, 90},
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
void RrSpendReplayCamera(void) {
    static const unsigned kind[7] = {1300, 3163, 3478, 2023, 3163, 3163, 3163};
    unsigned c = kind[RaceCameraKind >= 0 && RaceCameraKind < 7 ? RaceCameraKind : 3];
    if (RaceContext.cameraNodeChanged1C)
        c += 142;
    RR_CPU_CYCLES(c);
}
void RrSpendReplayFrame(void) {
    RR_CPU_CYCLES(1400);
}
