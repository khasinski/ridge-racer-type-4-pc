#include "rr/release.h"
#include "game/system/pad.h"
#include "game/gfx/gtepoly1.h"
#include "game/math/rotmat.h"
#include "psyq/libgte/gtemat5.h"
#include "psyq/libgte/gtemat4.h"
#include "psyq/libgte/gtemat3.h"
#include "psyq/libgte/gtemat.h"
#include "rr/matrix_workspace.h"
#include "types.h"
#include "rr/scene_camera_state.h"
#include "game/car/bounds.h"
#include "rr/player_cars.h"
#include "rr/camera_curve_tables.h"
#include "scratchpad.h"
#include "rr/matrix_words.h"
#include "psyq/libgpu/gpudisp.h"
#include "psyq/libgte/gtemat7.h"
#include "game/menu/menutran.h"
#include "game/gfx/scene1.h"
#include "rr/hud_number.h"
#include "rr/bios_devices.h"
#include "game/gfx/gfxotag.h"
#include "rr/frame_storage_types.h"
#include "rr/gpu_scratch_types.h"
#include "rr/gpu.h"
#include "game/data/globals.h"
#include "rr/textured_panel.h"
#include "rr/widescreen.h"
#include <math.h>
#include <psyz/video.h>
static const GpuRectangleFields CarCameraSpringStiffnessJp[4];
static const GpuRectangleFields CarCameraPitchOffsetsJp[2][4];






extern GpuRectangleFields CarCameraSpringStiffness[];
extern GpuRectangleFields CarCameraPitchOffsets[][4];
extern int ReplayCamBankWeights[];
extern int DrawSubOtFlag;
extern short SelectedMakerIndex;
/* PsyZ libgte ScaleMatrix(MATRIX *, VECTOR *). */
extern MatrixWords32 *ScaleMatrix(MatrixWords32 *matrix, VectorWords16 *scale);

void EmitSceneOverlaySprites(s32 a0, s32 a1, u8 *a2, s32 a3, s32 a4);

void InitSceneOverlayState(void) {
    RaceContext.overlayBlendE0 = 1;
    RaceContext.overlayEnabledD4 = 0;
    RaceContext.overlayStyleD8 = 0;
    RaceContext.overlayModeDC = 0;
    RaceContext.overlayColorE4[0] = 0x90;
    RaceContext.overlayColorE4[1] = 0x90;
    RaceContext.overlayColorE4[2] = 0x90;
}

void SetSceneOverlayColor(s32 mode, s32 red, s32 green, s32 blue) {
    RaceContext.overlayModeDC = mode;
    RaceContext.overlayColorE4[0] = red;
    RaceContext.overlayColorE4[1] = green;
    RaceContext.overlayColorE4[2] = blue;
}

void SetSceneOverlayEnabled(s32 enabled) {
    RaceContext.overlayEnabledD4 = enabled;
}

/* Widescreen: PsyZ keeps the display buffers squeezed (rr/widescreen.h),
 * so the copy of the other buffer is drawn across the whole squeezed buffer
 * instead: a textured quad from where buffer column first lands to where
 * column first + w does (the column's distance from the centre over the
 * squeeze, to the nearest pixel) reads every column into itself. */
static s16 WideBufferColumnX(s32 column) {
    const float squeeze = Psyz_VideoGetWidescreenSqueeze();

    return (s16)(0xA0 + (s32)lroundf((float)(column - 0xA0) / squeeze));
}

static u8 *EmitWideSceneOverlayQuad(u8 *cursor, const SpritePacket *sprite, u16 tpage) {
    PolyFT4Packet *q = (PolyFT4Packet *)cursor;
    s16 x0 = WideBufferColumnX(sprite->x), x1 = WideBufferColumnX(sprite->x + sprite->w);
    u8 u0 = sprite->u, u1 = (u8)(sprite->u + sprite->w - 1);
    u8 v0 = sprite->v, v1 = (u8)(sprite->v + sprite->h - 1);

    q->r = sprite->r;
    q->g = sprite->g;
    q->b = sprite->b;
    q->code = (u8)(0x2C | (sprite->code & 3));
    q->x0 = x0; q->y0 = sprite->y; q->u0 = u0; q->v0 = v0;
    q->x1 = x1; q->y1 = sprite->y; q->u1 = u1; q->v1 = v0;
    q->x2 = x0; q->y2 = (s16)(sprite->y + sprite->h); q->u2 = u0; q->v2 = v1;
    q->x3 = x1; q->y3 = (s16)(sprite->y + sprite->h); q->u3 = u1; q->v3 = v1;
    q->clut = sprite->clut;
    q->tpage = tpage;
    q->tag.length = (sizeof(PolyFT4Packet) - sizeof(GpuDmaTag)) / 4;
    return (u8 *)(q + 1);
}

void EmitSceneOverlaySprites(s32 a0, s32 a1, u8 *a2, s32 a3, s32 a4) {
    s32 i;
    const GpuRectangleFields *uv;
    SpritePacket *s2;
    GpuCommandPacket *page;
    GpuDrawModePacket *s0;
    GpuDmaTag *ordering;
    s16 y, x;

    ordering = &CurrentDrawBuf->buffer.ordering70.tags[0];
    for (i = 0; i < 4; i++) {
        s2 = (SpritePacket *)RR_GPU_SCRATCH->cursor;
        s2->tag.length = 4;
        s2->code = 0x64;
        if (a4 != 0) {
            s2->code |= 2;
        } else {
            s2->code &= ~2;
        }
        if (a1 != 0) {
            s2->code |= 1;
        } else {
            s2->code &= ~1;
        }
        s2->r = a2[0];
        s2->g = a2[1];
        s2->b = a2[2];
        s2->x = RR_PAL_JP(CarCameraSpringStiffness, CarCameraSpringStiffnessJp)[i].x;
        s2->y = RR_PAL_JP(CarCameraSpringStiffness, CarCameraSpringStiffnessJp)[i].y;
        s2->w = RR_PAL_JP(CarCameraSpringStiffness, CarCameraSpringStiffnessJp)[i].w;
        s2->h = RR_PAL_JP(CarCameraSpringStiffness, CarCameraSpringStiffnessJp)[i].h;
        uv = RR_RELEASE_JP ? &CarCameraPitchOffsetsJp[a0][i] : &CarCameraPitchOffsets[a0][i];
        s2->u = uv->x;
        s2->v = uv->y;
        y = uv->h;
        x = uv->w;
        if (s2->w > 0 && s2->h > 0 && WideFrameSqueezed()) {
            u8 *quad = (u8 *)(s2 + 1);
            u8 *end = EmitWideSceneOverlayQuad(quad, s2,
                (u16)(((a3 & 3) << 5) | 0x100 | ((s16)((y & 0x100) >> 4)) | ((x & 0x3FF) >> 6) | ((s16)(y & 0x200) << 2)));
            RR_GPU_LINK(ordering, quad);
            page = (GpuCommandPacket *)end;
        } else {
            RR_GPU_LINK(ordering, s2);
            page = (GpuCommandPacket *)(s2 + 1);
        }
        s0 = (GpuDrawModePacket *)(page + 1);
        page->tag.length = 1;
        page->command = 0xE1000200 | (((a3 & 3) << 5) | 0x100 | ((s16)((y & 0x100) >> 4)) | ((x & 0x3FF) >> 6) | ((s16)(y & 0x200) << 2));
        RR_GPU_LINK(ordering, page);
        RR_GPU_SCRATCH->cursor = (u8 *)s0;
        SetDrawStp(s0, 0);
        RR_GPU_LINK(ordering, s0);
        RR_GPU_SCRATCH->cursor = (u8 *)(s0 + 1);
    }
}

void QueueDrawStp(void)
{
    GpuDrawModePacket *packet = (GpuDrawModePacket *)RR_GPU_SCRATCH->cursor;
    SetDrawStp(packet, 1);
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[703], packet);
    packet++;
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
}

void DrawSceneOverlayIfActive(void) {
    if (RaceContext.overlayEnabledD4 != 0) {
        EmitSceneOverlaySprites(DrawBufIndex, RaceContext.overlayStyleD8, &RaceContext.overlayColorE4[0], RaceContext.overlayModeDC, RaceContext.overlayBlendE0);
    }
}

void UpdateAndDrawSceneOverlay(void) {
    if (Pad0Buttons & 0x10) {
        RaceContext.overlayToggle3DC ^= 1;
        SetSceneOverlayEnabled(RaceContext.overlayToggle3DC);
    }
    DrawSceneOverlayIfActive();
}

void InitCameraState(int mode, int flags)
{
    int distance;
    u32 bit;
    SceneCameraState160 *camera;
    RaceContext.camera24C = &RaceContext.cameraEC;
    PsyqBzero(RaceContext.camera24C, sizeof(SceneCameraState160));
    RaceContext.camera24C->lighting78.matrix.fields.vectors[0].x = 0x1C1;
    RaceContext.camera24C->lighting78.matrix.fields.vectors[0].y = -0xDF8;
    RaceContext.camera24C->lighting78.matrix.fields.vectors[0].z = -0x79A;
    RaceContext.camera24C->lighting78.matrix.fields.colors[0].red = 0x80;
    RaceContext.camera24C->lighting78.matrix.fields.colors[0].green = 0x80;
    RaceContext.camera24C->lighting78.matrix.fields.colors[0].blue = 0x80;
    RaceContext.camera24C->lighting78.matrix.fields.vectors[1].x = 0xB9A;
    RaceContext.camera24C->lighting78.matrix.fields.vectors[1].y = -0xADB;
    RaceContext.camera24C->lighting78.matrix.fields.vectors[1].z = 0x1E2;
    RaceContext.camera24C->lighting78.matrix.fields.colors[1].red = 0x40;
    RaceContext.camera24C->lighting78.matrix.fields.colors[1].green = 0x40;
    RaceContext.camera24C->lighting78.matrix.fields.colors[1].blue = 0x40;
    RaceContext.camera24C->lighting78.matrix.fields.vectors[2].x = -0xA4D;
    RaceContext.camera24C->lighting78.matrix.fields.vectors[2].y = -0x642;
    RaceContext.camera24C->lighting78.matrix.fields.vectors[2].z = 0xA86;
    RaceContext.camera24C->lighting78.matrix.fields.colors[2].red = 0x20;
    RaceContext.camera24C->lighting78.matrix.fields.colors[2].green = 0x20;
    RaceContext.camera24C->lighting78.matrix.fields.colors[2].blue = 0x20;
    RaceContext.camera24C->lighting78.ambient[0] = 0x40;
    RaceContext.camera24C->lighting78.ambient[1] = 0x40;
    RaceContext.camera24C->lighting78.ambient[2] = 0x40;
    RaceContext.camera24C->position110.xyz.x = 0;
    RaceContext.camera24C->position110.xyz.y = -0xF0;
    RaceContext.camera24C->position110.xyz.z = 0;
    RaceContext.camera24C->rotation120.fields.x = 0;
    RaceContext.camera24C->rotation120.fields.y = 0;
    RaceContext.camera24C->rotation120.fields.z = 0;
    RaceContext.camera24C->value12C = 0x800;
    RaceContext.camera24C->value130 = 0x1B8;
    RaceContext.camera24C->parts18[0].position00.xyz.x = -0xC0;
    RaceContext.camera24C->parts18[1].position00.xyz.x = 0xC0;
    if (!(ReplayCamBankWeights[SelectedMakerIndex] & ((bit = 1) << SelectedMakerCarIndex))) {
        RaceContext.camera24C->extentA0 = 0xA9;
        RaceContext.camera24C->mirrorA4 = 0;
    } else {
        RaceContext.camera24C->extentA0 = 0xCA;
        RaceContext.camera24C->mirrorA4 = 1;
    }
    camera = RaceContext.camera24C;
    ShowcaseCamDepthCurve[0].a = camera->extentA0 * 4;
    distance = camera->extentA0 - 4;
    distance = distance - (CarBoundsTable[0]->unk08 + (PlayerCars[0].halfSpan10E / 2));
    camera->distance100 = distance;
    RaceContext.camera24C->mode13C = mode;
    RaceContext.camera24C->mode140 = flags;
    SetModelTexturePage(0x19);
    DrawSubOtFlag = 0;
}

void SetupObjectLightMatrices(const SceneCameraPose18 *pose, const PreviewLighting40 *lighting, MatrixWords32 *light, MatrixWords32 *rotation) {
    MatrixWords32 basis, temporary, axis;
    MatrixWords32 color;
    VectorWords16 scale;
    int channel;
    scale = UnitScaleVector;
    BuildRotMatrixY(&axis, -pose->rotation10.y);
    MulMatrix2((const MatrixWords32 *)(SCRATCH_ROT_MATRIX), &axis);
    BuildRotMatrixX(&basis, -pose->rotation10.x);
    BuildRotMatrixZ(&temporary, -pose->rotation10.z);
    MulMatrix(&basis, &temporary);
    MulMatrix2(&axis, &basis);
    BuildRotMatrixY(&axis, pose->rotation10.y);
    BuildRotMatrixX(&temporary, pose->rotation10.x);
    MulMatrix(&axis, &temporary);
    BuildRotMatrixZ(&temporary, pose->rotation10.z);
    MulMatrix2(&axis, &temporary);
    *rotation = temporary;
    channel = 0;
    do {
        LightMatrix.matrix.fields.rotation00[channel][0] = lighting->matrix.fields.vectors[channel].x;
        LightMatrix.matrix.fields.rotation00[channel][1] = lighting->matrix.fields.vectors[channel].y;
        LightMatrix.matrix.fields.rotation00[channel][2] = lighting->matrix.fields.vectors[channel].z;
        color.fields.rotation00[0][channel] = lighting->matrix.fields.colors[channel].red * 16;
        color.fields.rotation00[1][channel] = lighting->matrix.fields.colors[channel].green * 16;
        color.fields.rotation00[2][channel] = lighting->matrix.fields.colors[channel].blue * 16;
        channel++;
    } while (channel < 3);
    MulMatrix0(&LightMatrix.matrix, &basis, &temporary);
    SetLightMatrix(&temporary);
    SetColorMatrix(&color);
    SetBackColor(lighting->ambient[0], lighting->ambient[1], lighting->ambient[2]);
    ScaleMatrix(&basis, &scale);
    *light = basis;
}

void TransformPoint(const MatrixWords32 *rotation, const Vec3i *origin,
                    const Vec3i *point, Vec3i *output,
                    MatrixWords32 *rotationWorkspace) {
    Vec3s8 local;
    /* The existing fifth call slot is unused by this implementation. */
    (void)rotationWorkspace;
    local.fields.x = (s16)point->x;
    local.fields.y = (s16)point->y;
    local.fields.z = (s16)point->z;
    ApplyMatrix(rotation, &local, output);
    /* Unsigned accumulation retains the target's word wrap. */
    {
        s32 transformed = output->x;
        u32 translation = origin->x;
        output->x = transformed + translation;
    }
    {
        s32 transformed = output->y;
        u32 translation = origin->y;
        output->y = transformed + translation;
    }
    {
        s32 transformed = output->z;
        u32 translation = origin->z;
        output->z = transformed + translation;
    }
}

/* Initial values of the globals this module owns. */
VectorWords16 UnitScaleVector = {.vector = {.x = 4096, .y = 4096, .z = 4096}};
GpuRectangleFields CarCameraSpringStiffness[4] = {{.w = 256}, {.w = 256, .h = 256}, {.x = 256, .w = 64}, {.x = 256, .w = 64, .h = 256}};
GpuRectangleFields CarCameraPitchOffsets[2][4] = {{{.y = 256}, {.h = 256}, {.y = 256, .w = 256}, {.w = 256, .h = 256}}, {{0}, {0}, {.w = 256}, {.w = 256}}};
/* The Japanese release's values (SLPS-01800). */
static const GpuRectangleFields CarCameraSpringStiffnessJp[4] = {{.w = 0x100, .h = 0x10}, {.y = 0x10, .w = 0x100, .h = 0xE0}, {.x = 0x100, .w = 0x40, .h = 0x10}, {.x = 0x100, .y = 0x10, .w = 0x40, .h = 0xE0}};
static const GpuRectangleFields CarCameraPitchOffsetsJp[2][4] = {{{.y = 0xF0}, {.h = 0x100}, {.y = 0xF0, .w = 0x100}, {.w = 0x100, .h = 0x100}}, {{}, {.y = 0x10}, {.w = 0x100}}};
int ReplayCamBankWeights[4] = {461824, 494864, 133120, 458752};
short SelectedMakerCarIndex;
