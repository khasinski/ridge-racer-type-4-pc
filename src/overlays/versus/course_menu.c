#include "rr/overlay_module.h"
#include "game/data/menuvars.h"
#include "rr/player_cars.h"
#include "game/system/pad.h"
#include "scratchpad.h"
#include "rr/overlay661_course_menu.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"
#include "rr/menu_selection.h"
#include "rr/projection_state.h"

s32 PsyqRand(void);
#ifndef RR_GPU_PROJECTION_SCRATCH
#define RR_GPU_PROJECTION_SCRATCH ((ProjectionScratchPrefix *)SCRATCHPAD)
#endif

void ovl661_select_scene(void) { ActiveSceneId = 0x137; }

void ovl661_init_scene(void)
{
    ModeFrame=0;
    ScreenFrame=0;
    MenuCursor=0;
    MenuItemCount=4;
    PlayerCourseChoices[2].first=(PsyqRand()&3)+10;
    PlayerCourseChoices[3].first=(PsyqRand()&3)+12;
    CourseIndex=DecodeCourseIndex(SelectedCourseChoice);
    ResetScreenCounters();
    RR_GPU_PROJECTION_SCRATCH->pose08.angles10.xyz.x=512;
    RR_GPU_PROJECTION_SCRATCH->pose08.position00.words[1]=-416;
    RR_GPU_PROJECTION_SCRATCH->pose08.angles10.xyz.y=0;
    RR_GPU_PROJECTION_SCRATCH->pose08.angles10.xyz.z=0;
    RR_GPU_PROJECTION_SCRATCH->pose08.position00.words[0]=0;
    RR_GPU_PROJECTION_SCRATCH->pose08.position00.words[2]=0;
    ComCars[0].position20.words[0]=0;ComCars[0].position20.words[1]=0;ComCars[0].position20.words[2]=0;
    ActivePlayerIndex=0;
    LoadCursor=MenuArenas.second;
    SetMenuFrameOptions(0,1,1);
}

void ovl661_update_menu(void) {
    s32 var_a0_3;
    s32 var_s2;
    s32 var_s0;
    u16 temp_s0;
    u8 *temp_v1_2;
    u8 temp_a2;
    s32 temp_s0_2;
    s32 temp_s0_3;
    s32 temp_s1;
    UiRow24 *temp_v1;
    UiRow24 *temp_v1_3;

    ModeFrame=ModeFrame+1U;
    ScreenFrame=ScreenFrame+1U;
    if ((SubmodeState == 3) || (LoadTrackNodesStep() == 1)) {
        switch (SubmodeState) {
        case 0:
            SetMenuFrameOptions(1, 1, 1);
            temp_s0 = SelectedCourseChoice;
            StepMenuValue(3, 2, (char *)&SelectedCourseChoice, ovl661_available_course_count, 0);
            if ((s16) temp_s0 != (s16) SelectedCourseChoice) {
                ResetScreenCounters();
            } else {
                DrawCourseMapPreview((s16) temp_s0, 1);
            }
            CourseIndex = DecodeCourseIndex((s16) SelectedCourseChoice);
            UiRows[18+MenuCursor].cells[0] = 0;
            StepMenuValue(0, 2, (char *)&MenuCursor, MenuItemCount, 0);
            UiRows[18+MenuCursor].cells[0] = 2;
            var_s0 = 0;
            if (UiRows[22].byte02 != 0) {
                do {
                    if (var_s0 == (s16) SelectedCourseChoice) {
                        UiRows[22].cells[var_s0] = 2;
                        UiRows[22].byte16 = var_s0;
                    } else {
                        UiRows[22].cells[var_s0] = 0;
                    }
                    var_s0 += 1;
                } while (var_s0 < UiRows[22].byte02);
            }
            temp_s0_2 = UiRows[18].byte14;
            var_s2 = 0;
            if (temp_s0_2 == 2) {
                if (PlayerControllers[0].action28 != 0) {
                    PlaySfx(1);
                    UiRows[18+MenuCursor].cells[0] = 1;
                    if ((s16) MenuCursor == 0) {
                        var_s0 = 0;
                        do {
                            temp_v1 = &UiRows[var_s0 + 18];
                            if (temp_v1->byte14 == 2) {
                                temp_v1->byte14 = 7U;
                            }
                            var_s0 += 1;
                        } while (var_s0 < 4);
                        {
                            u8 *parameterTable = ovl661_difficulty_parameters;
                            SelectionRows32 *choice = &SelectionRows.selection;
                            u8 first = choice->primary;
                            u8 second = choice->variant;
                            u8 third = choice->rows[0].selection;
                            choice->rows[1].selection = first;
                            choice->rows[1].variant = second;
                        temp_v1_2 = parameterTable + (s16)ComDifficultyLevel;
                        temp_a2 = *temp_v1_2;
                        choice->rows[2].selection=third;
                        choice->rows[2].variant = (u8) choice->rows[0].variant;
                        choice->rows[1].parameter = temp_a2;
                        choice->rows[2].parameter = (u8) *temp_v1_2;
                        }
                        ResetScreenCounters();
                        FadeCountdown = 0x40;
                        LoadCursor = MenuArenas.first;
                        SeqFadeVolume(0, 0x3C);
                        SubmodeState = 3;
                        SetMenuFrameOptions(0, 1, 1);
                    } else if ((s16) MenuCursor == 1) {
                        UiRows[23].byte14 = 0U;
                        SubmodeState = 4;
                    } else if ((s16) MenuCursor == temp_s0_2) {
                        UiRows[24].byte14 = 0U;
                        UiRows[25].byte14 = 0;
                        MenuCursor = 0;
                        SubmodeState = 5;
                    } else if ((s16) MenuCursor == 3) {
                        var_s2 = 1;
                    }
                } else if (PlayerControllers[0].action2C != 0) {
                    PlaySfx(2);
                    var_s2 = 1;
                }
                if (var_s2 == 1) {
                    SetMenuFrameOptions(0, 1, 1);
                    SetModelTexturePage(-1);
                    var_s0 = 0;
                    do {
                        temp_v1_3 = &UiRows[var_s0 + 18];
                        if (temp_v1_3->byte14 == 2) {
                            temp_v1_3->byte14 = 3U;
                        }
                        var_s0 += 1;
                    } while (var_s0 < 5);
                    SubmodeState = 8;
                }
            }
        default:
            goto block_63;
        case 3:
            DrawCourseMapPreview((s16) SelectedCourseChoice, 1);
            if (FadeCountdown > 0) {
                FadeCountdown = (u16) FadeCountdown - 1;
            }
            AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x40 - FadeCountdown) * 4, 0x45);
            if (NumCars >= 3) {
                GameModeType = 3;
                if (LoadCarRangeStep(2, NumCars - 2) == 1) {
                    MenuArenas.first = LoadCursor;
                    goto block_42;
                }
            } else {
                GameModeType = 2;
block_42:
                if (FadeCountdown <= 0) {
                    SeqMuteVolume();
                    ovl661_finish_ranks[0] = -1;
                    MenuScriptId = 0;
                    ModeGroup = 1;
                    ModeIndex = 0x23;
                    SubmodeState = 0;
                }
            }
            goto block_63;
        case 4:
            temp_s0_3 = UiRows[23].byte14;
            if (temp_s0_3 == 2) {
                SetMenuFrameOptions(1, 1, 0);
                UiRows[23].cells[0] = temp_s0_3;
                StepMenuValue(1, 4, (char *)&NumLaps, 0xA, temp_s0_3);
                if (PlayerControllers[0].action28 != 0) {
                    PlaySfx(1);
                    UiRows[23].cells[0] = 1U;
                    UiRows[23].byte14 = 3U;
                }
            } else {
                if (temp_s0_3 !=5) goto block_59;
                goto block_57;
            }
            goto block_59;
        case 5:
            temp_s1 = UiRows[24].byte14;
            if (temp_s1 == 2) {
                SetMenuFrameOptions(1, 1, 0);
                UiRows[24+MenuCursor].cells[0] = 0;
                StepMenuValue(0, 2, (char *)&MenuCursor, 2, 0);
                UiRows[24+MenuCursor].cells[0] = temp_s1;
                if ((s16)MenuCursor == 0) {
                    /* The PS1 stepped NumCars as a word; its upper half stays zero. */
                    StepMenuValue(1,2,(char *)&NumCars,5,temp_s1);
                } else {
                    StepMenuValue(1,2,(char *)&ComDifficultyLevel,4,0);
                }
                if (PlayerControllers[0].action28 != 0) {
                    PlaySfx(1);
                    UiRows[24].cells[0] = 1;
                    UiRows[24].byte14 = 3U;
                    UiRows[25].cells[0] = 1;
                    UiRows[25].byte14 = 3;
                    MenuCursor = 2;
                }
            } else if (temp_s1 == 5) {
block_57:
                SubmodeState=0;
            }
block_59:
            DrawCourseMapPreview((s16) SelectedCourseChoice, 1);
            goto block_63;
        case 8:
            if (UiRows[18].byte14 == 5) {
                ovl661_init_preview(0);
                ModeGroup = 1;
                ModeIndex = 0x38;
                SubmodeState = 0;
                goto block_64;
            }
            var_s0=0;
            break;
        }
    } else {
block_63:
        DrawCourseInfoPanel((s16) SelectedCourseChoice, 1);
block_64:
        var_s0 = 0;
    }
    do {
        DrawUiRow(var_s0 + 0x12);
        var_s0 += 1;
    } while (var_s0 < 5);
    if ((UiRows[18].byte14 == 2) && ((SubmodeState - 4) >= 2U)) {
        DrawMenuStepArrows(0xCE, 0xD0, 0x5E, 0, (s16) SelectedCourseChoice, -1, ovl661_available_course_count + 1);
    }
    if (SubmodeState == 4) {
        var_a0_3 = 0x17;
        goto block_73;
    }
    if (SubmodeState == 5) {
        DrawUiRow(0x18);
        var_a0_3 = 0x19;
block_73:
        DrawUiRow(var_a0_3);
    }
    MenuFrameState.titlePlayer08 = 0;
    UpdateScreenFade(0x10026);
}

/* Initial values of the overlay globals this module owns. */
u8 ovl661_difficulty_parameters[4] = {0, 3, 6, 0xF};
s32 ovl661_available_course_count;

/* Writable state of this unit; ovl661_reset_module restores it. */
static const OverlayStateRegion ovl661_course_menu_regions[] = {
    RR_OVERLAY_STATE(ovl661_available_course_count),
    RR_OVERLAY_STATE(ovl661_difficulty_parameters)};
const OverlayStateTable ovl661_course_menu_state = {ovl661_course_menu_regions, sizeof(ovl661_course_menu_regions) / sizeof(ovl661_course_menu_regions[0])};

/* Module reset: the state a fresh load of the versus image gave
 * (initialized data and zeroed BSS); see rr/overlay_module.h. */
extern const OverlayStateTable ovl661_course_menu_state, ovl661_preview_state, ovl661_race_state, ovl661_results_state;
static const OverlayStateTable *const ovl661_state_tables[] = {&ovl661_course_menu_state, &ovl661_preview_state, &ovl661_race_state, &ovl661_results_state};
static OverlayStateImage ovl661_state_image = {ovl661_state_tables, sizeof(ovl661_state_tables) / sizeof(ovl661_state_tables[0]), NULL};

void ovl661_capture_module(void) {
    OverlayStateCapture(&ovl661_state_image);
}

void ovl661_reset_module(void) {
    OverlayStateRestore(&ovl661_state_image);
}

/* Initial values of the globals this module owns. */
s16 MenuCursor;
