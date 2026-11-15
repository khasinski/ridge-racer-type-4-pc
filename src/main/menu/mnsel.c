#include "game/system/pad.h"
#include "game/data/menuvars.h"
#include "game/ui/uidraw.h"
#include "game/sound/sndvab.h"
#include "rr/save_slots.h"
#include "rr/menu_text_state.h"
#include "rr/menu_grid.h"
#include "rr/ui.h"
#include "rr/badge_row_types.h"
#include "rr/menu_state_types.h"
#include "game/menu/mnsel.h"
#include "rr/bios_devices.h"
#include "game/data/selection.h"
#include "rr/badge_rows.h"
#include "game/menu/mnrows.h"

extern int MenuStepDirection;
extern short SelectedTeamIndex;

void StepModeSelectChoice(void) {
    int s0;
    u8 *q;
    MenuSettings2C *r;
    int t = SelectionRows.bytes[0];
    SelectedTeamIndex = t;
    UiRows->cells[t] = 0;
    if ((MenuSelectedCarSlot & 0x2000) && SelectedTeamIndex < 3) {
        SelectedTeamIndex = (unsigned short)SelectedTeamIndex + 1;
        MenuStepDirection = 2;
    } else if ((MenuSelectedCarSlot & 0x8000) && SelectedTeamIndex > 0) {
        SelectedTeamIndex = (unsigned short)SelectedTeamIndex - 1;
        MenuStepDirection = 1;
    } else {
        MenuStepDirection = 0;
        goto skip_step;
    }
    PlaySfx(0);
    if (SelectedTeamIndex < 0) {
        SelectedTeamIndex = 0;
    } else if (SelectedTeamIndex >= 4) {
        SelectedTeamIndex = 3;
    }
    SelectionRows.bytes[0] = SelectedTeamIndex;
    Dialogue->b4 = (unsigned char)SelectedTeamIndex + 0x22;
    GlyphRows[SelectedTeamIndex].control02 = -6;
    GlyphRows[SelectedTeamIndex].frameIndex00 = 0;
    UiRows->byte16 = SelectedTeamIndex;
skip_step:;
    q = UiRows->cells;
    q[SelectedTeamIndex] = 2;
    if (MenuReturnSubmodeIndex != 0) {
        s0 = BadgeRows->state10;
        if (s0 == 3) {
            PlaySfx(1);
            ModeFrame = 0;
            ResetScreenCounters();
            r = &SelectionState.fields;
            r->mode1A = 1;
            r->copies[0].bytes[0] = SelectionRows.bytes[0];
            BadgeRows->state10 = 4;
            UiRows->byte14 = s0;
            ModeGroup = 1;
            ModeIndex = 0xB;
            SubmodeState = 0;
        }
    }
}

int AllocMenuGridRecords(void *buffer, int count) {
    int sz = count * sizeof(*MenuGridRecords);
    MenuGridRecords = buffer;
    PsyqBzero(buffer, sz);
    return sz;
}


void InitMenuGrid(s32 index, s32 layout, s32 count, s32 x, s32 y) {
    s32 six = 6;
    s32 quotient = six / count;
    MenuGrid38 *entry;
    s32 step_y;
    s32 step_x;
    /* MenuGridRecords is reloaded after each record write, as on the PS1. */
    entry = &MenuGridRecords[index];
    entry->columns2E = quotient * 3;
    MenuGridRecords[index].rows2F = six;
    entry = &MenuGridRecords[index];
    step_y = (s16)y / entry->rows2F;
    step_x = (s16)x / (s32)((u32)count * entry->columns2E);
    entry->count2C = count;
    entry->width04 = 64;
    entry->height06 = 8;
    entry->step_y0A = step_y;
    entry->step_x08 = step_x;
    __builtin_memcpy(&entry->u30, &SelectionAnimStepTable[layout], sizeof(GridRow8));
    entry = &MenuGridRecords[index];
    entry->du32 = entry->du32 / entry->columns2E;
    entry = &MenuGridRecords[index];
    entry->dv33 = entry->dv33 / entry->rows2F;
}

/* Initial values of the globals this module owns. */
GridRow8 SelectionAnimStepTable[2] = {{.first = 0x804C00B4u, .second = 0x2E2A000Au}, {.first = 0x804C00B4u, .second = 0x1B240029u}};
short SelectedTeamIndex;
