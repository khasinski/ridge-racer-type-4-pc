#include "game/ui/badge1.h"
#include "rr/release.h"
#include "rr/widescreen.h"
#include "rr/video_timing.h"
#include "game/camera/campath.h"
#include "rr/frame_storage_types.h"
#include "game/camera/camrun.h"
/* RODATA 0x80010448 0x18 */
#include "types.h"
#include "scratchpad.h"
#include "rr/geometry.h"
#include "rr/gpu.h"
#include "rr/race.h"
#include "game/sound/sndvab.h"
#include "game/ui/window1.h"
#include "rr/driver_control.h"
#include "rr/bios_devices.h"
#include "rr/menu_selection.h"
#include "rr/sprite_emit.h"
#include "rr/race_setup.h"
#include "rr/menu_background.h"
#include "game/data/ldscn1.h"
#include "game/system/pad.h"
#include "game/ui/uipart1.h"
#include "rr/trig.h"
#include "game/course/trkintp.h"
#include "rr/badge_rows.h"
#include "game/ui/badgespr.h"
#include "rr/animated_glyph.h"

typedef struct { unsigned char c[4]; } B4;

typedef UiUvRect R4;


typedef struct { u8 c[8]; } B8;

extern B4 ScreenFadeColors[];
extern unsigned short UiTitleMaskVramRect[];
extern R4 BadgeTitleGlyphUvs[];
#define BadgeTitleColonUv (RR_PAL_JP(BadgeTitleGlyphUvs, BadgeTitleGlyphUvsJp)[22])
extern signed char BadgeTitleGlyphIndices[][4];
static const R4 BadgeTitleGlyphUvsJp[51];
static const signed char BadgeTitleGlyphIndicesJp[37][4];
#define BADGE_TITLE_GLYPH_INDICES \
    (RR_RELEASE_JP ? BadgeTitleGlyphIndicesJp : (const signed char (*)[4])BadgeTitleGlyphIndices)
extern short BadgeTitleColumnX[];
extern short BadgeTitleBarX[];
extern R4 BadgeTitleDigitUv;
extern s8 BeamFadeStripHeights[];
extern char UiBeamFadePalettePair[];
extern s32 MenuStepDirection;
extern char RaceTimeDigits[];
extern int LapTimes[];
extern char TimeTextMMSS[];
extern unsigned short StepProgressBarWidths[];
extern char StepProgressBarColors[];
extern int ReplayState;

void ClearScreenOverlayState(void);
void DrawMenuFrame(int arg0);
void DrawArrowTrianglePair(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5);
void CopyStringClearTail(char *d, char *s);
void CopyStringPadSpaces(char *d, unsigned char *s, int n);
void DrawLapTimeList(int a0);
char *FormatFrameTimeMMSS(int a);
void DrawTimeRemaining(int a);
void DrawTimeRemainingForPlayer(int a, int b);

/* Packed GPU XY word (X low halfword, Y high). */
static s32 Vec2sWord(const Vec2s *v) {
    return (s32)((u32)(u16)v->x | ((u32)(u16)v->y << 16));
}

void UpdateScreenFade(int a0) {
    B4 sp;
    int a, t, a1;
    ScreenFadeState0C *st = &ScreenFadeState;
    CurrentDrawBuf->buffer.environment00.draw00.background = 0;
    a = (a0 >> 1) & 1;
    st->target = a;
    if (a != st->prevTarget) ScreenFadeState.phase = 0;
    st->prevTarget = a;
    t = st->tick;
    switch (ScreenFadeState.phase) {
    case 0:
        sp = ScreenFadeColors[a ^ 1];
        st->tick = 0;
        ScreenFadeState.phase = 1;
        break;
    case 1:
        {
            B4 *p = &ScreenFadeColors[st->target];
            B4 *q = &ScreenFadeColors[st->target ^ 1];
            a1 = 0x1E - t;
            sp.c[0] = (p->c[0] * t + q->c[0] * a1) / 30;
            sp.c[1] = (p->c[1] * t + q->c[1] * a1) / 30;
            sp.c[2] = (p->c[2] * t + q->c[2] * a1) / 30;
        }
        if (t >= 0x1E) {
            ScreenFadeState.phase = 2;
            st->tick = 0;
        } else {
            if (t == 2) {
                if (MenuFrameState.s0 == 0) SetDispMask(1);
            }
            ScreenFadeState.tick = ScreenFadeState.tick + 1;
        }
        break;
    case 2:
        sp = ScreenFadeColors[st->target];
        break;
    }
    {
        /* The Japanese release fills its 480 lines with one tile. */
        RR_GPU_SCRATCH->cursor =
        (RR_RELEASE_JP ? AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[703], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x1E0, sp.c[0], sp.c[1], sp.c[2])
                      : AddTileGrid2x2(&CurrentDrawBuf->buffer.ordering70.tags[703], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x200, sp.c[0], sp.c[1], sp.c[2]));
    }
    DrawMenuFrame(a0 >> 2);
    if (MainMenuChoice != 6 && MainMenuChoice != 8) {
        SetPresentationDrawEnabled(a0 & 1);
        StepCameraScript();
    }
}

void ClearScreenOverlayState(void) {
    PsyqBzero(&MenuFrameState, sizeof(MenuFrameState));
    PsyqBzero(&ScreenFadeState, sizeof(ScreenFadeState));
}


void DrawMenuFrame(int arg0) {
    R4 sp30;
    int s7;
    int y;
    int sp38;
    int s0;
    int s6;
    int v;
    int s1, j, m, k;
    int s4;
    int s5;
    unsigned short sp40;
    void *a1;
    int c;
    const R4 *r;
    int s2;
    unsigned short *pc;
    int col2, col3;

    s7 = 0x80;
    if (MenuFrameState.b6 != arg0) {
        MenuFrameState.b7 = MenuFrameState.b6;
    }
    /* The Japanese release draws the bottom bar 8 lines higher and all
     * seven columns (the PAL release starts at the third). */
    y = RR_PAL_JP(0x1C0, 0x1B8);
    MenuFrameState.b6 = arg0;
    s0 = MenuFrameState.s2;
    s6 = MenuFrameState.b6;
    sp38 = ScreenFadeState.target;

    switch (MenuFrameState.s0) {
    case 0:
        MenuFrameState.s12 = 0x38;
        MenuFrameState.s1A = RR_PAL_JP(0x1B8, 0x1B0);
        MenuFrameState.s16 = MenuFrameState.s1E = 2;
        if (ScreenFadeState.phase == 2) {
            MenuFrameState.s0 = 1;
            MenuFrameState.b4 = ScreenFadeState.target;
        }
        break;
    case 1:
        MenuFrameState.s14 = MenuFrameState.s1C = (rsin((s0 << 10) / 30) * 0x140) / 0x1000;
        if (s0 >= 30) {
            MenuFrameState.s0 = 2;
            MenuFrameState.s2 = 0;
        } else {
            MenuFrameState.s2++;
        }
        break;
    case 2:
        v = (rsin((s0 << 10) / 20) * 0x20) / 0x1000;
        MenuFrameState.s16 = MenuFrameState.s1E = v * 2 + 2;
        MenuFrameState.s12 = 0x38 - v;
        MenuFrameState.s1A = RR_PAL_JP(0x1B8, 0x1B0) - v;
        if (s0 >= 20) {
            MenuFrameState.s0 = 3;
            MenuFrameState.s2 = 0;
        } else {
            MenuFrameState.s2++;
        }
    case 3:
        if (ScreenFadeState.phase == 1) {
            MenuFrameState.s0 = 4;
            MenuFrameState.s2 = 0;
        }
        if (ScreenFadeState.phase != 2) {
            s6 = MenuFrameState.b7;
            sp38 = ScreenFadeState.target ^ 1;
        }
        break;
    case 4:
        if (s0 >= 20) {
            if (ScreenFadeState.phase == 2) {
                MenuFrameState.s0 = 5;
                MenuFrameState.s2 = 0;
                MenuFrameState.b4 = ScreenFadeState.target;
            }
        } else {
            MenuFrameState.s2++;
        }
        s7 = ((20 - s0) * 0x80) / 20;
        sp38 = ScreenFadeState.target ^ 1;
        s6 = MenuFrameState.b7;
        break;
    case 5:
        if (s0 >= 20) {
            MenuFrameState.s0 = 3;
            MenuFrameState.s2 = 0;
        } else {
            MenuFrameState.s2++;
        }
        s7 = (s0 * 0x80) / 20;
        break;
    }

    pc = UiTitleMaskVramRect;
    sp40 = (((sp38 + 1) & 3) << 5) | 0x1E;
    a1 = AddDrawAreaPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, RR_PAL_JP(0x200, 0x1E0));
    a1 = AddSpriteWithDrawMode(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, 0x10, 0x24, 0x20, 0x10, 0, 0x20, sp40, 0x7F38, 1, (unsigned short)s7);
    s1 = 0;
    s4 = 0x30;
    s5 = 0x24;
    s7++;

    c = BADGE_TITLE_GLYPH_INDICES[s6][0];
    while (c != -1) {
        r = &RR_PAL_JP(BadgeTitleGlyphUvs, BadgeTitleGlyphUvsJp)[c];
        a1 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, s4, s5, r, 0x7F38, ((unsigned short)s7 << 1) | 1);
        c = BADGE_TITLE_GLYPH_INDICES[s6][++s1];
        { int t = s4 + 2; s4 = t + r->w; }
    }

    if (MenuFrameState.titlePlayer08 > 0) {
        a1 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, s4, s5, &BadgeTitleColonUv, 0x7F38, ((unsigned short)s7 << 1) | 1);
        BadgeTitleDigitUv.u = MenuFrameState.titlePlayer08 * 8;
        { int t = s4 + 2; s4 = t + BadgeTitleColonUv.w; }
        /* The original passes an extra trailing argument; the callee reads flags = 1. */
        a1 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, s4, s5, &BadgeTitleDigitUv, 0x7F38, 1);
    }

    a1 = AddTileSemiTransPrim(
        &CurrentDrawBuf->buffer.ordering70.tags[0], a1, 8, 0x38, 0x130, 2,
        *pc, *pc, *pc
    );
    a1 = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, sp40);
    a1 = AddDrawAreaPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, MenuFrameState.s10, MenuFrameState.s12, MenuFrameState.s14, MenuFrameState.s16);

    for (j = RR_PAL_JP(2, 0); j < 7; j++) {
        s4 = BadgeTitleColumnX[j];
        s5 = (j >= 2) ? RR_PAL_JP(0x1BE, 0x1B6) : RR_PAL_JP(0x1AA, 0x1A2);
        sp30 = RR_PAL_JP(BadgeTitleGlyphUvs, BadgeTitleGlyphUvsJp)[j];
        if ((unsigned int)(j - 3) < 2) {
            s2 = (PlayerControllers[ActivePlayerIndex & 1].protocol10 == 1);
            if (s2 == 1) {
                sp30.u += 0x14;
                if (j == 4) {
                    s4 -= 4;
                } else {
                    s4 -= 2;
                }
            }
        } else if (j == 5) {
            if (MenuFrameState.bC != 1) {
                sp30.w -= 0x14;
            }
        } else if (j == 6) {
            if (MenuFrameState.bD != 1) {
                sp30.w -= 0x1E;
            }
        }
        a1 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, s4, s5, &sp30, 0x7F38, ((unsigned short)s7 << 1) | 1);
    }

    if (MenuFrameState.sE == 0) {
        s2 = 0x32;
    } else {
        s2 = 0x31;
    }
    if (ScreenFadeState.phase == 2) {
        j = sp38 + 0x7DBA;
        a1 = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, BadgeTitleBarX[7], y, 1, 0xC, 0xF0, 0x7A, 0x14);
        a1 = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, BadgeTitleBarX[7] + 8, s5, &RR_PAL_JP(BadgeTitleGlyphUvs, BadgeTitleGlyphUvsJp)[s2], j, 0);
    }

    col2 = *pc;
    a1 = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, sp40);
    {
        int w;
        for (j = RR_PAL_JP(3, 0); j < 7; j++) {
            a1 = AddTileSemiTransPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, BadgeTitleBarX[j], (j >= 3) ? y : RR_PAL_JP(0x1A8, 0x1A0), 1, 0xC, col2, col2, col2);
        }
        w = 0x130;
        col3 = *pc;
        a1 = AddTileSemiTransPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, 8, RR_PAL_JP(0x1B8, 0x1B0), w, 2, col3, col3, col3);
        if (MenuFrameState.bD != 1) {
            a1 = AddTileSemiTransPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, 0xEE, RR_PAL_JP(0x1C6, 0x1BE), 0x16, 2, col3, col3, col3);
        }
        if (MenuFrameState.bC != 1) {
            a1 = AddTileSemiTransPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, 0xA8, RR_PAL_JP(0x1C6, 0x1BE), 0x12, 2, col3, col3, col3);
        }
        a1 = AddDrawAreaPrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, MenuFrameState.s18, MenuFrameState.s1A, MenuFrameState.s1C, MenuFrameState.s1E);
        RR_GPU_SCRATCH->cursor = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], a1, sp40);
    }
}

void SetMenuFrameOptions(s32 a, s32 b, s32 c) { MenuFrameState.sE = a; MenuFrameState.bC = b; MenuFrameState.bD = c; }

void DrawArrowTrianglePair(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5) {
    s32 i, j, k, sel, sg, off;
    void *t0;
    s32 s1;    s8 *tb;
    Vec2s *p, *bp;
    char *ub;
    struct {
        Vec2s v[3];
        s32 pad;
        B8 u;
    } w;
    i = 0;
    bp = w.v;
    tb = BeamFadeStripHeights;
    t0 = RR_GPU_SCRATCH->cursor;
    w.u = *(B8 *)UiBeamFadePalettePair;
L0:
    j = 0;
L1:
    k = 0;
    sg = 1 - (i & 1) * 2;
    p = bp;
    off = j << 1;
    do {
        p->x = (tb[off] - a2) * sg + a0;
        p->y = tb[off + 1] + a1;
        off += 4;
        k++;
        p++;
    } while (k < 3);
    s1 = i + 1;
    sel = 0;
    if (a5 == s1) sel = 0 < j;
    j++;
    ub = (char *)&w.u;
    t0 = EmitPolyF3(&CurrentDrawBuf->buffer.ordering70.tags[0], t0, Vec2sWord(&w.v[0]), Vec2sWord(&w.v[1]), Vec2sWord(&w.v[2]), (const u8 *)ub + sel * 4, 0);
    if (j < 2) goto L1;
    i = s1;
    if (i < 2) goto L0;
    RR_GPU_SCRATCH->cursor = t0;
}


void DrawMenuStepArrows(s32 p0, s32 p1, s32 p2, s32 p3, s32 p4, s32 a5, s32 a6) {
    s32 i, k, g, sel, sg;
    void *t0;
    s32 one;    s32 a3; 
    s32 a4;    s32 a0, a1;
    s32 a2; 
    Vec2s *p, *bp;
    char *ub;
    s8 *tb, *nv;
    struct {
        Vec2s v[3];
        s32 pad;
        B8 u;
    } w;
    a0 = p0;
    a1 = p1;
    a2 = p2;
    a3 = p3;
    i = 0;
    bp = w.v;
    one = 1;
    a4 = p4;
    t0 = RR_GPU_SCRATCH->cursor;
    g = MenuStepDirection;
    nv = BeamFadeStripHeights;
    w.u = *(B8 *)UiBeamFadePalettePair;
L0:
    k = 0;
    sg = one - (i & 1) * 2;
    p = bp;
    tb = nv;
    do {
        p->x = (tb[0] - a2) * sg + a0;
        p->y = tb[1] + a1;
        tb += 4;
        k++;
        p++;
    } while (k < 3);
    if (a3 == one) g = 0;
    if (g == one && i == 0) {
        sel = 1;
    } else {
        sel = 0;
        if (g == 2) sel = i == 1;
    }
    sel ^= a3;
    if ((a4 != a5 || i != 0) && (a4 != a6 - 1 || i != one)) {
        ub = (char *)&w.u;
        t0 = EmitPolyF3(&CurrentDrawBuf->buffer.ordering70.tags[0], t0, Vec2sWord(&w.v[0]), Vec2sWord(&w.v[1]), Vec2sWord(&w.v[2]), (const u8 *)ub + (sel << 2), 0);
    }
    i++;
    if (i < 2) goto L0;
    RR_GPU_SCRATCH->cursor = t0;
    if (a3 == 1) {
        RR_GPU_SCRATCH->cursor = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], t0, a0, a1 - 0xC, a3, 0x18, 0xFF, 0xFF, 0xFF);
    }
}


void StepMenuValue(int a0, int a1, char *a2, int a3, int a4) {
    int s0 = 0;
    int a;
    unsigned short b;
    switch (a1) {
    case 2:
        s0 = *(short *)a2;
        a = (a0 >> 2) & 1;
        break;
    case 4:
        s0 = *(int *)a2;
        a = (a0 >> 2) & 1;
        break;
    case 1:
    case -1:
        s0 = *(unsigned char *)a2;
        a = (a0 >> 2) & 1;
        break;
    default:
        a = (a0 >> 2) & 1;
        break;
    }
    if ((a0 & 1) == 0) {
        b = PlayerControllers[a].edges08.fields.repeat;
        if ((b & 0x8000) == 0) {
            if ((b & 0x2000) == 0) {
                if (b & 0x1000) {
                    s0 -= 1;
                    goto L;
                }
                if (b & 0x4000) {
                    s0 += 1;
                    goto L;
                }
            }
        }
    } else {
        b = PlayerControllers[a].edges08.fields.repeat;
        if ((b & 0x1000) == 0) {
          if ((b & 0x4000) == 0) {
            if (b & 0x8000) {
                MenuStepDirection = 1;
                s0 -= 1;
                goto L;
            }
            if (b & 0x2000) {
                MenuStepDirection = 2;
                s0 += 1;
                goto L;
            }
          }
        }
        MenuStepDirection = 0;
    }
    goto write_value;
L:
    if ((a0 & 2) == 0) {
        if (s0 < a3) {
            if (s0 >= a4) {
                PlaySfx(0);
            } else {
                s0 = a4;
            }
        } else {
            s0 = a3 - 1;
        }
    } else {
        PlaySfx(0);
        if (s0 >= a3) {
            s0 = a4;
        } else if (s0 < a4) {
            s0 = a3 - 1;
        }
    }
write_value:
    switch (a1) {
    case 2: *(short *)a2 = s0; break;
    case 4: *(int *)a2 = s0; break;
    case 1:
    case -1: *(char *)a2 = s0; break;
    }
}

int FramesToMilliseconds(int a, int b)
{
    int q;
    if (VideoTimingNtsc) {
        /* NTSC release: 30 frames a second, 100/3 ms a frame. */
        q = a / 30;
        a -= q * 30;
        a = a * 100 / 3 + b;
        return q * 1000 + a;
    }
    q = a / 25;
    a -= q * 25;
    a = a * 40 + b;
    return q * 1000 + a;
}

void DrawRaceTime(s32 a0, s32 a1, s32 b, s32 a3, s32 off) {
    if (b >= 0) {
        int m = b / 1000;
        int r = m % 60;
        int t;
        m = m / 60;
        t = b % 1000;
        RaceTimeDigits[1] = m + 0x30;
        RaceTimeDigits[6] = t / 100 + 0x30;
        RaceTimeDigits[7] = t / 10 % 10 + 0x30;
        RaceTimeDigits[8] = t % 10 + 0x30;
        RaceTimeDigits[3] = r / 10 + 0x30;
        RaceTimeDigits[4] = r % 10 + 0x30;
    } else {
        RaceTimeDigits[1] = 0x3A;
        RaceTimeDigits[3] = 0x3A;
        RaceTimeDigits[4] = 0x3A;
        RaceTimeDigits[6] = 0x3A;
        RaceTimeDigits[7] = 0x3A;
        RaceTimeDigits[8] = 0x3A;
    }
    DrawDigitString12(a0, a1, RaceTimeDigits + off, a3);
}

void CopyStringClearTail(char *d, char *s)
{
    int i = 0;
    char *p;
    if (*s) {
        do {
            d[i] = *s++;
            i++;
        } while (*s);
    }
    while (*(p = d + i++)) {
        *p = 0;
    }
}

void CopyStringPadSpaces(char *d, unsigned char *s, int n) {
    int i = 0;
    if (*s != 0) do {
        unsigned char c = *s;
        s++;
        d[i] = c;
        i++;
    } while (*s != 0);
    if (i < n) do { d[i] = 0x20; i++; } while (i < n);
    d[i] = 0;
}

void FormatDecimalDigits(char *a, int b, int c) {
    int t;
    a[c] = b % 10 + 0x30;
    t = b / 10;
    switch (c) {
    case 1:
        a[0] = t + 0x30;
        return;
    case 2: {
        int h = b / 100;
        a[0] = h + 0x30;
        a[1] = t - h * 10 + 0x30;
        return;
    }
    case 3: {
        int k = b / 1000;
        int h = b / 100;
        a[0] = k + 0x30;
        a[1] = h - k * 10 + 0x30;
        a[2] = t - h * 10 + 0x30;
        return;
    }
    }
}

void FormatRaceTime(char *a, int b)
{
  int s;
  int t;
  int m;
  int r;
  if (b >= 0)
  {
    s = b / 1000;
    m = s / 60;
    r = s % 60;
    t = b % 1000;
    FormatDecimalDigits(a, m, 1);
    FormatDecimalDigits(a + 3, r, 1);
    FormatDecimalDigits(a + 6, t, 2);
    return;
  }
  a[8] = 0x2D;
  a[7] = 0x2D;
  a[6] = 0x2D;
  a[4] = 0x2D;
  a[3] = 0x2D;
  a[1] = 0x2D;
  a[0] = 0x2D;
}

void DrawLapTimeList(int a0) {
    RaceVehicle *p0;
    int dummy;
    int n;
    int s7;
    int i;
    int s3 = 0x7FF8; /* best-lap CLUT also when there are no laps */
    int v;
    int y;
    SpritePacket *p;

    s7 = PlayerCars[a0].control.dynamics94.steering.finish20;
    p0 = &PlayerCars[a0];
    v = NumLaps;
    s7 = p0->control.dynamics94.steering.finish20;
    n = NumLaps;
    if (n > 0) {
        i = 0;
        do {
            if (i == s7) {
                s3 = 0x7FF8;
            } else {
                s3 = 0x7FF8;
            }
            v = LapTimes[i];
            y = i * 10 + 0x2C;
            DrawRaceTime(WIDE_HUD_RIGHT(0xF0), y, v, s3, 0);
            p = (SpritePacket *)RR_GPU_SCRATCH->cursor;
            p->tag.length = 4;
            p->code = 0x64;
            p->x = (s16)WIDE_HUD_RIGHT(0xE7);
            p->y = y;
            p->u = i * 12 + 12;
            p->v = 0xA8;
            p->clut = s3;
            p->code |= 1;
            p->w = 0xC;
            p->h = 0xC;
            RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], p);
            p++;
            RR_GPU_SCRATCH->cursor = (u8 *)p;
            i++;
        } while (i < NumLaps);
    }
    DrawRaceTime(WIDE_HUD_RIGHT(0xF0), 0x1C, BestLapTime, s3, 0);
}

char *FormatFrameTimeMMSS(int a) {
    if (a >= 0) {
        int s = a / RR_PAL_NTSC(25, 30);
        int m = s / 60;
        int r = s % 60;
        TimeTextMMSS[0] = m / 10 + 0x30;
        TimeTextMMSS[1] = m % 10 + 0x30;
        TimeTextMMSS[3] = r / 10 + 0x30;
        TimeTextMMSS[4] = r % 10 + 0x30;
    } else {
        TimeTextMMSS[0] = 0x2D;
        TimeTextMMSS[1] = 0x2D;
        TimeTextMMSS[3] = 0x2D;
        TimeTextMMSS[4] = 0x2D;
    }
    return TimeTextMMSS;
}

void DrawTimeRemaining(int a)
{
    /* HUD sprite 5 carries the time-remaining row's Y. */
    SpritePacket *p = &CurrentDrawBuf->buffer.hudSprites[5];
    char *r = FormatFrameTimeMMSS(a);
    int i = a < RR_PAL_NTSC(0x5DC, 0x708); /* the last minute */
    int t = StepProgressBarWidths[i];
    int s = (NumLaps - 3) * 10 + 0x50;
    DrawDigitString8(&CurrentDrawBuf->buffer.ordering70.tags[0], WIDE_HUD_RIGHT(0x111), s, r, t);
    p->y = s;
}

void DrawTimeRemainingForPlayer(int a, int b)
{
    SpritePacket *p = &CurrentDrawBuf->buffer.hudSprites[b * 6 + 5];
    GpuDmaTag *q;
    char *r;
    int i;
    int t;
    if (b == 0) {
        q = &CurrentDrawBuf->buffer.ordering70.tags[0];
    } else {
        q = &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0];
    }
    r = FormatFrameTimeMMSS(a);
    i = a < RR_PAL_NTSC(0x5DC, 0x708);
    t = StepProgressBarWidths[i];
    DrawDigitString8(q, WIDE_HUD_RIGHT(0x111), p->y, r, t);
}

char *FormatMillisecondsTime(int t)
{
    int a, b, c;
    char *p;

    a = t / 1000;
    b = a / 60;
    c = a % 60;
    a = t % 1000;
    p = StepProgressBarColors;
    p[0] = b / 10 + '0';
    p[1] = b % 10 + '0';
    p[3] = c / 10 + '0';
    p[4] = c % 10 + '0';
    p[6] = a / 100 + '0';
    p[7] = a / 10 % 10 + '0';
    p[8] = a % 10 + '0';
    return p;
}

void ResetVehicleShake(RaceVehicle *vehicle) {
    vehicle->impactFlags1A0 = 0;
    vehicle->state1A4 = 0;
    vehicle->stateTimer1A5 = 0;
    vehicle->steerAngle1A2 = 0;
    vehicle->shakeActive19C = 0;
    vehicle->shakeTimer19E = 0;
    vehicle->shakeX198 = 0;
    vehicle->shakeY19A = 0;
    vehicle->bumpPitch1B4 = 0;
    vehicle->shakeRollOffset1B6 = 0;
    vehicle->shakePitch1B0 = 0;
    vehicle->shakeRoll1B2 = 0;
    vehicle->steeringLock1B8 = 0;
    vehicle->steerInput1BA = 0;
}

void StartCollisionShake(RaceVehicle *vehicle, s32 impactKind) {
    s16 reverseTrackHeading, perpendicularHeading, speed, strength;
    int scaledSine, panCosine, pan, shakeX, shakeY, volume;
    u16 boundedSpeed;
    vehicle->shakeActive19C = 1;
    reverseTrackHeading = 0x800 - vehicle->trackAngle1EC;
    perpendicularHeading = (reverseTrackHeading + (vehicle->lateralVelF4 < 0 ? -0x400 : 0x400)) & 0xFFF;
    if (impactKind < 3) {
        speed = vehicle->input;
        if (speed > 800) {
            boundedSpeed = 1800;
            if (speed < 1801) boundedSpeed = (u16)vehicle->input;
            scaledSine = (s16)boundedSpeed * rsin((s16)AngleDiffAbs(reverseTrackHeading, vehicle->heading1D4));
            strength = scaledSine / 65536 + 4;
        } else {
            strength = 12;
            if (speed < 240) strength = 30;
        }
        shakeX = (rsin(perpendicularHeading) * strength) >> 4;
        shakeY = (rcos(perpendicularHeading) * strength) >> 4;
        volume = 100;
        vehicle->shakeTimer19E = 30;
    } else {
        shakeX = (rsin(perpendicularHeading) * 8) >> 4;
        shakeY = (rcos(perpendicularHeading) * 8) >> 4;
        vehicle->shakeTimer19E = 15;
        volume = 50;
    }
    vehicle->shakeX198 = shakeX;
    vehicle->shakeY19A = shakeY;
    if (vehicle->flags1E4 & 0x10) {
        panCosine = rcos(AngleDiffAbs(reverseTrackHeading, vehicle->heading1D4)) >> 6;
        if (vehicle->lateralVelF4 < 0) panCosine = -panCosine;
        pan = panCosine + 0x40;
        if (vehicle == &PlayerCars[0] || vehicle == &PlayerCars[1]) {
            if (ReplayState != 2) PlaySfxVolPan(0x21, volume, (s16)pan, -1);
        }
    }
}

void SetVehicleShake(RaceVehicle *vehicle, s32 x, s32 y) {
    vehicle->shakeActive19C = 2;
    vehicle->shakeX198 = x;
    vehicle->shakeY19A = y;
    vehicle->shakeTimer19E = 15;
}

/* Initial values of the globals this module owns. */
unsigned short UiTitleMaskVramRect[16] = {0xC0};
char UiBeamFadePalettePair[68] = {8, 8, 8, 0, -1, -1, -1};
/* One table; entry 22 is also named BadgeTitleColonUv, 49/50 the help-bar
 * "Wait"/"Please" labels. */
R4 BadgeTitleGlyphUvs[51] = {{0x0, 0x70, 0x30, 0xC}, {0x34, 0x70, 0xBC, 0xC}, {0x0, 0x7C, 0x5C, 0x10}, {0x5C, 0x7C, 0xC, 0x10}, {0x68, 0x7C, 0xC, 0x10}, {0x8C, 0x7C, 0x30, 0x10}, {0xBC, 0x7C, 0x3A, 0x10}, {0xD0, 0x20, 0x1B, 0x10}, {0x20, 0x20, 0x2D, 0x10}, {0x50, 0x20, 0x14, 0x10}, {0x68, 0x20, 0x17, 0x10}, {0x80, 0x20, 0x18, 0x10}, {0x0, 0x30, 0x24, 0x10}, {0x98, 0x20, 0xC, 0x10}, {0xA8, 0x20, 0x11, 0x10}, {0xBC, 0x20, 0x11, 0x10}, {0xEC, 0x20, 0xF, 0x10}, {0x24, 0x30, 0x13, 0x10}, {0x48, 0x30, 0x11, 0x10}, {0x24, 0x30, 0x38, 0x10}, {0x5C, 0x30, 0x20, 0x10}, {0x7C, 0x30, 0x18, 0x10}, {0x94, 0x30, 0x17, 0x10}, {0xAC, 0x30, 0x1B, 0x10}, {0xC8, 0x30, 0x14, 0x10}, {0xDC, 0x30, 0x14, 0x10}, {0x0, 0x40, 0x10, 0x10}, {0x10, 0x40, 0x10, 0x10}, {0x20, 0x40, 0x14, 0x10}, {0x34, 0x40, 0x15, 0x10}, {0x50, 0x40, 0x1A, 0x10}, {0x6C, 0x40, 0xD, 0x10}, {0x7C, 0x40, 0x13, 0x10}, {0x90, 0x40, 0x34, 0x10}, {0xA4, 0x40, 0x1F, 0x10}, {0xC4, 0x40, 0x3A, 0x10}, {0x0, 0x50, 0x17, 0x10}, {0x18, 0x50, 0x3F, 0x10}, {0x5C, 0x50, 0x1A, 0x10}, {0x78, 0x50, 0x1A, 0x10}, {0x94, 0x50, 0x1F, 0x10}, {0xB4, 0x50, 0x29, 0x10}, {0x0, 0x60, 0x26, 0x10}, {0x28, 0x60, 0x2A, 0x10}, {0x54, 0x60, 0x19, 0x10}, {0x70, 0x60, 0x15, 0x10}, {0x88, 0x60, 0x1D, 0x10}, {0xE0, 0x50, 0x17, 0x10}, {0xF0, 0x60, 0xF, 0x10}, {0xA8, 0x60, 0x18, 0x10}, {0xD8, 0x60, 0x10, 0x10}};
signed char BadgeTitleGlyphIndices[37][4] = {{9, 11, -1, -1}, {10, 11, -1, -1}, {12, -1, -1, -1}, {44, -1, -1, -1}, {17, -1, -1, -1}, {18, -1, -1, -1}, {19, -1, -1, -1}, {8, -1, -1, -1}, {13, 14, -1, -1}, {7, 11, -1, -1}, {15, 13, -1, -1}, {7, 14, -1, -1}, {13, 11, 21, -1}, {13, 11, 23, -1}, {24, -1, -1, -1}, {26, 24, -1, -1}, {28, 11, -1, -1}, {30, -1, -1, -1}, {29, 11, -1, -1}, {29, 16, -1, -1}, {27, 11, -1, -1}, {34, -1, -1, -1}, {20, -1, -1, -1}, {31, 33, -1, -1}, {32, 33, -1, -1}, {46, -1, -1, -1}, {38, 35, -1, -1}, {36, 22, -1, -1}, {37, -1, -1, -1}, {38, 39, -1, -1}, {38, 39, -1, -1}, {38, 41, -1, -1}, {42, 43, -1, -1}, {47, 48, -1, -1}, {45, 24, -1, -1}, {13, 11, -1, -1}, {27, 11, -1, -1}};
/* The Japanese release's values (SLPS-01800). */
static const R4 BadgeTitleGlyphUvsJp[51] = {{0, 0x70, 0x30, 0xC}, {0x34, 0x70, 0xBC, 0xC}, {0, 0x7C, 0x5C, 0x10}, {0x5C, 0x7C, 0xC, 0x10}, {0x68, 0x7C, 0xC, 0x10}, {0x8C, 0x7C, 0x30, 0x10}, {0xBC, 0x7C, 0x3A, 0x10}, {0xD0, 0x20, 0x1B, 0x10}, {0x20, 0x20, 0x2D, 0x10}, {0x50, 0x20, 0x14, 0x10}, {0x68, 0x20, 0x17, 0x10}, {0x80, 0x20, 0x18, 0x10}, {0, 0x30, 0x24, 0x10}, {0x98, 0x20, 0xC, 0x10}, {0xA8, 0x20, 0x11, 0x10}, {0xBC, 0x20, 0x11, 0x10}, {0xEC, 0x20, 0xF, 0x10}, {0x24, 0x30, 0x13, 0x10}, {0x48, 0x30, 0x11, 0x10}, {0x24, 0x30, 0x38, 0x10}, {0x5C, 0x30, 0x20, 0x10}, {0x7C, 0x30, 0x18, 0x10}, {0x94, 0x30, 0x17, 0x10}, {0xAC, 0x30, 0x1B, 0x10}, {0xC8, 0x30, 0x14, 0x10}, {0xDC, 0x30, 0x14, 0x10}, {0, 0x40, 0x10, 0x10}, {0x10, 0x40, 0x10, 0x10}, {0x20, 0x40, 0x14, 0x10}, {0x34, 0x40, 0x1B, 0x10}, {0x50, 0x40, 0x1A, 0x10}, {0x6C, 0x40, 0xD, 0x10}, {0x7C, 0x40, 0x13, 0x10}, {0x90, 0x40, 0x34, 0x10}, {0xA4, 0x40, 0x1F, 0x10}, {0xC4, 0x40, 0x3A, 0x10}, {0, 0x50, 0x17, 0x10}, {0x18, 0x50, 0x42, 0x10}, {0x5C, 0x50, 0x1A, 0x10}, {0x78, 0x50, 0x1A, 0x10}, {0x94, 0x50, 0x1F, 0x10}, {0xB4, 0x50, 0x29, 0x10}, {0, 0x60, 0x26, 0x10}, {0x28, 0x60, 0x2A, 0x10}, {0x54, 0x60, 0x19, 0x10}, {0x70, 0x60, 0x15, 0x10}, {0x88, 0x60, 0x1D, 0x10}, {0xE0, 0x50, 0x17, 0x10}, {0xF0, 0x60, 0xF, 0x10}, {0xA8, 0x60, 0x18, 0x10}, {0xD8, 0x60, 0x10, 0x10}};
static const signed char BadgeTitleGlyphIndicesJp[37][4] = {{9, 0xB, -1, -1}, {0xA, 0xB, -1, -1}, {0xC, -1, -1, -1}, {0x2C, -1, -1, -1}, {0x11, -1, -1, -1}, {0x12, -1, -1, -1}, {0x13, -1, -1, -1}, {8, -1, -1, -1}, {0xD, 0xE, -1, -1}, {7, 0xB, -1, -1}, {0xF, 0xD, -1, -1}, {7, 0xE, -1, -1}, {0xD, 0xB, 0x15, -1}, {0xD, 0xB, 0x17, -1}, {0x18, -1, -1, -1}, {0x1A, 0x18, -1, -1}, {0x1C, 0xB, -1, -1}, {0x1E, -1, -1, -1}, {0x1D, 0xB, -1, -1}, {0x1D, 0x10, -1, -1}, {0x1B, 0xB, -1, -1}, {0x22, -1, -1, -1}, {0x14, -1, -1, -1}, {0x1F, 0x21, -1, -1}, {0x20, 0x21, -1, -1}, {0x2E, -1, -1, -1}, {0x23, -1, -1, -1}, {0x24, 0x16, -1, -1}, {0x25, -1, -1, -1}, {0x26, 0x27, -1, -1}, {0x28, 0x26, -1, -1}, {0x29, 0x26, -1, -1}, {0x2A, 0x2B, -1, -1}, {0x2F, 0x30, -1, -1}, {0x2D, 0x18, -1, -1}, {0xD, 0xB, -1, -1}, {0x1B, 0xB, -1, -1}};
B4 ScreenFadeColors[2] = {{0}, {.c = {0xFF, 0xBD}}};
short BadgeTitleColumnX[8] = {20, 82, 20, 128, 200, 138, 208};
short BadgeTitleBarX[8] = {12, 76, 308, 12, 120, 192, 308, 266};
R4 BadgeTitleDigitUv = {.w = 8, .h = 0x10};
s8 BeamFadeStripHeights[12] = {-6, 0, -10, 0, 0, -6, 0, -10, 0, 6, 0, 10};
char RaceTimeDigits[12] = {63, 48, 61, 48, 48, 62, 48, 48, 48};
char TimeTextMMSS[8] = {48, 48, 58, 48, 48, 59};
unsigned short StepProgressBarWidths[2] = {0x7FF8, 0x7EF9};
char StepProgressBarColors[12] = {48, 48, 59, 48, 48, 60, 48, 48, 48};
FrameStorage *CurrentDrawBuf;
ScreenFadeState0C ScreenFadeState;
MenuFrameState20 MenuFrameState;
