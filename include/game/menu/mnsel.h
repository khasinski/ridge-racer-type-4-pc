#ifndef RR_GAME_MENU_MNSEL_H
#define RR_GAME_MENU_MNSEL_H

#include "types.h"

void StepModeSelectChoice(void);
/* Bind caller-provided storage, publish MenuGridRecords, then clear count
 * complete records through the BIOS service. Count is nonnegative and the
 * byte count fits signed32; buffer covers that many aligned MenuGrid38 records.
 * Returns the byte count. A service replacement of the root is retained. */
int AllocMenuGridRecords(void *buffer, int count);
void InitMenuGrid(s32 index, s32 layout, s32 count, s32 x, s32 y);


#endif /* RR_GAME_MENU_MNSEL_H */
