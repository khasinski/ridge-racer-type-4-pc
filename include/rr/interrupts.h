#ifndef RR_INTERRUPTS_H
#define RR_INTERRUPTS_H

#include "rr/layout.h"

/* Interrupt status and mask register pair at the configured hardware base. */
typedef struct {
    volatile u32 pending;
    volatile u32 mask;
} InterruptRegisters;

extern InterruptRegisters *I_STAT_PadVsync, *I_STAT_Card, *I_STAT_Sio, *I_STAT_Pad;
/* Exchange the interrupt mask; return the previous zero-extended 16-bit value. */
s32 SetIntrMask(s32 value);
s32 _padVsyncHookCheck(void);
s32 _cardChkVsyncIrq(void);
s32 _cardChkSioIrq(void);
s32 _sio_irq_check(void);

RR_ASSERT_SIZE(InterruptRegisters, 8);
RR_ASSERT_OFFSET(InterruptRegisters, mask, 4);

/* Native interrupt callbacks can run on another thread. Serialize short
 * accesses to their shared state with the recursive PsyZ dispatch lock.
 * Release it before waiting for a callback to make progress. */
void Psyz_KernelLockInterrupts(void);
void Psyz_KernelUnlockInterrupts(void);

#endif
