#ifndef RR_SPU_TICK_H
#define RR_SPU_TICK_H

#include "rr/layout.h"

typedef void (*SpuTickCallback)(void);
typedef struct {
    s8 direct, divided, interrupt, reserved;
} SpuTickFlags4;
/* Timer state through the three-word D_800A4D4C owner, ending at0x800A4D58. The first two words also have the
 * assembler names _snd_seq_tick_env and D_800A4D3C; callback pointers retain native width. */
typedef struct {
    s32 selector, active;
    SpuTickCallback callback, previous;
    SpuTickFlags4 flags;
    s32 state[3];
} SpuTickState20;

extern SpuTickState20 _snd_seq_tick_env;
extern u32 VBLANK_MINUS;

s32 GetVideoMode(void);
void SsSetTickMode(s32 mode);

RR_ASSERT_SIZE(SpuTickFlags4,4);
RR_ASSERT_SIZE(SpuTickState20,24+2*sizeof(SpuTickCallback));
RR_ASSERT_OFFSET(SpuTickState20,active,4);
RR_ASSERT_OFFSET(SpuTickState20,callback,8);
RR_ASSERT_OFFSET(SpuTickState20,previous,8+sizeof(SpuTickCallback));
RR_ASSERT_OFFSET(SpuTickState20,flags,8+2*sizeof(SpuTickCallback));
RR_ASSERT_OFFSET(SpuTickState20,state,12+2*sizeof(SpuTickCallback));

#endif
