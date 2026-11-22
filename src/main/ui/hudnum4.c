#include "rr/field_rate.h"
#include "rr/widescreen.h"
#include "rr/cpu_time.h"
#include "types.h"
#include "rr/player_cars.h"
#include "rr/save_slots.h"
#include "rr/vehicle_table.h"
#include "rr/menu_selection.h"
#include "rr/frame_storage_types.h"
#include "scratchpad.h"
#include "game/ui/window2.h"
#include "game/race/objsim.h"
#include "psyq/libgpu/primattr.h"
#include "rr/hud_number.h"
#include "rr/fade_sprite.h"
#include "rr/audio_loop.h"
#include "rr/race_types.h"
#include "rr/gpu.h"
#include "game/data/globals.h"
#include "game/data/screenframe.h"
#include "rr/race_setup.h"
#include "game/ui/badge1.h"
#include "rr/trig.h"

extern unsigned char HudLapTargetThresholds[];
extern unsigned char HudRankDigitTileU[];
extern RaceVehicle PlayerCars[2];

void DrawHudDigitMod10(s32 arg0, s32 arg1) {
    NumberTarget *target = &HudNumberTargets[arg1];
    GpuDmaTag *ordering = target->heads[DrawBufIndex];
    void *cursor;
    RR_CPU_CYCLES(RR_CYCLES_HUD_DIGIT_MOD10);
    cursor = EmitHudGlyph12Alt(ordering, RR_GPU_SCRATCH->cursor, (s16)WIDE_HUD_RIGHT(target->digit.x), target->digit.y, arg0 % 10, 0x7EFB);
    RR_GPU_SCRATCH->cursor = AddDrawModePrim(ordering, cursor, 5);
}

/* Player a1's HUD sprites start at hudSprites[a1 * 6]: sprite 2 is the rank
 * digit and sprite 4 the field-size digit. */
void UpdateHudRankDisplay(int a0, int a1) {
    int a, s0, s1, v;
    SpritePacket *sprites, *s2;
    RR_CPU_CYCLES(RR_CYCLES_HUD_RANK_DISPLAY);
    sprites = CurrentDrawBuf->buffer.hudSprites;
    s2 = &sprites[2];
    sprites[a1 * 6 + 2].u = (a0 + 1) * 0x18;
    if (GameModeType == 4) {
        a = LinkPlayerCount;
    } else {
        if (GameModeType == 3) {
            a = 2;
        } else {
            a = NumCars;
        }
    }
    sprites[a1 * 6 + 4].u = HudRankDigitTileU[a - 1];
    if (GameModeType == 1) {
        s0 = 0x40;
        s1 = 0;
        if (PlayerCars[0].half1EE <= /* HudDisplayedLapIndex 0x800AC0E6 */ HudLapTargetThresholds[SelectionState.fields.phase17 / 2]) {
            s1 = 1;
            RR_CPU_CYCLES(RR_CYCLES_HUD_RANK_DISPLAY_FLASH);
            /* a 64-tick pulse, by the field (rr/field_rate.h) */
            v = rsin((((s32)RaceFieldTimeAfter(ScreenFrame & 0x3F) << 6) / RaceFieldsPerTick()) & 0xFFF);
            if (v < 0) v += 0x3F;
            s0 = v >> 6;
            if (s0 < 0) {
                s1 = 0;
                s0 = -s0;
            }
        }
        s0 += 0x40;
        s2->clut = s1 + 0x7EF8;
        SetShadeTex(s2, 0);
        s2->r = s0;
        s2->g = s0;
        s2->b = s0;
    }
}

void UpdateFinalLapHudSprite(int a0) {
    int a1, a2;
    SpritePacket *p;
    a2 = a0 - 1;
    if (a0 == 0) {
        a1 = 7;
        a2 = 0;
    } else {
        a1 = a2 * 6 + 1;
    }
    p = &CurrentDrawBuf->buffer.hudSprites[a1];
    if (PlayerCars[a2].control.dynamics94.steering.count22 < NumLaps) {
        p->w = 0;
        return;
    }
    p->w = 0x14;
}

/* Initial values of the globals this module owns. */
unsigned char HudLapTargetThresholds[4] = {2, 1};
unsigned char HudRankDigitTileU[8] = {0, 0xC0, 0xD0, 0xE0, 0xF0, 0xF0, 0xF0, 0xF0};
