#include "game/system/pad.h"
#include "game/data/menuvars.h"
#include "rr/overlay_module.h"
#include "game/car/carparam.h"
#include "rr/menu_selection.h"
#include "game/sound/sndque.h"
#include "scratchpad.h"
#include "rr/overlay667_checkpoint.h"
#include "rr/overlay_screen_exit.h"
#include "rr/frame_storage_types.h"
#include "rr/vehicle_table.h"
#include "types.h"
#include "game/race/racegrid.h"
#include "rr/race_types.h"
#include "rr/frame_loop.h"
#include "rr/gpu_scratch_types.h"
#include "game/sound/sndctrl.h"
#include "game/gfx/gtepoly1.h"
#include "game/ui/badge1.h"
#include "game/gfx/scrtile.h"
#include "game/ui/window1.h"
#include "game/ui/window2.h"
#include "game/ui/hudnum3.h"
#include "game/ui/hudnum4.h"
#include "game/ui/hudpl.h"
#include "game/ui/hudctrl.h"
#include "game/ui/hudmap.h"
#include "game/car/carsetup.h"
#include "game/car/carbound.h"
#include "game/race/raceai2.h"
#include "game/menu/mnchoice.h"
#include "game/race/racemai2.h"
#include "game/race/raceset.h"
#include "game/sound/sndvab.h"
#include "game/sound/sndslot.h"
#include "game/sound/cdstart.h"
#include "game/math/rotmat.h"
#include "game/camera/viewcam.h"
#include "game/gfx/poly1.h"
#include "game/race/objsim.h"
#include "game/race/objdraw.h"
#include "rr/driver_control.h"
#include "rr/race_setup.h"
#include "rr/video_timing.h"

extern s32 TrackLength;
extern s32 RaceState;
extern s32 TotalRaceTime;
extern void SaveCarSnapshots(s32);

void ovl667_update_checkpoint(RaceVehicle *arg0) {
    RaceVehicleControl *sub = &arg0->control;
    /* MIPS sums and products retain their low 32 bits. */
    if ((s32)((u32)arg0->second + arg0->first) >= (s32)((u32)(s32)sub->dynamics94.steering.count22 * TrackLength)) {
        if (sub->dynamics94.steering.count22 <= NumLaps) {
            sub->dynamics94.steering.count22=(s16)((u16)sub->dynamics94.steering.count22+1U);
            if (sub->dynamics94.steering.count22 == NumLaps) {
                SaveCarSnapshots(0);
            } else if (sub->dynamics94.steering.count22 > NumLaps) {
                if (RaceState < 4) {
                    TotalRaceTime = 1;
                    RaceState = 4;
                    FadeCountdown = RR_PAL_NTSC(75, 90);
                    SoundSetMasterVolume(0, RR_PAL_NTSC(71, 86));
                    SoundQueueEnqueue(19, 1);
                }
            }
        }
    }
}

void ovl667_exit_race(void)
{
    StopRaceAudioAndRumble();
    SetupRaceAndSceneId();
    ModeGroup=1;
    ModeIndex=26;
    SubmodeState=0;
    AddScreenFadeTiles(CurrentDrawBuf->buffer.ordering70.tags,255,69);
}

/* Versus (serial link) race frame: drives the shared race state machine,
 * exchanges pause/menu input through the serial packet bytes and renders
 * one or two viewports. */

typedef struct {
    s32 words[4];
} Ovl667Vector16;


extern s16 LinkPlayerCount;
extern s32 RaceStartWipeY;
extern s32 RaceState;
extern s32 TotalRaceTime;
extern s32 RaceTimeRemaining;
extern s8 RacePaused;
extern s8 PauseCursor;
extern RaceVehicle PlayerCars[];


extern s32 ovl667_link_shutdown_stage;
extern s32 ovl667_pause_packet_slot;
extern u8 ovl667_active_slot_index, ovl667_consumed_slot_index;
extern u8 ovl667_pending_pause_bits, ovl667_remote_pause_bits, ovl667_local_pause_bits;
extern Ovl667Vector16 ovl667_camera_rotations[];
extern Ovl667Vector16 ovl667_camera_positions[];

extern void ovl667_close_sio_session(void);
extern void ovl667_receive_peer_inputs(void);
extern void ovl667_transmit_local_inputs(void);
extern void ovl667_abort_on_link_timeout(void);
void DrawTimeRemainingForPlayer(s32, s32);
void UpdateRaceIntroCamera(RaceVehicle *, s32);

/* Scratch view: GPU pointer placeholder (RR_GPU_SCRATCH), the camera
 * position and rotation the viewport code reads, and the split-frame mode. */
typedef struct {
    u8 gpu_slot00[8];
    Ovl667Vector16 position08, rotation18;
    u8 reserved28[0x5E - 0x28];
    s16 mode5E;
} Ovl667FrameScratch;

#define SCRATCH RR_GPU_SCRATCH
#define RR_667_FRAME_SCRATCH ((Ovl667FrameScratch *)SCRATCHPAD)

void ovl667_update_race(void) {
    s32 split;
    GpuDmaTag *frame;
    s32 fade;
    s32 i;
    s32 n;
    s32 state;
    u16 buttons;

    split = (LinkPlayerCount != 2) << 1;
    if (split == 0) {
        frame = &CurrentDrawBuf->buffer.ordering70.tags[0];
    } else {
        RR_667_FRAME_SCRATCH->mode5E = 1;
        frame = &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0];
    }
    if (ModeFrame < 2) {
        SetDispMask(0);
    } else {
        SetDispMask(1);
    }
    SetModelTexturePage(-1);
    if (ovl667_link_shutdown_stage == 2) {
        ovl667_close_sio_session();
        ovl667_link_shutdown_stage = 3;
    } else if (ovl667_link_shutdown_stage == 3) {
        _sio_control(1, 1, 3);
    }
    ovl667_receive_peer_inputs();

    if (ModeFrame >= RR_PAL_NTSC(75, 90) && RaceState == 0) {
        RaceState = 1;
        SlotsStartForRace();
    } else if (RaceState == 1) {
        if (ModeFrame >= RR_PAL_NTSC(176, 211)) {
            for (i = 0; i < 4; i++) {
                UpdateCarStartLineSteering(CarTable[i]);
            }
            RaceState = 2;
            CdAudioBeginPlay(1);
            FadeCountdown = 30;
        }
    } else if (RaceState >= 4) {
        fade = 37;
        if (TotalRaceTime == -1) {
            fade = 69;
        }
        if (FadeCountdown < RR_PAL_NTSC(25, 30)) {
            AddScreenFadeTiles(frame, (RR_PAL_NTSC(25, 30) - FadeCountdown) * 10, fade);
            SCRATCH->cursor = AddDrawAreaPrim(frame, SCRATCH->cursor, 0, 0, 320, 256);
        }
        if (FadeCountdown <= 0) {
            if (ovl667_link_shutdown_stage != 3) {
                ovl667_close_sio_session();
            }
            for (i = 0; i < NumCars; i++) {
                LinkPrevCarRanks[i] = CarTable[i]->half1EE;
            }
            FinishRace();
            _sio_control(1, 1, 3);
        } else {
            FadeCountdown--;
        }
    } else if (ModeFrame >= RR_PAL_NTSC(51, 61) && RaceState == 0) {
        AddScreenFadeTiles(frame, (ModeFrame - RR_PAL_NTSC(50, 60)) * 10, 37);
        SCRATCH->cursor = AddDrawAreaPrim(frame, SCRATCH->cursor, 0, 0, 320, 256);
    }

    for (i = 0; i < NumCars; i++) {
        ovl667_update_checkpoint(CarTable[i]);
    }
    UpdateRaceRanking();

    if (ovl667_active_slot_index != ovl667_consumed_slot_index) {
        buttons = Pad0Buttons;
        ovl667_pending_pause_bits = (buttons & 0x800) && FadeCountdown == 0;
        ovl667_pending_pause_bits |= (buttons & 0x1000) ? 2 : 0;
        ovl667_pending_pause_bits |= (buttons & 0x4000) ? 4 : 0;
        state = RaceState;
        if ((state - 1) < 3U) {
            if ((ovl667_local_pause_bits | ovl667_remote_pause_bits) & 1) {
                RacePaused ^= 1;
                ovl667_pause_packet_slot = ovl667_active_slot_index;
                if (RacePaused == 0) {
                    FadeCountdown = 10;
                    SoundResume();
                    ApplyPauseMenuChoice(GameModeType);
                    if (PauseCursor != 0) {
                        ovl667_close_sio_session();
                        StopRaceAudioAndRumble();
                        return;
                    }
                } else {
                    SoundPause();
                }
                PauseCursor = 0;
            } else if (FadeCountdown > 0 && state < 4) {
                FadeCountdown--;
            }
        }
        if (RacePaused == 0) {
            ModeFrame++;
            ScreenFrame++;
            if ((RaceState - 2) < 2U) {
                if (--RaceTimeRemaining <= 0) {
                    RaceState = 5;
                    FadeCountdown = RR_PAL_NTSC(75, 90);
                    SoundSetMasterVolume(0, RR_PAL_NTSC(33, 41));
                }
            }
            StepRaceFrame(4);
        } else {
            if ((ovl667_local_pause_bits | ovl667_remote_pause_bits) & 2) {
                if (PauseCursor > 0) {
                    PlaySfx(0);
                    PauseCursor--;
                }
            }
            if ((ovl667_local_pause_bits | ovl667_remote_pause_bits) & 4) {
                if (PauseCursor < 2) {
                    PlaySfx(0);
                    PauseCursor++;
                }
            }
            DrawPauseMenu(frame, PauseCursor);
            InitPlayerRumble();
        }
        ovl667_consumed_slot_index = ovl667_active_slot_index;
    } else {
        ovl667_pending_pause_bits = 0;
    }

    n = 1;
    ovl667_transmit_local_inputs();
    if (split != 0) {
        n = 2;
    }
    if (RaceState < 4) {
        DrawStartCountdown(frame, ModeFrame);
        if (RacePaused == 0) {
            PlayStartCountdownVoice(ModeFrame);
        }
    }
    if ((RaceState - 1) < 3U) {
        for (i = 0; i < n; i++) {
            if (i == 1 && PlayerCars[i].half1E8 == -1) {
                SCRATCH->cursor = AddTilePrim(frame, SCRATCH->cursor, 0, 128, 320, 128, 8, 8, 8);
            }
            if (n == 1) {
                if (PlayerCars[i].half1E8 == -1) {
                    continue;
                }
                DrawHudRotatingQuad(PlayerDisplayRpm[i], i);
                DrawRaceMinimap(0);
                DrawLapCounter(0, PlayerCars[i].control.dynamics94.steering.count22);
                DrawSteeringIndicator(frame, 0);
            } else {
                DrawHudNumber5(PlayerDisplayRpm[i], i);
                DrawRaceMinimap(i + 1);
                DrawLapCounter(i + 1, PlayerCars[i].control.dynamics94.steering.count22);
                DrawSteeringIndicator(frame, i + 1);
            }
            DrawHudNumber3((PlayerCars[i].input * 160) / 1168, i);
            DrawHudDigitMod10(PlayerCars[i].control.previewSample84.halves.high, i);
            UpdateHudRankDisplay((s16)PlayerCars[i].half1EE, i);
            DrawTimeRemainingForPlayer(RaceTimeRemaining, i);
            UpdateFinalLapHudSprite(i + 1);
            if (i == 1) {
                DrawSplitScreenDivider(0);
            }
        }
        DrawViewportBorderFrames(split);
        SubmitRaceHudPackets(3);
    }

    for (i = 0; i < n; i++) {
        if (PlayerCars[i].half1E8 != -1) {
            RR_667_FRAME_SCRATCH->rotation18 = ovl667_camera_rotations[i];
            RR_667_FRAME_SCRATCH->position08 = ovl667_camera_positions[i];
            SetupViewport(split + i);
            if (RaceState == 0 && ModeFrame < RR_PAL_NTSC(75, 90)) {
                UpdateRaceIntroCamera(&PlayerCars[i], ModeFrame);
            } else {
                UpdateRaceChaseCamera(&PlayerCars[i]);
            }
            ovl667_camera_rotations[i] = RR_667_FRAME_SCRATCH->rotation18;
            ovl667_camera_positions[i] = RR_667_FRAME_SCRATCH->position08;
            BuildCameraMatrices();
            DrawCars(i + ((s16)(LinkHostFlag ^ 1) + 1) * 2);
            if (PlayerCars[i].half1EA != 0 && RacePaused == 0) {
                DrawRaceStartSignalSprite(n == 1 ? 0 : i + 1);
            }
            SetupPlayerView(i);
            UpdateTrackObjects();
            DrawTrackObjects(0);
            DrawTrackSceneryForPlayer(i + 1);
            if (n == 1) {
                UpdateAndDrawRearViewMirror(ScreenFrame);
            }
            if (RaceState == 0 && n == 1) {
                DrawBuffers[1].environment.draw00.background = n;
                DrawBuffers[0].environment.draw00.background = n;
                SCRATCH->cursor = AddDrawAreaPrim(frame + 702, SCRATCH->cursor, 0, RaceStartWipeY, 320,
                                               256 - (RaceStartWipeY << n));
                if (i == 0) {
                    if ((RaceStartWipeY -= 16) < 0) {
                        RaceStartWipeY = 0;
                        DrawBuffers[1].environment.draw00.background = 0;
                        DrawBuffers[0].environment.draw00.background = 0;
                    }
                }
            }
            SlotSetLevelProgram(PlayerDisplayRpm[i], PlayerThrottleLoad[i], i);
            if (i == 0) {
                ovl667_transmit_local_inputs();
            }
        }
    }

    if (RaceState >= 4) {
        StopRaceRumble();
        if (RaceState == 6) {
            StopRaceAudioAndRumble();
        }
    }
    if (ovl667_link_shutdown_stage == 0) {
        ovl667_abort_on_link_timeout();
    }
}

/* Initial values of the overlay globals this module owns. */
Ovl667Vector16 ovl667_camera_rotations[2];
Ovl667Vector16 ovl667_camera_positions[2];
s32 ovl667_pause_packet_slot;
u8 ovl667_pending_pause_bits;
u8 ovl667_remote_pause_bits;
u8 ovl667_local_pause_bits;
u8 ovl667_consumed_slot_index;

/* Writable state of this unit; ovl667_reset_module restores it. */
static const OverlayStateRegion ovl667_race_regions[] = {
    RR_OVERLAY_STATE(ovl667_camera_positions),
    RR_OVERLAY_STATE(ovl667_camera_rotations),
    RR_OVERLAY_STATE(ovl667_consumed_slot_index),
    RR_OVERLAY_STATE(ovl667_local_pause_bits),
    RR_OVERLAY_STATE(ovl667_pause_packet_slot),
    RR_OVERLAY_STATE(ovl667_pending_pause_bits),
    RR_OVERLAY_STATE(ovl667_remote_pause_bits)};
const OverlayStateTable ovl667_race_state = {ovl667_race_regions, sizeof(ovl667_race_regions) / sizeof(ovl667_race_regions[0])};
