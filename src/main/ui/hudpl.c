#include "rr/release.h"
#include "rr/widescreen.h"
#include "scratchpad.h"
#include "rr/menu_selection.h"
#include "rr/frame_storage_types.h"
#include "rr/menu_label_render.h"
#include "game/ui/hudpl.h"
extern UiUvRect HudPlayerLabelGlyphs[];

/* "LAP" label glyphs and the current/total lap digits (U = digit * 12). */
void DrawLapCounter(int a0, int a1) {
    UiUvRect buf;
    GpuDmaTag *s4;
    int s5, i;
    void *a;
    if (a0 == 0) {
        s5 = 0xC;
        s4 = &CurrentDrawBuf->buffer.ordering70.tags[0];
    } else {
        s4 = &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0];
        s5 = (a0 - 1) * RR_PAL_JP(128, 120) + 0xC;
    }
    a = RR_GPU_SCRATCH->cursor;
    i = 0;
    do {
        buf = HudPlayerLabelGlyphs[i];
        a = AddSpritePrim(s4, a, WIDE_HUD_RIGHT(0xEC + i * 0x38), s5, &buf, 0x7EF8, 0);
        i++;
    } while (i < 2);
    buf = HudPlayerLabelGlyphs[2];
    i = 0;
    if (a1 <= 0) {
        a1 = 1;
    }
    do {
        if (i == 0) {
            buf.u = a1 * 0xC;
        }
        if (i == 1) {
            buf.u = NumLaps * 0xC;
        }
        a = AddSpritePrim(s4, a, WIDE_HUD_RIGHT(0x118 + i * 0x14), s5, &buf, 0x7EF8, 0);
        i++;
    } while (i < 2);
    RR_GPU_SCRATCH->cursor = AddDrawModePrim(s4, a, 0x25);
}

/* Initial values of the globals this module owns. */
UiUvRect HudPlayerLabelGlyphs[3] = {{.u = 0xB0, .v = 0xA8, .w = 0x2C, .h = 0xC}, {.u = 0xDC, .v = 0xA8, .w = 4, .h = 0xC}, {.v = 0xA8, .w = 0xC, .h = 0xC}};
