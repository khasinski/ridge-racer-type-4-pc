#include <string.h>
#include "rr/release.h"
#include "game/ui/uipart1.h"
#include "game/data/cardchk.h"
#include "game/menu/loadmenu.h"
#include "game/menu/savemenu.h"
#include "game/data/cardini.h"
#include "game/menu/menumode.h"
#include "game/data/menuvars.h"
#include "game/system/pad.h"
#include "rr/ui.h"
#include "rr/menu_choice.h"
#include "rr/string_tail_copy.h"

extern short CardMenuOperation;
extern short CardActivePort;
extern short MenuActiveControllerIndex;
extern short MenuStateIndex;
extern short GpSaveSlotCursor;
extern short CardTransferMode;
extern char CardTargetFileName[];
extern char CardFileNameTemplate[];
extern char CardSaveFileName[];
extern u8 D_800115A4[];
extern unsigned char TeamSelectLogoPolys[][4];
extern unsigned char CardPortPollStatus[];
extern short CardPromptChoices[];
extern short CardPromptIndex;
extern unsigned char MenuCardStepRows[][4];
extern unsigned char CardFileCounts[];

void StepCardFileSlotSelection(void);
void StepCardPortSelection();
void UpdateCardMenuRowStates();

void StepCardFileSlotSelection(void) {
    int s3;
    int i;
    i = MenuStateTable[MenuStateIndex].tableIdx04;
    s3 = TeamSelectLogoPolys[i][1];
    switch (CardMenuOperation) {
    case 3:
    case 6:
    case 7:
        StepMenuValue((MenuActiveControllerIndex & 1) << 2, 2, (char *)&GpSaveSlotCursor, 3, 0);
        if (PlayerControllers[MenuActiveControllerIndex].action28 != 0) {
            PlaySfx(1);
            if ((unsigned short)(CardMenuOperation - 6) < 2) {
                CardTransferMode = 0x11;
                CopyStringClearTail(CardTargetFileName, CardTargetFileNames[GpSaveSlotCursor]);
            } else {
                CopyStringClearTail(CardTargetFileName, CardSaveFileName);
            }
        } else {
            if (PlayerControllers[MenuActiveControllerIndex].action2C != 0) {
                MenuStateIndex = 0;
            }
            return;
        }
        break;
    default:
        if (CardMenuOperation == 0) {
            if (MenuStateIndex == 0x3A) {
                CardPdaModeFlag = 1;
                CardTransferMode = 0x20;
                CopyStringClearTail(CardTargetFileName, CardFileNameTemplate);
            } else {
                CopyStringClearTail(CardTargetFileName, CardSaveFileName);
            }
        } else {
            CopyStringClearTail(CardTargetFileName, CardSaveFileName);
        }
        break;
    }
    MenuStateIndex = s3;
    PollCardStatus(CardActivePort, 0, 1);
}

void StepCardPortSelection(void) {
    int a, s4, s3;
    unsigned int idx;
    unsigned char *q;
    unsigned char *r;
    ControllerState5C *r2;
    ControllerState5C *b3;
    unsigned char *b2;
    int t, c;
    MenuStateEntry8 *base;
    MenuStateEntry8 *p;
    signed char sb;

    base = RR_RELEASE_JP ? (MenuStateEntry8 *)MenuStateTableJp : MenuStateTable;
    p = base + MenuStateIndex;
    idx = p->tableIdx04;
    c = p->promptIdx06;
    CardPromptIndex = c;
    if (SubmodeState == 0) {
        sb = p->defaultChoice07;
        if (sb >= 0) {
            CardPromptChoices[c] = sb;
        }
        SubmodeState = 1;
    }
    b2 = CardPortPollStatus;
    r = b2 + CardActivePort;
    q = &CardPollTransitionTable[idx * 4];
    a = q[0];
    s4 = q[1];
    s3 = q[2];
    if (*r < 0x10 || a == *r) {
        if (UiRows[0x26].byte14 == 2) {
            StepMenuValue(((unsigned short)MenuActiveControllerIndex & 1) << 2, 2, (char *)&CardPromptChoices[CardPromptIndex], 2, 0);
            b3 = PlayerControllers;
            r2 = b3 + MenuActiveControllerIndex;
            if (r2->action28 != 0) {
                PlaySfx(1);
                if (CardPromptChoices[CardPromptIndex] == 0) {
                    MenuStateIndex = s4;
                } else {
                    MenuStateIndex = s3;
                }
                SubmodeState = 0;
            } else if (r2->action2C != 0) {
                PlaySfx(2);
                MenuStateIndex = s3;
                SubmodeState = 0;
            }
        }
    } else {
        if (*r == 0x32) return;
        if (*r != 0x14) return;
        t = MenuStateIndex;
        if (t == 2 || t == 0x1A || t == 0x2D || t == 0x3C) {
            SavedMenuStateIndex = MenuStateIndex;
            MenuStateIndex = 8;
        } else if (t == 0x10 || t == 0x23) {
            MenuStateIndex = 0x12;
        } else if (t == 0x34) {
            MenuStateIndex = 0x36;
        }
        SubmodeState = 0;
    }
}


void UpdateCardMenuRowStates(void) {
    unsigned char *e;
    int s2, s4, s0, s3;
    int a0;
    unsigned char (*tab)[4];
    int i = MenuStateTable[MenuStateIndex].tableIdx04;
    tab = MenuCardStepRows;
    e = tab[i];
    s2 = e[0];
    s4 = e[1];
    s0 = e[2];
    s3 = e[3];
    if (CardMenuOperation == 0) {
        if (MenuStateIndex == 0x3D) {
            if (CardHasFile(CardFileCounts[CardActivePort], CardSaveFileName) != 0) {
                CardMainSaveExists = 1;
            } else {
                CardMainSaveExists = 0;
            }
        }
    }
    if (CardHasFile(CardFileCounts[CardActivePort], CardTargetFileName) == 1) {
        MenuStateIndex = s2;
        if (s0 == 1) {
            a0 = 2;
            goto cf0;
        }
        CardLoadStep(2, 0);
        return;
    }
    if (s0 == 1) {
        if (CardPortPollStatus[CardActivePort] + s3 >= 0x10) {
            MenuStateIndex = 0x16;
            return;
        }
    }
    MenuStateIndex = s4;
    if (s0 != 1) return;
    a0 = 1;
cf0:
    CardSaveStep(a0, 0);
}

/* The card files of the Japanese release are BISLPSP01800RR4_GAME and so
 * on (the PAL release's BESCESP01706...): saves do not carry over. */
void MenuModeSelectRelease(void) {
    static const char product[] = "BISLPSP01800";
    if (!RR_RELEASE_JP)
        return;
    memcpy(D_800115A4, product, 12);
    memcpy(D_800115A4 + 24, product, 12);
    memcpy(D_800115A4 + 48, product, 12);
    memcpy(CardSaveFileName, product, 12);
    memcpy(CardFileNameTemplate, product, 12);
}

/* Initial values of the globals this module owns. */
u8 D_800115A4[69] = {66, 69, 83, 67, 69, 83, 80, 48, 49, 55, 48, 54, 82, 82, 52, 77, 65, 82, 75, 51, 0, 0, 0, 0, 66, 69, 83, 67, 69, 83, 80, 48, 49, 55, 48, 54, 82, 82, 52, 77, 65, 82, 75, 50, 0, 0, 0, 0, 66, 69, 83, 67, 69, 83, 80, 48, 49, 55, 48, 54, 82, 82, 52, 77, 65, 82, 75, 49};
unsigned char TeamSelectLogoPolys[7][4] = {{0x5A, 2}, {0x5A, 0x10}, {0x5A, 0x1A}, {0x5A, 0x23}, {0x5A, 0x2D}, {0x5A, 0x34}, {0x5A, 0x3B}};
unsigned char CardPollTransitionTable[48] = {0, 3, 0, 0, 0, 5, 0, 0, 0x14, 9, 0, 0, 0, 0x11, 0, 0, 0, 0x1B, 0x18, 0, 0, 0x1D, 0x18, 0, 0, 0x24, 0x21, 0, 0, 0x2E, 0x2A, 0, 0, 0x30, 0x2A, 0, 0, 0x35, 0x2A, 0, 0, 0x3D, 0, 0, 0, 0x3F};
unsigned char MenuCardStepRows[7][4] = {{4, 5, 1, 1}, {0x13, 0x12, 0xD, 1}, {0x1C, 0x1D, 1, 1}, {0x26, 0x25, 0xD, 1}, {0x2F, 0x30, 1, 1}, {0x37, 0x36, 0xD, 1}, {0x3E, 0x3F, 1, 2}};
char CardSaveFileName[24] = {66, 69, 83, 67, 69, 83, 80, 48, 49, 55, 48, 54, 82, 82, 52, 95, 71, 65, 77, 69};
char *CardTargetFileNames[3] = {(char *)&D_800115A4[48], (char *)&D_800115A4[24], (char *)D_800115A4};
char CardFileNameTemplate[24] = {66, 69, 83, 67, 69, 83, 80, 48, 49, 55, 48, 54, 82, 82, 52, 95, 80, 68, 65};
short MenuActiveControllerIndex;
unsigned short SavedMenuStateIndex;
short CardMainSaveExists;
