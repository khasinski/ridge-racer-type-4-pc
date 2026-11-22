#include "rr/save_slots.h"
#include "game/menu/menu8.h"
#include "game/car/carparam.h"
#include "game/system/pad.h"
#include "rr/overlay659.h"
#include "rr/frame_storage_types.h"
#include "rr/scene_camera_state.h"
#include "rr/video_timing.h"
#include "rr/cpu_work.h"
#include "rr/field_rate.h"

void ovl659_exit_race(void) {
    SetupRaceAndSceneId();
    ModeGroup = 1;
    ModeIndex = 0x2F;
    SubmodeState = 0;
}

void ovl659_update_race(void) {
    /* The mode's counters, timers and state run once per original tick;
     * the drawing every field (rr/field_rate.h). */
    const int tick = RaceSlowTick();
    if (tick)
        ModeFrame = ModeFrame + 1U;
    SetupViewport(0);
    if (!tick) {
    } else if ((RaceState - 1U) < 2U && (PlayerControllers[0].edges08.fields.pressed & 0x800) && FadeCountdown <= 0) {
        FadeCountdown = 15;
        RacePaused = (RacePaused == 0);
        if (RacePaused != 0) {
            SoundPause();
            PauseCursor = 0;
        } else {
            ApplyPauseMenuChoice(GameModeType);
            SoundResume();
        }
    } else if (FadeCountdown > 0) {
        if (RaceState < 4) {
            FadeCountdown--;
        }
    }

    if (RacePaused != 0) {
        if (tick) {
            UpdatePauseMenu(GameModeType);
            ModeFrame = ModeFrame - 1U;
        }
        DrawRaceOverlaySprites(1);
        DrawPauseMenu(&CurrentDrawBuf->ordering.ordering70.tags[0], PauseCursor);
        DrawLapTimeList(0);
        DrawTimeRemaining(RaceTimeRemaining);
        DrawStartCountdown(&CurrentDrawBuf->ordering.ordering70.tags[0], ModeFrame);
        DrawHudRotatingQuad(PlayerDisplayRpm[0], 0);
        DrawViewportBorderFrames(0);
        DrawHudNumber3((PlayerCars[0].input * 160) / 1168, 0);
        DrawHudDigitMod10(PlayerCars[0].control.previewSample84.halves.high, 0);
        UpdateHudRankDisplay(PlayerCars[0].half1EE, 0);
        DrawRaceMinimap(0);
        DrawSteeringIndicator(&CurrentDrawBuf->ordering.ordering70.tags[0], 0);
        UpdateFinalLapHudSprite(0);
        SubmitRaceHudPackets(GameModeType);
        BuildCameraMatrices();
        DrawCars(0);
        RR_659_SCRATCH->word78 = RaceQuitTargetMode;
        SetupPlayerView(0);
        DrawTrackObjects(0);
        DrawTrackSceneryForPlayer(0);
        UpdateAndDrawRearViewMirror(ScreenFrame);
        return;
    }

    if (tick)
        ScreenFrame = ScreenFrame + 1U;
    if (tick && (RaceState - 2U) < 2U) {
        if ((RaceTimeRemaining = RaceTimeRemaining - 1U) <= 0) {
            RaceState = 5;
            PlayerCars[0].half1EE = 9;
            FadeCountdown = RR_PAL_NTSC(37, 45);
            RankEntries[0].score00 = (0U - TrackLength);
            SoundSetMasterVolume(0, RR_PAL_NTSC(33, 41));
        }
    }

    if (!tick) {
        if (RaceState >= 4 && FadeCountdown < RR_PAL_NTSC(25, 30))
            AddScreenFadeTiles(&CurrentDrawBuf->ordering.ordering70.tags[0], (RR_PAL_NTSC(25, 30) - FadeCountdown) * 10,
                               CarTable[0]->half1EE <= ovl659_finish_rank_limits[SelectionState.fields.phase17 >> 1] ? 37 : 69);
    } else if (RaceContext.opening88.done9C != 0 && RaceState == 0) {
        ScreenFrame = RR_PAL_NTSC(75, 90);
        ModeFrame = RR_PAL_NTSC(75, 90);
        SlotsStartForRace();
        RaceState = 1;
        PlayerCars[0].flags1E4 |= 0x100;
        ResetCarSlotBuffers();
        SetModelTexturePage(-1);
    } else if (RaceState == 1) {
        if (ModeFrame >= RR_PAL_NTSC(176U, 211U)) {
            UpdateCarStartLineSteering(PlayerCars);
            RaceState = 2;
            CdAudioBeginPlay(1);
        }
    } else if (RaceState >= 4) {
        s32 a2;
        if (FadeCountdown < RR_PAL_NTSC(25, 30)) {
            if (CarTable[0]->half1EE <= ovl659_finish_rank_limits[SelectionState.fields.phase17 >> 1]) {
                a2 = 37;
            } else {
                a2 = 69;
            }
            AddScreenFadeTiles(&CurrentDrawBuf->ordering.ordering70.tags[0], (RR_PAL_NTSC(25, 30) - FadeCountdown) * 10, a2);
        }
        if (FadeCountdown <= 0) {
            SelectionState.fields.result19 = PlayerCars[0].half1EE;
            if (PlayerCars[0].half1EE <= ovl659_finish_rank_limits[SelectionState.fields.phase17 / 2] &&
                RaceState == 4) {
                ModeGroup = 1;
                ModeIndex = 21;
                SubmodeState = 0;
            } else {
                FinishRace();
            }
        } else {
            FadeCountdown--;
        }
    }

    if (tick)
        UpdateLapTiming(PlayerCars);
    if (RaceState < 4) {
        DrawStartCountdown(&CurrentDrawBuf->ordering.ordering70.tags[0], ModeFrame);
        if (tick)
            PlayStartCountdownVoice(ModeFrame);
    }
    if (tick) {
        UpdateRaceRanking();
        CheckFinalLapEvent((char *)PlayerCars);
    }
    if ((RaceState - 1U) < 3U) {
        DrawLapTimeList(0);
        DrawTimeRemaining(RaceTimeRemaining);
        DrawHudRotatingQuad(PlayerDisplayRpm[0], 0);
        DrawViewportBorderFrames(0);
        DrawHudNumber3((PlayerCars[0].input * 160) / 1168, 0);
        DrawHudDigitMod10(PlayerCars[0].control.previewSample84.halves.high, 0);
        UpdateHudRankDisplay(PlayerCars[0].half1EE, 0);
        DrawRaceMinimap(0);
        DrawSteeringIndicator(&CurrentDrawBuf->ordering.ordering70.tags[0], 0);
        UpdateFinalLapHudSprite(0);
        SubmitRaceHudPackets(GameModeType);
    }
    /* the frame's own work before its step (rr/cpu_work.h) */
    RrSpendRaceModeOwn(2);
    StepRaceFrame(GameModeType);
    if (RaceState == 0) {
        DrawLensFlareAndSun(PlayerCars);
    } else if (!tick) {
    } else if (RaceState == 7) {
        UpdateFinishingPlayerCar(&PlayerCars[1]);
    } else {
        UpdateRaceChaseCamera(PlayerCars);
    }
    if (tick)
        UpdateTrackObjects();
    BuildCameraMatrices();
    DrawCars(0);
    if (PlayerCars[0].half1EA != ReverseCourse) {
        DrawRaceStartSignalSprite(0);
    }
    RR_659_SCRATCH->word78 = RaceQuitTargetMode;
    SetupPlayerView(0);
    DrawTrackObjects(0);
    DrawTrackSceneryForPlayer(0);
    UpdateAndDrawRearViewMirror(ScreenFrame);
    if (RaceState == 0) {
        RR_GPU_SCRATCH->cursor = AddDrawAreaPrim(
            CurrentDrawBuf->ordering.ordering70.tags + 2, RR_GPU_SCRATCH->cursor, 0, 0, 320, 256);
        DrawBuffers[1].environment.draw00.background = 1;
        DrawBuffers[0].environment.draw00.background = 1;
        RR_GPU_SCRATCH->cursor =
            AddDrawAreaPrim(CurrentDrawBuf->ordering.ordering70.tags + 702, RR_GPU_SCRATCH->cursor, 0,
                          RaceStartWipeY, 320, (256U - RaceStartWipeY * 2U));
        /* this block's own (its primitives held after the step: 765 on
         * DuckStation from UpdateAndDrawRearViewMirror to SlotSetLevelProgram) */
        RR_CPU_CYCLES(RR_CYCLES_GP_START_WIPE);
        if (tick && (RaceStartWipeY = (RaceStartWipeY - 16U)) < 0) {
            RaceStartWipeY = 0;
            DrawBuffers[1].environment.draw00.background = 0;
            DrawBuffers[0].environment.draw00.background = 0;
        }
    }
    if (tick)
        SlotSetLevelProgram(PlayerDisplayRpm[0], PlayerThrottleLoad[0], 0);
    if (RaceState >= 4) {
        StopRaceRumble();
        if (RaceState == 6) {
            StopRaceAudioAndRumble();
        }
    }
}

/* Initial values of the globals this module owns. */
s32 RaceQuitTargetMode;
s32 RaceStartWipeY;
