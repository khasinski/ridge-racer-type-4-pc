#include "rr/release.h"
#include "rr/video_timing.h"
#include "rr/save_slots.h"
#include "rr/menu_selection.h"
#include "rr/menu_frame_state.h"
#include "game/gfx/fontdraw.h"
#include "rr/gpu_scratch_types.h"
#include "game/data/bankrec.h"
#include "rr/frame_storage_types.h"
#include "rr/menu_state_types.h"
#include "game/data/selection.h"
#include "types.h"
#include "scratchpad.h"
#include "rr/player_status.h"
#include "game/race/raceai2.h"
#include "game/race/racegrid.h"
#include "game/sound/sndcmd.h"
#include "game/system/pad.h"
#include "game/system/pad3.h"
#include "game/ui/hudmap.h"
#include "game/ui/uipart1.h"
#include "rr/bios_events.h"
#include "rr/frame_environment.h"
#include "rr/driver_control.h"
#include "rr/menu_background.h"
#include "rr/audio_loop.h"
#include "game/menu/menu8.h"
#include "game/data/globals.h"
#include "rr/menu_setup.h"
#include "game/ui/badge1.h"
#include "rr/screen_tiles.h"
#include "game/sound/sndctrl.h"

extern int LinkReadHandle;
extern int LinkWriteHandle;
extern int VSyncThreshold;
extern int ModeFrame;
extern short ModeIndex;
extern short ModeGroup;
extern unsigned short SubmodeState;
extern short LinkState;
extern s16 MenuScriptId;
extern char LinkPlayerNames[4][12];
extern short ActiveSceneId;
extern short MenuTransitionOverlayMode;
extern char StrLinkError[];
extern char PressStartStr[];
extern short FadeCountdown;
extern char StrLinkBattleEnd[];
extern char PressStartText[];
extern s32 MaskBitSetting;
extern void ovl659_init_results();
extern void ovl661_init_results();
extern void ovl660_init_results();
extern void ovl667_init_results();
extern void ovl666_init_results();
extern int LinkCommTimeoutTimer;
extern int TotalRaceTime;
extern unsigned char GpStageMaxRetryRanks[];

void LinkWaitModeEnter(void);
void LinkWaitModeUpdate(void);
void LinkExchangeModeUpdate(void);
void LinkErrorModeUpdate();
void LinkBattleEndModeUpdate(void);
void StopRaceAudioAndRumble(void);
void RaceAbortToMenuUpdate(void);
void InitPostRaceScreen(void);
void PostRaceScreenModeUpdate(void);
void EvaluateGpQualifyingResult(void);
void SortRaceRankTable(void);
void EnterRaceResultScreen(void);
void SortRankAndShowResults(void);
void SortRankAndEnterResults(void);

void DrawLinkEntryScreen(s32 mode) {
    u8 text[4];
    s32 remaining,row,statusY,waitY,labelY;
    s8 status;

    if (mode>=0) {
        row=3;
        do {
            text[row]=0;
            row--;
        } while (row>=0);
        row=0;
        do {
            labelY=row*194+64;
            waitY=row*194+150;
            statusY=row*194+120;
            text[0]=row+((s32)((LinkHostFlag^1) * 0x10000U)>>15)+49;
            DrawBigFontString(12,labelY,StrPlayerLabel);
            DrawBigFontString(88,labelY,text);
            status=LinkEntryState[1-mode][row];
            switch (status) {
            case 0:
                DrawBigFontString(60,statusY,StrNowAcceptingEntries);
                break;
            case 1:
                DrawBigFontString(60,statusY,EntryAcceptedStr);
                DrawBigFontString(60,waitY,StrStarPleaseWait);
                break;
            case -1:
                DrawBigFontString(60,statusY,StrNoPlayer);
                break;
            }
            row++;
        } while (row<2);
        row=RaceTimeRemaining/RR_PAL_NTSC(50, 60);
        text[0]=row/10+48;
        text[1]=row%10+48;
        /* 16 lines higher on the 240-line screen of the Japanese release. */
        DrawBigFontString(44,RR_PAL_JP(244,228),text);
        DrawBigFontString(32,RR_PAL_JP(244,228),SecondsUntilCloseText);
        /* The 20 s entry window (NTSC release: 600 frames). */
        remaining=((u32)RR_PAL_NTSC(500, 600)-RaceTimeRemaining);
        DrawBigFontString(((s32)(remaining * 0x100U)/RR_PAL_NTSC(500, 600))+32,RR_PAL_JP(224,208),StrLinkProgressBarGlyph);
    }
}

int LinkExchange(void *first, void *second, s32 count, s32 bidirectional) {
    int r;
    s16 role = LinkHostFlag;
    if (role != 0) {
        r = -1;
        if (FileWrite(LinkWriteHandle, second, count) == count) {
            if (bidirectional == 0 || FileRead(LinkReadHandle, first, count) == count) {
                return 0;
            }
failed:
            return -1;
        }
        return r;
    }
    if (FileRead(LinkReadHandle, second, count) != count) goto failed;
    if (bidirectional != 0) {
        r = -1;
        if (FileWrite(LinkWriteHandle, first, count) != count) return r;
    }
    return 0;
}

void LinkWaitModeEnter(void) {
    unsigned a;
    SetDispMask(0);
    SetupDrawDispEnvs(0, 0, 0, 2);
    LinkEntryState[0][0] = 1;
    LinkEntryState[0][1] = 0;
    LinkEntryState[1][0] = 1;
    LinkEntryState[1][1] = 0;
    ClearScreenOverlayState();
    RaceTimeRemaining = RR_PAL_NTSC(0x1F4, 0x258); /* 20 seconds */
    ModeFrame = 0;
    VSyncThreshold = 0x80;
    SubmodeState = 0;
    LinkState = 0;
    a = ((unsigned)_sio_control(0, 0, 0) >> 7) & 3;
    if (a == 3) {
        ModeGroup = 1;
        ModeIndex = a;
        SubmodeState = 0;
    } else {
        ModeGroup = 1;
        ModeIndex = 0x1E;
        SubmodeState = 0;
    }
}

void LinkWaitModeUpdate(void) {
    ModeFrame += 1;
    if (ExitOnLinkLost() != 1) {
        if (UpdateLinkHandshake(0) != 0) {
            MenuScriptId = 8;
            ModeGroup = 1;
            ModeIndex = 0x23;
            SubmodeState = 0;
        }
        DrawLinkEntryScreen((s16)LinkHostFlag);
        AddPartialLine(&CurrentDrawBuf->buffer.ordering70.tags[0], 8, RR_PAL_JP(0xF0, 0xE0), 0x138, RR_PAL_JP(0xF0, 0xE0), ModeFrame * 8, 1, 8, 8, 8, 0);
        AddPartialLine(&CurrentDrawBuf->buffer.ordering70.tags[0], 8, RR_PAL_JP(0x10C, 0xFC), 0x138, RR_PAL_JP(0x10C, 0xFC), ModeFrame * 8, 1, 8, 8, 8, 0);
        SetMenuFrameOptions(1, 1, 1);
        UpdateScreenFade(0x1003E);
    }
}

void LinkExchangeModeUpdate(void) {
    switch (LinkState) {
    case 0:
        SetDispMask(0);
        OpenLinkSerial();
        _sio_control(3, 0, 0);
        _sio_control(5, 0, 0);
        _sio_control(3, 0, 0);
        LinkState = 1;
        return;
    case 1:
        if (LinkExchange(LinkEntryState[1], LinkEntryState[0], 2, 1) == -1) { LinkState = 3; return; }
        if (LinkExchange(LinkPlayerNames[2], LinkPlayerNames[0], 2 * sizeof(LinkPlayerNames[0]), 1) == -1) { LinkState = 3; return; }
        if (LinkExchange(SelectionRows.records + 2, SelectionRows.records, 2 * sizeof(MenuChoice4), 1) == -1) { LinkState = 3; return; }
        if (LinkExchange(&PlayerCourseChoices[2], &PlayerCourseChoices[0], 2 * sizeof(MenuGridPoint), 1) == -1) { LinkState = 3; return; }
        if (LinkExchange(&CourseIndex, &CourseIndex, sizeof(CourseIndex), 0) == -1) { LinkState = 3; return; }
        if (LinkExchange(&SelectedCourseChoice, &SelectedCourseChoice, sizeof(SelectedCourseChoice), 0) == -1) { LinkState = 3; return; }
        if (LinkExchange(&NumLaps, &NumLaps, sizeof(NumLaps), 0) == -1) { LinkState = 3; return; }
        LinkState = 2;
        CloseLinkSerial();
        SetDispMask(1);
        return;
    case 2:
        _sio_control(3, 0, 0);
        MenuTransitionOverlayMode = 0;
        MenuScriptId = 0xA;
        ActiveSceneId = 0x11A;
        ModeGroup = 1;
        ModeIndex = 0x23;
        SubmodeState = 0;
        return;
    default:
        CloseLinkSerial();
        SetDispMask(1);
        ModeGroup = 1;
        ModeIndex = 0x2C;
        SubmodeState = 0;
        break;
    }
}

void LinkErrorModeUpdate(void) {
  int s3 = (unsigned short)SubmodeState;
  void *new_var;
  GpuScratchPointers *g;
  switch (s3)
  {
    case 0:
      SetDispMask(0);
      SetupDrawDispEnvs(0, 0, 0, 3);
      VSyncThreshold = 0x80;
      StopRaceAudioAndRumble();
      ModeFrame = 0;
      SubmodeState += 1;
      return;

    case 1:
      SetDispMask(1);
      SubmodeState += 1;
      return;

    case 2:
      ModeFrame += 1;
      _sio_control(1, 1, 0);
      g = RR_GPU_SCRATCH;
      new_var = g->cursor;
      g->cursor = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], AddTextSprites(&CurrentDrawBuf->buffer.ordering70.tags[0], AddTextSprites(&CurrentDrawBuf->buffer.ordering70.tags[0], new_var, 0x78, RR_PAL_JP(0x100, 0xF0), StrLinkError, 0x7F38, 0, 1), 0x78, RR_PAL_JP(0x114, 0x104), PressStartStr, 0x7F38, 0, 1), 0, 0, 0x140, RR_PAL_JP(0x200, 0x1E0), 0x80, 0, 0), 0x5E);
      if (MenuReturnSubmodeIndex == 0)
    {
      if (ModeFrame < RR_PAL_NTSC(0xFBU, 0x12DU))
      {
        return;
      }
    }
      SubmodeState = 3;
      FadeCountdown = 0x20;
      return;

    case 3:
      if (FadeCountdown > 0)
    {
      FadeCountdown = ((unsigned short) FadeCountdown) - 1;
    }
      AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x20 - FadeCountdown) * 8, 0x45);
      RR_GPU_SCRATCH->cursor = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[1], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, RR_PAL_JP(0x200, 0x1E0), (0x20 - FadeCountdown) * 4, 0, 0);
      if (FadeCountdown <= 0)
    {
      ModeGroup = 1;
      ModeIndex = s3;
      SubmodeState = 0;
    }
      break;

  }

}

void LinkBattleEndModeUpdate(void)
{
  int s = (unsigned short)SubmodeState;
  void *new_var;
  switch (s)
  {
    case 0:
      SetDispMask(0);
      SetupDrawDispEnvs(0, 0, 0, 3);
      VSyncThreshold = 0x80;
      ModeFrame = 0;
      SubmodeState++;
      return;

    case 1:
      SetDispMask(1);
      SubmodeState++;
      return;

    case 2:
      ModeFrame++;
      new_var = RR_GPU_SCRATCH->cursor;
      RR_GPU_SCRATCH->cursor = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], AddTextSprites(&CurrentDrawBuf->buffer.ordering70.tags[0], AddTextSprites(&CurrentDrawBuf->buffer.ordering70.tags[0], new_var, 0x78, RR_PAL_JP(0x100, 0xF0), StrLinkBattleEnd, 0x7F38, 0, 1), 0x78, RR_PAL_JP(0x114, 0x104), PressStartText, 0x7F38, 0, 1), 0x3E);
      if (MenuReturnSubmodeIndex == 0)
    {
      if (ModeFrame < RR_PAL_NTSC(0xFBU, 0x12DU))
      {
        return;
      }
    }
      FadeCountdown = 0x20;
      SubmodeState = 3;
      return;

    case 3:
      if (FadeCountdown > 0)
    {
      FadeCountdown = FadeCountdown - 1;
    }
      AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x20 - FadeCountdown) * 8, 0x45);
      if (FadeCountdown <= 0)
    {
      ModeGroup = 1;
      ModeIndex = s;
      SubmodeState = 0;
    }
      break;

  }

}

void StopRaceRumble(void) {
    PadRumbleStop(0);
    PadRumbleStop(1);
    PadClearPulse(0);
    PadClearPulse(1);
}

void StopRaceAudioAndRumble(void) {
    SoundSetMasterVolume(0, 0);
    StopRaceRumble();
    MaskBitSetting = 0;
}

void RaceAbortToMenuUpdate(void) {
    if ((unsigned short)SubmodeState == 0) {
        SetDispMask(0);
        StopRaceAudioAndRumble();
        VSyncThreshold = 0x80;
        SubmodeState = 1;
    } else if ((unsigned short)SubmodeState == 1) {
        SubmodeState = 2;
    } else if ((unsigned short)SubmodeState == 2) {
        MenuScriptId = 6;
        ModeGroup = 1;
        ModeIndex = 0x23;
        SubmodeState = 0;
    }
    RR_GPU_SCRATCH->cursor = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, RR_PAL_JP(0x100, 0xF0), 8, 8, 8);
}

void InitPostRaceScreen(void) {
    SetupDrawDispEnvs(0, 0, 0, 2);
    VSyncThreshold = 0x80;
    ModeFrame = 0;
    LinkCommTimeoutTimer = 0;
    ScreenFadeState.phase = 2;
    SoundCloseVabsAndReset();
    switch (GameModeType) {
    case 1:
        ovl659_init_results();
        ActiveSceneId = 0x130;
        break;
    case 2:
    case 3:
        ovl661_init_results();
        ActiveSceneId = 0x13A;
        break;
    case 0:
        ovl660_init_results();
        ActiveSceneId = 0x136;
        break;
    case 4:
        ovl667_init_results();
        ActiveSceneId = 0x13E;
        break;
    case 6:
        ovl666_init_results();
        ActiveSceneId = 0x153;
        break;
    default:
        ActiveSceneId = 0x103;
        break;
    }
}

void PostRaceScreenModeUpdate(void) {
    s32 temp_s0;

    ModeFrame += 1;
    UpdateScreenFade(1);
    if ((ModeFrame >= RR_PAL_NTSC(0x1C2U, 0x21CU) || MenuReturnSubmodeIndex != 0) && LinkCommTimeoutTimer <= 0) {
        LinkCommTimeoutTimer += 1;
        ModeFrame = 0;
        return;
    }
    temp_s0 = LinkCommTimeoutTimer;
    if (temp_s0 == 1) {
        if (MenuReturnSubmodeIndex != 0) {
            ModeFrame = 0;
            ResetScreenCounters();
            ModeGroup = (s16) temp_s0;
            ModeIndex = 0xB;
            SubmodeState = 0;
        }
    }
}

void EvaluateGpQualifyingResult(void) {
    const s32 *targetFrames;
    const s32 *selectedTime;
    int rankIndex;
    int stageBase;
    targetFrames = CarRecordPtrs[0]->targetFrames1E8;
    if (SelectionState.fields.phase17 >= 0) {
        stageBase = 0;
        if (SelectionState.fields.phase17 >= 2) {
            stageBase = 2;
            if (SelectionState.fields.phase17 >= 4) {
                stageBase = 4;
            }
        }
    } else {
        stageBase = 4;
    }
    rankIndex = 0;
    do {
        if (RankTable[rankIndex]->carIndex04 == 0) {
            SelectionState.fields.results00[SelectionState.fields.phase17] = rankIndex + 1;
            if (rankIndex == 0) {
                if (stageBase < 4) {
                    selectedTime = targetFrames + (SelectionState.fields.phase17 - stageBase);
                    if (FramesToMilliseconds(*selectedTime, 0) > TotalRaceTime) {
                        SelectionState.fields.results00[SelectionState.fields.phase17] = 0;
                    }
                }
            }
            break;
        }
        rankIndex += 1;
    } while (rankIndex < 8);
    if (SelectionState.fields.result19 < 9) {
        SelectionState.fields.result19 = SelectionState.fields.results00[SelectionState.fields.phase17];
    }
}

void SortRaceRankTable(void) {
    int i, j;
    int t[2];
    for (i = 0; i < 7; i++) {
        for (j = 7; i < j; j--) {
            t[0] = RankTable[j - 1]->score00;
            t[1] = RankTable[j]->score00;
            if (t[0] < t[1] || (t[0] == t[1] && RankTable[j]->carIndex04 == 0)) {
                RankEntry8 *x = RankTable[j - 1];
                RankEntry8 *y = RankTable[j];
                RankTable[j] = x;
                RankTable[j - 1] = y;
            }
        }
    }
    if (GameModeType == 1) {
        EvaluateGpQualifyingResult();
    }
}

void EnterRaceResultScreen(void) {
    MenuSettings2C *p;
    if (GameModeType == 1) {
        p = &SelectionState.fields;
        if (p->result19 > GpStageMaxRetryRanks[p->phase17 / 2]) p->mode1A = 5;
        else p->mode1A = 4;
    }
    ModeGroup = 1;
    ModeIndex = 0x17;
    SubmodeState = 0;
}

void SortRankAndShowResults(void) {
    SortRaceRankTable();
    EnterRaceResultScreen();
}

void SortRankAndEnterResults(void) {
    SortRaceRankTable();
    EnterRaceResultScreen();
}

void FinishRace(void) {
    SortRankAndEnterResults();
}

/* Initial values of the globals this module owns. */
u8 StrPlayerLabel[12] = {0x2A, 0x50, 0x4C, 0x41, 0x59, 0x45, 0x52};
u8 StrNowAcceptingEntries[28] = {0x2A, 0x4E, 0x4F, 0x57, 0x20, 0x41, 0x43, 0x43, 0x45, 0x50, 0x54, 0x49, 0x4E, 0x47, 0x20, 0x45, 0x4E, 0x54, 0x52, 0x49, 0x45, 0x53, 0x2E};
u8 EntryAcceptedStr[20] = {0x2A, 0x45, 0x4E, 0x54, 0x52, 0x59, 0x20, 0x41, 0x43, 0x43, 0x45, 0x50, 0x54, 0x45, 0x44, 0x2E};
u8 StrStarPleaseWait[16] = {0x2A, 0x50, 0x4C, 0x45, 0x41, 0x53, 0x45, 0x20, 0x57, 0x41, 0x49, 0x54, 0x2E};
u8 StrNoPlayer[12] = {0x2A, 0x4E, 0x4F, 0x20, 0x50, 0x4C, 0x41, 0x59, 0x45, 0x52};
u8 SecondsUntilCloseText[28] = {0x2A, 0x20, 0x20, 0x20, 0x53, 0x45, 0x43, 0x4F, 0x4E, 0x44, 0x53, 0x20, 0x55, 0x4E, 0x54, 0x49, 0x4C, 0x20, 0x43, 0x4C, 0x4F, 0x53, 0x45, 0x2E};
u8 StrLinkProgressBarGlyph[4] = {0x5B, 0x5D};
char StrLinkError[12] = {76, 73, 78, 75, 32, 69, 82, 82, 79, 82, 33};
char PressStartStr[16] = {80, 82, 69, 83, 83, 32, 83, 84, 65, 82, 84, 33};
char StrLinkBattleEnd[16] = {76, 73, 78, 75, 32, 66, 65, 84, 84, 76, 69, 32, 69, 78, 68};
char PressStartText[40] = {80, 82, 69, 83, 83, 32, 83, 84, 65, 82, 84};
unsigned char GpStageMaxRetryRanks[4] = {3, 2, 1, 1};
s8 LinkEntryState[2][2];
short FadeCountdown;
int VSyncThreshold;
RankEntry8 *RankTable[8];
unsigned short SubmodeState;
int LinkCommTimeoutTimer;
