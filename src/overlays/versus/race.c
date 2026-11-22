#include "rr/field_rate.h"
#include "rr/overlay_module.h"
#include "game/car/carparam.h"
#include "game/system/pad.h"
#include "rr/overlay661_checkpoint.h"
#include "rr/overlay661_race_controller.h"
#include "rr/frame_storage_types.h"
#include "rr/video_timing.h"
#include "rr/cpu_work.h"

extern s32 RaceVoiceFlags;

void ovl661_update_checkpoint(RaceVehicle *vehicle) {
    RaceVehicleControl *control = &vehicle->control;
    s16 counter;
    s32 target;
    s32 message;

    counter = control->dynamics94.steering.count22;
    if (((s32)((u32)vehicle->second + vehicle->first)) >= ((s32)((u32)(s32)counter * TrackLength))) {
        target = NumLaps;
        if (target >= counter) {
            control->dynamics94.steering.count22=(s16)((u16)control->dynamics94.steering.count22+1U);
            LapVoicePending = 0;
            if (control->dynamics94.steering.count22 == target) {
                SaveCarSnapshots(0);
            } else if (target < control->dynamics94.steering.count22) {
                if (RaceState < 4) {
                    LapVoicePending = -1;
                    RaceState = 4;
                    FadeCountdown = RR_PAL_NTSC(75, 90);
                    SoundSetMasterVolume(0, RR_PAL_NTSC(33, 41));
                    TotalRaceTime = 0;
                    SoundQueueEnqueue(19, 1);
                }
            }
        }
    }
    if (LapVoicePending == 0) {
        message = (9U - ((u32)NumLaps - control->dynamics94.steering.count22));
        if (message >= 6) {
            if (message == 9) {
                message = 15;
            }
            if (!(RaceVoiceFlags & (0x200U << (message - 6U)))) {
                SoundQueueEnqueue(message, 0);
                RaceVoiceFlags |= 0x200U << (message - 6U);
            }
        }
        LapVoicePending = -1;
    }
}

void ovl661_exit_race(void)
{
    StopRaceAudioAndRumble();
    SetupRaceAndSceneId();
    ModeGroup = 1;
    ModeIndex = 38;
    SubmodeState = 0;
    AddScreenFadeTiles(CurrentDrawBuf->buffer.ordering70.tags, 255, 69);
}

void ovl661_update_race(void) {
    s32 var_a2;
    s32 var_a2_2;
    s32 temp_a1;
    s32 temp_a1_2;
    s32 temp_a2;
    s32 temp_s0_2;
    s32 temp_s0_4;
    s32 temp_v0_3;
    s32 var_a1;
    s32 var_s2;
    s8 temp_v1;
    RaceVehicle *temp_s0;
    RaceVehicle *temp_s0_3;

    /* The mode's counters, timers and state run once per original tick;
     * the drawing every field (rr/field_rate.h). */
    const int tick = RaceSlowTick();
    if (tick)
        ModeFrame=ModeFrame+1U;
    if (!tick) {
    } else if (((RaceState - 1U) < 2U) && (PlayerControllers[0].edges08.fields.pressed & 0x800)) {
        if (FadeCountdown <= 0) {
            FadeCountdown = 0xF;
            temp_v1 = RacePaused == 0;
            RacePaused = temp_v1;
            if (temp_v1 != 0) {
                SoundPause();
                PauseCursor = 0;
            } else {
                ApplyPauseMenuChoice(GameModeType);
                SoundResume();
            }
        } else {
            goto block_7;
        }
    } else if (FadeCountdown > 0) {
block_7:
        if (RaceState < 4) {
            FadeCountdown = (u16) FadeCountdown - 1;
        }
    }
    if (RacePaused != 0) {
        if (tick) {
            UpdatePauseMenu(GameModeType);
            ModeFrame=ModeFrame-1U;
        }
        DrawPauseMenu(CurrentDrawBuf->buffer.secondaryOrderingB70.tags, PauseCursor);
        if (RaceState < 4) {
            DrawStartCountdown(CurrentDrawBuf->buffer.secondaryOrderingB70.tags, ModeFrame);
        }
        var_s2 = 0;
        if ((RaceState - 1U) < 3U) {
            do {

                temp_s0 = &PlayerCars[var_s2];
                DrawHudNumber3((temp_s0->input * 0xA0) / 1168, var_s2);
                DrawHudNumber5(PlayerDisplayRpm[var_s2], var_s2);
                DrawHudDigitMod10(temp_s0->control.previewSample84.halves.high, var_s2);
                UpdateHudRankDisplay(temp_s0->half1EE, var_s2);
                DrawTimeRemainingForPlayer(RaceTimeRemaining, var_s2);
                DrawLapCounter(var_s2 + 1, temp_s0->control.dynamics94.steering.count22);
                DrawRaceMinimap(var_s2 + 1);
                DrawSteeringIndicator(CurrentDrawBuf->buffer.secondaryOrderingB70.tags, var_s2 + 1);
                UpdateFinalLapHudSprite(var_s2 + 1);
                var_s2++;
            } while (var_s2 < 2);
            DrawSplitScreenDivider(0);
            DrawViewportBorderFrames(2);
            SubmitRaceHudPackets(2);
        }
        var_s2 = 0;
        do {
            RR_661_FRAME_SCRATCH->rotation18 = ovl661_camera_rotations[var_s2];
            RR_661_FRAME_SCRATCH->position08 = ovl661_camera_positions[var_s2];
            SetupViewport(var_s2 + 2);
            if (!tick) {
            } else if (RaceState == 0 && ModeFrame < RR_PAL_NTSC(75U, 90U)) {
                UpdateRaceIntroCamera(&PlayerCars[var_s2], ModeFrame);
            } else {
                UpdateRaceChaseCamera(&PlayerCars[var_s2]);
            }
            ovl661_camera_rotations[var_s2] = RR_661_FRAME_SCRATCH->rotation18;
            ovl661_camera_positions[var_s2] = RR_661_FRAME_SCRATCH->position08;
            temp_s0_2 = var_s2 + 1;
            BuildCameraMatrices();
            DrawCars(var_s2 + 2);
            SetupPlayerView(var_s2);
            DrawTrackObjects(0);
            DrawTrackSceneryForPlayer(temp_s0_2);
            var_s2 = temp_s0_2;
        } while (var_s2 < 2);
        return;
    }
    if (tick)
        ScreenFrame=ScreenFrame+1U;
    if (tick && (RaceState - 2U) < 2U) {
        temp_v0_3 = RaceTimeRemaining - 1U;
        RaceTimeRemaining = temp_v0_3;
        if (temp_v0_3 <= 0) {
            RaceState = 5;
            FadeCountdown = RR_PAL_NTSC(0x4B, 0x5A);
            SoundSetMasterVolume(0,RR_PAL_NTSC(0x21, 0x29));
        }
    }
    if (!tick) {
        if (RaceState >= 4 && FadeCountdown < RR_PAL_NTSC(0x19, 0x1E)) {
            AddScreenFadeTiles(CurrentDrawBuf->buffer.secondaryOrderingB70.tags, (RR_PAL_NTSC(0x19, 0x1E) - FadeCountdown) * 0xA,
                               TotalRaceTime == -1 ? 0x45 : 0x25);
            RR_GPU_SCRATCH->cursor = AddDrawAreaPrim(CurrentDrawBuf->buffer.secondaryOrderingB70.tags, RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x100);
        } else if (RaceState == 0) {
            if (ModeFrame >= RR_PAL_NTSC(51U, 61U)) {
                AddScreenFadeTiles(CurrentDrawBuf->buffer.secondaryOrderingB70.tags,(ModeFrame-RR_PAL_NTSC(50U, 60U))*10U,37);
            } else if (ModeFrame < RR_PAL_NTSC(25U, 30U)) {
                AddScreenFadeTiles(CurrentDrawBuf->buffer.secondaryOrderingB70.tags,(RR_PAL_NTSC(25U, 30U)-ModeFrame)*10U,69);
            }
            RR_GPU_SCRATCH->cursor = AddDrawAreaPrim(CurrentDrawBuf->buffer.secondaryOrderingB70.tags, RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x100);
        }
    } else if ((ModeFrame >= RR_PAL_NTSC(0x4BU, 0x5AU)) && (RaceState == 0)) {
        RaceState = 1;
        SlotsStartForRace();
        SetModelTexturePage(-1);
        ResetCarSlotBuffers();
        DrawBuffers[1].environment.draw00.background = 0;
        DrawBuffers[0].environment.draw00.background = 0;
    } else {
        if (RaceState==1) {
            if (ModeFrame >= RR_PAL_NTSC(0xB0U, 0xD3U)) {
                UpdateCarStartLineSteering(&PlayerCars[0]);
                UpdateCarStartLineSteering(&PlayerCars[1]);
                CdAudioBeginPlay(1);
                RaceState = 2;
            }
        } else if (RaceState>=4) {
            var_a2 = 0x25;
            if (TotalRaceTime == -1) {
                var_a2 = 0x45;
            }
            if (FadeCountdown < RR_PAL_NTSC(0x19, 0x1E)) {
                AddScreenFadeTiles(CurrentDrawBuf->buffer.secondaryOrderingB70.tags, (RR_PAL_NTSC(0x19, 0x1E) - FadeCountdown) * 0xA, var_a2);
                RR_GPU_SCRATCH->cursor = AddDrawAreaPrim(CurrentDrawBuf->buffer.secondaryOrderingB70.tags, RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x100);
            }
            if (FadeCountdown <= 0) {
                ovl661_finish_ranks[0] = (u16) PlayerCars[0].half1EE;
                ovl661_finish_ranks[1] = (u16) PlayerCars[1].half1EE;
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
        } else if (RaceState==0) {
            if (ModeFrame >= RR_PAL_NTSC(51U, 61U)) {
                AddScreenFadeTiles(CurrentDrawBuf->buffer.secondaryOrderingB70.tags,(ModeFrame-RR_PAL_NTSC(50U, 60U))*10U,37);
            } else if (ModeFrame < RR_PAL_NTSC(25U, 30U)) {
                AddScreenFadeTiles(CurrentDrawBuf->buffer.secondaryOrderingB70.tags,(RR_PAL_NTSC(25U, 30U)-ModeFrame)*10U,69);
            }
            RR_GPU_SCRATCH->cursor = AddDrawAreaPrim(CurrentDrawBuf->buffer.secondaryOrderingB70.tags, RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x100);
        }
    }
    if (tick) {
        ovl661_update_checkpoint(&PlayerCars[0]);
        ovl661_update_checkpoint(&PlayerCars[1]);
        UpdateRaceRanking();
    }
    if (!tick) {
    } else if ((s16) PlayerCars[0].half1EE < (s16) PlayerCars[1].half1EE) {
        PlayerCars[0].half1EE = 0U;
        PlayerCars[1].half1EE = 1U;
    } else {
        PlayerCars[0].half1EE = 1U;
        PlayerCars[1].half1EE = 0U;
    }
    StepRaceFrame(GameModeType);
    if (RaceState < 4) {
        DrawStartCountdown(CurrentDrawBuf->buffer.secondaryOrderingB70.tags, ModeFrame);
        if (tick)
            PlayStartCountdownVoice(ModeFrame);
    }
    var_s2 = 0;
    if ((RaceState - 1U) < 3U) {
        do {

                temp_s0_3 = &PlayerCars[var_s2];
            DrawHudNumber3((temp_s0_3->input * 0xA0) / 1168, var_s2);
            DrawHudNumber5(PlayerDisplayRpm[var_s2], var_s2);
            DrawHudDigitMod10(temp_s0_3->control.previewSample84.halves.high, var_s2);
            UpdateHudRankDisplay(temp_s0_3->half1EE, var_s2);
            DrawTimeRemainingForPlayer(RaceTimeRemaining, var_s2);
            DrawLapCounter(var_s2 + 1, temp_s0_3->control.dynamics94.steering.count22);
            DrawRaceMinimap(var_s2 + 1);
            DrawSteeringIndicator(CurrentDrawBuf->buffer.secondaryOrderingB70.tags, var_s2 + 1);
            UpdateFinalLapHudSprite(var_s2 + 1);
            var_s2++;
        } while (var_s2 < 2);
        DrawSplitScreenDivider(0);
        DrawViewportBorderFrames(2);
        SubmitRaceHudPackets(2);
    }
    /* the frame's own work besides its routines, after its HUD
     * (rr/cpu_work.h) */
    RrSpendRaceModeOwn(1);
    var_s2 = 0;
    do {
        RR_661_FRAME_SCRATCH->rotation18 = ovl661_camera_rotations[var_s2];
        RR_661_FRAME_SCRATCH->position08 = ovl661_camera_positions[var_s2];
        SetupViewport(var_s2 + 2);
        if (!tick) {
        } else if (RaceState == 0 && ModeFrame < RR_PAL_NTSC(75U, 90U)) {
            UpdateRaceIntroCamera(&PlayerCars[var_s2], ModeFrame);
        } else {
            UpdateRaceChaseCamera(&PlayerCars[var_s2]);
        }
        ovl661_camera_rotations[var_s2] = RR_661_FRAME_SCRATCH->rotation18;
        ovl661_camera_positions[var_s2] = RR_661_FRAME_SCRATCH->position08;
        BuildCameraMatrices();
        DrawCars(var_s2 + 2);
        if (PlayerCars[var_s2].half1EA != 0) {
            DrawRaceStartSignalSprite(var_s2 + 1);
        }
        RR_661_FRAME_SCRATCH->mode5E = 1;
        SetupPlayerView(var_s2);
        if (tick)
            UpdateTrackObjects();
        DrawTrackObjects(0);
        temp_s0_4 = var_s2 + 1;
        DrawTrackSceneryForPlayer(temp_s0_4);
        if (tick)
            SlotSetLevelProgram(PlayerDisplayRpm[var_s2], PlayerThrottleLoad[var_s2], var_s2);
        var_s2 = temp_s0_4;
    } while (var_s2 < 2);
    if (RaceState >= 4) {
        StopRaceRumble();
        if (RaceState == 6) {
            StopRaceAudioAndRumble();
        }
    }
}

/* Initial values of the overlay globals this module owns. */
Rr661FrameVector16 ovl661_camera_rotations[2];
Rr661FrameVector16 ovl661_camera_positions[2];

/* Writable state of this unit; ovl661_reset_module restores it. */
static const OverlayStateRegion ovl661_race_regions[] = {
    RR_OVERLAY_STATE(ovl661_camera_positions),
    RR_OVERLAY_STATE(ovl661_camera_rotations)};
const OverlayStateTable ovl661_race_state = {ovl661_race_regions, sizeof(ovl661_race_regions) / sizeof(ovl661_race_regions[0])};

/* Initial values of the globals this module owns. */
RaceVehicle PlayerCars[2];
