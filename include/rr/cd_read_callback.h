#ifndef RR_CD_READ_CALLBACK_H
#define RR_CD_READ_CALLBACK_H
#include "rr/cd_read_request.h"

/* Overlapping window beginning at the full transfer-position pointer. */
typedef struct { u8 *position08; u32 mode0C,step10; } CdPositionView;
typedef void (*CdDataNotify)(s32 event,u8 *result);
extern CdPositionView D_800A67B8;
extern u32 D_800A67D0;

/* ILP32 PAL and LP64 native layouts: the first scalar word is padded to pointer
 * alignment, followed by two pointers and the remaining scalar fields. Native
 * fixtures assert these owner displacements against offsetof on the full record. */
#define RR_READ_COMPLETED_OWNER_OFFSET (3*sizeof(u8*)+5*sizeof(u32))
#define RR_READ_POSITION_OWNER_OFFSET (2*sizeof(u8*))

/* Requires accessible transfer-position storage for step*4 bytes and a filled
 * 12-byte header from either transfer helper. Event is narrowed to its low byte.
 * Timers compare signed32 ticks against wrapping32(start+1200), reloading start
 * after each clock call. Notification receives the original full result pointer. */
void cb_read(s32 event,u8 *result);
RR_ASSERT_SIZE(CdPositionView, sizeof(u8 *) + 8);
RR_ASSERT_OFFSET(CdPositionView, step10, sizeof(u8 *) + 4);
#endif
