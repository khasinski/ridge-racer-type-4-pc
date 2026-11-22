#include "psyq/libgte/gtemat5.h"
#include "psyq/libgte/gtemat4.h"
#include "psyq/libgte/gtemat3.h"
#include "types.h"
#include "scratchpad.h"
#include "psyq/libgte/libgte.h"
#include "rr_asm.h"
#include "psyq/libgte/gtemat7.h"
#include "game/gfx/polygt4.h"
#include "rr/menu_grid.h"
#include "rr/menu_overlay_runtime.h"
#include "game/math/rotmat.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "rr/projection_state.h"

/* POLY_GT4 with the host packet tag. */
typedef struct {
    GpuDmaTag tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    u8 r1, g1, b1, p1;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    u8 r2, g2, b2, p2;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad2;
    u8 r3, g3, b3, p3;
    s16 x3, y3;
    u8 u3, v3;
    u16 pad3;
} MenuGridPolyGT4;

void DrawMenuGridMesh(s32 arg0, GpuDmaTag *arg1, s32 arg2) {
    MatrixWords32 sp10;
    MatrixWords32 sp30;
    SVECTOR sp50;
    VECTOR sp58;
    s32 sp68;
    s32 sp6C;
    const ProjectionScratchPrefix *g = (const ProjectionScratchPrefix *)SCRATCHPAD;
    s32 t;
    s32 div_num;
    s32 count;
    s32 cols_total;
    s32 step_ang;
    s32 *sxy_buf;
    u8 *shade_buf;
    s32 s6;
    s32 s5, s2, s1;
    MenuGridPolyGT4 *pkt;

    if (arg2 != 0) {
        MenuGridRecords[arg0].w00++;
    }
    MenuGridRecords[arg0].w00 &= 0xFF;
    t = MenuGridRecords[arg0].w00;
    sp50.vx = MenuGridRecords[arg0].x0C - g->pose08.position00.vector.x;
    sp50.vy = 0x200 - g->pose08.position00.vector.y;
    sp50.vz = MenuGridRecords[arg0].z14 - g->pose08.position00.vector.z;
    ApplyMatrix((const MatrixWords32 *)((MatrixWords32 *)SCRATCH_ROT_MATRIX), (const Vec3s8 *)(&sp50), (Vec3i *)(&sp58));
    BuildRotMatrixX(&sp10, MenuGridRecords[arg0].rot_z1C);
    BuildRotMatrixZ(&sp30, MenuGridRecords[arg0].rot_x24);
    MulMatrix(&sp10, &sp30);
    BuildRotMatrixY(&sp30, MenuGridRecords[arg0].rot_y20);
    MulMatrix2(&sp30, &sp10);
    MulMatrix2((const MatrixWords32 *)SCRATCH_ROT_MATRIX, &sp10);
    sp10.fields.translation14[0] = sp58.vx;
    sp10.fields.translation14[1] = sp58.vy;
    sp10.fields.translation14[2] = sp58.vz;
    SetTransMatrix(sp10.w);
    SetRotMatrix(sp10.w);

    div_num = 0x600;
    count = MenuGridRecords[arg0].count2C;
    cols_total = MenuGridRecords[arg0].columns2E * count + 1;
    sxy_buf = (s32 *)(SCRATCHPAD + 0x7C);
    shade_buf = (u8 *)(SCRATCHPAD + 0x338);
    step_ang = div_num / count;
    s6 = 0;
    for (s5 = 0; s5 < MenuGridRecords[arg0].rows2F + 1; s5++) {
        sp50.vz = MenuGridRecords[arg0].step_y0A * ((MenuGridRecords[arg0].rows2F >> 1) - s5);
        for (s2 = 0; s2 < cols_total; s2++) {
            s32 s0_tmp;
            s1 = MenuGridRecords[arg0].height06;
            sp50.vx = MenuGridRecords[arg0].step_x08 * (s2 - (cols_total >> 1));
            s0_tmp = rsin(MenuGridRecords[arg0].width04 * t + s2 * step_ang);
            s1 = (s1 * (s0_tmp + rsin(MenuGridRecords[arg0].width04 * t + (s5 << 8)))) >> 12;
            sp50.vy = s1;
            RR_GTE_RTPS_LDV(&sp50);
            RR_GTE_SWC2_SXY2(&sxy_buf[s6]);
            RR_GTE_SWC2_IR0(&sp68);
            RR_GTE_STORE_FLAG(&sp68);
            RR_GTE_STORE_SZ3_SRA2(&sp6C);
            shade_buf[s6] = -s1 - (u8)sp6C;
            s6++;
        }
    }

    pkt = (MenuGridPolyGT4 *)RR_GPU_SCRATCH->cursor;
    for (s5 = 0; s5 < MenuGridRecords[arg0].columns2E * MenuGridRecords[arg0].rows2F * MenuGridRecords[arg0].count2C; s5++) {
        s32 v1;

        s1 = s5 / (cols_total - 1);
        s6 = s5 % MenuGridRecords[arg0].columns2E;
        pkt->code = 0x3C;
        s2 = s1 + s5;
        v1 = s2 + cols_total;
        RrSxyCopy(&pkt->x0, &sxy_buf[s2]);
        RrSxyCopy(&pkt->x1, &sxy_buf[s2 + 1]);
        RrSxyCopy(&pkt->x2, &sxy_buf[v1]);
        RrSxyCopy(&pkt->x3, &sxy_buf[v1 + 1]);

        pkt->u0 = pkt->u2 = MenuGridRecords[arg0].u30 + MenuGridRecords[arg0].du32 * s6;
        pkt->u1 = pkt->u3 = pkt->u0 + MenuGridRecords[arg0].du32;

        pkt->v0 = pkt->v1 = MenuGridRecords[arg0].v31 + MenuGridRecords[arg0].dv33 * s1;
        pkt->v2 = pkt->v3 = pkt->v0 + MenuGridRecords[arg0].dv33;

        pkt->tpage = MenuGridRecords[arg0].tpage34;
        pkt->clut = MenuGridRecords[arg0].clut36;

        s1 = shade_buf[s2];
        *(s32 *)&pkt->r0 = (s1 << 16) | (s1 << 8) | s1 | 0x3C000000;
        s1 = shade_buf[s2 + 1];
        *(s32 *)&pkt->r1 = (s1 << 16) | (s1 << 8) | s1;
        s1 = shade_buf[v1];
        *(s32 *)&pkt->r2 = (s1 << 16) | (s1 << 8) | s1;
        s1 = shade_buf[v1 + 1];
        *(s32 *)&pkt->r3 = (s1 << 16) | (s1 << 8) | s1;

        RR_GPU_ADD_PRIM(arg1, pkt, 0xC);
        pkt++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)pkt;
}

/* Initial values of the globals this module owns. */
MenuGrid38 *MenuGridRecords;
