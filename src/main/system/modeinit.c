#include "rr/release.h"
#include "rr/video_timing.h"
#include "game/system/pad.h"
#include "game/sound/sndvab.h"
#include "rr/save_progress.h"
#include "rr/menu_selection.h"
/* RODATA 0x800104c4 0x2c */
#include "rr/ui.h"
#include "rr/load_cursor.h"
#include "rr/badge_rows.h"
#include "rr/menu_text_state.h"
#include "rr/audio_loop.h"
#include "game/data/ldscn1.h"
#include "game/gfx/fadectl.h"
#include "game/gfx/fadein.h"
#include "game/gfx/fadesel.h"
#include "game/menu/menutran.h"
#include "game/race/racegrid.h"
#include "game/race/ranking.h"
#include "game/system/modepad.h"
#include "game/menu/menusave.h"
#include "game/system/modeinit.h"
#include "rr/audio_cd_controller.h"
#include "rr/driver_control.h"
#include "rr/frame_environment.h"
#include "game/gfx/gfxotag.h"
#include "game/data/globals.h"
#include "game/data/selection.h"
#include "game/data/screenframe.h"
#include "game/sound/fxplay.h"
#include "game/ui/uibar.h"
#include "game/menu/mnrows.h"
#include "game/ui/uidraw.h"
#include "game/demo/attract.h"

#include "rr/cd_directory_buffer.h"
extern char StrMainMenu0Log[];
extern short FadeCountdown;
extern int ActivePlayerIndex;
extern int DrawSubOtFlag;
extern short LinkHostFlag;
extern short ReverseCourse;
extern int MaskBitSetting;
extern short ModeIndex;
extern short MenuScriptId;
extern short ModeGroup;
extern int DialogueSceneMode;
#include "rr/menu_arena_base.h"
extern u16 SubmodeState;
extern short LinkState;
extern int ModeInitResetCount;
extern char StrInitMainMenu0[];
extern int VSyncThreshold;
extern short MenuCursor;
extern short ActiveSceneId;
extern int ModeFrame;

void TitleModeEnter(void);
void TitleMenuInit(void);
void TitleMenuUpdate(void);

void TitleModeEnter(void) {
    MenuGridPoint *a0;
    char *a1;
    int a2;
    SetDispMask(0);
    SeqMuteVolume();
    a2 = 0;
    a1 = SelectionRows.bytes;
    a0 = PlayerCourseChoices;
    LinkHostFlag = -1;
    LinkState = 0;
    FadeCountdown = 0;
    ActivePlayerIndex = 0;
    ModeInitResetCount = 0;
    ReverseCourse = 0;
    do {
        a0->first = 0;
        a0->second = 0;
        a1[3] = 0;
        a1 += 4;
        a2++;
        a0++;
    } while (a2 < 4);
    DialogueSceneMode = 2;
    MenuScriptId = 4;
    ModeGroup = 1;
    ModeIndex = 0x23;
    SubmodeState = 0;
    LoadCursor = CdDirectoryBuffer;
    MenuArenaBase = CdDirectoryBuffer;
    if ((u16)((u16)GameModeType - 2) < 3U) BackupSaveData();
    GameModeType = 0;
    _sio_control(1, 1, 0);
    DrawSubOtFlag = 0;
    MaskBitSetting = 0;
    ModeSelectBaseCount = (SaveProgress.prefix.unlocks50.mainMenuUnlocks54 & 1) + 7;
    DebugPrintStub(StrMainMenu0Log);
}

void TitleMenuInit(void)
{
  int i;
  int y;
  int v;
  short c7;
  SetupDrawDispEnvs(0, 0, 0, 3);
  c7 = 7;
  i = 0;
  ModeFrame = 0;
  ScreenFrame = 0;
  VSyncThreshold = 0x80;
  MainMenuChoice = 0;
  MenuCursor = -1;
  LinkEntryState[1][1] = 0;
  LinkEntryState[1][0] = 0;
  LinkEntryState[0][1] = 0;
  LinkEntryState[0][0] = 0;
  ActiveSceneId = 0x104;
  while (i < (ModeSelectBaseCount + 1))
  {
    v = i;
    if (ModeSelectBaseCount == c7)
    {
      if (i == ModeSelectBaseCount)
      {
        v = 8;
      }
    }
    InitUiRow(i, 0x6C, (i * 0x22) + 0x40, 0x54, 0x20, 0, 0, 1, 0x7F, v);
    i++;
  }

  if (CountProgressFlags() == 0x140)
  {
    SaveProgress.prefix.unlocks50.extraModeUnlocked50 = 1;
  }
  if (!RR_RELEASE_JP)
    CardDialogueStatePtr->b7 = 2;
  InitBadgeRow(0, 0xC, 0x130, 0x128, 0x58, 0, 0);
  BadgeRows->half02 = 1;
  _sio_control(1, 1, 0);
  DebugPrintStub(StrInitMainMenu0);
}

void TitleMenuUpdate(void)
{
    int s0;
    int v1;
    int a1;
    int s0b;
    int c3, c4;
    BadgeRow1C *a0;

    ModeFrame = ModeFrame + 1;
    ScreenFrame = ScreenFrame + 1;
    s0 = RunIntroScriptStep();
    switch (SubmodeState) {
    case 0:
    case 10:
        _sio_control(1, 1, 0);
        if (UnlockNoticeMask > 0) {
            UpdateBadgeRowAnim(0);
            if (BadgeRows->state10 == 3) {
                if (MenuReturnSubmodeIndex != 0 || SubmodeState == 0 || CardDialogueStatePtr->st == 4) {
                    if (SubmodeState == 0xA) {
                        PlaySfx(3);
                        CardDialogueStatePtr->b4 = CardDialogueStatePtr->b4 + 1;
                    } else {
                        goto set_submode;
                    set:
                        CardDialogueStatePtr->b4 = v1;
                        goto call;
                    set_submode:
                        SubmodeState = 0xA;
                    }
                    v1 = CardDialogueStatePtr->b4;
                    if (v1 < 4) {
                        c3 = 3;
                        c4 = 4;
                        a1 = UnlockNoticeMask;
                        a0 = BadgeRows;
                        do {
                            if ((a1 >> v1) & 1)
                                goto set;
                            if (v1 == c3)
                                a0->state10 = c4;
                            v1++;
                        } while (v1 < 4);
                    }
                }
            call:
                UpdateDialogue(BadgeRows->x08 + 0xA, BadgeRows->y0A + 0x28, CardDialogueStatePtr);
                return;
            } else if (BadgeRows->state10 == 0xB) {
                SubmodeState = 1;
                UnlockNoticeMask = 0;
                ScreenFrame = 0;
                return;
            }
        } else {
            SubmodeState = 1;
            ScreenFrame = 0;
            return;
        }
        return;
    case 1:
        _sio_control(1, 1, 0);
        if (Pad0Buttons & 0x800) {
            if (s0 == 0)
                ResetDemoFadeState();
            PlaySfx(1);
            EnableDemoFadeSkip();
            MenuCursor = 0;
            ResetLinkSioControl();
            SubmodeState = 2;
            /* The PAL release's language badge row. */
            if (!RR_RELEASE_JP) {
                InitBadgeRow(0, 0x10, 0x1B6, 0x40, 0x22, 0, 0xF00);
                BadgeRows->half02 = 1;
            }
        }
        /* 30 s to the attract demo (NTSC release: 900 frames, counted
         * from the title's start in the Japanese release). */
        if (RR_RELEASE_JP ? (u32)ModeFrame >= 0x384U : ScreenFrame >= (u32)RR_PAL_NTSC(0x2EE, 0x384))
            SubmodeState = 5;
        return;
    case 2:
        ResetLinkSioControl();
        s0b = MenuCursor;
        if (UiRows->byte14 == 2) {
            if (ModeSelectMenuStep() != 0) {
                SubmodeState = 3;
                FadeCountdown = 0x40;
                ResetScreenCounters();
                StartDemoFadeIn(MainMenuChoice);
                if (!RR_RELEASE_JP)
                    BadgeRows->state10 = 4;
            }
        }
        if (s0b != MenuCursor)
            ScreenFrame = 0;
        if (ScreenFrame >= (u32)RR_PAL_NTSC(0xBB8, 0xE10))
            SubmodeState = 5;
        DrawModeSelectRows();
        if (!RR_RELEASE_JP)
            UpdateBadgeRowAnim(0);
        return;
    case 3:
        if (FadeCountdown <= 0) {
            if (LoadOpeningMovieTimStep() == 1) {
                SubmodeState = 0;
                ModeGroup = PendingModeId >> 8;
                ModeIndex = PendingModeId & 0xFF;
            }
        } else {
            FadeCountdown = FadeCountdown - 1;
        }
        DrawModeSelectRows();
        if (!RR_RELEASE_JP)
            UpdateBadgeRowAnim(0);
        return;
    case 5:
        _sio_control(1, 1, 0);
        if (AttractCycleCounter & 1) {
            ModeGroup = 2;
            ModeIndex = 3;
            SubmodeState = 0;
        } else {
            StartAttractDemo(0);
        }
        AttractCycleCounter = AttractCycleCounter + 1;
        if (AttractCycleCounter >= 0x64)
            AttractCycleCounter = 0;
        return;
    }
}

/* ---------------------------------------------------------------- mode dispatch
 * MainLoop runs ModeFuncTable[ModeGroup][ModeIndex](); each mode is an
 * enter/update pair. Entries the PS1 resolved to the overlay window
 * 0x80114878 name the statically linked overlay function that the loaded
 * image held at that address (rr/overlay_module.h). Index 0 and Group 1
 * entries 19/20 are null in the original: 0x80114888/0x801148C4 lie in the
 * header data of every overlay image and no mode selects them. */
#include "rr/overlay_module.h"

void IntroSceneModeEnter(void), StepIntroScene(void), TileRectSceneModeEnter(void),
    TileRectSceneModeUpdate(void), FrmTaskModeEnter(void), FrmTaskModeUpdate(void);
void SplashModeEnter(void), SplashModeUpdate(void), GpMenuModeEnter(void), GpMenuModeUpdate(void),
    ModeSelectMenuEnter(void), ModeSelectMenuUpdate(void), NewCarModeEnter(void), NewCarModeUpdate(void),
    DialogueIntroModeUpdate(void), DialogueModeUpdate(void), GpSaveSlotMenuEnter(void),
    GpSaveSlotMenuUpdate(void), CarStandingsModeEnter(void), CarStandingsModeUpdate(void),
    ReplayModeEnter(void), ReplayModeUpdate(void), RaceAbortToMenuUpdate(void),
    PostRaceScreenModeUpdate(void), LinkWaitModeEnter(void), LinkWaitModeUpdate(void),
    CarPreviewLoadModeEnter(void), CarPreviewLoadModeUpdate(void), CarPreviewModeUpdate(void),
    GarageCarPreviewModeUpdate(void), InitMenuScript(void), UpdateMenuScript(void),
    LinkExchangeModeUpdate(void), YesNoPromptModeEnter(void), YesNoPromptModeUpdate(void),
    LinkErrorModeUpdate(void), LinkBattleEndModeUpdate(void), CardMenuModeEnter(void),
    CardMenuModeUpdate(void);
void IntroInit(void), IntroModeUpdate(void), AttractDemoRaceEnter(void), AttractDemoRaceUpdate(void);
/* Overlay mode entry points. */
void ovl659_init_preview(void), ovl659_update_preview(void), ovl659_exit_race(void),
    ovl659_update_race(void), ovl659_update_results(void);
void ovl660_init_scene(void), ovl660_update_menu(void), ovl660_update_preview(void),
    ovl660_exit_race(void), ovl660_update_race(void), ovl660_update_results(void);
void ovl661_exit_race(void), ovl661_update_race(void), ovl661_init_menu(void),
    ovl661_update_preview(void), ovl661_update_menu(void), ovl661_update_results(void);
void ovl662_init_name_entry(void), ovl662_update_name_entry(void), ovl662_init_garage_scene(void),
    ovl662_update_progress_menu(void), ovl662_init_team_car_choice(void),
    ovl662_update_team_car_select(void), ovl662_enter_history_screen(void),
    ovl662_update_history_screen(void), ovl662_update_available_cars(void),
    ovl662_update_garage_car_view(void), ovl662_enter_emblem_selector(void),
    ovl662_update_emblem_selector(void), ovl662_init_editor_arenas(void),
    ovl662_enter_editor_clean(void), ovl662_enter_editor_restore(void),
    ovl662_draw_editor_transition(void), ovl662_step_editor_frame(void);
void ovl663_init_scene(void), ovl663_update_menu(void), ovl663_update_selection(void),
    ovl663_update_preview(void), ovl663_update_result_preview(void);
void ovl665_init_scene(void), ovl665_update_options(void);
void ovl666_init_scene(void), ovl666_update_menu(void), ovl666_update_preview(void),
    ovl666_exit_race(void), ovl666_update_race(void), ovl666_update_results(void);
void ovl667_exit_race(void), ovl667_update_race(void), ovl667_init_session_menus(void),
    ovl667_update_car_selection(void), ovl667_update_course_menu(void), ovl667_update_results(void);
void ovl668_init_stream_playback(void), ovl668_update_stream_mode(void),
    ovl668_update_epilogue_credits(void);

void (*ModeGroup0FuncTable[])(void) = {
    NULL, IntroSceneModeEnter, StepIntroScene, TileRectSceneModeEnter, TileRectSceneModeUpdate,
    FrmTaskModeEnter, FrmTaskModeUpdate,
};

void (*ModeGroup1FuncTable[])(void) = {
    /*  0 */ NULL, SplashModeEnter, SplashModeUpdate, TitleModeEnter, TitleMenuUpdate,
    /*  5 */ GpMenuModeEnter, GpMenuModeUpdate, ModeSelectMenuEnter, ModeSelectMenuUpdate,
    /*  9 */ NewCarModeEnter, NewCarModeUpdate, DialogueIntroModeUpdate, DialogueModeUpdate,
    /* 13 */ GpSaveSlotMenuEnter, GpSaveSlotMenuUpdate, CarStandingsModeEnter, CarStandingsModeUpdate,
    /* 17 */ ovl659_init_preview, ovl659_update_preview, NULL, NULL,
    /* 21 */ ReplayModeEnter, ReplayModeUpdate, RaceAbortToMenuUpdate, PostRaceScreenModeUpdate,
    /* 25 */ ovl667_exit_race, ovl667_update_race, ovl662_init_name_entry, ovl662_update_name_entry,
    /* 29 */ LinkWaitModeEnter, LinkWaitModeUpdate, CarPreviewLoadModeEnter, CarPreviewLoadModeUpdate,
    /* 33 */ CarPreviewModeUpdate, GarageCarPreviewModeUpdate, InitMenuScript, UpdateMenuScript,
    /* 37 */ ovl661_exit_race, ovl661_update_race, ovl662_init_garage_scene, ovl662_update_progress_menu,
    /* 41 */ LinkExchangeModeUpdate, YesNoPromptModeEnter, YesNoPromptModeUpdate, LinkErrorModeUpdate,
    /* 45 */ LinkBattleEndModeUpdate, ovl659_exit_race, ovl659_update_race, ovl659_update_results,
    /* 49 */ ovl660_init_scene, ovl660_update_menu, ovl660_update_preview, ovl660_exit_race,
    /* 53 */ ovl660_update_race, ovl660_update_results, ovl661_init_menu, ovl661_update_preview,
    /* 57 */ ovl661_update_menu, ovl661_update_results, ovl667_init_session_menus,
    /* 60 */ ovl667_update_car_selection, ovl667_update_course_menu, ovl667_update_results,
    /* 63 */ ovl662_init_team_car_choice, ovl662_update_team_car_select, ovl662_enter_history_screen,
    /* 66 */ ovl662_update_history_screen, ovl662_update_available_cars, ovl662_update_garage_car_view,
    /* 69 */ ovl662_enter_emblem_selector, ovl662_update_emblem_selector, CardMenuModeEnter,
    /* 72 */ CardMenuModeUpdate, ovl663_init_scene, ovl663_update_menu, ovl663_update_selection,
    /* 76 */ ovl663_update_preview, ovl663_update_result_preview, ovl666_init_scene, ovl666_update_menu,
    /* 80 */ ovl666_update_preview, ovl666_exit_race, ovl666_update_race, ovl666_update_results,
    /* 84 */ NULL,
};

void (*ModeGroup2FuncTable[])(void) = {
    NULL, ovl665_init_scene, ovl665_update_options, ovl668_init_stream_playback,
    ovl668_update_stream_mode, IntroInit, IntroModeUpdate, ovl668_update_epilogue_credits,
    AttractDemoRaceEnter, AttractDemoRaceUpdate,
};

void (*ModeGroup3FuncTable[])(void) = {
    NULL, ovl662_init_editor_arenas, ovl662_enter_editor_clean, ovl662_enter_editor_restore,
    ovl662_draw_editor_transition, ovl662_step_editor_frame,
};

void (*ModeGroup4FuncTable[])(void) = {NULL};

void (**ModeFuncTable[])(void) = {
    ModeGroup0FuncTable, ModeGroup1FuncTable, ModeGroup2FuncTable, ModeGroup3FuncTable,
    ModeGroup4FuncTable,
};

/* Linked overlay registry, keyed by R4.BIN entry (GetR4FileIndex(1, n) is
 * entry 658 + n). Entries 669..672 are byte-identical copies loaded by the
 * mode menu; 664 and 673 are asset containers and are still read. */
static const OverlayModule OverlayModules[] = {
    {"grand_prix", 659, 670, ovl659_reset_module},
    {"time_attack", 660, 669, ovl660_reset_module},
    {"versus", 661, 671, ovl661_reset_module},
    {"garage", 662, 0, ovl662_reset_module},
    {"records", 663, 0, ovl663_reset_module},
    {"options", 665, 0, ovl665_reset_module},
    {"extra_trial", 666, 0, ovl666_reset_module},
    {"serial_link", 667, 672, ovl667_reset_module},
    {"movie_playback", 668, 0, ovl668_reset_module},
};

s32 OverlayModuleBind(s32 file) {
    u32 i;
    for (i = 0; i < sizeof(OverlayModules) / sizeof(OverlayModules[0]); i++) {
        const OverlayModule *module = &OverlayModules[i];
        if (file == module->entry || (module->duplicate != 0 && file == module->duplicate)) {
            module->reset();
            return 1;
        }
    }
    return 0;
}

/* Initial values of the globals this module owns. */
char StrMainMenu0Log[12] = {109, 97, 105, 110, 95, 109, 101, 110, 117, 48, 10};
char StrInitMainMenu0[64] = {105, 110, 105, 116, 95, 109, 97, 105, 110, 95, 109, 101, 110, 117, 48, 10};
u32 ScreenFrame;
int ModeInitResetCount;
s32 AttractCycleCounter;
