#include "rr/release.h"
#include "rr/draw_distance.h"
#include "psyq/libgte/rotmatyxz.h"
#include "rr/menu_selection.h"
#include "psyq/libgte/gtemat5.h"
#include "rr/frame_storage_types.h"
#include "rr/camera_matrices.h"
#include "rr/matrix_workspace.h"
#include "game/car/car.h"
#include "types.h"
#include "scratchpad.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "game/camera/viewini.h"
#include "game/car/aipath.h"
#include "game/gfx/poly1.h"
#include "game/math/rotmat.h"
#include "psyq/libgte/gtemat3.h"
#include "psyq/libgte/gtemat7.h"
#include "psyq/libgte/gtetrig.h"
#include "rr/hud_number.h"
#include "game/data/globals.h"
#include "rr/object_transform_types.h"
#include "rr/projection_state.h"
#include "rr/rro_model.h"
#include "psyq/libgpu/gpudisp.h"
#include "game/camera/viewcam.h"
#include "rr/animated_glyph.h"
#include "rr/widescreen.h"
#include <psyz/video.h>
static GpuRectangleFields CourseSelectPreviewUvsJp[2][7];
static const s16 CourseSelectLabelPosYJp[8];
static const s16 CourseSelectMapCoordsYJp[8];
static const s16 ViewOffsetYJp[8];
/* PsyZ libgte ScaleMatrix(MATRIX *, VECTOR *); MatrixWords32 shares MATRIX. */
extern MatrixWords32 *ScaleMatrix(MatrixWords32 *matrix, s32 *scale);

extern void SubmitCourseHorizon(void *scratch, VisibleCourseViews *views, s32 course);
extern unsigned short CameraTargetTrackNode;
extern s16 ViewOffsetX[];
extern s16 ViewOffsetY[];
extern s32 ViewScreenH[];
extern u16 CourseSelectMapCoordsX[];
extern s16 CourseSelectMapCoordsY[];
extern u16 CourseSelectLabelPosX[];
extern s16 CourseSelectLabelPosY[];
extern void SetGeomOffset(s32, s32);
extern void SetGeomScreen(s32);
extern int ViewUsesCarTrackNode;
extern RaceVehicle PlayerCars[];

void SetupHorizonViewTransform(int *a0, int *a1);
void SetupViewMatrix(void);
void RenderPlayerCourseView(int player);

void SetupHorizonViewTransform(int *a0, int *a1) {
    int v[4], w[4], x[4];
    int *g = (int *)(SCRATCHPAD + 0x7C);
    if (a0) {
        BuildRotMatrixAngles(a0);
        v[0] = (0x8000 - a1[0]) << 2;
        v[1] = (0x60 - a1[1]) << 2;
        v[2] = (0x8000 - a1[2]) << 2;
        ApplyMatrixLV((const MatrixWords32 *)((SCRATCHPAD + 0x108)), (const Vec3i *)(v), (Vec3i *)(w));
        x[2] = 0x4000;
        x[1] = 0x4000;
        x[0] = 0x4000;
        g[0x28] = w[0];
        g[0x29] = w[1];
        g[0x2A] = w[2];
        ScaleMatrix((MatrixWords32 *)(SCRATCHPAD + 0x108), x);
        ResetViewScratchMask(ActiveVisibleViews, a0, a1);
    }
}

void SetupViewMatrix(void) {
    VectorWords16 v, w, x;
    VectorWords16 *position = &((ViewMatrixScratch48 *)SCRATCHPAD)->projection.pose08.position00;
    v.words[0] = 0x8000 - (position->words[0] >> 2);
    v.words[1] = -(position->words[1] >> 2);
    v.words[2] = 0x8000 - (position->words[2] >> 2);
    ApplyMatrixLV((const MatrixWords32 *)SCRATCH_ROT_MATRIX, &v.xyz, &w.xyz);
    CameraViewMatrix.w[5] = w.words[0];
    CameraViewMatrix.w[6] = w.words[1];
    CameraViewMatrix.w[7] = w.words[2];
    HalfTransViewMatrix = CameraViewMatrix;
    v.words[0] = (0x20000U - position->words[0]);
    v.words[1] = (0U - position->words[1]);
    v.words[2] = (0x20000U - position->words[2]);
    HalfTransViewMatrix.w[5] >>= 1;
    HalfTransViewMatrix.w[6] >>= 1;
    HalfTransViewMatrix.w[7] >>= 1;
    ApplyMatrixLV((const MatrixWords32 *)SCRATCH_ROT_MATRIX, &v.xyz, &w.xyz);
    (*(MatrixWords32 *)SCRATCH_ROT_MATRIX).w[5] = w.words[0];
    (*(MatrixWords32 *)SCRATCH_ROT_MATRIX).w[6] = w.words[1];
    (*(MatrixWords32 *)SCRATCH_ROT_MATRIX).w[7] = w.words[2];
    SetTransMatrix((s32 *)SCRATCH_ROT_MATRIX);
    x.words[2] = 0x4000;
    x.words[1] = 0x4000;
    x.words[0] = 0x4000;
    ScaleMatrix((MatrixWords32 *)SCRATCH_ROT_MATRIX, x.words);
    SetRotMatrix((s32 *)SCRATCH_ROT_MATRIX);
}


void ApplyCourseDrawDistance(void) {
    /* Cars can be drawn before the course. Both must use the same depth
     * mapping, including the first frame after changing the preference. */
    ((RroSceneScratchPrefix *)SCRATCHPAD)->depthShift60 = ExtendedDrawDistance ? 6 : 5;
}

void SetupViewLighting(void) {
    SetColorMatrix(&ColorMatrix);
    BuildLightMatrix();
    SetLightMatrix(&LightMatrix.matrix);
    SetBackColor(BackColorR, BackColorG, BackColorB);
    SetFarColor(FarColorR, FarColorG, FarColorB);
    ApplyCourseDrawDistance();
    SetFogNear(0x4E20, 0x122);
}


void SetupPlayerView(int player) {
    BuildViewSegmentObjectList(player);
    ApplySegmentVisibilityMask();
    SetupViewLighting();
    /* Lead car's angles (rot50..58) and position words. */
    SetupHorizonViewTransform(&PlayerCars[0].rot50, PlayerCars[0].position20.words);
    SetupViewMatrix();
    WideCourseView();
    SubmitCourseHorizon(RrScratchpad, ActiveVisibleViews, CourseIndex);
}

void RenderPlayerCourseView(int player) {
    BuildViewSegmentObjectList(player);
    UpdateVisibleViewRecords();
    ApplySegmentVisibilityMask();
    SetupViewLighting();
    SetupViewMatrix();
    WideCourseView();
    SubmitCourseHorizon(RrScratchpad, ActiveVisibleViews, CourseIndex);
}


void RenderMirrorCourseView(int player) {
    short *g = (short *)SCRATCHPAD;
    unsigned int v; int unused[12];
    CameraTargetTrackNode = (unsigned short)g[0x2E];
    g[0x2E] = -1;
    BuildViewSegmentObjectList(player);
    v = LookupOctantVisibility();
    if (!ExtendedDrawDistance && v < PrimaryVisibleViews.objects.count) {
        PrimaryVisibleViews.objects.count = v;
    }
    SetupViewLighting();
    SetupViewMatrix();
    SubmitCourseHorizon(g, ActiveVisibleViews, CourseIndex);
    g[0x2E] = (short)CameraTargetTrackNode;
}


void SetGteObjectTransform(ObjectTransform38 *output, const s16 *position, s32 *rotation) {
    ProjectionScratchPrefix *view = (ProjectionScratchPrefix *)SCRATCHPAD;
    output->fields.relative00.fields.x = position[0] - (s16)view->pose08.position00.words[0];
    output->fields.relative00.fields.y = position[2] - (s16)view->pose08.position00.words[1];
    output->fields.relative00.fields.z = position[4] - (s16)view->pose08.position00.words[2];
    ApplyMatrix((const MatrixWords32 *)SCRATCH_ROT_MATRIX, &output->fields.relative00, &output->fields.transformed08);
    output->fields.matrix18.fields.translation14[0] = output->fields.transformed08.x;
    output->fields.matrix18.fields.translation14[1] = output->fields.transformed08.y;
    output->fields.matrix18.fields.translation14[2] = output->fields.transformed08.z;
    SetRotMatrix(rotation);
    SetTransMatrix(output->fields.matrix18.w);
}

void BuildLightMatrix(void) {
    int i;
    MatrixWords32 rotation;
    Vec3s8 direction;
    VectorWords16 rotated;
    direction.fields.y = 0;
    direction.fields.x = 0;
    for (i = 0; i < 3; i++) {
        direction.fields.z = -LightSourceIntensities[i];
        RotMatrixYXZ(&LightSourceAngles[i], &rotation);
        SetRotMatrix(rotation.w);
        ApplyRotMatrix(&direction, &rotated.xyz);
        LightMatrix.matrix.fields.rotation00[i][0] = rotated.xyz.x;
        LightMatrix.matrix.fields.rotation00[i][1] = rotated.xyz.y;
        LightMatrix.matrix.fields.rotation00[i][2] = rotated.xyz.z;
    }
}


void SetupViewport(s32 viewport) {
    RroSceneScratchPrefix *scene = (RroSceneScratchPrefix *)SCRATCHPAD;
    u8 *packet;
    s32 isMirror;
    s32 coordinate;
    GpuRectangleFields *rectangle;
    s32 top;
    s32 height;

    packet = RR_GPU_SCRATCH->cursor;
    ActiveViewportIndex = viewport;
    isMirror = (viewport == 1);
    SetGeomOffset(ViewOffsetX[viewport], RR_PAL_JP(ViewOffsetY, ViewOffsetYJp)[viewport]);
    SetGeomScreen(ViewScreenH[viewport]);
    scene->viewport6C.rect.x = CourseSelectMapCoordsX[viewport];
    coordinate = RR_PAL_JP(CourseSelectMapCoordsY, CourseSelectMapCoordsYJp)[viewport];
    if (isMirror) {
        coordinate = (s32)((u32)coordinate + (u32)RearViewMirrorY);
    }
    scene->viewport6C.rect.y = coordinate;
    scene->viewport6C.rect.w = CourseSelectLabelPosX[viewport];
    coordinate = RR_PAL_JP(CourseSelectLabelPosY, CourseSelectLabelPosYJp)[viewport];
    if (isMirror) {
        coordinate = (s32)((u32)coordinate + (u32)RearViewMirrorY);
    }
    scene->viewport6C.rect.h = coordinate;
    rectangle = &RR_PAL_JP(CourseSelectPreviewUvs, CourseSelectPreviewUvsJp)[DrawBufIndex][viewport];
    if (!isMirror) {
        scene->renderFlags64 = 0;
    } else {
        scene->renderFlags64 = 1;
        top = RearViewMirrorY;
        if (top < 0) {
            top = 0;
        }
        rectangle->y = top;
        height = (u16)scene->viewport6C.rect.h;
        if ((s16)top < (s16)height) {
            top = height - top;
        } else {
            top = 0;
        }
        rectangle->h = top;
        coordinate = rectangle->y;
        if (DrawBufIndex != 0) {
            coordinate += RR_SCREEN_BOTTOM;
        }
        rectangle->y = coordinate;
    }
    if (viewport & 1) {
        RR_GPU_SCRATCH->order = &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0];
    } else {
        RR_GPU_SCRATCH->order = &CurrentDrawBuf->buffer.ordering70.tags[0];
    }
    WideSetupViewport(viewport);
    SetDrawArea(packet, rectangle);
    AddPrim(&RR_GPU_SCRATCH->order[703], packet);
    packet += sizeof(GpuDrawModePacket);
    RR_GPU_SCRATCH->cursor = packet;
}

/* Widescreen (rr/widescreen.h). The culling reaches past the sides of a
 * full-width scene viewport (the race view 0, the split screen halves 2 and
 * 3, and the 512-line menu view 6, whose frames stay 4:3); not the mirror
 * (1) or the half-width views (4, 5). The drawing area stays the screen's
 * width, which PsyZ keeps as the whole squeezed buffer. Like the viewport,
 * the margin lasts until the next SetupViewport (a time attack race sets
 * its view up only when paused); WideBeginFrame follows the option. */
static s32 WideViewThisFrame, WideViewLastFrame, WideSqueezedThisFrame;

s32 WideMargin(void) {
    return Psyz_VideoGetWidescreenBufferMargin();
}

static void WideUpdateMargin(void) {
    const GpuRectangleFields *rect = &((RroSceneScratchPrefix *)SCRATCHPAD)->viewport6C.rect;

    WideMarginX = rect->x == 0 && rect->w == 0x140 ? WideMargin() : 0;
}

void WideSetupViewport(s32 viewport) {
    (void)viewport;
    WideUpdateMargin();
}

void WideBeginFrame(void) {
    WideUpdateMargin();
}

void WideCourseView(void) {
    if (WideMarginX)
        WideViewThisFrame = 1;
}

s32 WideFrameSqueezed(void) {
    if (!Psyz_VideoGetWidescreen() || !(WideViewThisFrame || WideViewLastFrame))
        return 0;
    WideSqueezedThisFrame = 1;
    return 1;
}

/* As far as the 16:9 view reaches past the screen (67 pixels for 320x256
 * at the square pixels of the widescreen aspect), so edge elements keep
 * their original distance from the view's edges. */
s32 WideHudShift(s32 side) {
    s32 margin;

    if (!side || !WideFrameSqueezed())
        return 0;
    margin = Psyz_VideoGetWidescreenViewMargin();
    return side < 0 ? -margin : margin;
}

/* Packed GP0 words of one shape command: polygons, lines (a polyline to its
 * terminator) and rectangles; 0 for anything else. */
static s32 WideShapeWords(const u32 *w, s32 left) {
    u32 code = w[0] >> 24;

    if (code >= 0x20 && code < 0x40) {
        s32 n = (code & 8) ? 4 : 3;
        return 1 + n * (1 + ((code >> 2) & 1)) + ((code & 0x10) ? n - 1 : 0);
    }
    if (code >= 0x40 && code < 0x60) {
        s32 i = (code & 0x10) ? 4 : 3;
        if (!(code & 8))
            return i;
        for (i = 2; i < left && (w[i] & 0xF000F000u) != 0x50005000u; i++)
            ;
        return i + 1;
    }
    if (code >= 0x60 && code < 0x80)
        return 2 + ((code >> 2) & 1) + (((code >> 3) & 3) == 0);
    return 0;
}

/* A frame with nothing but untextured shapes that together span the
 * screen's width (a flash or fade between race and replay), or nothing at
 * all, in the display mode of the race frame before it: it stays 16:9
 * instead of flashing bars. */
static s32 WideBlankFrame(const GpuDmaTag *tag) {
    s32 minX = 0x7FFF, maxX = -0x8000, packets = 0, any = 0;

    while (tag->address != RR_GPU_END_LINK && ++packets < 0x10000) {
        tag = (const GpuDmaTag *)tag->address;
        if (tag->length == 0 || ((const u32 *)(tag + 1))[0] >> 24 < 0x20 ||
            ((const u32 *)(tag + 1))[0] >> 24 >= 0x80)
            continue; /* ordering entry or state packet (u_long words) */
        for (const u32 *w = (const u32 *)(tag + 1), *end = w + tag->length; w < end;) {
            s32 n = WideShapeWords(w, (s32)(end - w)), code = (s32)(w[0] >> 24), step;
            if (n == 0) {
                w++;
                continue;
            }
            if (code < 0x40 ? (code & 4) : (code >= 0x60 && (code & 4)))
                return 0;
            /* vertex words: every word after the colour whose low half is x */
            step = code < 0x60 && (code & 0x10) ? 2 : 1;
            for (s32 i = 1; i < n; i += step) {
                s32 x = (s16)(w[i] & 0xFFFF);
                if (code >= 0x40 && code < 0x60 && (w[i] & 0xF000F000u) == 0x50005000u)
                    break;
                minX = x < minX ? x : minX;
                maxX = x > maxX ? x : maxX;
                if (code >= 0x60) {
                    s32 size = (code >> 3) & 3;
                    s32 wide = size == 0 ? (s16)(w[2] & 0xFFFF) : size == 1 ? 1 : size == 2 ? 8 : 16;
                    maxX = x + wide > maxX ? x + wide : maxX;
                    break;
                }
            }
            any = 1;
            w += n;
        }
    }
    return !any || (minX <= 0 && maxX >= 0x140);
}

void WideCommitFrame(s32 bufferY, s32 width, s32 height, const GpuDmaTag *ordering,
                     const GpuDmaTag *secondary) {
    static s32 presentedWide, wideWidth, wideHeight;
    s32 wide = WideViewThisFrame || WideSqueezedThisFrame;

    if (Psyz_VideoGetWidescreen()) {
        if (!wide && presentedWide && width == wideWidth && height == wideHeight &&
            WideBlankFrame(ordering) && (!secondary || WideBlankFrame(secondary)))
            wide = 1;
        /* Everything else is drawn and shown exactly as with the option off. */
        if (wide)
            Psyz_VideoSetWidescreenPicture(bufferY, -WideMargin(), width + WideMargin());
        else
            Psyz_VideoSetWidescreenNative(bufferY);
    } else {
        wide = 0;
    }
    presentedWide = wide;
    if (wide) {
        wideWidth = width;
        wideHeight = height;
    }
    WideViewLastFrame = WideViewThisFrame;
    WideViewThisFrame = 0;
    WideSqueezedThisFrame = 0;
}

int GetViewTrackNode(const s32 *position, s32 player) {
    if (ViewUsesCarTrackNode == 0) {
        return FindTrackNodeAtXZ(position[0], position[2]);
    }
    return PlayerCars[player].trackNodeF0;
}

/* Initial values of the globals this module owns. */
s16 ViewOffsetX[8] = {160, 160, 160, 160, 80, 240, 160};
s16 ViewOffsetY[8] = {128, 32, 64, 192, 128, 128, 256};
/* The Japanese release's values (SLPS-01800). */
static const s16 ViewOffsetYJp[8] = {0x78, 0x20, 0x3C, 0xB4, 0x78, 0x78, 0xF0, 0};
s32 ViewScreenH[7] = {290, 320, 290, 290, 290, 290, 290};
u16 CourseSelectMapCoordsX[8] = {0, 0x62, 0, 0, 0, 0xA0};
s16 CourseSelectMapCoordsY[8] = {0, 0, 0, 128};
/* The Japanese release's values (SLPS-01800). */
static const s16 CourseSelectMapCoordsYJp[8] = {0, 0, 0, 0x78, 0, 0, 0, 0};
u16 CourseSelectLabelPosX[8] = {0x140, 0xDE, 0x140, 0x140, 0xA0, 0x140, 0x140};
s16 CourseSelectLabelPosY[8] = {256, 28, 128, 256, 256, 256, 512};
/* The Japanese release's values (SLPS-01800). */
static const s16 CourseSelectLabelPosYJp[8] = {0xF0, 0x1C, 0x78, 0xF0, 0xF0, 0xF0, 0x1E0, 0};
GpuRectangleFields CourseSelectPreviewUvs[2][7] = {{{.w = 320, .h = 256}, {.x = 98, .y = 18, .w = 124, .h = 28}, {.w = 320, .h = 128}, {.y = 128, .w = 320, .h = 128}, {.w = 160, .h = 256}, {.x = 160, .w = 160, .h = 256}, {.w = 320, .h = 512}}, {{.y = 256, .w = 320, .h = 256}, {.x = 98, .y = 274, .w = 124, .h = 28}, {.y = 256, .w = 320, .h = 128}, {.y = 384, .w = 320, .h = 128}, {.y = 256, .w = 160, .h = 256}, {.x = 160, .y = 256, .w = 160, .h = 256}, {.w = 320, .h = 512}}};
/* The Japanese release's values (SLPS-01800): 240-line buffers. */
static GpuRectangleFields CourseSelectPreviewUvsJp[2][7] = {{{.w = 320, .h = 240}, {.x = 98, .y = 18, .w = 124, .h = 28}, {.w = 320, .h = 120}, {.y = 120, .w = 320, .h = 120}, {.w = 160, .h = 240}, {.x = 160, .w = 160, .h = 240}, {.w = 320, .h = 480}}, {{.y = 240, .w = 320, .h = 240}, {.x = 98, .y = 258, .w = 124, .h = 28}, {.y = 240, .w = 320, .h = 120}, {.y = 360, .w = 320, .h = 120}, {.y = 240, .w = 160, .h = 240}, {.x = 160, .y = 240, .w = 160, .h = 240}, {.w = 320, .h = 480}}};
int BackColorR;
s32 WideMarginX;
s32 ActiveViewportIndex;
int FarColorB;
int FarColorG;
int FarColorR;
int ViewUsesCarTrackNode;
int BackColorB;
int BackColorG;
unsigned short CameraTargetTrackNode;
