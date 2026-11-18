#include "rr/overlay_module.h"
#include "game/menu/menutra21.h"
#include "rr/menu_selection.h"
#include "rr/menu_frame_state.h"
#include "game/race/racegrid.h"
#include "game/system/pad.h"
#include "scratchpad.h"
#include "rr/menu_overlay_runtime.h"
#include "rr/renderer_setup.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"

extern s16 LinkPlayerWinCounts[4];
extern CourseChoice4 CourseChoiceTable[4][9];
extern CourseChoice4 LinkCarChoiceTable[2][2][9];
extern s16 LinkMenuSyncPhase;
extern s16 LinkCarChoiceCounts[2][2];
extern s16 LinkCarChoiceIndex[2];
extern s16 LinkCarChoiceGroup[2];
extern s32 UiRowCount;

extern u8 ovl667_group_option_counts[4];
extern s8 ovl667_group_slot_kinds[2][4];
extern s8 ovl667_group_ui_rows[2][2][4];
extern char LinkPlayerNames[4][12];
extern char ovl667_player_name_text[4][12];
extern char *ovl667_default_player_names[4];
extern s32 LinkAvailableCourseCount;
extern s16 LinkClientRoleState;
s32 CountChoiceGlyphs(const CourseChoice4 *);
void CopyStringClearTail(char *, char *);
void ovl667_init_car_selection(s32);

void ovl667_init_car_selection(s32 arg0) {
    s16 *temp_s0;
    s16 *temp_t0_2;
    s32 temp_a0;
    s32 temp_a2;
    s32 temp_a2_2;
    s32 var_s1;
    MenuGridPoint *temp_a3_2;

    ModeFrame = 0;
    ScreenFrame = 0;
    MenuCursor = 0;
    MenuItemCount = 3;
    ResetScreenCounters();
    MenuArenas.second = CdDirectoryBuffer + 0x2D000;
    InitMenuArenas();
    ActivePlayerIndex = arg0;
    LoadCursor = MenuArenas.second;
    InitUiRow(0, 0x18, 0x58, 0x44, 0x20, 0, 0, 1, 0, 0x53);
    InitUiRow(1, 0x18, 0x80, 0x44, 0x20, 0, 0, 1, 0, 0x36);
    InitUiRow(2, 0x18, 0xA8, 0x44, 0x20, 0, 0, 1, 0, 0x55);
    InitUiRow(3, 0x18, 0x58, 0x44, 0x20, 0, 0, 1, 0, 0x53);
    InitUiRow(4, 0x18, 0x80, 0x44, 0x20, 0, 0, 1, 0, 0x65);
    InitUiRow(5, 0x18, 0xA8, 0x44, 0x20, 0, 0, 1, 0, 0x35);
    InitUiRow(6, 0x18, 0xD0, 0x44, 0x20, 0, 0, 1, 0, 0x55);
    InitUiRow(7, 0x74, 0x58, 0x1C, 0x20, 0, 0, 1, 0, 0x37);
    InitUiRow(8, 0x74, 0x80, 0x1C, 0x20, 0, 0, 1, 0, 0x38);
    InitUiRow(9, 0x18, 0x58, 0x44, 0x20, 0, 0, 1, 0, 0x53);
    InitUiRow(0xA, 0x18, 0x80, 0x44, 0x20, 0, 0, 1, 0, 0x36);
    InitUiRow(0xB, 0x18, 0xA8, 0x44, 0x20, 0, 0, 1, 0, 0x55);
    InitUiRow(0xC, 0x18, 0x58, 0x44, 0x20, 0, 0, 1, 0, 0x53);
    InitUiRow(0xD, 0x18, 0x80, 0x44, 0x20, 0, 0, 1, 0, 0x65);
    InitUiRow(0xE, 0x18, 0xA8, 0x44, 0x20, 0, 0, 1, 0, 0x35);
    InitUiRow(0xF, 0x18, 0xD0, 0x44, 0x20, 0, 0, 1, 0, 0x55);
    InitUiRow(0x10, 0x74, 0x58, 0x1C, 0x20, 0, 0, 1, 0, 0x37);
    InitUiRow(0x11, 0x74, 0x80, 0x1C, 0x20, 0, 0, 1, 0, 0x38);
    InitUiRow(0x12, 0xC, 0x40, 0x44, 0x20, 0, 0, 1, 0, 0x54);
    InitUiRow(0x13, 0xC, 0x68, 0x44, 0x20, 0, 0, 1, 0, 3);
    InitUiRow(0x14, 0xC, 0x90, 0x44, 0x20, 0, 0, 1, 0, 0x55);
    InitUiRow(0x15, 0xC, 0x170, 0xD0, 0x20, 0, 0, 8, 4, SelectedCourseChoice);
    InitUiRow(0x17, 0x68, 0x68, 0x44, 0x20, 0, 0, 1, 5, 3);
    InitUiRow(0x18, 0x68, 0x40, 0x44, 0x20, 0, 0, 0, 0x7E, 0);
    SetupRenderer(5);
    MenuCursor = 0;
    MenuItemCount = 3;
    ResetScreenCounters();
    InitPreviewImageRows();
    LinkMenuSyncPhase = 0;
    LoadCursor = MenuArenaBase;
    if (!(ActivePlayerIndex & 1)) {
        MenuArenas.first = MenuArenaBase;
    } else {
        do {
            LoadCursor = MenuArenas.first;
        } while (0);
    }
    RR_PROJECTION_SCRATCH->pose08.angles10.xyz.x = 0x78;
    RR_PROJECTION_SCRATCH->pose08.position00.words[0] = -0x10;
    RR_PROJECTION_SCRATCH->pose08.position00.words[1] = -0xF8;
    RR_PROJECTION_SCRATCH->pose08.angles10.xyz.y = 0;
    RR_PROJECTION_SCRATCH->pose08.angles10.xyz.z = 0;
    RR_PROJECTION_SCRATCH->pose08.position00.words[2] = -0x60;
    for (var_s1 = 0; var_s1 < 2; var_s1++) {
        ResetCarState(&PlayerCars[var_s1]);
        PlayerCars[var_s1].position20.words[1] = 10;
        PlayerCars[var_s1].position20.words[2] = 308;
        PlayerCars[var_s1].rot54 = 0x800;
    }
    LinkCarChoiceCounts[0][0] = CountChoiceGlyphs(LinkCarChoiceTable[0][0]);
    LinkCarChoiceCounts[0][1] = CountChoiceGlyphs(LinkCarChoiceTable[0][1]);
    LinkCarChoiceCounts[1][0] = CountChoiceGlyphs(LinkCarChoiceTable[1][0]);
    LinkCarChoiceCounts[1][1] = CountChoiceGlyphs(LinkCarChoiceTable[1][1]);
    temp_a0 = arg0 & 1;
    if (LinkCarChoiceCounts[temp_a0][1] == 0) {
        LinkCarChoiceGroup[temp_a0] = 0;
    }
    temp_a2 = ActivePlayerIndex & 1;
    UnpackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex],
        LinkCarChoiceTable[temp_a2][LinkCarChoiceGroup[temp_a2]][LinkCarChoiceIndex[temp_a2]].glyph);
    temp_a2_2 = ActivePlayerIndex & 1;
    temp_s0 = &LinkCarChoiceGroup[temp_a2_2];
    temp_t0_2 = &LinkCarChoiceIndex[temp_a2_2];
    temp_a3_2 = &PlayerCourseChoices[ActivePlayerIndex];
    temp_a3_2->first = LinkCarChoiceTable[temp_a2_2][*temp_s0][*temp_t0_2].first;
    temp_a3_2->second = LinkCarChoiceTable[temp_a2_2][*temp_s0][*temp_t0_2].second;
}

void ovl667_init_session_menus(void){
 typedef struct {s8 first,second;} Pair;
 s32 i;
 s32 j;
 SetDispMask(0);
 SetupDrawDispEnvs(0,0,0,2);
 UpdateLinkHandshake(1);
 i=0;
 ClearScreenOverlayState();
 for(;i<2;i++) {
     LinkCarChoiceGroup[i]=0;
     LinkCarChoiceIndex[i]=0;
     LinkPlayerWinCounts[i]=0;
 }
 for(i=0;i<4;i++) {
     (&SelectionRows.records[i])->bytes[3]=0;
     LinkPlayerWinCounts[i]=0;
 }
 CourseIndex=0;SelectedCourseChoice=0;NumLaps=3;
 ActivePlayerIndex=(1-(s16)LinkHostFlag)*2;
 /* Bank 0 copies choice groups 0 and 1; bank 1 repeats group 0 in both rows
  * (as the versus overlay's ovl661_car_choices). */
 for(i=0;i<2;i++)for(j=0;j<9;j++) {
  LinkCarChoiceTable[0][i][j]=CourseChoiceTable[i][j];
  LinkCarChoiceTable[1][i][j]=CourseChoiceTable[0][j];
 }
 for(i=0;i<2;i++)CopyStringClearTail(LinkPlayerNames[ActivePlayerIndex+i],ovl667_default_player_names[ActivePlayerIndex+i]);
 ovl667_init_car_selection((1-(s16)LinkHostFlag)*2);
 UnpackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex],CourseChoiceTable[0][0].glyph);
 UnpackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex+1],CourseChoiceTable[0][0].glyph);
 LinkAvailableCourseCount=8;
 SeqStartTrack(0);
 ModeGroup=1;
 ModeIndex=60;
 SubmodeState=0;
 LinkMenuSyncPhase=0;
 if(LinkEntryState[ActivePlayerIndex/2][1]!=1)LinkClientRoleState=0;else LinkClientRoleState=2;
 SetMenuFrameOptions(0,1,1);
}

void ovl667_reset_preview_selection(void) {
    ResetScreenCounters();
    LoadCursor = MenuArenas.first;
    UnpackChoiceGlyph(
        (s8 *)&SelectionRows.records[ActivePlayerIndex],
        LinkCarChoiceTable[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]][LinkCarChoiceIndex[ActivePlayerIndex & 1]].glyph
    );
    PlayerCourseChoices[ActivePlayerIndex].first = LinkCarChoiceTable[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]][LinkCarChoiceIndex[ActivePlayerIndex & 1]].first;
    PlayerCourseChoices[ActivePlayerIndex].second = LinkCarChoiceTable[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]][LinkCarChoiceIndex[ActivePlayerIndex & 1]].second;
    ImageRows->f1 = 6;
}

/* Four-player selection: per-pair page, row and course group tables. */
/* Resident word reached through its enclosing array object (aliases row stores). */
s32 ExitOnLinkLost(void);
s32 UpdateLinkHandshake(s32);
void ovl667_reset_preview_selection(void);
void ovl667_init_course_menu(void);

void ovl667_update_car_selection(void) {
    PreviewColor color;
    s16 modelReady;
    s32 drawFlags;
    s32 drawModel;
    s32 scale;
    s32 loadResult;
    u32 previous;
    s32 state;
    s32 i;
    s32 cancel;
    s32 row;
    u16 player;
    u16 page;
    u16 other;
    s8 brightness;
    u8 closing;

    modelReady = 0;
    ModeFrame++;
    if (ExitOnLinkLost() != 0) {
        SeqMuteVolume();
        return;
    }
    UpdateLinkHandshake(2);
    PlayerCars[ActivePlayerIndex & 1].rot54 = (PlayerCars[ActivePlayerIndex & 1].rot54 + 8) & 0xFFF;
    SetupMenuCarCamera(1);
    SetupViewport(6);
    loadResult = LoadCarRangeStep(ActivePlayerIndex, 1);
    if (loadResult != 1) {
        if (ImageRows->f1 == 1) {
            ImageRows->f1 = 0;
        }
    } else {
        SetCarBounds(&PlayerCars[ActivePlayerIndex & 1]);
        if (ImageRows->f1 == 7) {
            ImageRows->f1 = loadResult;
        }
        modelReady = 1;
        switch (SubmodeState) {
        case 7:
            row = ovl667_group_ui_rows[ActivePlayerIndex & 1][(s16)(LinkCarChoiceGroup[ActivePlayerIndex & 1] ^ 1)][0];
            if (UiRows[row].byte14 == 5) {
                SubmodeState = 0;
            }
        case 0:
            SetMenuFrameOptions(1, 1, 1);
            previous = PackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex]);
            StepMenuValue(((ActivePlayerIndex & 1) * 4) | 1, 2, (char *)&LinkCarChoiceIndex[ActivePlayerIndex & 1],
                          LinkCarChoiceCounts[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]], 0);
            UnpackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex],
                          LinkCarChoiceTable[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]][LinkCarChoiceIndex[ActivePlayerIndex & 1]].glyph);
            if ((previous << 16) != (PackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex]) << 16)) {
                ResetScreenCounters();
                LoadCursor = MenuArenas.first;
                UnpackChoiceGlyph((s8 *)&SelectionRows.records[ActivePlayerIndex],
                              LinkCarChoiceTable[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]][LinkCarChoiceIndex[ActivePlayerIndex & 1]].glyph);
                PlayerCourseChoices[ActivePlayerIndex].first = LinkCarChoiceTable[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]][LinkCarChoiceIndex[ActivePlayerIndex & 1]].first;
                PlayerCourseChoices[ActivePlayerIndex].second = LinkCarChoiceTable[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]][LinkCarChoiceIndex[ActivePlayerIndex & 1]].second;
                ImageRows->f1 = 6;
            }
            row = ovl667_group_ui_rows[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]][MenuCursor];
            UiRows[row].cells[0] = 0;
            StepMenuValue((ActivePlayerIndex & 1) * 4, 2, (char *)&MenuCursor,
                          ovl667_group_option_counts[LinkCarChoiceGroup[ActivePlayerIndex & 1]], 0);
            row = ovl667_group_ui_rows[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]][MenuCursor];
            UiRows[row].cells[0] = 2;
            state = UiRows[row].byte14;
            cancel = 0;
            if (state != 2) {
                break;
            }
            if (BadgeRows->state10 != 3) {
                break;
            }
            if (PlayerControllers[ActivePlayerIndex & 1].action28 != 0) {
                PlaySfx(1);
                UiRows[row].cells[0] = 1;
                if (MenuCursor == ovl667_group_slot_kinds[LinkCarChoiceGroup[ActivePlayerIndex & 1]][0]) {
                    LinkMenuSyncPhase = state;
                    SubmodeState = 4;
                } else if (MenuCursor == ovl667_group_slot_kinds[LinkCarChoiceGroup[ActivePlayerIndex & 1]][1] && LinkCarChoiceTable[ActivePlayerIndex][1][0].glyph != -1) {
                    LinkCarChoiceIndex[ActivePlayerIndex & 1] = 0;
                    MenuCursor = 0;
                    for (i = 0; i < 4; i++) {
                        row = ovl667_group_ui_rows[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]][i];
                        if (row == -1) {
                            break;
                        }
                        if (UiRows[row].byte14 == 2) {
                            UiRows[row].byte14 = 3;
                        }
                    }
                    for (i = 0; i < 4; i++) {
                        row = ovl667_group_ui_rows[ActivePlayerIndex & 1][(s16)(LinkCarChoiceGroup[ActivePlayerIndex & 1] ^ 1)][i];
                        if (row == -1) {
                            break;
                        }
                        UiRows[row].byte14 = 0;
                    }
                    SubmodeState = 7;
                    LinkCarChoiceGroup[ActivePlayerIndex & 1] ^= 1;
                } else if (MenuCursor == ovl667_group_slot_kinds[LinkCarChoiceGroup[ActivePlayerIndex & 1]][2]) {
                    cancel = 1;
                } else if (MenuCursor == ovl667_group_slot_kinds[LinkCarChoiceGroup[ActivePlayerIndex & 1]][3]) {
                    for (i = 0; i < UiRowCount; i++) {
                        if (UiRows[i].byte14 == 2) {
                            UiRows[i].byte14 = 3;
                        }
                    }
                    ImageRows->f1 = 3;
                    FadeCountdown = 0x40;
                    SubmodeState = 10;
                    BadgeRows->state10 = 4;
                }
            } else if (PlayerControllers[ActivePlayerIndex & 1].action2C != 0) {
                PlaySfx(2);
                cancel = 1;
            }
            if (cancel == 1) {
                for (i = 0; i < UiRowCount; i++) {
                    if (UiRows[i].byte14 == 2) {
                        UiRows[i].byte14 = 3;
                    }
                }
                ImageRows->f1 = 3;
                FadeCountdown = 0x40;
                SubmodeState = 8;
                BadgeRows->state10 = 4;
                SeqFadeVolume(0, 0x3C);
            }
            break;
        case 3:
            row = ovl667_group_ui_rows[(ActivePlayerIndex - 1) & 1][1][3] + 1;
            if (UiRows[row].byte14 == 5) {
                LinkMenuSyncPhase = 1;
                SubmodeState = 0;
            }
            break;
        case 4:
            row = ovl667_group_ui_rows[ActivePlayerIndex & 1][1][3] + 1;
            state = UiRows[row].byte14;
            if (state != 2) {
                break;
            }
            UiRows[row + SelectionRows.records[ActivePlayerIndex].bytes[3]].cells[0] = 0;
            StepMenuValue((ActivePlayerIndex & 1) * 4, 1, (char *)&SelectionRows.records[ActivePlayerIndex].bytes[3], 2, 0);
            UiRows[row + SelectionRows.records[ActivePlayerIndex].bytes[3]].cells[0] = state;
            if (PlayerControllers[ActivePlayerIndex & 1].action28 != 0) {
                PlaySfx(1);
                UiRows[row + SelectionRows.records[ActivePlayerIndex].bytes[3]].cells[0] = 1;
                UiRows[row].byte14 = 3;
                UiRows[row + 1].byte14 = 3;
                for (i = 0; i < 4; i++) {
                    row = ovl667_group_ui_rows[ActivePlayerIndex & 1][LinkCarChoiceGroup[ActivePlayerIndex & 1]][i];
                    if (row == -1) {
                        break;
                    }
                    if (UiRows[row].byte14 == 2) {
                        UiRows[row].byte14 = 3;
                    }
                }
                modelReady = 0;
                MenuArenas.first = LoadCursor;
                if ((ActivePlayerIndex & 1) > 0 || LinkEntryState[1 - LinkHostFlag][1] != 1) {
                    SubmodeState = 5;
                    for (i = 0; i < UiRowCount; i++) {
                        if (UiRows[i].byte14 == 2) {
                            UiRows[i].byte14 = 3;
                        }
                    }
                    ImageRows->f1 = 3;
                    BadgeRows->state10 = 4;
                    break;
                }
                SubmodeState = 3;
                ActivePlayerIndex = ActivePlayerIndex + 1;
                ovl667_reset_preview_selection();
            } else if (PlayerControllers[ActivePlayerIndex & 1].action2C != 0) {
                PlaySfx(2);
                UiRows[row].byte14 = 3;
                UiRows[row + 1].byte14 = 3;
                SubmodeState = 6;
            }
            break;
        case 5:
            SetMenuFrameOptions(0, 1, 1);
            row = ovl667_group_ui_rows[ActivePlayerIndex & 1][1][3] + 1;
            closing = UiRows[row].byte14;
            if (closing == 5 && ImageRows->f1 == closing) {
                ResetScreenCounters();
                ovl667_init_course_menu();
                ModeGroup = 1;
                ModeIndex = 61;
                SubmodeState = 0;
            }
            break;
        case 6:
            row = ovl667_group_ui_rows[ActivePlayerIndex & 1][1][3] + 1;
            if (UiRows[row].byte14 == 5) {
                SubmodeState = 0;
                LinkMenuSyncPhase = 1;
                UiRows[row].byte14 = 0;
                UiRows[row + 1].byte14 = 0;
            }
            break;
        case 8:
            SetModelTexturePage(-1);
            SetMenuFrameOptions(0, 1, 1);
            FadeCountdown = (u16)FadeCountdown - 1;
            AddScreenFadeTiles(&CurrentDrawBuf->ordering.ordering70.tags[0], (0x40 - FadeCountdown) * 4, 0x45);
            if (FadeCountdown <= 0) {
                SeqMuteVolume();
                ModeGroup = 1;
                ModeIndex = 3;
                SubmodeState = 0;
            }
            break;
        case 10:
            SetModelTexturePage(-1);
            SetMenuFrameOptions(0, 1, 1);
            FadeCountdown = (u16)FadeCountdown - 1;
            AddScreenFadeTiles(&CurrentDrawBuf->ordering.ordering70.tags[0], (0x40 - FadeCountdown) * 4, 0x45);
            if (FadeCountdown <= 0) {
                CardMenuOperation = 2;
                ModeGroup = 1;
                ModeIndex = 71;
                SubmodeState = 0;
            }
            break;
        }
    }
    if (LinkMenuSyncPhase == 0 && ImageRows->f1 == 2) {
        LinkMenuSyncPhase = 1;
        BadgeRows->half02 = 1;
    }
    if (SubmodeState == 3) {
        player = (u16)ActivePlayerIndex - 1;
    } else {
        player = ActivePlayerIndex;
    }
    if (SubmodeState == 7) {
        page = LinkCarChoiceGroup[player & 1];
        other = page ^ 1;
        row = ovl667_group_ui_rows[player & 1][(s16)other][0];
        if (UiRows[row].byte14 == 5) {
            other = page;
        }
    } else {
        other = LinkCarChoiceGroup[player & 1];
    }
    for (i = 0; i < 4; i++) {
        row = ovl667_group_ui_rows[player & 1][(s16)other][i];
        if (row == -1) {
            break;
        }
        DrawUiRow(row);
    }
    if (LinkMenuSyncPhase == 2) {
        row = ovl667_group_ui_rows[player & 1][1][3];
        DrawUiRow(row + 1);
        DrawUiRow(row + 2);
    }
    if (((u8)ImageRows->f1 - 1) < 2U) {
        drawModel = modelReady;
        if (drawModel == 1) {
            DrawSelectionIconGrid(ActivePlayerIndex);
            UpdateCarWorldTransform(&PlayerCars[ActivePlayerIndex & 1]);
            scale = 0x100;
            if (ImageRows->f1 == drawModel) {
                brightness = ~((ImageRows->f2 * 0xFF) / 30);
                color.channels.r = brightness;
                color.channels.g = brightness;
                color.channels.b = brightness;
                drawFlags = 0x102;
                scale = (ImageRows->f2 << 8) / 30;
            } else {
                drawFlags = 1;
                color.channels.r = 0x80;
                color.channels.g = 0x80;
                color.channels.b = 0x80;
            }
            DrawPreviewCar(&PlayerCars[ActivePlayerIndex & 1], color.argument, scale, drawFlags);
        }
    }
    UpdateBadgeRowAnim(0);
    UpdateImageRow(0);
    MenuFrameState.titlePlayer08 = ActivePlayerIndex + 1;
    UpdateScreenFade(LinkCarChoiceGroup[ActivePlayerIndex & 1] != 0 ? 0x10036 : 0x10032);
}

/* Initial values of the overlay globals this module owns. */
char ovl667_player_name_text[4][12] = {{80, 76, 65, 89, 69, 82, 32, 52}, {80, 76, 65, 89, 69, 82, 32, 51}, {80, 76, 65, 89, 69, 82, 32, 50}, {80, 76, 65, 89, 69, 82, 32, 49}};
char *ovl667_default_player_names[4] = {(char *)&ovl667_player_name_text[3][0], (char *)&ovl667_player_name_text[2][0], (char *)&ovl667_player_name_text[1][0], (char *)&ovl667_player_name_text[0][0]};
u8 ovl667_group_option_counts[4] = {3, 4};
s8 ovl667_group_slot_kinds[2][4] = {{0, 1, 2, -1}, {0, 2, 3, 1}};
s8 ovl667_group_ui_rows[2][2][4] = {{{0, 1, 2, -1}, {3, 4, 5, 6}}, {{9, 10, 11, -1}, {12, 13, 14, 15}}};

/* Writable state of this unit; ovl667_reset_module restores it. */
static const OverlayStateRegion ovl667_car_selection_regions[] = {
    RR_OVERLAY_STATE(ovl667_default_player_names),
    RR_OVERLAY_STATE(ovl667_group_option_counts),
    RR_OVERLAY_STATE(ovl667_group_slot_kinds),
    RR_OVERLAY_STATE(ovl667_group_ui_rows),
    RR_OVERLAY_STATE(ovl667_player_name_text)};
const OverlayStateTable ovl667_car_selection_state = {ovl667_car_selection_regions, sizeof(ovl667_car_selection_regions) / sizeof(ovl667_car_selection_regions[0])};

/* Initial values of the globals this module owns. */
MenuArenas8 MenuArenas;
CourseChoice4 LinkCarChoiceTable[2][2][9];
s16 LinkMenuSyncPhase;
s16 LinkCarChoiceCounts[2][2];
s16 LinkCarChoiceIndex[2];
s16 LinkClientRoleState;
s16 LinkCarChoiceGroup[2];
