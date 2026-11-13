#include "game/ui/badge1.h"
#include "rr/release.h"
#include "game/ui/uipart1.h"
#include "rr/menu_frame_state.h"
#include "game/sound/sndvab.h"
#include "rr/save_slots.h"
#include "rr/work_arena.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"
#include "types.h"
#include "scratchpad.h"
#include "game/menu/menu8.h"
#include "game/menu/menutran.h"
#include "game/menu/mnsel.h"
#include "rr/driver_control.h"
#include "rr/frame_environment.h"
#include "rr/menu_background.h"
#include "rr/selection.h"
#include "rr/bios_devices.h"
#include "game/ui/barsel0.h"
#include "game/data/screenframe.h"
#include "game/system/pad.h"
#include "rr/ui.h"
#include "rr/menu_grid.h"
#include "rr/menu_state_types.h"
#include "rr/course_choices.h"
#include "game/ui/uiwin.h"
#include "rr/screen_tiles.h"
#include "rr/badge_rows.h"
#include "game/menu/mnrows.h"

extern unsigned short FadeCountdown;
extern int VSyncThreshold;
extern short MenuCursor;
extern int ModeFrame;
extern short ModeIndex;
extern short ModeGroup;
extern unsigned short SubmodeState;
extern int DrawUiRow();
extern short CardMenuOperation;
#include "rr/menu_arena_base.h"

void GpMenuModeEnter();
void GpMenuModeUpdate(void);
void DrawFirstThreeUiRows(void);
void StepGpMenuSelection();
void CarStandingsModeEnter(void);

void GpMenuModeEnter(void) {
    int a0, a1;
    short v;
    SavedMenuSettings *p;
    SetDispMask(0);
    SetupDrawDispEnvs(0, 0, 0, 3);
    ModeFrame = 0;
    ScreenFrame = 0;
    FadeCountdown = 0;
    MenuCursor = 1;
    VSyncThreshold = 0x80;
    SelectionState.signedBytes[0x17] = -1;
    SelectionState.signedBytes[0x19] = 9;
    ResetRankEntries();
    ClearScreenOverlayState();
    a1 = 0;
    a0 = 0;
    p = SaveSlotSelections;
    do {
        a0++;
        if (p->fields.phase17 > 0) goto T;
        p++;
    } while (a0 < 3);
    J:
    if (a1 == 0) {
        UpdateGrandPrixStageProgress();
        ModeGroup = 0;
        ModeIndex = 1;
        SubmodeState = 0;
        return;
    T:
        a1 = 1;
        goto J;
    }
    InitUiRow(0, 0x6C, 0xB8, 0x44, 0x20, 0, 0, 1, 0, 0x54);
    InitUiRow(1, 0x6C, 0xE0, 0x44, 0x20, 0, 0, 1, 0, 0x67);
    InitUiRow(2, 0x6C, 0x108, 0x44, 0x20, 0, 0, 1, 0, 0x55);
    SetMenuFrameOptions(0, 1, 1);
    ModeGroup = 1;
    ModeIndex = 6;
    SubmodeState = 0;
}

void GpMenuModeUpdate(void) {
    StepGpMenuSelection();
    DrawFirstThreeUiRows();
    UpdateScreenFade(0x10086);
}

void DrawFirstThreeUiRows(void) {
    DrawUiRow(0);
    DrawUiRow(1);
    DrawUiRow(2);
}

void StepGpMenuSelection(void) {
    int s;
    int v, t;
    s = SubmodeState;
    if (s == 0) {
        UiRows[MenuCursor].cells[0] = 0;
        StepMenuValue(0, 2, (char *)&MenuCursor, 3, 0);
        UiRows[MenuCursor].cells[0] = 2;
        s = UiRows->byte14;
        if (s == 2) {
            SetMenuFrameOptions(1, 1, 1);
            if (PlayerControllers[0].action28 != 0) {
                PlaySfx(1);
                UiRows[MenuCursor].cells[0] = 1;
                v = 3;
                if (MenuCursor < 2) v = 7;
                UiRows->byte14 = v;
                UiRows[1].byte14 = v;
                UiRows[2].byte14 = v;
                FadeCountdown = 0x40;
                SubmodeState = 1;
            } else if (PlayerControllers[0].action2C != 0) {
                PlaySfx(2);
                v = 3;
                MenuCursor = s;
                UiRows->byte14 = 3;
                UiRows[1].byte14 = v;
                UiRows[2].byte14 = v;
                FadeCountdown = 0x40;
                SubmodeState = 1;
            }
        }
    } else if (s == 1) {
        SetMenuFrameOptions(0, 1, 1);
        t = FadeCountdown - 1;
        FadeCountdown = t;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x40 - (short)t) * 4, 0x45);
        if ((short)FadeCountdown <= 0) {
            switch (MenuCursor) {
            case 0:
                UpdateGrandPrixStageProgress();
                ModeGroup = 0;
                ModeIndex = s;
                SubmodeState = 0;
                break;
            case 1:
                CardMenuOperation = 4;
                ModeGroup = s;
                ModeIndex = 0xD;
                SubmodeState = 0;
                break;
            case 2:
                ModeGroup = s;
                ModeIndex = 3;
                SubmodeState = 0;
                break;
            }
        }
    }
}

void CarStandingsModeEnter(void)
{
    int *sp = (int *)SCRATCHPAD;
    MenuGrid38 *p;
    AllocMenuGridRecords(SharedWorkArena, 1);
    InitMenuGrid(0, 0, 3, 0xF0, 0xA0);
    PsyqBzero((void *)(SCRATCHPAD + 0x8), 0x20);
    p = MenuGridRecords;
    p->rot_z1C = 0x60;
    p->rot_y20 = -0x110;
    p->rot_x24 = -0x160;
    p->z14 = 0x1A;
    sp[6] = 0x400;
    UpdateGrandPrixStageProgress();
    ModeFrame = 0;
    ScreenFrame = 0;
    VSyncThreshold = 0x80;
    ResetScreenCounters();
    if (SelectionState.fields.mode1A == 6) {
        InitSelectionRows();
        ClearScreenOverlayState();
        InitMenuArenas();
        LoadCursor = MenuArenaBase;
    }
    MenuFrameState.s0 = 0;
    /* The Japanese release fills its 480 lines with one tile. */
    RR_GPU_SCRATCH->cursor =
        (RR_RELEASE_JP ? AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x1E0, 8, 8, 8)
                      : AddTileGrid2x2(&CurrentDrawBuf->buffer.ordering70.tags[0], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x200, 8, 8, 8));
    ModeGroup = 1;
    ModeIndex = 0x10;
    SubmodeState = 0;
}
