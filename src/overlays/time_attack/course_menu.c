#include "rr/cpu_time.h"
#include "game/system/pad.h"
#include "rr/overlay_module.h"
#include "rr/overlay660.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"

void ovl660_select_scene(void) { ActiveSceneId = 0x131; }

void ovl660_init_menu(void) {
    ProjectionScratchPrefix *scratch;
    s32 selection;
    s16 *result;
    ModeFrame=0;
    ScreenFrame=0;
    MenuCursor=0;
    MenuItemCount=3;
    selection=DecodeCourseIndex(SelectedCourseChoice);
    result=&CourseIndex;
    *result=selection;
    ResetScreenCounters();
    MenuArenas.second=CdDirectoryBuffer+0x2D000;
    InitMenuArenas();
    LoadCursor=MenuArenas.second;
    InitUiRow(0,12,64,68,32,0,0,1,0,83);
    InitUiRow(1,12,104,68,32,0,0,1,0,102);
    InitUiRow(2,12,144,68,32,0,0,1,0,85);
    InitUiRow(3,12,368,208,32,0,0,8,4,SelectedCourseChoice&7);
    InitUiRow(4,24,88,68,32,0,0,1,0,84);
    InitUiRow(5,24,128,68,32,0,0,1,0,54);
    InitUiRow(6,24,168,68,32,0,0,1,0,85);
    InitUiRow(7,24,88,68,32,0,0,1,0,84);
    InitUiRow(8,24,128,68,32,0,0,1,0,53);
    InitUiRow(9,24,168,68,32,0,0,1,0,85);
    InitUiRow(10,116,88,28,32,0,0,1,0,55);
    InitUiRow(11,116,128,28,32,0,0,1,0,56);
    scratch=RR_PROJECTION_SCRATCH;
    scratch->pose08.angles10.xyz.x=512;
    scratch->pose08.position00.words[1]=-416;
    scratch->pose08.angles10.xyz.y=0;
    scratch->pose08.angles10.xyz.z=0;
    scratch->pose08.position00.words[0]=0;
    scratch->pose08.position00.words[2]=0;
    PlayerCars[0].position20.words[0]=0;
    PlayerCars[0].position20.words[1]=0;
    PlayerCars[0].position20.words[2]=0;
}

void ovl660_init_scene(void)
{
    s32 val;

    SetDispMask(0);
    ClearScreenOverlayState();
    ovl660_preview_bank = 0;
    ovl660_preview_selection = 0;
    ovl660_preview_confirmation = 0;
    CopyStringClearTail(ovl660_record_name, SaveProgress.prefix.progress.prefix00.fields.name);
    SetupDrawDispEnvs(0, 0, 0, 2);
    val = SaveProgress.prefix.progress.prefix00.fields.configuration0C;
    SelectedCourseChoice = 0;
    ovl660_available_course_count = 4;
    if (val >= 16) {
        ovl660_available_course_count = 8;
    }
    if (val & 0x100) {
        ovl660_available_course_count = 16;
    }
    ovl660_init_menu();
    UnpackChoiceGlyph((s8*)&SelectionRows,CourseChoiceTable[0][0].glyph);
    ModeGroup = 1;
    ModeIndex = 50;
    SubmodeState = 0;
    DebugPrintStub(ovl660_title);
    SeqStartTrack(0);
}

void ovl660_update_menu(void) {
    s32 loopCount;
    s16 cancelTimer;
    s16 settingsTimer;
    s32 row;
    s32 previewIndex;
    s32 settingsIndex;
    s32 cancelIndex;
    s32 backIndex;
    s32 cell;
    u16 previousCourse;
    u8 activeState;
    UiRow24 *preview;
    UiRow24 *settings;
    UiRow24 *cancel;
    UiRow24 *back;

    RR_CPU_CYCLES(RR_CYCLES_TA_COURSE_MENU);
    ModeFrame = (ModeFrame + 1U);
    ScreenFrame = (ScreenFrame + 1U);
    if (LoadTrackNodesStep() == 1) {
        switch (SubmodeState) {
        case 0:
            RR_CPU_CYCLES(RR_CYCLES_TA_COURSE_MENU_SELECT);
            SetMenuFrameOptions(1, 1, 1);
            previousCourse = SelectedCourseChoice;
            StepMenuValue(3, 2, (char *)&SelectedCourseChoice, ovl660_available_course_count, 0);
            if ((s16) previousCourse != (s16) SelectedCourseChoice) {
                ResetScreenCounters();
            } else {
                DrawCourseMapPreview((s16) previousCourse, 0);
            }
            CourseIndex = DecodeCourseIndex((s16) SelectedCourseChoice);
            UiRows[MenuCursor].cells[0] = 0;
            StepMenuValue(0, 2, (char *)&MenuCursor, MenuItemCount, 0);
            UiRows[MenuCursor].cells[0] = 2;
            cell = 0;
            if (UiRows[3].byte02 != 0) {
                do {
                    if (cell == (SelectedCourseChoice & 7)) {
                        UiRows[3].cells[cell] = (s8) (((u8 *)&ReverseCourse)[0] + 2);
                        UiRows[3].byte16 = cell;
                    } else if (cell >= ovl660_available_course_count) {
                        UiRows[3].cells[cell] = 1;
                    } else {
                        UiRows[3].cells[cell] = (u8) ((u8 *)&ReverseCourse)[0];
                    }
                    cell += 1;
                } while (cell < UiRows[3].byte02);
            }
            if (MenuReturnSubmodeIndex != 0) {
                activeState = UiRows[0].byte14;
                if (activeState == 2) {
                    PlaySfx(1);
                    SetMenuFrameOptions(0, 1, 1);
                    if ((s16) MenuCursor == 0) {
                        row = 0;
                        previewIndex = 0;
                        UiRows[0].cells[0] = 1;
                        do {
                            preview = (UiRow24 *)((previewIndex*sizeof(UiRow24))+(uintptr_t)UiRows);
                            if (preview->byte14 == 2) {
                                preview->byte14 = 3U;
                            }
                            row += 1;
                            previewIndex++;
                        } while (row < 4);
                        SubmodeState = 3;
                    } else if ((s16) MenuCursor == 1) {
                        row = 0;
                        if (UiRowCount > 0) {
                            settingsIndex = 0;
                            do {
                                settings = (UiRow24 *)((settingsIndex*sizeof(UiRow24))+(uintptr_t)UiRows);
                                if (settings->byte14 == 2) {
                                    settings->byte14 = 3U;
                                }
                                loopCount=UiRowCount;
                                row += 1;
                                settingsIndex++;
                            } while (row < loopCount);
                        }
                        FadeCountdown = 0x40;
                        SubmodeState = 0xA;
                    } else if ((s16) MenuCursor == activeState) {
                        row = 0;
                        if (UiRowCount > 0) {
                            cancelIndex = 0;
                            do {
                                cancel = (UiRow24 *)((cancelIndex*sizeof(UiRow24))+(uintptr_t)UiRows);
                                if (cancel->byte14 == 2) {
                                    cancel->byte14 = 3U;
                                }
                                loopCount=UiRowCount;
                                row += 1;
                                cancelIndex++;
                            } while (row < loopCount);
                        }
                        FadeCountdown = 0x40;
                        SubmodeState = 8;
                        SeqFadeVolume(0, 0x3C);
                    }
                } else {
                    goto check_back;
                }
            } else {
check_back:
                if ((CourseSelectLockedFlag != 0) && (UiRows[0].byte14 == 2)) {
                    PlaySfx(2);
                    SetMenuFrameOptions(0, 1, 1);
                    row = 0;
                    if (UiRowCount > 0) {
                        backIndex = 0;
                        do {
                            back = (UiRow24 *)((backIndex*sizeof(UiRow24))+(uintptr_t)UiRows);
                            if (back->byte14 == 2) {
                                back->byte14 = 3U;
                            }
                                loopCount=UiRowCount;
                                row += 1;
                            backIndex++;
                        } while (row < loopCount);
                    }
                    FadeCountdown = 0x40;
                    SubmodeState = 8;
                }
            default:;
            }
            goto draw_course;
        case 2:
            if (MenuReturnSubmodeIndex != 0) {
                if (UiRows[0].byte14 == 5) {
                    SubmodeState = 4;
                    InitUiRow(3, 0x78, 0xDC, 0x44, 0x20, 0, 0, 1, 0, 0, 0, 0);
                    InitUiRow(4, 0x78, 0x104, 0x44, 0x20, 0, 0, 1, 0, 0, 0, 0);
                }
            }
            goto draw_course;
        case 4:
            if (MenuReturnSubmodeIndex != 0) {
                if (UiRows[3].byte14 == 2) {
                    SubmodeState = 5;
                    ImageRows->f1 = 3;
                    UiRows[3].byte14 = 3U;
                    UiRows[4].byte14 = 3;
                }
            } else {
            case 5:;
            }
            if (UiRows[3].byte14 == 5) {
                UiRows[0].byte14 = 0U;
                UiRows[1].byte14 = 0;
                UiRows[2].byte14 = 0;
                SubmodeState = 0;
            }
            goto draw_course;
        case 3:
            if (UiRows[0].byte14 == 5) {
                ovl660_init_preview();
                ModeGroup = 1;
                ModeIndex = 0x33;
                SubmodeState=0;
            }
            break;
        case 8:
            cancelTimer = (u16)FadeCountdown - 1;
            FadeCountdown = (u16) cancelTimer;
            AddScreenFadeTiles(CurrentDrawBuf->buffer.ordering70.tags, (0x40 - cancelTimer) * 4, 0x45);
            if ((s16) FadeCountdown <= 0) {
                SeqMuteVolume();
                ModeGroup = 1;
                ModeIndex = 3;
                SubmodeState=0;
            }
            break;
        case 10:
            settingsTimer = (u16)FadeCountdown - 1;
            FadeCountdown = (u16) settingsTimer;
            AddScreenFadeTiles(CurrentDrawBuf->buffer.ordering70.tags, (0x40 - settingsTimer) * 4, 0x45);
            if ((s16) FadeCountdown <= 0) {
                CardMenuOperation = 1;
                CardMenuReturnModeId = 0x132;
                ModeGroup = 1;
                ModeIndex = 0x47;
                SubmodeState=0;
            }
            break;
        }
    } else {
draw_course:
        DrawCourseInfoPanel((s16)SelectedCourseChoice,0);
    }
    DrawUiRow(0);
    DrawUiRow(1);
    DrawUiRow(2);
    DrawUiRow(3);
    if (UiRows[0].byte14 == 2) {
        DrawMenuStepArrows(0xCE, 0xD0, 0x5E, 0, (s16) SelectedCourseChoice, -1, (ovl660_available_course_count + 1U));
    }
    UpdateScreenFade(0x10026);
}

/* Initial values of the overlay globals this module owns. */
const char ovl660_title[68] = {106, 111, 98, 95, 116, 97, 95, 99, 114, 115, 95, 105, 110, 102, 111, 48, 10, 0, 0, 0, 0, 0, 0, 0, -80, 77, 17, -128, 52, 84, 17, -128, -64, 81, 17, -128, 20, 83, 17, -128, 112, 82, 17, -128, -48, 82, 17, -128, 52, 84, 17, -128, 52, 84, 17, -128, 80, 83, 17, -128, 52, 84, 17, -128, -72, 83, 17, -128};
s32 ovl660_available_course_count;

/* Writable state of this unit; ovl660_reset_module restores it. */
static const OverlayStateRegion ovl660_course_menu_regions[] = {
    RR_OVERLAY_STATE(ovl660_available_course_count)};
const OverlayStateTable ovl660_course_menu_state = {ovl660_course_menu_regions, sizeof(ovl660_course_menu_regions) / sizeof(ovl660_course_menu_regions[0])};

/* Module reset: the state a fresh load of the time_attack image gave
 * (initialized data and zeroed BSS); see rr/overlay_module.h. */
extern const OverlayStateTable ovl660_course_menu_state, ovl660_preview_state, ovl660_results_state;
static const OverlayStateTable *const ovl660_state_tables[] = {&ovl660_course_menu_state, &ovl660_preview_state, &ovl660_results_state};
static OverlayStateImage ovl660_state_image = {ovl660_state_tables, sizeof(ovl660_state_tables) / sizeof(ovl660_state_tables[0]), NULL};

void ovl660_capture_module(void) {
    OverlayStateCapture(&ovl660_state_image);
}

void ovl660_reset_module(void) {
    OverlayStateRestore(&ovl660_state_image);
}

/* Initial values of the globals this module owns. */
s32 UiRowCount;
