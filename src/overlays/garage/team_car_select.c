#include "rr/overlay_module.h"
#include "scratchpad.h"
#include "game/data/cdfile2.h"
#include "game/menu/menuwork.h"
#include "rr/cd_directory_buffer.h"
#include "rr/diagnostics.h"
#include "rr/menu_arena_base.h"
#include "rr/overlay662_course_controller.h"
#include "rr/projection_state.h"
#include "rr/renderer_setup.h"
#include "rr/save_progress.h"
#include "rr/string_tail_copy.h"


extern s16 MenuItemCount;
extern u32 ScreenFrame;
extern s16 ovl662_selected_car_number, ovl662_selected_emblem_id;
extern char ovl662_saved_player_name[12];
extern const char ovl662_str_init_car_choice[];
void ClearScreenOverlayState(void);
void InitMenuArenas(void);
void InitPreviewImageRows(void);
void InitUiRow(s32 index, s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 count, s32 g, s32 h, ...);
void ResetCarState(RaceVehicle *vehicle);
s32 CountChoiceGlyphs(const CourseChoice4 *choices);
void ovl662_init_team_car_layout(void);
void ovl662_init_team_car_choice(void);
void ovl662_reset_team_car_preview(void);

void ovl662_init_team_car_layout(void) {
    s32 i;
    ProjectionScratchPrefix *scratch;
    void *arena;
    ClearScreenOverlayState();
    ModeFrame=0;
    ScreenFrame=0;
    MenuCursor=0;
    MenuItemCount=4;
    ResetScreenCounters();
    MenuArenas.second=CdDirectoryBuffer+0x2D000;
    InitMenuArenas();
    LoadCursor=MenuArenas.second;
    InitUiRow(0,24,88,68,32,0,0,1,0,21);
    InitUiRow(1,24,128,68,32,0,0,1,0,102);
    InitUiRow(2,24,168,68,32,0,0,1,0,22);
    InitUiRow(3,24,208,68,32,0,0,1,0,85);
    SetupRenderer(5);
    ResetScreenCounters();
    InitPreviewImageRows();
    i=0;
    ovl662_menu_substate=0;
    scratch=RR_PROJECTION_SCRATCH;
    scratch->pose08.angles10.xyz.x=120;
    scratch->pose08.position00.words[0]=-16;
    scratch->pose08.position00.words[1]=-248;
    scratch->pose08.angles10.xyz.y=0;
    scratch->pose08.angles10.xyz.z=0;
    scratch->pose08.position00.words[2]=-96;
    arena=MenuArenaBase;
    LoadCursor=arena;
    MenuArenas.first=arena;
    for (i = 0; i < 2; i++) {
        ResetCarState(&PlayerCars[i]);
        PlayerCars[i].position20.words[1] = 10;
        PlayerCars[i].position20.words[2] = 308;
        PlayerCars[i].rot54 = 2048;
    }
    UnpackChoiceGlyph((s8 *)&SelectionRows.selection,CourseChoiceTable[1][ovl662_selected_car_slot].glyph);
    PlayerCourseChoices[0].first=CourseChoiceTable[1][ovl662_selected_car_slot].first;
    PlayerCourseChoices[0].second=CourseChoiceTable[1][ovl662_selected_car_slot].second;
    ovl662_available_car_count=CountChoiceGlyphs(CourseChoiceTable[1]);
    SetMenuFrameOptions(0,1,1);
}

void ovl662_init_team_car_choice(void) {
    SetDispMask(0);
    ovl662_selected_car_slot = 0;
    CopyStringClearTail(ovl662_saved_player_name, SaveProgress.prefix.progress.prefix00.fields.name);
    ovl662_init_team_car_layout();
    UnpackChoiceGlyph((s8 *)&SelectionRows.selection, CourseChoiceTable[1][0].glyph);
    PlayerCourseChoices[0].first = CourseChoiceTable[1][ovl662_selected_car_slot].first;
    PlayerCourseChoices[0].second = CourseChoiceTable[1][ovl662_selected_car_slot].second;
    ModeGroup = 1;
    ModeIndex = 0x40;
    SubmodeState = 0;
    DebugPrintStub(ovl662_str_init_car_choice);
}

void ovl662_reset_team_car_preview(void)
{
    ResetScreenCounters();
    LoadCursor=MenuArenas.first;
    UnpackChoiceGlyph((s8 *)&SelectionRows.selection,CourseChoiceTable[1][0].glyph);
    ImageRows->f1=6;
}

void ovl662_update_team_car_select(void) {
    PreviewColor color;
    s32 var_a3;
    s32 temp_s1;
    s16 temp_v1_7;
    s16 temp_v1_8;
    s32 temp_a1;
    s16 closePhase;
    u32 temp_s0;
    s32 temp_s5;
    s32 var_a2_2;
    s32 var_s0;
    s8 var_s6;
    s32 temp_v0_2;
    s8 temp_v0_3;
    s32 var_a1;
    u8 temp_s3;
    RaceVehicle *temp_v0;
    RaceVehicle *vehicleBase;
    s32 playerIndex;
    s32 *tick;
    const CourseChoice4 *temp_v1;
    UiRow24 *temp_v1_2;
    UiRow24 *temp_v1_3;
    UiRow24 *temp_v1_4;
    UiRow24 *temp_v1_5;
    UiRow24 *temp_v1_6;

    var_s6 = 0;
    tick = &ModeFrame;
    playerIndex = ActivePlayerIndex;
    vehicleBase = PlayerCars;
    temp_v0 = &vehicleBase[playerIndex&1];
    temp_a1 = (temp_v0->rot54 + 8U) & 0xFFF;
    *tick=(u32)*tick+1;
    temp_v0->rot54 = temp_a1;
    SetupMenuCarCamera(1);
    SetupViewport(6);
    temp_v0_2 = LoadCarRangeStep(0, 1);
    if (temp_v0_2 != 1) {
        if (ImageRows->f1 == 1) {
            ImageRows->f1 = 0;
        }
        goto block_67;
    }
    SetCarBounds(vehicleBase);
    if (ImageRows->f1 == 7) {
        ImageRows->f1 = temp_v0_2;
    }
    var_s6 = 1;
    switch ((u16)SubmodeState) {
    case 0:
        SetMenuFrameOptions(1, 1, 1);
        temp_s0 = PackChoiceGlyph((s8*)&SelectionRows.selection);
        StepMenuValue(1, 2, (char*)&ovl662_selected_car_slot, ovl662_available_car_count, 0);
        UnpackChoiceGlyph((s8*)&SelectionRows.selection, CourseChoiceTable[1][ovl662_selected_car_slot].glyph);
        if ((temp_s0 << 0x10) != ((u32)PackChoiceGlyph((s8*)&SelectionRows.selection) << 0x10)) {
            ResetScreenCounters();
            LoadCursor = MenuArenas.first;
            UnpackChoiceGlyph((s8*)&SelectionRows.selection, CourseChoiceTable[1][ovl662_selected_car_slot].glyph);
            ImageRows->f1 = 6;
            temp_v1 = &CourseChoiceTable[1][ovl662_selected_car_slot];
            PlayerCourseChoices[0].first = (s16) (s8) temp_v1->first;
            PlayerCourseChoices[0].second = (s16) (s8) temp_v1->second;
        }
        UiRows[MenuCursor].cells[0] = 0;
        StepMenuValue(0, 2, (char*)&MenuCursor, 4, 0);
        var_a1 = 2;
        temp_s5 = ((SelectionRows.selection.pad02[0] % 20) & 0xFF) == 0x13;
        if (temp_s5 != 0) {
            if (ovl662_selected_car_slot == 8) {
                UiRows[0].half20 = 1;
                if (MenuCursor == 0) {
                    var_a1 = 4;
                }
            } else {
                UiRows[0].half20 = 0;
            }
            UiRows[2].half20 = 1;
            if (MenuCursor == 2) {
                var_a1 = 4;
            }
        } else {
            UiRows[0].half20 = 0;
            UiRows[2].half20 = 0;
        }
        UiRows[MenuCursor].cells[0] = var_a1;
        temp_s3 = UiRows[0].byte14;
        if (temp_s3 == 2) {
            temp_s1 = BadgeRows->state10;
            if (temp_s1 == 3) {
                if (PlayerControllers[0].action28 != 0) {
                    SetMenuFrameOptions(0, 1, 1);
                    UiRows[MenuCursor].cells[0] = 1;
                    if (MenuCursor == 0) {
                        if (ovl662_selected_car_slot < 8) {
                            SetModelTexturePage(-1);
                            PlaySfx(1);
                            var_s0 = 0;

                            SubmodeState = (u16) temp_s1;
                            BadgeRows->state10 = 4;
                            closePhase = 3;
                            ImageRows->f1 = closePhase;
                            do {
                                temp_v1_2 = (UiRow24 *)(var_s0 * sizeof(UiRow24) + (uintptr_t)UiRows);
                                if (temp_v1_2->byte14 == 2) {
                                    temp_v1_2->byte14 = 3U;
                                }
                                var_s0 += 1;

                            } while (var_s0 < 4);
                        } else {
                            goto block_40;
                        }
                    } else if (MenuCursor == 1) {
                        SetModelTexturePage(-1);
                        PlaySfx(1);
                        var_s0 = 0;

                        BadgeRows->state10 = 4;
                        ImageRows->f1 = (s8) temp_s1;
                        do {
                            temp_v1_3 = (UiRow24 *)(var_s0 * sizeof(UiRow24) + (uintptr_t)UiRows);
                            if (temp_v1_3->byte14 == 2) {
                                temp_v1_3->byte14 = 3U;
                            }
                            var_s0 += 1;

                        } while (var_s0 < 4);
                        FadeCountdown = 0x40;
                        SubmodeState = 5;
                    } else if (MenuCursor == temp_s3) {
                        if (temp_s5 == 0) {
                            SetModelTexturePage(-1);
                            PlaySfx(1);
                            var_s0 = 0;

                            SubmodeState = 4;
                            BadgeRows->state10 = 4;
                            ImageRows->f1 = (s8) temp_s1;
                            do {
                                temp_v1_4 = (UiRow24 *)(var_s0 * sizeof(UiRow24) + (uintptr_t)UiRows);
                                if (temp_v1_4->byte14 == 2) {
                                    temp_v1_4->byte14 = 3U;
                                }
                                var_s0 += 1;

                            } while (var_s0 < 4);
                        } else {
block_40:
                            PlaySfx(4);
                        }
                    } else if (MenuCursor == temp_s1) {
                        SetModelTexturePage(-1);
                        PlaySfx(1);
                        var_s0 = 0;
                        if (UiRowCount > 0) {

                            do {
                                temp_v1_5 = (UiRow24 *)(var_s0 * sizeof(UiRow24) + (uintptr_t)UiRows);
                                if (temp_v1_5->byte14 == 2) {
                                    temp_v1_5->byte14 = 3U;
                                }
                                var_s0 += 1;

                            } while (var_s0 < UiRowCount);
                        }
                        BadgeRows->state10 = 4;
                        ImageRows->f1 = 3;
                        goto block_54;
                    }
                } else if (PlayerControllers[0].action2C != 0) {
                    PlaySfx(2);
                    SetModelTexturePage(-1);
                    SetMenuFrameOptions(0, 1, 1);
                    var_s0 = 0;
                    if (UiRowCount > 0) {

                        do {
                            temp_v1_6 = (UiRow24 *)(var_s0 * sizeof(UiRow24) + (uintptr_t)UiRows);
                            if (temp_v1_6->byte14 == 2) {
                                temp_v1_6->byte14 = 3U;
                            }
                            BadgeRows->state10 = 4;
                            ImageRows->f1 = 3;
                            var_s0 += 1;

                        } while (var_s0 < UiRowCount);
                    }
block_54:
                    FadeCountdown = 0x40;
                    SubmodeState = 8;
                }
            }
        }
        break;
    case 3:
        if ((ImageRows->f1 == 5) && (BadgeRows->state10 == 0xB)) {
            ModeGroup = 1;
            ModeIndex = 0x41;
            SubmodeState = 0;
            ovl662_saved_car_choice_idx = (u16) ovl662_selected_car_slot;
        }
        break;
    case 4:
        if ((ImageRows->f1 == 5) && (BadgeRows->state10 == 0xB)) {
            ovl662_select_garage_car();
            goto done;
        }
        break;
    case 5:
        temp_v1_7 = FadeCountdown - 1;
        FadeCountdown = (u16) temp_v1_7;
        AddScreenFadeTiles(CurrentDrawBuf->ordering.ordering70.tags, (0x40 - temp_v1_7) * 4, 0x45);
        if ((s16) FadeCountdown <= 0) {
            ModeGroup = 1;
            ModeIndex = 0x1B;
            SubmodeState = 0;
        }
        break;
    case 8:
        temp_v1_8 = FadeCountdown - 1;
        FadeCountdown = (u16) temp_v1_8;
        AddScreenFadeTiles(CurrentDrawBuf->ordering.ordering70.tags, (0x40 - temp_v1_8) * 4, 0x45);
        if ((s16) FadeCountdown <= 0) {
            ModeGroup = 1;
            ModeIndex = 0x27;
            SubmodeState = 0;
        }
        break;
    default:
        break;
    }
block_67:
        if ((ovl662_menu_substate == 0) && (ImageRows->f1 == 2)) {
            ovl662_menu_substate = 1;
            BadgeRows->half02 = 1;
            var_s0 = 0;
        }
        var_s0 = 0;
        do {
            DrawUiRow(var_s0);
            var_s0 += 1;
        } while (var_s0 < 4);
        var_s0 = var_s6;
        if ((((u8) ImageRows->f1 - 1) < 2U) && (var_s0 == 1)) {
            DrawSelectionIconGrid(0);
            UpdateCarWorldTransform((char *)(void *)PlayerCars);
            var_a2_2 = 0x100;
            if (ImageRows->f1 == var_s0) {
                temp_v0_3 = ~((ImageRows->f2 * 0xFF) / 30);
                color.channels.r = temp_v0_3;
                color.channels.g = temp_v0_3;
                color.channels.b = temp_v0_3;
                var_a3 = 0x102;
                var_a2_2 = (ImageRows->f2 << 8) / 30;
            } else {
                var_a3 = 1;
                color.channels.r = 0x80;
                color.channels.g = 0x80;
                color.channels.b = 0x80;
            }
            DrawPreviewCar(PlayerCars, color.argument, var_a2_2, var_a3);
        }
        UpdateBadgeRowAnim(0);
        if (UpdateImageRow(0) > 0) {
            DrawMenuStepArrows(0xA0, 0x90, 8, 1, ovl662_selected_car_slot, 0, ovl662_available_car_count);
        }
done:
    UpdateScreenFade(0x1008E);
}

/* Initial values of the overlay globals this module owns. */
const char ovl662_str_init_car_choice[104] = {106, 111, 98, 95, 103, 97, 114, 97, 103, 101, 95, 99, 97, 114, 95, 105, 110, 102, 111, 48, 10, 0, 0, 0, 0, 0, 0, 0, -28, 85, 17, -128, 12, 93, 17, -128, 12, 93, 17, -128, -108, 91, 17, -128, -4, 91, 17, -128, 68, 92, 17, -128, 12, 93, 17, -128, 12, 93, 17, -128, -92, 92, 17, -128, 0, 0, 0, 0, 92, 98, 17, -128, -116, 98, 17, -128, -120, 99, 17, -128, -40, 99, 17, -128, -48, 100, 17, -128, 0, 102, 17, -128, 0, 102, 17, -128, 0, 102, 17, -128, -128, 101, 17, -128};
char ovl662_saved_player_name[12];
s16 ovl662_selected_car_slot;

/* Writable state of this unit; ovl662_reset_module restores it. */
static const OverlayStateRegion ovl662_team_car_select_regions[] = {
    RR_OVERLAY_STATE(ovl662_saved_player_name),
    RR_OVERLAY_STATE(ovl662_selected_car_slot)};
const OverlayStateTable ovl662_team_car_select_state = {ovl662_team_car_select_regions, sizeof(ovl662_team_car_select_regions) / sizeof(ovl662_team_car_select_regions[0])};

/* Initial values of the globals this module owns. */
MenuGridPoint PlayerCourseChoices[4];
