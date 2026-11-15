#ifndef RR_SEQUENCER_TEMPO_H
#define RR_SEQUENCER_TEMPO_H
#include "rr/spu_tick.h"
#include "rr/sequencer_init.h"

/* Consume a nonzero big-endian 24-bit tempo followed by a VLQ delta.
 * Bank and row use their signed low halves. Arithmetic wraps at 32 bits;
 * valid input must leave both division denominators nonzero. */
void _SsGetMetaEvent(s32 bank, s32 row);
#endif
