/* RODATA 0x80011588 0x1c */
#include "types.h"
#include "rr/mode_menu_state.h"
#include "game/data/cardini.h"
#include "game/menu/menustep.h"
#include "game/data/savedat.h"
#include "game/menu/loadmenu.h"
#include "game/data/globals.h"
#include "game/ui/uidraw.h"

extern short CardSaveDialogChoice;
extern short CardSavePromptId;

void ExitCardMenuToMode(int arg0);
void CardMenuModeUpdate(void);

int CardLoadStep(short a0, int a1) {
    int i;
    int r;
    int q;
    if (a0 > 0) {
        CardSaveDialogChoice = 0;
    }
    switch (CardSaveDialogChoice) {
    case 0:
        CardSavePromptId = 0;
        CardSaveDialogChoice = 1;
        break;
    case 1:
        CardSaveDialogChoice = 2;
        break;
    case 2:
        for (i = 0x1D; i >= 0; i--) {
        }
        if (CardTransferMode != 0x10) {
            r = CardReadFile((short)a1, 0, CardTargetFileName, SaveBuffer, 1);
        } else {
            r = CardReadFileAt((short)a1, 0, CardTargetFileName, SaveBuffer, 0x80, 0x280);
        }
        if (r == -1) {
            if (CardSavePromptId < 3) {
                CardSavePromptId = (unsigned short)CardSavePromptId + 1;
                break;
            }
            CardSaveDialogChoice = 5;
        } else {
            CardSaveDialogChoice = 3;
        }
        break;
    case 3:
        q = GetHwCardEvent();
        if (q != 0) {
            if (q == 1) {
                if (ApplySaveData(CardTransferMode) != 0) {
                    CardSaveDialogChoice = 4;
                } else {
                    CardSaveDialogChoice = 6;
                }
            } else {
                if (CardSavePromptId >= 3) {
                    CardSaveDialogChoice = 5;
                } else {
                    CardSaveDialogChoice = 1;
                    CardSavePromptId = (unsigned short)CardSavePromptId + 1;
                }
            }
        }
        break;
    case 4:
    case 5:
    case 6:
        break;
    }
    return CardSaveDialogChoice;
}


void ExitCardMenuToMode(int arg0) {
    if (arg0 == -1) {
        ModeGroup = 1;
        ModeIndex = 3;
    } else {
        ModeGroup = 1;
        ModeIndex = 0xF;
    }
    SubmodeState = 0;
}


void CardMenuModeEnter(void) {
    char *clearAddress;
    u16 clockLow;
    s32 selection;
    u16 modeRange;
    s16 bank;
    s32 ready;
    s32 i;
    s32 defaultChoice;
    u16 phase;

    if (GameModeType == 4) {
        UpdateLinkHandshake(2);
        if (LinkState == -1) {
            _sio_control(1, 1, 0);
        }
    }
    phase = SubmodeState;
    switch (phase) {
    case 0:
        SetDispMask(0);
        SetupDrawDispEnvs(0x20, 0x20, 0x20, 2);
        ModeFrame = 0;
        GpSaveSlotCursor = 0;
        CardPromptChoices[CardPromptIndex] = 3;
        __builtin_memcpy(&clockLow, &ActivePlayerIndex, sizeof(clockLow));
        bank = clockLow & 1;
        MenuActiveControllerIndex = bank;
        CardActivePort = bank;
        ClearScreenOverlayState();
        i = 23;
        clearAddress = CardTargetFileName;
        clearAddress += i;
        do {
            *clearAddress = 0;
            i--;
            clearAddress--;
        } while (i >= 0);
        CopyStringClearTail(CardTargetFileName,CardSaveFileName);
        selection = CardMenuOperation;
        modeRange = (u16)CardMenuOperation;
        modeRange = (u16)(modeRange - 6);
        if (selection == 0) {
            InitUiRow(0x22, 0xC, 0x40, 0x44, 0x20, 0, 0, 1, 0, 0x66);
            InitUiRow(0x23, 0xC, 0x68, 0x44, 0x20, 0, 0, 1, 0, 0x65);
            InitUiRow(0x24, 0xC, 0x90, 0x44, 0x20, 0, 0, 1, 0, 0x64);
            InitUiRow(0x25, 0xC, 0xB8, 0x44, 0x20, 0, 0, 1, 0, 0x55);
            MenuStateIndex = 0;
            CardTransferMode = 0;
            LoadMenuStatusPromptId = 0x1001A;
            SeqStartTrack(0);
        } else if (modeRange < 2U) {
            if (selection == 6) {
                InitUiRow(0x22, 0xC, 0x40, 0x44, 0x20, 0, 0, 1, 0, 0x66);
                InitUiRow(0x23, 0xC, 0x68, 0x44, 0x20, 0, 0, 1, 0, 0x65);
                InitUiRow(0x24, 0xC, 0x90, 0x44, 0x20, 0, 0, 1, 0, 0x55);
                LoadMenuStatusPromptId = 0x1001A;
            } else {
                InitUiRow(0x22, 0xC, 0x40, 0x44, 0x20, 0, 0, 1, 0, 0x66);
                InitUiRow(0x23, 0xC, 0x68, 0x44, 0x20, 0, 0, 1, 0, 0x55);
                LoadMenuStatusPromptId = 0x10012;
            }
            CardTransferMode = 0x10;
            MenuStateIndex = 0x29;
            InitBadgeRow(1, 0x80, 0x58, 0xA8, 0x80, 0, 0xE02);
            BadgeRows[1].half02 = 1;
            GarageExtraDefaultTimes[0] = -1;
            GarageExtraDefaultTimes[1] = -1;
            GarageExtraDefaultTimes[2] = -1;
        } else {
            if (selection == 1 || selection == 3 || selection == 5) {
                InitUiRow(0x22, 0xC, 0x40, 0x44, 0x20, 0, 0, 1, 0, 0x66);
                InitUiRow(0x23, 0xC, 0x68, 0x44, 0x20, 0, 0, 1, 0, 0x55);
                LoadMenuStatusPromptId = 0x10012;
                if (CardMenuOperation == 3) {
                    MenuStateIndex = 0x17;
                } else {
                    MenuStateIndex = 0x20;
                }
                InitBadgeRow(1, 0x80, 0x58, 0xA8, 0x80, 0, 0x702);
                BadgeRows[1].half02 = 1;
                CardTransferMode = (s16) (u16) CardMenuOperation;
            } else if (selection == 2) {
                LoadMenuStatusPromptId = 0x10016;
                InitUiRow(0x22, 0xC, 0x40, 0x44, 0x20, 0, 0, 1, 0, 0x65);
                InitUiRow(0x23, 0xC, 0x68, 0x44, 0x20, 0, 0, 1, 0, 0x55);
                MenuStateIndex = 0x21;
                CardTransferMode = selection;
            } else {
                LoadMenuStatusPromptId = 0x1001A;
            }
        }
        InitUiRow(0x26, 0x80, 0xE0, 0x1C, 0x20, 0, 0, 1, 0, 0x68);
        InitUiRow(0x27, 0x80, 0x108, 0x1C, 0x20, 0, 0, 1, 0, 0x69);
        InitBadgeRow(0, 0xC, 0x130, 0x128, 0x58, 0, 0);
        BadgeRows[0].half02 = 1;
        ResetScreenCounters();
        defaultChoice = 3;
        DialogueSceneMode = 3;
        CardPromptIndex = 0;
        for (i = 2; i >= 0; i--) {
            CardPromptChoices[i] = defaultChoice;
        }
        CardReplayLoadedFlag = 0;
        for (i = 0; i < 3; i++) {
            SaveSlotSnapshots[i] = SaveSlotSelections[i];
        }
        SubmodeState = 1;
        break;
    case 1:
        ModeFrame = (ModeFrame + 1U);
        if (ModeIndex == phase) {
            SetDispMask(1);
        }
        ready = LoadMenuCarModelsStep();
        if (ready == phase) {
            SetDispMask(1);
            ModeGroup = ready;
            ModeIndex = 0x48;
            SubmodeState = 0;
        }
        break;
    }
    UpdateScreenFade(LoadMenuStatusPromptId);
    SetMenuFrameOptions(0, 1, 1);
}

void CardMenuModeUpdate(void) {
    ModeFrame += 1;
    if (GameModeType == 4) {
        UpdateLinkHandshake(2);
        if (LinkState == -1) {
            _sio_control(1, 1, 0);
        }
    }
    StepCardMenuStateMachine();
    UpdateBadgeRowAnim(0);
    if (CardMenuOperation == 3 || CardMenuOperation == 6 || CardMenuOperation == 7) {
        UpdateBadgeRowAnim(1);
    }
    UpdateScreenFade(LoadMenuStatusPromptId);
    SetMenuFrameOptions(1, 1, 1);
}


/* Initial values of the globals this module owns. */
short CardSaveDialogChoice;
short CardSavePromptId;
s32 LoadMenuStatusPromptId = 65562;
s32 GarageExtraDefaultTimes[3];
