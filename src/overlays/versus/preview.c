#include "rr/cpu_time.h"
#include "rr/overlay_module.h"
#include "rr/versus_names.h"
#include "game/menu/menutra21.h"
#include "rr/vehicle_table.h"
#include "rr/menu_frame_state.h"
#include "game/system/pad.h"
#include "rr/menu_overlay_runtime.h"
#include "rr/renderer_setup.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"

/* Versus course selection: each player picks a course group and course. */
extern u8 ovl661_group_option_counts[];            /* option count per course group */
extern s8 ovl661_group_slot_kinds[2][4];        /* option meaning per group slot */
extern s8 ovl661_group_ui_rows[2][2][4];     /* UI row per player, group, slot */
extern s16 ovl661_preview_confirmation;
extern s16 ovl661_available_car_counts[2][2];       /* course count per player/group */
extern s16 ovl661_selected_cars[2];          /* selected course per player */
extern s16 ovl661_selected_groups[2];          /* selected group per player */
extern s16 ovl661_player_win_counts[2];
extern s16 ComDifficultyLevel;
extern s32 ovl661_available_course_count;
extern CourseChoice4 ovl661_car_choices[2][2][9];
extern char ovl661_player2_default_name[];
extern const char ovl661_scene_name[];
extern s32 UiRowCount;

s32 CountChoiceGlyphs(const CourseChoice4 *);
void CopyStringClearTail(char *destination, char *source);
void ovl661_init_preview(s32);
void ovl661_reset_preview_selection(void);
void ovl661_init_scene(void);

#define MODE (ImageRows->f1)
#define PLAYER (ActivePlayerIndex & 1)
#define GROUP_ROW(p, g, i) ovl661_group_ui_rows[p][g][i]

void ovl661_init_preview(s32 mode) {
    s32 i;
    ProjectionScratchPrefix *scratch;
    void *arena;
    ModeFrame=0;
    ScreenFrame=0;
    MenuCursor=0;
    MenuItemCount=3;
    ResetScreenCounters();
    MenuArenas.second=CdDirectoryBuffer+0x2D000;
    InitMenuArenas();
    ActivePlayerIndex=mode;
    LoadCursor=MenuArenas.second;
    InitUiRow(0,24,88,68,32,0,0,1,0,83);
    InitUiRow(1,24,128,68,32,0,0,1,0,54);
    InitUiRow(2,24,168,68,32,0,0,1,0,85);
    InitUiRow(3,24,88,68,32,0,0,1,0,83);
    InitUiRow(4,24,128,68,32,0,0,1,0,101);
    InitUiRow(5,24,168,68,32,0,0,1,0,53);
    InitUiRow(6,24,208,68,32,0,0,1,0,85);
    InitUiRow(7,116,88,28,32,0,0,1,0,55);
    InitUiRow(8,116,128,28,32,0,0,1,0,56);
    InitUiRow(9,24,88,68,32,0,0,1,0,83);
    InitUiRow(10,24,128,68,32,0,0,1,0,54);
    InitUiRow(11,24,168,68,32,0,0,1,0,85);
    InitUiRow(12,24,88,68,32,0,0,1,0,83);
    InitUiRow(13,24,128,68,32,0,0,1,0,101);
    InitUiRow(14,24,168,68,32,0,0,1,0,53);
    InitUiRow(15,24,208,68,32,0,0,1,0,85);
    InitUiRow(16,116,88,28,32,0,0,1,0,55);
    InitUiRow(17,116,128,28,32,0,0,1,0,56);
    InitUiRow(18,12,64,68,32,0,0,1,0,84);
    InitUiRow(19,12,104,68,32,0,0,1,0,3);
    InitUiRow(20,12,144,68,32,0,0,1,1,13);
    InitUiRow(21,12,184,68,32,0,0,1,0,85);
    InitUiRow(22,12,368,208,32,0,0,8,4,SelectedCourseChoice);
    InitUiRow(23,104,104,68,32,0,0,1,5,3);
    InitUiRow(24,104,144,68,32,0,0,1,6,23);
    InitUiRow(25,104,184,68,32,0,0,1,7,6);
    SetupRenderer(5);
    MenuCursor=0;
    MenuItemCount=3;
    ResetScreenCounters();
    InitPreviewImageRows();
    arena=MenuArenaBase;
    ovl661_preview_confirmation=0;
    LoadCursor=arena;
    if(!(ActivePlayerIndex&1))MenuArenas.first=arena;
    else do{LoadCursor=MenuArenas.first;}while(0);
    scratch=RR_PROJECTION_SCRATCH;
    scratch->pose08.angles10.xyz.x=120;
    scratch->pose08.position00.words[0]=-16;
    scratch->pose08.position00.words[1]=-248;
    scratch->pose08.angles10.xyz.y=0;
    scratch->pose08.angles10.xyz.z=0;
    scratch->pose08.position00.words[2]=-96;
    for (i = 0; i < 2; i++) {
        ResetCarState(&PlayerCars[i]);
        PlayerCars[i].position20.words[1] = 10;
        PlayerCars[i].position20.words[2] = 308;
        PlayerCars[i].rot54 = 2048;
    }
    ovl661_available_car_counts[0][0]=CountChoiceGlyphs(ovl661_car_choices[0][0]);
    ovl661_available_car_counts[0][1]=CountChoiceGlyphs(ovl661_car_choices[0][1]);
    ovl661_available_car_counts[1][0]=CountChoiceGlyphs(ovl661_car_choices[1][0]);
    ovl661_available_car_counts[1][1]=CountChoiceGlyphs(ovl661_car_choices[1][1]);
    if(ovl661_available_car_counts[mode&1][1]==0)ovl661_selected_groups[mode&1]=0;
    if(ovl661_selected_cars[ActivePlayerIndex&1]>=ovl661_available_car_counts[ActivePlayerIndex&1][ovl661_selected_groups[ActivePlayerIndex&1]])ovl661_selected_cars[ActivePlayerIndex&1]=0;
    UnpackChoiceGlyph((s8*)&SelectionRows.records[ActivePlayerIndex],ovl661_car_choices[ActivePlayerIndex&1][ovl661_selected_groups[ActivePlayerIndex&1]][ovl661_selected_cars[ActivePlayerIndex&1]].glyph);
    PlayerCourseChoices[ActivePlayerIndex].first=ovl661_car_choices[ActivePlayerIndex&1][ovl661_selected_groups[ActivePlayerIndex&1]][ovl661_selected_cars[ActivePlayerIndex&1]].first;
    PlayerCourseChoices[ActivePlayerIndex].second=ovl661_car_choices[ActivePlayerIndex&1][ovl661_selected_groups[ActivePlayerIndex&1]][ovl661_selected_cars[ActivePlayerIndex&1]].second;
}

/* Both destination name slots contain a NUL within12 bytes; the saved name
 * is terminated within its first12 bytes. String-tail cleanup keeps that bound. */
void ovl661_init_menu(void)
{
    s32 i;
    s32 j;

    SetDispMask(0);
    ClearScreenOverlayState();
    for (i = 0; i < 2; i++) {
        ovl661_selected_groups[i] = 0;
        ovl661_selected_cars[i] = 0;
        ovl661_player_win_counts[i] = 0;
    }
    ComDifficultyLevel = 0;
    SelectedCourseChoice = 0;
    CourseIndex = 0;
    NumLaps = 3;
    NumCars = 2;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 9; j++) {
            ovl661_car_choices[0][i][j] = CourseChoiceTable[i][j];
            ovl661_car_choices[1][i][j] = CourseChoiceTable[0][j];
        }
    }
    CopyStringClearTail(ovl661_player_names[0], (char *)SaveProgress.prefix.progress.prefix00.bytes);
    CopyStringClearTail(ovl661_player_names[1], ovl661_player2_default_name);
    SetupDrawDispEnvs(0, 0, 0, 2);
    ovl661_init_preview(0);
    UnpackChoiceGlyph((s8 *)&SelectionRows.records[0], CourseChoiceTable[0][0].glyph);
    UnpackChoiceGlyph((s8 *)&SelectionRows.records[1], CourseChoiceTable[0][0].glyph);
    SeqStartTrack(0);
    ovl661_available_course_count = 8;
    ModeGroup = 1;
    ModeIndex = 56;
    SubmodeState = 0;
    DebugPrintStub(ovl661_scene_name);
    SetMenuFrameOptions(0, 1, 1);
}

void ovl661_reset_preview_selection(void)
{
    ResetScreenCounters();
    LoadCursor = MenuArenas.first;
    UnpackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex], ovl661_car_choices[ActivePlayerIndex & 1][ovl661_selected_groups[ActivePlayerIndex & 1]][ovl661_selected_cars[ActivePlayerIndex & 1]].glyph);
    PlayerCourseChoices[ActivePlayerIndex].first = ovl661_car_choices[ActivePlayerIndex & 1][ovl661_selected_groups[ActivePlayerIndex & 1]][ovl661_selected_cars[ActivePlayerIndex & 1]].first;
    PlayerCourseChoices[ActivePlayerIndex].second = ovl661_car_choices[ActivePlayerIndex & 1][ovl661_selected_groups[ActivePlayerIndex & 1]][ovl661_selected_cars[ActivePlayerIndex & 1]].second;
    ImageRows->f1 = 6;
}

void ovl661_update_preview(void)
{
    RR_CPU_CYCLES(RR_CYCLES_VS_CAR_SELECT);
    PreviewColor color;
    s16 active;
    s32 result;
    s32 row;
    u8 status;
    s32 before;
    s32 i;
    s32 close;
    s16 player;
    s16 group;
    s32 brightness;
    s32 flags;
    s32 sound;
    s16 *groups;
    s8 last;
    RaceVehicle *vehicle;
    RaceVehicle *vehicles;
    s32 *tick;
    ControllerState5C *input;

    active = 0;
    tick = &ModeFrame;
    vehicles = PlayerCars;
    vehicle = &vehicles[PLAYER];
    (*tick)++;
    vehicle->rot54 = (vehicle->rot54 + 8) & 0xFFF;
    SetupMenuCarCamera(1);
    SetupViewport(6);
    result = LoadCarRangeStep(ActivePlayerIndex, 1);
    if (result != 1) {
        if (MODE == 1) {
            MODE = 0;
        }
    } else {
        SetCarBounds(&vehicles[PLAYER]);
        if (MODE == 7) {
            MODE = result;
        }
        active = 1;
        switch (SubmodeState) {
        case 7:
            row = GROUP_ROW(PLAYER, (s16)(ovl661_selected_groups[PLAYER] ^ 1), 0);
            if (UiRows[row].byte14 == 5) {
                SubmodeState = 0;
            }
        case 0:
            SetMenuFrameOptions(1, 1, 1);
            before = PackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex]);
            StepMenuValue(PLAYER * 4 | 1, 2, (char *)&ovl661_selected_cars[PLAYER],
                          ovl661_available_car_counts[PLAYER][ovl661_selected_groups[PLAYER]], 0);
            UnpackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex],
                          ovl661_car_choices[PLAYER][ovl661_selected_groups[PLAYER]][ovl661_selected_cars[PLAYER]].glyph);
            if ((s16)before != (s16)PackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex])) {
                ResetScreenCounters();
                LoadCursor = MenuArenas.first;
                UnpackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex],
                              ovl661_car_choices[PLAYER][ovl661_selected_groups[PLAYER]][ovl661_selected_cars[PLAYER]].glyph);
                PlayerCourseChoices[ActivePlayerIndex].first = ovl661_car_choices[PLAYER][ovl661_selected_groups[PLAYER]][ovl661_selected_cars[PLAYER]].first;
                PlayerCourseChoices[ActivePlayerIndex].second = ovl661_car_choices[PLAYER][ovl661_selected_groups[PLAYER]][ovl661_selected_cars[PLAYER]].second;
                MODE = 6;
            }
            row = GROUP_ROW(PLAYER, ovl661_selected_groups[PLAYER], MenuCursor);
            UiRows[row].cells[0] = 0;
            StepMenuValue(PLAYER * 4, 2, (char *)&MenuCursor, ovl661_group_option_counts[ovl661_selected_groups[PLAYER]], 0);
            row = GROUP_ROW(PLAYER, ovl661_selected_groups[PLAYER], MenuCursor);
            UiRows[row].cells[0] = 2;
            i = UiRows[row].byte14;
            close = 0;
            if (i != 2) {
                break;
            }
            if (BadgeRows->state10 != 3) {
                break;
            }
            {
                ControllerState5C *inputs = PlayerControllers;
                input = inputs + PLAYER;
            }
            if (input->action28 != 0) {
                PlaySfx(1);
                UiRows[row].cells[0] = 1;
                if (MenuCursor == ovl661_group_slot_kinds[ovl661_selected_groups[PLAYER]][0]) {
                    ovl661_preview_confirmation = i;
                    SubmodeState = 4;
                } else if (MenuCursor == ovl661_group_slot_kinds[ovl661_selected_groups[PLAYER]][1]
                           && ovl661_car_choices[ActivePlayerIndex][1][0].glyph != -1) {
                    ovl661_selected_cars[PLAYER] = 0;
                    MenuCursor = 0;
                    for (i = 0; i < 4; i++) {
                        row = GROUP_ROW(PLAYER, ovl661_selected_groups[PLAYER], i);
                        if (row == -1) {
                            break;
                        }
                        if (UiRows[row].byte14 == 2) {
                            UiRows[row].byte14 = 3;
                        }
                    }
                    for (i = 0; i < 4;) {
                        row = GROUP_ROW(PLAYER, (s16)(ovl661_selected_groups[PLAYER] ^ 1), i);
                        i++;
                        if (row == -1) {
                            break;
                        }
                        UiRows[row].byte14 = 0;
                    }
                    SubmodeState = 7;
                    ovl661_selected_groups[PLAYER] ^= 1;
                } else if (MenuCursor == ovl661_group_slot_kinds[ovl661_selected_groups[PLAYER]][2]) {
                    close = 1;
                } else if (MenuCursor == ovl661_group_slot_kinds[ovl661_selected_groups[PLAYER]][3]) {
                    for (i = 0; i < UiRowCount; i++) {
                        if (UiRows[i].byte14 == 2) {
                            UiRows[i].byte14 = 3;
                        }
                    }
                    MODE = 3;
                    FadeCountdown = 0x40;
                    SubmodeState = 10;
                    BadgeRows->state10 = 4;
                }
            } else if (input->action2C != 0) {
                PlaySfx(2);
                close = 1;
            }
            if (close == 1) {
                for (i = 0; i < UiRowCount; i++) {
                    if (UiRows[i].byte14 == 2) {
                        UiRows[i].byte14 = 3;
                    }
                }
                MODE = 3;
                FadeCountdown = 0x40;
                BadgeRows->state10 = 4;
                SeqFadeVolume(0, 60);
                SubmodeState = 8;
            }
            break;
        case 3:
            row = GROUP_ROW((ActivePlayerIndex - 1) & 1, 1, 3) + 1;
            if (UiRows[row].byte14 == 5) {
                ovl661_preview_confirmation = 1;
                SubmodeState = 0;
            }
            break;
        case 4:
            row = GROUP_ROW(PLAYER, 1, 3) + 1;
            i = UiRows[row].byte14;
            if (i != 2) {
                break;
            }
            UiRows[row + SelectionRows.entries[ActivePlayerIndex].option].cells[0] = 0;
            StepMenuValue(PLAYER * 4, 1, (char *)&SelectionRows.entries[ActivePlayerIndex].option, 2, 0);
            UiRows[row + SelectionRows.entries[ActivePlayerIndex].option].cells[0] = i;
            {
                ControllerState5C *inputs = PlayerControllers;
                input = inputs + PLAYER;
            }
            if (input->action28 != 0) {
                PlaySfx(1);
                UiRows[row + SelectionRows.entries[0].option].cells[0] = 1;
                UiRows[row].byte14 = 3;
                UiRows[row + 1].byte14 = 3;
                for (i = 0; i < 4; i++) {
                    row = GROUP_ROW(PLAYER, ovl661_selected_groups[PLAYER], i);
                    if (row == -1) {
                        break;
                    }
                    if (UiRows[row].byte14 == 2) {
                        UiRows[row].byte14 = 3;
                    }
                }
                active = 0;
                MenuArenas.first = LoadCursor;
                if ((ActivePlayerIndex & 1) > 0) {
                    SubmodeState = 5;
                    for (i = 0; i < UiRowCount; i++) {
                        if (UiRows[i].byte14 == 2) {
                            UiRows[i].byte14 = 3;
                        }
                    }
                    MODE = 3;
                    BadgeRows->state10 = 4;
                } else {
                    SubmodeState = 3;
                    ActivePlayerIndex++;
                    ovl661_reset_preview_selection();
                }
            } else if (input->action2C != 0) {
                PlaySfx(2);
                UiRows[row].byte14 = 3;
                UiRows[row + 1].byte14 = 3;
                SubmodeState = 6;
            }
            break;
        case 5:
            SetMenuFrameOptions(0, 1, 1);
            row = GROUP_ROW(PLAYER, 1, 3) + 1;
            status = UiRows[row].byte14;
            if (status == 5 && MODE == status) {
                ResetScreenCounters();
                ovl661_init_scene();
                ModeGroup = 1;
                ModeIndex = 0x39;
                SubmodeState = 0;
            }
            break;
        case 6:
            row = GROUP_ROW(PLAYER, 1, 3) + 1;
            if (UiRows[row].byte14 == 5) {
                SubmodeState = 0;
                ovl661_preview_confirmation = 1;
                UiRows[row].byte14 = 0;
                UiRows[row + 1].byte14 = 0;
            }
            break;
        case 9:
            row = GROUP_ROW(PLAYER, 1, 3) + 1;
            status = UiRows[row].byte14;
            if (status == 5 && MODE == status) {
                ovl661_init_preview(0);
                SubmodeState = 0;
            }
            break;
        case 8:
            SetMenuFrameOptions(0, 1, 1);
            FadeCountdown--;
            SetModelTexturePage(-1);
            AddScreenFadeTiles(CurrentDrawBuf->ordering.ordering70.tags, (0x40 - FadeCountdown) * 4, 0x45);
            if (FadeCountdown <= 0) {
                SeqMuteVolume();
                ModeGroup = 1;
                ModeIndex = 3;
                SubmodeState = 0;
            }
            break;
        case 10:
            SetMenuFrameOptions(0, 1, 1);
            SetModelTexturePage(-1);
            FadeCountdown--;
            AddScreenFadeTiles(CurrentDrawBuf->ordering.ordering70.tags, (0x40 - FadeCountdown) * 4, 0x45);
            if (FadeCountdown <= 0) {
                CardMenuOperation = 2;
                ModeGroup = 1;
                ModeIndex = 0x47;
                SubmodeState = 0;
            }
            break;
        }
    }
    if (ovl661_preview_confirmation == 0 && MODE == 2) {
        ovl661_preview_confirmation = 1;
        BadgeRows->half02 = 1;
    }
    if (SubmodeState == 3) {
        player = ActivePlayerIndex - 1;
    } else {
        player = ActivePlayerIndex;
    }
    if (SubmodeState == 7) {
        group = ovl661_selected_groups[player & 1] ^ 1;
        row = GROUP_ROW(player & 1, group, 0);
        if (UiRows[row].byte14 == 5) {
            group = ovl661_selected_groups[player & 1];
        }
    } else {
        group = ovl661_selected_groups[player & 1];
    }
    for (i = 0; i < 4; i++) {
        row = GROUP_ROW(player & 1, group, i);
        if (row == -1) {
            break;
        }
        DrawUiRow(row);
    }
    if (ovl661_preview_confirmation == 2) {
        last = GROUP_ROW(player & 1, 1, 3);
        DrawUiRow(last + 1);
        DrawUiRow(last + 2);
    }
    if ((u8)MODE - 1 < 2U) {
        i = active;
        if (i == 1) {
            DrawSelectionIconGrid(ActivePlayerIndex);
            UpdateCarWorldTransform(&PlayerCars[PLAYER]);
            brightness = 0x100;
            if (MODE == i) {
                last = ~((ImageRows->f2 * 0xFF) / 30);
                color.channels.r = last;
                color.channels.g = last;
                color.channels.b = last;
                flags = 0x102;
                brightness = (ImageRows->f2 << 8) / 30;
            } else {
                flags = 1;
                color.channels.r = 0x80;
                color.channels.g = 0x80;
                color.channels.b = 0x80;
            }
            DrawPreviewCar(&PlayerCars[PLAYER], color.argument, brightness, flags);
        }
    }
    UpdateBadgeRowAnim(0);
    if (UpdateImageRow(0) > 0) {
        DrawMenuStepArrows(0xA0, 0x90, 8, 1, ovl661_selected_cars[PLAYER], 0,
                      ovl661_available_car_counts[PLAYER][ovl661_selected_groups[PLAYER]]);
    }
    MenuFrameState.titlePlayer08 = ActivePlayerIndex + 1;
    sound = 0x10032;
    groups = ovl661_selected_groups;
    if (groups[ActivePlayerIndex & 1] != 0) {
        sound = 0x10036;
    }
    UpdateScreenFade(sound);
}

/* Initial values of the overlay globals this module owns. */
char ovl661_player2_default_name[12] = {80, 76, 65, 89, 69, 82, 32, 50};
const char ovl661_scene_name[100] = {106, 111, 98, 95, 118, 115, 95, 99, 97, 114, 95, 105, 110, 102, 111, 48, 10, 0, 0, 0, -92, 98, 17, -128, -108, 111, 17, -128, -108, 111, 17, -128, -76, 105, 17, -128, 20, 106, 17, -128, 0, 109, 17, -128, -96, 109, 17, -128, 52, 98, 17, -128, -116, 110, 17, -128, 20, 110, 17, -128, 8, 111, 17, -128, -68, 116, 17, -128, -128, 123, 17, -128, -128, 123, 17, -128, 92, 120, 17, -128, 84, 121, 17, -128, -20, 121, 17, -128, -128, 123, 17, -128, -128, 123, 17, -128, 56, 123, 17, -128};
u8 ovl661_group_option_counts[4] = {3, 4};
s8 ovl661_group_slot_kinds[2][4] = {{0, 1, 2, -1}, {0, 2, 3, 1}};
s8 ovl661_group_ui_rows[2][2][4] = {{{0, 1, 2, -1}, {3, 4, 5, 6}}, {{9, 10, 11, -1}, {12, 13, 14, 15}}};
char ovl661_player_names[2][12];
s16 ovl661_preview_confirmation;
s16 ovl661_available_car_counts[2][2];
s16 ovl661_selected_cars[2];
s16 ovl661_selected_groups[2];
CourseChoice4 ovl661_car_choices[2][2][9];

/* Writable state of this unit; ovl661_reset_module restores it. */
static const OverlayStateRegion ovl661_preview_regions[] = {
    RR_OVERLAY_STATE(ovl661_available_car_counts),
    RR_OVERLAY_STATE(ovl661_car_choices),
    RR_OVERLAY_STATE(ovl661_group_option_counts),
    RR_OVERLAY_STATE(ovl661_group_slot_kinds),
    RR_OVERLAY_STATE(ovl661_group_ui_rows),
    RR_OVERLAY_STATE(ovl661_player_names),
    RR_OVERLAY_STATE(ovl661_player2_default_name),
    RR_OVERLAY_STATE(ovl661_preview_confirmation),
    RR_OVERLAY_STATE(ovl661_selected_cars),
    RR_OVERLAY_STATE(ovl661_selected_groups)};
const OverlayStateTable ovl661_preview_state = {ovl661_preview_regions, sizeof(ovl661_preview_regions) / sizeof(ovl661_preview_regions[0])};

/* Initial values of the globals this module owns. */
s32 ActivePlayerIndex;
