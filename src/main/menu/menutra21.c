#include "rr/release.h"
#include "game/ui/uipart1.h"
#include "game/sound/sndvab.h"
#include "game/data/savetype.h"
#include "rr/save_slots.h"
#include "game/system/pad.h"
/* RODATA 0x8001160c 0x74 */
/* Candidate module: menu transitions, 8007BD18..8007BFF4.
 * Contiguous functions indexed by the same menu state and entry table. */
#include "rr/tables.h"
#include "rr/frame_storage_types.h"
#include "game/data/cardchk.h"
#include "game/data/cardini.h"
#include "game/menu/loadmenu.h"
#include "game/menu/menumode.h"
#include "game/menu/menutra21.h"
#include "game/menu/menutra22.h"
#include "rr/course_choices.h"
#include "rr/menu_selection.h"
#include "game/data/menuvars.h"
#include "rr/ui.h"
#include "rr/badge_row_types.h"
#include "rr/menu_text_state.h"
#include "rr/screen_tiles.h"
#include "rr/string_tail_copy.h"
#include "game/sound/cdmain.h"
#include "game/data/ldscn1.h"
#include "game/sound/fxplay.h"
#include "game/menu/savemenu.h"
#include "rr/badge_rows.h"



typedef struct { s16 a, b; } P2;



extern MenuChoice TeamSelectBannerUvs[];
extern s32 MenuCursorBlinkColors[];
extern s16 MenuPanelBorderColors[];
extern MenuChoice MenuDividerLineColors[];
extern s16 MenuActiveControllerIndex;
extern s32 ModeFrame;
extern s16 MenuStateIndex;
extern u8 CardPortPollStatus[];
extern s16 CardActivePort;
extern s16 CardReplayLoadedFlag;
extern unsigned char MenuHighlightBarColors[][4];
extern unsigned short CardTransferMode;
extern unsigned short SubmodeState;
extern short CardMenuOperation;
extern short CardPromptChoices[];
extern short CardPromptIndex;
extern int ovl660_init_menu();
extern int ovl661_init_preview();
extern int ovl667_init_car_selection();
extern int ovl662_init_team_car_layout();
extern short FadeCountdown;
extern short ModeIndex;
extern short LinkState;
extern unsigned short CardMenuReturnModeId;
extern unsigned short GameModeType;
extern int DialogueSceneMode;
extern short GpSaveSlotCursor;
extern int GarageExtraDefaultTimes[];
extern s16 MenuSlideStepVectors[][2];
#include "rr/save_buffer.h"
extern int MenuSaveBannerTimer;
extern short MenuSubpageCursor;
extern short MenuSubpageScroll;
extern unsigned char CardFileCounts[];
extern char CardSaveFileName[];
extern char CardTargetFileName[];
extern int MenuConfirmDialogState;
extern int MenuTransitionTimer;
extern unsigned char GaragePreviewCamPresets[];

void RunCardSaveOrLoadStep(void);
void StepCardConfirmPrompt(void);
void WaitCardDialogueComplete(void);
void RunCardFormatStep(void);
void InitCardMenuSession();
void CloseCardMenuUiRows(void);
void StepCardSlotHeaderScan();
void CheckCardPollStatusTransition(void);
void CheckCardFileExistsTransition(void);
void StepCardDataTransfer(void);
int ScanBothCardPortsStep();
int ReadCardSlotHeaderStep();

void RunCardSaveOrLoadStep(void) {
    s32 i = MenuStateTable[MenuStateIndex].tableIdx04;
    s32 a = TeamSelectBannerUvs[i].a;
    s32 c = TeamSelectBannerUvs[i].c;
    s32 b = TeamSelectBannerUvs[i].b;
    s32 r;
    if (c == 1) r = CardSaveStep(0, CardActivePort);
    else r = CardLoadStep(0, CardActivePort);
    if (r == 4) {
        MenuStateIndex = a;
    } else if (r == 5) {
        MenuStateIndex = b;
    } else if (r == 6) {
        MenuStateIndex = b;
    }
    if (c == 0xD) {
        if (r == 4) CardReplayLoadedFlag = 1;
    }
}

void StepCardConfirmPrompt(void)
{
  s32 a1 = *(MenuCursorBlinkColors + MenuStateTable[MenuStateIndex].tableIdx04);
  if ((&PlayerControllers[MenuActiveControllerIndex])->action28 != 0)
  {
    if (a1 == 0)
    {
      PlaySfx(1);
      if (MenuStateIndex == 0xC)
      {
        MenuStateIndex = SavedMenuStateIndex;
        CardPortPollStatus[CardActivePort] = 0x5A;
        PollCardStatus(CardActivePort, 0, 1);
        return;
      }
      MenuStateIndex = 0;
    }
  }
  else
    if ((&PlayerControllers[MenuActiveControllerIndex])->action2C != 0)
  {
    if (a1 == 0)
    {
      PlaySfx(2);
      MenuStateIndex = 0;
    }
  }
}

void WaitCardDialogueComplete(void) {
    s32 i = MenuStateTable[MenuStateIndex].tableIdx04;
    s32 v = MenuPanelBorderColors[i * 2];
    if (CardDialogueStatePtr->st == 4) {
        MenuStateIndex = v;
        ModeFrame -= 1;
    }
}

void RunCardFormatStep(void) {
    s32 k;
    s32 i;
    s32 a, b;
    for (k = 0x63; k >= 0; k--) ;
    i = MenuStateTable[MenuStateIndex].tableIdx04;
    a = MenuDividerLineColors[i].a;
    b = MenuDividerLineColors[i].c;
    if (CardFormatSlot(CardActivePort, 0) == 1) {
        MenuStateIndex = a;
        return;
    }
    MenuStateIndex = b;
}

void InitCardMenuSession(void) {
    int arr[4];
    int i, c, flag, n;
    MenuStateEntry8 *base, *p;
    unsigned char *q;
    ControllerState5C *r, *b3;
    signed char sb;

    CardPdaModeFlag = 0;
    CardTransferMode = *(unsigned short *)&CardMenuOperation;
    switch ((short)CardTransferMode) {
    case 0: MenuStateIndex = 0; break;
    case 1: case 5: MenuStateIndex = 0x20; break;
    case 2: MenuStateIndex = 0x21; break;
    case 3: MenuStateIndex = 0x18; break;
    case 6: MenuStateIndex = 0x2A; CardTransferMode = 0x10; break;
    case 7: MenuStateIndex = 0x2B; CardTransferMode = 0x10; break;
    }
    /* The Japanese release defaults four yes/no prompts to the first choice. */
    base = RR_RELEASE_JP ? (MenuStateEntry8 *)MenuStateTableJp : MenuStateTable;
    p = base + MenuStateIndex;
    i = p->tableIdx04;
    c = p->promptIdx06;
    CardPromptIndex = c;
    if (SubmodeState == 0) {
        sb = p->defaultChoice07;
        if (sb >= 0) {
            CardPromptChoices[c] = sb;
        }
        SubmodeState = 1;
    }
    flag = 0;
    q = MenuHighlightBarColors[i];
    i = 0;
    arr[0] = q[0];
    arr[1] = q[1];
    arr[2] = q[2];
    arr[3] = q[3];
    n = 4;
    for (; i < 4; i++) {
        if (arr[i] == 0xFF) { n = i; break; }
        if (i == 1 && (arr[1] & 0x80)) {
            flag = 1;
            arr[1] ^= 0x80;
        }
    }
    if (flag) {
        StepMenuValue(((unsigned short)MenuActiveControllerIndex & 1) << 2, 2, (char *)&CardPromptChoices[CardPromptIndex], n, 0);
    } else {
        StepMenuValue((((unsigned short)MenuActiveControllerIndex & 1) << 2) | 1, 2, (char *)&CardPromptChoices[CardPromptIndex], n, 0);
    }
    b3 = PlayerControllers;
    r = b3 + MenuActiveControllerIndex;
    if (r->action28 != 0) {
        PlaySfx(1);
        SubmodeState = 0;
        MenuStateIndex = arr[CardPromptChoices[CardPromptIndex]];
        if (MenuStateIndex != 0x43) {
            CardPortPollStatus[CardActivePort] = 0x5A;
            PollCardStatus(CardActivePort, 0, 1);
        }
    } else if (r->action2C != 0) {
        PlaySfx(2);
        SubmodeState = 0;
        MenuStateIndex = arr[n - 1];
    }
}


void CloseCardMenuUiRows(void) {
    int i;
    UiRow24 *q;
    if (UiRows[0x23].byte14 == 2) {
        for (i = 0; i < 4; i++) {
            q = &UiRows[0x22 + i];
            if (q->byte14 == 2) {
                q->byte14 = 3;
            }
        }
        BadgeRows->state10 = 4;
        if (CardMenuOperation == 3 || CardMenuOperation == 6 || CardMenuOperation == 7) {
            BadgeRows[1].state10 = 4;
        }
        FadeCountdown = 0x40;
        if (CardMenuOperation == 0) {
            SeqFadeVolume(0, 0x3C);
        }
        ResetScreenCounters();
        DialogueSceneMode = 2;
        return;
    }
    if (FadeCountdown > 0) {
        FadeCountdown = FadeCountdown - 1;
    }
    AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x40 - FadeCountdown) << 2, 0x45);
    if (LoadMenuCarModelsStep() != 1) return;
    if (FadeCountdown > 0) return;
    if (UiRows[0x23].byte14 != 5) return;
    if (BadgeRows->state10 != 0xB) return;
    for (i = 0; i < 3; i++) {
        SaveSlotSelections[i] = SaveSlotSnapshots[i];
    }
    ClearScreenOverlayState();
    switch (CardMenuOperation) {
    case 0:
    default:
    case 4:
        SeqMuteVolume();
        ModeGroup = 1;
        ModeIndex = 3;
        SubmodeState = 0;
        break;
    case 1:
        if (CardMenuReturnModeId == 0x132) {
            ovl660_init_menu();
            SubmodeState = 0;
            ModeGroup = CardMenuReturnModeId >> 8;
            ModeIndex = CardMenuReturnModeId & 0xFF;
        }
        break;
    case 2:
        if ((GameModeType - 2) < 2U) {
            ovl661_init_preview(ActivePlayerIndex);
            ModeGroup = 1;
            ModeIndex = 0x38;
            SubmodeState = 0;
        } else if ((short)GameModeType == 4) {
            if (LinkState != -1) {
                ovl667_init_car_selection(ActivePlayerIndex);
                ModeGroup = 1;
                ModeIndex = 0x3C;
            } else {
                SeqMuteVolume();
                ModeGroup = 1;
                ModeIndex = 0x2D;
            }
            SubmodeState = 0;
        }
        break;
    case 3:
        ModeGroup = 1;
        ModeIndex = 0xF;
        SubmodeState = 0;
        break;
    case 5:
        ovl662_init_team_car_layout();
        ModeGroup = 1;
        ModeIndex = 0x40;
        SubmodeState = 0;
        break;
    case 6:
        if (CardReplayLoadedFlag != 0) {
            ModeGroup = 3;
            ModeIndex = 3;
        } else {
            ModeGroup = 3;
            ModeIndex = 2;
        }
        SubmodeState = 0;
        break;
    case 7:
        ModeGroup = 1;
        ModeIndex = 0x27;
        SubmodeState = 0;
        break;
    }
}

void StepCardSlotHeaderScan(void) {
    short t;
    if (SubmodeState == 0) {
        GarageExtraDefaultTimes[0] = -1;
        GarageExtraDefaultTimes[1] = -1;
        GarageExtraDefaultTimes[2] = -1;
        GpSaveSlotCursor = 0;
        ReadCardSlotHeaderStep(0, 0);
        SubmodeState = 1;
        return;
    }
    if (ReadCardSlotHeaderStep(1, GpSaveSlotCursor) != 0) {
        t = (unsigned short)GpSaveSlotCursor + 1;
        GpSaveSlotCursor = t;
        if (t == 3) {
            if ((short)CardMenuOperation == 7) {
                MenuStateIndex = 0x2B;
            } else {
                MenuStateIndex = 0x2A;
            }
            GpSaveSlotCursor = 0;
            SubmodeState = 0;
            return;
        }
        ReadCardSlotHeaderStep(0, t);
    }
}

void CheckCardPollStatusTransition(void) {
    int i = MenuStateTable[MenuStateIndex].tableIdx04;
    unsigned char *q = CardPollTransitionTable + i * 4;
    int x = q[0], y = q[1];
    if (CardPortPollStatus[CardActivePort] < 0x10 || x == CardPortPollStatus[CardActivePort]) {
        MenuStateIndex = y;
        return;
    }
    if (CardPortPollStatus[CardActivePort] == 0x14 && MenuStateIndex == 2) {
        MenuStateIndex = 8;
    }
}


void CheckCardFileExistsTransition(void) {
    s32 i = MenuStateTable[MenuStateIndex].tableIdx04;
    s32 a = MenuSlideStepVectors[i][0];
    s32 b = MenuSlideStepVectors[i][1];
    if (MenuStateIndex == 0x3B) {
        if (CardPortUnformatted[CardActivePort] == 1) {
            MenuStateIndex = a;
            return;
        }
        MenuStateIndex = b;
        return;
    }
    MenuStateIndex = a;
}

void StepCardDataTransfer(void)
{
  int r;
  int t1;
  unsigned v1;
  short sum;
  CardSlotScanWindow100 *window;
  SavedMenuSettings *slot;
  if (SubmodeState == 0)
  {
    ScanBothCardPortsStep(0);
    MenuSaveBannerTimer = 0;
    SubmodeState = 1;
    return;
  }
  r = ScanBothCardPortsStep(1);
  if (r > 0)
  {
    /* The slot records read at arena +0x1F00 (file offset 0x300). */
    window = &((CardSaveBlock2000 *)SaveBuffer)->scan.window1F00;
    for (t1 = 0; t1 < 3; t1++)
    {
      slot = &window->slots60[t1];
      sum = 0;
      for (v1 = 0; v1 < RR_SAVE_SLOT_HALFWORDS; v1++)
      {
        if (v1 != RR_SAVE_SLOT_CHECKSUM_HALFWORD)
        {
          sum += slot->halves[v1];
        }
      }
      if (sum == slot->signedHalves[RR_SAVE_SLOT_CHECKSUM_HALFWORD])
      {
        SaveSlotSelections[t1] = *slot;
      }
      else
      {
        SaveSlotSelections[t1].signedBytes[0x17] = -1;
      }
    }
    MenuStateIndex = 0x18;
    SubmodeState = 0;
  }
  else
    if (r < 0)
  {
    for (t1 = 2; t1 >= 0; t1--)
    {
      SaveSlotSelections[t1].signedBytes[0x17] = -1;
    }

    MenuStateIndex = 0x18;
    SubmodeState = 0;
  }
}


int ScanBothCardPortsStep(int a0) {
    int sp[14];
    int ret;
    int i;
    int r;
    int q;
    int v;
    if (a0 == 0) {
        MenuSubpageCursor = 0;
    }
    ret = 0;
    switch (MenuSubpageCursor) {
    case 0:
        CardPortPollStatus[1] = 0x5A;
        CardPortPollStatus[0] = 0x5A;
        PollCardStatus(CardActivePort, 0, 1);
        CopyStringClearTail(CardTargetFileName, CardSaveFileName);
        MenuSubpageCursor = 1;
        break;
    case 1:
        PollCardStatus(CardActivePort, 0, 0);
        if (CardPortPollStatus[0] < 0x10U) {
            if (CardHasFile(CardFileCounts[0], CardTargetFileName) == 1) {
                MenuSubpageScroll = 0;
                MenuSubpageCursor = 2;
            } else {
                MenuSubpageCursor = 7;
            }
            break;
        }
        v = CardPortPollStatus[0];
        switch (v) {
        case 0x14:
        case 0x1E:
        case 0x28:
        case 0x3C:
            MenuSubpageCursor = 7;
            break;
        case 0x32:
        case 0x5A:
            ret = 0;
            break;
        }
        break;
    case 2:
        MenuSubpageCursor = 3;
        break;
    case 3:
        for (i = 0x63; i >= 0; i--) {
        }
        r = CardReadFileAt(CardActivePort, 0, CardTargetFileName, SaveBuffer + RR_SAVE_SLOT_SCAN_BUFFER_OFFSET, sizeof(CardSlotScanWindow100), RR_SAVE_SLOT_SCAN_FILE_OFFSET);
        if (r == -1) {
            if (MenuSubpageScroll < 3) {
                MenuSubpageScroll = (unsigned short)MenuSubpageScroll + 1;
            } else {
                MenuSubpageCursor = 7;
            }
        } else {
            MenuSubpageCursor = 4;
        }
        break;
    case 4:
        for (i = 0x63; i >= 0; i--) {
        }
        q = GetHwCardEvent();
        if (q != 0) {
            if (q == 1) {
                MenuSubpageCursor = 5;
            } else if (MenuSubpageScroll >= 3) {
                MenuSubpageCursor = 7;
            } else {
                MenuSubpageCursor = 2;
                MenuSubpageScroll = (unsigned short)MenuSubpageScroll + 1;
            }
        }
        break;
    case 5:
        ret = 1;
        break;
    case 6:
    case 7:
    default:
        ret = -1;
        break;
    }
    return ret;
}


int LoadSaveFromCardStep(int a0) {
    int ret;
    int i;
    int v;
    char *p;
    if (a0 == 0) {
        MenuConfirmDialogState = 0;
    }
    ret = 0;
    switch (MenuConfirmDialogState) {
    case 0:
        CardPortPollStatus[1] = 0x5A;
        CardPortPollStatus[0] = 0x5A;
        PollCardStatus(0, 0, 1);
        i = 0x17;
        p = CardTargetFileName + i;
        do {
            *p = 0;
            i--;
            p--;
        } while (i >= 0);
        CopyStringClearTail(CardTargetFileName, CardSaveFileName);
        CardTransferMode = 0x30;
        MenuConfirmDialogState = 1;
        break;
    case 1:
        PollCardStatus(0, 0, 0);
        if (CardPortPollStatus[0] < 0x10U) {
            if (CardHasFile(CardFileCounts[0], CardTargetFileName) == 1) {
                CardLoadStep(2, 0);
                MenuConfirmDialogState = 2;
            } else {
                goto set1;
            }
            break;
        }
        v = CardPortPollStatus[0];
        switch (v) {
        case 0x14:
        case 0x1E:
        case 0x28:
        case 0x3C:
            MenuConfirmDialogState = 3;
            goto set1;
        case 0x32:
        case 0x5A:
            ret = 0;
            break;
        }
        break;
    case 2:
        ret = CardLoadStep(0, 0);
        if (ret == 4) {
            goto set1;
        } else if (ret == 5) {
            goto set1;
        } else if (ret == 6) {
            goto set1;
        } else {
            ret = 0;
        }
        break;
    case 3:
set1:
        ret = 1;
        break;
    default:
        return ret;
    }
    return ret;
}


int ReadCardSlotHeaderStep(int a0, int a1) {
    int ret;
    int v;
    if (a0 == 0) {
        MenuTransitionTimer = 0;
    }
    ret = 0;
    switch (MenuTransitionTimer) {
    default:
    case 0:
        CardPortPollStatus[1] = 0x5A;
        CardPortPollStatus[0] = 0x5A;
        PollCardStatus(0, 0, 1);
        MenuTransitionTimer = 1;
        CopyStringClearTail(CardTargetFileName, CardTargetFileNames[a1]);
        CardTransferMode = 0x10;
        break;
    case 1:
        PollCardStatus(0, 0, 0);
        if (CardPortPollStatus[0] < 0x10U) {
            if (CardHasFile(CardFileCounts[0], CardTargetFileName) == 1) {
                CardLoadStep(2, 0);
                MenuTransitionTimer = 2;
            } else {
                ret = -1;
            }
            break;
        }
        v = CardPortPollStatus[0];
        switch (v) {
        case 0x14:
        case 0x1E:
        case 0x28:
        case 0x3C:
            MenuTransitionTimer = 3;
            ret = -2;
            break;
        case 0x32:
        case 0x5A:
            ret = 0;
            break;
        }
        break;
    case 2:
        ret = CardLoadStep(0, 0);
        if (ret == 4) {
            MenuTransitionTimer = ret;
            ret = 2;
        } else if (ret == 5) {
            ret = -1;
        } else if (ret == 6) {
            ret = -1;
        } else {
            ret = 0;
        }
        break;
    case 3:
        ret = -1;
        break;
    case 4:
        ret = 3;
        break;
    }
    return ret;
}


void DrawCardMenuStatusText(int a0, int a1, int a2, int a3, int a4, int a5) {
    unsigned char *v1;
    unsigned char *end;
    int s0 = a1;
    if (a2 != 0) {
        int c14 = 0x14, c16 = 0x16, c4 = 4, c1 = 1;
        v1 = GaragePreviewCamPresets;
        end = v1 + c14;
        do {
            if (a5 == v1[1]) {
                if (a5 != c14) {
                    if (a0 != c16 || s0 != c4 || a2 != c1) s0 = 0;
                    a0 = v1[0];
                }
                break;
            }
            v1 += 4;
        } while (v1 < end);
        if (a0 == 5) {
            if (CardPdaModeFlag == 1) {
                a0 = 0x1A;
            } else if (CardMenuOperation == 2 && CardActivePort == 1) {
                a0 = 0x21;
            }
        }
    }
    if (BadgeRows->state10 == 3) {
        if (a0 == -1) {
            if (CardMenuOperation == 3 || CardMenuOperation == 6 || CardMenuOperation == 7) a0 = 0x19;
        } else if (a0 == -128) {
            a0 = CardDialogueStatePtr->b4;
        } else {
            if (a0 == 0x12 && CardPdaModeFlag == 1 && CardPortUnformatted[CardActivePort] == 1) {
                a0 = 0x1C;
            } else if (a0 == 0x15 && CardActivePort == 1) {
                a0 = 0x22;
            } else if (a0 == 0x25 && CardActivePort == 1) {
                a0 = 0x26;
            } else if (a0 == 0x1F && CardMainSaveExists == 0) {
                a0 = 0x20;
            }
        }
        if (a0 >= 0) {
            CardDialogueStatePtr->b4 = a0;
            UpdateDialogue(BadgeRows->x08 + 10, BadgeRows->y0A + 0x28, CardDialogueStatePtr);
        }
    }
    UpdateCardSlotUiRows(a4, ((s0 & 2) > 0) & (CardDialogueStatePtr->s12 >= 6));
    DrawCardMenuCommandRows(a4, (s0 & 4) > 0);
    if (s0 & 8) DrawSaveSlotTableHeader(GpSaveSlotCursor);
}

/* Initial values of the globals this module owns. */
MenuStateEntry8 MenuStateTable[68] = {{.textId00 = 4, .kind02 = 22, .handlerIdx03 = 7, .defaultChoice07 = 3}, {.textId00 = 1, .kind02 = -1, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 1, .handlerIdx03 = 1, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 1, .handlerIdx03 = 2, .flags05 = 3}, {.textId00 = 2, .kind02 = 13, .handlerIdx03 = 1, .tableIdx04 = 1, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 3, .handlerIdx03 = 3}, {.kind02 = 11, .handlerIdx03 = 4}, {.kind02 = 15, .handlerIdx03 = 4}, {.textId00 = 2, .kind02 = 18, .handlerIdx03 = 1, .tableIdx04 = 2, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 17, .handlerIdx03 = 5, .flags05 = 3}, {.kind02 = 17, .handlerIdx03 = 5, .tableIdx04 = 1, .flags05 = 3}, {.kind02 = 17, .handlerIdx03 = 6}, {.kind02 = 24, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 20, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 19, .handlerIdx03 = 4, .flags05 = 3}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 1, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 2, .handlerIdx03 = 1, .tableIdx04 = 3, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 2, .handlerIdx03 = 2, .tableIdx04 = 1, .flags05 = 3}, {.kind02 = 21, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 4, .handlerIdx03 = 3, .tableIdx04 = 1}, {.kind02 = 12, .handlerIdx03 = 4}, {.kind02 = 16, .handlerIdx03 = 4}, {.kind02 = 10, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 14, .handlerIdx03 = 0xC}, {.textId00 = 4, .kind02 = 22, .handlerIdx03 = 7, .tableIdx04 = 2, .flags05 = 1}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 2, .flags05 = 1, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 1, .handlerIdx03 = 1, .tableIdx04 = 4, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 1, .handlerIdx03 = 2, .tableIdx04 = 2, .flags05 = 3}, {.textId00 = 2, .kind02 = 13, .handlerIdx03 = 1, .tableIdx04 = 5, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 3, .handlerIdx03 = 3, .tableIdx04 = 2}, {.kind02 = 11, .handlerIdx03 = 4, .flags05 = 1}, {.kind02 = 15, .handlerIdx03 = 4, .flags05 = 1}, {.textId00 = 4, .kind02 = 22, .handlerIdx03 = 7, .tableIdx04 = 3}, {.textId00 = 4, .kind02 = 22, .handlerIdx03 = 7, .tableIdx04 = 4}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 3, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 37, .handlerIdx03 = 1, .tableIdx04 = 6, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 37, .handlerIdx03 = 2, .tableIdx04 = 3, .flags05 = 3}, {.kind02 = 21, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 4, .handlerIdx03 = 3, .tableIdx04 = 3}, {.kind02 = 12, .handlerIdx03 = 4}, {.kind02 = 16, .handlerIdx03 = 4}, {.kind02 = 14, .handlerIdx03 = 9}, {.textId00 = 4, .kind02 = 22, .handlerIdx03 = 7, .tableIdx04 = 5, .flags05 = 1, .defaultChoice07 = 2}, {.textId00 = 4, .kind02 = 22, .handlerIdx03 = 7, .tableIdx04 = 6, .flags05 = 1}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 4, .flags05 = 3, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 35, .handlerIdx03 = 1, .tableIdx04 = 7, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 35, .handlerIdx03 = 2, .tableIdx04 = 4, .flags05 = 3}, {.textId00 = 2, .kind02 = 13, .handlerIdx03 = 1, .tableIdx04 = 8, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 3, .handlerIdx03 = 3, .tableIdx04 = 4}, {.kind02 = 11, .handlerIdx03 = 4, .flags05 = 1}, {.kind02 = 15, .handlerIdx03 = 4, .flags05 = 1}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 5, .flags05 = 3, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 36, .handlerIdx03 = 1, .tableIdx04 = 9, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 36, .handlerIdx03 = 2, .tableIdx04 = 5, .flags05 = 3}, {.kind02 = 23, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 4, .handlerIdx03 = 3, .tableIdx04 = 5}, {.kind02 = 12, .handlerIdx03 = 4, .flags05 = 1}, {.kind02 = 16, .handlerIdx03 = 4, .flags05 = 1}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 6, .promptIdx06 = 1}, {.kind02 = -128, .handlerIdx03 = 0xB, .tableIdx04 = 1, .flags05 = 3}, {.textId00 = 2, .kind02 = 29, .handlerIdx03 = 1, .tableIdx04 = 0xA, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 29, .handlerIdx03 = 2, .tableIdx04 = 6, .flags05 = 3}, {.textId00 = 2, .kind02 = 13, .handlerIdx03 = 1, .tableIdx04 = 0xB, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 30, .handlerIdx03 = 3, .tableIdx04 = 6}, {.kind02 = 31, .handlerIdx03 = 4}, {.kind02 = 39, .handlerIdx03 = 4}, {.kind02 = 27, .handlerIdx03 = 4, .flags05 = 3}, {.handlerIdx03 = 8}};
/* The Japanese release's values (SLPS-01800). */
const MenuStateEntry8 MenuStateTableJp[68] = {{.textId00 = 4, .kind02 = 0x16, .handlerIdx03 = 7, .defaultChoice07 = 3}, {.textId00 = 1, .kind02 = -1, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 1, .handlerIdx03 = 1, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 1, .handlerIdx03 = 2, .flags05 = 3}, {.textId00 = 2, .kind02 = 0xD, .handlerIdx03 = 1, .tableIdx04 = 1, .flags05 = 3, .promptIdx06 = 2}, {.kind02 = 3, .handlerIdx03 = 3}, {.kind02 = 0xB, .handlerIdx03 = 4}, {.kind02 = 0xF, .handlerIdx03 = 4}, {.textId00 = 2, .kind02 = 0x12, .handlerIdx03 = 1, .tableIdx04 = 2, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 0x11, .handlerIdx03 = 5, .flags05 = 3}, {.kind02 = 0x11, .handlerIdx03 = 5, .tableIdx04 = 1, .flags05 = 3}, {.kind02 = 0x11, .handlerIdx03 = 6}, {.kind02 = 0x18, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 0x14, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 0x13, .handlerIdx03 = 4, .flags05 = 3}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 1, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 2, .handlerIdx03 = 1, .tableIdx04 = 3, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 2, .handlerIdx03 = 2, .tableIdx04 = 1, .flags05 = 3}, {.kind02 = 0x15, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 4, .handlerIdx03 = 3, .tableIdx04 = 1}, {.kind02 = 0xC, .handlerIdx03 = 4}, {.kind02 = 0x10, .handlerIdx03 = 4}, {.kind02 = 0xA, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 0xE, .handlerIdx03 = 0xC}, {.textId00 = 4, .kind02 = 0x16, .handlerIdx03 = 7, .tableIdx04 = 2, .flags05 = 1}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 2, .flags05 = 1, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 1, .handlerIdx03 = 1, .tableIdx04 = 4, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 1, .handlerIdx03 = 2, .tableIdx04 = 2, .flags05 = 3}, {.textId00 = 2, .kind02 = 0xD, .handlerIdx03 = 1, .tableIdx04 = 5, .flags05 = 3, .promptIdx06 = 2}, {.kind02 = 3, .handlerIdx03 = 3, .tableIdx04 = 2}, {.kind02 = 0xB, .handlerIdx03 = 4, .flags05 = 1}, {.kind02 = 0xF, .handlerIdx03 = 4, .flags05 = 1}, {.textId00 = 4, .kind02 = 0x16, .handlerIdx03 = 7, .tableIdx04 = 3}, {.textId00 = 4, .kind02 = 0x16, .handlerIdx03 = 7, .tableIdx04 = 4}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 3, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 0x25, .handlerIdx03 = 1, .tableIdx04 = 6, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 0x25, .handlerIdx03 = 2, .tableIdx04 = 3, .flags05 = 3}, {.kind02 = 0x15, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 4, .handlerIdx03 = 3, .tableIdx04 = 3}, {.kind02 = 0xC, .handlerIdx03 = 4}, {.kind02 = 0x10, .handlerIdx03 = 4}, {.kind02 = 0xE, .handlerIdx03 = 9}, {.textId00 = 4, .kind02 = 0x16, .handlerIdx03 = 7, .tableIdx04 = 5, .flags05 = 1, .defaultChoice07 = 2}, {.textId00 = 4, .kind02 = 0x16, .handlerIdx03 = 7, .tableIdx04 = 6, .flags05 = 1}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 4, .flags05 = 3, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 0x23, .handlerIdx03 = 1, .tableIdx04 = 7, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 0x23, .handlerIdx03 = 2, .tableIdx04 = 4, .flags05 = 3}, {.textId00 = 2, .kind02 = 0xD, .handlerIdx03 = 1, .tableIdx04 = 8, .flags05 = 3, .promptIdx06 = 2}, {.kind02 = 3, .handlerIdx03 = 3, .tableIdx04 = 4}, {.kind02 = 0xB, .handlerIdx03 = 4, .flags05 = 1}, {.kind02 = 0xF, .handlerIdx03 = 4, .flags05 = 1}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 5, .flags05 = 3, .promptIdx06 = 1}, {.textId00 = 2, .kind02 = 0x24, .handlerIdx03 = 1, .tableIdx04 = 9, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 0x24, .handlerIdx03 = 2, .tableIdx04 = 5, .flags05 = 3}, {.kind02 = 0x17, .handlerIdx03 = 4, .flags05 = 3}, {.kind02 = 4, .handlerIdx03 = 3, .tableIdx04 = 5}, {.kind02 = 0xC, .handlerIdx03 = 4, .flags05 = 1}, {.kind02 = 0x10, .handlerIdx03 = 4, .flags05 = 1}, {.textId00 = 1, .kind02 = -1, .tableIdx04 = 6, .promptIdx06 = 1}, {.kind02 = -0x80, .handlerIdx03 = 0xB, .tableIdx04 = 1, .flags05 = 3}, {.textId00 = 2, .kind02 = 0x1D, .handlerIdx03 = 1, .tableIdx04 = 0xA, .flags05 = 3, .promptIdx06 = 2, .defaultChoice07 = 1}, {.kind02 = 0x1D, .handlerIdx03 = 2, .tableIdx04 = 6, .flags05 = 3}, {.textId00 = 2, .kind02 = 0xD, .handlerIdx03 = 1, .tableIdx04 = 0xB, .flags05 = 3, .promptIdx06 = 2}, {.kind02 = 0x1E, .handlerIdx03 = 3, .tableIdx04 = 6}, {.kind02 = 0x1F, .handlerIdx03 = 4}, {.kind02 = 0x27, .handlerIdx03 = 4}, {.kind02 = 0x1B, .handlerIdx03 = 4, .flags05 = 3}, {.handlerIdx03 = 8}};
unsigned char GaragePreviewCamPresets[20] = {5, 0x1E, 0, 0, 7, 0x14, 0, 0, 6, 0x28, 0, 0, 0xE, 0x5A, 0, 0, 6, 0x3C};
MenuChoice TeamSelectBannerUvs[7] = {{.a = 6, .b = 7, .c = 1, .d = 1}, {.a = 0x14, .b = 0x15, .c = 0xD, .d = 1}, {.a = 0x1E, .b = 0x1F, .c = 1, .d = 1}, {.a = 0x27, .b = 0x28, .c = 0xD, .d = 1}, {.a = 0x31, .b = 0x32, .c = 1, .d = 1}, {.a = 0x38, .b = 0x39, .c = 0xD, .d = 1}, {.a = 0x40, .b = 0x41, .c = 1, .d = 2}};
s32 MenuCursorBlinkColors[1];
s16 MenuPanelBorderColors[6] = {10, 0, 11, 0, 2};
MenuChoice MenuDividerLineColors[1] = {{.a = 0xC, .b = 0xE, .c = 0xD}};
unsigned char MenuHighlightBarColors[7][4] = {{1, 0x8F, 0x3A, 0x43}, {0xF, 0, 0xFF}, {0x19, 0xC3, 0xFF, 1}, {1, 0xC3, 0xFF, 0xFF}, {0x22, 0xC3, 0xFF, 0xFF}, {0x2C, 0xB3, 0x43, 0xFF}, {0x2C, 0xC3, 0xFF, 0xFF}};
s16 MenuSlideStepVectors[2][2] = {{16}, {60, 66}};
short MenuSubpageCursor;
short MenuSubpageScroll;
int MenuConfirmDialogState;
int MenuTransitionTimer;
s16 MenuStateIndex;
unsigned short CardMenuReturnModeId;
u8 CardPortPollStatus[2];
short ModeIndex;
unsigned short ModeGroup;
char CardTargetFileName[24];
s16 CardActivePort;
short CardMenuOperation;
short CardPromptChoices[4];
s16 CardReplayLoadedFlag;
short CardPromptIndex;
short CardPdaModeFlag;
unsigned char CardPortUnformatted[4];
int MenuSaveBannerTimer;
