#include "rr/cpu_time.h"
#include "rr/video_timing.h"
#include "game/data/menuvars.h"
#include "game/system/pad.h"
#include "game/sound/cdstart.h"
#include "rr/player_cars.h"
#include "game/data/resinit.h"
#include "game/camera/viewcam.h"
#include "rr/vehicle_table.h"
#include "rr/frame_storage_types.h"
#include "scratchpad.h"
#include "game/car/car.h"
#include "game/course/trkintp2.h"
#include "game/course/trktext.h"
#include "game/gfx/gfxdraw.h"
#include "game/gfx/scene1.h"
#include "game/sound/cdstop.h"
#include "game/sound/sndslot.h"
#include "game/ui/hudmap2.h"
#include "psyq/libc/printf.h"
#include "game/demo/attract.h"
#include "game/demo/opening.h"
#include "game/menu/title.h"
#include "rr/frame_environment.h"
#include "rr/color_conversion.h"
#include "rr/driver_control.h"
#include "rr/audio_state_types.h"
#include "game/gfx/gfxotag.h"
#include "game/car/carwheel.h"
#include "rr/screen_tiles.h"
#include "game/car/carbound.h"
#include "game/race/racemai2.h"
#include "rr/text_sprites.h"
#include "game/math/rotmat.h"
#include "game/race/objsim.h"
#include "game/race/objdraw.h"
#include "game/gfx/gtepoly1.h"
#include "game/menu/mnchoice.h"
#include "game/sound/sndctrl.h"
#include "game/sound/cdplay.h"
#include "rr/scene_camera_state.h"

extern char RepeatLabel[];
extern char StrFmtTwoDigitsTitle[];
/* Consecutive 4-byte UV records: speed units (2), view-mode icon, lap slash. */
extern u8 HudSpeedUnitSprites[];
extern u8 HudViewModeIconUvs[];
extern u8 HudLapSlashSprite[];
extern int RaceQuitTargetMode;
extern int MaskBitSetting;

extern int ViewUsesCarTrackNode;
void AttractDemoRaceUpdate(void) {
  char sp18[16];
  int i;
  int old;
  int v;
  int one;
  int vv;
  int c8;
  RaceVehicle *tp;
  int *ip;
  if (RaceContext.attract4BC.frame00 == 0)
  {
    SetDispMask(0);
    SetupDrawDispEnvs(0, 0, 0, 1);
    SelectCourseBgmTrack();
    CdAudioBeginPlay(0);
    RaceContext.attract4BC.idleTimer20 = RR_PAL_NTSC(0xFA, 0x12C);
    for (i = 0; i < 7; i++)
    {
      if (AttractCarTeams[i & 3] < 2)
      {
        ComCars[i].control.steeringEnabled124 = 1;
      }
      else
      {
        ComCars[i].control.steeringEnabled124 = 0;
      }
    }

    for (i = 0; i < 4; i++)
    {
      SlotSetPause(i, 1);
    }

    MaskBitSetting = 1;
  }
  else
    if (RaceContext.attract4BC.frame00 > 0)
  {
    if (RaceContext.attract4BC.frame00 == 4)
    {
      SetDispMask(1);
      SetupDrawDispEnvs(0, 0, 0, 0);
    }
  }
  StepRaceFrame(5);
  UpdateTrackObjects();
  /* The demo race's PS1 time: after a long CD wait at its start the
   * frame runs past the next vblank, as on the console (rr/cpu_time.h). */
  RR_CPU_CYCLES(RR_CYCLES_ATTRACT_DEMO_FRAME);
  MaskBitSetting = (one = 1);
  RaceContext.attract4BC.frame00 = RaceContext.attract4BC.frame00 + one;
  RaceContext.attract4BC.cameraTimer08 = RaceContext.attract4BC.cameraTimer08 + one;
  if (RaceContext.attract4BC.frame00 >= RR_PAL_NTSC(0x1771, 0x1C21))
  {
    RaceContext.attract4BC.frame00 = RR_PAL_NTSC(0x1770, 0x1C20);
  }
  if (RaceContext.attract4BC.mode0C == 0)
  {
    vv = MenuReturnSubmodeIndex;
  }
  else
  {
    vv = TitleSubmenuSelection;
  }
  if (vv != 0)
  {
    if (FadeCountdown == 0)
    {
      if (RaceContext.attract4BC.frame00 >= RR_PAL_NTSC(0x1A, 0x1F))
      {
        RaceContext.attract4BC.frame00 = RR_PAL_NTSC(0xBB8, 0xE10);
        RaceContext.attract4BC.exitStage10 = one;
      }
    }
  }
  if (RaceContext.attract4BC.mode0C == 0)
  {
    if (RaceContext.attract4BC.frame00 == RR_PAL_NTSC(0xBB8, 0xE10))
    {
      FadeCountdown = 0x40;
      SoundSetMasterVolume(0, 0x3C);
    }
  }
  else
    if (RaceContext.attract4BC.exitStage10 == 1)
  {
    FadeCountdown = 0x40;
    SoundSetMasterVolume(0, 0x3C);
    RaceContext.attract4BC.exitStage10 = 2;
  }
  if (RaceContext.attract4BC.mode0C == 1)
  {
    if (Pad0Buttons & 0x10)
    {
      RaceContext.attract4BC.viewMode78 = RaceContext.attract4BC.viewMode78 + 1;
      if (RaceContext.attract4BC.viewMode78 >= 4)
      {
        RaceContext.attract4BC.viewMode78 = 0;
      }
      switch (RaceContext.attract4BC.viewMode78)
      {
        case 0:
          RaceContext.overlayToggle3DC = 0;
          RaceContext.attract4BC.freeCamera74 = 0;
          ResetCarSlotBuffers();
          break;

        case 1:
          RaceContext.overlayToggle3DC = 1;
          RaceContext.attract4BC.freeCamera74 = 0;
          break;

        case 2:
          RaceContext.overlayToggle3DC = 0;
          RaceContext.attract4BC.freeCamera74 = 1;
          ResetCarSlotBuffers();
          break;

        case 3:
          RaceContext.overlayToggle3DC = 1;
          RaceContext.attract4BC.freeCamera74 = 1;
          break;

      }

      SetSceneOverlayEnabled(RaceContext.overlayToggle3DC);
    }
    if (RaceContext.overlayToggle3DC == 1)
    {
      if (RaceContext.attract4BC.freeCamera74 == 0)
      {
        if (AttractPromptBlinkMask & 8)
        {
          RaceContext.attract4BC.overlayHsv24[0] = RaceContext.attract4BC.overlayHsv24[0] + 1;
        }
        if (AttractPromptBlinkMask & 4)
        {
          RaceContext.attract4BC.overlayHsv24[0] = RaceContext.attract4BC.overlayHsv24[0] - 1;
        }
        if (((short) RaceContext.attract4BC.overlayHsv24[0]) < 0)
        {
          RaceContext.attract4BC.overlayHsv24[0] = 0x167;
        }
        if (!(((short) RaceContext.attract4BC.overlayHsv24[0]) < 0x168))
        {
          RaceContext.attract4BC.overlayHsv24[0] = 0;
        }
        if (AttractPromptBlinkMask & 2)
        {
          RaceContext.attract4BC.overlayHsv24[1] = 0x800;
        }
        if (AttractPromptBlinkMask & 1)
        {
          RaceContext.attract4BC.overlayHsv24[1] = 0;
        }
        if (RaceContext.attract4BC.overlayHsv24[1] >= 0x801)
        {
          RaceContext.attract4BC.overlayHsv24[1] = 0x800;
        }
        if (RaceContext.attract4BC.overlayHsv24[1] < 0)
        {
          RaceContext.attract4BC.overlayHsv24[1] = 0x800;
        }
        HsvToRgb(RaceContext.attract4BC.overlayHsv24, RaceContext.attract4BC.overlayRgb2C);
        SetSceneOverlayColor(0, RaceContext.attract4BC.overlayRgb2C[0], RaceContext.attract4BC.overlayRgb2C[1], RaceContext.attract4BC.overlayRgb2C[2]);
      }
    }
    if (RaceContext.attract4BC.freeCamera74 == 1)
    {
      if (AttractPromptBlinkMask & 4)
      {
        RaceContext.attract4BC.camera30[5] = RaceContext.attract4BC.camera30[5] - 0x10;
      }
      if (AttractPromptBlinkMask & 8)
      {
        RaceContext.attract4BC.camera30[5] = RaceContext.attract4BC.camera30[5] + 0x10;
      }
      if (RaceContext.attract4BC.camera30[5] < 0)
      {
        RaceContext.attract4BC.camera30[5] = RaceContext.attract4BC.camera30[5] + 0x1000;
      }
      if (RaceContext.attract4BC.camera30[5] >= 0x1001)
      {
        RaceContext.attract4BC.camera30[5] = RaceContext.attract4BC.camera30[5] - 0x1000;
      }
    }
      if (Pad0Buttons & 0x100)
      {
        RaceContext.attract4BC.bgmLocked7C = RaceContext.attract4BC.bgmLocked7C ^ 1;
      }
  }
  SetupViewport(0);
  if ((RaceContext.attract4BC.mode0C == 0) || (RaceContext.attract4BC.freeCamera74 == 0))
  {
    UpdateRaceCamera(2, ComCars + RaceContext.attract4BC.cameraCar04);
  }
  else
  {
    ViewUsesCarTrackNode = 1;
    PlayerCars[0].trackNodeF0 = ComCars[0].trackNodeF0; /* TitleDemoTrackNode */
    InitDemoComCars(ComCars, RaceContext.attract4BC.camera30);
  }
  if (RaceContext.attract4BC.cameraTimer08 >= RR_PAL_NTSC(0x178, 0x1C3))
  {
    if (RaceContext.cameraNodeChanged1C != 0)
    {
      if (RaceContext.attract4BC.freeCamera74 == 0)
      {
        v = RaceContext.attract4BC.cameraCar04;
        RaceContext.attract4BC.cameraCar04 = v - 2;
        if ((v - 2) < 0)
        {
          RaceContext.attract4BC.cameraCar04 = v + 5;
        }
        RaceContext.cameraNodeIndex4C = -1;
        RaceContext.attract4BC.cameraTimer08 = 0;
        UpdateRaceCamera(2, ComCars + RaceContext.attract4BC.cameraCar04);
      }
    }
  }
  BuildCameraMatrices();
  if (RaceContext.attract4BC.frame00 >= 5)
  {
    if (RaceContext.attract4BC.mode0C == 0)
    {
      UpdateAndDrawSceneOverlay();
    }
    else
    {
      DrawSceneOverlayIfActive();
    }
  }
  if (FadeCountdown > 0)
  {
    FadeCountdown = FadeCountdown - 1;
    AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x40 - FadeCountdown) * 4, 0x45);
    if (FadeCountdown == 0)
    {
      if (RaceContext.attract4BC.mode0C == 0)
      {
        ModeGroup = 1;
        ModeIndex = 3;
        SubmodeState = 0;
        MaskBitSetting = 0;
      }
      else
      {
        RaceContext.bgmViewerReturn18 = 1;
        ModeGroup = 2;
        ModeIndex = 8;
        SubmodeState = 0;
        MaskBitSetting = 0;
      }
      SoundSetMasterVolume(0, 0);
    }
  }
  else
    if (RaceContext.attract4BC.frame00 < RR_PAL_NTSC(0x1A, 0x1F))
  {
    AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (RR_PAL_NTSC(0x19, 0x1E) - RaceContext.attract4BC.frame00) * 8, 0x45);
  }
  if (RaceContext.attract4BC.mode0C == 1)
  {
    if (RaceContext.attract4BC.bgmLocked7C == 0)
    {
      old = SoundState.track28A4;
      if (Pad0Buttons & 0x8000)
      {
        SoundState.track28A4 = SoundState.track28A4 - 1;
      }
      if (Pad0Buttons & 0x2000)
      {
        SoundState.track28A4 = SoundState.track28A4 + 1;
      }
      if (SoundState.track28A4 <= 0)
      {
        SoundState.track28A4 = SoundState.count28A8 - 1;
      }
      if (SoundState.track28A4 >= SoundState.count28A8)
      {
        SoundState.track28A4 = 1;
      }
      if (old != SoundState.track28A4)
      {
        RaceContext.attract4BC.trackDelay18 = RR_PAL_NTSC(0x1D, 0x22);
        if (PollCdAudioStatus() >= 0)
        {
          if (RaceContext.attract4BC.cdFadeTimer14 == 0)
          {
            CdAudioFade(0, RR_PAL_NTSC(0x11, 0x16));
            RaceContext.attract4BC.cdFadeTimer14 = RR_PAL_NTSC(0x19, 0x1E);
          }
        }
      }
      if (RaceContext.attract4BC.idleTimer20 > 0)
      {
        RaceContext.attract4BC.idleTimer20 = RaceContext.attract4BC.idleTimer20 - 1;
      }
      if (RaceContext.attract4BC.trackDelay18 == 0)
      {
        if (RaceContext.attract4BC.idleTimer20 == 0)
        {
          if (PollCdAudioStatus() < 0)
          {
            if (RaceContext.attract4BC.repeat1C == 0)
            {
              SoundState.track28A4 = SoundState.track28A4 + 1;
              if (SoundState.track28A4 >= SoundState.count28A8)
              {
                SoundState.track28A4 = 1;
              }
            }
            CdAudioFade(0, RR_PAL_NTSC(0x11, 0x16));
            RaceContext.attract4BC.cdFadeTimer14 = RR_PAL_NTSC(0x19, 0x1E);
            RaceContext.attract4BC.trackDelay18 = RR_PAL_NTSC(0x1D, 0x22);
          }
        }
      }
      RaceContext.attract4BC.cdFadeTimer14 = RaceContext.attract4BC.cdFadeTimer14 - 1;
      if (RaceContext.attract4BC.cdFadeTimer14 < 0)
      {
        RaceContext.attract4BC.cdFadeTimer14 = 0;
      }
      if (RaceContext.attract4BC.trackDelay18 > 0)
      {
        RaceContext.attract4BC.trackDelay18 = RaceContext.attract4BC.trackDelay18 - 1;
        if (RaceContext.attract4BC.trackDelay18 == 0)
        {
          CdAudioSelectTrack(0, BgmNumberToCdTrack[SoundState.track28A4 - 1]);
          if (RaceContext.attract4BC.repeat1C == 0)
          {
            CdAudioBeginPlay(0);
          }
          else
          {
            CdAudioBeginPlay(1);
          }
          RaceContext.attract4BC.idleTimer20 = RR_PAL_NTSC(0xFA, 0x12C);
        }
      }
      if (TitleStartPressedFlag != 0)
      {
        if (RaceContext.attract4BC.repeat1C == 0)
        {
          RaceContext.attract4BC.repeat1C = 1;
        }
        else
        {
          RaceContext.attract4BC.repeat1C = 0;
        }
      }
      if (RaceContext.attract4BC.repeat1C != 0)
      {
        old = 0xC8;
        DrawFontStringRight(0x131, old, RepeatLabel, 0xFF, 0);
      }
      DrawFontStringRight(0xFA, 0xD4, CD_TRACK_TITLE(BgmNumberToCdTrack[SoundState.track28A4 - 1]), 0xFF, 0);
      AddBrightnessSprite(HudLapSlashSprite, 0xFC, 0xD4, 0xFF);
      AddBrightnessSprite(HudViewModeIconUvs, 0x105, 0xD4, 0xFF);
      PsyqSprintf((s8 *)sp18, (const s8 *)StrFmtTwoDigitsTitle, SoundState.track28A4);
      DrawFontString(0x112, 0xD4, sp18, 0xFF, 0);
      AddBrightnessSprite(HudSpeedUnitSprites, 0x121, 0xD4, 0xFF);
        AddBrightnessSprite(HudSpeedUnitSprites + 4, 0x12A, 0xD4, 0xFF);
        AddSetTexpagePrim(0);
    }
  }
  SetModelTexturePage(0xA);
  if ((RaceContext.attract4BC.mode0C == 0) || (RaceContext.attract4BC.freeCamera74 == 0))
  {
    DrawCars(6);
  }
  else
  {
    int k;
    for (k = 1; k < 7; k++)
    {
      DrawOpponentCar(CarTable[k], 2);
    }
  }
  ((int *)SCRATCHPAD)[0x78 / 4] = RaceQuitTargetMode;
  SetupPlayerView(0);
  DrawTrackObjects(0);
  DrawTrackSceneryForPlayer(0);
  QueueDrawStp();
}

/* Initial values of the globals this module owns. */
char RepeatLabel[8] = {82, 69, 80, 69, 65, 84};
char StrFmtTwoDigitsTitle[80] = {37, 48, 50, 100, 0, 0, 0, 0, 67, 82, 83, 53, 65, 0, 0, 0, 67, 82, 83, 52, 66, 0, 0, 0, 67, 82, 83, 52, 65, 0, 0, 0, 67, 82, 83, 51, 66, 0, 0, 0, 67, 82, 83, 51, 65, 0, 0, 0, 67, 82, 83, 50, 66, 0, 0, 0, 67, 82, 83, 50, 65, 0, 0, 0, 67, 82, 83, 49, 66, 0, 0, 0, 67, 82, 83, 49, 65};
int MaskBitSetting;
