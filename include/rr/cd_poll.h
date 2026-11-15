#ifndef RR_CD_POLL_H
#define RR_CD_POLL_H
#include "rr/cd.h"
#include "rr/timing.h"
/* Names are indexed by command0..27 and completion code0..6. The timeout
 * diagnostic requires the two status bytes to select those seven names. */
extern const char *CD_comstr[28],*CD_intstr[7];
extern volatile u8 CD_intr_status[];
extern s32 CD_timeout_deadline;
extern s32 CD_timeout_count;
extern const char *CdWaitCallerName[];
extern const char CdTimeoutPrefixStr[],CdTimeoutStatusFmt[],CdDatasyncNameStr[];
s32 DebugPrintStub(const char *format, ...);
s32 PsyqPuts(const char *message);
void CD_flush(void);
/* Mode0 waits; a nonzero mode polls once. Returns-1 on timeout,0 when idle,
 * and1 when a nonblocking poll finds DMA busy. Counters wrap in32 bits. */
s32 CD_datasync(s32 mode);
#endif
