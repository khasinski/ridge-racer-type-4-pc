#include "rr/release.h"
#include "types.h"
#include "rr/cpu_time.h"
#include "rr/video_timing.h"
#include "rr/runtime_settings.h"
#include "game/system/pad.h"
#include "rr/race_setup.h"
#include "rr/audio_cd_controller.h"
#include "game/menu/mnpulse.h"
#include "game/system/pad3.h"

#ifndef NULL
#define NULL 0
#endif

extern s32 PadActuatorInitStage[2];
extern s32 PadPortConnectionFlags[2];
extern s32 PadMotorActuatorCmd[2];
extern s16 TwistRangeLimits[4];
extern s16 PadRangeLimits[6];
extern s32 WheelDeadzoneTable[4];
extern s32 PadDeadzoneTable[4];
extern s16 PadDigitalSteerSpeed;
int abs(int);
extern char StrControllerNoErrorFmt[];
extern char StrActuatorNoErrorFmt[];

void PadUpdate(void) {
    RR_CPU_CYCLES(RR_CYCLES_PAD_UPDATE);

    PadPort28 *ports[2] = { &PadPorts[0], &PadPorts[1] };
    ControllerState5C *states[2] = { &PlayerControllers[0], &PlayerControllers[1] };
    s32 i;
    s16 val;

    for (i = 0; i < 2; i++) {
        if (ports[i]->type01 != states[i]->input00.fields.type) {
            states[i]->changed1E = 1;
            states[i]->protocol10 = -1;
        }
        states[i]->input00.fields.status = ports[i]->status00;
        states[i]->input00.fields.type = ports[i]->type01;
        if (ports[i]->status00 != 0) {
            PadActuatorInitStage[i] = 1;
            PadMotorActuatorCmd[i] = 0x22;
            PadPortConnectionFlags[i] |= 0x10;
        } else { /* the original tests the array itself: always true */
            PadMotorActuatorCmd[i]--;
            if (ports[i]->type01 == 0x23) {
                u16 btn = ~((ports[i]->data02 << 8) | ports[i]->data03);
                if ((btn & 0x5000) == 0x5000 || (btn & 0xA000) == 0xA000 || (btn & 0x1C4)) {
                    PadActuatorInitStage[i] = 2;
                    PadMotorActuatorCmd[i] = 0x22;
                    PadPortConnectionFlags[i] |= 0x10;
                }
            }
        }
        PadPortConnectionFlags[i] >>= 1;
        if (PadPortConnectionFlags[i] != 0) {
            PadPort28 *port = ports[i];
            ControllerState5C *pad = states[i];
            port->type01 = 0;
            pad->input00.fields.type = 0;
        } else {
            PadActuatorInitStage[i] = 0;
        }
        if (states[i]->input00.fields.type != 0 && states[i]->protocol10 < 0) {
            if (states[i]->input00.fields.type == 0x41) {
                states[i]->protocol10 = 0;
            } else if (states[i]->input00.fields.type == 0x23) {
                states[i]->protocol10 = 1;
            } else if (states[i]->input00.fields.type == 0x73) {
                states[i]->protocol10 = 0;
            } else {
                states[i]->protocol10 = -1;
            }
        }
        switch (ports[i]->type01) {
        case 0x41:
            states[i]->previous04 = states[i]->input00.fields.buttons;
            states[i]->input00.fields.buttons = ~((ports[i]->data02 << 8) | ports[i]->data03);
            states[i]->edges08.fields.pressed = states[i]->input00.fields.buttons & ~states[i]->previous04;
            states[i]->released06 = states[i]->previous04 & (states[i]->input00.fields.buttons ^ states[i]->previous04);
            states[i]->steering20 = (((states[i]->input00.fields.buttons >> 13) & 1) - ((states[i]->input00.fields.buttons & 0x8000) ? 1 : 0)) * TwistRangeLimits[PadDigitalSteerSpeed] + 0x80;
            states[i]->pressure22 = (states[i]->input00.fields.buttons & 0x40) ? 0x6A : 0;
            states[i]->pressure24 = (states[i]->input00.fields.buttons & 0x80) ? 0x6A : 0;
            states[i]->pressure26 = (states[i]->input00.fields.buttons & 4) ? 0x6A : 0;
            states[i]->action28 = states[i]->edges08.fields.pressed & 0x820;
            states[i]->action2C = states[i]->edges08.fields.pressed & 0x40;
            states[i]->action30 = states[i]->edges08.fields.pressed & 0x20;
            states[i]->action34 = states[i]->edges08.fields.pressed & 0x40;
            states[i]->idleStage54 = 0;
            states[i]->idleTick58 = 0;
            break;
        case 0x23:
            states[i]->previous04 = states[i]->input00.fields.buttons;
            states[i]->input00.fields.buttons = ~((ports[i]->data02 << 8) | ports[i]->data03);
            states[i]->released06 = states[i]->previous04 & (states[i]->input00.fields.buttons ^ states[i]->previous04);
            states[i]->steering20 = ports[i]->analog04.bytes[0];
            states[i]->pressure22 = ports[i]->analog04.bytes[1] - Options.pad[i].offset02;
            states[i]->pressure24 = ports[i]->analog04.bytes[2] - Options.pad[i].offset04;
            states[i]->pressure26 = ports[i]->analog04.bytes[3] - Options.pad[i].offset06;
            if (states[i]->pressure22 < 0) {
                states[i]->pressure22 = 0;
            }
            if (states[i]->pressure24 < 0) {
                states[i]->pressure24 = 0;
            }
            if (states[i]->pressure26 < 0) {
                states[i]->pressure26 = 0;
            }
            if (states[i]->steering20 >= 0xA3) {
                states[i]->input00.fields.buttons |= 0x2000;
            }
            if (states[i]->steering20 < 0x5E) {
                states[i]->input00.fields.buttons |= 0x8000;
            }
            if (states[i]->pressure22 >= 0x36) {
                states[i]->input00.fields.buttons |= 0x40;
            }
            if (states[i]->pressure24 >= 0x36) {
                states[i]->input00.fields.buttons |= 0x80;
            }
            if (states[i]->pressure26 >= 0x36) {
                states[i]->input00.fields.buttons |= 4;
            }
            states[i]->edges08.fields.pressed = states[i]->input00.fields.buttons & ~states[i]->previous04;
            states[i]->action28 = states[i]->edges08.fields.pressed & 0x840;
            states[i]->action2C = states[i]->edges08.fields.pressed & 0x80;
            states[i]->action30 = states[i]->edges08.fields.pressed & 0x40;
            states[i]->action34 = states[i]->edges08.fields.pressed & 0x80;
            states[i]->idleStage54 = 0;
            states[i]->idleTick58 = 0;
            break;
        case 0x73:
            states[i]->previous04 = states[i]->input00.fields.buttons;
            states[i]->input00.fields.buttons = ~((ports[i]->data02 << 8) | ports[i]->data03);
            states[i]->edges08.fields.pressed = states[i]->input00.fields.buttons & ~states[i]->previous04;
            states[i]->released06 = states[i]->previous04 & (states[i]->input00.fields.buttons ^ states[i]->previous04);
            /* The Japanese release steers an analog pad with the d-pad, as
             * a digital one; the PAL one with the left stick. */
            if (RR_RELEASE_JP)
                states[i]->steering20 = (((states[i]->input00.fields.buttons >> 13) & 1) - ((states[i]->input00.fields.buttons & 0x8000) ? 1 : 0)) * TwistRangeLimits[PadDigitalSteerSpeed] + 0x80;
            else
                states[i]->steering20 = ports[i]->analog04.bytes[2];
            states[i]->pressure22 = (states[i]->input00.fields.buttons & 0x40) ? 0x6A : 0;
            states[i]->pressure24 = (states[i]->input00.fields.buttons & 0x80) ? 0x6A : 0;
            states[i]->pressure26 = (states[i]->input00.fields.buttons & 4) ? 0x6A : 0;
            states[i]->action28 = states[i]->edges08.fields.pressed & 0x820;
            states[i]->action2C = states[i]->edges08.fields.pressed & 0x40;
            states[i]->action30 = states[i]->edges08.fields.pressed & 0x20;
            states[i]->action34 = states[i]->edges08.fields.pressed & 0x40;
            states[i]->axes14.word = ports[i]->analog04.word;
            states[i]->idleStage54 = 0;
            states[i]->idleTick58 = 0;
            break;
        case 0xE3:
            states[i]->previous04 = states[i]->input00.fields.buttons;
            states[i]->input00.fields.buttons = ~((ports[i]->data02 << 8) | ports[i]->data03);
            states[i]->edges08.fields.pressed = states[i]->input00.fields.buttons & ~states[i]->previous04;
            states[i]->released06 = states[i]->previous04 & (states[i]->input00.fields.buttons ^ states[i]->previous04);
            states[i]->position41 = ports[i]->analog04.bytes[0];
            states[i]->position42 = ports[i]->analog04.bytes[1];
            states[i]->position43 = ports[i]->analog04.bytes[2];
            states[i]->position45 = states[i]->position42;
            states[i]->previous4E = states[i]->position4C;
            states[i]->steering20 = states[i]->position4C = states[i]->position41 + ((s8)states[i]->position45 << 8);
            states[i]->pressure22 = (states[i]->input00.fields.buttons & 0x40) ? 0x6A : 0;
            states[i]->pressure24 = (states[i]->input00.fields.buttons & 0x80) ? 0x6A : 0;
            states[i]->pressure26 = (states[i]->input00.fields.buttons & 4) ? 0x6A : 0;
            states[i]->action28 = states[i]->edges08.fields.pressed & 0x820;
            states[i]->action2C = states[i]->edges08.fields.pressed & 0x40;
            states[i]->action30 = states[i]->edges08.fields.pressed & 0x20;
            states[i]->action34 = states[i]->edges08.fields.pressed & 0x40;
            states[i]->idleStage54 = 0;
            if (states[i]->previous04 == states[i]->input00.fields.buttons) {
                if (VSyncThreshold == 0x80) {
                    states[i]->idleTick58 += 1;
                } else {
                    states[i]->idleTick58 += 2;
                }
                if (states[i]->idleTick58 >= RR_PAL_NTSC(0xBB9, 0xE11)) {
                    states[i]->idleTick58 = RR_PAL_NTSC(0xBB8, 0xE10);
                }
            } else {
                states[i]->idleTick58 = 0;
            }
            /* Idle for 120 s (NTSC release: 3600 frames), warned 110 s. */
            if (states[i]->idleTick58 >= RR_PAL_NTSC(0xBB8, 0xE10)) {
                states[i]->idleStage54 = 2;
            } else if (states[i]->idleTick58 >= RR_PAL_NTSC(0xABF, 0xCE5)) {
                states[i]->idleStage54 = 1;
            }
            break;
        case 0x12:
            states[i]->previous04 = states[i]->input00.fields.buttons;
            states[i]->input00.fields.buttons = ~((ports[i]->data02 << 8) | ports[i]->data03);
            states[i]->edges08.fields.pressed = states[i]->input00.fields.buttons & ~states[i]->previous04;
            states[i]->released06 = states[i]->previous04 & (states[i]->input00.fields.buttons ^ states[i]->previous04);
            states[i]->axes14.word = ports[i]->analog04.word;
            states[i]->action28 = 0;
            states[i]->action2C = 0;
            states[i]->action30 = 0;
            states[i]->action34 = 0;
            states[i]->idleStage54 = 0;
            states[i]->idleTick58 = 0;
            break;
        default:
            if (PadActuatorInitStage[i] == 0) {
                PadActuatorInitStage[i] = 2;
            }
            states[i]->input00.fields.status = 1;
            states[i]->input00.fields.buttons = 0;
            states[i]->edges08.fields.pressed = 0;
            states[i]->edges08.fields.repeat = 0;
            states[i]->steering20 = 0x80;
            states[i]->pressure22 = 0;
            states[i]->pressure24 = 0;
            states[i]->pressure26 = 0;
            states[i]->action28 = 0;
            states[i]->action2C = 0;
            states[i]->action30 = 0;
            states[i]->action34 = 0;
            states[i]->idleStage54 = 0;
            states[i]->idleTick58 = 0;
            break;
        }
        states[i]->edges08.fields.repeat = states[i]->edges08.fields.pressed;
        if (states[i]->input00.fields.buttons == states[i]->previous04) {
            states[i]->repeatTick0C++;
            if (states[i]->repeatTick0C >= 0x22) {
                states[i]->edges08.fields.repeat = states[i]->input00.fields.buttons;
                states[i]->repeatTick0C = 0x1E;
            }
        } else {
            states[i]->repeatTick0C = 0;
        }
        switch (ports[i]->type01) {
        case 0x23:
            val = states[i]->steering20 - (Options.pad[i].center00 + 0x80);
            if (val > 0) {
                val -= PadDeadzoneTable[Options.pad[i].deadzone08];
                if (val < 0) {
                    val = 0;
                }
                if (val > TwistRangeLimits[Options.pad[i].range0A]) {
                    val = TwistRangeLimits[Options.pad[i].range0A];
                }
            } else {
                val += PadDeadzoneTable[Options.pad[i].deadzone08];
                if (val > 0) {
                    val = 0;
                }
                if (val < -TwistRangeLimits[Options.pad[i].range0A]) {
                    val = -TwistRangeLimits[Options.pad[i].range0A];
                }
            }
            states[i]->steering3C = val;
            break;
        case 0xE3:
            val = states[i]->steering20;
            if (val > 0) {
                val -= WheelDeadzoneTable[Options.wheel[i].deadzone00];
                if (val < 0) {
                    val = 0;
                }
                if (val > PadRangeLimits[Options.wheel[i].range01]) {
                    val = PadRangeLimits[Options.wheel[i].range01];
                }
            } else {
                val += WheelDeadzoneTable[Options.wheel[i].deadzone00];
                if (val > 0) {
                    val = 0;
                }
                if (val < -PadRangeLimits[Options.wheel[i].range01]) {
                    val = -PadRangeLimits[Options.wheel[i].range01];
                }
            }
            states[i]->steering3C = (val << 7) / PadRangeLimits[Options.wheel[i].range01];
            break;
        case 0x73:
            val = states[i]->steering20 - 0x80;
            if (abs(val) < 0x18) {
                val = 0;
            }
            if (val > 0) {
                val -= 0x10;
            }
            if (val < 0) {
                val += 0x10;
            }
            states[i]->steering3C = (val * 0x7F) / 111;
            break;
        }
        if (states[i]->pressure22 >= 0x6B) {
            states[i]->pressure22 = 0x6A;
        }
        if (states[i]->pressure24 >= 0x6B) {
            states[i]->pressure24 = 0x6A;
        }
        if (states[i]->pressure26 >= 0x6B) {
            states[i]->pressure26 = 0x6A;
        }
    }
    PadStepPulse();
    PadUpdateRumble();
    for (i = 0; i < 2; i++) {
        PadPollPort(i);
    }
}

int GetControllerWarning(int i) {
    return PlayerControllers[i].idleStage54;
}

void PadRangeCheckSet(unsigned a, unsigned b, short c, short d) {
    PadPulseState18 *t;
    if (a >= 2) {
        DebugPrintStub(StrControllerNoErrorFmt, a);
    }
    if (b >= 2) {
        DebugPrintStub(StrActuatorNoErrorFmt, b);
    }
    t = &PadPulseState[a];
    t->delay10[b] = d;
    t->limit00[b] = c;
    t->active14 = 0;
    t->reserved16 = 0;
}

void PadStartPulse(int i, int k, int v) {
    PadPulseState18 *t = &PadPulseState[i];
    switch (k) {
    case 0:
        t->repeat04 = 0;
        return;
    case 1:
        t->repeat04 = v + 1;
        t->repeatTick08 = 0;
        t->repeatStep0C = 0;
        t->active14 = 0;
        return;
    case 2:
        t->once06 = v + 1;
        t->onceTick0A = 0;
        t->onceStep0E = 0;
        t->active14 = 0;
        return;
    }
}

void PadClearPulse(int i) {
    PadPulseState18 *t = &PadPulseState[i];
    t->delay10[0] = 0;
    t->limit00[0] = 0;
    t->delay10[1] = 0;
    t->limit00[1] = 0;
    t->repeat04 = 0;
    t->once06 = 0;
    t->repeatTick08 = 0;
    t->onceTick0A = 0;
    t->repeatStep0C = 0;
    t->onceStep0E = 0;
}


/* Initial values of the globals this module owns. */
char StrControllerNoErrorFmt[28] = {67, 111, 110, 116, 114, 111, 108, 108, 101, 114, 32, 110, 111, 32, 101, 114, 114, 111, 114, 33, 32, 45, 32, 37, 100, 10};
char StrActuatorNoErrorFmt[52] = {65, 99, 116, 117, 97, 116, 111, 114, 32, 110, 111, 32, 101, 114, 114, 111, 114, 33, 32, 45, 32, 37, 100, 10};
s16 TwistRangeLimits[4] = {25, 38, 75, 113};
s16 PadRangeLimits[6] = {21, 32, 64, 96, 128};
s32 PadDeadzoneTable[4] = {0, 6, 10, 14};
s32 PadPortConnectionFlags[2];
s16 PadDigitalSteerSpeed;
s32 PadMotorActuatorCmd[2];
s32 PadActuatorInitStage[2];
PadPulseState18 PadPulseState[2];
