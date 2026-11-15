#ifndef RR_GAME_MATH_SHUFFLE_H
#define RR_GAME_MATH_SHUFFLE_H

#include "types.h"
#include "rr/layout.h"

/* Attract-demo course order (0x8010E240). The PS1 map names cursor24
 * ShuffleRandomSeedState (0x8010E264). */
typedef struct {
    s32 pending00;  /* non-zero: reshuffle before the next demo */
    s32 order04[8]; /* shuffled course order */
    s32 cursor24;   /* next order04 entry */
} SlotOrder28;
extern SlotOrder28 SlotOrder;
RR_ASSERT_SIZE(SlotOrder28, 0x28);

void ShuffleSlotOrder(void);

#endif /* RR_GAME_MATH_SHUFFLE_H */
