#include "rr/release.h"
#include "rr/video_timing.h"
#include "rr/widescreen.h"
#include "rr/race_frame_scratch.h"
#include "game/car/carstat.h"
#include "rr/runtime_settings.h"
#include "game/car/carparam.h"
#include "rr/menu_selection.h"
#include "rr/replay_types.h"
#include "rr/frame_storage_types.h"
#include "rr/vehicle_table.h"
#include "scratchpad.h"
#include "game/gfx/scene1.h"
#include "game/sound/sndcmd.h"
#include "game/ui/hudmap2.h"
#include "game/gfx/gfxdraw.h"
#include "game/sound/sndvab.h"
#include "game/system/pad.h"
#include "game/system/pad3.h"
#include "game/race/objclr.h"
#include "game/race/objsim.h"
#include "game/race/objdraw.h"
#include "game/race/raceai2.h"
#include "game/race/racesave.h"
#include "game/data/screenframe.h"
#include "rr/record_banks.h"
#include "rr/screen_tiles.h"
#include "game/ui/badgespr.h"
#include "game/car/carbound.h"
#include "game/race/racemai2.h"
#include "game/sound/cdstart.h"
#include "game/math/rotmat.h"
#include "game/camera/viewcam.h"
#include "rr/trig.h"
#include "game/sound/sndctrl.h"
#include "game/gfx/gtepoly1.h"
#include "game/menu/mnchoice.h"
#include "game/sound/sndslot.h"



extern int ReplayState;
extern int SavedRaceState;
extern int RaceState;
extern short TwistRangeLimits[];
extern int ReplayFrame;
extern short LinkHostFlag;
extern short GameModeType;
extern short FadeCountdown;
extern int TrackLength;
extern RaceVehicle PlayerCars[2];
extern short ModeIndex;
extern short ModeGroup;
extern short SubmodeState;
extern int ReplayFocusCarIndex;
extern int RaceQuitTargetMode;
extern int ModeFrame;

void ReadCarInput(RaceVehicle *arg0, s32 arg1);
void ReplayModeEnter();
void ReplayModeUpdate(void);

/* Callers explicitly supply this legacy argument; the body ignores it. */
void SaveCarSnapshots(int unused)
{
  int i;
  if (ReplayState == 0)
  {
    SavedRaceState = RaceState;
    for (i = 0; i < NumCars; i++)
      *CarSnapshots[i] = *CarTable[i];
    ReplayState = 1;
  }
}

void ReadCarInput(RaceVehicle *arg0, s32 arg1) {
    RaceVehicleControl *control;
    short t3;
    short t5;
    short t1;
    short a1;
    int idx;
    int t2;    int b;
    int f;
    int mode;

    control = &arg0->control;
    if (RaceState != 0) {
        if (ReplayState == 2) {
            if (ReplayFrame < ReplayLength) {
                CarInputs[arg1].frame = ReplayInputs[arg1][ReplayFrame];
                if (GameModeType == 4) {
                    if (arg1 == 3) {
                        ReplayFrame += 1;
                    }
                } else if (arg1 == 1 || GameModeType < 2 || GameModeType == 6) {
                    ReplayFrame += 1;
                }
            }
        } else {
            t3 = 0;
            t2 = idx = arg1 & 1;
            if (RaceState < 4) {
                mode = PlayerControllers[idx].protocol10;
                if (mode == 0) {
                    t1 = 0;
                    if (PlayerControllers[idx].input00.fields.buttons & PadButtonMap[idx].standard[2]) {
                        t1 = 0xFF;
                    }
                    b = (PlayerControllers[idx].input00.fields.buttons & PadButtonMap[idx].standard[3]) != 0;
                    {
                        int f1, tand;
                        tand = PlayerControllers[idx].input00.fields.buttons & PadButtonMap[idx].standard[0];
                        f1 = tand != 0;
                        if ((PlayerControllers[idx].input00.fields.buttons & PadButtonMap[idx].standard[1]) > 0) {
                            f1 |= 2;
                        }
                        if ((PlayerControllers[idx].edges08.fields.pressed & PadButtonMap[idx].standard[4]) > 0) {
                            f1 |= 4;
                        }
                        if ((PlayerControllers[idx].edges08.fields.pressed & PadButtonMap[idx].standard[5]) > 0) {
                            f1 |= 8;
                        }
                        f = f1;
                        if ((PlayerControllers[idx].edges08.fields.pressed & PadButtonMap[idx].standard[7]) > 0) {
                            f |= 0x10;
                        }
                    }
                    /* DualShock analog steering (class 0x80) is PAL-only. */
                    if (!RR_RELEASE_JP && PlayerControllers[idx].input00.fields.type == 0x73) {
                        t5 = (PlayerControllers[idx].steering3C * 0x7F) / 128;
                        t3 = f | 0x80;
                    } else {
                        t5 = 0;
                        t3 = f | 0x20;
                    }
                    control->dynamics94.player.digitalThrottle1C += b * 4;
                    if (b == 0) {
                        control->dynamics94.player.digitalThrottle1C = 0;
                    }
                    if (control->dynamics94.player.digitalThrottle1C >= 0x74) {
                        control->dynamics94.player.digitalThrottle1C = 0x73;
                    }
                    a1 = control->dynamics94.player.digitalThrottle1C + (b * 140);
                } else if (mode == 1) {
                    t5 = (PlayerControllers[idx].steering3C * 0x7F) / TwistRangeLimits[Options.pad[idx].range0A];
                    t1 = 0;
                    if (PlayerControllers[idx].input00.fields.buttons & PadButtonMap[idx].calibrated[2]) {
                        t1 = 0xFF;
                    }
                    a1 = 0;
                    if (PlayerControllers[idx].input00.fields.buttons & PadButtonMap[idx].calibrated[3]) {
                        a1 = 0xFF;
                    }
                    {
                        int fm1;
                        fm1 = ((PlayerControllers[idx].edges08.fields.pressed & PadButtonMap[idx].calibrated[4]) != 0) * 4;
                        if ((PlayerControllers[idx].edges08.fields.pressed & PadButtonMap[idx].calibrated[5]) > 0) {
                            fm1 |= 8;
                        }
                        if ((PlayerControllers[idx].edges08.fields.pressed & PadButtonMap[idx].calibrated[7]) > 0) {
                            fm1 |= 0x10;
                        }
                        t3 = fm1 | 0x40;
                    }
                    switch (Options.pad[idx].buttonLayout0C) {
                    case 0:
                    case 5: {
                        int i0;
                        t1 = (PlayerControllers[i0 = t2].pressure22 * 0xFF) / 106;
                        a1 = (PlayerControllers[i0].pressure24 * 0xFF) / 106;
                        break;
                    }
                    case 1:
                    case 6: {
                        int i1;
                        t1 = (PlayerControllers[i1 = t2].pressure24 * 0xFF) / 106;
                        a1 = (PlayerControllers[i1].pressure22 * 0xFF) / 106;
                        break;
                    }
                    case 2: {
                        int i2;
                        a1 = (PlayerControllers[i2 = t2].pressure26 * 0xFF) / 106;
                        break;
                    }
                    case 3: {
                        int i3;
                        t1 = (PlayerControllers[i3 = t2].pressure24 * 0xFF) / 106;
                        a1 = (PlayerControllers[i3].pressure26 * 0xFF) / 106;
                        break;
                    }
                    case 4:
                    case 7:
                        break;
                    }
                } else if (mode == 2) {
                    t5 = (PlayerControllers[idx].steering3C * 0x7F) / 128;
                    t1 = 0;
                    if (PlayerControllers[idx].input00.fields.buttons & PadButtonMap[idx].wheel[2]) {
                        t1 = 0xFF;
                    }
                    b = (PlayerControllers[idx].input00.fields.buttons & PadButtonMap[idx].wheel[3]) != 0;
                    control->dynamics94.player.digitalThrottle1C += b * 4;
                    if (b == 0) {
                        control->dynamics94.player.digitalThrottle1C = 0;
                    }
                    if (control->dynamics94.player.digitalThrottle1C >= 0x74) {
                        control->dynamics94.player.digitalThrottle1C = 0x73;
                    }
                    a1 = control->dynamics94.player.digitalThrottle1C + (b * 140);
                    {
                        unsigned short u2;                        int tand;                        int fb, fc;
                        u2 = PlayerControllers[idx].input00.fields.buttons;
                        tand = u2 & PadButtonMap[idx].wheel[0];
                        f = tand != 0;
                        if ((u2 & PadButtonMap[idx].wheel[1]) > 0) {
                            f |= 2;
                        }
                        fb = f;
                        if ((PlayerControllers[idx].edges08.fields.pressed & PadButtonMap[idx].wheel[4]) > 0) {
                            fb |= 4;
                        }
                        fc = fb;
                        if ((PlayerControllers[idx].edges08.fields.pressed & PadButtonMap[idx].wheel[5]) > 0) {
                            fc |= 8;
                        }
                        if ((PlayerControllers[idx].edges08.fields.pressed & PadButtonMap[idx].wheel[7]) > 0) {
                            fc |= 0x10;
                        }
                        t3 = fc | 0x60;
                    }
                } else {
                    a1 = 0;
                    t1 = 0;
                }
            } else {
                t1 = 0;
                a1 = 0;
            }
            CarInputs[arg1].steer = t5;
            CarInputs[arg1].throttle = t1;
            CarInputs[arg1].brake = a1;
            CarInputs[arg1].buttons = t3;
            if (GameModeType == 4) {
                if (LinkHostFlag != 0) {
                    if (arg1 >= 2) {
                        CarInputs[arg1] = LinkRemoteCarInputs.cars[t2];
                    } else {
                        LinkPendingCarInputs.cars[t2] = CarInputs[arg1];
                        CarInputs[arg1] = LinkLocalCarInputs.cars[t2];
                    }
                } else {
                    if (arg1 < 2) {
                        CarInputs[arg1] = LinkRemoteCarInputs.cars[t2];
                    } else {
                        LinkPendingCarInputs.cars[t2] = CarInputs[arg1];
                        CarInputs[arg1] = LinkLocalCarInputs.cars[t2];
                    }
                };                if (ReplayState == 1) {
                    if (ReplayFrame < ReplayLength) {
                        ReplayInputs[arg1][ReplayFrame] = CarInputs[arg1].frame;
                        if (arg1 == 3) {
                            ReplayFrame += 1;
                        }
                    }
                }
            } else if (ReplayState == 1) {
                if (ReplayFrame < ReplayLength) {
                    ReplayInputs[arg1][ReplayFrame] = CarInputs[arg1].frame;
                    if (arg1 == 1 || GameModeType < 2 || GameModeType == 6) {
                        ReplayFrame += 1;
                    }
                }
            }
        }
    }
}

void CheckRaceFinish(RaceVehicle *vehicle) {
    RaceVehicleControl *control = &vehicle->control;
    int a1 = control->dynamics94.steering.count22;
    if (vehicle->second + vehicle->first >= a1 * TrackLength) {
        unsigned short u = (u16)control->dynamics94.steering.count22;
        if (NumLaps >= a1) {
            control->dynamics94.steering.count22 = u + 1;
            if (NumLaps < (short)(u + 1)) {
                if (RaceState < 4) {
                    if (FadeCountdown == -1) {
                        RaceState = 4;
                        FadeCountdown = RR_PAL_NTSC(0x4B, 0x5A);
                        SoundSetMasterVolume(0, RR_PAL_NTSC(0x47, 0x56));
                    }
                }
            }
        }
    }
    if (ReplayFrame >= ReplayLength - RR_PAL_NTSC(0x4B, 0x5A) && FadeCountdown == -1) {
        FadeCountdown = RR_PAL_NTSC(0x25, 0x2D);
        SoundSetMasterVolume(0, RR_PAL_NTSC(0x21, 0x29));
    } else if (MenuReturnSubmodeIndex != 0) {
        if (FadeCountdown == -1) {
            FadeCountdown = RR_PAL_NTSC(0x25, 0x2D);
            SoundSetMasterVolume(0, RR_PAL_NTSC(0x21, 0x29));
        }
    }
}

void ReplayModeEnter(void) {
    SoundInitForRace();
    SelectReplayBgmTrack();
    AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], 0xFF, 0x25);
    SetupRaceMemoryArena(0, 2);
    RaceState = SavedRaceState;
    ClearTrackObjectStates(1);
    SetModelTexturePage(10);
    ReplayFocusCarIndex = 0;
    if (((unsigned short)GameModeType - 2) < 2U) {
        if (PlayerCars[0].first < PlayerCars[1].first) ReplayFocusCarIndex = 1;
    }
    FadeCountdown = -1;
    ModeGroup = 1;
    ModeIndex = 0x16;
    SubmodeState = 0;
}

void ReplayModeUpdate(void) {
    int i;
    short t;
    ModeFrame += 1;
    if (ReplayFrame < RR_PAL_NTSC(0x1A, 0x1F)) {
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (RR_PAL_NTSC(0x19, 0x1E) - ReplayFrame) * 10, 0x25);
        if (ReplayFrame == 10) {
            CdAudioBeginPlay(0);
        }
    } else {
        UpdateAndDrawSceneOverlay();
    }
    ScreenFrame += 1;
    if (FadeCountdown >= 0) {
        if (FadeCountdown == 0) {
            SetReverbDepthBoth(0);
            FinishRace();
        }
        t = (unsigned short)FadeCountdown - 1;
        FadeCountdown = t;
        if (t < RR_PAL_NTSC(0x19, 0x1E)) {
            AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (RR_PAL_NTSC(0x19, 0x1E) - t) * 10, 0x45);
        }
    }
    if (rsin((ReplayFrame << 7) & 0xF80) > 0) {
        RR_GPU_SCRATCH->cursor = AddSpriteWithDrawMode(&CurrentDrawBuf->buffer.ordering70.tags[0], RR_GPU_SCRATCH->cursor, WIDE_HUD_LEFT(0x10), 0x10, 0x4C, 0xC, 0x64, 0xC0, 5, 0x7FF8, 0, 0x80);
    }
    for (i = 0; i < NumCars; i++) {
        if ((s16)CarTable[i]->flags1E4 >= 0x10) {
            CheckRaceFinish(CarTable[i]);
        }
    }
    StepRaceFrame(GameModeType);
    if (((unsigned short)GameModeType - 2) < 2U) {
        if ((unsigned char)ReplayFrame == 0) {
            if (PlayerCars[0].first < PlayerCars[1].first) {
                ReplayFocusCarIndex = 1;
            } else {
                ReplayFocusCarIndex = 0;
            }
        }
    }
    SetupViewport(0);
    UpdateRaceCamera(2, &PlayerCars[ReplayFocusCarIndex]);
    UpdateTrackObjects(0);
    BuildCameraMatrices();
    DrawCars(6);
    ((RaceFrameScratch *)SCRATCHPAD)->word78 = RaceQuitTargetMode;
    SetupPlayerView(0);
    DrawTrackObjects(0);
    DrawTrackSceneryForPlayer(0);
    if (ReplayFrame >= RR_PAL_NTSC(0x19, 0x1E)) {
        QueueDrawStp();
    }
    SlotSetLevelProgram(PlayerDisplayRpm[0], PlayerThrottleLoad[0], 0);
    SlotSetLevelProgram(PlayerDisplayRpm[1], PlayerThrottleLoad[1], 1);
    PadClearPulse(0);
    PadClearPulse(1);
    PadRumbleStop(0);
    PadRumbleStop(1);
}

/* Initial values of the globals this module owns. */
int ReplayFrame;
int SavedRaceState;
CarInput4 CarInputs[4];
s16 PlayerThrottleLoad[2];
PadButtonMap30 PadButtonMap[2];
s32 ReplayLength;
int ReplayFocusCarIndex;
