#include "rr/field_rate.h"
#include "types.h"
#include "game/data/menuvars.h"
#include "rr/trig.h"
#include "game/math/rotmat.h"
#include "game/car/bounds.h"
#include "game/car/carphys1.h"
#include "game/course/trknode.h"
#include "game/course/trkintp.h"
#include "game/sound/sndown.h"
#include "game/system/pad.h"
#include "game/car/carphys2.h"
#include "rr/steering.h"
#include "rr/audio_loop.h"
#include "rr/overlay661_init.h"
#include "rr/overlay668_camera.h"

extern RaceVehicle PlayerCars[2];

void StepCarStateNormal(RaceVehicle *c);
void StepCarStateSlide(RaceVehicle *c0);
void StepCarStateSpinOut(RaceVehicle *a0);
int LerpAngle12(int a0, int a1, int a2);

void SteerCarTowardTrack(RaceVehicle *arg0) {
    s32 sp[4];
    s32 temp_a1;
    s32 temp_hi;
    s32 temp_s0;
    s32 temp_s0_2;
    s32 temp_s2;
    s32 var_v1;
    s32 var_v1_2;
    u32 temp_v0;

    temp_s2 = arg0->lateralVelF4;
    temp_s0 = arg0->trackNodeF0 - 2;
    var_v1 = temp_s0;
    if (temp_s0 < 0) {
        var_v1 = TrackNodeCount + temp_s0;
    }
    temp_hi = var_v1 % TrackNodeCount;
    temp_s0 = temp_hi;
    InterpTrackPosition(temp_hi, sp, arg0->trackFraction170);
    temp_s0_2 = 0x800 - GetSmoothedTrackHeading(temp_hi, arg0->trackFraction170);
    var_v1_2 = rcos(temp_s0_2) * temp_s2;
    if (var_v1_2 < 0) {
        var_v1_2 += 0xFFF;
    }
    sp[0] += var_v1_2 >> 0xC;
    temp_a1 = sp[2] - ((rsin(temp_s0_2) * temp_s2) / 4096);
    sp[2] = temp_a1;
    temp_v0 = AngleDiffSigned(arg0->heading1D4, 0x400 - Atan2Table(sp[0] - arg0->position20.words[0], temp_a1 - arg0->position20.words[2]));
    arg0->heading1D4 = (s16) ((u16) arg0->heading1D4 + ((s32) ((temp_v0 >> 0x1F) + temp_v0) >> 1));
}

int GetTrackTargetHeading(RaceVehicle *vehicle)
{
  int position[4];
  u32 target;
  int direction;
  int trackValue;
#ifdef __mips__
  int halfTurn;
#else
  int halfTurn;
#endif
  direction = vehicle->half1EA;
  trackValue = vehicle->trackNodeF0;
  target = direction == 0 ? trackValue - 2U : trackValue + 2U;
  target = (s32)(target + TrackNodeCount) % TrackNodeCount;
  InterpTrackPosition(target, position, vehicle->trackFraction170);
  trackValue = GetSmoothedTrackHeading(target, vehicle->trackFraction170);
  halfTurn = 0x800;
  target = (u32)halfTurn - trackValue;
  {
    s32 delta = (u32)rcos(target) * vehicle->lateralVelF4;
    position[0] = (u32)position[0] + (delta >> 12);
  }
  {
    s32 delta = (u32)rsin(target) * vehicle->lateralVelF4;
    position[2] = (u32)position[2] - (delta >> 12);
  }
  return 0x400U - Atan2Table(
      (u32)position[0] - vehicle->position20.vector.x,
      (u32)position[2] - vehicle->position20.vector.z);
}

void StepCarStateNormal(RaceVehicle *c) {
    RaceVehicleControl *d;
    VehicleHandlingPrefix *t;
    int s2;
    int s4;
    int s1;
    int a0;
    int a2;
    int a3;
    int v0;
    int v1;
    short x;

    /* Field rate (rr/field_rate.h): the yaw and heading approaches keep the
     * square root of a tick's share; the pedal-edge entries, the lift
     * counter and the slide cooldown act by the original tick. */
    CarFieldState *fr = CarFieldStateOf(c);
    const int tick = RaceSlowTick(), tickEnd = RaceTickEnd();
    d = &c->control;
    t = d->handling1C;
    if (tick)
        fr->hist[CAR_FIELD_HIST_ROT_TICK] = c->rot54;
    s2 = AngleDiffSigned(c->rot54, d->dynamics94.player.steeringHeading08);
    if (RaceFieldRateLive && tick && (fr->rem[CAR_FIELD_REM_YAW_FLAGS] & 1)) {
        /* the yaw the tick ends on: the original's step from the yaw now
         * to the command the tick ends on; the second field lands on it */
        s32 g = AngleDiffSigned(c->rot54, fr->rem[CAR_FIELD_REM_STH_END]);
        fr->rem[CAR_FIELD_REM_ROT_END] =
            c->rot54 + (g < d->holdFlag48 && -d->holdFlag48 < g ? g >> 1 : g / d->holdFlag48);
        fr->rem[CAR_FIELD_REM_YAW_FLAGS] = 2;
    }
    if (RaceFieldRateLive && !tick && (fr->rem[CAR_FIELD_REM_YAW_FLAGS] & 2)) {
        c->rot54 = fr->rem[CAR_FIELD_REM_ROT_END];
        fr->rem[CAR_FIELD_REM_YAW_FLAGS] = 0;
    } else if (s2 < d->holdFlag48 && -d->holdFlag48 < s2) {
        c->rot54 = c->rot54 + RaceFieldApproach(s2 >> 1, s2, &fr->rem[CAR_FIELD_REM_ROT]);
    } else {
        c->rot54 = c->rot54 + RaceFieldApproach(s2 / d->holdFlag48, s2,
                                                &fr->rem[CAR_FIELD_REM_ROT]);
    }
    if (!tick) {
    } else if (d->laneTarget3C > 0) {
        d->laneTarget3C = d->laneTarget3C - 1;
    } else {
        d->laneTarget3C = 0;
    }
    UpdateCarHeading(c);
    c->vel30 = (c->input * rsin(c->heading1D4)) >> 8;
    v0 = rcos(c->heading1D4);
    v1 = c->input;
    c->vel38 = (v1 * v0) >> 8;
    if (t->steeringMode0A == 0) {
        if (d->dynamics94.player.acceleratorEdge14 == 1 && tickEnd) {
            /* at field rate the gap as a tick sees it: the yaw command
             * a whole tick on, the yaw from the tick's start
             * (rr/field_rate.h) */
            if (RaceFieldRateLive)
                s2 = AngleDiffSigned(fr->hist[CAR_FIELD_HIST_ROT_TICK], d->dynamics94.player.steeringHeading08);
            a0 = __builtin_abs(s2);
            d->lateralSpeed58 = (c->input * a0 * d->engine54.word) / 64;
            d->engine54.word = 0;
            if (c->input >= 0x3C1 && d->lateralSpeed58 > d->dynamics94.player.slideThreshold00) {
                d->unk2C = 1;
                d->slideDivisor5E = 0x200;
                c->prevLateralVelEC = c->lateralVelF4;
                d->dynamics94.player.motionState10 = 2;
                v1 = 1000 - (t->baseGrip00 - 1000) * 8;
                if (v1 < 1000) {
                    v1 = 1000;
                }
                v0 = ((s2 * v1) / 1000) << 6;
                *(s32 *)&d->latch60 = v0;
                v1 = c->half1EA;
                *(s32 *)&d->shiftState64 = v0 << 1;
                d->accel50 = v1;
                v0 = c->half1EA;
                v1 = c->lateralOffset172;
                if (v0 != 0) {
                    v0 = s2 * -v1;
                } else {
                    v0 = s2 * v1;
                }
                if (v0 < 0) {
                    v0 += 0xF;
                }
                d->attenuation4A = (unsigned int)v0 >> 4;
            }
        } else if (d->dynamics94.player.throttle18 < 0x80) {
            a2 = d->dynamics94.player.brakeEdge16;
            if (a2 == 1 && tickEnd) {
                if (RaceFieldRateLive)
                    s2 = AngleDiffSigned(fr->hist[CAR_FIELD_HIST_ROT_TICK], d->dynamics94.player.steeringHeading08);
                a0 = __builtin_abs(s2);
                d->lateralSpeed58 = (c->input * a0) / 5;
                d->engine54.word = 0;
                if (c->input >= 0x3C1 && d->lateralSpeed58 > d->dynamics94.player.slideThreshold00) {
                    d->unk2C = a2;
                    d->slideDivisor5E = 0x200;
                    c->prevLateralVelEC = c->lateralVelF4;
                    d->dynamics94.player.motionState10 = 2;
                    v1 = 1000 - (d->handling1C->baseGrip00 - 1000) * 8;
                    if (v1 < 1000) {
                        v1 = 1000;
                    }
                    v0 = ((s2 * v1) / 1000) << 6;
                    *(s32 *)&d->latch60 = v0;
                    v1 = c->half1EA;
                    *(s32 *)&d->shiftState64 = v0 << 1;
                    d->accel50 = v1;
                    v0 = c->half1EA;
                    v1 = c->lateralOffset172;
                    if (v0 != 0) {
                        v0 = s2 * -v1;
                    } else {
                        v0 = s2 * v1;
                    }
                    if (v0 < 0) {
                        v0 += 0xF;
                    }
                    d->attenuation4A = (unsigned int)v0 >> 4;
                }
            } else {
                if (tickEnd && d->engine54.word < 0x3C && d->laneTarget3C == 0) {
                    d->engine54.word = d->engine54.word + 1;
                }
                d->lateralSpeed58 = 0;
            }
        }
    } else {
        if (c->steeringLock1B8 == 0) {
            v0 = *(unsigned short *)&c->steerRoll1DE;
            a0 = v0 << 16;
            v1 = a0 >> 16;
            if (v1 >= 0) {
                a0 = a0 >> 19;
                v0 = __builtin_abs(c->steerPitch1DC) >> 2;
            } else {
                v1 = -v1;
                a0 = v1 >> 1;
                v0 = __builtin_abs(c->steerPitch1DC) + 2;
            }
            a0 = a0 * v0;
            if (a0 < 0x80) {
                a2 = a0;
            } else {
                a2 = 0x7F;
            }
            if (c == PlayerCars || c == PlayerCars + 1) {
                a3 = d->playerFlags24;
                if (CarBoundsTable[a3]->wheelRotationAxis == 1) {
                    a2 = 0;
                }
                SetPlayerVoice(1, 0x1418, a2, a3 & 1);
            }
        } else {
            if (c == PlayerCars || c == PlayerCars + 1) {
                SetPlayerVoice(0, 0, 0, d->playerFlags24 & 1);
            }
        }
        s4 = GetTrackTargetHeading(c);
        if (c->steerRoll1DE > 0) {
            s1 = 0x190 - c->steerRoll1DE;
        } else {
            s1 = 0x21C;
        }
        s4 = (AngleDiffSigned(c->heading1D4, s4) * t->unk1A) / s1;
        v0 = c->lateralOffset172;
        v1 = t->steeringMode0A;
        if (v0 < 0) {
            v0 = -v0;
        }
        s1 = (v0 * v1) >> 7;
        if (s1 > 0x400) {
            s1 = 0x400;
        }
        s2 = AngleDiffAbs(c->heading1D4, c->rot54);
        if (s2 > 0x400) {
            s2 = 0x400;
        }
        if (c->steerRoll1DE > 0) {
            d->engine54.word = d->engine54.word + RaceFieldHalf(s2 >> 3, &fr->rem[CAR_FIELD_REM_DRIFT_LIFT]);
        } else {
            d->engine54.word = RaceFieldDecay((d->engine54.word * 2) / 3, d->engine54.word,
                                              &fr->rem[CAR_FIELD_REM_DRIFT_LIFT]);
        }
        c->heading1D4 = c->heading1D4 + RaceFieldHalf(((s4 * s2) / 1024) * s1 >> 10, &fr->rem[CAR_FIELD_REM_DRIFT_HEADING]);
    }
    c->rot54 = c->rot54 & 0xFFF;
}

void StepCarStateSlide(RaceVehicle *c0) {
    RaceVehicle *c;    RaceVehicleControl *d;
    short *s6;
    int v;
    int s3;
    int s4;
    int s5;
    int a1;
    int a2;
    int a3;
    int x;
    int s0i;
    int w;
    int z;

    /* Field rate (rr/field_rate.h): as StepCarStateNormal; the slide's
     * energy, yaw latch and divisor integrate half a tick a field. */
    CarFieldState *fr = CarFieldStateOf(c0);
    const int tickEnd = RaceTickEnd();
    c = c0;
    d = &c->control;
    s6 = (short *)&d->record18->gears04[d->previewSample84.halves.high];
    v = AngleDiffSigned(c->rot54, d->dynamics94.player.steeringHeading08);
    if (v > 0x400) {
        d->dynamics94.player.steeringHeading08 = c->rot54 + 0x400;
    } else if (v < -0x400) {
        d->dynamics94.player.steeringHeading08 = c->rot54 - 0x400;
    }
    if (c->steeringLock1B8 == 0) {
        if ((v + 0x13) < 0x27U) {
            c->rot54 = c->rot54 + RaceFieldClose(v >> 1, v, &fr->rem[CAR_FIELD_REM_ROT]);
        } else {
            c->rot54 = c->rot54 + RaceFieldClose(v / 20, v, &fr->rem[CAR_FIELD_REM_ROT]);
        }
    }
    s3 = AngleDiffSigned(c->heading1D4, c->rot54);
    if (s3 > 0x600) {
        s3 = 0x600;
        s4 = 1;
    } else if (s3 < -0x600) {
        s3 = -0x600;
        s4 = 1;
    } else {
        s4 = 0;
    }
    s4 |= c->steeringLock1B8 > 0;
    if (c->steeringLock1B8 == 0) {
        s0i = AngleDiffAbs(c->rot54, c->heading1D4);
        s5 = s0i;
        if (s0i < 0x201) {
            a2 = s0i / 8 + 0x40;
        } else {
            a2 = 0x7F;
        }
        if (s0i < 0x201) {
            a1 = s0i * 3 + 0x1800;
        } else {
            a1 = 0x1E00;
        }
        if (c == PlayerCars || c == PlayerCars + 1) {
            a3 = d->playerFlags24;
            if (CarBoundsTable[a3]->wheelRotationAxis == 1) {
                a2 = 0;
            }
            SetPlayerVoice(1, a1, a2, a3 & 1);
        }
    } else {
        if (c == PlayerCars || c == PlayerCars + 1) {
            SetPlayerVoice(0, 0, 0, d->playerFlags24 & 1);
        }
        s5 = 0x200;
    }
    x = d->handling1C->unk10;
    if (s4 == 0) {
        if (s3 < x && -x < s3) {
            c->heading1D4 = c->heading1D4 + RaceFieldClose(s3 >> 1, s3, &fr->rem[CAR_FIELD_REM_HEADING]);
        } else {
            c->heading1D4 = c->heading1D4 + RaceFieldClose(s3 / x, s3, &fr->rem[CAR_FIELD_REM_HEADING]);
        }
    }
    s3 = GetTrackTargetHeading(c);
    s0i = (short)(d->attenuation4A >> 8);
    if (s0i < 0) {
        s0i = s0i >> 2;
    }
    w = c->lateralOffset172;
    if (w < 0) {
        w = -w;
    }
    s0i = w - s0i;
    if (s0i > 0x400) {
        s0i = 0x400;
    } else if (s0i < 0) {
        s0i = 0;
    }
    x = AngleDiffSigned(c->heading1D4, s3);
    x = ((x * d->handling1C->unk02) >> 8) * s0i;
    v = x / 1024;
    if (s4 == 0) {
        c->heading1D4 = c->heading1D4 + RaceFieldHalf(v, &fr->rem[CAR_FIELD_REM_SLIDE_HEADING_TRACK]);
    }
    c->input = (u16)c->input - RaceFieldHalf(s0i / 256, &fr->rem[CAR_FIELD_REM_SLIDE_SPEED]);
    c->vel30 = (c->input * rsin(c->heading1D4)) >> 8;
    c->vel38 = (c->input * rcos(c->heading1D4)) >> 8;
    c->rot54 = c->rot54 + RaceFieldHalf((*(s32 *)&d->latch60) / (d->slideDivisor5E + s5 / 2),
                                        &fr->rem[CAR_FIELD_REM_SLIDE_YAW]);
    if (c->input < 0x348) {
        s0i = (0x348 - c->input) * 8;
    } else {
        s0i = 0;
    }
    z = 0x180;
    d->lateralSpeed58 = d->lateralSpeed58 - RaceFieldHalf((s5 >> 2) + (s0i + z), &fr->rem[CAR_FIELD_REM_SLIDE_ENERGY]);
    *(s32 *)&d->latch60 = *(s32 *)&d->latch60 +
                          RaceFieldHalf(d->steerInput0C / (d->handling1C->slideSteerDivisor1E + 8),
                                        &fr->rem[CAR_FIELD_REM_SLIDE_LATCH]);
    if (d->steerInput0C != 0) {
        if (d->handling1C->unk04 < d->slideDivisor5E) {
            d->slideDivisor5E = d->slideDivisor5E - RaceFieldHalf(8, &fr->rem[CAR_FIELD_REM_SLIDE_DIVISOR]);
        }
    }
    if (*(s32 *)&d->latch60 > 0x9600) {
        *(s32 *)&d->latch60 = 0x9600;
    } else if (*(s32 *)&d->latch60 < -0x9600) {
        *(s32 *)&d->latch60 = -0x9600;
    }
    if (d->dynamics94.player.throttle18 < 0x80) {
        d->lateralSpeed58 = d->lateralSpeed58 - RaceFieldHalf(0x80, &fr->rem[CAR_FIELD_REM_SLIDE_ENERGY]);
        *(s32 *)&d->latch60 = RaceFieldDecay(*(s32 *)&d->latch60 * 0x1F / 32, *(s32 *)&d->latch60,
                                             &fr->rem[CAR_FIELD_REM_SLIDE_LATCH_DECAY]);
    } else if (d->dynamics94.player.acceleratorEdge14 == 1 && tickEnd) {
        *(s32 *)&d->latch60 = *(s32 *)&d->latch60 + (*(s32 *)&d->latch60 >> 9);
    }
    if (d->lateralSpeed58 < 0) {
        if (c == PlayerCars || c == PlayerCars + 1) {
            SetPlayerVoice(0, 0, 0, d->playerFlags24 & 1);
        }
        d->dynamics94.player.steeringHeading08 = c->rot54;
        d->dynamics94.player.motionState10 = 0;
        d->laneTarget3C = 0x1E;
        *(short *)&d->laneCooldown40 = (d->laneMode88 - c->input * 0x2710 / (s6[1] * 0x490 / 160)) / 30;
    }
    c->rot54 = c->rot54 & 0xFFF;
    c->heading1D4 = c->heading1D4 & 0xFFF;
}

void StepCarStateSpinOut(RaceVehicle *a0) {
    RaceVehicleControl *q = &a0->control;
    int t, a1, a2, a3, v;
    int s0, s2;
    /* Field rate (rr/field_rate.h): the approaches and the speed loss per
     * field, the spin timer half a tick; the random kicks and the pad
     * pulse once per original tick. */
    CarFieldState *fr = CarFieldStateOf(a0);
    const int tick = RaceSlowTick();
    t = AngleDiffSigned(a0->rot54, q->dynamics94.player.steeringHeading08);
    v = RaceFieldClose(t / 4, t, &fr->rem[CAR_FIELD_REM_ROT]);
    a1 = a0->rot54 + v;
    a0->rot54 = a1;
    t = AngleDiffSigned(a0->heading1D4, a1);
    a0->heading1D4 = a0->heading1D4 + RaceFieldClose(t / 10, t, &fr->rem[CAR_FIELD_REM_HEADING]);
    a0->input = RaceFieldDecay(a0->input / 10, a0->input, &fr->rem[CAR_FIELD_REM_SPIN_SPEED]);
    a0->vel30 = (a0->input * rsin(a0->heading1D4)) >> 8;
    a0->vel38 = (a0->input * rcos(a0->heading1D4)) >> 8;
    a2 = q->relDist74;
    if (a2 >= 0x3D) a2 = 0x3C;
    else if (a2 < 0) a2 = 0;
    a2 = (a2 << 1) * q->dynamics94.player.throttle18;
    if (a2 < 0) a2 += 0xFF;
    a2 = a2 >> 8;
    if (q->relDist74 > 0) {
        if (a0 == PlayerCars || a0 == PlayerCars + 1) {
            a3 = q->playerFlags24;
            if (CarBoundsTable[a3]->wheelRotationAxis == 1) a2 = 0;
            SetPlayerVoice(1, 0x1A80, a2, a3 & 1);
            if (tick)
                PadStartPulse(q->playerFlags24 & 1, 2, 1);
        }
        q->relDist74 = q->relDist74 -
                       RaceFieldHalf(0x30 + q->dynamics94.player.brake1A * 2 +
                                         (q->dynamics94.player.throttle18 < 0x80 ? 0x30 : 0),
                                     &fr->rem[CAR_FIELD_REM_SPIN_TIMER]);
        if (!tick) {
        } else if (GameModeType != 4) {
            s2 = q->relDist74 >> 10;
            s0 = q->dynamics94.player.steeringHeading08;
            if ((PsyqRand() & 3) >= 2) q->dynamics94.player.steeringHeading08 = s0 + s2;
            else q->dynamics94.player.steeringHeading08 = s0 - s2;
            s0 = a0->wheelOffsetE4[0];
            if (PsyqRand() & 1) s0 += 0x10;
            a0->wheelOffsetE4[0] = s0;
            s0 = a0->wheelOffsetE4[1];
            if (PsyqRand() & 1) s0 += 0x10;
            a0->wheelOffsetE4[1] = s0;
        } else {
            int c;
            int z, w, u;
            int x;
            int y;
            y = q->relDist74;
            x = y >> 10;
            c = (ModeFrame & 3) < 2U;
            y = q->dynamics94.player.steeringHeading08;
            if (c) y = y - x; else y = y + x;
            z = ModeFrame & 1;
            q->dynamics94.player.steeringHeading08 = y;
            u = a0->wheelOffsetE4[0];
            if (z) u += 0x10;
            w = a0->wheelOffsetE4[1];
            a0->wheelOffsetE4[0] = u;
            if (z) w += 0x10;
            a0->wheelOffsetE4[1] = w;
        }
        if (q->previewSample84.halves.high >= 2) {
            a0->input = 0;
            q->dynamics94.player.driveProduct0C = 0;
            q->relDist74 = 0;
        }
    } else {
        q->dynamics94.player.motionState10 = 0;
        if (a0 == PlayerCars || a0 == PlayerCars + 1) {
            SetPlayerVoice(0, 0, 0, q->playerFlags24 & 1);
            PadStartPulse(q->playerFlags24 & 1, 0, 1);
        }
    }
}

int LerpAngle12(int a0, int a1, int a2) {
    int d;
    a0 &= 0xFFF;
    a1 &= 0xFFF;
    d = 0x400 - a2;
    if (a1 < a0) {
        if (a0 - a1 > 0x800) a1 += 0x1000;
    } else {
        if (a1 - a0 > 0x800) a0 += 0x1000;
    }
    return ((a0 * d + a1 * a2) / 1024) & 0xFFF;
}
