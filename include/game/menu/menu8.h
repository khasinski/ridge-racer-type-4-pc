#ifndef RR_GAME_MENU_MENU8_H
#define RR_GAME_MENU_MENU8_H

#include "types.h"
#include "rr/layout.h"

typedef struct {
    s32 score00;
    s16 carIndex04;
    s8 gridRank06;
    s8 unk07;
} RankEntry8;
RR_ASSERT_SIZE(RankEntry8, 0x8);

/* Race standings scratch (0x800F2A68), one entry per grid slot. */
extern RankEntry8 RankEntries[8];
extern RankEntry8 *RankTable[];

void SelectDialogueScript(void);
void UpdateDialoguePanel(void);
void UpdateDialogueBox(void);
void DialogueWaitEnterHook(void);
void DialogueWaitExitHook(void);
void ResetRankEntries(void);
void ResetRankEntriesForCars(void);
void InitSelectionRows(void);
void GpSaveSlotMenuEnter(void);
void GpSaveSlotMenuUpdate(void);
void StepReplaySavePrompt(void);
void YesNoPromptModeEnter(void);

extern s16 GpSaveSlotCursor;

#endif /* RR_GAME_MENU_MENU8_H */
