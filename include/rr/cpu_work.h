#ifndef RR_CPU_WORK_H
#define RR_CPU_WORK_H

/* Leftovers of the removed PS1 CPU time model. The port runs on the host's
 * clock: nothing spends PS1 time any more (docs/ARCHITECTURE.md, Time).
 * src/main/race and src/main/car still name the model's calls while their
 * field-rate conversion is under way; these no-ops keep them building until
 * the calls are deleted there (docs/TECH_DEBT.md D21). Nothing else may
 * include this header. */

#include "types.h"

#define RR_CPU_CYCLES(...) ((void)0)
#define RR_WORK_CALL(call, cost)                                               \
    do {                                                                       \
        call;                                                                  \
    } while (0)
#define RR_WORK_DRAW(call, cost) RR_WORK_CALL(call, cost)

typedef struct {
    int unused;
} RrWork;
typedef struct {
    unsigned long long per[1];
} RrWorkCost;

#define RrWorkBegin(...) ((void)0)
#define RrWorkSpend(...) ((void)0)
#define RrWorkSpendPart(...) ((void)0)
#define RrWorkArmEnd(...) ((void)0)
#define RrWorkPush(...) ((void)0)
#define RrWorkPop() ((void)0)
#define RrSpendStepRaceFrameLoop() ((void)0)
#define RrSpendStepRaceFrameImpacts(...) ((void)0)
#define RrSpendStepRaceFrameOwn(...) ((void)0)
#define RrSpendReplayFrame() ((void)0)
#define Psyz_EmulatedClocks() 0ULL

enum {
    RR_WORK_BUMP_NODES,
    RR_WORK_CAR_FAR,
    RR_WORK_CAR_MID,
    RR_WORK_CAR_NEAR,
    RR_WORK_COLLIDE_CARS,
    RR_WORK_CURVE_STEPS,
    RR_WORK_EMIT_ITEMS,
    RR_WORK_OBJECT_KEYS,
    RR_WORK_OBJECTS,
    RR_WORK_OBJECTS_ACTIVE,
    RR_WORK_WHEEL_AXLES,
    RR_WORK_WHEEL_CURVED,
    RR_WORK_WHEEL_SHOULDER,
    RR_WORK_WHEEL_SHOULDER_PAST,
    RR_WORK_LEFTOVER_COUNTS
};
static unsigned long long RrWorkCount[RR_WORK_LEFTOVER_COUNTS];
static int RrWorkFrame, RrWorkDepth;

/* The BIOS bzero of a structure the host lays out larger: the host's
 * bytes are cleared. */
void PsyqBzero(void *buffer, s32 size);
#define PsyqBzeroPs1(buffer, hostBytes, ps1Bytes) PsyqBzero((buffer), (s32)(hostBytes))

#endif
