#include "game/ui/badge1.h"
#include "rr/release.h"
#include "rr/cpu_time.h"
#include "game/ui/uipart1.h"
#include "rr/save_slots.h"
/* RODATA 0x80010774 0x4c */
#include "types.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"
#include "scratchpad.h"
#include "rr/selection.h"
#include "rr/menu_transition.h"
#include "game/gfx/gfxmtx.h"
#include "game/ui/hudmap2.h"
#include "game/ui/uidraw.h"
#include "game/ui/uitext.h"
#include "game/ui/uiwin.h"
#include "game/menu/mnrows.h"
#include "rr/audio_cd_controller.h"
#include "rr/ui.h"
#include "rr/menu_text_rows.h"
#include "rr/frame_environment.h"
#include "game/data/screenframe.h"
#include "rr/race_setup.h"
#include "rr/menu_screen_exit.h"

#include "rr/cd_directory_buffer.h"
extern char StrCdReadPtrFmt[];
#include "rr/menu_arena_base.h"
extern short SelectedMakerIndex;

void InitMenuArenas();
void InitMenuScript();

void UpdateSelectionRank(void) {
 /* SelectionState viewed through its prefix; history rows follow at 0x20. */
 SelectionStatePrefix *state=(SelectionStatePrefix *)SelectionState.bytes;
 s32 word;
 s32 phase;
 s32 level;
 s32 group;
 s32 i;
 s32 sum;
 word=state->phase;
 phase=word;
 if(word>0) {
  level=state->current.parameter;
  if(phase<4) {
   sum=0;
   i=phase-1;
   if(i<=phase) {
    do {
     s32 choice=state->choices[i];
     sum+=CarSpecStatWeights[choice+i*4];
     i++;
    }while(i<=phase);
   }
  }
  word=phase+((u32)phase>>31);
  group=word>>1;
  if(group==1) {word=SelectionState.fields.reserved10[4];group=word+1;}
  {
   i=0;
   if(state->phase<4) {
    s32 next;
    s32 delta;
    group*=4;
    next=group+1;
    do {
     s32 lower;
     if(i<3)lower=CarSpecStatThresholds[i+next];else lower=-1;
     if(CarSpecStatThresholds[i+group]>=sum && lower<sum) {
      delta=4-i;
      if(state->phase>=2)goto high_level;
      level+=delta;
      break;
     }
     i++;
    }while(i<4);
    goto publish_rank;
high_level:
    word=level+3;
    level=word+delta;
   }else {level+=7;i=level<16;}
publish_rank:;
  }
  {
   SelectionRow *target;
   u32 index;
   index=state->index;
   state->rank=i;
   target=(SelectionRow *)&SelectionState.bytes[sizeof(SelectionStatePrefix)+index*sizeof(SelectionRow)];
   __builtin_memcpy(target,&state->current,sizeof(SelectionRow));
   state->current.parameter=level;
   __builtin_memcpy(&SelectionRows.entry,&state->current,sizeof(SelectionRow));

   {s32 check=state->phase;
   i=1;
   if(check==6) {SelectTeamCarGroup();i=1;}}
  }

  {SelectionRow *row;
  row=SelectionRows.selection.rows;
  do {
      row->parameter=level;
      i++;
      row++;
  }while(i<8);
  }
 }
}

void ResetScreenCounters(void) {
    LoadState = 0;
    MenuScriptStep = 0;
    ScreenFrame = 0;
}

void InitMenuArenas(void) {
  const int arenaOffset = 0x2D000;
  BadgeRowCount = 10;
  Rect16RowCount = 4;
  GlyphRowCount = 4;
  UiRowCount = 0x28;
  ImageRowCount = 1;
  MenuTextRowCount = 0x37;
  MenuArenas.second = CdDirectoryBuffer + arenaOffset;
  LoadCursor = MenuArenas.second;
  DebugPrintStub(StrCdReadPtrFmt, LoadCursor);
  LoadCursor = (u8 *)LoadCursor + AllocBadgeRows(LoadCursor);
  LoadCursor = (u8 *)LoadCursor + AllocRect16Rows(LoadCursor);
  LoadCursor = (u8 *)LoadCursor + AllocGlyphRows(LoadCursor);
  LoadCursor = (u8 *)LoadCursor + AllocImageRows(LoadCursor);
  LoadCursor = (u8 *)LoadCursor + AllocUiRows(LoadCursor);
  MenuBaseLoadCursor = LoadCursor;
  MenuArenas.second = (u8 *)LoadCursor + 0x19000;
  LoadCursor = (u8 *)LoadCursor + 0x19000;
}

void InitMenuScript(void) {
    int a0, a1, a3;
    ModeFrame = 0;
    VSyncThreshold = 0x80;
    LoadCursor = MenuArenaBase;
    a3 = 0;
    switch (MenuScriptId) {
    case 0:
        SetupDrawDispEnvs(0, 0, 0, 1);
        MenuTransitionOverlayMode = 1;
        LoadCursor = MenuArenas.first;
        goto l428;
    case 10:
        if (GameModeType != 5) {
            SetupDrawDispEnvs(0, 0, 0, 1);
            MenuTransitionOverlayMode = 1;
            LoadCursor = CdDirectoryBuffer;
        } else {
            SetupDrawDispEnvs(0, 0, 0, 2);
            MenuTransitionOverlayMode = 0;
        }
        goto l428;
    case 6:
        SetDispMask(0);
        StopRaceAudioAndRumble();
        a3 = 1;
    case 1:
    case 4:
        SetupDrawDispEnvs(8, 8, 8, a3 | 2);
        MenuTransitionOverlayMode = 2;
        break;
    default:
        SetupDrawDispEnvs(0, 0, 0, 2);
        MenuTransitionOverlayMode = 0;
        break;
    }
    ClearScreenOverlayState();
    InitMenuArenas();
l428:
    ResetScreenCounters();
    if (MenuTransitionOverlayMode == 1) {
        SetDispMask(0);
        a1 = 0;
        switch (GameModeType) {
        case 0: a0 = 8; break;
        case 1:
            a0 = SelectionState.signedBytes[0x17];
            a1 = 3 - SelectionState.signedBytes[a0 + 8];
            break;
        case 2:
        case 3: a0 = 9; break;
        case 4: a0 = 10; break;
        case 6: a0 = SelectedMakerIndex + 0xB; break;
        default:
            a0 = 0;
            a1 = a0;
            MenuTransitionOverlayMode = 3;
            break;
        }
        if (MenuTransitionOverlayMode == 1) InitRaceOpeningOverlay(a0, a1);
    } else if (MenuTransitionOverlayMode == 0) {
        IntroScriptStep();
    }
    ModeGroup = 1;
    ModeIndex = 0x24;
    SubmodeState = 0;
}

void UpdateMenuScript(void) {
    RR_CPU_CYCLES(RR_CYCLES_UPDATE_MENU_SCRIPT);

    s32 next_frame;
    s32 mode;
    s32 status;
    s8 next_step;
    s8 opcode;
    s32 step;

    next_frame = (ModeFrame + 1U);
    ModeFrame = next_frame;
    if (next_frame == 2) {
        SetDispMask(1);
    }
    if (GameModeType == 4) {
        UpdateLinkHandshake(1);
    }
    mode=MenuTransitionOverlayMode;
    if (mode == 1) {
        status = UpdateRaceOpeningOverlay(ModeFrame);
    } else {
        if (mode == 0) {
            status = IntroScriptStep();
        } else {
            status = 1;
        }
    }
    step = MenuScriptStep;
    if (step >= 0) {
        if (MenuScriptOps[MenuScripts[MenuScriptId][step]]() == 1) {
            LoadState = 0;
            next_step = (u8) MenuScriptStep + 1;
            MenuScriptStep = next_step;
            opcode = MenuScripts[MenuScriptId][next_step];
            if (opcode < 0) {
                MenuScriptId = (u16) -opcode;
                MenuScriptStep = -1;
                if (MenuTransitionOverlayMode == 0) {
                    StartDemoFadeOutAndCloseVab();
                }
            }
        }
    } else {
        if((status==1 && MenuTransitionOverlayMode==status) || (MenuTransitionOverlayMode==0 && status==2) || (MenuTransitionOverlayMode>=2 && status==1)) {
            MenuSceneEntries[MenuScriptId]();
            SubmodeState = 0;
            ModeGroup = (u16) ((u16) ActiveSceneId >> 8);
            ModeIndex = ActiveSceneId & 0xFF;
        }
    }
    if (ModeGroup != 1) {
        /* The Japanese release fills its 480 lines with one tile. */
        RR_GPU_SCRATCH->cursor =
        (RR_RELEASE_JP ? AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[703], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x1E0, 0, 0, 0)
                      : AddTileGrid2x2(&CurrentDrawBuf->buffer.ordering70.tags[703], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x200, 0, 0, 0));
    }
}

/* Initial values of the globals this module owns. */
#include "rr/frame_storage_types.h"
extern FrameStorage DrawBuffers[2];
#include "game/menu/menucalc.h"
#include "game/menu/mntrow.h"
#include "game/race/raceai2.h"
#include "game/data/ldscn1.h"
#include "game/data/ldscn4.h"
#include "game/menu/menutran.h"
#include "game/system/modeinit.h"
signed char D_8009F8A0[4] = {5, 1, -2};
signed char D_8009F8A4[8] = {0, 2, 5, 1, -11};
signed char D_8009F8AC[4] = {7, 4, -3};
signed char D_8009F8B0[4] = {4, -4};
signed char D_8009F8B4[4] = {3, 11, -7};
signed char D_8009F8B8[4] = {3, -7};
signed char D_8009F8BC[4] = {6, 10, -5};
signed char D_8009F8C0[4] = {8, 11, 10, -9};
signed char D_8009F8C4[4] = {-10};
signed char D_8009F8C8[4] = {2, -6};
signed char D_8009F8CC[4] = {2, -8};
char StrCdReadPtrFmt[100] = {99, 100, 95, 114, 101, 97, 100, 95, 112, 116, 114, 32, 61, 32, 48, 120, 37, 108, 120, 10};
u8 CarSpecStatWeights[16] = {5, 5, 4, 3, 0xA, 0xA, 3, 2, 0xC, 4, 2, 1, 0xF, 0xA, 1, 1};
s8 CarSpecStatThresholds[20] = {15, 14, 7, 6, 27, 17, 13, 4, 27, 17, 13, 4, 27, 17, 13, 4, 27, 17, 13, 4};
MenuScriptOperation MenuScriptOps[12] = {(int (*)(void))LoadModeMenuAssetsStep, (int (*)(void))LoadSceneStep, (int (*)(void))LoadAllCarsStep, (int (*)(void))LoadIndexedTimStep, (int (*)(void))LoadDialogueStep, (int (*)(void))LoadDefaultRaceStep, (int (*)(void))LoadAttractTimStep, (int (*)(void))LoadGlyphRowsStep, (int (*)(void))LoadResultsSceneStep, (int (*)(void))LoadMenuSoundStep, (int (*)(void))LoadMenuCarModelsStep, (int (*)(void))LoadCarClutStep};
/* Entries 6 and 10 were overlay-window addresses: the garage progress menu
 * (0x80114998) and the time attack scene selector (0x80114910). */
void ovl662_init_progress_menu(void), ovl660_select_scene(void);
MenuSceneEntry MenuSceneEntries[12] = {0, (void (*)(void))MenuSceneNoop, (void (*)(void))SetupRaceAndSceneId, (void (*)(void))InitModeSelectMenu, (void (*)(void))DialogueSceneSetup, (void (*)(void))TitleMenuInit, ovl662_init_progress_menu, (void (*)(void))InitPostRaceScreen, (void (*)(void))InitCarPreviewScene, (void (*)(void))SetNextModeByGameType, ovl660_select_scene, (void (*)(void))SetupRace};
s8 *MenuScripts[11] = {(signed char *)D_8009F8A0, (signed char *)D_8009F8AC, (signed char *)D_8009F8B0, (signed char *)D_8009F8B8, (signed char *)D_8009F8BC, (signed char *)D_8009F8C8, (signed char *)D_8009F8B4, (signed char *)D_8009F8CC, (signed char *)D_8009F8C0, (signed char *)D_8009F8C4, (signed char *)D_8009F8A4};
s8 MenuScriptStep;
u16 MenuScriptId;
u16 MenuTransitionOverlayMode;
