#include "rr/release.h"
#include "rr/overlay_module.h"
#include "rr/player_cars.h"
#include "game/system/pad.h"
#include "rr/overlay660.h"
#include "rr/frame_storage_types.h"

void ovl660_init_results(void)
{
    s32 i;
    s32 off;
    s32 mode;

    ModeFrame = 0;
    if (TotalRaceTime != -1) {
        ovl660_rank_standings();
        ovl660_race_record_rank = ovl660_insert_record(0, LapTimes[(s16)PlayerCars[0].control.dynamics94.steering.finish20]);
        ovl660_lap_record_rank = ovl660_insert_record(1, TotalRaceTime);
    } else {
        ovl660_race_record_rank = 6;
        ovl660_lap_record_rank = 6;
    }
    InitMenuTextRows((MenuTextRow60 *)SharedWorkArena);
    MenuCursor = 0;
    MenuItemCount = 3;
    off = 0;
    if (CourseIndex == 7) {
        off = 11;
    }
    for (i = 0; i < 44; i++) {
        SetMenuTextRow(i, i % 11, (i >= 11) * 2, 0, ovl660_result_labels[off + i]);
        if (i == 10) {
            off = 11;
        }
    }
    for (i = 44; i < 55; i++) {
        SetMenuTextRow(i, i - 44, 0, 0, -1);
    }
    ovl660_result_kind = 0;
    mode = ResultNextActionMode;
    if (mode == 2) {
        ovl660_result_kind = mode;
    } else if (mode == 1) {
        ovl660_result_kind = mode;
    }
    ovl660_result_transition = 0;
    SetMenuFrameOptions(0, 1, 0);
}

s32 ovl660_update_retry_choice(s32 reserved){
 SetMenuTextRowStyle(MenuCursor+37,0,0);
 StepMenuValue(0,2,(char*)&MenuCursor,MenuItemCount,0);
 SetMenuTextRowStyle(MenuCursor+37,1,1);
 if(MenuReturnSubmodeIndex){PlaySfx(1);return MenuCursor;}
 return -1;
}

void ovl660_update_results(void) {
    s32 group;
    s32 base;
    s32 row;
    s32 i;
    s16 timer;
    s32 ready;

    ModeFrame++;
    switch (SubmodeState) {
    case 0:
        if (TotalRaceTime == -1) {
            group = 44;
            if (MenuTextRows[44].signedBytes[2] >= 2) {
                SubmodeState = 3;
            }
            break;
        }
        group = 0;
        if (MenuTextRows[1].signedBytes[3] >= 15) {
            SetMenuFrameOptions(1, 1, 0);
            if (MenuReturnSubmodeIndex != 0) {
                PlaySfx(1);
                ovl660_name_cursor = 0;
                SubmodeState++;
                SetMenuFrameOptions(0, 1, 0);
            }
        }
        base = 5;
        if (CourseIndex == 7) {
            base = 3;
        }
        for (i = 0; i < NumLaps; i++) {
            row = base + i;
            FormatDecimalDigits((char *)&MenuTextRows[row].bytes[32], i + 1, 0);
            FormatRaceTime((char *)&MenuTextRows[row].bytes[35], LapTimes[i]);
            if (i == PlayerCars[0].control.dynamics94.steering.finish20 && ovl660_race_record_rank < 5) {
                SetMenuTextRowStyle(row, 1, -1);
                CopyStringClearTail((char *)&MenuTextRows[row].bytes[45], ovl660_new_record_label);
            }
        }
        base = 9;
        if (CourseIndex == 7) {
            base = 10;
        }
        if (ovl660_lap_record_rank < 5) {
            SetMenuTextRowStyle(base, 1, -1);
            CopyStringClearTail((char *)&MenuTextRows[base].bytes[45], ovl660_new_record_label);
        }
        break;
    case 1:
        group = 11;
        if (ovl660_race_record_rank < 5) {
            StepMenuValue(1, 4, (char *)&ovl660_name_cursor, 8, 0);
            StepNameChar(&ovl660_record_name[ovl660_name_cursor]);
            {
                s32 entry = ovl660_race_record_rank + 15;
                CopyStringClearTail((char *)&MenuTextRows[entry].bytes[42], ovl660_record_name);
            }
            CopyStringClearTail(LapRecords[ReverseCourse][CourseIndex][ovl660_race_record_rank].name, ovl660_record_name);
            SetMenuTextRowStyle(ovl660_race_record_rank + 15, 1, -((ovl660_name_cursor + 15) << 8));
        }
        if (MenuTextRows[15].signedBytes[3] >= 15) {
            SetMenuFrameOptions(1, 1, 0);
            if (MenuReturnSubmodeIndex != 0) {
                PlaySfx(1);
                ovl660_name_cursor = 0;
                SubmodeState++;
                SetMenuFrameOptions(0, 1, 0);
            }
        }
        break;
    case 2:
        group = 22;
        if (ovl660_lap_record_rank < 5) {
            StepMenuValue(1, 4, (char *)&ovl660_name_cursor, 8, 0);
            StepNameChar(&ovl660_record_name[ovl660_name_cursor]);
            {
                s32 entry = ovl660_lap_record_rank + 26;
                CopyStringClearTail((char *)&MenuTextRows[entry].bytes[42], ovl660_record_name);
            }
            CopyStringClearTail(RaceRecords[ReverseCourse][CourseIndex][ovl660_lap_record_rank].name, ovl660_record_name);
            SetMenuTextRowStyle(ovl660_lap_record_rank + 26, 1, -((ovl660_name_cursor + 15) << 8));
        }
        if (MenuTextRows[15].signedBytes[3] >= 15) {
            SetMenuFrameOptions(1, 1, 0);
            if (MenuReturnSubmodeIndex != 0) {
                PlaySfx(1);
                SubmodeState++;
                SetMenuFrameOptions(0, 1, 0);
            }
        }
        break;
    case 3:
        group = 33;
        if (MenuTextRows[38].signedBytes[3] >= 17) {
            SetMenuFrameOptions(1, 1, 0);
            if (ovl660_update_retry_choice(0) >= 0) {
                SetMenuFrameOptions(0, 1, 0);
                ResetScreenCounters();
                InitMenuArenas();
                FadeCountdown = 64;
                SubmodeState = MenuCursor + 4;
            }
        } else {
            SetMenuFrameOptions(0, 1, 0);
        }
        if (TotalRaceTime != -1) {
            if (ovl660_result_transition == 1) {
                ready = PackChoiceGlyph((s8 *)&SelectionRows);
                MenuTextRows[42].bytes[28] = 34;
                EncodeRecordPassword(SelectedCourseChoice, ready, TotalRaceTime, ovl660_result_kind, Options.storyBranch3C,
                              &MenuTextRows[42].bytes[29]);
                /* The Japanese password is one character shorter and is
                 * encoded again every frame. */
                MenuTextRows[42].bytes[RR_PAL_JP(42, 41)] = 34;
                if (!RR_RELEASE_JP)
                    ovl660_result_transition = 2;
            } else if (ovl660_result_transition == 0 && (PlayerControllers[0].input00.word & 0x900000) == 0x900000) {
                ovl660_result_transition = 1;
            }
        }
        break;
    case 4:
        timer = (u16)FadeCountdown - 1;
        FadeCountdown = timer;
        AddScreenFadeTiles(&CurrentDrawBuf->ordering.ordering70.tags[0], (64 - timer) * 4, 69);
        group = 33;
        if (FadeCountdown <= 0) {
            MenuScriptId = 0;
            ModeGroup = 1;
            ModeIndex = 35;
            SubmodeState = 0;
        }
        break;
    case 5:
        timer = (u16)FadeCountdown - 2;
        FadeCountdown = timer;
        AddScreenFadeTiles(&CurrentDrawBuf->ordering.ordering70.tags[0], (64 - timer) * 4, 69);
        group = 33;
        if (FadeCountdown <= 0) {
            group = -1;
            SubmodeState = 8;
        }
        break;
    case 6:
        timer = (u16)FadeCountdown - 1;
        FadeCountdown = timer;
        AddScreenFadeTiles(&CurrentDrawBuf->ordering.ordering70.tags[0], (64 - timer) * 4, 69);
        group = -1;
        if (FadeCountdown <= 0) {
            ModeGroup = 1;
            ModeIndex = 3;
            SubmodeState = 0;
        }
        break;
    case 8:
        ready = LoadResultsSceneStep();
        if (ready == 1) {
            SeqStartTrack(0);
            ovl660_init_menu();
            ModeGroup = ready;
            ModeIndex = 50;
            SubmodeState = 0;
        }
        return;
    }
    UpdateScreenFade(0x1000C);
    DrawResultsStandingsOverlay(ModeFrame);
    if (group >= 0) {
        i = 0;
        do {
            UpdateMenuTextRow(i + group);
            i++;
        } while (i < 11);
    }
    if ((MenuTextRows[0].bytes[2] - 3) < 5U || (MenuTextRows[44].bytes[2] - 3) < 5U) {
        DrawResultsRankIcons();
    }
}

s32 ovl660_insert_record(s32 mode, s32 time) {
    s32 times[6];
    s16 order[6];
    s32 i;
    s32 j;
    s16 o;

    times[5] = time;
    order[5] = 5;
    for (i = 0; i < 5; i++) {
        order[i] = i;
        if (mode == 1) {
            times[i] = RaceRecords[ReverseCourse][CourseIndex][i].time;
        } else {
            times[i] = LapRecords[ReverseCourse][CourseIndex][i].time;
        }
    }
    for (i = 0; i < 5; i++) {
        for (j = 5; i < j; j--) {
            s32 earlier = times[j - 1];
            s32 later = times[j];
            if (later < earlier) {
                o = order[j];
                times[j] = earlier;
                times[j - 1] = later;
                order[j] = order[j - 1];
                order[j - 1] = o;
            }
        }
    }
    for (i = 0; i < 5; i++) {
        if (order[i] == 5) {
            for (j = 4; i < j; j--) {
                if (mode == 1) {
                    RaceRecords[ReverseCourse][CourseIndex][j] = RaceRecords[ReverseCourse][CourseIndex][j - 1];
                } else {
                    LapRecords[ReverseCourse][CourseIndex][j] = LapRecords[ReverseCourse][CourseIndex][j - 1];
                }
            }
            if (mode == 1) {
                RaceRecords[ReverseCourse][CourseIndex][i].car = PackChoiceGlyph((s8 *)&SelectionRows);
                RaceRecords[ReverseCourse][CourseIndex][i].time = times[i];
                CopyStringClearTail(RaceRecords[ReverseCourse][CourseIndex][i].name, ovl660_record_name);
            } else {
                LapRecords[ReverseCourse][CourseIndex][i].car = PackChoiceGlyph((s8 *)&SelectionRows);
                LapRecords[ReverseCourse][CourseIndex][i].time = times[i];
                CopyStringClearTail(LapRecords[ReverseCourse][CourseIndex][i].name, ovl660_record_name);
            }
            return i;
        }
    }
    return i;
}

void ovl660_rank_standings(void)
{
    s32 values[6];
    u16 indices[6];
    s32 n;
    s32 *destination, *source;
    u16 *order;
    s32 i;
    s32 j, previous;
    s32 *left, *right;
    s32 l, r;
    u16 *slot, *prior;
    u16 index;
    n = 6;
    i = 0;
    destination = values; source = LapTimes; order = indices;
    for (; i < 6; destination++, source++, i++, order++) {
        *order = i;
        *destination = *source;
        if (*destination < 0) { n = i; break; }
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
    PlayerCars[0].control.dynamics94.steering.finish20 = indices[0]; /* HudMapZoomMode 0x800AC1A0 */
}

/* Initial values of the overlay globals this module owns. */
char ovl660_new_record_label[80] = {78, 69, 87, 33, 33, 0, 0, 0, 0, 0, 0, 0, -108, 87, 17, -128, -108, 89, 17, -128, -16, 90, 17, -128, 68, 92, 17, -128, -124, 93, 17, -128, -20, 93, 17, -128, 72, 94, 17, -128, -4, 94, 17, -128, -76, 94, 17, -128, 116, 102, 17, -128, -116, 109, 17, -128, -116, 109, 17, -128, -12, 106, 17, -128, 116, 107, 17, -128, 4, 109, 17, -128, -60, 108, 17, -128, 44, 102, 17, -128};
s32 ovl660_result_labels[55] = {7702, 3852, 7702, 0, -1, 269, 525, 781, 20, 1038, 7702, 7702, 3852, 0, 269, 525, 781, 269, 525, 781, 20, 1038, 7702, 3852, 1, 7702, 1296, 1552, 1808, 2064, 2320, 7702, -1, 7702, 3852, 2, 7702, 2576, 2832, 3088, 3344, 3600, 7702, -1, 7702, 17, 7702, -1, 6, 7, 9, -1, 7702, -1, 7702};
s32 ovl660_race_record_rank;
s32 ovl660_name_cursor;
s16 ovl660_result_kind;
char ovl660_record_name[12];
s32 ovl660_result_transition;
s32 ovl660_lap_record_rank;

/* Writable state of this unit; ovl660_reset_module restores it. */
static const OverlayStateRegion ovl660_results_regions[] = {
    RR_OVERLAY_STATE(ovl660_lap_record_rank),
    RR_OVERLAY_STATE(ovl660_name_cursor),
    RR_OVERLAY_STATE(ovl660_new_record_label),
    RR_OVERLAY_STATE(ovl660_race_record_rank),
    RR_OVERLAY_STATE(ovl660_record_name),
    RR_OVERLAY_STATE(ovl660_result_kind),
    RR_OVERLAY_STATE(ovl660_result_labels),
    RR_OVERLAY_STATE(ovl660_result_transition)};
const OverlayStateTable ovl660_results_state = {ovl660_results_regions, sizeof(ovl660_results_regions) / sizeof(ovl660_results_regions[0])};

/* Initial values of the globals this module owns. */
s16 ReverseCourse;
TimeRecord16 RaceRecords[2][8][5];
TimeRecord16 LapRecords[2][8][5];
