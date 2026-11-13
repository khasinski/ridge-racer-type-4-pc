#include "rr/release.h"
#include "rr/sprite_emit.h"
#include "rr/player_cars.h"
#include "rr/menu_selection.h"
#include "rr/gpu_line.h"
#include "rr/gpu_scratch_types.h"
#include "scratchpad.h"
#include "rr/frame_storage_types.h"
#include "game/race/objsim.h"
#include "game/ui/hudmap.h"
#include "game/course/trktext.h"
#include "game/ui/uipart1.h"
#include "rr/race_types.h"
#include "rr/track_lateral.h"
#include "rr/badge_rows.h"

extern char StrTotalTimeLabel[];
extern char LapTimeLabel[];
extern char StrPressStart[];
extern int TotalRaceTime;
extern int LapTimes[];
extern UiUvRect HudSignalLightUv;
extern int RaceState;



static inline u32 ScaleLineDeltaQ8(u32 delta, u32 scale) {
    s32 product = delta * scale;
    return product >> 8;
}


void DrawRaceTimesScreen(void) {
    char sp18[8];
    int i;
    int y;
    int *q;
    char *b;
    int col, v;
    int sel;
    RR_GPU_SCRATCH->cursor = AddDrawAreaPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, RR_PAL_JP(0x100, 0xF0));
    i = 0;
    sel = PlayerCars[0].control.dynamics94.steering.finish20; /* HudMapZoomMode 0x800AC1A0 */
    if (NumLaps > 0) {
        y = 0x86;
        q = LapTimes;
    L1:
        v = *q;
        col = 0x7EF8;
        if (i == sel) {
            col = 0x7EF8;
        }
        DrawRaceTime(0x78, y, v, col, 1);
        y += 0xA;
        q += 1;
        if (++i < NumLaps) goto L1;
    }
    DrawRaceTime(0x78, 0x5E, TotalRaceTime, 0x7EF8, 1);
    DrawFontStringCentered(0x78, 0x50, StrTotalTimeLabel, 0x80, 0);
    DrawFontStringCentered(0x78, 0x78, LapTimeLabel, 0x80, 0);
    i = 7;
    b = sp18 + 7;
    do {
        *b = 0;
        i -= 1;
        b -= 1;
    } while (i >= 0);
    FormatDecimalDigits(sp18, PlayerCars[0].half1EE + 1, 1); /* HudDisplayedLapIndex */
    DrawFontStringCentered(0x78, 0x3C, sp18, 0x80, 0);
    DrawFontStringCentered(0x78, 0xC8, StrPressStart, 0x80, 0);
    AddSetTexpagePrim(0);
}

void DrawRaceStartSignalSprite(int a0) {
    int a3;
    GpuDmaTag *s0;
    void *p;
    if ((RaceState - 1) < 3U) {
        p = RR_GPU_SCRATCH->cursor;
        if (a0 > 0) {
            /* Split screen halves of 120 lines in the Japanese release. */
            a3 = RR_RELEASE_JP ? (a0 - 1) * 120 + 0x34 : ((a0 - 1) << 7) + 0x38;
            if (a0 == 1) {
                s0 = &CurrentDrawBuf->buffer.ordering70.tags[0];
            } else {
                s0 = &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0];
            }
        } else {
            a3 = RR_PAL_JP(0x78, 0x70);
            s0 = &CurrentDrawBuf->buffer.ordering70.tags[0];
        }
        RR_GPU_SCRATCH->cursor = AddDrawModePrim(s0, AddSpritePrim(s0, p, 0x6A, a3, &HudSignalLightUv, 0x7EF8, 0), 0x3E);
    }
}

void AddPartialLine(GpuDmaTag *a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8, s32 a9, s32 a10)
{
    int d1, d2, x, y;
    void *r;
    if (a5 > 0) {
        d1 = a3 - (u32)a1;
        d2 = a4 - (u32)a2;
        if (a5 >= 0x100) a5 = 0x100;
        r = RR_GPU_SCRATCH->cursor;
        x = a1 + ScaleLineDeltaQ8(d1, a5);
        y = a2 + ScaleLineDeltaQ8(d2, a5);
        if (a10 >= 0) {
            r = AddLineF2SemiTrans(a0, r, a1, a2, x, y, a7, a8, a9);
            if (a6 != 0) {
                r = AddLineF2SemiTrans(a0, r, a1, a2 + 1U, x, y + 1U, a7, a8, a9);
            }
            r = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], r, ((a10 & 3) << 5) | 5);
        } else {
            r = GameQueueTileTrans(a0, r, a1, a2, x, y, a7, a8, a9);
            if (a6 != 0) {
                r = GameQueueTileTrans(a0, r, a1, a2 + 1U, x, y + 1U, a7, a8, a9);
            }
        }
        RR_GPU_SCRATCH->cursor = r;
    }
}


int IsCarFacingReverse(RaceVehicle *vehicle) {
    int reverseHeading = 0x800 - TrackNodes[vehicle->trackNodeF0].road.heading;
    return (unsigned)(((vehicle->heading1D4 - reverseHeading) & 0xFFF) - 0x401) < 0x7FF;
}

/* Initial values of the globals this module owns. */
char StrTotalTimeLabel[12] = {84, 79, 84, 65, 76, 32, 84, 73, 77, 69};
char LapTimeLabel[12] = {76, 65, 80, 32, 84, 73, 77, 69};
char StrPressStart[44] = {80, 82, 69, 83, 83, 32, 83, 84, 65, 82, 84};
UiUvRect HudSignalLightUv = {.u = 0x54, .v = 0x10, .w = 0x68, .h = 0x10};
