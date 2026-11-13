#include "rr/release.h"
#include "types.h"
#include "rr/runtime_settings.h"
#include "game/system/pad.h"
#include "game/system/pad3.h"
#include "rr/race_setup.h"
#include "rr/bios_devices.h"
#include "psyq/libpad/padctl.h"

extern s16 PadRangeLimits[];
extern u8 PadDualShockActAlignCmd[];
extern u8 PadActAlignBytes[];

void PadSetRumbleFlags(int i, int flags);
void PadUpdateRumble(void);
void PadPollPort(int a0);

void PadSetRumbleParams(s32 channel, s32 direction, s32 level) {
    ControllerState5C *controller = &PlayerControllers[channel];
    controller->rumbleDir52 = direction;
    controller->rumbleLevel49 = level;
}

void PadRumbleStart(int i) {
    ControllerState5C *t = &PlayerControllers[i];
    t->rumbleActive50 = 1;
    PadSetRumbleFlags(i, 2);
    t->rumbleLevel49 = 0xF;
}

void PadRumbleStop(int i) {
    ControllerState5C *t = &PlayerControllers[i];
    t->rumbleActive50 = 0;
    t->rumbleLevel49 = 0;
    t->rumbleDir52 = 0;
}

void PadSetRumbleFlags(int i, int flags) {
    ControllerState5C *t = &PlayerControllers[i];
    if (flags & 1) {
        if (!(t->actuator44 & 0x40)) {
            t->rumbleFlags48 |= 0x40;
        }
    }
    if (flags & 2) {
        if (!(t->actuator44 & 0x80)) {
            t->rumbleFlags48 |= 0x80;
        }
    }
    if ((flags & 3) == 3) {
        t->rumble46 = 0;
        t->rumble47 = 0;
        t->rumbleActive50 = 0;
        t->rumbleLevel49 = 0;
    }
    if (flags == 4) {
        if (!(t->rumbleFlags48 & 4)) {
            t->rumble47 = t->position42;
            t->rumbleFlags48 |= 0x84;
        }
    }
}

void PadUpdateRumble(void) {
    s32 i;
    s32 j;
    u8 mask;
    s32 t1;
    u8 t0;
    u8 a0;
    ControllerState5C *t;

    for (i = 0; i < 2; i++) {
        t = &PlayerControllers[i];
        for (j = 0; j < 2; j++) {
            mask = 1 << (j + 6);
            if (t->rumbleFlags48 & mask) {
                if (t->actuator44 & mask) {
                    t->actuator44 &= ~mask;
                } else {
                    t->actuator44 |= mask;
                    t->rumbleFlags48 &= ~mask;
                }
            } else if (t->actuator44 & mask) {
                s32 cond = (VSyncThreshold == 0x80) && !(t->position43 & mask);
                if (!cond) {
                    t->actuator44 &= ~mask;
                    t->rumble46 += t->rumble47;
                    t->rumble47 = 0;
                    t->rumbleFlags48 &= ~4;
                }
            }
        }
        t1 = t->actuator44 & 0xC0;
        t0 = t1;
        if (t->rumbleLevel49 >= 0x10) {
            t->rumbleLevel49 = 0xF;
        }
        a0 = (t->rumbleLevel49 * Options.wheel[i].rumble02) / 2;
        if (t->rumbleActive50 != 0) {
            if (t->rumbleDir52 != 0 && __builtin_abs((s16)t->position4C) < PadRangeLimits[Options.wheel[i].range01]) {
                if (a0 != 0) {
                    t0 = t1 | (1 << (t->rumbleDir52 + 3));
                }
            } else {
                if (t->rumbleDir52 != 0) {
                    t->rumbleDir52 = 0;
                }
                if (a0 != 0) {
                    t0 = t1 | 0x30;
                }
            }
        } else {
            if (a0 != 0) {
                t0 = t1 | (1 << (t->rumbleDir52 + 3));
            }
        }
        t->actuator44 = t0 | a0;
    }
}

void PadPollPort(int a0)
{
  int w11_r;
  int s3 = a0;
  int s2 = a0 * 16;
  int s0;
  int v1;
  ControllerState5C *s1 = &PlayerControllers[a0];
  s0 = PadGetState(s2);
  if (s0 == 0)
  {
    s1->input00.fields.type = 0;
    s1->protocol10 = -1;
    return;
  }
  if (PadInfoMode(s2, 2, 0) == 0)
    s1->kind1C = 0;
  else
    s1->kind1C = 1;
  if (s0 == 1)
    s1->flag1B = 0;
  if (s1->changed1E != 0)
  {
    if (s1->flag1B == 0)
      goto B;
    s1->flag1B = 0;
  }
  if (s1->flag1B != 0)
    return;
B:
  if (s0 == 2)
  {
    if (RR_RELEASE_JP)
    {
      /* The Japanese release sets up the actuators here; the PAL one
       * leaves the analog flag cleared instead. */
      PadSetAct(s2, &s1->output19[0], 2);
      PadSetActAlign(s2, PadDualShockActAlignCmd);
    }
    else
      s1->flag40 = 0;
    s1->flag1B = 1;
    s1->changed1E = 0;
    return;
  }
  if (s0 != 6)
    return;
  { int t = PadInfoMode(s2, 1, 0); v1 = PadInfoMode(s2, 2, 0); s0 = t; }
  if (s0 == 0xE && v1 == 0x300)
  {
    PsyqBzero(&s1->flag40, 0x1C);
    s1->flag40 = 1;
    s1->protocol10 = 2;
    PadSetAct(s2, &s1->actuator44, 1);
    if (PadSetActAlign(s2, PadActAlignBytes) != 0)
    {
      s1->flag1B = 1;
      s1->changed1E = 0;
      PadSetRumbleFlags(s3, 3);
    }
    return;
  }
  if (s0 != 2)
  {
    s1->flag40 = 0;
    s1->protocol10 = 0;
    s0 = 0;
    s3 = PadInfoAct(s2, -1, s0);
    if (s3 == 1)
    {
      w11_r = PadInfoAct(s2, 0, 1);
      v1 = PadInfoAct(s2, 0, 2);
      if (w11_r == 4 && v1 == 3)
        s1->flag40 = s3;
    }
    PadSetAct(s2, &s1->output19[0], 2);
    if (PadSetActAlign(s2, PadDualShockActAlignCmd) == 0)
      return;
    s1->flag1B = 1;
    s1->changed1E = 0;
  }
  else
  {
    s1->protocol10 = -1;
    s1->flag40 = 0;
    s1->flag1B = 1;
  }
}


/* Initial values of the globals this module owns. */
u8 PadDualShockActAlignCmd[8] = {0, 1, 0xFF, 0xFF, 0xFF, 0xFF};
u8 PadActAlignBytes[8] = {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
