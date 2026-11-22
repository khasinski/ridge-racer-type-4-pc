/* RODATA 0x800115ec 0x20 */
#include "game/data/cardchk.h"
#include "game/menu/savemenu.h"
#include "game/menu/menu8.h"
#include "game/menu/menumode.h"
#include "game/menu/menutra21.h"
#include "game/system/pad.h"
#include "game/menu/menustep.h"
#include "rr/frame_loop.h"
extern void (*MenuStepHandlerTable[])(void);
extern short MenuStateIndex;

extern short CardActivePort;

extern short CardPromptIndex;
extern short MenuActiveControllerIndex;



static inline int IsNonZero(int arg0)
{
  return arg0 != 0;
}

void StepCardMenuStateMachine(void) {
  int a;
  int t0;
  unsigned short v;
  int r;
  int s7;
  int s0;
  int s1;
  int s3;
  int one;
  int fp;
  r = 1;
  s0 = MenuStateTable[MenuStateIndex].flags05;
  s1 = MenuStateTable[MenuStateIndex].handlerIdx03;
  t0 = MenuStateTable[MenuStateIndex].textId00;
  s7 = s0 & 1;
  s3 = MenuStateTable[MenuStateIndex].kind02;
  fp = CardPortPollStatus[CardActivePort];
  if (IsNonZero(s7))
  {
    PollCardStatus(CardActivePort, 0, 0);
  }
  if (s0 & 2)
  {
    r = GetCardMenuInitialState();
  }
  if (r == 1)
  {
    MenuStepHandlerTable[s1]();
  }
  DrawCardMenuStatusText(s3, t0, s0, CardActivePort, CardPromptChoices[CardPromptIndex], CardPortPollStatus[CardActivePort]);
  if (IsNonZero(s7))
  {
    if (fp == 0x1E)
    {
      if (CardPortPollStatus[CardActivePort] != fp)
      {
        SubmodeState = 0;
        GpSaveSlotCursor = 0;
        v = CardMenuOperation;
        CardTransferMode = v;
        a = (short) v;
        if (a == 0)
        {
          MenuStateIndex = 0;
          return;
        }
        if (((unsigned short) (v - 6)) < 2)
        {
          MenuStateIndex = 0x29;
          CardTransferMode = 0x10;
          return;
        }
        one = 1;
        if ((a == one) || (a == 5))
        {
          MenuStateIndex = 0x20;
        }
        else
          if (a == 2)
        {
          MenuStateIndex = 0x21;
        }
        else
          if (a == 3)
        {
          MenuStateIndex = 0x17;
        }
      }
    }
  }
}

int GetCardMenuInitialState(void) {
    int a1;
    int a2;
    int v;
    a2 = -1;
    switch ((short)CardMenuOperation) {
    case 0:
        a1 = 0;
        break;
    case 1:
    case 5:
        a1 = 0x20;
        break;
    case 2:
        a1 = 0x21;
        break;
    case 3:
        a1 = 0x17;
        break;
    case 6:
    case 7:
        a1 = 0x29;
        break;
    case 4:
        break;
    }
    v = CardPortPollStatus[CardActivePort];
    switch (v) {
    case 0x28:
    case 0x3C:
    case 0x1E:
        if (PlayerControllers[MenuActiveControllerIndex].action28 != 0 || PlayerControllers[MenuActiveControllerIndex].action2C != 0) {
            MenuStateIndex = a1;
        }
        break;
    case 0x14:
    case 0x32:
        a2 = 1;
        break;
    case 0x5A:
        break;
    default:
        a2 = 1;
        break;
    }
    return a2;
}

/* Initial values of the globals this module owns. */
void (*MenuStepHandlerTable[13])(void) = {StepCardFileSlotSelection, StepCardPortSelection, UpdateCardMenuRowStates, RunCardSaveOrLoadStep, StepCardConfirmPrompt, WaitCardDialogueComplete, RunCardFormatStep, InitCardMenuSession, CloseCardMenuUiRows, StepCardSlotHeaderScan, CheckCardPollStatusTransition, CheckCardFileExistsTransition, StepCardDataTransfer};
