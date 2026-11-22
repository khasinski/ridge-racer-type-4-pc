#include "game/system/pad.h"
#include "rr/player_cars.h"
#include "rr/save_slots.h"
#include "rr/overlay659.h"
#include "rr/frame_storage_types.h"

void ovl659_init_results(void) {
    s32 i;
    s32 offset;

    ovl659_rank_standings();
    InitMenuTextRows((MenuTextRow60 *)SharedWorkArena);
    MenuCursor = 0;
    MenuItemCount = 2;
    offset = 0;
    if (SelectionState.fields.mode1A != 4) {
        offset = 11;
    }
    for (i = 0; i < 11; i++) {
        if (SelectionState.fields.phase17 >= 4 && i == 4 && offset == 0) {
            SetMenuTextRow(i, i, 0, 0, ovl659_result_labels[22]);
        } else {
            SetMenuTextRow(i, i, 0, 0, ovl659_result_labels[i + offset]);
        }
    }
    for (i = 0; i < 11; i++) {
        SetMenuTextRow(i + 33, i, 2, 0, ovl659_result_labels[i + 23]);
    }
    SetMenuFrameOptions(0, 1, 0);
    ovl659_preview_timer = -2;
    ModeFrame = 0;
}

void ovl659_update_results(void) {
    s32 group;
    s32 lapOffset;
    s32 rowOffset;
    s32 i;
    s16 timer;
    s32 ready;

    ModeFrame = (ModeFrame + 1U);
    switch (SubmodeState) {
    case 0:
        group = 0;
        if (MenuTextRows[3].signedBytes[3] >= 16) {
            SetMenuFrameOptions(1, 1, 0);
            if (MenuReturnSubmodeIndex != 0) {
                SetMenuFrameOptions(0, 1, 0);
                PlaySfx(1);
                if (SelectionState.fields.mode1A == 4) {
                    InitMenuArenas();
                    ResetScreenCounters();
                    SubmodeState = 5;
                    break;
                } else {
                    SubmodeState = 3;
                }
            }
        }
        lapOffset = 0;
        if (CourseIndex == 7) {
            if (rsin((ModeFrame & 0xFF) << 4) > 0) {
                lapOffset = 3;
            }
        }
        rowOffset = 6;
        if (SelectionState.fields.mode1A != 4) {
            rowOffset = 5;
        }
        for (i = 0; i < 3; i++) {
            if (i >= NumLaps) {
                MenuTextRows[rowOffset + 2].state.d34 = -1;
                break;
            }
            if (i + lapOffset == PlayerCars[0].control.dynamics94.steering.finish20) {
                SetMenuTextRowStyle(i + rowOffset, 1, -1);
            } else {
                SetMenuTextRowStyle(i + rowOffset, 0, -1);
            }
            FormatDecimalDigits((char *)&MenuTextRows[i + rowOffset].bytes[32], i + lapOffset + 1, 0);
            FormatRaceTime((char *)&MenuTextRows[i + rowOffset].bytes[35], LapTimes[i + lapOffset]);
        }
        break;
    case 3:
        group = 1;
        if (SelectionState.fields.retries08[SelectionState.fields.phase17] == 0) {
            SubmodeState = 7;
        } else if (MenuTextRows[38].signedBytes[3] >= 14) {
            SetMenuFrameOptions(1, 1, 0);
            SetMenuTextRowStyle(MenuCursor + 37, 0, 0);
            StepMenuValue(0, 2, (char *)&MenuCursor, MenuItemCount, 0);
            SetMenuTextRowStyle(MenuCursor + 37, 1, 1);
            if (MenuReturnSubmodeIndex != 0) {
                SetMenuFrameOptions(0, 1, 0);
                PlaySfx(1);
                if (MenuCursor == 0) {
                    SubmodeState = 4;
                    FadeCountdown = 64;
                    SelectionState.fields.retries08[SelectionState.fields.phase17]--;
                    FormatDecimalDigits((char *)&MenuTextRows[40].bytes[36],
                                  SelectionState.fields.retries08[SelectionState.fields.phase17], 0);
                } else {
                    SubmodeState = 7;
                }
            }
        }
        break;
    case 4:
        timer = (u16)FadeCountdown - 1;
        FadeCountdown = timer;
        AddScreenFadeTiles(&CurrentDrawBuf->ordering.ordering70.tags[0], (64 - timer) * 4, 69);
        group = 1;
        if (FadeCountdown <= 0) {
            MenuScriptId = 0;
            ModeGroup = 1;
            ModeIndex = 35;
            SubmodeState = 0;
        }
        break;
    case 5:
        group = 0;
        for (i = 0; i < 11; i++) {
            if (MenuTextRows[i].signedBytes[2] < 8) {
                MenuTextRows[i].signedBytes[2] = 8;
                MenuTextRows[i].signedBytes[0] = 0;
            }
        }
        if (MenuTextRows[0].signedBytes[2] == 9) {
            SubmodeState = 6;
            ovl659_preview_timer = 64;
        }
        break;
    case 6:
        SetMenuFrameOptions(0, 1, 0);
        group = -1;
        if (ovl659_preview_timer > 0) {
            ovl659_preview_timer--;
        }
        ready = LoadResultsSceneStep();
        if (ready == 1 && ovl659_preview_timer <= 0) {
            if (SelectionState.fields.phase17 == 7) {
                UpdateSaveProgress();
                ModeGroup = 0;
                ModeIndex = 3;
            } else {
                ResetScreenCounters();
                SeqStartTrack(1);
                ModeGroup = ready;
                ModeIndex = 11;
            }
            SubmodeState = 0;
            UpdateScreenFade(0x1000C);
            return;
        }
        break;
    case 7:
        group = 0;
        for (i = 0; i < 11; i++) {
            if (MenuTextRows[i].signedBytes[2] < 8) {
                MenuTextRows[i].signedBytes[2] = 8;
                MenuTextRows[i].signedBytes[0] = 0;
            }
        }
        if (MenuTextRows[0].signedBytes[2] == 9) {
            SubmodeState = 8;
        }
        break;
    case 8:
        InitMenuArenas();
        ModeGroup = 1;
        ModeIndex = 31;
        SubmodeState = 0;
        UpdateScreenFade(0x1000C);
        return;
    default:
        break;
    }
    UpdateScreenFade(0x1000C);
    if (ovl659_preview_timer == -2) {
        DrawResultsStandingsOverlay(ModeFrame);
    } else {
        DrawResultsStandingsOverlay(ovl659_preview_timer);
    }
    if (group >= 0) {
        i = 0;
        do {
            UpdateMenuTextRow(i + group * 33);
            i++;
        } while (i < 11);
    }
    if (MenuTextRows[0].signedBytes[2] >= 3 && SubmodeState < 6) {
        DrawResultsRankIcons();
    }
}

void ovl659_rank_standings(void) {
    s32 values[6];
    u16 indices[6];
    s32 n, rank;
    s32 *destination, *source;
    u16 *order;
    s32 i;
    s32 j, previous;
    s32 *left, *right;
    s32 l, r;
    u16 *slot, *prior;
    u16 index;
    RaceVehicle *vehicle;
    n = 6;
    i = 0;
    destination = values;
    source = LapTimes;
    order = indices;
    for (; i < 6; destination++, source++, i++, order++) {
        *order = i;
        *destination = *source;
        if (*destination < 0) {
            n = i;
            break;
        }
    }
    for (i = 0; i < n - 1; i++) {
        for (j = n - 1; j > i; j = previous) {
            previous = j - 1;
            left = &values[previous];
            right = &values[j];
            l = *left;
            r = *right;
            if (r < l) {
                slot = &indices[j];
                index = *slot;
                *right = l;
                prior = &indices[previous];
                *left = r;
                *slot = *prior;
                *prior = index;
            }
        }
    }
    rank = TotalRaceTime;
    if (rank == -1) {
        vehicle = PlayerCars;
        vehicle->control.dynamics94.steering.finish20 = rank;
        if (vehicle->control.dynamics94.steering.count22 > 0)
            LapTimes[vehicle->control.dynamics94.steering.count22 - 1] = rank;
    } else {
        PlayerCars[0].control.dynamics94.steering.finish20 = indices[0]; /* HudMapZoomMode 0x800AC1A0 */
    }
}
