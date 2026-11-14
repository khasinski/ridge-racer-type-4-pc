#include "rr/release.h"
#include "rr/overlay668_track_project.h"
#include "rr/overlay668_camera.h"
#include "psyq/libgte/gtemat5.h"
#include "types.h"
#include "rr/ui_bar.h"
#include "game/ui/badgeprim.h"
#include "rr/frame_storage_types.h"
#include "psyq/libgpu/gpudisp.h"
#include "psyq/libgpu/gpuenv.h"
#include "rr/gpu_scratch_types.h"
#include "rr/gpu_address.h"
#include "rr/sprite_emit.h"
#include "scratchpad.h"

/* The initializer supplies its position pointer; this body uses globals. */
void ovl668_project_camera_to_track(s32 *unused_position) {
    TrackOffsetArgument offsets[2];
    s32 position[3];
    s32 height;
    s32 idx;

    (void)unused_position;

    position[0] = RaceContext.work444.epilogue.position450[0] >> 2;
    position[1] = RaceContext.work444.epilogue.position450[1] >> 2;
    position[2] = RaceContext.work444.epilogue.position450[2] >> 2;
    idx = FindTrackNode(position, RaceContext.work444.epilogue.node498, (TrackNodeCount + 1U));
    RaceContext.work444.epilogue.node498 = idx;
    offsets[0].value = (RaceContext.work444.epilogue.position450[0] / 4) - (u16)TrackNodes[idx].road.x;
    offsets[1].value = (RaceContext.work444.epilogue.position450[2] / 4) - (u16)TrackNodes[idx].road.z;
    ProjectToTrackSegment(offsets[0], offsets[1], RaceContext.work444.epilogue.node498, &RaceContext.work444.epilogue.fraction49C, &RaceContext.work444.epilogue.lateral49E, &height);
}

void ovl668_step_epilogue_camera_yaw(void) {
    s32 unused[4];
    s32 var_a3;
    s32 idx1;
    s32 idx2;
    u32 old_unk462;
    s16 new_unk462;
    s32 diff;
    u32 unk4A0;

    var_a3 = RaceContext.work444.epilogue.fraction49C;
    idx1 = RaceContext.work444.epilogue.node498 - ((3072 - var_a3) / 1024);
    idx2 = idx1 - 1;
    if (idx1 < 0) {
        idx1 += TrackNodeCount;
        idx2 = idx1 - 1;
    }
    if (idx2 < 0) {
        idx2 += TrackNodeCount;
    }
    InterpolateLanePointXYZ(&TrackNodes[idx1], &TrackNodes[idx2], 0, (1024 - var_a3) & 0x3FF, RaceContext.work444.epilogue.target468);
    RaceContext.work444.epilogue.delta478[0] = ((u32)RaceContext.work444.epilogue.target468[0] - (RaceContext.work444.epilogue.position450[0] / 4));
    RaceContext.work444.epilogue.delta478[1] = ((u32)RaceContext.work444.epilogue.target468[1] - (RaceContext.work444.epilogue.position450[1] / 4));
    RaceContext.work444.epilogue.delta478[2] = ((u32)RaceContext.work444.epilogue.target468[2] - (RaceContext.work444.epilogue.position450[2] / 4));
    old_unk462 = RaceContext.work444.epilogue.yaw462;
    new_unk462 = 1024 - Atan2Table(RaceContext.work444.epilogue.delta478[0], RaceContext.work444.epilogue.delta478[2]);
    old_unk462 = (new_unk462 - old_unk462) * 2;
    diff = old_unk462;
    old_unk462 &= 0xFFF;
    RaceContext.work444.epilogue.yaw462 = new_unk462;
    if (diff & 0x800) {
        old_unk462 |= ~0xFFF;
    }
    unk4A0 = RaceContext.work444.epilogue.velocity4A0;
    RaceContext.work444.epilogue.velocity4A0 = (unk4A0 + ((s32)((old_unk462 - unk4A0) * 15U) / 16));
    RaceContext.work444.epilogue.roll464 = (s32)((u32)RaceContext.work444.epilogue.velocity4A0 + RaceContext.work444.epilogue.roll464) / 2;
}

void ovl668_step_epilogue_camera_pitch(void) {
    s32 sp18[4];
    s32 sp28[4];
    s32 sp38[4];
    s32 unused[2];
    s32 idx;
    s32 next;
    u32 dx;
    u32 dz;

    idx = RaceContext.work444.epilogue.node498;
    next = idx + 1;
    if (next >= TrackNodeCount) {
        next = 0;
    }
    InterpolateLanePointXYZ(&TrackNodes[idx], &TrackNodes[next], RaceContext.work444.epilogue.lateral49E, RaceContext.work444.epilogue.fraction49C, sp18);
    next = RaceContext.work444.epilogue.node498;
    idx = next - 1;
    RaceContext.work444.epilogue.position450[1] = (sp18[1] - 120U);
    if (idx < 0) {
        idx += TrackNodeCount;
    }
    InterpolateLanePointXYZ(&TrackNodes[idx], &TrackNodes[next], RaceContext.work444.epilogue.lateral49E, RaceContext.work444.epilogue.fraction49C, sp28);
    dx = (((u32)sp28[0] - sp18[0]) << 2);
    sp38[0] = dx;
    dz = (((u32)sp28[2] - sp18[2]) << 2);
    sp38[2] = dz;
    sp38[1] = ((u32)sp28[1] - sp18[1]);
    next = 1024 - Atan2Table(sp38[1], CordicSqrt((dx * dx + dz * dz)) >> 6);
    RaceContext.work444.epilogue.pitch460 = next;
}

void ovl668_apply_epilogue_camera(void) {
    PreviewMatrixWorkspace sp10;
    PreviewVector8 sp30;
    s32 *scratch = RR_REPLAY_SCRATCH_WORDS;

    scratch[6] = RaceContext.work444.epilogue.pitch460;
    scratch[7] = RaceContext.work444.epilogue.yaw462;
    scratch[8] = RaceContext.work444.epilogue.roll464;
    scratch[2] = RaceContext.work444.epilogue.position450[0];
    scratch[3] = RaceContext.work444.epilogue.position450[1];
    scratch[4] = RaceContext.work444.epilogue.position450[2];
    sp30.x = scratch[6];
    sp30.y = scratch[7];
    sp30.z = scratch[8];
    RotMatrix(&sp30, &sp10.matrix);
    ApplyMatrixSV((const MatrixWords32 *)(&sp10.matrix), (const Vec3s8 *)(RaceContext.work444.epilogue.offset488), (s16 *)(&sp30));
    PlayerCars[0].position20.words[0] = (RaceContext.work444.epilogue.position450[0] / 4) + sp30.x;
    PlayerCars[0].position20.words[1] = (RaceContext.work444.epilogue.position450[1] / 4) + sp30.y;
    PlayerCars[0].position20.words[2] = (RaceContext.work444.epilogue.position450[2] / 4) + sp30.z;
    PlayerCars[0].rot50 = RaceContext.work444.epilogue.pitch460 + RaceContext.work444.epilogue.angle490;
    PlayerCars[0].rot54 = RaceContext.work444.epilogue.yaw462;
    PlayerCars[0].rot58 = RaceContext.work444.epilogue.roll464;
    BuildCameraMatrices();
}

s32 ovl668_calc_credit_line_fade(s16 arg0) {
    if (arg0 >= 157) {
        return 0;
    }
    if (arg0 >= 140) {
        return (156 - arg0) << 4;
    }
    if (arg0 < 48) {
        return 0;
    }
    if (arg0 < 65) {
        return (arg0 - 48) << 4;
    }
    return 256;
}

/* Textured gouraud quad (GPU command 0x3E, 12 payload words). */
typedef struct {
    GpuDmaTag tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    u8 r1, g1, b1, pad1;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    u8 r2, g2, b2, pad2;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad3;
    u8 r3, g3, b3, pad4;
    s16 x3, y3;
    u8 u3, v3;
    u16 pad5;
} StrCreditQuad;


extern s32 EpilogueCreditScrollY;
const s32 ovl668_credit_tpages[2] = {0x35, 0x36};
const s32 ovl668_credit_clut = 0x7F94;
s32 ovl668_calc_credit_line_fade(s16 arg0);

/* Drawing-environment packet with the PsyZ DR_ENV layout (host tag and
 * fifteen command cells) that SetDrawEnv fills. */
typedef struct {
    GpuDmaTag tag;
    uintptr_t code[15];
} StrDrawEnvPacket;

/* Draws one 8-pixel band of the scrolling credits texture. */
void ovl668_draw_credit_band(s32 line, s32 unused) {
    u8 color[4] = {0, 0, 255, 8};
    s32 page;
    u8 v;
    s32 y;
    s32 y2;
    s32 top;
    s32 bottom;
    StrCreditQuad *p;

    page = line / 31;
    v = (line % 31) * 8;
    y = EpilogueCreditScrollY / 100 + line * 8;
    color[1] = v;
    if ((y - 1) < 239U) {
        top = ovl668_calc_credit_line_fade(y) * 160 / 256;
        y2 = y + 8;
        bottom = ovl668_calc_credit_line_fade(y2) * 160 / 256;
        p = (StrCreditQuad *)RR_GPU_SCRATCH->cursor;
        p->code = 0x3E;
        p->x0 = 32;
        p->y0 = y;
        p->x1 = 287;
        p->y1 = y;
        p->x2 = 32;
        p->y2 = y2;
        p->x3 = 287;
        p->y3 = y2;
        p->u0 = 0;
        p->v0 = v;
        p->u1 = 255;
        p->v1 = v;
        p->u2 = 0;
        p->v2 = v + 8;
        p->u3 = 255;
        p->v3 = v + 8;
        p->r0 = top;
        p->g0 = top;
        p->b0 = top;
        p->r1 = top;
        p->g1 = top;
        p->b1 = top;
        p->r2 = bottom;
        p->g2 = bottom;
        p->b2 = bottom;
        p->r3 = bottom;
        p->g3 = bottom;
        p->b3 = bottom;
        p->clut = ovl668_credit_clut;
        p->tpage = ovl668_credit_tpages[page];
        RR_GPU_ADD_PRIM(&CurrentDrawBuf->buffer.ordering70.tags[0], p, 12);
        p++;
        RR_GPU_SCRATCH->cursor = (u8 *)p;
    }
}

extern s32 DrawBufIndex;

/* Queues the letterbox drawing-environment / fade packets for one pass. */
void ovl668_emit_letterbox_pass(s32 mode) {
    DrawEnvironment env;
    StrDrawEnvPacket *packet;
    GpuDmaTag *ot;
    GpuTilePacket *tile;

    switch (mode) {
    case 0:
        SetDefDrawEnv(&env, 0, RR_RELEASE_JP ? DrawBufIndex * 240U + 40 : (DrawBufIndex * 256U) | 40, 320, 160);
        packet = (StrDrawEnvPacket *)RR_GPU_SCRATCH->cursor;
        SetDrawEnv((void *)packet, &env);
        RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[703], packet);
        packet++;
        RR_GPU_SCRATCH->cursor = (u8 *)packet;
        break;
    case 1:
        SetDefDrawEnv(&env, 0, DrawBufIndex * (u32)RR_SCREEN_BOTTOM, 320, RR_SCREEN_BOTTOM);
        packet = (StrDrawEnvPacket *)RR_GPU_SCRATCH->cursor;
        SetDrawEnv((void *)packet, &env);
        RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], packet);
        packet++;
        RR_GPU_SCRATCH->cursor = (u8 *)packet;
        break;
    case 2:
        tile = (GpuTilePacket *)RR_GPU_SCRATCH->cursor;
        tile->code = 0x62;
        tile->y = 40;
        tile->w = 320;
        tile->h = 160;
        tile->r = 24;
        tile->g = 24;
        tile->b = 12;
        tile->x = 0;
        ot = &CurrentDrawBuf->buffer.ordering70.tags[0];
        RR_GPU_ADD_PRIM(ot, tile, 3);
        tile++;
        RR_GPU_SCRATCH->cursor = (u8 *)tile;
        RR_GPU_SCRATCH->cursor = AddDrawModePrim(ot, (GpuDrawModePacket *)tile, 37);
        break;
    case 3:
        SetDefDrawEnv(&env, 0, (DrawBufIndex * 256U) | 80, 320, 160);
        packet = (StrDrawEnvPacket *)RR_GPU_SCRATCH->cursor;
        SetDrawEnv((void *)packet, &env);
        RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], packet);
        packet++;
        RR_GPU_SCRATCH->cursor = (u8 *)packet;
        break;
    }
}

/* The Japanese release's credits are twelve pages of two sprites (248 and
 * 56 wide, 15 lines, from the 0x35/0x36 texture pages), shown in turn at
 * y 100 with a brightness, instead of the PAL release's scrolling bands. */
void ovl668_draw_credit_page(s32 page, s32 brightness) {
    UiUvRect rects[2] = {{0, 0, 248, 15}, {0, 0, 56, 15}};
    GpuCommandPacket *mode;
    s32 i, x = 8;

    for (i = 0; i < 2; i++, x += 248) {
        rects[i].v = page * 16;
        mode = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], RR_GPU_SCRATCH->cursor, x, 100,
                             &rects[i], 0x7014, (brightness << 1) | 1);
        RR_GPU_SCRATCH->cursor = (u8 *)mode;
        mode->command = (ovl668_credit_tpages[i] & 0x9FF) | 0xE1000200;
        RR_GPU_ADD_PRIM(&CurrentDrawBuf->buffer.ordering70.tags[0], mode, 1);
        RR_GPU_SCRATCH->cursor = (u8 *)(mode + 1);
    }
}
