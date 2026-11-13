#include "rr/cpu_time.h"
#include "types.h"
#include "rr/frame_storage_types.h"
#include "rr/gpu_scratch_types.h"
#include "scratchpad.h"
#include "rr/ui.h"
#include "rr/ui_primitives.h"
#include "game/course/trktext.h"
#include "game/effect/ptinit.h"
#include "game/gfx/fadespr.h"
#include "rr/frame_environment.h"
#include "rr/bios_devices.h"
#include "rr/frame_loop.h"
#include "psyq/libc/rand.h"
#include "rr/scene_camera_state.h"

typedef GpuPolyFT4Packet PolyFT4;

extern unsigned char CameraModeBaseFovTable[];
extern GpuColorCommand ViewViewportClipRect;

void InitParticles(void) {
    SetupDrawDispEnvs(0, 0, 0, 3);
    VSyncThreshold = 0x80;
    ParticleSystem.unk00 = 0;
    PsyqBzero(&ParticleSystem, sizeof(ParticleSystem));
    ParticleSystem.brightness18 = 0x100;
}

int FindFreeParticle(void) {
    int i;
    for (i = 0; i < 24; i++) {
        if (ParticleSystem.entries2C[i].active00 == 0) return i;
    }
    return -1;
}

void SpawnParticlePair(void)
{
    UiParticle20 *s0;
    unsigned char *s1;
    unsigned char *tb;
    int v0, r1, r2, r3, r4, r5, r6, one;
    int s5, s2;
    v0 = FindFreeParticle();
    if (v0 >= 0) {
        s0 = &ParticleSystem.entries2C[v0];
        one = 1;
        s0->active00 = one;
        s0->state04 = 0;
        s0->timer08 = 0;
        r1 = PsyqRand();
        v0 = r1 % 240 - 0x28;
        s0->x0C = v0;
        s5 = (short)v0;
        r2 = PsyqRand();
        s0->spanX14 = 0x168 - (u16)s0->x0C;
        s0->y0E = r2 % 140 + 0xAA;
        r3 = PsyqRand();
        s2 = ((r3 & 3) + one) << one;
        s0->speed16 = s2;
        r4 = PsyqRand() % 2;
        tb = CameraModeBaseFovTable;
        s1 = tb + r4 * 4;
        s0->r10 = s1[0];
        s0->g11 = s1[1];
        s0->b12 = s1[2];
        s0->rot18 = 0;
        s0->velX1C = 0;
        s0->velY1E = 0;
        s0->scale1A = 0x100;
        v0 = FindFreeParticle();
        if (v0 >= 0) {
            s0 = &ParticleSystem.entries2C[v0];
            s0->active00 = one;
            s0->state04 = 0;
            s0->timer08 = 0;
            r5 = PsyqRand();
            s0->x0C = s5 + r5 % 60 - 0x1E;
            r6 = PsyqRand();
            s0->speed16 = s2;
            s0->spanX14 = 0x168 - (u16)s0->x0C;
            s0->y0E = r6 % 140 + 0xAA;
            s0->r10 = s1[0];
            s0->g11 = s1[1];
            s0->b12 = s1[2];
            s0->rot18 = 0;
            s0->velX1C = 0;
            s0->velY1E = 0;
            s0->scale1A = 0x100;
        }
    }
}

void UpdateParticle(int a0) {
    RR_CPU_CYCLES(RR_CYCLES_UPDATE_PARTICLE);

    UiParticle20 *e = &ParticleSystem.entries2C[a0];
    int st = e->state04;
    int t, d;
    switch (st) {
    case 0:
        if (e->velX1C < 0xC) {
            e->velX1C = e->velX1C + 1;
        }
        *(u16 *)&e->rot18 = *(u16 *)&e->rot18 + e->velX1C;
        d = e->spanX14 - e->rot18;
        if (e->velX1C * 3 < d) {
            break;
        }
        goto L15;
    case 1:
        if (e->velX1C > 0) {
            e->velX1C = e->velX1C - 2;
            *(u16 *)&e->rot18 = *(u16 *)&e->rot18 + e->velX1C;
        } else {
            e->state04 = 2;
            e->timer08 = 0x32;
        }
        break;
    case 2:
        e->timer08 = e->timer08 - 1;
        if (e->timer08 <= 0) {
    L15:
            e->state04 = e->state04 + 1;
        }
        break;
    case 3:
        t = *(u16 *)&e->scale1A;
        t -= 8;
        e->scale1A = t;
        if ((t << 16) <= 0) {
            e->active00 = 0;
        }
        break;
    }
    *(u16 *)&e->velY1E = *(u16 *)&e->velY1E + 1;
}


void DrawParticle(int a0) {
    RR_CPU_CYCLES(RR_CYCLES_DRAW_PARTICLE);

    GpuColorCommand c[4];
    GpuRectangleFields rect;
    UiParticle20 *e;
    PolyFT4 *p;
    int i, v;
    int s3, sp30, s7, s1;

    e = &ParticleSystem.entries2C[a0];
    c[0] = c[1] = c[2] = c[3] = *(GpuColorCommand *)&e->r10;
    for (i = 0; i < 4; i++) {
        v = (c[i].r * ParticleSystem.brightness18) / 256;
        v = (v * RaceContext.fade3E0.scale3E8) / 256;
        c[i].r = v;
        v = (c[i].g * ParticleSystem.brightness18) / 256;
        v = (v * RaceContext.fade3E0.scale3E8) / 256;
        c[i].g = v;
        v = (c[i].b * ParticleSystem.brightness18) / 256;
        v = (v * RaceContext.fade3E0.scale3E8) / 256;
        c[i].b = v;
    }
    c[1].r /= 2;
    c[1].g /= 2;
    c[1].b /= 2;
    c[2].r /= 4;
    c[2].g /= 4;
    c[2].b /= 4;
    c[3].r = c[2].r + c[2].r / 2;
    c[3].g = c[2].g + c[2].g / 2;
    c[3].b = c[2].b + c[2].b / 2;
    for (i = 0; i < 4; i++) {
        c[i].r = (c[i].r * e->scale1A) / 256;
        c[i].g = (c[i].g * e->scale1A) / 256;
        c[i].b = (c[i].b * e->scale1A) / 256;
    }

    p = (PolyFT4 *)RR_GPU_SCRATCH->cursor;
    if (e->velY1E >= 4) {
        p->tag.length = 9;
        p->code = 0x2E;
        p->r0 = c[3].r;
        p->g0 = c[3].g;
        p->b0 = c[3].b;
        p->x0 = e->x0C - 9;
        p->y0 = e->y0E - 0x18;
        p->x1 = e->x0C + 3;
        p->y1 = e->y0E - 0x18;
        p->x2 = e->x0C - 9;
        p->y2 = e->y0E + 0x18;
        p->x3 = e->x0C + 3;
        p->y3 = e->y0E + 0x18;
        p->u0 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x30;
        p->v0 = 0x70;
        p->u1 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x3C;
        p->v1 = 0x70;
        p->u2 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x30;
        p->v2 = 0xA0;
        p->u3 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x3C;
        p->v3 = 0xA0;
        p->tpage = 0xA5;
        p->clut = 0x7EB8;
        RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[70], p);
        p++;
    }

    p->tag.length = 9;
    p->code = 0x2E;
    p->r0 = c[2].r;
    p->g0 = c[2].g;
    p->b0 = c[2].b;
    p->x0 = e->x0C - 0x10;
    p->y0 = e->y0E - 0x18;
    p->x1 = e->x0C;
    p->y1 = e->y0E - 0x18;
    p->x2 = e->x0C - 0x10;
    p->y2 = e->y0E + 0x18;
    p->x3 = e->x0C;
    p->y3 = e->y0E + 0x18;
    p->u0 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x30;
    p->v0 = 0x70;
    p->u1 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x40;
    p->v1 = 0x70;
    p->u2 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x30;
    p->v2 = 0xA0;
    p->u3 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x40;
    p->v3 = 0xA0;
    p->tpage = 0xA5;
    p->clut = 0x7EB8;
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[70], p);
    p++;

    if (e->velY1E >= 5) {
        p->tag.length = 9;
        p->code = 0x2E;
        p->r0 = c[0].r;
        p->g0 = c[0].g;
        p->b0 = c[0].b;
        p->x0 = e->x0C + e->rot18 - 0xC;
        p->y0 = e->y0E - 0x18;
        p->x1 = e->x0C + e->rot18 + 4;
        p->y1 = e->y0E - 0x18;
        p->x2 = e->x0C + e->rot18 - 0xC;
        p->y2 = e->y0E + 0x18;
        p->x3 = e->x0C + e->rot18 + 4;
        p->y3 = e->y0E + 0x18;
        p->u0 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x30;
        p->v0 = 0x70;
        p->u1 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x40;
        p->v1 = 0x70;
        p->u2 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x30;
        p->v2 = 0xA0;
        p->u3 = ((s16)(*(u16 *)&e->speed16) / 2) * 16 + 0x40;
        p->v3 = 0xA0;
        p->tpage = 0xA5;
        p->clut = 0x7EB8;
        RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[70], p);
        p++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)p;

    s3 = 0x18;
    if (e->rot18 < 0x18) {
        s3 = e->rot18;
    }
    sp30 = e->rot18 - s3;
    s7 = e->x0C + 3;
    if (e->velY1E >= 5) {
        s1 = s7 + sp30;
        rect.x = s1;
        rect.y = e->y0E - (s16)(*(u16 *)&e->speed16) / 2 - 0x10;
        rect.w = s3;
        rect.h = 0x10;
        AddSlantedGradientRect((unsigned short *)&rect, 0, (unsigned char *)&ViewViewportClipRect, (unsigned char *)&c[2], 0x41);

        rect.x = s1;
        rect.y = e->y0E - (s16)(*(u16 *)&e->speed16) / 2;
        rect.w = s3;
        rect.h = (s16)(*(u16 *)&e->speed16) / 2;
        AddGouraudRect(&rect, &c[1], &c[2], &c[0], &c[1], 0x41);

        rect.x = s1;
        rect.y = e->y0E;
        rect.w = s3;
        rect.h = (s16)(*(u16 *)&e->speed16) / 2;
        AddGouraudRect(&rect, &c[0], &c[1], &c[1], &c[2], 0x41);

        rect.x = s1;
        rect.y = e->y0E + (s16)(*(u16 *)&e->speed16) / 2;
        rect.w = s3;
        rect.h = 0x10;
        AddSlantedGradientRect((unsigned short *)&rect, 1, (unsigned char *)&c[2], (unsigned char *)&ViewViewportClipRect, 0x41);

        if (sp30 > 0) {
            rect.x = s7;
            rect.y = e->y0E - (s16)(*(u16 *)&e->speed16) / 2 - 0x10;
            rect.w = e->rot18 - s3;
            rect.h = 0x10;
            AddVerticalGradientRect(&rect, &ViewViewportClipRect, &c[2], 0x41);

            rect.x = s7;
            rect.y = e->y0E;
            rect.w = e->rot18 - s3;
            rect.h = (s16)(*(u16 *)&e->speed16) / 2;
            AddVerticalGradientRect(&rect, &c[0], &c[1], 0x41);

            rect.x = s7;
            rect.y = e->y0E - (s16)(*(u16 *)&e->speed16) / 2;
            rect.w = e->rot18 - s3;
            rect.h = (s16)(*(u16 *)&e->speed16) / 2;
            AddVerticalGradientRect(&rect, &c[1], &c[0], 0x41);

            rect.x = s7;
            rect.y = e->y0E + (s16)(*(u16 *)&e->speed16) / 2;
            rect.w = e->rot18 - s3;
            rect.h = 0x10;
            AddVerticalGradientRect(&rect, &c[2], &ViewViewportClipRect, 0x41);
        }
    }
}

/* Initial values of the globals this module owns. */
GpuColorCommand ViewViewportClipRect;
unsigned char CameraModeBaseFovTable[16] = {0xF0, 0, 0, 0, 0xFF, 0xF0, 0xB4, 0, 0xFF, 0xA0, 0x3C, 0, 0x78, 0xFF, 0xB4};
