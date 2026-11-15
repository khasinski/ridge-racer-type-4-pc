#include "game/system/pad.h"
#include "game/data/menuvars.h"
#include "rr/overlay_module.h"
#include "rr/vehicle_table.h"
#include "game/race/racegrid.h"
#include "rr/menu_text_rows.h"
#include "rr/overlay_menu.h"
#include "rr/frame_storage_types.h"

extern s16 MenuCursor;
extern s16 MenuItemCount;
extern s32 ovl667_result_labels[22];
extern s32 TotalRaceTime;
extern s16 LinkFinishCarOrder[4];
extern s16 LinkPlayerWinCounts[4];
extern s16 LinkState;
extern void SetMenuFrameOptions(s32, s32, s32);

void ovl667_init_results(void) {
    s32 i;

    /* The active participant count is0..8; order entries and the preexisting
     * inverse position for participant0 are valid slots0..7. */
    ModeFrame = 0;
    InitMenuTextRows((MenuTextRow60 *)SharedWorkArena);
    MenuCursor = 0;
    MenuItemCount = 3;
    for (i = 0; i < 11; i++) {
        SetMenuTextRow(i, i, 0, 0, ovl667_result_labels[i]);
    }
    for (i = 11; i < 22; i++) {
        SetMenuTextRow(i, i - 11, 0, 0, -1);
    }
    for (i = 33; i < 44; i++) {
        SetMenuTextRow(i, i - 33, 2, 0, ovl667_result_labels[i - 22]);
    }
    if (TotalRaceTime >= 0) {
        for (i = 0; i < NumCars; i++) {
            LinkFinishCarOrder[LinkPrevCarRanks[i]] = i;
        }
        if (LinkPlayerWinCounts[LinkFinishCarOrder[0]] < 99) {
            LinkPlayerWinCounts[LinkFinishCarOrder[0]]++;
        }
    }
    LinkState = 0;
    SetMenuFrameOptions(0, 1, 0);
}

s32 ovl667_update_retry_choice(s32 mode){
 (void)mode;
 SetMenuTextRowStyle(MenuCursor+39,0,0);
 StepMenuValue(0,2,(char*)&MenuCursor,2,0);
 SetMenuTextRowStyle(MenuCursor+39,1,1);
 if(MenuReturnSubmodeIndex){PlaySfx(1);return MenuCursor;}
 return -1;
}

extern s32 TotalRaceTime, ActivePlayerIndex;
extern s16 MenuCursor;
extern s16 LinkState, LinkRaceAbortFlag, LinkPlayerCount;
extern char LinkPlayerNames[4][12];

s32 ExitOnLinkLost(void);
s32 UpdateLinkHandshake(s32);
void BeginLinkSync(void);
void SetMenuFrameOptions(s32, s32, s32);
void CopyStringClearTail(char *, char *);
void CopyStringPadSpaces(char *, char *, s32);
void FormatDecimalDigits(char *, s32, s32);
#include "rr/screen_tiles.h"
void ResetScreenCounters(void);
void InitMenuArenas(void);
void UpdateScreenFade(s32);
void DrawResultsStandingsOverlay(s32);
void UpdateMenuTextRow(s32);
void DrawResultsRankIcons(void);
s32 LoadResultsSceneStep(void);
void SeqStartTrack(s32);
s32 ovl667_update_retry_choice(s32);
void ovl667_init_car_selection(s32);

void ovl667_update_results(void) {
    s32 pressed;
    s32 group;
    s32 i;
    s32 result;
    s32 player;

    ModeFrame++;
    if (ExitOnLinkLost() != 0) {
        return;
    }
    pressed = UpdateLinkHandshake(2);
    if (ModeFrame >= 0x08000000U) {
        ModeFrame = 3;
    }
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
        if (MenuTextRows[3].signedBytes[3] >= 16) {
            SetMenuFrameOptions(1, 1, group);
            if (MenuReturnSubmodeIndex != 0) {
                SubmodeState = 3;
                SetMenuFrameOptions(group, 1, group);
            }
        }
        FormatDecimalDigits(MenuTextRows[4].marquee.at28 + 8, LinkFinishCarOrder[0] + 1, 0);
        CopyStringPadSpaces(MenuTextRows[5].marquee.at28 + 1, LinkPlayerNames[LinkFinishCarOrder[0]], 9);
        /* The original literal occupies 12 bytes (zero word before "WIN "). */
        CopyStringClearTail(MenuTextRows[5].marquee.at28 + 10, "WON !!!\0\0\0");
        for (i = 0; i < 4; i++) {
            if (i < LinkPlayerCount) {
                player = LinkFinishCarOrder[i];
                CopyStringPadSpaces(MenuTextRows[7 + i].marquee.at28 + 5, LinkPlayerNames[player], 9);
                CopyStringClearTail(MenuTextRows[7 + i].marquee.at28 + 14, "WIN ");
                FormatDecimalDigits(MenuTextRows[7 + i].marquee.at28 + 18, LinkPlayerWinCounts[player], 1);
            } else {
                MenuTextRows[7 + i].marquee.at28[0] = 0;
            }
        }
        break;
    case 3:
        group = 3;
        if (MenuTextRows[39].signedBytes[3] >= 17) {
            SetMenuFrameOptions(1, 1, 0);
            if (ovl667_update_retry_choice(0) >= 0) {
                ResetScreenCounters();
                InitMenuArenas();
                FadeCountdown = 64;
                SubmodeState = MenuCursor + 5;
                BeginLinkSync();
                SetMenuFrameOptions(0, 1, 0);
            }
        }
        break;
    case 4:
        group = -1;
        if (pressed == 0 && FadeCountdown == 64) {
            break;
        }
        FadeCountdown--;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (64 - FadeCountdown) * 4, 69);
        if (FadeCountdown <= 0) {
            LinkRaceAbortFlag = 1;
            ModeGroup = 1;
            ModeIndex = 41;
            SubmodeState = 0;
            LinkState = 0;
        }
        break;
    case 5:
        group = 3;
        if (pressed == 0 && FadeCountdown == 64) {
            CopyStringClearTail(MenuTextRows[38].marquee.at28, "*PLEASE WAIT! ");
            break;
        }
        FadeCountdown -= 2;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (64 - FadeCountdown) * 4, 69);
        if (FadeCountdown <= 0) {
            group = -1;
            SubmodeState = 8;
        }
        break;
    case 6:
        group = -1;
        FadeCountdown--;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (64 - FadeCountdown) * 4, 69);
        if (FadeCountdown <= 0) {
            ModeGroup = 1;
            ModeIndex = 3;
            SubmodeState = 0;
        }
        break;
    case 8:
        result = LoadResultsSceneStep();
        if (result != 1) {
            return;
        }
        SeqStartTrack(0);
        LinkState = 0;
        ActivePlayerIndex = (result - LinkHostFlag) * 2;
        ovl667_init_car_selection(ActivePlayerIndex);
        ModeGroup = result;
        ModeIndex = 60;
        SubmodeState = 0;
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
s32 ovl667_result_labels[22] = {7702, 0, -1, 3, 25, 25, -1, 4633, 4889, 5145, 5401, 7702, 17, 7702, 7702, -1, -1, 7, 9, -1, 7702, -1};

/* Writable state of this unit; ovl667_reset_module restores it. */
static const OverlayStateRegion ovl667_results_regions[] = {
    RR_OVERLAY_STATE(ovl667_result_labels)};
const OverlayStateTable ovl667_results_state = {ovl667_results_regions, sizeof(ovl667_results_regions) / sizeof(ovl667_results_regions[0])};

/* Initial values of the globals this module owns. */
s16 LinkFinishCarOrder[4];
s16 LinkPlayerWinCounts[4];
/* 0x8010C088: four link player names; the name exchange sends rows 2..3
 * (the PS1 map's LinkPlayerPacketBuf, 0x8010C0A0) and receives rows 0..1. */
char LinkPlayerNames[4][12];
