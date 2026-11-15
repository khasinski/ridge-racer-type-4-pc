#ifndef RR_GAME_CAR_CARSTAT_H
#define RR_GAME_CAR_CARSTAT_H

#include "types.h"
#include "rr/layout.h"

/* Per-player button masks for the eight game actions (0x800F2F78, two
 * players), one table per controller family: standard digital/analogue pads
 * (type 0x41/0x73), calibrated neGcon-style pads (0x23) and wheels (0xE3). */
typedef struct {
    u16 standard[8];   /* +0x00 from PadButtonLayoutPresetsA */
    u16 calibrated[8]; /* +0x10 from PadButtonLayoutPresetsB */
    u16 wheel[8];      /* +0x20 from PadDefaultButtonPresets */
} PadButtonMap30;
extern PadButtonMap30 PadButtonMap[2];
RR_ASSERT_SIZE(PadButtonMap30, 0x30);

void InitGamePads(void);
void SetPadButtonMapLayout(s32 player, s32 standard, s32 calibrated, s32 wheel);
void ApplyOptionsPadButtonMaps(void);

extern u16 PadDefaultButtonPresets[8][8];

#endif /* RR_GAME_CAR_CARSTAT_H */
