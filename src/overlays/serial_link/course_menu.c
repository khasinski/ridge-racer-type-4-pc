#include "rr/projection_state.h"
#include "game/data/menuvars.h"
#include "rr/player_cars.h"
#include "scratchpad.h"
#include "rr/load_cursor.h"
#include "game/system/pad.h"
#include "rr/overlay667_course_menu.h"
#include "rr/frame_storage_types.h"

extern s16 LinkRaceAbortFlag;
extern s32 ActivePlayerIndex;

void ovl667_select_scene(void) { ActiveSceneId = 315; }

#ifndef RR_GPU_PROJECTION_SCRATCH
#define RR_GPU_PROJECTION_SCRATCH ((ProjectionScratchPrefix *)SCRATCHPAD)
#endif

void ovl667_init_course_menu(void)
{
    s32 choice;
    ModeFrame=0;
    ScreenFrame=0;
    MenuCursor=0;
    MenuItemCount=3;
    LinkRaceAbortFlag=0;
    choice=DecodeCourseIndex(SelectedCourseChoice);
    CourseIndex=choice;
    ResetScreenCounters();
    RR_GPU_PROJECTION_SCRATCH->pose08.angles10.xyz.x=512;
    RR_GPU_PROJECTION_SCRATCH->pose08.position00.words[1]=-416;
    RR_GPU_PROJECTION_SCRATCH->pose08.angles10.xyz.y=0;
    RR_GPU_PROJECTION_SCRATCH->pose08.angles10.xyz.z=0;
    RR_GPU_PROJECTION_SCRATCH->pose08.position00.words[0]=0;
    RR_GPU_PROJECTION_SCRATCH->pose08.position00.words[2]=0;
    ComCars[0].position20.words[0]=0;
    ComCars[0].position20.words[1]=0;
    ComCars[0].position20.words[2]=0;
    ActivePlayerIndex=0;
    LinkMenuSyncPhase=0;
    LoadCursor=MenuArenas.second;
}

void ovl667_update_course_menu(void) {
    s32 previewIndex;
    s32 waitResult;
    s32 backIndex;
    s32 close;
    s32 row;
    s32 cellCount;
    u16 previousCourse;
    u8 activeState;
    s32 detailState;
    UiRow24 *preview;
    UiRow24 *back;

    if (ExitOnLinkLost() != 0) {
        SeqMuteVolume();
        return;
    }
    waitResult = UpdateLinkHandshake(2);
    ModeFrame = (ModeFrame + 1U);
    ScreenFrame = (ScreenFrame + 1U);
    if ((SubmodeState == 3) || (LoadTrackNodesStep() == 1)) {
        switch (SubmodeState) {                       /* irregular */
        case 0:
            SetMenuFrameOptions(1, 1, 1);
            previousCourse = SelectedCourseChoice;
            StepMenuValue(3, 2, (char *)&SelectedCourseChoice, LinkAvailableCourseCount, 0);
            if ((s16) previousCourse != (s16) SelectedCourseChoice) {
                ResetScreenCounters();
            } else {
                DrawCourseMapPreview((s16) previousCourse, 2);
            }
            CourseIndex = DecodeCourseIndex((s16) SelectedCourseChoice);
            UiRows[MenuCursor+18].cells[0] = 0;
            StepMenuValue(0, 2, (char *)&MenuCursor, 3, 0);
            UiRows[MenuCursor+18].cells[0] = 2;
            row = 0;
            if (UiRows[21].byte02 != 0) {
                do {
                    if (row == (s16) SelectedCourseChoice) {
                        UiRows[21].cells[row] = 2;
                        UiRows[21].byte16 = row;
                    } else {
                        UiRows[21].cells[row] = 0;
                    }
                    cellCount=UiRows[21].byte02;
                    row += 1;
                } while (row < cellCount);
            }
            activeState = UiRows[18].byte14;
            close = 0;
            if (activeState == 2) {
                if (PlayerControllers[0].action28 != 0) {
                    PlaySfx(1);
                    UiRows[MenuCursor+18].cells[0] = 1;
                    if ((s16) MenuCursor == 0) {
                        row = 0;
                        previewIndex = 0;
                        do {
                            preview = &UiRows[previewIndex + 18];
                            if (preview->byte14 == 2) {
                                preview->byte14 = 7U;
                            }
                            row += 1;
                            previewIndex++;
                        } while (row < 4);
                        ResetScreenCounters();
                        FadeCountdown = 0x40;
                        LoadCursor = MenuArenas.first;
                        SeqFadeVolume(0, 0x3C);
                        BeginLinkSync();
                        SubmodeState = 3;
                        SetMenuFrameOptions(0, 1, 1);
                    } else if ((s16) MenuCursor == 1) {
                        UiRows[23].byte14 = 0U;
                        SubmodeState = 4;
                    } else if ((s16) MenuCursor == activeState) {
                        close = 1;
                    }
                } else if (PlayerControllers[0].action2C != 0) {
                    PlaySfx(2);
                    close = 1;
                }
                if (close == 1) {
                    SetMenuFrameOptions(0, 1, 1);
                    row = 0;
                    backIndex = 0;
                    do {
                        back = &UiRows[backIndex + 18];
                        if (back->byte14 == 2) {
                            back->byte14 = 3U;
                        }
                        row += 1;
                        backIndex++;
                    } while (row < 5);
                    SubmodeState = 8;
                }
            }
            break;
        case 3:
            DrawCourseMapPreview((s16) SelectedCourseChoice, 2);
            if ((waitResult == 0) && (FadeCountdown == 0x40)) {
                DrawUiRow(0x18);
            } else {
                LinkMenuSyncPhase = 1;
                if (FadeCountdown > 0) {
                    FadeCountdown = (s16)((u16) FadeCountdown - 1);
                }
                AddScreenFadeTiles(CurrentDrawBuf->buffer.ordering70.tags, (0x40 - FadeCountdown) * 4, 0x45);
                if (FadeCountdown <= 0) {
                    LinkPrevCarRanks[0] = -1;
                    SeqMuteVolume();
                    ModeGroup = 1;
                    ModeIndex = 0x29;
                    SubmodeState = 0;
                    LinkState = 0;
                    _sio_control(3, 0, 0);
                }
            }
            break;
        case 4:
            detailState = UiRows[23].byte14;
            if (detailState == 2) {
                SetMenuFrameOptions(1, 1, 0);
                UiRows[23].cells[0] = detailState;
                StepMenuValue(1, 4, (char *)&NumLaps, 0xA, detailState);
                if (MenuReturnSubmodeIndex != 0) {
                    PlaySfx(1);
                    UiRows[23].cells[0] = 1U;
                    UiRows[23].byte14 = 3U;
                }
            } else if (detailState == 5) {
                SubmodeState = 0;
            }
            DrawCourseMapPreview((s16) SelectedCourseChoice, 2);
            break;
        case 8:
            if (UiRows[18].byte14 == 5) {
                ovl667_init_car_selection((1 - LinkHostFlag) * 2);
                ModeGroup = 1;
                ModeIndex = 0x3C;
                SubmodeState = 0;
            }
            break;
        }
    }
    DrawCourseInfoPanel((s16) SelectedCourseChoice, 2);
    row = 0;
    do {
        DrawUiRow(row + 0x12);
        row += 1;
    } while (row < 4);
    if (UiRows[18].byte14 == 2 && SubmodeState != 4) {
        DrawMenuStepArrows(0xCE, 0xD0, 0x5E, 0, (s16) SelectedCourseChoice, -1, (LinkAvailableCourseCount + 1U));
    }
    if (SubmodeState == 4) {
        DrawUiRow(0x17);
    }
    MenuFrameState.titlePlayer08 = 0;
    UpdateScreenFade(0x10026);
}

/* Initial values of the globals this module owns. */
s16 SelectedCourseChoice;
s16 LinkRaceAbortFlag;
s32 LinkAvailableCourseCount;
s16 LinkPrevCarRanks[4];
