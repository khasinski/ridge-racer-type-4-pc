#include "rr/save_slots.h"
#include "rr/menu_arena_state.h"
#include "game/system/pad.h"
#include "rr/overlay659.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"

void ovl659_init_preview(void) {
    s32 index;
    s32 *scratch;

    if (SubmodeState == 0) {
        ModeFrame = 0;
        ScreenFrame = 0;
        MenuCursor = 0;
        MenuItemCount = 3;
        SeqMuteVolume();
        ResetScreenCounters();
        ClearScreenOverlayState();
        InitMenuArenas();
        LoadCursor = MenuArenas.second;
        InitUiRow(0, 0xC, 0x40, 0x44, 0x20, 0, 0, 1, 0, 0x54);
        InitUiRow(1, 0xC, 0x68, 0x44, 0x20, 0, 0, 1, 1, 0xC);
        InitUiRow(2, 0xC, 0x90, 0x44, 0x20, 0, 0, 1, 0, 0x1D);
        InitUiRow(3, 0xC, 0x170, 0xD0, 0x20, 0, 0, 8, 4, SelectionState.fields.phase17);
        InitUiRow(4, 0x74, 0x58, 0x1C, 0x20, 0, 0, 1, 0, 0x37);
        InitUiRow(5, 0x74, 0x80, 0x1C, 0x20, 0, 0, 1, 0, 0x38);
        InitUiRow(6, 0x18, 0x58, 0x44, 0x20, 0, 0, 1, 0, 0x53);
        index = 0;
        do {
            if (index == SelectionState.fields.phase17) {
                UiRows[3].cells[index] = 2;
            } else {
                UiRows[3].cells[index] = 0;
            }
            index += 1;
        } while (index < 8);
        scratch = RR_MENU_PREVIEW_SCRATCH_WORDS;
        scratch[6] = 0x200;
        scratch[7] = 0;
        scratch[8] = 0;
        scratch[2] = 0;
        scratch[3] = -0x1A0;
        scratch[4] = 0;
        PlayerCars[1].position20.words[0] = 0;
        PlayerCars[1].position20.words[1] = 0;
        PlayerCars[1].position20.words[2] = 0;
        ResetCarState(PlayerCars);
        PlayerCars[0].position20.words[1] = 0xA;
        PlayerCars[0].position20.words[2] = 0x134;
        PlayerCars[0].rot54 = 0xA00;
        SubmodeState = 1;
    } else if (SubmodeState == 1) {
        if (LoadTrackNodesStep() == 1) {
            SubmodeState = 2;
        }
    } else if (SubmodeState == 2) {
        CdAudioInit();
        CdAudioSelectTrack(0, 2);
        ModeGroup = 1;
        ModeIndex = 0x12;
        SubmodeState = 0;
    }
    UpdateScreenFade(0x1002E);
}

void ovl659_update_preview(void) {

    PreviewColor color;
    s32 renderMask;
    s32 drawFlags;
    s32 scale;
    s32 phase;
    u8 state4;
    u8 state6;
    s16 timer6;
    s16 timer3;
    s32 row;

    renderMask = 0x1002E;
    ModeFrame = ModeFrame + 1U;
    ScreenFrame = ScreenFrame + 1U;
    PlayerCars[0].rot54 = (PlayerCars[0].rot54 + 8U) & 0xFFF;
    phase = SelectionState.fields.phase17;
    switch (SubmodeState) {
    case 0:
    case 7:
        if (UiRows[0].byte14 == 2) {
            if (SubmodeState == 0) {
                CdAudioBeginPlay(1);
            }
            SubmodeState = 1;
            MenuCursor = 0;
            SetMenuFrameOptions(1, 1, 0);
        }
        break;
    case 1:
        UiRows[3].cells[SelectionState.fields.phase17] = 2;
        UiRows[MenuCursor].cells[0] = 0;
        StepMenuValue(0, 2, (char *)&MenuCursor, 3, 0);
        UiRows[MenuCursor].cells[0] = 2;
        if (PlayerControllers[0].action28 != 0) {
            PlaySfx(1);
            SetMenuFrameOptions(0, 1, 0);
            UiRows[MenuCursor].cells[0] = 1;
            if (MenuCursor == 0) {
                row = 0;

                do {
                    if (UiRows[row].byte14 == 2) {
                        UiRows[row].byte14 = 7;
                    }
                    row += 1;
                } while (row < 4);
                FadeCountdown = 0x40;
                SubmodeState = 3;
                CdAudioFade(0, 0x3C);
            } else if (MenuCursor == 1) {
                SubmodeState = 9;
                row = 0;

                do {
                    UiRows[row].byte14 = 3;
                    row += 1;
                } while (row < 4);
            } else if (MenuCursor == 2) {
                FadeCountdown = 0x40;
                row = 0;

                do {
                    UiRows[row].byte14 = 3;
                    row += 1;
                } while (row < 4);
                SubmodeState = 6;
                CdAudioFade(0, FadeCountdown - 4);
            }
        }
        break;
    case 9:
        if (UiRows[0].byte14 == 5) {
            SubmodeState = 2;
            InitPreviewImageRows();
            UiRows[6].byte14 = 0;
            goto draw_vehicle_preview;
        }
        goto draw_rows;
    case 2:
        state6 = UiRows[6].byte14;
        if (state6 == 2) {
            SetMenuFrameOptions(1, 1, 1);
            UiRows[6].cells[0] = state6;
            if (PlayerControllers[0].action28 != 0) {
                PlaySfx(1);
                UiRows[4].byte14 = 0;
                UiRows[5].byte14 = 0;
                UiRows[6].cells[0] = 1;
                SubmodeState = 4;
            } else if (PlayerControllers[0].action2C != 0) {
                PlaySfx(2);
                UiRows[6].byte14 = 3;
                ImageRows->f1 = 3;
                SubmodeState = 10;
                BadgeRows->state10 = 4;
            }
        }
        goto draw_vehicle_preview;
    case 4:
        state4 = UiRows[4].byte14;
        if (state4 == 2) {
            SetMenuFrameOptions(1, 1, 0);
            UiRows[SelectionState.fields.copies[0].bytes[3] + 4].cells[0] = 0;
            StepMenuValue(0, 1, (char *)&SelectionState.fields.copies[0].bytes[3], 2, 0);
            UiRows[SelectionState.fields.copies[0].bytes[3] + 4].cells[0] = state4;
            SelectionRows.choice = SelectionState.fields.copies[0];
            if (PlayerControllers[0].action28 != 0) {
                PlaySfx(1);
                UiRows[SelectionState.fields.copies[0].bytes[3] + 4].cells[0] = 1;
                ImageRows->f1 = 3;
                UiRows[4].byte14 = 3;
                UiRows[5].byte14 = 3;
                UiRows[6].byte14 = 3;
                SubmodeState = 5;
                BadgeRows->state10 = 4;
            }
        }
        goto draw_vehicle_preview;
    case 5:
    case 10:
        if (UiRows[6].byte14 == 5) {
            UiRows[0].byte14 = 0;
            UiRows[1].byte14 = 0;
            UiRows[2].byte14 = 0;
            UiRows[3].byte14 = 0;
            SubmodeState = 7;
        }
        goto draw_vehicle_preview;
    case 6:
        timer6 = (u16)FadeCountdown - 1;
        FadeCountdown = timer6;
        AddScreenFadeTiles(&CurrentDrawBuf->ordering.ordering70.tags[0], (0x40 - timer6) * 4, 0x45);
        if (FadeCountdown <= 0) {
            ModeGroup = 1;
            ModeIndex = 3;
            SubmodeState = 0;
        }
        goto draw_rows;
    case 3:
        timer3 = (u16)FadeCountdown - 1;
        FadeCountdown = timer3;
        AddScreenFadeTiles(&CurrentDrawBuf->ordering.ordering70.tags[0], (0x40 - timer3) * 4, 0x45);
        if (FadeCountdown <= 0) {
            SelectionRows.choice = SelectionState.fields.copies[0];
            MenuScriptId = 0;
            ModeGroup = 1;
            ModeIndex = 0x23;
            SubmodeState = 0;
        }
        break;
    }
    DrawCourseMapPreview(phase, 0);
    DrawCourseInfoPanel(phase, 0);
draw_rows:
    for (row = 0; row < 4; row++) {
        DrawUiRow(row);
    }
    goto end;
draw_vehicle_preview:
    UpdateImageRow(0);
    if (ImageRows->f1 >= 2) {
        BadgeRows->half02 = 1;
        UpdateBadgeRowAnim(0);
        DrawUiRow(6);
        if ((u16)(SubmodeState - 4) < 2U) {
            DrawUiRow(4);
            DrawUiRow(5);
        }
    }
    if (((u8)ImageRows->f1 - 1) < 2U) {
        SetupMenuCarCamera(1);
        SetupViewport(6);
        DrawSelectionIconGrid(0);
        UpdateCarWorldTransform(PlayerCars);
        if (ImageRows->f1 == 1) {
            s8 c = ~((ImageRows->f2 * 0xFF) / 30);
            color.channels.r = c;
            color.channels.g = c;
            color.channels.b = c;
            drawFlags = 0x102;
            scale = (ImageRows->f2 << 8) / 30;
        } else if (ImageRows->f1 == 2) {
            scale = 0x100;
            drawFlags = 1;
            color.channels.r = 0x80;
            color.channels.g = 0x80;
            color.channels.b = 0x80;
        }
        DrawPreviewCar(PlayerCars, color.argument, scale, drawFlags);
    }
    renderMask = 0x10022;
end:
    UpdateScreenFade(renderMask);
}
