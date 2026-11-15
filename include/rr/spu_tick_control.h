#ifndef RR_SPU_TICK_CONTROL_H
#define RR_SPU_TICK_CONTROL_H
#include "rr/spu_tick.h"
#include "rr/library.h"
#include "rr/counters.h"
/* The first member is the existing four-slot prefix. A prefix pointer can
 * be converted back to its complete eight-slot owner without narrowing. */
typedef void (*SpuTickInstall)(s32 slot,SpuTickCallback callback);
typedef struct {
    LibInterruptDriverPrefix prefix;
    void (*reserved10)(void);
    SpuTickInstall install14;
    void (*reserved18)(void);
    LibSavedState *state1C;
} SpuTickInterruptDriver20;
extern SpuTickCallback VSyncCallbackTable[8];
void setIntrVSync(s32 slot,SpuTickCallback callback);
void VSyncCallback(SpuTickCallback callback);
/* PsyZ kernel.h prefixes these with PS1_ to avoid the Win32 names. */
void PS1_EnterCriticalSection(void);
void PS1_ExitCriticalSection(void);
#define EnterCriticalSection PS1_EnterCriticalSection
#define ExitCriticalSection PS1_ExitCriticalSection
s32 ResetRCnt(u32 selector);
s32 SetRCnt(u32 selector,u16 target,s32 mode);
void _SsTickCallback(void);
void _SsTickCallbackDivided(void);
void _SsStart(s32 direct);
RR_ASSERT_SIZE(SpuTickInterruptDriver20, sizeof(LibInterruptDriverPrefix) + 3 * sizeof(SpuTickCallback) + sizeof(LibSavedState *));
RR_ASSERT_OFFSET(SpuTickInterruptDriver20, install14, sizeof(LibInterruptDriverPrefix) + sizeof(SpuTickCallback));
RR_ASSERT_OFFSET(SpuTickInterruptDriver20, state1C, sizeof(LibInterruptDriverPrefix) + 3 * sizeof(SpuTickCallback));
#endif
