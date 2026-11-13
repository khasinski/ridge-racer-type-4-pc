#include "rr/release.h"
#include "game/ui/uipart1.h"
#include "rr/runtime_settings.h"
#include "game/system/pad.h"
/* RODATA 0x800104f4 0x1c */
#include "types.h"
#include "rr/tables.h"
#include "rr/ui.h"
#include "game/system/modepad.h"
#include "game/race/racegrid.h"
#include "game/race/ranking.h"
#include "rr/audio_loop.h"
#include "game/data/ldscn1.h"
#include "game/data/globals.h"
#include "rr/controller_hud.h"
#include "rr/menu_choice.h"
#include "game/ui/uitext.h"
#include "game/demo/intro.h"
#include "rr/driver_control.h"

extern short MenuCursor;
extern short MenuScriptId;
extern int DialogueSceneMode;
extern short MenuTransitionOverlayMode;

int RunIntroScriptStep(void) {
    return IntroScriptStep();
}

void DrawModeSelectRows(void) {
    int n = ModeSelectBaseCount + IsLinkPeerPresent();
    int i = 0;
    if (n > 0) {
        do {
            /* The PAL release greys out row 2 (VS) without a second pad. */
            if (i == 2 && !RR_RELEASE_JP) {
                if (PlayerControllers[1].protocol10 == -1) UiRows[2].half20 = 1;
                else UiRows[2].half20 = 0;
            }
            DrawUiRow(i);
            i++;
        } while (i < n);
    }
}

int ModeSelectMenuStep(void)
{
    int s1, s2, i;
    int v;

    s1 = ModeSelectBaseCount + IsLinkPeerPresent();
    if (MenuCursor == ModeSelectBaseCount) {
        if (s1 == MenuCursor) {
            UiRows[s1].cells[0] = 0;
            MenuCursor = 0;
        }
    }
    UiRows[MenuCursor].cells[0] = 0;
    StepMenuValue(0, 2, (char *)&MenuCursor, s1, 0);
    /* The PAL release's language (left/right) and its greyed-out VS row. */
    if (!RR_RELEASE_JP)
        StepMenuValue(3, 4, (char *)&Options.storyBranch3C, 5, 0);
    if (!RR_RELEASE_JP && MenuCursor == 2 && MainMenuSavedCursor == -1)
        UiRows[2].cells[0] = 4;
    else
        UiRows[MenuCursor].cells[0] = 2;
    s2 = 0;
    if (MenuReturnSubmodeIndex != 0) {
        if (!RR_RELEASE_JP && UiRows[MenuCursor].half20 == 1) {
            PlaySfx(4);
            return s2;
        }
        PlaySfx(1);
        s2 = 1;
        if (MenuCursor == ModeSelectBaseCount)
            ResetLinkSioControl();
        else
            _sio_control(1, 1, 0);
        DialogueSceneMode = 2;
        MenuScriptId = 8;
        PendingModeId = 0x123;
        MainMenuChoice = MenuCursor;
        v = (short)MenuCursor;
        switch (v) {
        case 0:
            GameModeType = 1;
            break;
        case 1:
            GameModeType = 0;
            MenuTransitionOverlayMode = 0;
            break;
        case 2:
            RestoreSaveData();
            GameModeType = 2;
            MenuTransitionOverlayMode = 0;
            break;
        case 3:
        case 4:
            GameModeType = 0;
            break;
        case 5:
            DialogueSceneMode = 3;
            break;
        case 6:
            break;
        default:
            if (v == ModeSelectBaseCount) {
                RestoreSaveData();
                GameModeType = 4;
                MainMenuChoice = 8;
                PendingModeId = 0x11D;
            } else {
                MainMenuChoice = 7;
                GameModeType = 6;
            }
            break;
        }
    } else {
        if (s1 == ModeSelectBaseCount + 1) {
            if ((((u32)_sio_control(0, 0, 0) >> 7) & 3) == 2) {
                UiRows[MenuCursor].cells[0] = 0;
                s2 = 1;
                UiRows[(short)MainMenuChoice].cells[0] = 0;
                RestoreSaveData();
                MainMenuChoice = 8;
                GameModeType = 4;
                PendingModeId = 0x11D;
            }
        }
    }
    if (s2 == 1) {
        UiRows[MenuCursor].cells[0] = s2;
        for (i = 0; i < UiRowCount; i++) {
            if (UiRows[i].byte14 == 2) UiRows[i].byte14 = 3;
        }
    }
    return s2;
}

/* Initial values of the globals this module owns. */
short ModeSelectBaseCount;
short MainMenuChoice;
