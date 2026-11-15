#ifndef RR_CD_READ_REQUEST_H
#define RR_CD_READ_REQUEST_H
#include "rr/library.h"

/* Input-side completion fields in the same PAL record as the older partial
 * LibTransferState38 view. Buffer and callback slots retain native pointers. */
typedef struct {
    LibCompletionCallback saved28;
    LibCompletionCallback volatile saved2C;
    LibCompletionCallback saved30;
    u8 *notify34; /* Callback result buffer, saved without address narrowing. */
} LibReadRestoreTail;
typedef struct {
    s32 sectors00;
    u8 *buffer04;
    u8 *position08;
    volatile u32 mode0C;
    u32 step10;
    s32 remaining14;
    u32 retry18;
    volatile u32 start1C;
    u32 completed20;
    s32 finished24;
    LibReadRestoreTail restore;
} LibReadRequestState;
extern LibReadRequestState CdReadState;
extern volatile s32 D_800A67D4;
extern LibCompletionCallback CdReadSavedSyncCallback;

/* Waits for pending completion with unsigned elapsed ticks (timeout at121),
 * restores its saved callback on timeout, configures the request, and saves
 * displaced callbacks. Returns whether the begin helper reports >0.
 * Buffer and callback values are passed/stored without pointer truncation. */
s32 CdRead(s32 sectors,u8 *buffer,u32 mode);
/* Clears active callbacks, optionally repositions, captures the requested mode,
 * then starts transfer. Shell-open/command failure publishes remaining=-1.
 * Successful return reloads remaining after the clock helper completes. */
s32 cd_read_retry(s32 restart);
RR_ASSERT_SIZE(LibReadRestoreTail, 3 * sizeof(LibCompletionCallback) + sizeof(u8 *));
RR_ASSERT_OFFSET(LibReadRestoreTail, saved2C, sizeof(LibCompletionCallback));
RR_ASSERT_OFFSET(LibReadRestoreTail, saved30, 2 * sizeof(LibCompletionCallback));
RR_ASSERT_OFFSET(LibReadRestoreTail, notify34, 3 * sizeof(LibCompletionCallback));
RR_ASSERT_SIZE(LibReadRequestState, RR_FIELD_OFFSET(LibReadRequestState, restore) + sizeof(LibReadRestoreTail));
RR_ASSERT_OFFSET(LibReadRequestState, buffer04, RR_ALIGN_UP(4, _Alignof(u8 *)));
RR_ASSERT_OFFSET(LibReadRequestState, mode0C, RR_FIELD_OFFSET(LibReadRequestState, buffer04) + 2 * sizeof(u8 *));
RR_ASSERT_OFFSET(LibReadRequestState, step10, RR_FIELD_OFFSET(LibReadRequestState, mode0C) + 4);
RR_ASSERT_OFFSET(LibReadRequestState, start1C, RR_FIELD_OFFSET(LibReadRequestState, mode0C) + 16);
RR_ASSERT_OFFSET(LibReadRequestState, finished24, RR_FIELD_OFFSET(LibReadRequestState, mode0C) + 24);
RR_ASSERT_OFFSET(LibReadRequestState, restore, RR_ALIGN_UP(RR_FIELD_OFFSET(LibReadRequestState, mode0C) + 28, _Alignof(LibReadRestoreTail)));
#endif
