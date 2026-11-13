#include "rr/release.h"
#include "rr/video_timing.h"
#include "game/system/pad.h"
#include "game/sound/cdpos.h"
#include "rr/save_slots.h"
/* RODATA 0x80010538 0x18 */
#include "scratchpad.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"
#include "game/data/trkld1.h"
#include "game/menu/menuicon.h"
#include "game/ui/barsel0.h"
#include "game/menu/menustat.h"
#include "game/data/selection.h"
#include "game/data/menuvars.h"
#include "rr/vehicle_table.h"
#include "rr/screen_tiles.h"
#include "game/sound/cdstart.h"
#include "game/ui/badgespr.h"
#include "game/sound/cdplay.h"
#include "game/ui/badge1.h"
#include "game/menu/mnrows.h"
#include "game/data/ldscn1.h"
#include "game/car/carbound.h"
#include "game/gfx/scene1.h"
#include "rr/badge_rows.h"

extern unsigned int ModeFrame;
extern unsigned short SubmodeState;
extern RaceVehicle PlayerCars[2];
extern unsigned char MenuCameraPresetIndices[];
extern unsigned short FadeCountdown;
extern short ModeIndex;

void CarStandingsModeUpdate();

void CarStandingsModeUpdate(void) {
    int sp[3];
    unsigned int x;
    int r;
    MenuSettings2C *p;
    ModeFrame++;
    if (ModeFrame > 0x7FFFF) ModeFrame = 0x1000;
    switch (SubmodeState) {
    case 0:
        p = &SelectionState.fields;
        if (p->mode1A == 6) {
            if (LoadCarRangeStep(0, 1) == 1) {
                ResetCarState(PlayerCars);
                SavedLoadCursor = LoadCursor;
                r = (p->phase17 - 1) / 2;
                x = MenuCameraPresetIndices[p->reserved10[4]];
                if (r >= 3) { r = 2; x = x * 3; }
                InitCameraState(r, x);
                SetRaceLoadCursor(LoadCursor);
                SubmodeState = 3;
            }
        } else if (ModeFrame >= RR_PAL_NTSC(0x96, 0xB4)) {
            if (MenuReturnSubmodeIndex != 0 || ModeFrame >= RR_PAL_NTSC(0x177, 0x1C2)) {
                SubmodeState = 1;
            }
        }
        break;
    case 1:
        FadeCountdown = 0x20;
        SubmodeState = 5;
        break;
    case 3:
        if (LoadMenuCarStep(SelectionRows.bytes[0], SelectionRows.bytes[1])) SubmodeState = 4;
        break;
    case 4:
        if (MenuReturnSubmodeIndex != 0 || ModeFrame >= RR_PAL_NTSC(0x177, 0x1C2)) {
            CdAudioInit();
            CdAudioSelectTrack(0, 1);
            SubmodeState = 1;
        }
        break;
    case 5:
        FadeCountdown--;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x20 - (short)FadeCountdown) << 3, 0x45);
        RR_GPU_SCRATCH->cursor = AddDrawAreaPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, RR_PAL_JP(0x200, 0x1E0));
        if ((short)FadeCountdown <= 0) {
            if (SelectionState.fields.mode1A == 6) {
                InitMenuArenas();
                ModeGroup = 1;
                ModeIndex = 0x21;
                SubmodeState = 0;
                LoadCursor = SavedLoadCursor;
                CdAudioBeginPlay(0);
            } else {
                ModeGroup = 1;
                ModeIndex = 0x11;
                SubmodeState = 0;
            }
            return;
        }
        break;
    default:
        break;
    }
    DrawMenuCarPreviewScene(ModeFrame);
    r = x = ModeFrame;
    if (x < 0x20) {
        sp[0] = (int)(x * 255 + (0x20 - x) * 8) / 32;
        sp[1] = (int)(x * 189 + (0x20 - x) * 8) / 32;
        r = 0x20 - x;
        sp[2] = r / 4;
    } else {
        sp[0] = 0xFF;
        sp[1] = 0xBD;
        sp[2] = 0;
    }
    /* The Japanese release fills its 480 lines with one tile. */
    RR_GPU_SCRATCH->cursor =
        (RR_RELEASE_JP ? AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[703], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x1E0, sp[0], sp[1], sp[2])
                      : AddTileGrid2x2(&CurrentDrawBuf->buffer.ordering70.tags[703], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x200, sp[0], sp[1], sp[2]));
}

void *AddBadgeSpriteStrip(int a, GpuDmaTag *p, void *x, int y, int w, int z) {
    void *t0 = x;
    int s1 = y;
    int i = 0;
    do {
        signed char *q = &BadgeStripPatterns[i + a * 8];
        int v = *q;
        const UiUvRect *e;
        int s0, d, c0, c1, c3;
        i++;
        if (v == -1) break;
        e = &BadgeGlyphUvTable[v];
        d = q[4];
        c0 = e->u;
        c1 = e->v;
        s0 = e->w;
        c3 = e->h;
        s1 += d;
        t0 = AddSpriteWithDrawMode(p, t0, s1, w, s0, c3, c0, c1, 0x4A, 0x2E28, 1, z);
        s1 += s0;
    } while (i < 4);
    return t0;
}

/* Initial values of the globals this module owns. */
unsigned char MenuCameraPresetIndices[4] = {0x1E, 0x19, 0x14, 0xF};
void *SavedLoadCursor;
