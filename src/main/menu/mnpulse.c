#include "game/system/pad.h"
#include "rr/menu_pulse.h"

void PadStepPulse(void) {
    s32 port;
    u16 repeat_tick;
    u16 once_tick;
    const PulseRow *repeat_row;
    const PulseRow *once_row;

    for (port = 0; port < 2; port++) {
        ControllerState5C *output;
        PadPulseState18 *state;
        state = &PadPulseState[port];
        output = &PlayerControllers[port];
        if (Options.pulse[port].disabled02 != 0) {
            output->output19[0] = 0;
            output->output19[1] = 0;
        } else {
            if (state->repeat04 != 0) {
                if (output->kind1C != 0) {
                    repeat_row = &PadPulseRepeatStepTable[state->repeat04 - 1][state->repeatStep0C];
                    output->output19[0] = repeat_row->first;
                    output->output19[1] = repeat_row->second;
                } else {
                    repeat_row = &PadPulseRepeatStepTable[state->repeat04 + 2][state->repeatStep0C];
                    output->output19[0] = 64;
                    output->output19[1] = repeat_row->first;
                }
                repeat_tick = ++state->repeatTick08;
                if ((s16)repeat_tick >= repeat_row->duration) {
                    state->repeatStep0C = (u16)state->repeatStep0C + 1;
                    if (repeat_row[1].duration < 0) {
                        state->repeatTick08 = 0;
                        state->repeatStep0C = 0;
                    }
                }
            } else {
                output->output19[0] = 0;
                output->output19[1] = 0;
            }
            if (state->once06 != 0) {
                if (output->kind1C != 0) {
                    once_row = &PadPulseOnceStepTable[state->once06 - 1][state->onceStep0E];
                    output->output19[0] = once_row->first;
                    output->output19[1] = once_row->second;
                } else {
                    once_row = &PadPulseOnceStepTable[state->once06 + 2][state->onceStep0E];
                    output->output19[0] = 64;
                    output->output19[1] = once_row->first;
                }
                once_tick = ++state->onceTick0A;
                if ((s16)once_tick >= once_row->duration) {
                    state->onceStep0E = (u16)state->onceStep0E + 1;
                    if (once_row[1].duration < 0) {
                        state->once06 = 0;
                        if (output->kind1C != 0) {
                            output->output19[0] = 0;
                        } else {
                            output->output19[0] = 64;
                        }
                        output->output19[1] = 0;
                    }
                }
            }
        }
    }
}

/* Initial values of the globals this module owns. */
PulseRow D_8009FE8C[4] = {{.second = 0x96, .duration = 6}, {.first = 1, .second = 0x40, .duration = 8}, {.first = 1, .duration = 10}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FE9C[5] = {{.second = 0xC8, .duration = 6}, {.first = 1, .second = 0x32, .duration = 7}, {.first = 1, .second = 0xB4, .duration = 12}, {.first = 1, .duration = 15}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FEB0[5] = {{.first = 1, .second = 0xFF, .duration = 8}, {.first = 1, .second = 0x32, .duration = 9}, {.first = 1, .second = 0xFF, .duration = 12}, {.first = 1, .duration = 15}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FEC4[2] = {{.second = 0xFF, .duration = 4}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FECC[6] = {{.second = 0xAA, .duration = 4}, {.first = 1, .duration = 8}, {.second = 0x32, .duration = 12}, {.first = 1, .duration = 13}, {.duration = 15}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FEE4[5] = {{.second = 0xC8, .duration = 6}, {.first = 1, .duration = 8}, {.second = 0x32, .duration = 12}, {.first = 1, .duration = 14}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FEF8[5] = {{.first = 1, .second = 0xFF, .duration = 6}, {.first = 1, .duration = 8}, {.second = 0x64, .duration = 10}, {.first = 1, .duration = 12}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FF0C[2] = {{.first = 1, .duration = 4}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FF14[4] = {{.first = 1, .duration = 4}, {.duration = 6}, {.first = 1, .duration = 8}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FF24[6] = {{.first = 1, .duration = 6}, {.duration = 8}, {.first = 1, .duration = 10}, {.duration = 12}, {.first = 1, .duration = 13}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FF3C[5] = {{.first = 1, .duration = 1}, {.duration = 4}, {.first = 1, .duration = 6}, {.duration = 10}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FF50[5] = {{.first = 1, .duration = 1}, {.duration = 3}, {.first = 1, .duration = 6}, {.duration = 10}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
PulseRow D_8009FF64[7] = {{.first = 1, .duration = 2}, {.duration = 3}, {.first = 1, .duration = 4}, {.duration = 5}, {.first = 1, .duration = 8}, {.duration = 10}, {.first = 0xFF, .second = 0xFF, .duration = -1}};
const PulseRow *PadPulseOnceStepTable[8] = {(const PulseRow *)D_8009FE8C, (const PulseRow *)D_8009FE9C, (const PulseRow *)D_8009FEB0, (const PulseRow *)D_8009FEC4, (const PulseRow *)D_8009FF0C, (const PulseRow *)D_8009FF14, (const PulseRow *)D_8009FF24, (const PulseRow *)D_8009FEC4};
const PulseRow *PadPulseOnceStepTable[], *PadPulseRepeatStepTable[6] = {(const PulseRow *)D_8009FECC, (const PulseRow *)D_8009FEE4, (const PulseRow *)D_8009FEF8, (const PulseRow *)D_8009FF3C, (const PulseRow *)D_8009FF50, (const PulseRow *)D_8009FF64};
