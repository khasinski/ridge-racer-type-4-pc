#include "rr/field_rate.h"
#include "game/system/pad.h"
#include "game/car/carparam.h"
#include "rr/overlay660.h"
#include "rr/frame_storage_types.h"
#include "rr/video_timing.h"
#include "rr/cpu_work.h"

void ovl660_check_reverse_run(RaceVehicle *player){
 RaceVehicleControl *control=&player->control;
 if(RaceState-2U<2){
  s32 allow=0;
  if(control->dynamics94.steering.count22<=0)allow=player->half1EA!=ReverseCourse;
  else if(player->first<=(s32)(0U-TrackLength))allow=1;
  if(allow==1){RaceState=5;FadeCountdown=RR_PAL_NTSC(75, 90);SoundSetMasterVolume(0,RR_PAL_NTSC(71, 86));}
 }
}

void ovl660_exit_race(void)
{
    StopRaceAudioAndRumble();
    SetupRaceAndSceneId();
    ModeGroup = 1;
    ModeIndex = 53;
    SubmodeState = 0;
    AddScreenFadeTiles(CurrentDrawBuf->buffer.ordering70.tags, 255, 69);
}

void ovl660_update_race(void) {
    s32 fadeFlags;
    s8 paused;

    /* The mode's counters, timers and state run once per original tick;
     * the drawing every field (rr/field_rate.h). */
    const int tick = RaceSlowTick();
    if (tick)
        ModeFrame = (ModeFrame + 1U);
    if (!tick) {
    } else if (((RaceState - 1U) < 2U) && (PlayerControllers[0].edges08.fields.pressed & 0x800)) {
        if (FadeCountdown <= 0) {
            FadeCountdown = 0xF;
            paused = RacePaused == 0;
            RacePaused = paused;
            if (paused != 0) {
                SoundPause();
                PauseCursor = 0;
            } else {
                ApplyPauseMenuChoice(GameModeType);
                SoundResume();
            }
        } else {
            goto update_pause_timer;
        }
    } else if (FadeCountdown > 0) {
update_pause_timer:
        if (RaceState < 4) {
            FadeCountdown = (u16) FadeCountdown - 1;
        }
    }
    if (RacePaused != 0) {
        if (tick) {
            UpdatePauseMenu(GameModeType);
            ModeFrame = (ModeFrame - 1U);
        }
        DrawPauseMenu(CurrentDrawBuf->ordering.ordering70.tags, PauseCursor);
        DrawRaceOverlaySprites(1);
        DrawLapTimeList(0);
        DrawStartCountdown(CurrentDrawBuf->ordering.ordering70.tags, ModeFrame);
        DrawHudRotatingQuad(PlayerDisplayRpm[0], 0);
        DrawViewportBorderFrames(0);
        DrawHudNumber3((PlayerCars[0].input * 0xA0) / 1168, 0);
        DrawHudDigitMod10(PlayerCars[0].control.previewSample84.halves.high, 0);
        DrawRaceMinimap(0);
        DrawSteeringIndicator(CurrentDrawBuf->ordering.ordering70.tags, 0);
        SubmitRaceHudPackets(GameModeType);
        BuildCameraMatrices();
        SetupViewport(0);
        DrawCars(0);
        RR_660_FRAME_SCRATCH->word78 = RaceQuitTargetMode;
        SetupPlayerView(0);
        DrawTrackObjects(0);
        DrawTrackSceneryForPlayer(0);
        UpdateAndDrawRearViewMirror(ScreenFrame);
        return;
    }
    if (tick)
        ScreenFrame = (ScreenFrame + 1U);
    if (ModeFrame >= RR_PAL_NTSC(75U, 90U) && RaceState == 0) {
        if (tick) {
        SlotsStartForRace();
        RaceState=1;
        ResetCarSlotBuffers();
        SetModelTexturePage(-1);
        }
    } else if (RaceState == 1) {
            if (tick && ModeFrame >= RR_PAL_NTSC(0xB0U, 0xD3U)) {
                UpdateCarStartLineSteering(PlayerCars);
                RaceState = 2;
                CdAudioBeginPlay(1);
            }
    } else if (RaceState >= 4) {
            if (FadeCountdown < RR_PAL_NTSC(0x19, 0x1E)) {
                fadeFlags = 0x45;
                if (RaceState == 4) {
                    fadeFlags = 0x25;
                }
                AddScreenFadeTiles(CurrentDrawBuf->ordering.ordering70.tags, (RR_PAL_NTSC(0x19, 0x1E) - FadeCountdown) * 0xA, fadeFlags);
            }
            if (!tick) {
            } else if (FadeCountdown <= 0) {
                if (RaceState == 4) {
                    ModeGroup = 1;
                    ModeIndex = 0x15;
                    SubmodeState = 0;
                } else {
                    FinishRace();
                }
            } else {
                FadeCountdown = (u16) FadeCountdown - 1;
            }
    } else if (RaceState == 0) {
            if (ModeFrame >= RR_PAL_NTSC(0x33U, 0x3DU)) {
                AddScreenFadeTiles(CurrentDrawBuf->ordering.ordering70.tags, (ModeFrame - RR_PAL_NTSC(0x32U, 0x3CU)) * 0xAU, 0x25);
                DrawBuffers[1].environment.draw00.background = 0;
                DrawBuffers[0].environment.draw00.background = 0;
            } else if (ModeFrame < RR_PAL_NTSC(0x19U, 0x1EU)) {
                AddScreenFadeTiles(CurrentDrawBuf->ordering.ordering70.tags, (RR_PAL_NTSC(0x19U, 0x1EU) - ModeFrame) * 0xAU, 0x45);
                DrawBuffers[1].environment.draw00.background = 1;
                DrawBuffers[0].environment.draw00.background = 1;
            }
            RR_GPU_SCRATCH->cursor = AddDrawAreaPrim(CurrentDrawBuf->ordering.ordering70.tags, RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x100);
    }
    if (tick)
        UpdateLapTiming(PlayerCars);
    DrawRaceOverlaySprites(0);
    if (tick) {
        ovl660_check_reverse_run(PlayerCars);
        UpdateRaceRanking();
    }
    if (RaceState < 4) {
        DrawStartCountdown(CurrentDrawBuf->ordering.ordering70.tags, ModeFrame);
        if (tick)
            PlayStartCountdownVoice(ModeFrame);
    }
    if (tick)
        CheckFinalLapEvent((char *)(void *)PlayerCars);
    if ((RaceState - 1U) < 3U) {
        DrawLapTimeList(0);
        DrawHudRotatingQuad(PlayerDisplayRpm[0], 0);
        DrawViewportBorderFrames(0);
        DrawHudNumber3((PlayerCars[0].input * 0xA0) / 1168, 0);
        DrawHudDigitMod10(PlayerCars[0].control.previewSample84.halves.high, 0);
        DrawRaceMinimap(0);
        DrawSteeringIndicator(CurrentDrawBuf->ordering.ordering70.tags, 0);
        SubmitRaceHudPackets(GameModeType);
    }
    /* the frame's own work before its step (rr/cpu_work.h) */
    RrSpendRaceModeOwn(0);
    StepRaceFrame(GameModeType);
    /* every field (objsim.c, rr/field_rate.h) */
    UpdateTrackObjects();
    if (!tick) {
    } else if (RaceState == 0 && ModeFrame < RR_PAL_NTSC(0x4BU, 0x5AU)) {
        UpdateRaceIntroCamera(PlayerCars, ModeFrame);
    } else {
        UpdateRaceChaseCamera(PlayerCars);
    }
    BuildCameraMatrices();
    DrawCars(0);
    if (PlayerCars[0].half1EA != ReverseCourse) {
        if (RaceState < 4) {
            DrawRaceStartSignalSprite(0);
        }
    }
    RR_660_FRAME_SCRATCH->word78 = RaceQuitTargetMode;
    SetupPlayerView(0);
    DrawTrackObjects(0);
    DrawTrackSceneryForPlayer(0);
    if (tick)
        SlotSetLevelProgram(PlayerDisplayRpm[0], PlayerThrottleLoad[0], 0);
    if (RaceState >= 4) {
        StopRaceRumble();
        if (RaceState == 6) {
            StopRaceAudioAndRumble();
        }
    }
}
