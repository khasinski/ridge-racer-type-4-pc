#ifndef RR_GAME_MENU_MENUTRA21_H
#define RR_GAME_MENU_MENUTRA21_H

#include "types.h"

int LoadSaveFromCardStep(int a0);
void DrawCardMenuStatusText(int a0, int a1, int a2, int a3, int a4, int a5);
void RunCardSaveOrLoadStep(void);
void StepCardConfirmPrompt(void);
void WaitCardDialogueComplete(void);
void RunCardFormatStep(void);
void InitCardMenuSession(void);
void CloseCardMenuUiRows(void);
void StepCardSlotHeaderScan(void);
void CheckCardPollStatusTransition(void);
void CheckCardFileExistsTransition(void);
void StepCardDataTransfer(void);
int ScanBothCardPortsStep(int a0);
int ReadCardSlotHeaderStep(int a0, int a1);

extern s16 CardMenuOperation;
extern u16 CardMenuReturnModeId;
extern s16 CardPromptChoices[4];
extern u8 CardPortPollStatus[2];

#endif /* RR_GAME_MENU_MENUTRA21_H */
