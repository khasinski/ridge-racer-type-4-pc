#include "game/system/pad.h"
#include "types.h"
#include "psyq/libpad/padctl.h"
#include "game/car/carstat.h"
#include "rr/bios_devices.h"
#include "psyq/libpad/padgate.h"
#include "rr/runtime_settings.h"
#include "rr/race_setup.h"
#include "rr/tables.h"
#include "rr/menu_pulse.h"

extern unsigned short PadButtonLayoutPresetsA[][8];
extern unsigned short PadButtonLayoutPresetsB[][8];
extern unsigned short ActivePadButtonLayouts[][2][8];

void InitGamePads(void) {
    PsyqBzero(PlayerControllers, sizeof(PlayerControllers));
    PsyqBzero(PadPulseState, sizeof(PadPulseState));
    PlayerControllers[0].protocol10 = -1;
    PlayerControllers[1].protocol10 = -1;
    PadInitDirect((u8 *)&PadPorts[0], (u8 *)&PadPorts[1]);
    PadStartCom();
}

void SetPadButtonMapLayout(int a, int b, int c, int d) {
    int i;
    for (i = 0; i < 8; i++) {
        PadButtonMap[a].standard[i] = PadButtonLayoutPresetsA[b][i];
        PadButtonMap[a].calibrated[i] = PadButtonLayoutPresetsB[c][i];
        PadButtonMap[a].wheel[i] = PadDefaultButtonPresets[d][i];
        ActivePadButtonLayouts[0][0][i] = PadDefaultButtonPresets[b][i];
        ActivePadButtonLayouts[0][1][i] = PadButtonLayoutPresetsB[c][i];
    }
}

void ApplyOptionsPadButtonMaps(void) {
    SetPadButtonMapLayout(0, Options.pulse[0].buttonLayout00, Options.pad[0].buttonLayout0C, Options.wheel[0].buttonLayout03);
    SetPadButtonMapLayout(1, Options.pulse[1].buttonLayout00, Options.pad[1].buttonLayout0C, Options.wheel[1].buttonLayout03);
}

/* Initial values of the globals this module owns. */
unsigned short PadButtonLayoutPresetsA[8][8] = {{0x8000, 0x2000, 0x40, 0x80, 0xA, 5, 0x10, 0x20}, {0x8000, 0x2000, 0x40, 0x80, 8, 2, 0x10, 0x20}, {0x8000, 0x2000, 0x80, 0x40, 0xA, 5, 0x10, 0x20}, {0x8000, 0x2000, 0x80, 0x40, 8, 2, 0x10, 0x20}, {0x8000, 0x2000, 0x20, 0x40, 0xA, 5, 0x10, 0x80}, {0x8000, 0x2000, 0x20, 0x40, 8, 2, 0x10, 0x80}, {0x8000, 0x2000, 0x40, 0x20, 0xA, 5, 0x10, 0x80}, {0x8000, 0x2000, 0x40, 0x20, 8, 2, 0x10, 0x80}};
unsigned short PadButtonLayoutPresetsB[8][8] = {{0x8000, 0x2000, 0x40, 0x80, 0x4000, 0x1000, 0x10, 0x20}, {0x8000, 0x2000, 0x80, 0x40, 0x4000, 0x1000, 0x10, 0x20}, {0x8000, 0x2000, 8, 4, 0x4000, 0x1000, 0x10, 0x20}, {0x8000, 0x2000, 0x80, 4, 0x4000, 0x1000, 0x10, 0x20}, {0x8000, 0x2000, 0x20, 0x10, 0x4000, 0x1000, 0x80, 0x40}, {0x8000, 0x2000, 0x40, 0x80, 8, 4, 0x10, 0x20}, {0x8000, 0x2000, 0x80, 0x40, 8, 4, 0x10, 0x20}, {0x8000, 0x2000, 0x10, 0x20, 8, 4, 0x80, 0x40}};
/* Eight rows (0x800A00EC..0x800A016B): the first-person layout copy indexes
 * this table with the 8-entry PadButtonLayoutPresetsA selector. */
unsigned short PadDefaultButtonPresets[8][8] = {{0x8000, 0x2000, 0xA, 5, 0x4000, 0x1000, 0x10, 0x20}, {0x8000, 0x2000, 0xA, 5, 0x40, 0x80, 0x10, 0x20}, {0x8000, 0x2000, 8, 2, 4, 1, 0x10, 0x20}, {0x8000, 0x2000, 4, 1, 8, 2, 0x10, 0x20}, {0x8000, 0x2000, 4, 1, 0x4000, 0x1000, 0x10, 0x20}, {0x8000, 0x2000, 0x40, 0x80, 8, 2, 0x10, 0x20}, {0x8000, 0x2000, 8, 2, 0x40, 0x80, 0x10, 0x20}, {0x8000, 0x2000, 0x40, 0x80, 0xA, 5, 0x10, 0x20}};
unsigned short ActivePadButtonLayouts[1][2][8];
PadPort28 PadPorts[2];
