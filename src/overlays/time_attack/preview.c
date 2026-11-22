#include "rr/overlay_module.h"
#include "game/system/pad.h"
#include "rr/overlay660.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"

void ovl660_init_preview(void)
{
    RaceVehicle *state;
    ProjectionScratchPrefix *scratch;

    ModeFrame = 0;
    ScreenFrame = 0;
    SetupRenderer(5);
    MenuCursor = 0;
    MenuItemCount = 3;
    ResetScreenCounters();
    InitPreviewImageRows();
    state = PlayerCars;
    scratch = RR_PROJECTION_SCRATCH;
    scratch->pose08.angles10.xyz.x = 120;
    scratch->pose08.position00.words[0] = -16;
    scratch->pose08.position00.words[1] = -248;
    scratch->pose08.position00.words[2] = -96;
    scratch->pose08.angles10.xyz.y = 0;
    scratch->pose08.angles10.xyz.z = 0;
    LoadCursor = MenuArenaBase;
    ResetCarState(state);
    state->position20.words[1] = 10;
    state->position20.words[2] = 308;
    state->rot54 = 2048;
    ovl660_preview_confirmation = 0;
    UnpackChoiceGlyph((s8 *)&SelectionRows, CourseChoiceTable[ovl660_preview_bank][ovl660_preview_selection].glyph);
    PlayerCourseChoices[0].first = CourseChoiceTable[ovl660_preview_bank][ovl660_preview_selection].first;
    PlayerCourseChoices[0].second = CourseChoiceTable[ovl660_preview_bank][ovl660_preview_selection].second;
    ovl660_available_car_counts[0] = CountChoiceGlyphs(&CourseChoiceTable[0][0]);
    ovl660_available_car_counts[1] = CountChoiceGlyphs(&CourseChoiceTable[1][0]);
}

void ovl660_update_preview(void) {
    PreviewColor color;
    s32 drawFlags;
    s32 scale;
    s8 modelReady;
    s32 drawModel;
    s32 rowIndex;
    s32 first;
    s32 loadResult;
    u32 previousCar;
    s32 other;
    s32 selectionState;
    u8 confirmationState;
    u8 closingState;
    u8 headerState;
    s8 brightness;

    modelReady = 0;
    ModeFrame = ModeFrame + 1U;
    PlayerCars[0].rot54 = (PlayerCars[0].rot54 + 8U) & 0xFFF;
    SetupMenuCarCamera(1);
    SetupViewport(6);
    loadResult = LoadCarRangeStep(0, 1);
    if (loadResult != 1) {
        if (ImageRows->f1 == 1) {
            ImageRows->f1 = 0;
        }
    } else {
        SetCarBounds(PlayerCars);
        if (ImageRows->f1 == 7) {
            ImageRows->f1 = loadResult;
        }
        modelReady = 1;
        switch (SubmodeState) {
        case 7:
            if (UiRows[(s16)(ovl660_preview_bank ^ 1) * 3 + 4].byte14 == 5) {
                SubmodeState = 0;
            }
        case 0:
            SetMenuFrameOptions(1, 1, 1);
            previousCar = PackChoiceGlyph((s8 *)&SelectionRows);
            StepMenuValue(1, 2, (char *)&ovl660_preview_selection,
                          ovl660_available_car_counts[ovl660_preview_bank], 0);
            UnpackChoiceGlyph((s8 *)&SelectionRows,
                          CourseChoiceTable[ovl660_preview_bank][ovl660_preview_selection].glyph);
            if ((previousCar << 16) != (PackChoiceGlyph((s8 *)&SelectionRows) << 16)) {
                ResetScreenCounters();
                LoadCursor = MenuArenaBase;
                UnpackChoiceGlyph((s8 *)&SelectionRows,
                              CourseChoiceTable[ovl660_preview_bank][ovl660_preview_selection].glyph);
                PlayerCourseChoices[0].first = CourseChoiceTable[ovl660_preview_bank][ovl660_preview_selection].first;
                PlayerCourseChoices[0].second = CourseChoiceTable[ovl660_preview_bank][ovl660_preview_selection].second;
                ImageRows->f1 = 6;
            }
            UiRows[ovl660_preview_row_map[ovl660_preview_bank][MenuCursor]].cells[0] = 0;
            StepMenuValue(0, 2, (char *)&MenuCursor, MenuItemCount, 0);
            UiRows[ovl660_preview_row_map[ovl660_preview_bank][MenuCursor]].cells[0] = 2;
            selectionState = UiRows[ovl660_preview_bank * 3 + 4].byte14;
            if (selectionState != 2) {
                break;
            }
            if (PlayerControllers[0].action28 != 0) {
                UiRows[ovl660_preview_row_map[ovl660_preview_bank][MenuCursor]].cells[0] = 1;
                if (MenuCursor == 0) {
                    PlaySfx(1);
                    ovl660_preview_confirmation = selectionState;
                    SubmodeState = 4;
                } else if (MenuCursor == 1 && CourseChoiceTable[1][0].glyph != -1) {
                    PlaySfx(1);
                    ovl660_preview_selection = 0;
                    rowIndex = 0;
                    do {
                        if (UiRows[ovl660_preview_bank * 3 + rowIndex + 4].byte14 == 2) {
                            UiRows[ovl660_preview_bank * 3 + rowIndex + 4].byte14 = 3;
                        }
                        UiRows[(s16)(ovl660_preview_bank ^ 1) * 3 + rowIndex + 4].byte14 = 0;
                        rowIndex += 1;
                    } while (rowIndex < 3);
                    SubmodeState = 7;
                    MenuCursor = 0;
                    ovl660_preview_bank ^= 1;
                } else if (MenuCursor == 2) {
                    PlaySfx(1);
                    rowIndex = 0;
                    do {
                        if (UiRows[ovl660_preview_bank * 3 + rowIndex + 4].byte14 == 2) {
                            UiRows[ovl660_preview_bank * 3 + rowIndex + 4].byte14 = 3;
                        }
                        rowIndex += 1;
                    } while (rowIndex < 3);
                    goto close_selection;
                }
            } else if (PlayerControllers[0].action2C != 0) {
                PlaySfx(2);
                rowIndex = 0;
                do {
                    if (UiRows[ovl660_preview_bank * 3 + rowIndex + 4].byte14 == 2) {
                        UiRows[ovl660_preview_bank * 3 + rowIndex + 4].byte14 = 3;
                    }
                    rowIndex += 1;
                } while (rowIndex < 3);
            close_selection:
                ImageRows->f1 = 3;
                SubmodeState = 5;
                BadgeRows->state10 = 4;
            }
            break;
        case 3:
            FadeCountdown = (u16)FadeCountdown - 1;
            SetModelTexturePage(-1);
            AddScreenFadeTiles(&CurrentDrawBuf->ordering.ordering70.tags[0], (0x40 - FadeCountdown) * 4, 0x45);
            if (FadeCountdown <= 0) {
                SeqMuteVolume();
                MenuScriptId = 0;
                ModeGroup = 1;
                ModeIndex = 0x23;
                SubmodeState = 0;
                goto save_preview_arena;
            }
            break;
        case 4:
            confirmationState = UiRows[10].byte14;
            if (confirmationState == 2) {
                UiRows[SelectionRows.selection.pad02[1] + 10].cells[0] = 0;
                StepMenuValue(0, 1, (char *)&SelectionRows.selection.pad02[1], 2, 0);
                UiRows[SelectionRows.selection.pad02[1] + 10].cells[0] = confirmationState;
                if (PlayerControllers[0].action28 != 0) {
                    PlaySfx(1);
                    SetMenuFrameOptions(0, 1, 1);
                    UiRows[SelectionRows.selection.pad02[1] + 10].cells[0] = 1;
                    UiRows[10].byte14 = 7;
                    UiRows[11].byte14 = 7;
                    SubmodeState = 3;
                    FadeCountdown = 0x40;
                    SeqFadeVolume(0, 0x3C);
                save_preview_arena:
                    MenuArenas.first = LoadCursor;
                } else if (PlayerControllers[0].action2C != 0) {
                    PlaySfx(2);
                    UiRows[10].byte14 = 3;
                    UiRows[11].byte14 = 3;
                    SubmodeState = 6;
                }
            }
            break;
        case 6:
            if (UiRows[10].byte14 == 5) {
                SubmodeState = 0;
                ovl660_preview_confirmation = 1;
                UiRows[10].byte14 = 0;
                UiRows[11].byte14 = 0;
            }
            break;
        case 5:
            closingState = UiRows[ovl660_preview_bank * 3 + 4].byte14;
            if (closingState == 5 && ImageRows->f1 == closingState) {
                ResetScreenCounters();
                ovl660_init_menu();
                ModeGroup = 1;
                ModeIndex = 50;
                SubmodeState = 0;
                goto end;
            }
            break;
        }
    }
    if (ovl660_preview_confirmation == 0 && ImageRows->f1 == 2) {
        rowIndex = 0;
        do {
            if (UiRows[rowIndex].byte14 == 9) {
                UiRows[rowIndex].byte14 = 3;
            }
            rowIndex += 1;
        } while (rowIndex < 4);
    }
    headerState = UiRows[0].byte14;
    if (headerState == 5) {
        if (ovl660_preview_confirmation == 0) {
            ovl660_preview_confirmation = 1;
            BadgeRows->half02 = 1;
        }
        if (SubmodeState == 7) {
            other = (s16)(ovl660_preview_bank ^ 1);
            if (UiRows[other * 3 + 4].byte14 == headerState) {
                first = ovl660_preview_bank * 3 + 4;
            } else {
                first = other * 3 + 4;
            }
        } else {
            first = ovl660_preview_bank * 3 + 4;
        }
        rowIndex = 0;
        do {
            DrawUiRow(first + rowIndex);
            rowIndex += 1;
        } while (rowIndex < 3);
        if (ovl660_preview_confirmation == 2) {
            DrawUiRow(10);
            DrawUiRow(11);
        }
    }
    drawModel = modelReady;
    if ((((u8)ImageRows->f1 - 1) < 2U) && (drawModel == 1)) {
        DrawSelectionIconGrid(0);
        UpdateCarWorldTransform(PlayerCars);
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
        DrawPreviewCar(PlayerCars, color.argument, scale, drawFlags);
    }
    UpdateBadgeRowAnim(0);
    if (UpdateImageRow(0) > 0) {
        DrawMenuStepArrows(0xA0, 0x90, 8, 1, ovl660_preview_selection, 0,
                      ovl660_available_car_counts[ovl660_preview_bank]);
    }
end:
    UpdateScreenFade(ovl660_preview_bank != 0 ? 0x10036 : 0x10032);
}

/* Initial values of the overlay globals this module owns. */
u8 ovl660_preview_row_map[2][4] = {{4, 5, 6}, {7, 8, 9}};
s16 ovl660_available_car_counts[2];
s16 ovl660_preview_confirmation;
s16 ovl660_preview_selection;
s16 ovl660_preview_bank;

/* Writable state of this unit; ovl660_reset_module restores it. */
static const OverlayStateRegion ovl660_preview_regions[] = {
    RR_OVERLAY_STATE(ovl660_available_car_counts),
    RR_OVERLAY_STATE(ovl660_preview_bank),
    RR_OVERLAY_STATE(ovl660_preview_confirmation),
    RR_OVERLAY_STATE(ovl660_preview_row_map),
    RR_OVERLAY_STATE(ovl660_preview_selection)};
const OverlayStateTable ovl660_preview_state = {ovl660_preview_regions, sizeof(ovl660_preview_regions) / sizeof(ovl660_preview_regions[0])};
