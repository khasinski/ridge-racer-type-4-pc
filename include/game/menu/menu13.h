#ifndef RR_GAME_MENU_MENU13_H
#define RR_GAME_MENU_MENU13_H

#include "types.h"
#include "rr/gpu.h"
#include "rr/menu_text_state.h"

extern unsigned char *DialoguePageCounts;
extern u16 *DialogueScriptPtrs[];
void SkipDialoguePage(MenuTextState *arg0);
s32 DecodeCourseIndex(s32 a0);
void *EmitSpinningWheelIcon(GpuDmaTag *ordering, void *cursor, s32 x, s32 y, s32 frame);


#endif /* RR_GAME_MENU_MENU13_H */
