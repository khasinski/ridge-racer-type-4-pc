#include "game/system/pad.h"
#include "psyq/libgte/gtemat3.h"
#include "game/data/bankrec.h"
#include "rr/race_types.h"
#include "rr/replay_types.h"
#include "rr/tables.h"
#include "scratchpad.h"
#include "rr_asm.h"
#include "game/car/carparam.h"
#include "game/course/trkintp.h"
#include "rr/overlay661_init.h"
#include "rr/renderer_setup.h"
#include "rr/steering.h"
#include "game/math/rotmat.h"
#include "rr/overlay665_render.h"
#include "rr/trig.h"
#include "game/car/carphys2.h"

typedef struct {
  s32 pad00;
  MatrixWords32 m04;
  MatrixWords32 m24;
  MatrixWords32 m44;
  struct { s16 x, y, z, pad; } v64;
  struct { s16 x, y, z, pad; } v6C;
  struct { s16 x, y, z, pad; } v74;
} CarMotionScratch;

#include "game/system/pad3.h"
extern RaceVehicle PlayerCars[];
extern int RaceState;

void UpdatePlayerSteering(RaceVehicle *arg0, int arg1);

void UpdatePlayerSteering(RaceVehicle *arg0, int arg1) {
  RaceVehicleControl *s1;
  CarInput4 *a2;
  int v6;
  int v7;
  short h;
  int sw;
  int p0;
  int p1;
  int d0;
  int d1;
  int d2;
  int r0;
  int r1;
  int r2;
  int a1;
  int v;
  int s2;
  int s0;
  int a0;
  int c;
  int d;
  int t;
  int t2;
  int v0_3;
  s2 = 0x1800;
  s1 = &arg0->control;
  if (RaceState < 2)
  {
    s1->steerInput0C = 0;
    arg0->lateralMotion178 = 0;
  }
  else
    if (RaceState < 4)
  {
    a2 = &CarInputs[arg1];
    a1 = a2->buttons;
    sw = a1 & 0xE0;
    if (sw == 0x20)
    {
      s2 = (s1->handling1C->steeringGain08 * 0x1400) / 1000;
      if (MirrorRenderFlag != 0)
      {
        p1 = a1 & 1;
        p0 = a1 & 2;
      }
      else
      {
        p0 = a1 & 1;
        p1 = a1 & 2;
      }
      if (p0 != 0)
      {
        t2 = s1->steerInput0C;
        s1->boostTimer44 = 1;
        if (t2 <= 0)
        {
          if ((-s2) < t2)
          {
            s1->steerInput0C = t2 - 0x500;
          }
        }
        else
        {
          s1->steerInput0C = 0;
        }
      }
      else
        if (p1 != 0)
      {
        t2 = s1->steerInput0C;
        s1->boostTimer44 = -1;
        if (t2 < 0)
        {
          s1->steerInput0C = 0;
        }
        else
          if (t2 < s2)
        {
          s1->steerInput0C = t2 + 0x500;
        }
      }
      else
      {
        s1->boostTimer44 = 0;
        s1->steerInput0C = s1->steerInput0C / 2;
      }
      arg0->lateralMotion178 = -s1->steerInput0C;
    }
    else
      if (sw == 0x40)
    {
      s2 = (s1->handling1C->steeringGain08 * 0x1800) / 1000;
      a0 = (s2 * a2->steer) / 127;
      d0 = MirrorRenderFlag;
      s1->steerInput0C = a0;
      if (d0 != 0)
      {
        s1->steerInput0C = -a0;
      }
      r0 = s1->steerInput0C;
      if (r0 < 0)
      {
        s1->boostTimer44 = 1;
      }
      else
        if (r0 > 0)
      {
        s1->boostTimer44 = -1;
      }
      else
      {
        s1->boostTimer44 = 0;
      }
      arg0->lateralMotion178 = -s1->steerInput0C;
    }
    else
      if (sw == 0x60)
    {
      s2 = (s1->handling1C->steeringGain08 * 0x1800) / 1000;
      a0 = (s2 * a2->steer) / 127;
      d1 = MirrorRenderFlag;
      s1->steerInput0C = a0;
      if (d1 != 0)
      {
        s1->steerInput0C = -a0;
      }
      r1 = s1->steerInput0C;
      if (r1 < 0)
      {
        s1->boostTimer44 = 1;
      }
      else
        if (r1 > 0)
      {
        s1->boostTimer44 = -1;
      }
      else
      {
        s1->boostTimer44 = 0;
      }
      arg0->lateralMotion178 = -s1->steerInput0C;
    }
    else
      if (sw == 0x80)
    {
      s2 = (s1->handling1C->steeringGain08 * 0x1400) / 1000;
      a0 = (s2 * a2->steer) / 127;
      d2 = MirrorRenderFlag;
      s1->steerInput0C = a0;
      if (d2 != 0)
      {
        s1->steerInput0C = -a0;
      }
      r2 = s1->steerInput0C;
      if (r2 < 0)
      {
        s1->boostTimer44 = 1;
      }
      else
        if (r2 > 0)
      {
        s1->boostTimer44 = -1;
      }
      else
      {
        s1->boostTimer44 = 0;
      }
      arg0->lateralMotion178 = -s1->steerInput0C;
    }
    else
    {
      s1->steerInput0C = 0;
    }
  }
  else
  {
    int s0_else;
    int t5;
    t5 = arg0->half1EA << 0xB;
    t5 += 0x800;
    s0_else = AngleDiffSigned(arg0->rot54, t5 - (short)arg0->trackAngle1EC) << 8;
    a0 = 0x1000 - rcos((arg0->lateralVelF4 << 8) / 96);
    if (arg0->input < 0x320)
    {
      a0 = a0 * 8;
    }
    else
    {
      a0 = a0 * 6;
    }
    if (arg0->input >= 0x51)
    {
      if (arg0->half1EA != 0)
      {
        if (arg0->lateralVelF4 < 0)
        {
          a0 = -a0;
        }
      }
      else
        if (arg0->lateralVelF4 > 0)
      {
        a0 = -a0;
      }
      a0 = a0 + s0_else;
    }
    else
    {
      s1->steerInput0C = 0;
      a0 = 0;
    }
    if (a0 < (-0x1400))
    {
      a0 = -0x1400;
    }
    if (!(a0 < 0x1401))
    {
      a0 = 0x1400;
    }
    s1->steerInput0C = a0;
    arg0->lateralMotion178 = -a0;
  }
  if (arg0->steeringLock1B8 == 0)
  {
    t = s1->reserved28;
    if ((t > 0) && (t < s1->steeringThreshold2A))
    {
      a0 = 0xC8;
    }
    else
    {
      if ((arg0->shakeTimer19E == 1) && (s1->dynamics94.player.motionState10 == 0))
      {
        int h_sh;
        h_sh = (s1->reserved36 / 3) << 16;
        a0 = (h_sh >> 16) / 30;
      }
      else
      {
        a0 = 0;
      }
    }
    a1 = arg0->input;
    if ((a1 < 0x100) && (s1->dynamics94.player.motionState10 == 0))
    {
      d = (((s1->steerInput0C * (s1->reserved36 + a0)) / 256) * a1) / 0x10000;
      s1->dynamics94.player.steeringHeading08 = s1->dynamics94.player.steeringHeading08 + d;
    }
    else
      if ((a1 < 0x200) && (s1->dynamics94.player.motionState10 == 4))
    {
      d = (((s1->steerInput0C * s1->reserved36) / 256) * a1) / 0x20000;
      s1->dynamics94.player.steeringHeading08 = s1->dynamics94.player.steeringHeading08 + d;
    }
    else
    {
      d = (s1->steerInput0C * (s1->reserved36 + a0)) / 0x10000;
      s1->dynamics94.player.steeringHeading08 = s1->dynamics94.player.steeringHeading08 + d;
    }
  }
  s1->dynamics94.player.steeringHeading08 = s1->dynamics94.player.steeringHeading08 & 0xFFF;
  if ((arg0 == &PlayerCars[0]) || (arg0 == &PlayerCars[1]))
  {
    ControllerState5C *p4;
    p4 = PlayerControllers;
    p4[arg1 & 1].rumbleActive50 = 1;
    s0 = arg0->input;
    t = CarRecordPtrs[arg1]->topSpeed1F0 * 8;
    if (t < s0)
    {
      s0 = t;
    }
    a0 = s1->steerInput0C;
    if ((a0 >> 3) != 0)
    {
      s2 = (((s0 * 0x14) / t) * a0) / s2;
    }
    else
    {
      s2 = 0;
    }
    c = s1->dynamics94.player.motionState10;
    if (c == 0)
      goto L_not2;
    if (c != 2)
    {
    L_not2:
      if (s1->handling1C->steeringMode0A > 0)
      {
        v6 = arg0->steerRoll1DE;
        if (v6 > 0)
        {
          s2 += ((int) (((-v6) >> 3) + (((unsigned) (-v6)) >> 31))) >> 1;
        }
      }
    }
    else
    {
      s0 = AngleDiffSigned(arg0->heading1D4, arg0->rot54);
      if (s0 >= 0x201)
      {
        s0 = 0x200;
      }
      else
        if (s0 < (-0x200))
      {
        s0 = -0x200;
      }
      s2 = (s2 * rcos(s0 * 2)) / 0x1000;
    }
    s0 = 2;
    if (s2 <= 0)
    {
      s0 = 1;
      s2 = -s2;
    }
    if (arg0->steeringLock1B8 != 0)
    {
      s2 = 0;
    }
    if (!(s2 < 0xC))
    {
      s2 -= PsyqRand() & 3;
    }
    if (!(s2 < 0x10))
    {
      s2 = 0xF;
    }
    if (arg0->stateTimer1A5 >= 0x10U)
    {
      if (arg0->state1A4 != 1)
      {
        v = __builtin_abs(arg0->steerAngle1A2);
        if (v >= 0x96)
        {
          s2 = 0xF;
        }
        else
        {
          s2 = v / 10;
        }
        s0 = 1;
        if ((arg0->stateTimer1A5 & 7) >= 4U)
        {
          s0 = 2;
        }
      }
    }
    else
      if (arg0->contactTimer1A7 != 0)
    {
      s2 = 0xD;
      s0 = 1;
      if (arg0->shakeRollOffset1B6 > 0)
      {
        s0 = 2;
      }
    }
    if (arg0->input == 0)
    {
      s0 = 0;
      s2 = 0xF;
    }
    if (RaceState < 4)
    {
      PadSetRumbleParams(arg1 & 1, s0, s2);
    }
  }
}

void UpdateCarEngine(RaceVehicle *a0) {
  RaceVehicleControl *q;
  VehicleHandlingPrefix *r;
  CarGearRecord3C *s;
  int t0;
  int a2;
  int a1;
  int v0;
  int v1;
  int s1;
  int s3;
  int s4;
  short v;
  short *pf;
  int m1;
  int m2;
  int m3;
  int m4;
  int m5;
  int m6;
  int m7;
  int m8;
  int m9;
  int n1;
  int n2;
  int n3;
  int n4;
  int n5;
  int d1;
  int a;
  int b;
  int off;
  int x;
  int lo;
  int y;
  int k1;
  int k2;
  int k3;
  int k4;
  int k5;
  int k6;
  int fl;
  short t;
  int ten;
  q = &a0->control;
  r = q->handling1C;
  off = (q->previewSample84.halves.high * 0x3C) + 4;
  s = (CarGearRecord3C *) ((u8 *) q->record18 + off);
  if (RaceState < 4)
  {
    q->dynamics94.player.throttle18 = CarInputs[q->playerFlags24].throttle;
    q->dynamics94.player.brake1A = CarInputs[q->playerFlags24].brake;
    if (q->dynamics94.player.throttle18 >= 0xFF)
    {
      q->dynamics94.player.throttle18 = 0x100;
    }
    if (q->dynamics94.player.brake1A >= 0xFF)
    {
      q->dynamics94.player.brake1A = 0x100;
    }
  }
  else
  {
    q->dynamics94.player.throttle18 = (0x400 - a0->lateralOffset172) / 4;
    q->dynamics94.player.brake1A = a0->lateralOffset172 / 8;
  }
  if (q->dynamics94.player.acceleratorEdge14 == 0)
  {
    if (q->dynamics94.player.throttle18 >= 0x85)
    {
      q->dynamics94.player.acceleratorEdge14 = 1;
    }
  }
  else
    if (q->dynamics94.player.acceleratorEdge14 == 1)
  {
    q->dynamics94.player.acceleratorEdge14 = 2;
  }
  else
    if (q->dynamics94.player.throttle18 < 0x7C)
  {
    q->dynamics94.player.acceleratorEdge14 = 0;
  }
  if (q->dynamics94.player.brakeEdge16 == 0)
  {
    if (q->dynamics94.player.brake1A >= 0x85)
    {
      q->dynamics94.player.brakeEdge16 = 1;
    }
  }
  else
    if (q->dynamics94.player.brakeEdge16 == 1)
  {
    q->dynamics94.player.brakeEdge16 = 2;
  }
  else
    if (q->dynamics94.player.brake1A < 0x7C)
  {
    q->dynamics94.player.brakeEdge16 = 0;
  }
  t0 = r->baseGrip00;
  if (r->steeringMode0A > 0)
  {
    if (a0->input >= 0x321)
    {
      a2 = a0->steerRoll1DE + 0x12C;
    }
    else
    {
      a2 = 0x12C;
    }
    if (a0->steerRoll1DE > 0)
    {
      t0 += (a0->steerRoll1DE * r->rollFactor18) >> 2;
    }
  }
  else
  {
    m1 = q->dynamics94.player.throttle18 * 0x64;
    m2 = m1 >> 8;
    if (m1 < 0)
    {
      m1 += 0xFF;
      m2 = m1 >> 8;
    }
    a2 = 0x17C - m2;
    if (q->laneTarget3C > 0)
    {
      a2 += q->laneTarget3C * 3;
    }
    a2 += ((q->dynamics94.player.brake1A * (0x64 + (r->steeringMode0A / 2))) * r->brakeBalance0C) / 256000;
  }
  if (q->dynamics94.player.motionState10 == 1)
  {
    q->reserved36 = (q->reserved36 + a2) / 2;
  }
  else
  {
    n2 = q->boostTimer44 * a0->rot58;
    if (n2 > 0)
    {
      if (r->maxSlopePitch12 < n2)
      {
        n2 = r->maxSlopePitch12;
      }
      a2 = a2 + ((n2 * r->slopeFactor14) / 1000);
    }
    m4 = a2 * t0;
    q->reserved36 = q->reserved36 + ((((m4 / 1000) - q->reserved36) * 0x1C) / 32);
  }
  ten = 10;
  s3 = s->scaledRate00;
  if (q->laneTimer38 == ten)
  {
    a2 = ((q->laneMode88 * s3) - q->dynamics94.player.driveProduct0C) / s3;
    *(s32 *)&q->steering8C = a2 / 10;
    *(s16 *)&q->laneCooldown40 = (q->laneMode88 - ((a0->input * 10000) / ((s->speed02 * 0x490) / 160))) / (q->laneTarget3C + 1);
  }
  if (q->laneTimer38 > 0)
  {
    s1 = *(s32 *)&q->steering8C;
    q->laneTimer38 = q->laneTimer38 - 1;
    if (q->reserved32 == 1)
    {
      s3 = ((s->scaledRate00 * (ten - q->laneTimer38)) + (s[-1].scaledRate00 * q->laneTimer38)) / 10;
    }
  }
  else
  {
    m5 = (q->laneMode88 * s->scaledRate00) - q->dynamics94.player.driveProduct0C;
    q->reserved32 = 0;
    s1 = m5 / s3;
  }
  if (q->laneTarget3C > 0)
  {
    s1 += *(s16 *)&q->laneCooldown40;
  }
  k1 = q->laneMode88;
  if (k1 >= 10000)
  {
    a1 = 0x26AC - k1;
  }
  else
  {
    n2 = k1 / 500;
    m7 = k1 - (n2 * 500);
    if ((q->previewSample84.halves.high == 1) && (n2 < 8))
    {
      k1 = s->torqueCurve08[n2] * 3;
    }
    else
    {
      k1 = s->torqueCurve08[n2];
    }
    a1 = ((k1 * (500 - m7)) + (s->torqueCurve08[n2 + 1] * m7)) / 500;
  }
  s4 = ((a1 * q->dynamics94.player.throttle18) >> 8) * ((0x100 - q->pathDeltaX30) >> 8);
  if (a0->steeringLock1B8 == 0)
  {
    k2 = q->laneMode88;
    n2 = k2 / 256;
    n2 += (((k2 * q->dynamics94.player.brake1A) / 256) * q->handling1C->brakeBalance0C) / 32000;
    if (((q->dynamics94.player.motionState10 - 1)) < 2U)
    {
      n2 = 0;
    }
    s1 += n2;
  }
  if (a1 > 0)
  {
    if (q->dynamics94.player.throttle18 < 0x7F)
    {
      s1 += a1 / 2;
    }
  }
  else
  {
    s1 -= a1 / 2;
  }
  k3 = AngleDiffAbs(a0->rot54, a0->heading1D4);
  if (k3 > 0x400)
  {
    k3 = 0x800 - k3;
  }
  k6 = k3;
  if (k3 < 0)
  {
    k6 = k3 + 0xFF;
  }
  s1 += k6 >> 8;
  if (q->dynamics94.player.motionState10 == 0)
  {
    k4 = q->steerInput0C;
    if ((CarInputs[q->playerFlags24].buttons & 0xE0) != 0x20)
    {
      k4 = k4 >> 1;
    }
    if (q->steerInput0C >= 0)
    {
      s1 += k4 / r->steerDragDivisor06;
    }
    else
    {
      s1 -= k4 / r->steerDragDivisor06;
    }
  }
  k1 = a0->rot50;
  if (k1 < (-200))
  {
    k1 = -200;
  }
  else
    if (k1 > 100)
  {
    k1 = 100;
  }
  s4 = (s4 * (k1 + 1000)) / 1000;
  n3 = q->laneMode88 + (s4 - s1);
  q->laneMode88 = n3;
  if (n3 < 0)
  {
    q->laneMode88 = 0;
  }
  else
    if (n3 >= 0x3A99)
  {
    q->laneMode88 = 0x3A98;
  }
  lo = q->laneMode88 * s3;
  q->dynamics94.player.driveProduct0C = lo;
  if (a0->steeringLock1B8 != 0)
  {
    a0->physicsFlags1E0 = 0;
    t = a0->steeringLock1B8 | 0x100;
    a0->steeringLock1B8 = t;
    a0->input = (a0->input * 999) / 1000;
    if (t == 0x100)
    {
      q->laneTarget3C = 30;
      *(s16 *)&q->laneCooldown40 = (q->laneMode88 - ((a0->input * 10000) / ((s->speed02 * 0x490) / 160))) / 30;
      a0->steeringLock1B8 = 0;
    }
  }
  else
    if (((q->dynamics94.player.motionState10 - 1)) < 2U)
  {
    n2 = 0x1000 - rcos(AngleDiffAbs(a0->heading1D4, a0->rot54));
    x = (n2 * 100) >> 12;
    k3 = r->slideRetention0E;
    k3 = k3 + 9000;
    a0->input = (a0->input * (k3 - x)) / 10000;
    a0->physicsFlags1E0 = 0;
  }
  else
    if (q->laneTarget3C > 0)
  {
    x = lo;
    if (x < 0)
    {
      x += 0x1FFFF;
    }
    a0->physicsFlags1E0 = ((x >> 17) * (30 - q->laneTarget3C)) / 30;
    k2 = (((a0->input * 6) / 100) * (30 - q->laneTarget3C)) / 30;
    a0->input = a0->input - k2;
  }
  else
  {
    n5 = lo;
    if (n5 < 0)
    {
      n5 += 0x1FFFF;
    }
    a0->physicsFlags1E0 = n5 >> 17;
    k2 = (a0->input * 6) / 100;
    a0->input = a0->input - k2;
  }
  if (a0->input < 8)
  {
    a0->heading1D4 = a0->rot54;
  }
  if (RaceState >= 2)
  {
    switch (q->dynamics94.player.motionState10)
    {
      case 0:
        StepCarStateNormal(a0);
        break;

      case 2:
        StepCarStateSlide(a0);
        break;

      case 4:
        StepCarStateSpinOut(a0);
        break;

    }

  }
  else
  {
    a0->input = 0;
  }
  if (q->dynamics94.player.throttle18 == 0)
  {
    if (a0->input < 0x20)
    {
      a0->input = a0->input >> 1;
    }
  }
  if ((a0 == &PlayerCars[0]) || (a0 == &PlayerCars[1]))
  {
    d1 = PlayerDisplayRpm[q->playerFlags24 & 1];
    k2 = a0->steerRoll1DE;
    if (k2 > 0)
    {
      k2 = 0x64;
    }
    else
    {
      k2 += 0x50;
    }
    if (k2 <= 0)
    {
      k2 = 1;
    }
    PlayerDisplayRpm[q->playerFlags24 & 1] = d1 + (((q->laneMode88 - d1) * 0x14) / k2);
    if (PlayerDisplayRpm[q->playerFlags24 & 1] < 0x1F5)
    {
      PlayerDisplayRpm[q->playerFlags24 & 1] += 0x1F4 - ((PsyqRand() * 0x64) / 32767);
    }
    PlayerThrottleLoad[q->playerFlags24 & 1] = (q->dynamics94.player.throttle18 >= 0x41) && (q->laneTimer38 == 0);
  }
}

void IntegrateCarMotion(RaceVehicle *p) {
    CarMotionScratch *s5;
    MatrixWords32 *s2;
    MatrixWords32 *s1;
    MatrixWords32 *s0;
    int s3;
    int v1, v0, a1, a0, m1, m2;

    s5 = (CarMotionScratch *)SCRATCH_CAR_WORK;
    if ((p->flags1E4 & 0x10) == 0) {
        m1 = p->halfSpan10E * p->input;
        p->vel30 = p->vel30 << 8;
        p->vel38 = p->vel38 << 8;
        s5->v64.z = m1 / (p->control.maxSpeedF8 / 128 + 1);
    } else {
        m2 = p->halfSpan10E * p->input;
        v0 = p->control.dynamics94.player.topGearSpeed04 + 1;
        s5->v64.z = (m2 / v0) / 2;
        p->prevInput1DA = (unsigned short)p->input;
        p->prevHeading1D6 = (unsigned short)p->heading1D4;
    }
    s2 = &s5->m24;
    s3 = 0x800;
    a1 = s3 - p->rot54;
    p->posScaled00[0] = p->posScaled00[0] + p->vel30;
    p->posScaled00[2] = p->posScaled00[2] + p->vel38;
    p->worldPosition10.words[0] = p->posScaled00[0] >> 6;
    p->worldPosition10.words[2] = p->posScaled00[2] >> 6;
    BuildRotMatrixY(s2, a1);
    s0 = &s5->m04;
    BuildRotMatrixX(s0, p->rot50);
    MulMatrix(s2, s0);
    BuildRotMatrixZ(s0, p->rot58);
    MulMatrix(s2, s0);
    s1 = &s5->m44;
    BuildRotMatrixY(s1, s3 - p->ang82);
    BuildRotMatrixX(s0, p->ang80);
    MulMatrix(s1, s0);
    BuildRotMatrixZ(s0, p->ang84);
    MulMatrix(s1, s0);
    s5->v64.x = 0;
    s5->v64.y = -(unsigned short)p->height108;
    RR_GTE_SETROT(s2);
    RR_GTE_MVMVA_ROT_V0_NOCV(&s5->v64);
    s5->v6C.x = 0;
    v0 = p->wheelOffsetE4[2];
    v1 = p->wheelOffsetE4[3];
    s5->v6C.z = 0;
    s5->v6C.y = (v0 + v1) >> 7;
    RR_GTE_STORE_IR(p->ir40);
    RR_GTE_SETROT(s1);
    RR_GTE_MVMVA_ROT_V0_NOCV(&s5->v6C);
    s5->v74.x = 0;
    v0 = p->wheelSuspensionDC[2];
    v1 = p->wheelSuspensionDC[3];
    s5->v74.z = 0;
    s5->v74.y = (v0 + v1) >> 3;
    RR_GTE_STORE_IR(p->ir70);
    RR_GTE_MVMVA_ROT_V0_NOCV(&s5->v74);
    p->worldPosition10.words[0] = p->worldPosition10.words[0] + p->ir40[0];
    p->worldPosition10.words[2] = p->worldPosition10.words[2] + p->ir40[2];
    p->position20.words[0] = p->worldPosition10.words[0] >> 2;
    p->position20.words[2] = p->worldPosition10.words[2] >> 2;
    RR_GTE_STORE_IR(p->ir60);
}

/* Initial values of the globals this module owns. */
s32 MirrorRenderFlag;
s16 PlayerDisplayRpm[2];
