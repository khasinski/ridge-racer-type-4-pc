#include "game/system/pad.h"
#include "rr/versus_names.h"
#include "game/data/menuvars.h"
#include "rr/overlay_module.h"
#include "rr/menu_text_rows.h"
#include "rr/overlay_menu.h"
#include "rr/frame_storage_types.h"

extern s32 TotalRaceTime;

extern s32 ovl661_result_labels[22];
extern s16 ovl661_finish_ranks[2];
extern s16 ovl661_player_standings[2];
extern s16 ovl661_player_win_counts[2];
extern char ovl661_wins_suffix_label[];
extern char ovl661_win_count_prefix[];

void CopyStringClearTail(char *, char *);
void CopyStringPadSpaces(char *, char *, s32);
void FormatDecimalDigits(char *, s32, s32);
#include "rr/screen_tiles.h"
void InitMenuArenas(void);
void UpdateScreenFade(s32);
void DrawResultsStandingsOverlay(s32);
void UpdateMenuTextRow(s32);
void DrawResultsRankIcons(void);
s32 LoadResultsSceneStep(void);
void SeqStartTrack(s32);
void ovl661_init_preview(s32);

void ovl661_init_results(void) {
    s32 i;

    InitMenuTextRows((MenuTextRow60 *)SharedWorkArena);
    MenuCursor = 0;
    MenuItemCount = 3;
    for (i = 0; i < 11; i++) {
        SetMenuTextRow(i, i, 0, 0, ovl661_result_labels[i]);
    }
    for (i = 11; i < 22; i++) {
        SetMenuTextRow(i, i - 11, 0, 0, -1);
    }
    for (i = 33; i < 44; i++) {
        SetMenuTextRow(i, i - 33, 2, 0, ovl661_result_labels[i - 22]);
    }
    if (TotalRaceTime >= 0) {
        if (ovl661_finish_ranks[0] > ovl661_finish_ranks[1]) {
            ovl661_player_standings[0] = 1;
            ovl661_player_standings[1] = 0;
        } else {
            ovl661_player_standings[0] = 0;
            ovl661_player_standings[1] = 1;
        }
        if (ovl661_player_win_counts[ovl661_player_standings[0]] < 99) {
            ovl661_player_win_counts[ovl661_player_standings[0]]++;
        }
    }
    ModeFrame = 0;
    SetMenuFrameOptions(0, 1, 0);
}

s32 ovl661_update_retry_choice(s32 mode){
 SetMenuTextRowStyle(MenuCursor+38,0,0);
 StepMenuValue(0,2,(char*)&MenuCursor,3,0);
 SetMenuTextRowStyle(MenuCursor+38,1,1);
 if(MenuReturnSubmodeIndex){PlaySfx(1);return MenuCursor;}
 return -1;
}

void ovl661_update_results(void) {
    s32 group;
    s32 i;
    s32 player;
    s32 ready;

    ModeFrame++;
    switch (SubmodeState) {
    case 0:
        if (TotalRaceTime == -1) {
            group = 1;
            if (MenuTextRows[11].signedBytes[2] >= 2) {
                SubmodeState = 3;
            }
            break;
        }
        group = 0;
        if (MenuTextRows[3].signedBytes[3] >= 17) {
            SetMenuFrameOptions(1, 1, group);
            if (MenuReturnSubmodeIndex != 0) {
                PlaySfx(1);
                SubmodeState = 3;
                SetMenuFrameOptions(group, 1, group);
            }
        }
        FormatDecimalDigits(MenuTextRows[4].marquee.at28 + 8, ovl661_player_standings[0] + 1, 0);
        CopyStringPadSpaces(MenuTextRows[5].marquee.at28 + 1, ovl661_player_names[ovl661_player_standings[0]], 9);
        CopyStringClearTail(MenuTextRows[5].marquee.at28 + 10, ovl661_wins_suffix_label);
        for (i = 0; i < 2; i++) {
            player = ovl661_player_standings[i];
            CopyStringPadSpaces(MenuTextRows[7 + i].marquee.at28 + 5, ovl661_player_names[player], 9);
            CopyStringClearTail(MenuTextRows[7 + i].marquee.at28 + 14, ovl661_win_count_prefix);
            FormatDecimalDigits(MenuTextRows[7 + i].marquee.at28 + 18, ovl661_player_win_counts[player], 1);
        }
        break;
    case 3:
        group = 3;
        if (MenuTextRows[39].signedBytes[3] >= 17) {
            SetMenuFrameOptions(1, 1, 0);
            if (ovl661_update_retry_choice(0) >= 0) {
                ResetScreenCounters();
                InitMenuArenas();
                FadeCountdown = 64;
                SubmodeState = MenuCursor + 4;
                SetMenuFrameOptions(0, 1, 0);
            }
        }
        break;
    case 4:
        FadeCountdown--;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (64 - FadeCountdown) * 4, 69);
        group = 3;
        if (FadeCountdown <= 0) {
            MenuScriptId = 0;
            ovl661_finish_ranks[0] = -1;
            ModeGroup = 1;
            ModeIndex = 35;
            SubmodeState = 0;
        }
        break;
    case 5:
        FadeCountdown -= 2;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (64 - FadeCountdown) * 4, 69);
        group = 3;
        if (FadeCountdown <= 0) {
            group = -1;
            SubmodeState = 8;
        }
        break;
    case 6:
        FadeCountdown--;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (64 - FadeCountdown) * 4, 69);
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
            ovl661_init_preview(0);
            ModeGroup = ready;
            ModeIndex = 56;
            SubmodeState = 0;
        }
        return;
    }
    UpdateScreenFade(0x1000C);
    DrawResultsStandingsOverlay(ModeFrame);
    if (group >= 0) {
        for (i = 0; i < 11; i++) {
            UpdateMenuTextRow(i + group * 11);
        }
    }
    if ((u8)(MenuTextRows[0].bytes[2] - 3) < 5 || (u8)(MenuTextRows[11].bytes[2] - 3) < 5) {
        DrawResultsRankIcons();
    }
}

/* Initial values of the overlay globals this module owns. */
char ovl661_wins_suffix_label[12] = {87, 79, 78, 32, 33, 33, 33};
char ovl661_win_count_prefix[44] = {87, 73, 78, 32, 0, 0, 0, 0, -8, 126, 17, -128, -100, -126, 17, -128, -100, -126, 17, -128, -112, -128, 17, -128, 24, -127, 17, -128, -116, -127, 17, -128, -24, -127, 17, -128, -100, -126, 17, -128, 84, -126, 17, -128};
s32 ovl661_result_labels[22] = {7702, 0, -1, 3, 25, 25, -1, 4633, 4889, -1, 7702, 7702, 17, 7702, 7702, -1, 6, 7, 9, -1, 7702, -1};
s16 ovl661_player_standings[2];
s16 ovl661_player_win_counts[2];
s16 ovl661_finish_ranks[2];

/* Writable state of this unit; ovl661_reset_module restores it. */
static const OverlayStateRegion ovl661_results_regions[] = {
    RR_OVERLAY_STATE(ovl661_finish_ranks),
    RR_OVERLAY_STATE(ovl661_player_standings),
    RR_OVERLAY_STATE(ovl661_player_win_counts),
    RR_OVERLAY_STATE(ovl661_result_labels),
    RR_OVERLAY_STATE(ovl661_win_count_prefix),
    RR_OVERLAY_STATE(ovl661_wins_suffix_label)};
const OverlayStateTable ovl661_results_state = {ovl661_results_regions, sizeof(ovl661_results_regions) / sizeof(ovl661_results_regions[0])};
