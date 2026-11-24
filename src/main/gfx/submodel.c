#include "rr/course_occlusion.h"
#include "rr/field_rate.h"
/* Handwritten GTE renderers of the resident executable.
 *
 *   80058118..800587E0  SubmitModelTwoTri and its face loop
 *   8005F0D0..8005FC70  SubmitModel/SubmitModel2, RRO face loop and emitters
 *   8005FC74..80060C70  relative course models (projected vertex cache)
 *   80060C70..8006E8xx  course horizon section passes, EmitAnimatedTrackLines
 *
 * The original routines passed state in registers and scratchpad words; here
 * the scratch words become the named fields of the static work records below,
 * while scene configuration is read from RroSceneScratchPrefix by name. GTE
 * command order, register usage and packet contents follow the original. */
#include "rr/widescreen.h"
#include "rr/draw_distance.h"
#include "game/data/globals.h"
#include "rr/frame_loop.h"
#include <stddef.h>
#include "types.h"
#include "rr_asm.h"
#include "rr/depth_buffer.h"
#include "scratchpad.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "rr/rro_model.h"
#include "rr/renderer_setup.h"
#include "rr/relative_file.h"
#include "rr/course_view_types.h"
#include "rr/camera_matrices.h"
#include "game/camera/viewini.h"
#include "rr/animated_track_types.h"
#include "rr/overlay665_render_types.h"
#include "game/data/screenframe.h"
#include "game/gfx/submodel.h"

/* ------------------------------------------------------------------------ */
/* GTE access                                                               */
/* ------------------------------------------------------------------------ */

enum {
    GTE_VXY0 = 0, GTE_VZ0, GTE_VXY1, GTE_VZ1, GTE_VXY2, GTE_VZ2, GTE_RGBC, GTE_OTZ,
    GTE_IR0, GTE_IR1, GTE_IR2, GTE_IR3, GTE_SXY0, GTE_SXY1, GTE_SXY2, GTE_SXYP,
    GTE_SZ0, GTE_SZ1, GTE_SZ2, GTE_SZ3, GTE_RGB0, GTE_RGB1, GTE_RGB2, GTE_RES1,
    GTE_MAC0, GTE_MAC1, GTE_MAC2, GTE_MAC3
};
enum {
    GTE_R11R12 = 0, GTE_R13R21 = 1, GTE_TRX = 5, GTE_TRY = 6, GTE_TRZ = 7,
    GTE_FC_R = 21, GTE_FC_G = 22, GTE_FC_B = 23, GTE_FLAG = 31
};
/* Command words of the original sequences (Psyz_GteCommand takes bits 0..24). */
enum {
    GTE_RTPS = 0x0180001, GTE_RTPT = 0x0280030, GTE_NCLIP = 0x1400006,
    GTE_AVSZ3 = 0x158002D, GTE_AVSZ4 = 0x168002E,
    GTE_DPCS = 0x0780010, GTE_DPCT = 0x0F8002A, GTE_DPCL = 0x0680029,
    GTE_INTPL = 0x0980011, GTE_SQR12 = 0x0A80428, GTE_CC = 0x138041C,
    GTE_NCS = 0x0C8041E, GTE_NCT = 0x0D80420, GTE_NCDS = 0x0E80413,
    GTE_NCDT = 0x0F80416, GTE_NCCS = 0x108041B, GTE_NCCT = 0x118043F,
    /* mvmva sf=1, light matrix, V0/V1/V2, no translation, lm=1 */
    GTE_MVMVA_LLM_V0 = 0x04A6412, GTE_MVMVA_LLM_V1 = 0x04AE412,
    GTE_MVMVA_LLM_V2 = 0x04B6412
};

static inline void gte_mtc2(unsigned reg, s32 value) { Psyz_GteDataWrite(reg, (u32)value); }
static inline s32 gte_mfc2(unsigned reg) { return (s32)Psyz_GteDataRead(reg); }
static inline void gte_ctc2(unsigned reg, s32 value) { Psyz_GteCtrlWrite(reg, (u32)value); }
static inline s32 gte_cfc2(unsigned reg) { return (s32)Psyz_GteCtrlRead(reg); }
static inline void gte_cmd(u32 word) { Psyz_GteCommand(word & 0x1FFFFFF); }
/* lwc2 of the two words of an eight-byte vertex into V0, V1 or V2. */
static inline void gte_ldv(unsigned slot, const void *vertex)
{
    const u32 *words = (const u32 *)vertex;
    gte_mtc2(slot * 2, (s32)words[0]);
    gte_mtc2(slot * 2 + 1, (s32)words[1]);
}

/* The ordering-table region these renderers use starts at entry 128. */
static inline GpuDmaTag *RenderOrderBase(void) { return RR_GPU_SCRATCH->order + 128; }

/* Low/high signed halves of a packed screen word. */
static inline s32 ScreenX(s32 xy) { return (s16)xy; }
static inline s32 ScreenY(s32 xy) { return xy >> 16; }

/* True when all four points lie beyond one edge of the scene viewport; the
 * viewport rectangle stores the minimum in x/y and the maximum in w/h.
 * Widescreen reaches WideMarginX past both sides of a full-width view. */
static int QuadOutsideViewportX(const RroSceneScratchPrefix *scene, s32 x0, s32 x1, s32 x2, s32 x3)
{
    const GpuRectangleFields *vp = &scene->viewport6C.rect;
    const s32 margin = WideMarginX;
    const s32 left = vp->x - margin, right = vp->w + margin;
    if (right < x0 && right < x1 && right < x2 && right < x3) return 1;
    return x0 < left && x1 < left && x2 < left && x3 < left;
}

static int QuadOutsideViewportY(const RroSceneScratchPrefix *scene, s32 y0, s32 y1, s32 y2, s32 y3)
{
    const GpuRectangleFields *vp = &scene->viewport6C.rect;
    const s32 top = vp->y, bottom = vp->h;
    if (bottom < y0 && bottom < y1 && bottom < y2 && bottom < y3) return 1;
    return y0 < top && y1 < top && y2 < top && y3 < top;
}

/* QuadOffscreenTest0..13: x0..x3 then y0..y3, as the copies take them in
 * $t2..$t5 and $t6..$t9. */
static int QuadOutsideViewport(const RroSceneScratchPrefix *scene,
                               s32 x0, s32 x1, s32 x2, s32 x3,
                               s32 y0, s32 y1, s32 y2, s32 y3)
{
    return QuadOutsideViewportX(scene, x0, x1, x2, x3) || QuadOutsideViewportY(scene, y0, y1, y2, y3);
}

static int ScreenQuadOutsideX(const RroSceneScratchPrefix *scene, const s32 sxy[4])
{
    return QuadOutsideViewportX(scene, ScreenX(sxy[0]), ScreenX(sxy[1]), ScreenX(sxy[2]), ScreenX(sxy[3]));
}

static int ScreenQuadOutsideY(const RroSceneScratchPrefix *scene, const s32 sxy[4])
{
    return QuadOutsideViewportY(scene, ScreenY(sxy[0]), ScreenY(sxy[1]), ScreenY(sxy[2]), ScreenY(sxy[3]));
}

static int ScreenQuadOutsideViewport(const RroSceneScratchPrefix *scene, const s32 sxy[4])
{
    return ScreenQuadOutsideX(scene, sxy) || ScreenQuadOutsideY(scene, sxy);
}

/* Mirror the projection across X: negate the first rotation row and TRX. */
static void MirrorGteRotationX(void)
{
    s32 r0 = gte_cfc2(GTE_R11R12);
    s32 r1 = gte_cfc2(GTE_R13R21);
    s32 trx;
    gte_ctc2(GTE_R11R12, (s32)(((u32)-(s32)(s16)r0 & 0xFFFF) | ((u32)-(r0 >> 16) << 16)));
    trx = gte_cfc2(GTE_TRX);
    gte_ctc2(GTE_R13R21, (s32)(((u32)-(s32)(s16)r1 & 0xFFFF) | ((u32)(r1 >> 16) << 16)));
    gte_ctc2(GTE_TRX, -trx);
}

/* Host packet layouts not shared elsewhere. */
typedef struct {
    GpuDmaTag tag;
    u32 color;
    s32 xy0, xy1;
} LineF2Words;
typedef struct {
    GpuDmaTag tag;
    u32 color;
    s32 xy0, xy1, xy2;
    u32 terminator;
} LineF3Words;
typedef struct {
    GpuDmaTag tag;
    u32 color;
    s32 xy0, xy1, xy2, xy3;
    u32 terminator;
} LineF4Words;
enum { GPU_LINE_TERMINATOR = 0x55555555 };

/* ------------------------------------------------------------------------ */
/* RRO model faces (SubmitModel, SubmitModel2, SubmitModelTwoTri)           */
/* ------------------------------------------------------------------------ */

/* Scratch words 0x7C, 0x80, 0x84 and 0x98 of the model renderers. The fog
 * colours are latched by SubmitModel/SubmitModelTwoTri and only read by the
 * depth-cued emitters of SubmitModel2. */
static s32 RroLightingMode;
static u32 RroFogColorFT4, RroFogColorGT4;
static s32 RroDepthBias;

/* Face records start with two words of packed vertex indices (lower then upper
 * halfword); the signed ordering bias byte follows each record's attributes. */
typedef u8 *(*RroFaceEmitter)(const RroSceneScratchPrefix *scene, u8 *packet,
                              const u32 *face, const s32 sxy[4], GpuDmaTag *ot);
typedef struct {
    RroFaceEmitter emit;
    u32 stride;
} RroFaceKind;

static inline s32 FaceBias(const u32 *face, unsigned byteOffset)
{
    return ((const s8 *)face)[byteOffset];
}

/* Depth buffer (rr/depth_buffer.h): the face's ordering bias goes with its
 * packet vertices. Faces on one surface get the same depth, and the bias
 * decides between them as the ordering table did: a road marking (a smaller
 * bias) stays on the road it lies on. */
static inline void FaceDepthClass(const u32 *face, unsigned byteOffset)
{
    if (DepthBuffer)
        Psyz_GteSetPgxpDepthLayer(1, FaceBias(face, byteOffset));
}

static inline void FaceDepthClassEnd(void)
{
    if (DepthBuffer)
        Psyz_GteSetPgxpDepthLayer(0, 0);
}

/* mvmva(light matrix, Vn) followed by two squarings and a colour multiply. */
static void RroSquaredLight(u32 mvmva)
{
    gte_cmd(mvmva);
    gte_cmd(GTE_SQR12);
    gte_cmd(GTE_SQR12);
    gte_cmd(GTE_CC);
}

static void RroLoadFaceNormals(const RroSceneScratchPrefix *scene, const u32 *face,
                               unsigned word, const Vec3s8 **fourth)
{
    const Vec3s8 *normals = RroScene.normals;
    u32 n01 = face[word], n23 = face[word + 1];
    gte_ldv(0, &normals[n01 & 0xFFFF]);
    gte_ldv(1, &normals[n01 >> 16]);
    gte_ldv(2, &normals[n23 & 0xFFFF]);
    *fourth = &normals[n23 >> 16];
}

/* 8005F3D0: flat quad, colour word 2, bias byte 0xD. */
static u8 *EmitPolyF4(const RroSceneScratchPrefix *scene, u8 *packet, const u32 *face,
                      const s32 sxy[4], GpuDmaTag *ot)
{
    PolyF4Packet *p = (PolyF4Packet *)packet;
    (void)scene;
    RR_SXY_COPY(p->xy0, sxy[0]);
    RR_SXY_COPY(p->xy1, sxy[1]);
    RR_SXY_COPY(p->xy2, sxy[2]);
    RR_SXY_COPY(p->xy3, sxy[3]);
    p->colorCode04 = (s32)face[2];
    RR_GPU_ADD_PRIM(ot + FaceBias(face, 0xD), p, 5);
    return (u8 *)(p + 1);
}

/* 8005F434: raw textured quad. Only the command byte is written; the packet's
 * colour bytes are left as they are (raw texturing ignores them). */
static u8 *EmitPolyFT4Raw(const RroSceneScratchPrefix *scene, u8 *packet, const u32 *face,
                          const s32 sxy[4], GpuDmaTag *ot)
{
    GpuPolyFT4 *p = (GpuPolyFT4 *)packet;
    RR_SXY_COPY(p->words.xy0, sxy[0]);
    RR_SXY_COPY(p->words.xy1, sxy[1]);
    RR_SXY_COPY(p->words.xy2, sxy[2]);
    RR_SXY_COPY(p->words.xy3, sxy[3]);
    p->fields.code = 0x2D;
    p->words.uv0 = (s32)(face[2] + (u32)scene->draw74.draw.counter);
    p->words.uv1 = (s32)face[3];
    p->fields.u2 = (u8)face[4];
    p->fields.v2 = (u8)(face[4] >> 8);
    p->fields.u3 = (u8)(face[4] >> 16);
    p->fields.v3 = (u8)(face[4] >> 24);
    RR_GPU_ADD_PRIM(ot + FaceBias(face, 0x15), p, 9);
    return (u8 *)(p + 1);
}

/* 8005F4C4: Gouraud quad lit from four normals; RGBC is colour word 4. */
static u8 *EmitPolyG4(const RroSceneScratchPrefix *scene, u8 *packet, const u32 *face,
                      const s32 sxy[4], GpuDmaTag *ot)
{
    PolyG4WordPacket *p = (PolyG4WordPacket *)packet;
    const Vec3s8 *fourth;
    RR_SXY_COPY(p->xy0, sxy[0]);
    RR_SXY_COPY(p->xy1, sxy[1]);
    RR_SXY_COPY(p->xy2, sxy[2]);
    RR_SXY_COPY(p->xy3, sxy[3]);
    RroLoadFaceNormals(scene, face, 2, &fourth);
    gte_mtc2(GTE_RGBC, (s32)face[4]);
    if (RroLightingMode == 0) {
        gte_cmd(GTE_NCCT);
        RR_GPU_ADD_PRIM(ot + FaceBias(face, 0x15), p, 8);
        p->color04 = gte_mfc2(GTE_RGB0);
        p->color0C = gte_mfc2(GTE_RGB1);
        p->color14 = gte_mfc2(GTE_RGB2);
        gte_ldv(0, fourth);
        gte_cmd(GTE_NCCS);
        p->color1C = gte_mfc2(GTE_RGB2);
    } else {
        RroSquaredLight(GTE_MVMVA_LLM_V0);
        RroSquaredLight(GTE_MVMVA_LLM_V1);
        RroSquaredLight(GTE_MVMVA_LLM_V2);
        p->color04 = gte_mfc2(GTE_RGB0);
        p->color0C = gte_mfc2(GTE_RGB1);
        p->color14 = gte_mfc2(GTE_RGB2);
        gte_ldv(0, fourth);
        RroSquaredLight(GTE_MVMVA_LLM_V0);
        p->color1C = gte_mfc2(GTE_RGB2);
        RR_GPU_ADD_PRIM(ot + FaceBias(face, 0x15), p, 8);
    }
    return (u8 *)(p + 1);
}

/* 8005F640: lit textured quad; RGBC 3C808080, texture words 4..6. */
static u8 *EmitPolyGT4(const RroSceneScratchPrefix *scene, u8 *packet, const u32 *face,
                       const s32 sxy[4], GpuDmaTag *ot)
{
    PolyGT4WordPacket *p = (PolyGT4WordPacket *)packet;
    const Vec3s8 *fourth;
    RR_SXY_COPY(p->xy0, sxy[0]);
    RR_SXY_COPY(p->xy1, sxy[1]);
    RR_SXY_COPY(p->xy2, sxy[2]);
    RR_SXY_COPY(p->xy3, sxy[3]);
    RroLoadFaceNormals(scene, face, 2, &fourth);
    gte_mtc2(GTE_RGBC, 0x3C808080);
    if (RroLightingMode == 0) {
        s32 first;
        gte_cmd(GTE_NCT);
        p->uvClut0C = (s32)(face[4] + (u32)scene->draw74.draw.counter);
        p->uvTpage18 = (s32)face[5];
        p->uv2 = (u16)face[6];
        p->uv3 = (u16)(face[6] >> 16);
        RR_GPU_ADD_PRIM(ot + FaceBias(face, 0x1D), p, 12);
        first = gte_mfc2(GTE_RGB0);
        p->color10 = gte_mfc2(GTE_RGB1);
        p->color1C = gte_mfc2(GTE_RGB2);
        gte_ldv(0, fourth);
        gte_cmd(GTE_NCS);
        p->color04 = (s32)(((u32)first & 0xFFFFFF) | 0x3C000000);
        p->color28 = gte_mfc2(GTE_RGB2);
    } else {
        RroSquaredLight(GTE_MVMVA_LLM_V0);
        RroSquaredLight(GTE_MVMVA_LLM_V1);
        RroSquaredLight(GTE_MVMVA_LLM_V2);
        p->color04 = gte_mfc2(GTE_RGB0);
        p->color10 = gte_mfc2(GTE_RGB1);
        p->color1C = gte_mfc2(GTE_RGB2);
        gte_ldv(0, fourth);
        RroSquaredLight(GTE_MVMVA_LLM_V0);
        p->color28 = gte_mfc2(GTE_RGB2);
        p->uvClut0C = (s32)(face[4] + (u32)scene->draw74.draw.counter);
        p->uvTpage18 = (s32)face[5];
        p->uv2 = (u16)face[6];
        p->uv3 = (u16)(face[6] >> 16);
        RR_GPU_ADD_PRIM(ot + FaceBias(face, 0x1D), p, 12);
    }
    return (u8 *)(p + 1);
}

/* 8005F82C: flat quad depth-cued from the last projected vertex. */
static u8 *EmitPolyF4Fog(const RroSceneScratchPrefix *scene, u8 *packet, const u32 *face,
                         const s32 sxy[4], GpuDmaTag *ot)
{
    PolyF4Packet *p = (PolyF4Packet *)packet;
    (void)scene;
    RR_SXY_COPY(p->xy0, sxy[0]);
    RR_SXY_COPY(p->xy1, sxy[1]);
    RR_SXY_COPY(p->xy2, sxy[2]);
    RR_SXY_COPY(p->xy3, sxy[3]);
    gte_mtc2(GTE_RGBC, (s32)face[2]);
    gte_cmd(GTE_DPCS);
    RR_GPU_ADD_PRIM(ot + FaceBias(face, 0xD), p, 5);
    p->colorCode04 = gte_mfc2(GTE_RGB2);
    return (u8 *)(p + 1);
}

/* 8005F894: textured quad modulated by the depth-cued FT4 fog colour. */
static u8 *EmitPolyFT4Fog(const RroSceneScratchPrefix *scene, u8 *packet, const u32 *face,
                          const s32 sxy[4], GpuDmaTag *ot)
{
    GpuPolyFT4 *p = (GpuPolyFT4 *)packet;
    (void)scene;
    RR_SXY_COPY(p->words.xy0, sxy[0]);
    RR_SXY_COPY(p->words.xy1, sxy[1]);
    RR_SXY_COPY(p->words.xy2, sxy[2]);
    RR_SXY_COPY(p->words.xy3, sxy[3]);
    gte_mtc2(GTE_RGBC, (s32)RroFogColorFT4);
    gte_cmd(GTE_DPCS);
    p->words.uv0 = (s32)face[2];
    p->words.uv1 = (s32)face[3];
    p->fields.u2 = (u8)face[4];
    p->fields.v2 = (u8)(face[4] >> 8);
    p->fields.u3 = (u8)(face[4] >> 16);
    p->fields.v3 = (u8)(face[4] >> 24);
    RR_GPU_ADD_PRIM(ot + FaceBias(face, 0x15), p, 9);
    p->words.color04 = gte_mfc2(GTE_RGB2);
    return (u8 *)(p + 1);
}

/* 8005F920: lit and depth-cued Gouraud quad. */
static u8 *EmitPolyG4Fog(const RroSceneScratchPrefix *scene, u8 *packet, const u32 *face,
                         const s32 sxy[4], GpuDmaTag *ot)
{
    PolyG4WordPacket *p = (PolyG4WordPacket *)packet;
    const Vec3s8 *fourth;
    RR_SXY_COPY(p->xy0, sxy[0]);
    RR_SXY_COPY(p->xy1, sxy[1]);
    RR_SXY_COPY(p->xy2, sxy[2]);
    RR_SXY_COPY(p->xy3, sxy[3]);
    RroLoadFaceNormals(scene, face, 2, &fourth);
    gte_mtc2(GTE_RGBC, (s32)face[4]);
    if (RroLightingMode == 0) {
        gte_cmd(GTE_NCDT);
        RR_GPU_ADD_PRIM(ot + FaceBias(face, 0x15), p, 8);
        p->color04 = gte_mfc2(GTE_RGB0);
        p->color0C = gte_mfc2(GTE_RGB1);
        p->color14 = gte_mfc2(GTE_RGB2);
        gte_ldv(0, fourth);
        gte_cmd(GTE_NCDS);
        p->color1C = gte_mfc2(GTE_RGB2);
    } else {
        RroSquaredLight(GTE_MVMVA_LLM_V0);
        RroSquaredLight(GTE_MVMVA_LLM_V1);
        RroSquaredLight(GTE_MVMVA_LLM_V2);
        gte_cmd(GTE_DPCT);
        p->color04 = gte_mfc2(GTE_RGB0);
        p->color0C = gte_mfc2(GTE_RGB1);
        p->color14 = gte_mfc2(GTE_RGB2);
        gte_ldv(0, fourth);
        RroSquaredLight(GTE_MVMVA_LLM_V0);
        gte_cmd(GTE_DPCT);
        p->color1C = gte_mfc2(GTE_RGB2);
        RR_GPU_ADD_PRIM(ot + FaceBias(face, 0x15), p, 8);
    }
    return (u8 *)(p + 1);
}

/* 8005FAA4: lit, depth-cued textured quad; RGBC is the GT4 fog colour. */
static u8 *EmitPolyGT4Fog(const RroSceneScratchPrefix *scene, u8 *packet, const u32 *face,
                          const s32 sxy[4], GpuDmaTag *ot)
{
    PolyGT4WordPacket *p = (PolyGT4WordPacket *)packet;
    const Vec3s8 *fourth;
    RR_SXY_COPY(p->xy0, sxy[0]);
    RR_SXY_COPY(p->xy1, sxy[1]);
    RR_SXY_COPY(p->xy2, sxy[2]);
    RR_SXY_COPY(p->xy3, sxy[3]);
    RroLoadFaceNormals(scene, face, 2, &fourth);
    gte_mtc2(GTE_RGBC, (s32)RroFogColorGT4);
    if (RroLightingMode == 0) {
        gte_cmd(GTE_NCDT);
        p->uvClut0C = (s32)face[4];
        p->uvTpage18 = (s32)face[5];
        p->uv2 = (u16)face[6];
        p->uv3 = (u16)(face[6] >> 16);
        RR_GPU_ADD_PRIM(ot + FaceBias(face, 0x1D), p, 12);
        p->color04 = gte_mfc2(GTE_RGB0);
        p->color10 = gte_mfc2(GTE_RGB1);
        p->color1C = gte_mfc2(GTE_RGB2);
        gte_ldv(0, fourth);
        gte_cmd(GTE_NCDS);
        p->color28 = gte_mfc2(GTE_RGB2);
    } else {
        RroSquaredLight(GTE_MVMVA_LLM_V0);
        RroSquaredLight(GTE_MVMVA_LLM_V1);
        RroSquaredLight(GTE_MVMVA_LLM_V2);
        gte_cmd(GTE_DPCT);
        p->color04 = gte_mfc2(GTE_RGB0);
        p->color10 = gte_mfc2(GTE_RGB1);
        p->color1C = gte_mfc2(GTE_RGB2);
        gte_ldv(0, fourth);
        RroSquaredLight(GTE_MVMVA_LLM_V0);
        gte_cmd(GTE_DPCT);
        p->color28 = gte_mfc2(GTE_RGB2);
        p->uvClut0C = (s32)face[4];
        p->uvTpage18 = (s32)face[5];
        p->uv2 = (u16)face[6];
        p->uv3 = (u16)(face[6] >> 16);
        RR_GPU_ADD_PRIM(ot + FaceBias(face, 0x1D), p, 12);
    }
    return (u8 *)(p + 1);
}

/* jtbl_800A2158: face kinds 0..3 are depth-cued (SubmitModel2), 4..7 plain
 * (SubmitModel adds 4 to every header). jtbl_800A0FDC of the two-triangle
 * loop has the same entries. */
static const RroFaceKind RroFaceKinds[8] = {
    {EmitPolyF4Fog, 0x10}, {EmitPolyFT4Fog, 0x18}, {EmitPolyG4Fog, 0x18}, {EmitPolyGT4Fog, 0x20},
    {EmitPolyF4, 0x10}, {EmitPolyFT4Raw, 0x18}, {EmitPolyG4, 0x18}, {EmitPolyGT4, 0x20},
};

/* 8005F200 SubmitModelFaces / 80058118 SubmitModelFacesTwoTri: one face list.
 * header = kind | count << 16; the list starts at RroScene.drawPrimitive
 * and the pointer is advanced past it. twoTri keeps a quad when either of its
 * triangles faces the viewer. In mirrored rendering the two-triangle loop
 * tests a stale register instead of the first triangle's area (its value is
 * tracked in staleV0 below), so it normally rejects every face there. */
static void SubmitModelFaceList(RroSceneScratchPrefix *scene, u32 header, int twoTri)
{
    const RroFaceKind *kind = &RroFaceKinds[(header & 0xFFFF) & 7];
    u32 count = header >> 16;
    const u8 *face = (const u8 *)RroScene.drawPrimitive;
    const Vec3s8 *vertices = RroScene.vertices;
    s32 depthShift = scene->depthShift60;
    u8 *packet = RR_GPU_SCRATCH->cursor;
    GpuDmaTag *ot = RenderOrderBase() + RroDepthBias;
    s32 staleV0 = 0;

    do {
        const u32 *words = (const u32 *)face;
        s32 sxy[4];
        s32 area, depth;
        gte_ldv(0, &vertices[words[0] & 0xFFFF]);
        gte_ldv(1, &vertices[words[0] >> 16]);
        gte_ldv(2, &vertices[words[1] & 0xFFFF]);
        gte_cmd(GTE_RTPT);
        gte_cmd(GTE_NCLIP);
        area = gte_mfc2(GTE_MAC0);
        if (!twoTri) {
            if (scene->renderFlags64 == 0 ? area <= 0 : area >= 0) goto next;
        } else if (scene->renderFlags64 != 0 && staleV0 >= 0) {
            goto next;
        }
        RR_GTE_STORE_SXY(GTE_SXY0, &sxy[0]);
        RR_GTE_STORE_SXY(GTE_SXY1, &sxy[1]);
        RR_GTE_STORE_SXY(GTE_SXY2, &sxy[2]);
        gte_ldv(0, &vertices[words[1] >> 16]);
        gte_cmd(GTE_RTPS);
        if (twoTri) {
            staleV0 = scene->viewport6C.rect.w;
            gte_cmd(GTE_NCLIP);
            if (!(area > 0) && !(gte_mfc2(GTE_MAC0) < 0)) goto next;
        }
        RR_GTE_STORE_SXY(GTE_SXY2, &sxy[3]);
        gte_cmd(GTE_AVSZ4);
        depth = gte_mfc2(GTE_OTZ);
        if (ScreenQuadOutsideX(scene, sxy)) goto next;
        if (twoTri) staleV0 = scene->viewport6C.rect.h;
        if (ScreenQuadOutsideY(scene, sxy)) goto next;
        depth >>= depthShift & 31;
        staleV0 = (u32)(depth - 1) < 0x1BF;
        if (!staleV0) goto next;
        FaceDepthClass(words, kind->stride - 3);
        {
            u8 *face = packet;
            packet = kind->emit(scene, packet, words, sxy, ot + depth);
            if (CourseOcclusionCarActive && packet != face)
                CourseOcclusionFace(ot + depth + FaceBias(words, kind->stride - 3),
                                    (GpuDmaTag *)face, (GpuDmaTag *)face, NULL, sxy, 4);
        }
        /* The emitters leave a nonnegative value in that register. */
        staleV0 = 0;
    next:
        face += kind->stride;
    } while (--count != 0);
    FaceDepthClassEnd();

    RroScene.drawPrimitive = (const u32 *)face;
    RR_GPU_SCRATCH->cursor = packet;
}

/* Walk a model's command stream: a header word per face list, zero ends. */
static void SubmitModelStream(RroSceneScratchPrefix *scene, s32 modelIndex, u32 kindOffset, int twoTri)
{
    const u32 *stream = RroScene.commands[modelIndex].pointer;
    u32 header = stream[0];
    const u32 *cursor = stream + 1;
    while (header != 0) {
        RroScene.drawPrimitive = cursor;
        SubmitModelFaceList(scene, header + kindOffset, twoTri);
        cursor = RroScene.drawPrimitive;
        header = *cursor++;
    }
}

/* 8005F0D0 */
void SubmitModel(RroSceneScratchPrefix *scene, s32 modelIndex, s32 flags, s32 depthBias)
{
    RroFogColorFT4 = *(const u32 *)&DefaultScreenClearColor;
    RroFogColorGT4 = *(const u32 *)&LoadingScreenClearColor;
    RroLightingMode = flags;
    RroDepthBias = depthBias;
    SubmitModelStream(scene, modelIndex, 4, 0);
}

/* 8005F14C: depth-cued faces, mirrored when renderFlags64 is set. The depth
 * bias and fog colours are those latched by the last SubmitModel call. */
void SubmitModel2(RroSceneScratchPrefix *scene, s32 modelIndex, s32 lightingMode)
{
    RroLightingMode = lightingMode;
    if (scene->renderFlags64 != 0) MirrorGteRotationX();
    SubmitModelStream(scene, modelIndex, 0, 0);
}

/* 80058768 */
void SubmitModelTwoTri(PreviewRenderScratch *scratch, s32 object, s32 blend, s32 depth)
{
    RroFogColorFT4 = *(const u32 *)&DefaultScreenClearColor;
    RroFogColorGT4 = *(const u32 *)&LoadingScreenClearColor;
    RroLightingMode = blend;
    RroDepthBias = depth;
    SubmitModelStream(&scratch->scene, object, 4, 1);
}

/* ------------------------------------------------------------------------ */
/* Relative course models (SubmitCourseModel, SubmitCourseModel2)           */
/* ------------------------------------------------------------------------ */

/* TransformCourseModel projects the whole vertex pool once (scratch 0x120:
 * screen XY, 0x300: SZ >> depthShift); the faces then index the cache. The
 * PS1 areas hold 120 and 128 entries; triples are projected, so the cache is
 * one triple larger than the largest pool. */
enum { COURSE_MODEL_CACHE = 129 };
static const Vec3s8 *CourseModelVertices;           /* scratch 0x4C */
static s32 CourseProjectedXY[COURSE_MODEL_CACHE];
static s16 CourseProjectedDepth[COURSE_MODEL_CACHE];

/* Subdivided textured quad staging (scratch 0x7C..0xF4). */
typedef struct {
    Vec3s8 vertex[4];          /* 0x84..0xA3 */
    u8 uv[4][2];               /* 0xA4, 0xA8, 0xAC, 0xAE (u, v) */
    u16 clut, tpage;           /* 0xA6, 0xAA */
    u32 color;                 /* 0x7C: command/colour word of every piece */
    GpuDmaTag *ot;             /* 0x80 */
    s32 sxy[4];                /* 0xDC..0xE8 */
} CourseQuad;
static CourseQuad CourseSubdivQuad;

/* Second and fourth words of a face's texture coordinates, as the packet
 * words uv0|clut and uv1|tpage plus the two lone uv halfwords. */
static void CourseQuadSetUV(CourseQuad *q, u32 uvClut, u32 uvTpage, u32 uv2, u32 uv3)
{
    q->uv[0][0] = (u8)uvClut;
    q->uv[0][1] = (u8)(uvClut >> 8);
    q->clut = (u16)(uvClut >> 16);
    q->uv[1][0] = (u8)uvTpage;
    q->uv[1][1] = (u8)(uvTpage >> 8);
    q->tpage = (u16)(uvTpage >> 16);
    q->uv[2][0] = (u8)uv2;
    q->uv[2][1] = (u8)(uv2 >> 8);
    q->uv[3][0] = (u8)uv3;
    q->uv[3][1] = (u8)(uv3 >> 8);
}

static u32 CourseQuadUVWord(const CourseQuad *q, int i, u16 high)
{
    return (u32)q->uv[i][0] | ((u32)q->uv[i][1] << 8) | ((u32)high << 16);
}

/* 8005FD24 */
static const u32 *TransformCourseModel(RroSceneScratchPrefix *scene, s32 modelIndex)
{
    const RroEffectMesh *entry = &RroScene.modelEntries[modelIndex];
    const Vec3s8 *v;
    s32 remaining = (s32)entry->vertexCount;
    s32 shift = scene->depthShift60 & 31;
    int i = 0;

    CourseModelVertices = entry->vertices;
    if (scene->renderFlags64 != 0) MirrorGteRotationX();
    v = CourseModelVertices;
    do {
        if (i + 3 > COURSE_MODEL_CACHE) break;
        gte_ldv(0, &v[i]);
        gte_ldv(1, &v[i + 1]);
        gte_ldv(2, &v[i + 2]);
        gte_cmd(GTE_RTPT);
        RR_GTE_STORE_SXY(GTE_SXY0, &CourseProjectedXY[i]);
        RR_GTE_STORE_SXY(GTE_SXY1, &CourseProjectedXY[i + 1]);
        RR_GTE_STORE_SXY(GTE_SXY2, &CourseProjectedXY[i + 2]);
        CourseProjectedDepth[i] = (s16)((u32)gte_mfc2(GTE_SZ1) >> shift);
        CourseProjectedDepth[i + 1] = (s16)((u32)gte_mfc2(GTE_SZ2) >> shift);
        CourseProjectedDepth[i + 2] = (s16)((u32)gte_mfc2(GTE_SZ3) >> shift);
        i += 3;
        remaining -= 3;
    } while (remaining > 0);
    return entry->commands;
}

/* Common face test of loop .L8006064C/.L800608A4: cached SXY, back-face cull,
 * viewport and depth range. Returns the depth (sum of the first and last
 * vertex depths / 8) or -1 to skip the face. */
static s32 CourseFaceVisible(const RroSceneScratchPrefix *scene, const u32 *face, s32 sxy[4])
{
    u32 i0 = face[0] & 0xFFFF, i1 = face[0] >> 16;
    u32 i2 = face[1] & 0xFFFF, i3 = face[1] >> 16;
    s32 area, depth;
    RR_GTE_LOAD_SXY(GTE_SXY0, &CourseProjectedXY[i0]);
    RR_GTE_LOAD_SXY(GTE_SXY1, &CourseProjectedXY[i1]);
    RR_GTE_LOAD_SXY(GTE_SXY2, &CourseProjectedXY[i2]);
    gte_cmd(GTE_NCLIP);
    area = gte_mfc2(GTE_MAC0);
    if (scene->renderFlags64 == 0 ? area <= 0 : area >= 0) return -1;
    RR_GTE_STORE_SXY(GTE_SXY0, &sxy[0]);
    RR_GTE_STORE_SXY(GTE_SXY1, &sxy[1]);
    RR_GTE_STORE_SXY(GTE_SXY2, &sxy[2]);
    RR_SXY_COPY(sxy[3], CourseProjectedXY[i3]);
    depth = CourseProjectedDepth[i3] + CourseProjectedDepth[i0];
    if (ScreenQuadOutsideY(scene, sxy) || ScreenQuadOutsideX(scene, sxy)) return -1;
    depth >>= 3;
    if ((u32)(depth - 1) >= 0x1BF) return -1;
    return depth;
}

/* Fog variants depth-cue against the projection of the fourth vertex. */
static void CourseFogFromFourthVertex(const u32 *face)
{
    gte_ldv(0, &CourseModelVertices[face[1] >> 16]);
    gte_cmd(GTE_RTPS);
}

/* 8005FE64 / 800605AC: flat quads, 16-byte records, bias byte 0xD. */
static void CourseEmitF4(RroSceneScratchPrefix *scene, u32 count, int fog)
{
    const u32 *face = RroScene.drawPrimitive;
    u8 *packet = RR_GPU_SCRATCH->cursor;
    do {
        s32 sxy[4];
        s32 depth;
        FaceDepthClass(face, 0xD);
        depth = CourseFaceVisible(scene, face, sxy);
        if (depth >= 0) {
            PolyF4Packet *p = (PolyF4Packet *)packet;
            RR_SXY_COPY(p->xy0, sxy[0]);
            RR_SXY_COPY(p->xy1, sxy[1]);
            RR_SXY_COPY(p->xy2, sxy[2]);
            RR_SXY_COPY(p->xy3, sxy[3]);
            if (fog) {
                CourseFogFromFourthVertex(face);
                gte_mtc2(GTE_RGBC, (s32)face[2]);
                gte_cmd(GTE_DPCS);
                p->colorCode04 = gte_mfc2(GTE_RGB2);
            } else {
                p->colorCode04 = (s32)face[2];
            }
            RR_GPU_ADD_PRIM(RenderOrderBase() + depth + FaceBias(face, 0xD), p, 5);
            packet = (u8 *)(p + 1);
        }
        face += 4;
    } while (--count != 0);
    FaceDepthClassEnd();
    RroScene.drawPrimitive = face;
    RR_GPU_SCRATCH->cursor = packet;
}

/* 8005FEE0 / 800607D8: textured quads, 28-byte records, bias byte 0x19. */
static void CourseEmitFT4(RroSceneScratchPrefix *scene, u32 count, int fog)
{
    const u32 *face = RroScene.drawPrimitive;
    u8 *packet = RR_GPU_SCRATCH->cursor;
    do {
        s32 sxy[4];
        s32 depth;
        FaceDepthClass(face, 0x19);
        depth = CourseFaceVisible(scene, face, sxy);
        if (depth >= 0) {
            GpuPolyFT4 *p = (GpuPolyFT4 *)packet;
            RR_SXY_COPY(p->words.xy0, sxy[0]);
            RR_SXY_COPY(p->words.xy1, sxy[1]);
            RR_SXY_COPY(p->words.xy2, sxy[2]);
            RR_SXY_COPY(p->words.xy3, sxy[3]);
            if (fog) {
                CourseFogFromFourthVertex(face);
                gte_mtc2(GTE_RGBC, (s32)face[2]);
                gte_cmd(GTE_DPCS);
            } else {
                p->words.color04 = (s32)face[2];
            }
            p->words.uv0 = (s32)(face[3] + (u32)scene->draw74.draw.counter);
            p->words.uv1 = (s32)face[4];
            p->fields.u2 = (u8)face[5];
            p->fields.v2 = (u8)(face[5] >> 8);
            p->fields.u3 = (u8)(face[5] >> 16);
            p->fields.v3 = (u8)(face[5] >> 24);
            RR_GPU_ADD_PRIM(RenderOrderBase() + depth + FaceBias(face, 0x19), p, 9);
            /* the scene counter scrolls the texture */
            if (fog) p->words.color04 = gte_mfc2(GTE_RGB2);
            packet = (u8 *)(p + 1);
        }
        face += 7;
    } while (--count != 0);
    FaceDepthClassEnd();
    RroScene.drawPrimitive = face;
    RR_GPU_SCRATCH->cursor = packet;
}

/* Texture-window state packet (E2 word, zero) linked ahead of the pieces. */
static u8 *CourseEmitTextureWindow(u8 *packet, GpuDmaTag *ot, u32 command)
{
    GpuDrawModePacket *p = (GpuDrawModePacket *)packet;
    p->command[0] = command;
    p->command[1] = 0;
    RR_GPU_ADD_PRIM(ot, p, 2);
    return (u8 *)(p + 1);
}

/* .L80060528: a quad that needs no subdivision. */
static u8 *CourseEmitWholeQuad(const CourseQuad *q, u8 *packet)
{
    GpuPolyFT4 *p = (GpuPolyFT4 *)packet;
    RR_SXY_COPY(p->words.xy0, q->sxy[0]);
    RR_SXY_COPY(p->words.xy1, q->sxy[1]);
    RR_SXY_COPY(p->words.xy2, q->sxy[2]);
    RR_SXY_COPY(p->words.xy3, q->sxy[3]);
    p->words.uv0 = (s32)CourseQuadUVWord(q, 0, q->clut);
    p->words.uv1 = (s32)CourseQuadUVWord(q, 1, q->tpage);
    p->fields.u2 = q->uv[2][0];
    p->fields.v2 = q->uv[2][1];
    p->fields.u3 = q->uv[3][0];
    p->fields.v3 = q->uv[3][1];
    p->words.color04 = (s32)q->color;
    RR_GPU_ADD_PRIM(q->ot, p, 9);
    return (u8 *)(p + 1);
}

/* INTPL of two GTE inputs: IR1..IR3 + (FC - IR) * IR0. */
static void CourseIntpl(s32 fcR, s32 fcG, s32 fcB, s32 ir1, s32 ir2, s32 ir3)
{
    gte_ctc2(GTE_FC_R, fcR);
    gte_ctc2(GTE_FC_G, fcG);
    gte_ctc2(GTE_FC_B, fcB);
    gte_mtc2(GTE_IR1, ir1);
    gte_mtc2(GTE_IR2, ir2);
    gte_mtc2(GTE_IR3, ir3);
    gte_cmd(GTE_INTPL);
}

/* .L80060078: split the quad into (0x1000 >> levelU) x (0x1000 >> levelV)
 * steps by GTE interpolation of positions and texture coordinates, each piece
 * a back-face/viewport tested FT4. The far colour (the INTPL target) is saved
 * and restored. Rows run from edge 0-2 to edge 1-3, columns from the 0-1 side
 * to the 2-3 side. A shift of 13 or more gives a zero step and never ends, as
 * in the original. */
static u8 *CourseEmitSubdividedQuad(const RroSceneScratchPrefix *scene, CourseQuad *q,
                                    u8 *packet, s32 levelU, s32 levelV)
{
    s32 savedR, savedG, savedB;
    s32 stepU, stepV, t, s;
    Vec3s8 prevLeft, prevRight, newLeft, newRight;
    u8 prevLeftUV[2], prevRightUV[2], newLeftUV[2], newRightUV[2];
    u16 colUV[2]; /* 0xD8: previous column uv on the previous and the new row */

    if ((levelU | levelV) == 0) return CourseEmitWholeQuad(q, packet);
    savedR = gte_cfc2(GTE_FC_R);
    savedG = gte_cfc2(GTE_FC_G);
    savedB = gte_cfc2(GTE_FC_B);
    stepU = (s32)(0x1000u >> (levelU & 31));
    stepV = (s32)(0x1000u >> (levelV & 31));
    newLeft = q->vertex[0];
    newRight = q->vertex[2];
    newLeftUV[0] = q->uv[0][0];
    newLeftUV[1] = q->uv[0][1];
    newRightUV[0] = q->uv[2][0];
    newRightUV[1] = q->uv[2][1];
    t = 0x1000;
    do {
        t -= stepU;
        gte_mtc2(GTE_IR0, t);
        prevLeft = newLeft;
        prevRight = newRight;
        prevLeftUV[0] = newLeftUV[0];
        prevLeftUV[1] = newLeftUV[1];
        prevRightUV[0] = newRightUV[0];
        prevRightUV[1] = newRightUV[1];
        /* Left and right edge texture coordinates, then positions. */
        gte_ctc2(GTE_FC_R, q->uv[0][0]);
        gte_ctc2(GTE_FC_G, q->uv[0][1]);
        gte_mtc2(GTE_IR1, q->uv[1][0]);
        gte_mtc2(GTE_IR2, q->uv[1][1]);
        gte_cmd(GTE_INTPL);
        gte_ctc2(GTE_FC_R, q->uv[2][0]);
        gte_ctc2(GTE_FC_G, q->uv[2][1]);
        newLeftUV[0] = (u8)gte_mfc2(GTE_IR1);
        newLeftUV[1] = (u8)gte_mfc2(GTE_IR2);
        gte_mtc2(GTE_IR1, q->uv[3][0]);
        gte_mtc2(GTE_IR2, q->uv[3][1]);
        gte_cmd(GTE_INTPL);
        gte_ctc2(GTE_FC_R, q->vertex[0].fields.x);
        gte_ctc2(GTE_FC_G, q->vertex[0].fields.y);
        gte_ctc2(GTE_FC_B, q->vertex[0].fields.z);
        newRightUV[0] = (u8)gte_mfc2(GTE_IR1);
        newRightUV[1] = (u8)gte_mfc2(GTE_IR2);
        gte_mtc2(GTE_IR1, q->vertex[1].fields.x);
        gte_mtc2(GTE_IR2, q->vertex[1].fields.y);
        gte_mtc2(GTE_IR3, q->vertex[1].fields.z);
        gte_cmd(GTE_INTPL);
        gte_ctc2(GTE_FC_R, q->vertex[2].fields.x);
        gte_ctc2(GTE_FC_G, q->vertex[2].fields.y);
        gte_ctc2(GTE_FC_B, q->vertex[2].fields.z);
        newLeft.words[0] = ((u32)gte_mfc2(GTE_IR1) & 0xFFFF) | ((u32)gte_mfc2(GTE_IR2) << 16);
        newLeft.words[1] = (u32)gte_mfc2(GTE_IR3);
        gte_mtc2(GTE_IR1, q->vertex[3].fields.x);
        gte_mtc2(GTE_IR2, q->vertex[3].fields.y);
        gte_mtc2(GTE_IR3, q->vertex[3].fields.z);
        gte_cmd(GTE_INTPL);
        /* Project both row starts; V2 keeps its previous contents. */
        gte_ldv(0, &prevLeft);
        gte_ldv(1, &newLeft);
        newRight.words[0] = ((u32)gte_mfc2(GTE_IR1) & 0xFFFF) | ((u32)gte_mfc2(GTE_IR2) << 16);
        newRight.words[1] = (u32)gte_mfc2(GTE_IR3);
        gte_cmd(GTE_RTPT);
        colUV[0] = (u16)(prevLeftUV[0] | (prevLeftUV[1] << 8));
        colUV[1] = (u16)(newLeftUV[0] | (newLeftUV[1] << 8));
        RR_GTE_STORE_SXY(GTE_SXY0, &q->sxy[0]);
        RR_GTE_STORE_SXY(GTE_SXY1, &q->sxy[1]);
        s = 0x1000;
        do {
            s32 x, y, z, visible;
            u32 uvPrev, uvNew;
            s -= stepV;
            gte_mtc2(GTE_IR0, s);
            CourseIntpl(prevLeft.fields.x, prevLeft.fields.y, prevLeft.fields.z,
                        prevRight.fields.x, prevRight.fields.y, prevRight.fields.z);
            gte_ctc2(GTE_FC_R, newLeft.fields.x);
            gte_ctc2(GTE_FC_G, newLeft.fields.y);
            gte_ctc2(GTE_FC_B, newLeft.fields.z);
            x = gte_mfc2(GTE_IR1);
            y = gte_mfc2(GTE_IR2);
            z = gte_mfc2(GTE_IR3);
            gte_mtc2(GTE_IR1, newRight.fields.x);
            gte_mtc2(GTE_IR2, newRight.fields.y);
            gte_mtc2(GTE_IR3, newRight.fields.z);
            gte_cmd(GTE_INTPL);
            gte_mtc2(GTE_VXY0, (s32)(((u32)x & 0xFFFF) | ((u32)y << 16)));
            gte_mtc2(GTE_VZ0, z);
            gte_ctc2(GTE_FC_R, prevLeftUV[0]);
            gte_ctc2(GTE_FC_G, prevLeftUV[1]);
            x = gte_mfc2(GTE_IR1);
            y = gte_mfc2(GTE_IR2);
            z = gte_mfc2(GTE_IR3);
            gte_mtc2(GTE_IR1, prevRightUV[0]);
            gte_mtc2(GTE_IR2, prevRightUV[1]);
            gte_mtc2(GTE_VXY1, (s32)(((u32)x & 0xFFFF) | ((u32)y << 16)));
            gte_mtc2(GTE_VZ1, z);
            gte_cmd(GTE_INTPL);
            {
                GpuPolyFT4 *p = (GpuPolyFT4 *)packet;
                p->words.uv0 = (s32)((u32)colUV[0] | ((u32)q->clut << 16));
                p->fields.u2 = (u8)colUV[1];
                p->fields.v2 = (u8)(colUV[1] >> 8);
                gte_ctc2(GTE_FC_R, newLeftUV[0]);
                gte_ctc2(GTE_FC_G, newLeftUV[1]);
                x = gte_mfc2(GTE_IR1);
                y = gte_mfc2(GTE_IR2);
                gte_mtc2(GTE_IR1, newRightUV[0]);
                gte_mtc2(GTE_IR2, newRightUV[1]);
                gte_cmd(GTE_INTPL);
                uvPrev = ((u32)x & 0xFF) | ((u32)y << 8) | ((u32)q->tpage << 16);
                colUV[0] = (u16)uvPrev;
                p->words.uv1 = (s32)uvPrev;
                uvNew = ((u32)gte_mfc2(GTE_IR1) & 0xFF) | ((u32)gte_mfc2(GTE_IR2) << 8);
                colUV[1] = (u16)uvNew;
                p->fields.u3 = (u8)uvNew;
                p->fields.v3 = (u8)(uvNew >> 8);
                gte_cmd(GTE_RTPT);
                RR_GTE_STORE_SXY(GTE_SXY0, &q->sxy[2]);
                RR_GTE_STORE_SXY(GTE_SXY1, &q->sxy[3]);
                /* Facing of (current prev-row, current new-row, previous
                 * new-row), else of (current prev-row, previous prev-row,
                 * previous new-row). */
                RR_GTE_LOAD_SXY(GTE_SXY0, &q->sxy[2]);
                RR_GTE_LOAD_SXY(GTE_SXY1, &q->sxy[3]);
                RR_GTE_LOAD_SXY(GTE_SXY2, &q->sxy[1]);
                gte_cmd(GTE_NCLIP);
                visible = gte_mfc2(GTE_MAC0);
                if (scene->renderFlags64 == 0) visible = -visible;
                if (visible <= 0) {
                    RR_GTE_LOAD_SXY(GTE_SXY1, &q->sxy[0]);
                    gte_cmd(GTE_NCLIP);
                    visible = gte_mfc2(GTE_MAC0);
                    RR_GTE_LOAD_SXY(GTE_SXY1, &q->sxy[3]);
                    if (scene->renderFlags64 != 0) visible = -visible;
                }
                if (visible > 0 && !ScreenQuadOutsideX(scene, q->sxy) && !ScreenQuadOutsideY(scene, q->sxy)) {
                    RR_SXY_COPY(p->words.xy0, q->sxy[0]);
                    RR_SXY_COPY(p->words.xy2, q->sxy[1]);
                    p->words.color04 = (s32)q->color;
                    RR_GPU_ADD_PRIM(q->ot, p, 9);
                    RR_SXY_COPY(p->words.xy1, q->sxy[2]);
                    RR_SXY_COPY(p->words.xy3, q->sxy[3]);
                    packet = (u8 *)(p + 1);
                }
                RR_SXY_COPY(q->sxy[0], q->sxy[2]);
                RR_SXY_COPY(q->sxy[1], q->sxy[3]);
            }
        } while (s != 0);
    } while (t != 0);
    gte_ctc2(GTE_FC_R, savedR);
    gte_ctc2(GTE_FC_G, savedG);
    gte_ctc2(GTE_FC_B, savedB);
    return packet;
}

enum { COURSE_QUAD_PLAIN, COURSE_QUAD_FOG, COURSE_QUAD_SCROLL, COURSE_QUAD_SCROLL_FOG };

/* 8005FF78 / 8006088C / func_80060B10 / 80060BB8: subdivided textured quads,
 * 32-byte records: indices, colour, uv0|clut, uv1|tpage, uv2, uv3, bias byte
 * 0x19, levels halfword 0x1A (low: columns, high: rows) and a texture-window
 * command 0x1C. Scrolling variants add the scene counter to the coordinates. */
static void CourseEmitSubdivided(RroSceneScratchPrefix *scene, u32 count, int variant)
{
    const u32 *face = RroScene.drawPrimitive;
    u8 *packet = RR_GPU_SCRATCH->cursor;
    CourseQuad *q = &CourseSubdivQuad;
    do {
        s32 depth;
        FaceDepthClass(face, 0x19);
        depth = CourseFaceVisible(scene, face, q->sxy);
        if (depth >= 0) {
            u32 levels = ((const u16 *)face)[0xD];
            s32 x2 = ScreenX(q->sxy[2]);
            /* The original shifts by the third vertex's screen X minus the
             * depth shift (a register reused by the viewport test). */
            s32 reduce = depth >> ((x2 - scene->depthShift60) & 31);
            s32 levelU = (s32)(levels & 0xFF) - reduce;
            s32 levelV = (s32)(levels >> 8) - reduce;
            u32 counter = (u32)scene->draw74.draw.counter;
            u32 c16 = counter & 0xFFFF;
            if (levelU < 0) levelU = 0;
            if (levelV < 0) levelV = 0;
            q->ot = RenderOrderBase() + depth + FaceBias(face, 0x19);
            packet = CourseEmitTextureWindow(packet, q->ot, 0xE2000000);
            if (variant == COURSE_QUAD_FOG || variant == COURSE_QUAD_SCROLL_FOG)
                CourseFogFromFourthVertex(face);
            q->vertex[0] = CourseModelVertices[face[0] & 0xFFFF];
            q->vertex[1] = CourseModelVertices[face[0] >> 16];
            q->vertex[2] = CourseModelVertices[face[1] & 0xFFFF];
            q->vertex[3] = CourseModelVertices[face[1] >> 16];
            if (variant == COURSE_QUAD_SCROLL || variant == COURSE_QUAD_SCROLL_FOG)
                CourseQuadSetUV(q, face[3] + counter, face[4] + c16,
                                (u16)(((const u16 *)face)[0xA] + c16), (u16)(((const u16 *)face)[0xB] + c16));
            else
                CourseQuadSetUV(q, face[3] + counter, face[4], ((const u16 *)face)[0xA], ((const u16 *)face)[0xB]);
            if (variant == COURSE_QUAD_FOG || variant == COURSE_QUAD_SCROLL_FOG) {
                gte_mtc2(GTE_RGBC, (s32)face[2]);
                gte_cmd(GTE_DPCS);
                q->color = (u32)gte_mfc2(GTE_RGB2);
            } else {
                q->color = face[2];
            }
            packet = CourseEmitSubdividedQuad(scene, q, packet, levelU, levelV);
            packet = CourseEmitTextureWindow(packet, q->ot, face[7]);
        }
        face += 8;
    } while (--count != 0);
    FaceDepthClassEnd();
    RroScene.drawPrimitive = face;
    RR_GPU_SCRATCH->cursor = packet;
}

typedef void (*CourseFaceHandler)(RroSceneScratchPrefix *scene, u32 count);
static void CourseF4(RroSceneScratchPrefix *s, u32 n) { CourseEmitF4(s, n, 0); }
static void CourseFT4(RroSceneScratchPrefix *s, u32 n) { CourseEmitFT4(s, n, 0); }
static void CourseGT4(RroSceneScratchPrefix *s, u32 n) { CourseEmitSubdivided(s, n, COURSE_QUAD_PLAIN); }
static void CourseF4Fog(RroSceneScratchPrefix *s, u32 n) { CourseEmitF4(s, n, 1); }
static void CourseFT4Fog(RroSceneScratchPrefix *s, u32 n) { CourseEmitFT4(s, n, 1); }
static void CourseGT4Fog(RroSceneScratchPrefix *s, u32 n) { CourseEmitSubdivided(s, n, COURSE_QUAD_FOG); }
/* func_80060B10 / 80060BB8: the scroll counter advances by ScreenFrame & 0x7F
 * on every list (it accumulates in the scene word); at field rate each of a
 * tick's fields adds half of it (rr/field_rate.h). */
static void CourseScroll(RroSceneScratchPrefix *s, u32 n)
{
    s->draw74.draw.counter += RaceFieldSplit(ScreenFrame & 0x7F);
    CourseEmitSubdivided(s, n, COURSE_QUAD_SCROLL);
}
static void CourseScrollFog(RroSceneScratchPrefix *s, u32 n)
{
    s->draw74.draw.counter += RaceFieldSplit(ScreenFrame & 0x7F);
    CourseEmitSubdivided(s, n, COURSE_QUAD_SCROLL_FOG);
}

/* D_800A2198 / D_800A21A8 */
static const CourseFaceHandler CourseFaceHandlers[4] = {CourseF4, CourseFT4, CourseGT4, CourseScroll};
static const CourseFaceHandler CourseFogFaceHandlers[4] = {CourseF4Fog, CourseFT4Fog, CourseGT4Fog, CourseScrollFog};

static void SubmitCourseModelWith(RroSceneScratchPrefix *scene, s32 modelIndex, const CourseFaceHandler *handlers)
{
    const u32 *cursor = TransformCourseModel(scene, modelIndex);
    u32 header = *cursor++;
    while (header != 0) {
        RroScene.drawPrimitive = cursor;
        handlers[header & 3](scene, header >> 16);
        cursor = RroScene.drawPrimitive;
        header = *cursor++;
    }
}

/* 8005FC74 */
void SubmitCourseModel(RroSceneScratchPrefix *scene, s32 modelIndex)
{
    SubmitCourseModelWith(scene, modelIndex, CourseFaceHandlers);
}

/* 8005FCCC */
void SubmitCourseModel2(void *scene, s32 modelIndex)
{
    SubmitCourseModelWith((RroSceneScratchPrefix *)scene, modelIndex, CourseFogFaceHandlers);
}

/* ------------------------------------------------------------------------ */
/* Course horizon (SubmitCourseHorizon and its section passes)              */
/* ------------------------------------------------------------------------ */

/* A corner or subdivision grid vertex (scratch 0x130.. and 0x178/0x25C rows):
 * position, then after projection SXY in word 0 and SZ in z; texture u,v
 * and a command/colour word. */
typedef union {
    struct {
        s16 x, y, z;
        u8 u, v;
        u8 r, g, b, code;
    } f;
    struct {
        s32 xy;
        s16 z;
        u16 uv;
        u32 color;
    } w;
} HorizonVertex;
_Static_assert(sizeof(HorizonVertex) == 12, "horizon vertex");

/* Subdivision rows hold up to 16 segments plus the two entries the triple
 * projection writes past the end. */
enum { HORIZON_ROW_ENTRIES = 19, HORIZON_MAX_SEGMENTS = 16 };

/* Scratch state shared by the passes and their emitters (PS1 offsets). */
typedef struct {
    RroSceneScratchPrefix *scene;
    u8 *cursor;                    /* $s0 */
    GpuDmaTag *ot;                 /* $v0 of the tile emitters */
    s32 savedFarColor[3];          /* 0xD8..0xE0 */
    s32 otBias;                    /* 0x68 */
    u32 lineFlags;                 /* $s3: bit0 GTE error, bit1 colour bit 31 */
    s32 sxy[4];                    /* 0xF4..0x100 */
    u32 lineColor;                 /* 0x104 */
    u16 clut, tpage;               /* 0xD4, 0xD6 */
    HorizonVertex corner[4];       /* 0x130, 0x13C, 0x148, 0x154 */
    HorizonVertex rows[2][HORIZON_ROW_ENTRIES]; /* 0x178, 0x25C */
} HorizonWork;
static HorizonWork Horizon;

/* The ordering-table entry of a horizon tile; with the depth buffer its
 * bias goes with the tile's packet vertices (FaceDepthClass), four times
 * over: the road's tiles are not planar where its markings lie on them, so
 * a marking (bias 5) stays over the road (bias 15..30) by 20 depth units
 * (some 20 cm) and more. (Not decals: the smallest bias also holds
 * kerbs and barriers, which must hide what lies behind them.) */
static GpuDmaTag *HorizonOrder(s32 depth, s32 bias)
{
    if (DepthBuffer)
        Psyz_GteSetPgxpDepthLayer(1, bias * 4);
    return RenderOrderBase() + depth + bias;
}

/* The loop over a pass's views: nonzero while view is one of them. */
static int HorizonView(u32 view, const VisibleCourseViews *views)
{
    return view <= views->stream[0].count;
}

/* The facing test of a tile just projected (front: it faces the view).
 * Nonzero keeps the tile. */
static int HorizonFacing(int front)
{
    return front;
}

/* Draw a tile's grid. */
#define HORIZON_GRID(grid, levelU, levelV, ...)                                       \
    do {                                                                              \
        grid((levelU), (levelV), __VA_ARGS__);                                        \
    } while (0)

/* Scalar scene halfwords at their PS1 scratch offsets (written through raw
 * halfword views, e.g. ApplySegmentVisibilityMask/RenderMirrorCourseView):
 * 0x5C is the depth beyond which tiles are drawn flat, 0x5E selects the
 * level-of-detail distance table. */
static s32 HorizonFarDepth(void) { return ((const s16 *)RrScratchpad)[0x5C / 2]; }
static s32 HorizonLodSelect(void) { return ((const s16 *)RrScratchpad)[0x5E / 2]; }

/* EmitLineF2Gn / EmitLineF3Gn / EmitLineF4Gn: outline polylines in the line
 * colour, 20 ordering entries behind the tile. */
static void HorizonLineF2(s32 xy0, s32 xy1)
{
    LineF2Words *p = (LineF2Words *)Horizon.cursor;
    RR_SXY_COPY(p->xy0, xy0);
    RR_SXY_COPY(p->xy1, xy1);
    p->color = (Horizon.lineColor & 0xFFFFFF) | 0x40000000;
    RR_GPU_ADD_PRIM(Horizon.ot + 20, p, 3);
    Horizon.cursor = (u8 *)(p + 1);
}

static void HorizonLineF3(s32 xy0, s32 xy1, s32 xy2)
{
    LineF3Words *p = (LineF3Words *)Horizon.cursor;
    RR_SXY_COPY(p->xy0, xy0);
    RR_SXY_COPY(p->xy1, xy1);
    RR_SXY_COPY(p->xy2, xy2);
    p->color = (Horizon.lineColor & 0xFFFFFF) | 0x48000000;
    p->terminator = GPU_LINE_TERMINATOR;
    RR_GPU_ADD_PRIM(Horizon.ot + 20, p, 5);
    Horizon.cursor = (u8 *)(p + 1);
}

static void HorizonLineF4(s32 xy0, s32 xy1, s32 xy2, s32 xy3)
{
    LineF4Words *p = (LineF4Words *)Horizon.cursor;
    RR_SXY_COPY(p->xy0, xy0);
    RR_SXY_COPY(p->xy1, xy1);
    RR_SXY_COPY(p->xy2, xy2);
    RR_SXY_COPY(p->xy3, xy3);
    p->color = (Horizon.lineColor & 0xFFFFFF) | 0x4C000000;
    p->terminator = GPU_LINE_TERMINATOR;
    RR_GPU_ADD_PRIM(Horizon.ot + 20, p, 6);
    Horizon.cursor = (u8 *)(p + 1);
}

/* EmitLineStripGn: polyline through row[last] .. row[0], four points per
 * packet sharing their ends. */
static void HorizonLineStrip(const HorizonVertex *row, int last)
{
    int end = last;
    while (2 < end) {
        HorizonLineF4(row[end].w.xy, row[end - 1].w.xy, row[end - 2].w.xy, row[end - 3].w.xy);
        end -= 3;
    }
    if (end == 0) return;
    if (end == 2) HorizonLineF3(row[2].w.xy, row[1].w.xy, row[0].w.xy);
    else HorizonLineF2(row[1].w.xy, row[0].w.xy);
}

/* DPCS of an RGBC word toward a far colour given in 1/16 units. */
static s32 HorizonDpcs(s32 rgbc, s32 fcR, s32 fcG, s32 fcB)
{
    gte_mtc2(GTE_RGBC, rgbc);
    gte_ctc2(GTE_FC_R, fcR);
    gte_ctc2(GTE_FC_G, fcG);
    gte_ctc2(GTE_FC_B, fcB);
    gte_cmd(GTE_DPCS);
    return gte_mfc2(GTE_RGB2);
}

/* Project row[0..last] in triples: XY becomes SXY and z becomes SZ. Up to two
 * entries past the end are projected as well. */
static void HorizonProjectRow(HorizonVertex *row, int last)
{
    int i = 0;
    do {
        gte_mtc2(GTE_VXY0, row[i].w.xy);
        gte_mtc2(GTE_VZ0, row[i].f.z);
        gte_mtc2(GTE_VXY1, row[i + 1].w.xy);
        gte_mtc2(GTE_VZ1, row[i + 1].f.z);
        gte_mtc2(GTE_VXY2, row[i + 2].w.xy);
        gte_mtc2(GTE_VZ2, row[i + 2].f.z);
        gte_cmd(GTE_RTPT);
        RR_GTE_STORE_SXY(GTE_SXY0, &row[i].w.xy);
        RR_GTE_STORE_SXY(GTE_SXY1, &row[i + 1].w.xy);
        RR_GTE_STORE_SXY(GTE_SXY2, &row[i + 2].w.xy);
        row[i].f.z = (s16)gte_mfc2(GTE_SZ1);
        row[i + 1].f.z = (s16)gte_mfc2(GTE_SZ2);
        row[i + 2].f.z = (s16)gte_mfc2(GTE_SZ3);
        i += 3;
    } while (i < last);
}

/* InterpEdgeVertsA0..A5: fill row[1..last-1] between row[0] and row[last]
 * (position by INTPL, texture coordinates and colour by DPCS; factor 0x1000
 * at row[0] falling by step), then project the row. */
static void HorizonInterpRowLit(HorizonVertex *row, int last, s32 step)
{
    const HorizonVertex *end = &row[last];
    const HorizonVertex *start = &row[0];
    s32 t = 0x1000 - step;
    int i = 1;
    for (; t != 0; t -= step, i++) {
        HorizonVertex *e = &row[i];
        gte_mtc2(GTE_IR0, t);
        CourseIntpl(start->f.x, start->f.y, start->f.z, end->f.x, end->f.y, end->f.z);
        e->f.x = (s16)gte_mfc2(GTE_IR1);
        e->f.y = (s16)gte_mfc2(GTE_IR2);
        e->f.z = (s16)gte_mfc2(GTE_IR3);
        /* FC blue keeps the start z here; the blue result is unused. */
        e->w.uv = (u16)HorizonDpcs((s16)end->w.uv, start->f.u << 4, start->f.v << 4, start->f.z);
        e->w.color = (u32)HorizonDpcs((s32)end->w.color, start->f.r << 4, start->f.g << 4, start->f.b << 4);
    }
    HorizonProjectRow(row, last);
}

/* func_80060D54 local: corner c from the tile, packed as in the records. */
static void HorizonSetCorner(int c, s32 xy, u32 z, u32 color)
{
    Horizon.corner[c].w.xy = xy;
    Horizon.corner[c].w.z = (s16)z;
    Horizon.corner[c].w.uv = 0;
    Horizon.corner[c].w.color = color;
}

/* EmitQuadLinesG0 (and its copy func_80062158): the near-tile grid of
 * (1 << levelV) x (0x1000 >> levelU) textured Gouraud pieces between the four
 * corners, with optional outline polylines. uv are the tile's four texture
 * words; the clut and tpage halves are Horizon.clut/tpage. */
static void HorizonEmitLitGridLevels(s32 levelU, s32 levelV, const u32 uv[4])
{
    HorizonVertex *cur = Horizon.rows[0], *prev = Horizon.rows[1];
    HorizonVertex *c = Horizon.corner;
    RroSceneScratchPrefix *scene = Horizon.scene;
    s32 stepRows = (s32)(0x1000u >> (levelU & 31));
    s32 stepCols = (s32)(0x1000u >> (levelV & 31));
    int last;
    s32 t;
    /* Host bound: the rows hold 16 segments (levels above 4 overran the
     * PS1 scratchpad rows). */
    if ((levelV & 31) > 4) levelV = 4;
    stepCols = (s32)(0x1000u >> levelV);
    last = 1 << levelV;

    c[0].w.uv = (u16)uv[0];
    c[1].w.uv = (u16)uv[1];
    c[2].w.uv = (u16)uv[2];
    c[3].w.uv = (u16)uv[3];
    cur[0] = c[0];
    cur[last] = c[2];
    HorizonInterpRowLit(cur, last, stepCols);
    if (!(Horizon.lineFlags & 2)) {
        if (!(Horizon.lineFlags & 1)) {
            HorizonLineF3(Horizon.sxy[0], Horizon.sxy[1], Horizon.sxy[3]);
            HorizonLineF3(Horizon.sxy[0], Horizon.sxy[2], Horizon.sxy[3]);
        }
        HorizonLineStrip(cur, last);
    }
    t = 0x1000;
    do {
        HorizonVertex *swap;
        int j;
        t -= stepRows;
        gte_mtc2(GTE_IR0, t);
        swap = cur;
        cur = prev;
        prev = swap;
        /* Row ends: corner 0 -> 1 and corner 2 -> 3. */
        CourseIntpl(c[0].f.x, c[0].f.y, c[0].f.z, c[1].f.x, c[1].f.y, c[1].f.z);
        cur[0].f.x = (s16)gte_mfc2(GTE_IR1);
        cur[0].f.y = (s16)gte_mfc2(GTE_IR2);
        cur[0].f.z = (s16)gte_mfc2(GTE_IR3);
        cur[0].w.uv = (u16)HorizonDpcs((s16)c[1].w.uv, c[0].f.u << 4, c[0].f.v << 4, c[0].f.z);
        cur[0].w.color = (u32)HorizonDpcs((s32)c[1].w.color, c[0].f.r << 4, c[0].f.g << 4, c[0].f.b << 4);
        CourseIntpl(c[2].f.x, c[2].f.y, c[2].f.z, c[3].f.x, c[3].f.y, c[3].f.z);
        cur[last].f.x = (s16)gte_mfc2(GTE_IR1);
        cur[last].f.y = (s16)gte_mfc2(GTE_IR2);
        cur[last].f.z = (s16)gte_mfc2(GTE_IR3);
        cur[last].w.uv = (u16)HorizonDpcs((s16)c[3].w.uv, c[2].f.u << 4, c[2].f.v << 4, c[2].f.z);
        cur[last].w.color = (u32)HorizonDpcs((s32)c[3].w.color, c[2].f.r << 4, c[2].f.g << 4, c[2].f.b << 4);
        HorizonInterpRowLit(cur, last, stepCols);
        if (!(Horizon.lineFlags & 2)) {
            HorizonLineF2(cur[0].w.xy, prev[0].w.xy);
            HorizonLineF2(cur[last].w.xy, prev[last].w.xy);
            if (t == 0) HorizonLineStrip(cur, last);
        }
        for (j = 0; j < last; j++) {
            const HorizonVertex *p0 = &prev[j], *p1 = &cur[j], *p2 = &prev[j + 1], *p3 = &cur[j + 1];
            s32 a, b, mirror;
            RR_GTE_LOAD_SXY(GTE_SXY0, &p0->w.xy);
            RR_GTE_LOAD_SXY(GTE_SXY1, &p1->w.xy);
            RR_GTE_LOAD_SXY(GTE_SXY2, &p2->w.xy);
            gte_cmd(GTE_NCLIP);
            if (QuadOutsideViewport(scene, ScreenX(p0->w.xy), ScreenX(p1->w.xy), ScreenX(p2->w.xy), ScreenX(p3->w.xy),
                                    ScreenY(p0->w.xy), ScreenY(p1->w.xy), ScreenY(p2->w.xy), ScreenY(p3->w.xy)))
                continue;
            a = gte_mfc2(GTE_MAC0);
            RR_GTE_LOAD_SXY(GTE_SXY0, &p3->w.xy);
            gte_cmd(GTE_NCLIP);
            b = gte_mfc2(GTE_MAC0);
            mirror = 0 < scene->renderFlags64;
            if (((0 < a) ^ mirror) | ((b < 0) ^ mirror)) {
                PolyGT4WordPacket *p = (PolyGT4WordPacket *)Horizon.cursor;
                RR_SXY_COPY(p->xy0, p0->w.xy);
                RR_SXY_COPY(p->xy1, p1->w.xy);
                RR_SXY_COPY(p->xy2, p2->w.xy);
                RR_SXY_COPY(p->xy3, p3->w.xy);
                p->uvClut0C = (s32)(((u32)Horizon.clut << 16) | p0->w.uv);
                p->uvTpage18 = (s32)(((u32)Horizon.tpage << 16) | p1->w.uv);
                p->uv2 = p2->w.uv;
                p->uv3 = p3->w.uv;
                p->color04 = (s32)p0->w.color;
                p->color10 = (s32)p1->w.color;
                p->color1C = (s32)p2->w.color;
                p->color28 = (s32)p3->w.color;
                RR_GPU_ADD_PRIM(Horizon.ot, p, 12);
                Horizon.cursor = (u8 *)(p + 1);
            }
        }
    } while (t != 0);
}

/* Level of detail from a tile's average depth (two distance tables). */
static s32 HorizonLod(s32 otz)
{
    static const s16 near[4] = {0x3F0, 0x330, 0x260, 0x180};
    static const s16 far[4] = {0x38C, 0x2CC, 0x1FC, 0x11C};
    const s16 *limits = HorizonLodSelect() != 0 ? far : near;
    int i;
    for (i = 0; i < 4; i++)
        if (!(otz < limits[i])) return 4 - i;
    return 0;
}

/* Section tile record of func_80060D54 (RelFileSection00, 64 bytes). */
typedef struct {
    s32 xy0;
    u32 z0z1;
    s32 xy1, xy2;
    u32 z2z3;
    s32 xy3;
    u32 uv[3];          /* 0x18 uv0|clut, 0x1C uv1|tpage, 0x20 uv2|uv3 */
    u16 flags24;        /* bit 0: halve far texture coordinates; high byte: OT bias */
    s16 levels26;       /* bits 0..4 columns, 5..9 rows */
    u32 farColor28;
    u32 lineColor2C;
    u32 color[4];       /* 0x30..0x3C */
} HorizonTile;
_Static_assert(sizeof(HorizonTile) == 0x40, "horizon tile");

/* func_80060D54: section 0 (ranges10[0], RelFileSection00) lit tiles. Near
 * tiles are drawn as one GT4 or as a subdivided grid, far tiles (beyond the
 * scene's far depth) as one FT4 in the tile's far colour. */
static void HorizonPassLitTiles(const VisibleCourseViews *views)
{
    RroSceneScratchPrefix *scene = Horizon.scene;
    u32 view;
    for (view = 1; HorizonView(view, views); view++) {
        const CoursePrimitiveRange *range = &views->stream[view].record->ranges10[0];
        const HorizonTile *tile = (const HorizonTile *)RelFileSection00 + range->first;
        u32 n;
        for (n = range->count; n != 0; n--, tile++) {
            s32 x[4], y[4];
            s32 area0, area1, otz, depth, lod, mirror, offscreen;
            s32 far, halve, levelU = 0, levelV = 0, subdivide = 0;
            u32 flagsGte, uvs[4];
            gte_mtc2(GTE_VXY0, tile->xy0);
            gte_mtc2(GTE_VZ0, (s32)(tile->z0z1 & 0xFFFF));
            gte_mtc2(GTE_VXY1, tile->xy1);
            gte_mtc2(GTE_VZ1, (s32)(tile->z0z1 >> 16));
            gte_mtc2(GTE_VXY2, tile->xy2);
            gte_mtc2(GTE_VZ2, (s32)(tile->z2z3 & 0xFFFF));
            HorizonSetCorner(0, tile->xy0, tile->z0z1 & 0xFFFF, tile->color[0]);
            HorizonSetCorner(1, tile->xy1, tile->z0z1 >> 16, tile->color[1]);
            HorizonSetCorner(2, tile->xy2, tile->z2z3 & 0xFFFF, tile->color[2]);
            HorizonSetCorner(3, tile->xy3, tile->z2z3 >> 16, tile->color[3]);
            gte_cmd(GTE_RTPT);
            RR_GTE_STORE_SXY(GTE_SXY0, &Horizon.sxy[0]);
            RR_GTE_STORE_SXY(GTE_SXY1, &Horizon.sxy[1]);
            RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[2]);
            flagsGte = (u32)gte_cfc2(GTE_FLAG);
            gte_cmd(GTE_NCLIP);
            area0 = gte_mfc2(GTE_MAC0);
            gte_mtc2(GTE_VXY0, tile->xy3);
            gte_mtc2(GTE_VZ0, (s32)(tile->z2z3 >> 16));
            gte_cmd(GTE_RTPS);
            Horizon.otBias = tile->flags24 >> 8;
            Horizon.lineColor = tile->lineColor2C;
            RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[3]);
            flagsGte |= (u32)gte_cfc2(GTE_FLAG);
            gte_cmd(GTE_NCLIP);
            Horizon.lineFlags = (flagsGte >> 31) | ((tile->lineColor2C >> 31) << 1);
            area1 = gte_mfc2(GTE_MAC0);
            gte_cmd(GTE_AVSZ4);
            mirror = 0 < scene->renderFlags64;
            if (!HorizonFacing(((0 < area0) ^ mirror) | ((area1 < 0) ^ mirror))) continue;
            for (lod = 0; lod < 4; lod++) {
                x[lod] = ScreenX(Horizon.sxy[lod]);
                y[lod] = ScreenY(Horizon.sxy[lod]);
            }
            offscreen = QuadOutsideViewport(scene, x[0], x[1], x[2], x[3], y[0], y[1], y[2], y[3]);
            otz = gte_mfc2(GTE_OTZ);
            far = HorizonFarDepth() < otz;
            depth = otz >> (scene->depthShift60 & 31);
            if ((u32)depth >= 0x1C0) continue;
            if (depth == 0 && !(0x122 < gte_mfc2(GTE_SZ0) || 0x122 < gte_mfc2(GTE_SZ1) ||
                                0x122 < gte_mfc2(GTE_SZ2) || 0x122 < gte_mfc2(GTE_SZ3)))
                continue;
            lod = HorizonLod(otz);
            halve = far & tile->flags24 & 1;
            if (far) {
                if (offscreen) continue;
                Horizon.corner[0].w.color = tile->farColor28;
            } else {
                s32 levels = tile->levels26 & 0x3FF;
                levelV = (levels >> 5) - lod;
                levelU = (levels & 0x1F) - lod;
                if (levelU < 0) levelU = 0;
                if (levelV < 0) levelV = 0;
                subdivide = (levelU | levelV) != 0;
                if (offscreen) continue;
            }
            Horizon.ot = HorizonOrder(depth, Horizon.otBias);
            CourseOcclusionTile(Horizon.sxy, Horizon.ot, Horizon.cursor);
            uvs[0] = tile->uv[0];
            uvs[1] = tile->uv[1];
            uvs[2] = tile->uv[2] & 0xFFFF;
            uvs[3] = tile->uv[2] >> 16;
            if (far) {
                GpuPolyFT4 *p = (GpuPolyFT4 *)Horizon.cursor;
                if (halve) {
                    uvs[0] = (uvs[0] & 0xFFFF0000) | ((uvs[0] >> 1) & 0x7F7F);
                    uvs[1] = (uvs[1] & 0xFFFF0000) | ((uvs[1] >> 1) & 0x7F7F);
                    uvs[2] = (uvs[2] >> 1) & 0x7F7F;
                    uvs[3] = (uvs[3] >> 1) & 0x7F7F;
                }
                p->words.uv0 = (s32)uvs[0];
                p->words.uv1 = (s32)uvs[1];
                p->words.uv2 = (s32)(((u32)p->words.uv2 & 0xFFFF0000) | uvs[2]);
                p->words.uv3 = (s32)(((u32)p->words.uv3 & 0xFFFF0000) | uvs[3]);
                RR_SXY_COPY(p->words.xy0, Horizon.sxy[0]);
                RR_SXY_COPY(p->words.xy1, Horizon.sxy[1]);
                RR_SXY_COPY(p->words.xy2, Horizon.sxy[2]);
                RR_SXY_COPY(p->words.xy3, Horizon.sxy[3]);
                p->words.color04 = (s32)Horizon.corner[0].w.color;
                RR_GPU_ADD_PRIM(Horizon.ot, p, 9);
                Horizon.cursor = (u8 *)(p + 1);
            } else if (subdivide) {
                Horizon.clut = (u16)(tile->uv[0] >> 16);
                Horizon.tpage = (u16)(tile->uv[1] >> 16);
                HORIZON_GRID(HorizonEmitLitGridLevels, levelU, levelV, uvs);
            } else {
                PolyGT4WordPacket *p = (PolyGT4WordPacket *)Horizon.cursor;
                p->uvClut0C = (s32)uvs[0];
                p->uvTpage18 = (s32)uvs[1];
                p->uv2 = (u16)uvs[2];
                p->uv3 = (u16)uvs[3];
                RR_SXY_COPY(p->xy0, Horizon.sxy[0]);
                RR_SXY_COPY(p->xy1, Horizon.sxy[1]);
                RR_SXY_COPY(p->xy2, Horizon.sxy[2]);
                RR_SXY_COPY(p->xy3, Horizon.sxy[3]);
                p->color04 = (s32)Horizon.corner[0].w.color;
                p->color10 = (s32)Horizon.corner[1].w.color;
                p->color1C = (s32)Horizon.corner[2].w.color;
                p->color28 = (s32)Horizon.corner[3].w.color;
                RR_GPU_ADD_PRIM(Horizon.ot, p, 12);
                Horizon.cursor = (u8 *)(p + 1);
            }
        }
    }
}

typedef void (*HorizonPass)(const VisibleCourseViews *views);

/* ---- Shared tile pipeline of the section passes 1..4 and C7d ------------ */

/* State packet (two E-commands) linked at the head of ot. */
static void HorizonEmitState(GpuDmaTag *ot, u32 command0, u32 command1)
{
    GpuDrawModePacket *p = (GpuDrawModePacket *)Horizon.cursor;
    p->command[0] = command0;
    p->command[1] = command1;
    RR_GPU_ADD_PRIM(ot, p, 2);
    Horizon.cursor = (u8 *)(p + 1);
}

/* A tile's words decoded with the section's record layout. */
typedef struct {
    s32 xy[4];
    u32 z[4];
    u32 uv[3];          /* uv0|clut, uv1|tpage, uv2|uv3 */
    u32 flags;          /* halfword: bit 0 halve far coordinates; high byte OT bias */
    s16 levels;         /* bits 0..4 columns, 5..9 rows */
    u32 lineColor;
    u32 window;         /* texture-window command (windowed sections) */
    const u32 *normal;  /* NCS-lit sections: normal XY word, Z word */
} HorizonSectionTile;

typedef struct {
    s32 depth, far, halve, subdivide, levelU, levelV;
} HorizonTileState;

/* Vertex words 0..5 of every section record: xy0, z0|z1, xy1, xy2, z2|z3, xy3. */
static void HorizonDecodeVertices(HorizonSectionTile *t, const u32 *w)
{
    t->xy[0] = (s32)w[0];
    t->xy[1] = (s32)w[2];
    t->xy[2] = (s32)w[3];
    t->xy[3] = (s32)w[5];
    t->z[0] = w[1] & 0xFFFF;
    t->z[1] = w[1] >> 16;
    t->z[2] = w[4] & 0xFFFF;
    t->z[3] = w[4] >> 16;
}

/* Corner position words only; texture coordinates zeroed, colours kept. */
static void HorizonSetCornerPositions(const HorizonSectionTile *t)
{
    int c;
    for (c = 0; c < 4; c++) {
        Horizon.corner[c].w.xy = t->xy[c];
        Horizon.corner[c].w.z = (s16)t->z[c];
        Horizon.corner[c].w.uv = 0;
    }
}

/* The common head of func_80060D54 and its copies: project, cull by facing,
 * viewport and depth, choose far/near and the subdivision levels. A normal
 * (C7d) is lit by NCS with RGBC 2C000000 right after the depth is read; the
 * result stays in RGB2. Returns 0 to skip the tile. */
static int HorizonProjectTile(const HorizonSectionTile *t, HorizonTileState *st)
{
    RroSceneScratchPrefix *scene = Horizon.scene;
    s32 area0, area1, otz, lod, mirror, offscreen;
    u32 flagsGte;
    int i;

    gte_mtc2(GTE_VXY0, t->xy[0]);
    gte_mtc2(GTE_VZ0, (s32)t->z[0]);
    gte_mtc2(GTE_VXY1, t->xy[1]);
    gte_mtc2(GTE_VZ1, (s32)t->z[1]);
    gte_mtc2(GTE_VXY2, t->xy[2]);
    gte_mtc2(GTE_VZ2, (s32)t->z[2]);
    gte_cmd(GTE_RTPT);
    Horizon.otBias = (s32)(t->flags >> 8);
    RR_GTE_STORE_SXY(GTE_SXY0, &Horizon.sxy[0]);
    RR_GTE_STORE_SXY(GTE_SXY1, &Horizon.sxy[1]);
    RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[2]);
    flagsGte = (u32)gte_cfc2(GTE_FLAG);
    gte_cmd(GTE_NCLIP);
    Horizon.lineColor = t->lineColor;
    area0 = gte_mfc2(GTE_MAC0);
    gte_mtc2(GTE_VXY0, t->xy[3]);
    gte_mtc2(GTE_VZ0, (s32)t->z[3]);
    gte_cmd(GTE_RTPS);
    RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[3]);
    flagsGte |= (u32)gte_cfc2(GTE_FLAG);
    gte_cmd(GTE_NCLIP);
    Horizon.lineFlags = (flagsGte >> 31) | ((t->lineColor >> 31) << 1);
    area1 = gte_mfc2(GTE_MAC0);
    gte_cmd(GTE_AVSZ4);
    mirror = 0 < scene->renderFlags64;
    if (!HorizonFacing(((0 < area0) ^ mirror) | ((area1 < 0) ^ mirror))) return 0;
    {
        s32 x[4], y[4];
        for (i = 0; i < 4; i++) {
            x[i] = ScreenX(Horizon.sxy[i]);
            y[i] = ScreenY(Horizon.sxy[i]);
        }
        offscreen = QuadOutsideViewport(scene, x[0], x[1], x[2], x[3], y[0], y[1], y[2], y[3]);
    }
    otz = gte_mfc2(GTE_OTZ);
    st->far = HorizonFarDepth() < otz;
    if (t->normal != NULL) {
        gte_mtc2(GTE_VXY0, (s32)t->normal[0]);
        gte_mtc2(GTE_VZ0, (s32)t->normal[1]);
        gte_mtc2(GTE_RGBC, 0x2C000000);
        gte_cmd(GTE_NCS);
    }
    st->depth = otz >> (scene->depthShift60 & 31);
    if ((u32)st->depth >= 0x1C0) return 0;
    if (st->depth == 0 && !(0x122 < gte_mfc2(GTE_SZ0) || 0x122 < gte_mfc2(GTE_SZ1) ||
                            0x122 < gte_mfc2(GTE_SZ2) || 0x122 < gte_mfc2(GTE_SZ3)))
        return 0;
    lod = HorizonLod(otz);
    st->halve = st->far & (s32)(t->flags & 1);
    st->levelU = st->levelV = 0;
    st->subdivide = 0;
    if (!st->far) {
        s32 levels = t->levels & 0x3FF;
        st->levelV = (levels >> 5) - lod;
        st->levelU = (levels & 0x1F) - lod;
        if (st->levelU < 0) st->levelU = 0;
        if (st->levelV < 0) st->levelV = 0;
        st->subdivide = (st->levelU | st->levelV) != 0;
    }
    if (offscreen) return 0;
    Horizon.ot = HorizonOrder(st->depth, Horizon.otBias);
    CourseOcclusionTile(Horizon.sxy, Horizon.ot, Horizon.cursor);
    return 1;
}

/* Texture words of the packets; halved for far tiles that ask for it. */
static void HorizonTileUVs(const HorizonSectionTile *t, int halve, u32 uvs[4], u32 *window)
{
    uvs[0] = t->uv[0];
    uvs[1] = t->uv[1];
    uvs[2] = t->uv[2] & 0xFFFF;
    uvs[3] = t->uv[2] >> 16;
    *window = t->window;
    if (halve) {
        *window = ((t->window >> 1) & 0x7BDEF) | 0xE2000000;
        uvs[0] = (uvs[0] & 0xFFFF0000) | ((uvs[0] >> 1) & 0x7F7F);
        uvs[1] = (uvs[1] & 0xFFFF0000) | ((uvs[1] >> 1) & 0x7F7F);
        uvs[2] = (uvs[2] >> 1) & 0x7F7F;
        uvs[3] = (uvs[3] >> 1) & 0x7F7F;
    }
}

/* One FT4 in colour; windowed sections enclose it in texture-window state
 * packets (drawn: E2000000/window, the quad, E2000000/0). */
static void HorizonEmitTileFT4(const u32 uvs[4], u32 color, int windowed, u32 window)
{
    GpuPolyFT4 *p;
    if (windowed) HorizonEmitState(Horizon.ot, 0xE2000000, 0);
    p = (GpuPolyFT4 *)Horizon.cursor;
    p->words.uv0 = (s32)uvs[0];
    p->words.uv1 = (s32)uvs[1];
    p->fields.u2 = (u8)uvs[2];
    p->fields.v2 = (u8)(uvs[2] >> 8);
    p->fields.u3 = (u8)uvs[3];
    p->fields.v3 = (u8)(uvs[3] >> 8);
    RR_SXY_COPY(p->words.xy0, Horizon.sxy[0]);
    RR_SXY_COPY(p->words.xy1, Horizon.sxy[1]);
    RR_SXY_COPY(p->words.xy2, Horizon.sxy[2]);
    RR_SXY_COPY(p->words.xy3, Horizon.sxy[3]);
    p->words.color04 = (s32)color;
    RR_GPU_ADD_PRIM(Horizon.ot, p, 9);
    Horizon.cursor = (u8 *)(p + 1);
    if (windowed) HorizonEmitState(Horizon.ot, 0xE2000000, window);
}

/* One GT4 in the four corner colours, optionally windowed as above. */
static void HorizonEmitTileGT4(const u32 uvs[4], const u32 colors[4], int windowed, u32 window)
{
    PolyGT4WordPacket *p;
    if (windowed) HorizonEmitState(Horizon.ot, 0xE2000000, 0);
    p = (PolyGT4WordPacket *)Horizon.cursor;
    p->uvClut0C = (s32)uvs[0];
    p->uvTpage18 = (s32)uvs[1];
    p->uv2 = (u16)uvs[2];
    p->uv3 = (u16)uvs[3];
    RR_SXY_COPY(p->xy0, Horizon.sxy[0]);
    RR_SXY_COPY(p->xy1, Horizon.sxy[1]);
    RR_SXY_COPY(p->xy2, Horizon.sxy[2]);
    RR_SXY_COPY(p->xy3, Horizon.sxy[3]);
    p->color04 = (s32)colors[0];
    p->color10 = (s32)colors[1];
    p->color1C = (s32)colors[2];
    p->color28 = (s32)colors[3];
    RR_GPU_ADD_PRIM(Horizon.ot, p, 12);
    Horizon.cursor = (u8 *)(p + 1);
    if (windowed) HorizonEmitState(Horizon.ot, 0xE2000000, window);
}

/* ---- Flat (single colour) subdivision: InterpEdgeVertsB*, func_80062C60 */

/* InterpEdgeVertsB0..B5: as HorizonInterpRowLit without colours. */
static void HorizonInterpRowFlat(HorizonVertex *row, int last, s32 step)
{
    const HorizonVertex *end = &row[last];
    const HorizonVertex *start = &row[0];
    s32 t = 0x1000 - step;
    int i = 1;
    for (; t != 0; t -= step, i++) {
        HorizonVertex *e = &row[i];
        gte_mtc2(GTE_IR0, t);
        CourseIntpl(start->f.x, start->f.y, start->f.z, end->f.x, end->f.y, end->f.z);
        e->f.x = (s16)gte_mfc2(GTE_IR1);
        e->f.y = (s16)gte_mfc2(GTE_IR2);
        e->f.z = (s16)gte_mfc2(GTE_IR3);
        /* FC blue keeps the start z; the blue result is unused. */
        e->w.uv = (u16)HorizonDpcs((s16)end->w.uv, start->f.u << 4, start->f.v << 4, start->f.z);
    }
    HorizonProjectRow(row, last);
}

/* Row end interpolated between corners a and b: position and uv only. */
static void HorizonFlatRowEnd(HorizonVertex *e, const HorizonVertex *a, const HorizonVertex *b)
{
    CourseIntpl(a->f.x, a->f.y, a->f.z, b->f.x, b->f.y, b->f.z);
    e->f.x = (s16)gte_mfc2(GTE_IR1);
    e->f.y = (s16)gte_mfc2(GTE_IR2);
    e->f.z = (s16)gte_mfc2(GTE_IR3);
    e->w.uv = (u16)HorizonDpcs((s16)b->w.uv, a->f.u << 4, a->f.v << 4, a->f.z);
}

/* func_80062C60 (identical copies func_80063780, EmitQuadLinesG9): grid of
 * FT4 pieces in corner 0's colour. Contract as HorizonEmitLitGrid; the grid
 * rows keep their previous colour words. */
static void HorizonEmitFlatGridLevels(s32 levelU, s32 levelV, const u32 uv[4])
{
    HorizonVertex *cur = Horizon.rows[0], *prev = Horizon.rows[1];
    HorizonVertex *c = Horizon.corner;
    RroSceneScratchPrefix *scene = Horizon.scene;
    s32 stepRows = (s32)(0x1000u >> (levelU & 31));
    s32 stepCols;
    int last;
    s32 t;
    /* Host bound, as HorizonEmitLitGrid. */
    if ((levelV & 31) > 4) levelV = 4;
    stepCols = (s32)(0x1000u >> levelV);
    last = 1 << levelV;

    c[0].w.uv = (u16)uv[0];
    c[1].w.uv = (u16)uv[1];
    c[2].w.uv = (u16)uv[2];
    c[3].w.uv = (u16)uv[3];
    cur[0].w.xy = c[0].w.xy;
    cur[0].w.z = c[0].w.z;
    cur[0].w.uv = c[0].w.uv;
    cur[last].w.xy = c[2].w.xy;
    cur[last].w.z = c[2].w.z;
    cur[last].w.uv = c[2].w.uv;
    HorizonInterpRowFlat(cur, last, stepCols);
    if (!(Horizon.lineFlags & 2)) {
        if (!(Horizon.lineFlags & 1)) {
            HorizonLineF3(Horizon.sxy[0], Horizon.sxy[1], Horizon.sxy[3]);
            HorizonLineF3(Horizon.sxy[0], Horizon.sxy[2], Horizon.sxy[3]);
        }
        HorizonLineStrip(cur, last);
    }
    t = 0x1000;
    do {
        HorizonVertex *swap;
        int j;
        t -= stepRows;
        gte_mtc2(GTE_IR0, t);
        swap = cur;
        cur = prev;
        prev = swap;
        HorizonFlatRowEnd(&cur[0], &c[0], &c[1]);
        HorizonFlatRowEnd(&cur[last], &c[2], &c[3]);
        HorizonInterpRowFlat(cur, last, stepCols);
        if (!(Horizon.lineFlags & 2)) {
            HorizonLineF2(cur[0].w.xy, prev[0].w.xy);
            HorizonLineF2(cur[last].w.xy, prev[last].w.xy);
            if (t == 0) HorizonLineStrip(cur, last);
        }
        for (j = 0; j < last; j++) {
            const HorizonVertex *p0 = &prev[j], *p1 = &cur[j], *p2 = &prev[j + 1], *p3 = &cur[j + 1];
            s32 a, b, mirror;
            RR_GTE_LOAD_SXY(GTE_SXY0, &p0->w.xy);
            RR_GTE_LOAD_SXY(GTE_SXY1, &p1->w.xy);
            RR_GTE_LOAD_SXY(GTE_SXY2, &p2->w.xy);
            gte_cmd(GTE_NCLIP);
            if (QuadOutsideViewport(scene, ScreenX(p0->w.xy), ScreenX(p1->w.xy), ScreenX(p2->w.xy), ScreenX(p3->w.xy),
                                    ScreenY(p0->w.xy), ScreenY(p1->w.xy), ScreenY(p2->w.xy), ScreenY(p3->w.xy)))
                continue;
            a = gte_mfc2(GTE_MAC0);
            RR_GTE_LOAD_SXY(GTE_SXY0, &p3->w.xy);
            gte_cmd(GTE_NCLIP);
            b = gte_mfc2(GTE_MAC0);
            mirror = 0 < scene->renderFlags64;
            if (((0 < a) ^ mirror) | ((b < 0) ^ mirror)) {
                GpuPolyFT4 *p = (GpuPolyFT4 *)Horizon.cursor;
                RR_SXY_COPY(p->words.xy0, p0->w.xy);
                RR_SXY_COPY(p->words.xy1, p1->w.xy);
                RR_SXY_COPY(p->words.xy2, p2->w.xy);
                RR_SXY_COPY(p->words.xy3, p3->w.xy);
                p->words.uv0 = (s32)(((u32)Horizon.clut << 16) | p0->w.uv);
                p->words.uv1 = (s32)(((u32)Horizon.tpage << 16) | p1->w.uv);
                p->fields.u2 = (u8)p2->w.uv;
                p->fields.v2 = (u8)(p2->w.uv >> 8);
                p->fields.u3 = (u8)p3->w.uv;
                p->fields.v3 = (u8)(p3->w.uv >> 8);
                p->words.color04 = (s32)c[0].w.color;
                RR_GPU_ADD_PRIM(Horizon.ot, p, 9);
                Horizon.cursor = (u8 *)(p + 1);
            }
        }
    } while (t != 0);
}

/* ---- Lit tile sections (DrawWireQuadG1, func_80063B40) ----------------- */

/* Record: vertex words 0..5, uv words 6..8, flags/levels word 9, far colour
 * 0x28, line colour 0x2C, corner colours 0x30..0x3C, [window 0x40]. Fogged
 * sections depth-cue every colour toward the saved far colour. */
static void HorizonLitSection(const VisibleCourseViews *views, int section, const u8 *base,
                              u32 stride, int windowed, int fogged)
{
    u32 view;
    if (fogged) {
        gte_ctc2(GTE_FC_R, Horizon.savedFarColor[0]);
        gte_ctc2(GTE_FC_G, Horizon.savedFarColor[1]);
        gte_ctc2(GTE_FC_B, Horizon.savedFarColor[2]);
    }
    for (view = 1; HorizonView(view, views); view++) {
        const CoursePrimitiveRange *range = &views->stream[view].record->ranges10[section];
        const u8 *record = base + (size_t)range->first * stride;
        u32 n;
        for (n = range->count; n != 0; n--, record += stride) {
            const u32 *w = (const u32 *)record;
            HorizonSectionTile t;
            HorizonTileState st;
            u32 uvs[4], window;
            int c;
            HorizonDecodeVertices(&t, w);
            t.uv[0] = w[6];
            t.uv[1] = w[7];
            t.uv[2] = w[8];
            t.flags = w[9] & 0xFFFF;
            t.levels = (s16)(w[9] >> 16);
            t.lineColor = w[11];
            t.window = windowed ? w[16] : 0;
            t.normal = NULL;
            HorizonSetCornerPositions(&t);
            for (c = 0; c < 4; c++) Horizon.corner[c].w.color = w[12 + c];
            if (!HorizonProjectTile(&t, &st)) continue;
            if (st.far) Horizon.corner[0].w.color = w[10];
            HorizonTileUVs(&t, st.halve, uvs, &window);
            if (fogged) {
                /* IR0 is still the depth cue of the fourth vertex. */
                gte_mtc2(GTE_RGBC, (s32)Horizon.corner[0].w.color);
                gte_cmd(GTE_DPCS);
            }
            if (st.far) {
                u32 color = fogged ? (u32)gte_mfc2(GTE_RGB2) : Horizon.corner[0].w.color;
                HorizonEmitTileFT4(uvs, color, windowed, window);
                continue;
            }
            if (fogged) {
                /* Corner 0 takes the cued colour; corners 1..3 go through
                 * the colour FIFO and DPCT. */
                Horizon.corner[0].w.color = (u32)gte_mfc2(GTE_RGB2);
                gte_mtc2(GTE_RGB0, (s32)Horizon.corner[1].w.color);
                gte_mtc2(GTE_RGB1, (s32)Horizon.corner[2].w.color);
                gte_mtc2(GTE_RGB2, (s32)Horizon.corner[3].w.color);
                gte_cmd(GTE_DPCT);
            }
            if (st.subdivide) {
                Horizon.clut = (u16)(t.uv[0] >> 16);
                Horizon.tpage = (u16)(t.uv[1] >> 16);
                if (windowed) HorizonEmitState(Horizon.ot, 0xE2000000, 0);
                if (fogged) {
                    /* func_800642D8: the cued colours become corners 1..3
                     * and the far colour is restored for the next tile. */
                    Horizon.corner[1].w.color = (u32)gte_mfc2(GTE_RGB0);
                    Horizon.corner[2].w.color = (u32)gte_mfc2(GTE_RGB1);
                    Horizon.corner[3].w.color = (u32)gte_mfc2(GTE_RGB2);
                }
                HORIZON_GRID(HorizonEmitLitGridLevels, st.levelU, st.levelV, uvs);
                if (fogged) {
                    gte_ctc2(GTE_FC_R, Horizon.savedFarColor[0]);
                    gte_ctc2(GTE_FC_G, Horizon.savedFarColor[1]);
                    gte_ctc2(GTE_FC_B, Horizon.savedFarColor[2]);
                }
                if (windowed) HorizonEmitState(Horizon.ot, t.window, 0);
            } else {
                u32 colors[4];
                colors[0] = Horizon.corner[0].w.color;
                if (fogged) {
                    colors[1] = (u32)gte_mfc2(GTE_RGB0);
                    colors[2] = (u32)gte_mfc2(GTE_RGB1);
                    colors[3] = (u32)gte_mfc2(GTE_RGB2);
                } else {
                    colors[1] = Horizon.corner[1].w.color;
                    colors[2] = Horizon.corner[2].w.color;
                    colors[3] = Horizon.corner[3].w.color;
                }
                HorizonEmitTileGT4(uvs, colors, windowed, t.window);
            }
        }
    }
}

/* DrawWireQuadG1: section 1 (RelFileSection01), 0x44-byte lit tiles with a
 * texture window. */
static void HorizonPass1(const VisibleCourseViews *views)
{
    HorizonLitSection(views, 1, RelFileSection01, 0x44, 1, 0);
}

/* func_80063B40: section 4 (TrackRelSegment4Ptr), 0x40-byte lit tiles
 * depth-cued toward the far colour. */
static void HorizonPass4(const VisibleCourseViews *views)
{
    HorizonLitSection(views, 4, TrackRelSegment4Ptr, 0x40, 0, 1);
}

/* ---- Flat tile sections (func_800625A4, DrawWireQuadG3, func_8006B0C8) - */

enum { HORIZON_FLAT_PLAIN, HORIZON_FLAT_WINDOWED, HORIZON_FLAT_LIT_WINDOWED };

/* Plain/windowed record: vertex words 0..5, uv words 6..8, flags/levels word
 * 9, line colour 0x28, colour 0x2C, [window 0x30]. Lit record (C7d): vertex
 * words, normal 0x18/0x1C, uv words 8..10, flags/levels word 11, line colour
 * 0x30, window 0x34; its colour is the NCS-lit normal. */
static void HorizonFlatSection(const VisibleCourseViews *views, int section, const u8 *base,
                               u32 stride, int kind)
{
    int windowed = kind != HORIZON_FLAT_PLAIN;
    int lit = kind == HORIZON_FLAT_LIT_WINDOWED;
    u32 view;
    for (view = 1; HorizonView(view, views); view++) {
        const CoursePrimitiveRange *range = &views->stream[view].record->ranges10[section];
        const u8 *record = base + (size_t)range->first * stride;
        u32 n;
        for (n = range->count; n != 0; n--, record += stride) {
            const u32 *w = (const u32 *)record;
            const u32 *attr = lit ? w + 8 : w + 6;
            HorizonSectionTile t;
            HorizonTileState st;
            u32 uvs[4], window;
            HorizonDecodeVertices(&t, w);
            t.uv[0] = attr[0];
            t.uv[1] = attr[1];
            t.uv[2] = attr[2];
            t.flags = attr[3] & 0xFFFF;
            t.levels = (s16)(attr[3] >> 16);
            t.lineColor = attr[4];
            t.window = windowed ? (lit ? w[13] : w[12]) : 0;
            t.normal = lit ? w + 6 : NULL;
            HorizonSetCornerPositions(&t);
            if (!lit) Horizon.corner[0].w.color = w[11];
            if (!HorizonProjectTile(&t, &st)) continue;
            if (lit) Horizon.corner[0].w.color = (u32)gte_mfc2(GTE_RGB2);
            HorizonTileUVs(&t, st.halve, uvs, &window);
            if (!st.far && st.subdivide) {
                Horizon.clut = (u16)(t.uv[0] >> 16);
                Horizon.tpage = (u16)(t.uv[1] >> 16);
                if (lit) {
                    /* Lines take the line colour scaled by twice the lit
                     * colour (DPCL with IR0 0). */
                    gte_mtc2(GTE_RGBC, (s32)Horizon.lineColor);
                    gte_mtc2(GTE_IR0, 0);
                    gte_mtc2(GTE_IR1, Horizon.corner[0].f.r << 5);
                    gte_mtc2(GTE_IR2, Horizon.corner[0].f.g << 5);
                    gte_mtc2(GTE_IR3, Horizon.corner[0].f.b << 5);
                    gte_cmd(GTE_DPCL);
                }
                if (windowed) HorizonEmitState(Horizon.ot, 0xE2000000, 0);
                if (lit) Horizon.lineColor = (u32)gte_mfc2(GTE_RGB2);
                HORIZON_GRID(HorizonEmitFlatGridLevels, st.levelU, st.levelV, uvs);
                if (windowed) HorizonEmitState(Horizon.ot, t.window, 0);
                continue;
            }
            HorizonEmitTileFT4(uvs, Horizon.corner[0].w.color, windowed, window);
        }
    }
}

/* func_800625A4: section 2 (TrackRelSegment2Ptr), 0x30-byte flat tiles. */
static void HorizonPass2(const VisibleCourseViews *views)
{
    HorizonFlatSection(views, 2, TrackRelSegment2Ptr, 0x30, HORIZON_FLAT_PLAIN);
}

/* DrawWireQuadG3: section 3 (RelFileOffsetPtr3), 0x34-byte flat tiles with a
 * texture window. */
static void HorizonPass3(const VisibleCourseViews *views)
{
    HorizonFlatSection(views, 3, RelFileOffsetPtr3, 0x34, HORIZON_FLAT_WINDOWED);
}

/* func_8006B0C8: course 7 section 14 (TrackSectionBoundsPtr), 0x38-byte
 * NCS-lit windowed tiles. */
static void HorizonPassC7d(const VisibleCourseViews *views)
{
    HorizonFlatSection(views, 14, TrackSectionBoundsPtr, 0x38, HORIZON_FLAT_LIT_WINDOWED);
}

/* ---- Passes 5..8 (DrawWireQuadG5, func_80065420, func_80065ED8,
 * func_80066A34) and the unreferenced DrawWireQuadVerts1. ---- */

/* Two-word texture-window state packet (both E2-family words). */
static void HorizonEmitDrawMode(u32 first, u32 second)
{
    GpuDrawModePacket *p = (GpuDrawModePacket *)Horizon.cursor;
    p->command[0] = first;
    p->command[1] = second;
    RR_GPU_ADD_PRIM(Horizon.ot, p, 2);
    Horizon.cursor = (u8 *)(p + 1);
}

static void HorizonRestoreFarColor(void)
{
    gte_ctc2(GTE_FC_R, Horizon.savedFarColor[0]);
    gte_ctc2(GTE_FC_G, Horizon.savedFarColor[1]);
    gte_ctc2(GTE_FC_B, Horizon.savedFarColor[2]);
}

/* Front end shared by the tile passes (copies of func_80060D54's): project
 * the four corners, latch the screen positions, line flags, OT bias and line
 * colour, and test either triangle for facing. Returns nonzero if visible. */
static int HorizonTileProject6(s32 xy0, u32 z0z1, s32 xy1, s32 xy2, u32 z2z3, s32 xy3,
                               u32 flags24, u32 lineColor)
{
    s32 area0, area1, mirror;
    u32 flagsGte;
    gte_mtc2(GTE_VXY0, xy0);
    gte_mtc2(GTE_VZ0, (s32)(z0z1 & 0xFFFF));
    gte_mtc2(GTE_VXY1, xy1);
    gte_mtc2(GTE_VZ1, (s32)(z0z1 >> 16));
    gte_mtc2(GTE_VXY2, xy2);
    gte_mtc2(GTE_VZ2, (s32)(z2z3 & 0xFFFF));
    HorizonSetCorner(0, xy0, z0z1 & 0xFFFF, Horizon.corner[0].w.color);
    HorizonSetCorner(1, xy1, z0z1 >> 16, Horizon.corner[1].w.color);
    HorizonSetCorner(2, xy2, z2z3 & 0xFFFF, Horizon.corner[2].w.color);
    HorizonSetCorner(3, xy3, z2z3 >> 16, Horizon.corner[3].w.color);
    gte_cmd(GTE_RTPT);
    RR_GTE_STORE_SXY(GTE_SXY0, &Horizon.sxy[0]);
    RR_GTE_STORE_SXY(GTE_SXY1, &Horizon.sxy[1]);
    RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[2]);
    flagsGte = (u32)gte_cfc2(GTE_FLAG);
    gte_cmd(GTE_NCLIP);
    area0 = gte_mfc2(GTE_MAC0);
    gte_mtc2(GTE_VXY0, xy3);
    gte_mtc2(GTE_VZ0, (s32)(z2z3 >> 16));
    gte_cmd(GTE_RTPS);
    Horizon.otBias = flags24 >> 8;
    Horizon.lineColor = lineColor;
    RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[3]);
    flagsGte |= (u32)gte_cfc2(GTE_FLAG);
    gte_cmd(GTE_NCLIP);
    Horizon.lineFlags = (flagsGte >> 31) | ((lineColor >> 31) << 1);
    area1 = gte_mfc2(GTE_MAC0);
    gte_cmd(GTE_AVSZ4);
    mirror = 0 < Horizon.scene->renderFlags64;
    return HorizonFacing(((0 < area0) ^ mirror) | ((area1 < 0) ^ mirror));
}

typedef struct {
    s32 far, halve, subdivide, levelU, levelV;
} HorizonTileClass6;

/* Depth classification after the facing test: viewport, depth range, the
 * near-plane SZ check, level of detail and far/near split. Sets Horizon.ot.
 * Returns zero to skip the tile. */
static int HorizonTileClassify6(u32 flags24, s16 levels26, HorizonTileClass6 *c)
{
    RroSceneScratchPrefix *scene = Horizon.scene;
    s32 otz, depth, lod, offscreen;
    offscreen = ScreenQuadOutsideViewport(scene, Horizon.sxy);
    otz = gte_mfc2(GTE_OTZ);
    c->far = HorizonFarDepth() < otz;
    depth = otz >> (scene->depthShift60 & 31);
    if ((u32)depth >= 0x1C0) return 0;
    if (depth == 0 && !(0x122 < gte_mfc2(GTE_SZ0) || 0x122 < gte_mfc2(GTE_SZ1) ||
                        0x122 < gte_mfc2(GTE_SZ2) || 0x122 < gte_mfc2(GTE_SZ3)))
        return 0;
    lod = HorizonLod(otz);
    c->halve = c->far & flags24 & 1;
    c->subdivide = 0;
    c->levelU = c->levelV = 0;
    if (!c->far) {
        s32 levels = levels26 & 0x3FF;
        c->levelV = (levels >> 5) - lod;
        c->levelU = (levels & 0x1F) - lod;
        if (c->levelU < 0) c->levelU = 0;
        if (c->levelV < 0) c->levelV = 0;
        c->subdivide = (c->levelU | c->levelV) != 0;
    }
    if (offscreen) return 0;
    Horizon.ot = HorizonOrder(depth, Horizon.otBias);
    CourseOcclusionTile(Horizon.sxy, Horizon.ot, Horizon.cursor);
    return 1;
}

/* Far-tile texture coordinate halving (bit 0 of the tile flags). */
static void HorizonHalveUV6(u32 uvs[4])
{
    uvs[0] = (uvs[0] & 0xFFFF0000) | ((uvs[0] >> 1) & 0x7F7F);
    uvs[1] = (uvs[1] & 0xFFFF0000) | ((uvs[1] >> 1) & 0x7F7F);
    uvs[2] = (uvs[2] >> 1) & 0x7F7F;
    uvs[3] = (uvs[3] >> 1) & 0x7F7F;
}

/* Halved texture window: ((window >> 1) & 0x7BDEF) | E2000000. */
static u32 HorizonHalveWindow6(u32 window)
{
    return ((window >> 1) & 0x7BDEF) | 0xE2000000;
}

static void HorizonEmitPlainFT4(const u32 uvs[4], u32 color)
{
    GpuPolyFT4 *p = (GpuPolyFT4 *)Horizon.cursor;
    p->words.uv0 = (s32)uvs[0];
    p->words.uv1 = (s32)uvs[1];
    p->fields.u2 = (u8)uvs[2];
    p->fields.v2 = (u8)(uvs[2] >> 8);
    p->fields.u3 = (u8)uvs[3];
    p->fields.v3 = (u8)(uvs[3] >> 8);
    RR_SXY_COPY(p->words.xy0, Horizon.sxy[0]);
    RR_SXY_COPY(p->words.xy1, Horizon.sxy[1]);
    RR_SXY_COPY(p->words.xy2, Horizon.sxy[2]);
    RR_SXY_COPY(p->words.xy3, Horizon.sxy[3]);
    p->words.color04 = (s32)color;
    RR_GPU_ADD_PRIM(Horizon.ot, p, 9);
    Horizon.cursor = (u8 *)(p + 1);
}

static void HorizonEmitPlainGT4(const u32 uvs[4], const u32 color[4])
{
    PolyGT4WordPacket *p = (PolyGT4WordPacket *)Horizon.cursor;
    p->uvClut0C = (s32)uvs[0];
    p->uvTpage18 = (s32)uvs[1];
    p->uv2 = (u16)uvs[2];
    p->uv3 = (u16)uvs[3];
    RR_SXY_COPY(p->xy0, Horizon.sxy[0]);
    RR_SXY_COPY(p->xy1, Horizon.sxy[1]);
    RR_SXY_COPY(p->xy2, Horizon.sxy[2]);
    RR_SXY_COPY(p->xy3, Horizon.sxy[3]);
    p->color04 = (s32)color[0];
    p->color10 = (s32)color[1];
    p->color1C = (s32)color[2];
    p->color28 = (s32)color[3];
    RR_GPU_ADD_PRIM(Horizon.ot, p, 12);
    Horizon.cursor = (u8 *)(p + 1);
}

/* func_80065B04 / EmitQuadLinesG7: func_80062C60 followed (when asked) by a
 * restore of the saved far colour. */
static void HorizonEmitFlatGrid6(s32 levelU, s32 levelV, const u32 uv[4], int restoreFarColor)
{
    HORIZON_GRID(HorizonEmitFlatGridLevels, levelU, levelV, uv);
    if (restoreFarColor) HorizonRestoreFarColor();
}

/* Section 5 record (RelFileSection05, 68 bytes): a lit tile plus a texture
 * window command. */
typedef struct {
    HorizonTile tile;
    u32 window40;
} HorizonWindowTile5;
_Static_assert(sizeof(HorizonWindowTile5) == 0x44, "section 5 tile");

/* DrawWireQuadG5: section 5 (ranges10[5]) lit tiles drawn inside their
 * texture window. The tile colours are depth-cued (corner 0 by DPCS, corners
 * 1..3 by DPCT); far tiles use the far colour. Each tile is bracketed by a
 * window packet and a window-reset packet. The far colour is restored at the
 * start of the pass and after each subdivided grid only, so a grid's INTPL
 * target stays in FC for nothing else in between. */
static void HorizonPass5(const VisibleCourseViews *views)
{
    u32 view;
    HorizonRestoreFarColor();
    for (view = 1; HorizonView(view, views); view++) {
        const CoursePrimitiveRange *range = &views->stream[view].record->ranges10[5];
        const HorizonWindowTile5 *rec = (const HorizonWindowTile5 *)RelFileSection05 + range->first;
        u32 n;
        for (n = range->count; n != 0; n--, rec++) {
            const HorizonTile *tile = &rec->tile;
            HorizonTileClass6 c;
            u32 uvs[4], colors[4];
            Horizon.corner[0].w.color = tile->color[0];
            Horizon.corner[1].w.color = tile->color[1];
            Horizon.corner[2].w.color = tile->color[2];
            Horizon.corner[3].w.color = tile->color[3];
            if (!HorizonTileProject6(tile->xy0, tile->z0z1, tile->xy1, tile->xy2, tile->z2z3, tile->xy3,
                                     tile->flags24, tile->lineColor2C))
                continue;
            if (!HorizonTileClassify6(tile->flags24, tile->levels26, &c)) continue;
            if (c.far) Horizon.corner[0].w.color = tile->farColor28;
            gte_mtc2(GTE_RGBC, (s32)Horizon.corner[0].w.color);
            gte_cmd(GTE_DPCS);
            uvs[0] = tile->uv[0];
            uvs[1] = tile->uv[1];
            uvs[2] = tile->uv[2] & 0xFFFF;
            uvs[3] = tile->uv[2] >> 16;
            if (c.far) {
                u32 window = rec->window40;
                u32 color = (u32)gte_mfc2(GTE_RGB2);
                if (c.halve) {
                    window = HorizonHalveWindow6(window);
                    HorizonHalveUV6(uvs);
                }
                HorizonEmitDrawMode(0xE2000000, 0);
                HorizonEmitPlainFT4(uvs, color);
                HorizonEmitDrawMode(0xE2000000, window);
                continue;
            }
            Horizon.corner[0].w.color = (u32)gte_mfc2(GTE_RGB2);
            gte_mtc2(GTE_RGB0, (s32)Horizon.corner[1].w.color);
            gte_mtc2(GTE_RGB1, (s32)Horizon.corner[2].w.color);
            gte_mtc2(GTE_RGB2, (s32)Horizon.corner[3].w.color);
            gte_cmd(GTE_DPCT);
            if (c.subdivide) {
                Horizon.clut = (u16)(tile->uv[0] >> 16);
                Horizon.tpage = (u16)(tile->uv[1] >> 16);
                HorizonEmitDrawMode(0xE2000000, 0);
                Horizon.corner[1].w.color = (u32)gte_mfc2(GTE_RGB0);
                Horizon.corner[2].w.color = (u32)gte_mfc2(GTE_RGB1);
                Horizon.corner[3].w.color = (u32)gte_mfc2(GTE_RGB2);
                /* EmitQuadLinesG5 = EmitQuadLinesG0 + far-colour restore. */
                HORIZON_GRID(HorizonEmitLitGridLevels, c.levelU, c.levelV, uvs);
                HorizonRestoreFarColor();
                HorizonEmitDrawMode(rec->window40, 0);
            } else {
                colors[0] = Horizon.corner[0].w.color;
                colors[1] = (u32)gte_mfc2(GTE_RGB0);
                colors[2] = (u32)gte_mfc2(GTE_RGB1);
                colors[3] = (u32)gte_mfc2(GTE_RGB2);
                HorizonEmitDrawMode(0xE2000000, 0);
                HorizonEmitPlainGT4(uvs, colors);
                HorizonEmitDrawMode(0xE2000000, rec->window40);
            }
        }
    }
}

/* Section 6/7 records: a flat-coloured tile (48 bytes), optionally followed
 * by a texture window command (52 bytes). */
typedef struct {
    s32 xy0;
    u32 z0z1;
    s32 xy1, xy2;
    u32 z2z3;
    s32 xy3;
    u32 uv[3];          /* 0x18 uv0|clut, 0x1C uv1|tpage, 0x20 uv2|uv3 */
    u16 flags24;        /* bit 0: halve far texture coordinates; high byte: OT bias */
    s16 levels26;       /* bits 0..4 columns, 5..9 rows */
    u32 lineColor28;
    u32 color2C;
} HorizonFlatTile6;
typedef struct {
    HorizonFlatTile6 tile;
    u32 window30;
} HorizonFlatWindowTile7;
_Static_assert(sizeof(HorizonFlatTile6) == 0x30, "section 6 tile");
_Static_assert(sizeof(HorizonFlatWindowTile7) == 0x34, "section 7 tile");

/* Shared body of func_80065420 (window == NULL) and func_80065ED8. */
static void HorizonFlatTile(const HorizonFlatTile6 *tile, const u32 *window)
{
    HorizonTileClass6 c;
    u32 uvs[4];
    Horizon.corner[0].w.color = tile->color2C;
    if (!HorizonTileProject6(tile->xy0, tile->z0z1, tile->xy1, tile->xy2, tile->z2z3, tile->xy3,
                             tile->flags24, tile->lineColor28))
        return;
    if (!HorizonTileClassify6(tile->flags24, tile->levels26, &c)) return;
    gte_mtc2(GTE_RGBC, (s32)Horizon.corner[0].w.color);
    gte_cmd(GTE_DPCS);
    uvs[0] = tile->uv[0];
    uvs[1] = tile->uv[1];
    uvs[2] = tile->uv[2] & 0xFFFF;
    uvs[3] = tile->uv[2] >> 16;
    Horizon.corner[0].w.color = (u32)gte_mfc2(GTE_RGB2);
    if (c.far) {
        u32 win = window != NULL ? *window : 0;
        if (c.halve) {
            if (window != NULL) win = HorizonHalveWindow6(win);
            HorizonHalveUV6(uvs);
        }
        if (window != NULL) HorizonEmitDrawMode(0xE2000000, 0);
        HorizonEmitPlainFT4(uvs, Horizon.corner[0].w.color);
        if (window != NULL) HorizonEmitDrawMode(0xE2000000, win);
    } else if (c.subdivide) {
        Horizon.clut = (u16)(tile->uv[0] >> 16);
        Horizon.tpage = (u16)(tile->uv[1] >> 16);
        if (window != NULL) HorizonEmitDrawMode(0xE2000000, 0);
        HorizonEmitFlatGrid6(c.levelU, c.levelV, uvs, 1);
        if (window != NULL) HorizonEmitDrawMode(*window, 0);
    } else {
        if (window != NULL) HorizonEmitDrawMode(0xE2000000, 0);
        HorizonEmitPlainFT4(uvs, Horizon.corner[0].w.color);
        if (window != NULL) HorizonEmitDrawMode(0xE2000000, *window);
    }
}

/* func_80065420: section 6 (ranges10[6], RelFileOffsetPtr6) flat tiles,
 * depth-cued, subdivided near the camera. */
static void HorizonPass6(const VisibleCourseViews *views)
{
    u32 view;
    HorizonRestoreFarColor();
    for (view = 1; HorizonView(view, views); view++) {
        const CoursePrimitiveRange *range = &views->stream[view].record->ranges10[6];
        const HorizonFlatTile6 *tile = (const HorizonFlatTile6 *)RelFileOffsetPtr6 + range->first;
        u32 n;
        for (n = range->count; n != 0; n--, tile++) HorizonFlatTile(tile, NULL);
    }
}

/* func_80065ED8: section 7 (ranges10[7], RelFileOffsetPtr7) flat tiles
 * inside their texture window. */
static void HorizonPass7(const VisibleCourseViews *views)
{
    u32 view;
    HorizonRestoreFarColor();
    for (view = 1; HorizonView(view, views); view++) {
        const CoursePrimitiveRange *range = &views->stream[view].record->ranges10[7];
        const HorizonFlatWindowTile7 *rec = (const HorizonFlatWindowTile7 *)RelFileOffsetPtr7 + range->first;
        u32 n;
        for (n = range->count; n != 0; n--, rec++) HorizonFlatTile(&rec->tile, &rec->window30);
    }
}

/* Section 8/15 record (48 bytes): a camera-facing textured quad placed at an
 * origin. Corner positions are relative to the origin. */
typedef struct {
    s32 xy0;
    s16 z0, z1;
    s32 xy1, xy2;
    s16 z2, z3;
    s32 xy3;
    u32 uv[3];
    s16 flags24;        /* bit 0: halve far texture coordinates; high byte: signed OT bias */
    s16 originZ26;
    s32 originXY28;
    u32 color2C;
} HorizonBillboard8;
_Static_assert(sizeof(HorizonBillboard8) == 0x30, "section 8 billboard");

enum {
    GTE_MVMVA_LLM_V0_BK = 0x04A2012, /* mvmva sf=1, light matrix, V0, + BK */
    GTE_MVMVA_RT_V0 = 0x0486012      /* mvmva sf=1, rotation, V0, no translation */
};

static s32 HorizonSavedTransform8[8], HorizonSavedLight8[8];

/* func_80066A34 body (and DrawWireQuadVerts1's): the light matrix/BK pair
 * holds the camera view transform, which places each origin; the rotation is
 * the camera's pitch/roll only, so the quads face the viewer. Any GTE error
 * flag skips the quad. color == NULL takes each record's colour. The
 * original prefetched the next record while processing one (reading one
 * record past each range); the staging is equivalent to loading each record
 * in turn. */
static void HorizonEmitBillboards(const VisibleCourseViews *views, int section, const u8 *table,
                                  const u32 *color)
{
    RroSceneScratchPrefix *scene = Horizon.scene;
    u32 view;
    for (view = 1; HorizonView(view, views); view++) {
        const CoursePrimitiveRange *range = &views->stream[view].record->ranges10[section];
        const HorizonBillboard8 *b = (const HorizonBillboard8 *)table + range->first;
        u32 n;
        for (n = range->count; n != 0; n--, b++) {
            s32 otz, depth, bias, halve;
            u32 uvs[4];
            gte_mtc2(GTE_VXY0, b->originXY28);
            gte_mtc2(GTE_VZ0, b->originZ26);
            gte_cmd(GTE_MVMVA_LLM_V0_BK);
            if ((u32)gte_cfc2(GTE_FLAG) >> 31) continue;
            gte_ctc2(GTE_TRX, gte_mfc2(GTE_MAC1) << 2);
            gte_ctc2(GTE_TRY, gte_mfc2(GTE_MAC2) << 2);
            gte_ctc2(GTE_TRZ, gte_mfc2(GTE_MAC3) << 2);
            gte_mtc2(GTE_VXY0, b->xy0);
            gte_mtc2(GTE_VZ0, b->z0);
            gte_mtc2(GTE_VXY1, b->xy1);
            gte_mtc2(GTE_VZ1, b->z1);
            gte_mtc2(GTE_VXY2, b->xy2);
            gte_mtc2(GTE_VZ2, b->z2);
            gte_cmd(GTE_RTPT);
            RR_GTE_STORE_SXY(GTE_SXY0, &Horizon.sxy[0]);
            if ((u32)gte_cfc2(GTE_FLAG) >> 31) continue;
            RR_GTE_STORE_SXY(GTE_SXY1, &Horizon.sxy[1]);
            RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[2]);
            gte_mtc2(GTE_VXY0, b->xy3);
            gte_mtc2(GTE_VZ0, b->z3);
            gte_cmd(GTE_RTPS);
            RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[3]);
            if ((u32)gte_cfc2(GTE_FLAG) >> 31) continue;
            gte_cmd(GTE_AVSZ4);
            /* QuadOffscreenTestVerts0/1 */
            if (ScreenQuadOutsideViewport(scene, Horizon.sxy)) continue;
            otz = gte_mfc2(GTE_OTZ);
            depth = otz >> (scene->depthShift60 & 31);
            if ((u32)(depth - 1) >= 0x1BF) continue;
            gte_mtc2(GTE_RGBC, (s32)(color != NULL ? *color : b->color2C));
            halve = !(otz < HorizonFarDepth());
            gte_cmd(GTE_DPCS);
            bias = b->flags24 >> 8;
            Horizon.ot = HorizonOrder(depth, bias);
            CourseOcclusionTile(Horizon.sxy, Horizon.ot, Horizon.cursor);
            halve &= b->flags24;
            uvs[0] = b->uv[0];
            uvs[1] = b->uv[1];
            uvs[2] = b->uv[2] & 0xFFFF;
            uvs[3] = b->uv[2] >> 16;
            if (halve) HorizonHalveUV6(uvs);
            HorizonEmitPlainFT4(uvs, (u32)gte_mfc2(GTE_RGB2));
        }
    }
}

static void HorizonSaveTransforms8(void)
{
    int i;
    for (i = 0; i < 8; i++) HorizonSavedTransform8[i] = gte_cfc2(i);
}

static void HorizonLoadViewLight8(void)
{
    int i;
    for (i = 0; i < 5; i++) gte_ctc2(i, CameraPitchRollMatrix.w[i]);
    for (i = 0; i < 8; i++) HorizonSavedLight8[i] = gte_cfc2(8 + i);
    for (i = 0; i < 8; i++) gte_ctc2(8 + i, CameraViewMatrix.w[i]);
}

static void HorizonRestoreTransforms8(void)
{
    int i;
    for (i = 0; i < 8; i++) gte_ctc2(i, HorizonSavedTransform8[i]);
    for (i = 0; i < 8; i++) gte_ctc2(8 + i, HorizonSavedLight8[i]);
}

/* func_80066A34: section 8 (ranges10[8], TrackRelSegment8Ptr) billboards. */
static void HorizonPass8(const VisibleCourseViews *views)
{
    HorizonSaveTransforms8();
    HorizonLoadViewLight8();
    HorizonEmitBillboards(views, 8, TrackRelSegment8Ptr, NULL);
    HorizonRestoreTransforms8();
}

/* DrawWireQuadVerts1 (8006D5A4): nothing references it (no pass program
 * lists it). Section 15 (ranges10[15], RelFileSection15) billboards in one
 * colour: the light colour of a vertical unit normal under the camera yaw
 * (NCS with RGBC 2C000000). */
static RR_UNUSED void HorizonPassSection15(const VisibleCourseViews *views)
{
    u32 lit;
    int i;
    HorizonRestoreFarColor();
    HorizonSaveTransforms8();
    for (i = 0; i < 5; i++) gte_ctc2(i, CameraYawRotMatrix.w[i]);
    gte_mtc2(GTE_VXY0, 0);
    gte_mtc2(GTE_VZ0, 0x1000);
    gte_cmd(GTE_MVMVA_RT_V0);
    gte_mtc2(GTE_VXY0, (s32)(((u32)gte_mfc2(GTE_IR1) & 0xFFFF) | ((u32)gte_mfc2(GTE_IR2) << 16)));
    gte_mtc2(GTE_VZ0, gte_mfc2(GTE_IR3));
    gte_mtc2(GTE_RGBC, 0x2C000000);
    gte_cmd(GTE_NCS);
    HorizonLoadViewLight8();
    lit = (u32)gte_mfc2(GTE_RGB2);
    Horizon.lineColor = lit;
    HorizonEmitBillboards(views, 15, RelFileSection15, &lit);
    HorizonRestoreTransforms8();
}

/* Triangle tiles of sections 9 and 10 (func_80066F18, func_80067A94). Near
 * tiles split recursively into four triangles at edge midpoints
 * (func_80067318/func_80067F6C), with outline polylines along the outer
 * edges at the last split level. */

/* Triangle vertex of these passes (scratch 0x130 onwards, 20 bytes):
 * position, texture u,v, command/colour word, projected SXY and SZ. */
typedef struct {
    s16 x, y, z;
    u16 uv;
    u32 color;
    s32 sxy;
    s32 sz;
} HorizonTriVertex;

/* Each split level takes three midpoints after the previous level's
 * (0x130 + 0x3C * level). From level 5 on, the PS1 arena runs into the
 * recursion frames at 0x25C, so valid data has at most four levels; the host
 * clamps deeper requests to four instead of corrupting memory. */
enum { HORIZON_TRI_MAX_LEVELS = 4 };
static HorizonTriVertex HorizonTriArena[HORIZON_TRI_MAX_LEVELS + 1][3];

/* Section 9 record (RelFileOffsetPtr9, 0x34 bytes); section 10 records
 * (TrackCollisionGridPtr, 0x38 bytes) append a texture-window command. */
typedef struct {
    s32 xy0;
    u32 z0z1;
    s32 xy1, xy2;
    u32 z2flags;        /* low: z2, high: bit 0 halve far uv, high byte OT bias */
    u32 uv0;            /* uv0|clut */
    u32 uv1;            /* uv1|tpage */
    s16 uv2;
    s16 levels1E;       /* bits 5..9: split levels */
    u32 farColor20;
    u32 lineColor24;
    u32 color[3];       /* 0x28..0x30 */
} HorizonTriTile;
_Static_assert(sizeof(HorizonTriTile) == 0x34, "horizon tri tile");
typedef struct {
    HorizonTriTile tri;
    u32 textureWindow34;
} HorizonTriTileWindowed;
_Static_assert(sizeof(HorizonTriTileWindowed) == 0x38, "horizon windowed tri tile");

/* Flat textured and lit textured triangle packets as payload words. */
typedef struct {
    GpuDmaTag tag;
    s32 color04, xy0;
    u32 uvClut;
    s32 xy1;
    u32 uvTpage;
    s32 xy2;
    u16 uv2, pad;
} HorizonFT3Words;
typedef struct {
    GpuDmaTag tag;
    s32 color0, xy0;
    u32 uvClut;
    s32 color1, xy1;
    u32 uvTpage;
    s32 color2, xy2;
    u16 uv2, pad;
} HorizonGT3Words;

/* TriOffscreenTest / func_80068660: all three points beyond one edge. */
static int HorizonTriOutside(const HorizonTriVertex *a, const HorizonTriVertex *b,
                             const HorizonTriVertex *c)
{
    return QuadOutsideViewport(Horizon.scene, ScreenX(a->sxy), ScreenX(b->sxy), ScreenX(c->sxy), ScreenX(c->sxy),
                               ScreenY(a->sxy), ScreenY(b->sxy), ScreenY(c->sxy), ScreenY(c->sxy));
}

/* Facing (by the mirror flag) and viewport test of a split triangle. */
static int HorizonTriVisible(const HorizonTriVertex *a, const HorizonTriVertex *b,
                             const HorizonTriVertex *c)
{
    s32 area;
    RR_GTE_LOAD_SXY(GTE_SXY0, &a->sxy);
    RR_GTE_LOAD_SXY(GTE_SXY1, &b->sxy);
    RR_GTE_LOAD_SXY(GTE_SXY2, &c->sxy);
    gte_cmd(GTE_NCLIP);
    if (HorizonTriOutside(a, b, c)) return 0;
    area = gte_mfc2(GTE_MAC0);
    return (0 < area) ^ (0 < Horizon.scene->renderFlags64);
}

/* Leaf of func_80067318: one lit textured triangle. */
static void HorizonTriEmitGT3(const HorizonTriVertex *a, const HorizonTriVertex *b,
                              const HorizonTriVertex *c)
{
    HorizonGT3Words *p = (HorizonGT3Words *)Horizon.cursor;
    p->uvClut = ((u32)Horizon.clut << 16) | a->uv;
    p->uvTpage = ((u32)Horizon.tpage << 16) | b->uv;
    p->uv2 = c->uv;
    RR_SXY_COPY(p->xy0, a->sxy);
    RR_SXY_COPY(p->xy1, b->sxy);
    RR_SXY_COPY(p->xy2, c->sxy);
    p->color0 = (s32)a->color;
    p->color1 = (s32)b->color;
    p->color2 = (s32)c->color;
    RR_GPU_ADD_PRIM(Horizon.ot, p, 9);
    Horizon.cursor = (u8 *)(p + 1);
}

/* func_80067318 / func_80067F6C: split (a, b, c) at the edge midpoints
 * m0 = a-b, m1 = c-b, m2 = c-a (INTPL/DPCS at one half), project them and
 * recurse into the visible triangles (a, m0, m2), (m2, m1, c), (m0, b, m1)
 * and (m2, m0, m1). edges marks the outer edges (bit 0: a-b, 1: b-c,
 * 2: c-a); at the last split level each child's outer half-edges are drawn
 * as three-point polylines whose first two points coincide. */
static void HorizonTriSplit(const HorizonTriVertex *a, const HorizonTriVertex *b,
                            const HorizonTriVertex *c, int levels, int depth, u32 edges, u32 path)
{
    HorizonTriVertex *m;
    int lines;
    if (levels == 0) {
        HorizonTriEmitGT3(a, b, c);
        return;
    }
    lines = levels < 2 && !(Horizon.lineFlags & 2);
    m = HorizonTriArena[depth + 1];
    gte_mtc2(GTE_IR0, 0x800);
    CourseIntpl(b->x, b->y, b->z, a->x, a->y, a->z);
    m[0].x = (s16)gte_mfc2(GTE_IR1);
    m[0].y = (s16)gte_mfc2(GTE_IR2);
    m[0].z = (s16)gte_mfc2(GTE_IR3);
    CourseIntpl(b->x, b->y, b->z, c->x, c->y, c->z);
    m[1].x = (s16)gte_mfc2(GTE_IR1);
    m[1].y = (s16)gte_mfc2(GTE_IR2);
    m[1].z = (s16)gte_mfc2(GTE_IR3);
    CourseIntpl(a->x, a->y, a->z, c->x, c->y, c->z);
    m[2].x = (s16)gte_mfc2(GTE_IR1);
    m[2].y = (s16)gte_mfc2(GTE_IR2);
    m[2].z = (s16)gte_mfc2(GTE_IR3);
    /* FC blue keeps a's z for the texture coordinates (unused result). */
    m[0].uv = (u16)HorizonDpcs((s16)a->uv, (b->uv & 0xFF) << 4, (b->uv >> 8) << 4, a->z);
    m[1].uv = (u16)HorizonDpcs((s16)c->uv, (b->uv & 0xFF) << 4, (b->uv >> 8) << 4, a->z);
    m[2].uv = (u16)HorizonDpcs((s16)c->uv, (a->uv & 0xFF) << 4, (a->uv >> 8) << 4, a->z);
    m[0].color = (u32)HorizonDpcs((s32)a->color, (b->color & 0xFF) << 4,
                                  ((b->color >> 8) & 0xFF) << 4, ((b->color >> 16) & 0xFF) << 4);
    m[1].color = (u32)HorizonDpcs((s32)c->color, (b->color & 0xFF) << 4,
                                  ((b->color >> 8) & 0xFF) << 4, ((b->color >> 16) & 0xFF) << 4);
    m[2].color = (u32)HorizonDpcs((s32)c->color, (a->color & 0xFF) << 4,
                                  ((a->color >> 8) & 0xFF) << 4, ((a->color >> 16) & 0xFF) << 4);
    gte_mtc2(GTE_VXY0, (s32)(((u32)(u16)m[0].x) | ((u32)(u16)m[0].y << 16)));
    gte_mtc2(GTE_VZ0, m[0].z);
    gte_mtc2(GTE_VXY1, (s32)(((u32)(u16)m[1].x) | ((u32)(u16)m[1].y << 16)));
    gte_mtc2(GTE_VZ1, m[1].z);
    gte_mtc2(GTE_VXY2, (s32)(((u32)(u16)m[2].x) | ((u32)(u16)m[2].y << 16)));
    gte_mtc2(GTE_VZ2, m[2].z);
    gte_cmd(GTE_RTPT);
    RR_GTE_STORE_SXY(GTE_SXY0, &m[0].sxy);
    RR_GTE_STORE_SXY(GTE_SXY1, &m[1].sxy);
    RR_GTE_STORE_SXY(GTE_SXY2, &m[2].sxy);
    m[0].sz = gte_mfc2(GTE_SZ1);
    m[1].sz = gte_mfc2(GTE_SZ2);
    m[2].sz = gte_mfc2(GTE_SZ3);

    if (HorizonTriVisible(a, &m[0], &m[2])) {
        HorizonTriSplit(a, &m[0], &m[2], levels - 1, depth + 1, edges & 5, path * 4 + 0);
        if (lines) {
            if (edges & 1) HorizonLineF3(a->sxy, a->sxy, m[0].sxy);
            if (edges & 4) HorizonLineF3(a->sxy, m[2].sxy, m[2].sxy);
        }
    }
    if (HorizonTriVisible(&m[2], &m[1], c)) {
        HorizonTriSplit(&m[2], &m[1], c, levels - 1, depth + 1, edges & 6, path * 4 + 1);
        if (lines) {
            if (edges & 2) HorizonLineF3(c->sxy, m[1].sxy, m[1].sxy);
            if (edges & 4) HorizonLineF3(c->sxy, c->sxy, m[2].sxy);
        }
    }
    if (HorizonTriVisible(&m[0], b, &m[1])) {
        HorizonTriSplit(&m[0], b, &m[1], levels - 1, depth + 1, edges & 3, path * 4 + 2);
        if (lines) {
            if (edges & 1) HorizonLineF3(b->sxy, m[0].sxy, m[0].sxy);
            if (edges & 2) HorizonLineF3(b->sxy, b->sxy, m[1].sxy);
        }
    }
    if (HorizonTriVisible(&m[2], &m[0], &m[1]))
        HorizonTriSplit(&m[2], &m[0], &m[1], levels - 1, depth + 1, 0, path * 4 + 3);
}

/* func_8006794C / func_800685A0: split the tile triangle (HorizonTriArena[0])
 * and draw its outline: the closed triangle when more than one level is
 * split, else (unless the GTE flagged an error) the three closed
 * corner/midpoint/corner polylines. The far colour is restored afterwards. */
static void HorizonTriSplitTile(u32 uv0, u32 uv1, u32 uv2, int levels)
{
    HorizonTriVertex *v = HorizonTriArena[0];
    const HorizonTriVertex *m = HorizonTriArena[1];
    if (levels > HORIZON_TRI_MAX_LEVELS) levels = HORIZON_TRI_MAX_LEVELS;
    v[0].uv = (u16)uv0;
    v[1].uv = (u16)uv1;
    v[2].uv = (u16)uv2;
    HorizonTriSplit(&v[0], &v[1], &v[2], levels, 0, levels == 1 ? 0 : 7, (u32)levels + 1);
    if (!(Horizon.lineFlags & 2)) {
        if (levels != 1) {
            HorizonLineF4(v[0].sxy, v[1].sxy, v[2].sxy, v[0].sxy);
        } else if (!(Horizon.lineFlags & 1)) {
            HorizonLineF4(v[0].sxy, m[0].sxy, v[1].sxy, v[0].sxy);
            HorizonLineF4(v[1].sxy, m[1].sxy, v[2].sxy, v[1].sxy);
            HorizonLineF4(v[2].sxy, m[2].sxy, v[0].sxy, v[2].sxy);
        }
    }
    gte_ctc2(GTE_FC_R, Horizon.savedFarColor[0]);
    gte_ctc2(GTE_FC_G, Horizon.savedFarColor[1]);
    gte_ctc2(GTE_FC_B, Horizon.savedFarColor[2]);
}

/* State packet pair (E2000000, command) that sets a texture window. */
static void HorizonTriWindowOpen(GpuDrawModePacket *p, u32 command)
{
    p->command[0] = 0xE2000000;
    p->command[1] = command;
    RR_GPU_ADD_PRIM(Horizon.ot, p, 2);
}

/* Shared body of func_80066F18 (windowed = 0) and func_80067A94 (windowed:
 * every piece is bracketed by texture-window state packets: the window is
 * reset after the piece and set before it, halved with the coordinates for
 * halved far tiles). Far tiles are one depth-cued FT3 in the far colour,
 * near tiles a depth-cued GT3 or a split triangle. */
static void HorizonPassTris(const VisibleCourseViews *views, int section,
                            const u8 *records, u32 stride, int windowed)
{
    RroSceneScratchPrefix *scene = Horizon.scene;
    u32 view;
    gte_ctc2(GTE_FC_R, Horizon.savedFarColor[0]);
    gte_ctc2(GTE_FC_G, Horizon.savedFarColor[1]);
    gte_ctc2(GTE_FC_B, Horizon.savedFarColor[2]);
    for (view = 1; HorizonView(view, views); view++) {
        const CoursePrimitiveRange *range = &views->stream[view].record->ranges10[section];
        const u8 *record = records + (size_t)range->first * stride;
        u32 n;
        for (n = range->count; n != 0; n--, record += stride) {
            const HorizonTriTile *t = (const HorizonTriTile *)record;
            u32 window = windowed ? ((const HorizonTriTileWindowed *)record)->textureWindow34 : 0;
            HorizonTriVertex *v = HorizonTriArena[0];
            u32 flags16 = t->z2flags >> 16, flagsGte, uv0, uv1, uv2;
            s32 area, mirror, otz, far, depth, lod, levels, halve, offscreen;
            s32 farRgb = 0, rgb[3] = {0, 0, 0};
            int i;
            gte_mtc2(GTE_VXY0, t->xy0);
            gte_mtc2(GTE_VZ0, (s32)(t->z0z1 & 0xFFFF));
            gte_mtc2(GTE_VXY1, t->xy1);
            gte_mtc2(GTE_VZ1, (s32)(t->z0z1 >> 16));
            gte_mtc2(GTE_VXY2, t->xy2);
            gte_mtc2(GTE_VZ2, (s32)(t->z2flags & 0xFFFF));
            gte_cmd(GTE_RTPT);
            v[0].x = (s16)t->xy0;
            v[0].y = (s16)(t->xy0 >> 16);
            v[0].z = (s16)t->z0z1;
            v[1].x = (s16)t->xy1;
            v[1].y = (s16)(t->xy1 >> 16);
            v[1].z = (s16)(t->z0z1 >> 16);
            v[2].x = (s16)t->xy2;
            v[2].y = (s16)(t->xy2 >> 16);
            v[2].z = (s16)t->z2flags;
            for (i = 0; i < 3; i++) {
                v[i].uv = 0;
                v[i].color = t->color[i];
            }
            RR_GTE_STORE_SXY(GTE_SXY0, &v[0].sxy);
            RR_GTE_STORE_SXY(GTE_SXY1, &v[1].sxy);
            RR_GTE_STORE_SXY(GTE_SXY2, &v[2].sxy);
            flagsGte = (u32)gte_cfc2(GTE_FLAG);
            gte_cmd(GTE_NCLIP);
            area = gte_mfc2(GTE_MAC0);
            mirror = 0 < scene->renderFlags64;
            gte_cmd(GTE_AVSZ3);
            if (!HorizonFacing((0 < area) ^ mirror)) continue;
            Horizon.otBias = (s32)(flags16 >> 8);
            otz = gte_mfc2(GTE_OTZ);
            far = HorizonFarDepth() < otz;
            Horizon.lineColor = t->lineColor24;
            if (far) {
                gte_mtc2(GTE_RGBC, (s32)t->farColor20);
                gte_cmd(GTE_DPCS);
                farRgb = gte_mfc2(GTE_RGB2);
            } else {
                gte_mtc2(GTE_RGBC, (s32)v[0].color);
                gte_mtc2(GTE_RGB0, (s32)v[0].color);
                gte_mtc2(GTE_RGB1, (s32)v[1].color);
                gte_mtc2(GTE_RGB2, (s32)v[2].color);
                gte_cmd(GTE_DPCT);
                rgb[0] = gte_mfc2(GTE_RGB0);
                rgb[1] = gte_mfc2(GTE_RGB1);
                rgb[2] = gte_mfc2(GTE_RGB2);
            }
            Horizon.lineFlags = (flagsGte >> 31) | ((t->lineColor24 >> 31) << 1);
            halve = (s32)(flags16 & 1) & far;
            levels = (t->levels1E & 0x3FF) >> 5;
            offscreen = HorizonTriOutside(&v[0], &v[1], &v[2]);
            depth = otz >> (scene->depthShift60 & 31);
            if ((u32)depth >= 0x1C0) continue;
            if (depth == 0 && !(0x122 < gte_mfc2(GTE_SZ1) || 0x122 < gte_mfc2(GTE_SZ2) ||
                                0x122 < gte_mfc2(GTE_SZ3)))
                continue;
            lod = HorizonLod(otz);
            if (far) {
                if (offscreen) continue;
                v[0].color = t->farColor20;
            } else {
                levels -= lod;
                if (levels < 0) levels = 0;
                if (offscreen) continue;
            }
            Horizon.ot = HorizonOrder(depth, Horizon.otBias);
            CourseOcclusionTile(Horizon.sxy, Horizon.ot, Horizon.cursor);
            uv0 = t->uv0;
            uv1 = t->uv1;
            uv2 = (u32)(s32)t->uv2;
            if (far) {
                GpuDrawModePacket *reset = (GpuDrawModePacket *)Horizon.cursor;
                HorizonFT3Words *p = (HorizonFT3Words *)(windowed ? (u8 *)(reset + 1) : Horizon.cursor);
                if (halve) {
                    window = ((window >> 1) & 0x7BDEF) | 0xE2000000;
                    uv0 = (uv0 & 0xFFFF0000) | ((uv0 >> 1) & 0x7F7F);
                    uv1 = (uv1 & 0xFFFF0000) | ((uv1 >> 1) & 0x7F7F);
                    uv2 = (uv2 >> 1) & 0x7F7F;
                }
                p->color04 = farRgb;
                p->uvClut = uv0;
                p->uvTpage = uv1;
                p->uv2 = (u16)uv2;
                RR_SXY_COPY(p->xy0, v[0].sxy);
                RR_SXY_COPY(p->xy1, v[1].sxy);
                RR_SXY_COPY(p->xy2, v[2].sxy);
                if (windowed) {
                    GpuDrawModePacket *set = (GpuDrawModePacket *)(p + 1);
                    (void)CourseEmitTextureWindow((u8 *)reset, Horizon.ot, 0xE2000000);
                    RR_GPU_ADD_PRIM(Horizon.ot, p, 7);
                    HorizonTriWindowOpen(set, window);
                    Horizon.cursor = (u8 *)(set + 1);
                } else {
                    RR_GPU_ADD_PRIM(Horizon.ot, p, 7);
                    Horizon.cursor = (u8 *)(p + 1);
                }
            } else if (levels != 0) {
                Horizon.clut = (u16)(uv0 >> 16);
                Horizon.tpage = (u16)(uv1 >> 16);
                if (windowed) Horizon.cursor = CourseEmitTextureWindow(Horizon.cursor, Horizon.ot, 0xE2000000);
                v[0].color = (u32)rgb[0];
                v[1].color = (u32)rgb[1];
                v[2].color = (u32)rgb[2];
                HorizonTriSplitTile(uv0, uv1, uv2, levels);
                /* The closing packet carries the window command first. */
                if (windowed) Horizon.cursor = CourseEmitTextureWindow(Horizon.cursor, Horizon.ot, window);
            } else {
                GpuDrawModePacket *reset = (GpuDrawModePacket *)Horizon.cursor;
                HorizonGT3Words *p = (HorizonGT3Words *)(windowed ? (u8 *)(reset + 1) : Horizon.cursor);
                p->uvClut = uv0;
                p->uvTpage = uv1;
                p->uv2 = (u16)uv2;
                RR_SXY_COPY(p->xy0, v[0].sxy);
                RR_SXY_COPY(p->xy1, v[1].sxy);
                RR_SXY_COPY(p->xy2, v[2].sxy);
                p->color0 = rgb[0];
                p->color1 = rgb[1];
                p->color2 = rgb[2];
                if (windowed) {
                    GpuDrawModePacket *set = (GpuDrawModePacket *)(p + 1);
                    (void)CourseEmitTextureWindow((u8 *)reset, Horizon.ot, 0xE2000000);
                    RR_GPU_ADD_PRIM(Horizon.ot, p, 9);
                    HorizonTriWindowOpen(set, window);
                    Horizon.cursor = (u8 *)(set + 1);
                } else {
                    RR_GPU_ADD_PRIM(Horizon.ot, p, 9);
                    Horizon.cursor = (u8 *)(p + 1);
                }
            }
        }
    }
}

/* func_80066F18: section 9 (ranges10[9], RelFileOffsetPtr9). */
static void HorizonPass9(const VisibleCourseViews *views)
{
    HorizonPassTris(views, 9, RelFileOffsetPtr9, sizeof(HorizonTriTile), 0);
}

/* func_80067A94: section 10 (ranges10[10], TrackCollisionGridPtr). */
static void HorizonPass10(const VisibleCourseViews *views)
{
    HorizonPassTris(views, 10, TrackCollisionGridPtr, sizeof(HorizonTriTileWindowed), 1);
}

/* --- Course 8 passes (func_800686E8, func_80069630) ---------------------- */

/* Scratch words of the course 8 passes at their PS1 offsets: a second view
 * matrix (rotation and translation, 0x108..0x124) used to light tiles from a
 * second projection, and a per-view enable mask (0x128, bit = view index). */
static u32 HorizonC8ScratchWord(unsigned offset) { return *(const u32 *)(RrScratchpad + offset); }

static void HorizonC8LoadMatrix(const u32 *words)
{
    unsigned i;
    for (i = 0; i < 8; i++) gte_ctc2(i, (s32)words[i]);
}

static void HorizonC8LoadAltMatrix(void)
{
    u32 words[8];
    unsigned i;
    for (i = 0; i < 8; i++) words[i] = HorizonC8ScratchWord(0x108 + i * 4);
    HorizonC8LoadMatrix(words);
}

/* Camera matrix of the scene (scratch 0x28..0x44). */
static void HorizonC8LoadCameraMatrix(void)
{
    HorizonC8LoadMatrix((const u32 *)((const ViewMatrixScratch48 *)Horizon.scene)->matrix28.w);
}

/* func_800694A4 / func_8006A478: glow colour of a vertex from its screen
 * distance to (160, 240) and its depth; the sector boundaries are found with
 * NCLIP against the lines to (120, 200) and (40, 240). */
static u32 HorizonC8Glow(s32 sxy, s32 sz)
{
    s32 x = (s16)sxy, y = sxy >> 16;
    s32 dy = y < 0xF1 ? 0xF0 - y : y - 0xF0;
    s32 dx = x < 0xA1 ? 0xA0 - x : x - 0xA0;
    s32 glow = 0, t, twice, m;
    if (!(dy < 0x78)) {
        gte_mtc2(GTE_SXY0, 0);
        gte_mtc2(GTE_SXY1, (s32)(((u32)dy << 16) | ((u32)dx & 0xFFFF)));
        gte_mtc2(GTE_SXY2, 0x00C80078);
        gte_cmd(GTE_NCLIP);
        t = (dx * 2 + dy) << 8;
        glow = (t >> 9) + (t >> 12) + (t >> 14) + (t >> 16);
        if (gte_mfc2(GTE_MAC0) < 0) {
            gte_mtc2(GTE_SXY2, 0x00F00028);
            gte_cmd(GTE_NCLIP);
            t = (dy * 2 + dx) << 8;
            glow = (t >> 9) - (t >> 15);
            if (gte_mfc2(GTE_MAC0) < 0) {
                t = dy << 8;
                glow = (t >> 8) + (t >> 12) + (t >> 16);
            }
        }
    }
    dx -= 0x14;
    sz >>= 3;
    if (dx < 0) dx = 0;
    twice = dx * 2;
    sz = (sz + dx) >> 1;
    m = twice < glow ? (sz < glow ? glow : sz) : (sz < twice ? twice : sz);
    if (m >= 0x100) m = 0xFF;
    m = (0xFF - m) * 2;
    if (m >= 0x100) m = 0xFF;
    return (u32)m | ((u32)m << 8) | ((u32)(m - (m >> 2)) << 16) | 0x3C000000;
}

/* func_80068E8C / func_80069E60: fill row[1..last-1] (position by INTPL,
 * texture coordinates by DPCS), give every entry a glow colour from the second
 * projection when lineFlags bit 3 is set, then project the row. */
static void HorizonC8InterpRow(HorizonVertex *row, int last, s32 step)
{
    const HorizonVertex *end = &row[last];
    const HorizonVertex *start = &row[0];
    s32 t = 0x1000 - step;
    int i = 1;
    for (; t != 0; t -= step, i++) {
        HorizonVertex *e = &row[i];
        gte_mtc2(GTE_IR0, t);
        CourseIntpl(start->f.x, start->f.y, start->f.z, end->f.x, end->f.y, end->f.z);
        e->f.x = (s16)gte_mfc2(GTE_IR1);
        e->f.y = (s16)gte_mfc2(GTE_IR2);
        e->f.z = (s16)gte_mfc2(GTE_IR3);
        e->w.uv = (u16)HorizonDpcs((s16)end->w.uv, start->f.u << 4, start->f.v << 4, start->f.z);
    }
    if (Horizon.lineFlags & 8) {
        HorizonC8LoadAltMatrix();
        i = 0;
        do {
            s32 sxy0, sxy1, sxy2, sz1, sz2, sz3;
            gte_mtc2(GTE_VXY0, row[i].w.xy);
            gte_mtc2(GTE_VZ0, row[i].f.z);
            gte_mtc2(GTE_VXY1, row[i + 1].w.xy);
            gte_mtc2(GTE_VZ1, row[i + 1].f.z);
            gte_mtc2(GTE_VXY2, row[i + 2].w.xy);
            gte_mtc2(GTE_VZ2, row[i + 2].f.z);
            gte_cmd(GTE_RTPT);
            RR_GTE_STORE_SXY(GTE_SXY0, &sxy0);
            RR_GTE_STORE_SXY(GTE_SXY1, &sxy1);
            RR_GTE_STORE_SXY(GTE_SXY2, &sxy2);
            sz1 = gte_mfc2(GTE_SZ1);
            sz2 = gte_mfc2(GTE_SZ2);
            sz3 = gte_mfc2(GTE_SZ3);
            row[i].w.color = HorizonC8Glow(sxy0, sz1);
            row[i + 1].w.color = HorizonC8Glow(sxy1, sz2);
            row[i + 2].w.color = HorizonC8Glow(sxy2, sz3);
            i += 3;
        } while (i < last);
        HorizonC8LoadCameraMatrix();
    }
    HorizonProjectRow(row, last);
}

/* Corner c position and texture halfword into a grid entry (its colour word
 * is left as it is). */
static void HorizonC8CopyCorner(HorizonVertex *dst, const HorizonVertex *src)
{
    dst->w.xy = src->w.xy;
    dst->w.z = src->w.z;
    dst->w.uv = src->w.uv;
}

/* func_80069134 / func_8006A108: a (0x1000 >> levelU) x (1 << levelV) grid
 * between the corners without back-face culling: glow-lit GT4 pieces when
 * lineFlags bit 3 is set, otherwise black flat F4 pieces. */
static void HorizonC8GridLevels(s32 levelU, s32 levelV, const u32 uv[4])
{
    HorizonVertex *cur = Horizon.rows[0], *prev = Horizon.rows[1];
    HorizonVertex *c = Horizon.corner;
    RroSceneScratchPrefix *scene = Horizon.scene;
    s32 stepRows = (s32)(0x1000u >> (levelU & 31));
    s32 stepCols;
    int last;
    s32 t;
    /* Host bound as in HorizonEmitLitGrid (rows of 16 segments). */
    if ((levelV & 31) > 4) levelV = 4;
    stepCols = (s32)(0x1000u >> levelV);
    last = 1 << levelV;

    c[0].w.uv = (u16)uv[0];
    c[1].w.uv = (u16)uv[1];
    c[2].w.uv = (u16)uv[2];
    c[3].w.uv = (u16)uv[3];
    HorizonC8CopyCorner(&cur[0], &c[0]);
    HorizonC8CopyCorner(&cur[last], &c[2]);
    HorizonC8InterpRow(cur, last, stepCols);
    t = 0x1000;
    do {
        HorizonVertex *swap;
        int j;
        t -= stepRows;
        gte_mtc2(GTE_IR0, t);
        CourseIntpl(c[0].f.x, c[0].f.y, c[0].f.z, c[1].f.x, c[1].f.y, c[1].f.z);
        swap = cur;
        cur = prev;
        prev = swap;
        cur[0].f.x = (s16)gte_mfc2(GTE_IR1);
        cur[0].f.y = (s16)gte_mfc2(GTE_IR2);
        cur[0].f.z = (s16)gte_mfc2(GTE_IR3);
        cur[0].w.uv = (u16)HorizonDpcs((s16)c[1].w.uv, c[0].f.u << 4, c[0].f.v << 4, c[0].f.z);
        CourseIntpl(c[2].f.x, c[2].f.y, c[2].f.z, c[3].f.x, c[3].f.y, c[3].f.z);
        cur[last].f.x = (s16)gte_mfc2(GTE_IR1);
        cur[last].f.y = (s16)gte_mfc2(GTE_IR2);
        cur[last].f.z = (s16)gte_mfc2(GTE_IR3);
        cur[last].w.uv = (u16)HorizonDpcs((s16)c[3].w.uv, c[2].f.u << 4, c[2].f.v << 4, c[2].f.z);
        HorizonC8InterpRow(cur, last, stepCols);
        for (j = 0; j < last; j++) {
            const HorizonVertex *p0 = &prev[j], *p1 = &cur[j], *p2 = &prev[j + 1], *p3 = &cur[j + 1];
            if (QuadOutsideViewport(scene, ScreenX(p0->w.xy), ScreenX(p1->w.xy), ScreenX(p2->w.xy), ScreenX(p3->w.xy),
                                    ScreenY(p0->w.xy), ScreenY(p1->w.xy), ScreenY(p2->w.xy), ScreenY(p3->w.xy)))
                continue;
            if (Horizon.lineFlags & 8) {
                PolyGT4WordPacket *p = (PolyGT4WordPacket *)Horizon.cursor;
                RR_SXY_COPY(p->xy0, p0->w.xy);
                RR_SXY_COPY(p->xy1, p1->w.xy);
                RR_SXY_COPY(p->xy2, p2->w.xy);
                RR_SXY_COPY(p->xy3, p3->w.xy);
                p->uvClut0C = (s32)(((u32)Horizon.clut << 16) | p0->w.uv);
                p->uvTpage18 = (s32)(((u32)Horizon.tpage << 16) | p1->w.uv);
                p->uv2 = p2->w.uv;
                p->uv3 = p3->w.uv;
                p->color04 = (s32)p0->w.color;
                p->color10 = (s32)p1->w.color;
                p->color1C = (s32)p2->w.color;
                p->color28 = (s32)p3->w.color;
                RR_GPU_ADD_PRIM(Horizon.ot, p, 12);
                Horizon.cursor = (u8 *)(p + 1);
            } else {
                PolyF4Packet *p = (PolyF4Packet *)Horizon.cursor;
                RR_SXY_COPY(p->xy0, p0->w.xy);
                RR_SXY_COPY(p->xy1, p1->w.xy);
                RR_SXY_COPY(p->xy2, p2->w.xy);
                RR_SXY_COPY(p->xy3, p3->w.xy);
                p->colorCode04 = 0x28000000;
                RR_GPU_ADD_PRIM(Horizon.ot, p, 5);
                Horizon.cursor = (u8 *)(p + 1);
            }
        }
    } while (t != 0);
}

/* Course 8 always uses the near distance table. */
static s32 HorizonC8Lod(s32 otz)
{
    static const s16 limits[4] = {0x3F0, 0x330, 0x260, 0x180};
    int i;
    for (i = 0; i < 4; i++)
        if (!(otz < limits[i])) return 4 - i;
    return 0;
}

/* State packet of two words (E2 texture-window commands). */
static void HorizonC8State(u32 first, u32 second)
{
    GpuDrawModePacket *p = (GpuDrawModePacket *)Horizon.cursor;
    p->command[0] = first;
    p->command[1] = second;
    RR_GPU_ADD_PRIM(Horizon.ot, p, 2);
    Horizon.cursor = (u8 *)(p + 1);
}

/* func_800686E8 (section 2, 48-byte records at TrackRelSegment2Ptr) and
 * func_80069630 (section 3, 52-byte records at RelFileOffsetPtr3, word 0x30 a
 * texture-window command). Tiles are black silhouettes; in views enabled by
 * the 0x128 mask, tiles seen by the second projection are drawn glow-lit
 * (and subdivided more finely). Record: positions as in section 0, uv words
 * 0x18..0x20, flags 0x24 (high byte OT bias), levels 0x26, line colour 0x28,
 * corner-0 colour 0x2C. */
static void HorizonC8Pass(const VisibleCourseViews *views, int section, u32 stride,
                          const u8 *records, int windowed)
{
    RroSceneScratchPrefix *scene = Horizon.scene;
    u32 viewMask = HorizonC8ScratchWord(0x128);
    u32 view;
    for (view = 1; HorizonView(view, views); view++) {
        const CoursePrimitiveRange *range = &views->stream[view].record->ranges10[section];
        const u8 *rec = records + (u32)range->first * stride;
        u32 viewFlag = (viewMask >> (view & 31)) & 1;
        u32 n;
        for (n = range->count; n != 0; n--, rec += stride) {
            const u32 *w = (const u32 *)rec;
            u32 flags24 = ((const u16 *)rec)[0x24 / 2];
            s16 levels26 = ((const s16 *)rec)[0x26 / 2];
            s32 area0, area1, otz, depth, lod, mirror, offscreen, far;
            s32 levelU, levelV, subdivide;
            u32 flagsGte, uvs[4];
            gte_mtc2(GTE_VXY0, (s32)w[0]);
            gte_mtc2(GTE_VZ0, (s32)(w[1] & 0xFFFF));
            gte_mtc2(GTE_VXY1, (s32)w[2]);
            gte_mtc2(GTE_VZ1, (s32)(w[1] >> 16));
            gte_mtc2(GTE_VXY2, (s32)w[3]);
            gte_mtc2(GTE_VZ2, (s32)(w[4] & 0xFFFF));
            gte_cmd(GTE_RTPT);
            /* Corner colours 1..3 keep their previous values. */
            Horizon.corner[0].w.xy = (s32)w[0];
            Horizon.corner[0].w.z = (s16)w[1];
            Horizon.corner[0].w.uv = 0;
            Horizon.corner[1].w.xy = (s32)w[2];
            Horizon.corner[1].w.z = (s16)(w[1] >> 16);
            Horizon.corner[1].w.uv = 0;
            Horizon.corner[2].w.xy = (s32)w[3];
            Horizon.corner[2].w.z = (s16)w[4];
            Horizon.corner[2].w.uv = 0;
            Horizon.corner[3].w.xy = (s32)w[5];
            Horizon.corner[3].w.z = (s16)(w[4] >> 16);
            Horizon.corner[3].w.uv = 0;
            Horizon.corner[0].w.color = w[11];
            Horizon.lineColor = w[10];
            Horizon.otBias = (s32)(flags24 >> 8);
            RR_GTE_STORE_SXY(GTE_SXY0, &Horizon.sxy[0]);
            RR_GTE_STORE_SXY(GTE_SXY1, &Horizon.sxy[1]);
            RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[2]);
            flagsGte = (u32)gte_cfc2(GTE_FLAG);
            gte_cmd(GTE_NCLIP);
            area0 = gte_mfc2(GTE_MAC0);
            gte_mtc2(GTE_VXY0, (s32)w[5]);
            gte_mtc2(GTE_VZ0, (s32)(w[4] >> 16));
            gte_cmd(GTE_RTPS);
            uvs[0] = w[6];
            uvs[1] = w[7];
            uvs[2] = w[8] & 0xFFFF;
            uvs[3] = w[8] >> 16;
            RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[3]);
            flagsGte |= (u32)gte_cfc2(GTE_FLAG);
            gte_cmd(GTE_NCLIP);
            Horizon.lineFlags = (flagsGte >> 31) | ((w[10] >> 31) << 1);
            mirror = 0 < scene->renderFlags64;
            area1 = gte_mfc2(GTE_MAC0);
            gte_cmd(GTE_AVSZ4);
            if (!HorizonFacing(((0 < area0) ^ mirror) | ((area1 < 0) ^ mirror))) continue;
            offscreen = QuadOutsideViewport(scene,
                ScreenX(Horizon.sxy[0]), ScreenX(Horizon.sxy[1]), ScreenX(Horizon.sxy[2]), ScreenX(Horizon.sxy[3]),
                ScreenY(Horizon.sxy[0]), ScreenY(Horizon.sxy[1]), ScreenY(Horizon.sxy[2]), ScreenY(Horizon.sxy[3]));
            otz = gte_mfc2(GTE_OTZ);
            far = HorizonFarDepth() < otz;
            depth = otz >> (scene->depthShift60 & 31);
            if ((u32)depth >= 0x1C0) continue;
            if (depth == 0 && !(0x122 < gte_mfc2(GTE_SZ0) || 0x122 < gte_mfc2(GTE_SZ1) ||
                                0x122 < gte_mfc2(GTE_SZ2) || 0x122 < gte_mfc2(GTE_SZ3)))
                continue;
            lod = HorizonC8Lod(otz);
            if (viewFlag) {
                s32 alt[4], altArea0, altArea1, probe;
                /* Second projection of the corners. In the far case the level
                 * registers still hold the raw flags halfword and OT bias. */
                HorizonC8LoadAltMatrix();
                gte_mtc2(GTE_VXY0, Horizon.corner[0].w.xy);
                gte_mtc2(GTE_VZ0, Horizon.corner[0].w.z);
                gte_mtc2(GTE_VXY1, Horizon.corner[1].w.xy);
                gte_mtc2(GTE_VZ1, Horizon.corner[1].w.z);
                gte_mtc2(GTE_VXY2, Horizon.corner[2].w.xy);
                gte_mtc2(GTE_VZ2, Horizon.corner[2].w.z);
                gte_cmd(GTE_RTPT);
                levelU = (s32)flags24;
                levelV = (s32)(flags24 >> 8);
                if (!far) {
                    s32 levels = levels26 & 0x3FF;
                    levelV = (levels >> 5) - lod;
                    levelU = (levels & 0x1F) - lod;
                    if (levelU < 0) levelU = 0;
                    if (levelV < 0) levelV = 0;
                } else if (offscreen) {
                    HorizonC8LoadCameraMatrix();
                    continue;
                }
                RR_GTE_STORE_SXY(GTE_SXY0, &alt[0]);
                RR_GTE_STORE_SXY(GTE_SXY1, &alt[1]);
                RR_GTE_STORE_SXY(GTE_SXY2, &alt[2]);
                gte_cmd(GTE_NCLIP);
                altArea0 = gte_mfc2(GTE_MAC0);
                gte_mtc2(GTE_VXY0, Horizon.corner[3].w.xy);
                gte_mtc2(GTE_VZ0, Horizon.corner[3].w.z);
                gte_cmd(GTE_RTPS);
                if (offscreen) {
                    HorizonC8LoadCameraMatrix();
                    continue;
                }
                Horizon.ot = HorizonOrder(depth, Horizon.otBias);
                CourseOcclusionTile(Horizon.sxy, Horizon.ot, Horizon.cursor);
                RR_GTE_STORE_SXY(GTE_SXY2, &alt[3]);
                gte_cmd(GTE_NCLIP);
                altArea1 = gte_mfc2(GTE_MAC0);
                gte_cmd(GTE_AVSZ4);
                HorizonC8LoadCameraMatrix();
                /* The subdivision flag register holds the second projection's
                 * OTZ (near) or the level of detail (far) from here on. */
                probe = far ? lod : gte_mfc2(GTE_OTZ);
                subdivide = probe;
                if (!((altArea0 < 0) & (0 < altArea1)) && probe < 0x2580) {
                    s32 sz0 = gte_mfc2(GTE_SZ0), sz1 = gte_mfc2(GTE_SZ1);
                    s32 sz2 = gte_mfc2(GTE_SZ2), sz3 = gte_mfc2(GTE_SZ3);
                    /* The right-edge test compares a sign word with 0x140 and
                     * never rejects; the left-edge test uses the X signs. */
                    if (!(ScreenX(alt[0]) < 0 && ScreenX(alt[1]) < 0 && ScreenX(alt[2]) < 0 && ScreenX(alt[3]) < 0) &&
                        (sz0 < 0x2580 || sz1 < 0x2580 || sz2 < 0x2580 || sz3 < 0x2580) &&
                        !(sz0 < 0x120 && sz1 < 0x120 && sz2 < 0x120 && sz3 < 0x120)) {
                        u32 any;
                        Horizon.corner[0].w.color = HorizonC8Glow(alt[0], sz0);
                        Horizon.corner[1].w.color = HorizonC8Glow(alt[1], sz1);
                        Horizon.corner[2].w.color = HorizonC8Glow(alt[2], sz2);
                        Horizon.corner[3].w.color = HorizonC8Glow(alt[3], sz3);
                        any = (Horizon.corner[0].w.color | Horizon.corner[1].w.color |
                               Horizon.corner[2].w.color | Horizon.corner[3].w.color) << 8;
                        if (any != 0 || probe < 0x120) {
                            levelU += 2;
                            levelV += 2;
                            if (!(levelU < 4)) levelU = 4;
                            if (!(levelV < 4)) levelV = 4;
                            subdivide = (levelU | levelV) != 0;
                            Horizon.lineFlags |= 8;
                        }
                    }
                }
            } else {
                levelU = levelV = 0;
                subdivide = 0;
                if (!far) {
                    s32 levels = levels26 & 0x3FF;
                    levelV = (levels >> 5) - lod;
                    levelU = (levels & 0x1F) - lod;
                    if (levelU < 0) levelU = 0;
                    if (levelV < 0) levelV = 0;
                    subdivide = (levelU | levelV) != 0;
                }
                if (offscreen) continue;
                Horizon.ot = HorizonOrder(depth, Horizon.otBias);
                CourseOcclusionTile(Horizon.sxy, Horizon.ot, Horizon.cursor);
            }
            if (!far && subdivide) {
                Horizon.clut = (u16)(w[6] >> 16);
                Horizon.tpage = (u16)(w[7] >> 16);
                if (windowed) HorizonC8State(0xE2000000, 0);
                HORIZON_GRID(HorizonC8GridLevels, levelU, levelV, uvs);
                if (windowed) HorizonC8State(w[12], 0);
            } else if (Horizon.lineFlags & 8) {
                PolyGT4WordPacket *p;
                /* One PS1 chain of window reset, quad and window set, split
                 * into state and shape packets linked in the same order. */
                if (windowed) HorizonC8State(0xE2000000, 0);
                p = (PolyGT4WordPacket *)Horizon.cursor;
                p->uvClut0C = (s32)uvs[0];
                p->uvTpage18 = (s32)uvs[1];
                p->uv2 = (u16)uvs[2];
                p->uv3 = (u16)uvs[3];
                RR_SXY_COPY(p->xy0, Horizon.sxy[0]);
                RR_SXY_COPY(p->xy1, Horizon.sxy[1]);
                RR_SXY_COPY(p->xy2, Horizon.sxy[2]);
                RR_SXY_COPY(p->xy3, Horizon.sxy[3]);
                p->color04 = (s32)Horizon.corner[0].w.color;
                p->color10 = (s32)Horizon.corner[1].w.color;
                p->color1C = (s32)Horizon.corner[2].w.color;
                p->color28 = (s32)Horizon.corner[3].w.color;
                RR_GPU_ADD_PRIM(Horizon.ot, p, 12);
                Horizon.cursor = (u8 *)(p + 1);
                if (windowed) HorizonC8State(0xE2000000, w[12]);
            } else {
                PolyF4Packet *p = (PolyF4Packet *)Horizon.cursor;
                RR_SXY_COPY(p->xy0, Horizon.sxy[0]);
                RR_SXY_COPY(p->xy1, Horizon.sxy[1]);
                RR_SXY_COPY(p->xy2, Horizon.sxy[2]);
                RR_SXY_COPY(p->xy3, Horizon.sxy[3]);
                p->colorCode04 = 0x28000000;
                RR_GPU_ADD_PRIM(Horizon.ot, p, 5);
                Horizon.cursor = (u8 *)(p + 1);
            }
        }
    }
}

/* func_800686E8 */
static void HorizonPassC8a(const VisibleCourseViews *views)
{
    HorizonC8Pass(views, 2, 0x30, TrackRelSegment2Ptr, 0);
}

/* func_80069630 */
static void HorizonPassC8b(const VisibleCourseViews *views)
{
    HorizonC8Pass(views, 3, 0x34, RelFileOffsetPtr3, 1);
}

/* ---- Course 7 sections 11..13 (DrawWireQuadG12, func_8006C880, func_8006A604) ---- */

/* Normal-lit tile of sections 11 (0x44 bytes) and 12 (0x48 bytes, with the
 * texture-window word). The four normals interleave like the positions. */
typedef struct {
    s32 xy0;
    u32 z0z1;
    s32 xy1, xy2;
    u32 z2z3;
    s32 xy3;
    s32 n0xy;           /* 0x18 */
    u32 n0zn1z;         /* 0x1C */
    s32 n1xy, n2xy;     /* 0x20, 0x24 */
    u32 n2zn3z;         /* 0x28 */
    s32 n3xy;           /* 0x2C */
    u32 uv[3];          /* 0x30 uv0|clut, 0x34 uv1|tpage, 0x38 uv2|uv3 */
    u16 flags3C;        /* bit 0: halve far texture coordinates; high byte: OT bias */
    s16 levels3E;       /* bits 0..4 columns, 5..9 rows */
    u32 lineColor40;
    u32 textureWindow44; /* section 12 only */
} HorizonNormalTile;
_Static_assert(RR_FIELD_OFFSET(HorizonNormalTile, textureWindow44) == 0x44, "normal tile");

/* Flat normal-lit tile of section 13 (0x34 bytes). */
typedef struct {
    s32 xy0;
    u32 z0z1;
    s32 xy1, xy2;
    u32 z2z3;
    s32 xy3;
    s32 nxy;            /* 0x18 */
    u32 nz;             /* 0x1C */
    u32 uv[3];          /* 0x20, 0x24, 0x28 */
    u16 flags2C;
    s16 levels2E;
    u32 lineColor30;
} HorizonFlatNormalTile;
_Static_assert(sizeof(HorizonFlatNormalTile) == 0x34, "flat normal tile");

/* Scratch 0x160..0x178: the tile's normals as GTE vector words. */
static Vec3s8 HorizonC7Normal[4];

/* State packet of the course-7 windowed tiles: two E2 command words. */
static void HorizonC7StatePacket(u32 command0, u32 command1)
{
    GpuDrawModePacket *p = (GpuDrawModePacket *)Horizon.cursor;
    p->command[0] = command0;
    p->command[1] = command1;
    RR_GPU_ADD_PRIM(Horizon.ot, p, 2);
    Horizon.cursor = (u8 *)(p + 1);
}

/* DPCL with IR0 = 0: the line colour scaled by twice corner 0's colour. */
static u32 HorizonC7LineColorFromCorner(void)
{
    gte_mtc2(GTE_RGBC, (s32)Horizon.lineColor);
    gte_mtc2(GTE_IR0, 0);
    gte_mtc2(GTE_IR1, Horizon.corner[0].f.r << 5);
    gte_mtc2(GTE_IR2, Horizon.corner[0].f.g << 5);
    gte_mtc2(GTE_IR3, Horizon.corner[0].f.b << 5);
    gte_cmd(GTE_DPCL);
    return (u32)gte_mfc2(GTE_RGB2);
}

static void HorizonC7HalveUV(u32 uvs[4])
{
    uvs[0] = (uvs[0] & 0xFFFF0000) | ((uvs[0] >> 1) & 0x7F7F);
    uvs[1] = (uvs[1] & 0xFFFF0000) | ((uvs[1] >> 1) & 0x7F7F);
    uvs[2] = (uvs[2] >> 1) & 0x7F7F;
    uvs[3] = (uvs[3] >> 1) & 0x7F7F;
}

/* One FT4 covering the tile (far tiles and undivided flat tiles). */
static void HorizonC7WholeFT4(const u32 uvs[4], s32 color)
{
    GpuPolyFT4 *p = (GpuPolyFT4 *)Horizon.cursor;
    p->words.uv0 = (s32)uvs[0];
    p->words.uv1 = (s32)uvs[1];
    p->fields.u2 = (u8)uvs[2];
    p->fields.v2 = (u8)(uvs[2] >> 8);
    p->fields.u3 = (u8)uvs[3];
    p->fields.v3 = (u8)(uvs[3] >> 8);
    RR_SXY_COPY(p->words.xy0, Horizon.sxy[0]);
    RR_SXY_COPY(p->words.xy1, Horizon.sxy[1]);
    RR_SXY_COPY(p->words.xy2, Horizon.sxy[2]);
    RR_SXY_COPY(p->words.xy3, Horizon.sxy[3]);
    p->words.color04 = color;
    RR_GPU_ADD_PRIM(Horizon.ot, p, 9);
    Horizon.cursor = (u8 *)(p + 1);
}

/* Shared front of the three passes: project the tile, test facing, viewport,
 * depth range and level of detail exactly as func_80060D54. Returns 0 to
 * skip the tile; otherwise sets *far, the clamped levels and Horizon.ot.
 * normal0 (section 13) is lit with NCS after the viewport test, as there. */
static int HorizonC7Front(const s32 pos[4], const u32 zw[2], u32 tileFlags, s32 levelsWord,
                          u32 lineColor, const Vec3s8 *flatNormal,
                          s32 *farOut, s32 *halveOut, s32 *levelU, s32 *levelV, s32 *subdivide)
{
    RroSceneScratchPrefix *scene = Horizon.scene;
    s32 area0, area1, otz, depth, lod, mirror, offscreen, far;
    u32 flagsGte;
    int i;
    gte_mtc2(GTE_VXY0, pos[0]);
    gte_mtc2(GTE_VZ0, (s32)(zw[0] & 0xFFFF));
    gte_mtc2(GTE_VXY1, pos[1]);
    gte_mtc2(GTE_VZ1, (s32)(zw[0] >> 16));
    gte_mtc2(GTE_VXY2, pos[2]);
    gte_mtc2(GTE_VZ2, (s32)(zw[1] & 0xFFFF));
    HorizonSetCorner(0, pos[0], zw[0] & 0xFFFF, Horizon.corner[0].w.color);
    HorizonSetCorner(1, pos[1], zw[0] >> 16, Horizon.corner[1].w.color);
    HorizonSetCorner(2, pos[2], zw[1] & 0xFFFF, Horizon.corner[2].w.color);
    HorizonSetCorner(3, pos[3], zw[1] >> 16, Horizon.corner[3].w.color);
    gte_cmd(GTE_RTPT);
    RR_GTE_STORE_SXY(GTE_SXY0, &Horizon.sxy[0]);
    RR_GTE_STORE_SXY(GTE_SXY1, &Horizon.sxy[1]);
    RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[2]);
    flagsGte = (u32)gte_cfc2(GTE_FLAG);
    gte_cmd(GTE_NCLIP);
    area0 = gte_mfc2(GTE_MAC0);
    gte_mtc2(GTE_VXY0, pos[3]);
    gte_mtc2(GTE_VZ0, (s32)(zw[1] >> 16));
    gte_cmd(GTE_RTPS);
    Horizon.otBias = tileFlags >> 8;
    Horizon.lineColor = lineColor;
    RR_GTE_STORE_SXY(GTE_SXY2, &Horizon.sxy[3]);
    flagsGte |= (u32)gte_cfc2(GTE_FLAG);
    gte_cmd(GTE_NCLIP);
    Horizon.lineFlags = (flagsGte >> 31) | ((lineColor >> 31) << 1);
    area1 = gte_mfc2(GTE_MAC0);
    gte_cmd(GTE_AVSZ4);
    mirror = 0 < scene->renderFlags64;
    if (!HorizonFacing(((0 < area0) ^ mirror) | ((area1 < 0) ^ mirror))) return 0;
    {
        s32 x[4], y[4];
        for (i = 0; i < 4; i++) {
            x[i] = ScreenX(Horizon.sxy[i]);
            y[i] = ScreenY(Horizon.sxy[i]);
        }
        offscreen = QuadOutsideViewport(scene, x[0], x[1], x[2], x[3], y[0], y[1], y[2], y[3]);
    }
    otz = gte_mfc2(GTE_OTZ);
    far = HorizonFarDepth() < otz;
    if (flatNormal != NULL) {
        gte_ldv(0, flatNormal);
        gte_mtc2(GTE_RGBC, 0x2C000000);
        gte_cmd(GTE_NCS);
    }
    depth = otz >> (scene->depthShift60 & 31);
    if ((u32)depth >= 0x1C0) return 0;
    if (depth == 0 && !(0x122 < gte_mfc2(GTE_SZ0) || 0x122 < gte_mfc2(GTE_SZ1) ||
                        0x122 < gte_mfc2(GTE_SZ2) || 0x122 < gte_mfc2(GTE_SZ3)))
        return 0;
    lod = HorizonLod(otz);
    *halveOut = far & tileFlags & 1;
    *levelU = *levelV = *subdivide = 0;
    if (!far) {
        s32 levels = levelsWord & 0x3FF;
        *levelV = (levels >> 5) - lod;
        *levelU = (levels & 0x1F) - lod;
        if (*levelU < 0) *levelU = 0;
        if (*levelV < 0) *levelV = 0;
        *subdivide = (*levelU | *levelV) != 0;
    }
    if (offscreen) return 0;
    *farOut = far;
    Horizon.ot = HorizonOrder(depth, Horizon.otBias);
    CourseOcclusionTile(Horizon.sxy, Horizon.ot, Horizon.cursor);
    return 1;
}

/* DrawWireQuadG12 (section 11, RelFileSection11) and func_8006C880 (section
 * 12, RelFileOffsetPtr12): tiles lit from four normals (RGBC 3C000000; far
 * tiles one FT4 lit from normal 0 with RGBC 2C000000). Section 12 brackets
 * each tile with E2 texture-window packets: reset behind, the tile's window
 * (halved with the coordinates for far tiles) in front. */
static void HorizonPassNormalTiles(const VisibleCourseViews *views, int section, const u8 *base,
                                   u32 stride, int windowed)
{
    u32 view;
    for (view = 1; HorizonView(view, views); view++) {
        const CoursePrimitiveRange *range = &views->stream[view].record->ranges10[section];
        const u8 *cursor = base + (u32)range->first * stride;
        u32 n;
        for (n = range->count; n != 0; n--, cursor += stride) {
            const HorizonNormalTile *tile = (const HorizonNormalTile *)cursor;
            s32 pos[4] = {tile->xy0, tile->xy1, tile->xy2, tile->xy3};
            u32 zw[2] = {tile->z0z1, tile->z2z3};
            s32 far, halve, levelU, levelV, subdivide;
            u32 uvs[4];
            HorizonC7Normal[0].words[0] = (u32)tile->n0xy;
            HorizonC7Normal[0].words[1] = tile->n0zn1z;
            HorizonC7Normal[1].words[0] = (u32)tile->n1xy;
            HorizonC7Normal[1].words[1] = tile->n0zn1z >> 16;
            HorizonC7Normal[2].words[0] = (u32)tile->n2xy;
            HorizonC7Normal[2].words[1] = tile->n2zn3z;
            HorizonC7Normal[3].words[1] = tile->n2zn3z >> 16;
            if (!HorizonC7Front(pos, zw, tile->flags3C, tile->levels3E, tile->lineColor40, NULL,
                                &far, &halve, &levelU, &levelV, &subdivide))
                continue;
            uvs[0] = tile->uv[0];
            uvs[1] = tile->uv[1];
            uvs[2] = tile->uv[2] & 0xFFFF;
            uvs[3] = tile->uv[2] >> 16;
            if (far) {
                u32 window = windowed ? tile->textureWindow44 : 0;
                gte_ldv(0, &HorizonC7Normal[0]);
                gte_mtc2(GTE_RGBC, 0x2C000000);
                gte_cmd(GTE_NCS);
                if (halve) {
                    if (windowed) window = ((window >> 1) & 0x7BDEF) | 0xE2000000;
                    HorizonC7HalveUV(uvs);
                }
                if (windowed) HorizonC7StatePacket(0xE2000000, 0);
                HorizonC7WholeFT4(uvs, gte_mfc2(GTE_RGB2));
                if (windowed) HorizonC7StatePacket(0xE2000000, window);
                continue;
            }
            gte_ldv(0, &HorizonC7Normal[0]);
            gte_ldv(1, &HorizonC7Normal[1]);
            gte_mtc2(GTE_VXY2, (s32)HorizonC7Normal[2].words[0]);
            gte_mtc2(GTE_VZ2, (s32)HorizonC7Normal[2].words[1]);
            gte_mtc2(GTE_RGBC, 0x3C000000);
            gte_cmd(GTE_NCT);
            HorizonC7Normal[3].words[0] = (u32)tile->n3xy;
            if (subdivide) {
                Horizon.clut = (u16)(tile->uv[0] >> 16);
                Horizon.tpage = (u16)(tile->uv[1] >> 16);
                if (windowed) HorizonC7StatePacket(0xE2000000, 0);
                /* func_8006C3E4 / func_8006D138 preamble: corner colours from
                 * the four normals, line colour scaled by corner 0. */
                Horizon.corner[0].w.color = (u32)gte_mfc2(GTE_RGB0);
                Horizon.corner[1].w.color = (u32)gte_mfc2(GTE_RGB1);
                Horizon.corner[2].w.color = (u32)gte_mfc2(GTE_RGB2);
                gte_ldv(0, &HorizonC7Normal[3]);
                gte_cmd(GTE_NCS);
                Horizon.corner[3].w.color = (u32)gte_mfc2(GTE_RGB2);
                Horizon.lineColor = HorizonC7LineColorFromCorner();
                HORIZON_GRID(HorizonEmitLitGridLevels, levelU, levelV, uvs);
                if (windowed) HorizonC7StatePacket(tile->textureWindow44, 0);
            } else {
                PolyGT4WordPacket *p;
                if (windowed) HorizonC7StatePacket(0xE2000000, 0);
                p = (PolyGT4WordPacket *)Horizon.cursor;
                p->uvClut0C = (s32)uvs[0];
                p->uvTpage18 = (s32)uvs[1];
                p->uv2 = (u16)uvs[2];
                p->uv3 = (u16)uvs[3];
                RR_SXY_COPY(p->xy0, Horizon.sxy[0]);
                RR_SXY_COPY(p->xy1, Horizon.sxy[1]);
                RR_SXY_COPY(p->xy2, Horizon.sxy[2]);
                RR_SXY_COPY(p->xy3, Horizon.sxy[3]);
                p->color04 = gte_mfc2(GTE_RGB0);
                p->color10 = gte_mfc2(GTE_RGB1);
                p->color1C = gte_mfc2(GTE_RGB2);
                gte_ldv(0, &HorizonC7Normal[3]);
                gte_cmd(GTE_NCS);
                RR_GPU_ADD_PRIM(Horizon.ot, p, 12);
                p->color28 = gte_mfc2(GTE_RGB2);
                Horizon.cursor = (u8 *)(p + 1);
                if (windowed) HorizonC7StatePacket(0xE2000000, tile->textureWindow44);
            }
        }
    }
}

/* DrawWireQuadG12 */
static void HorizonPassC7a(const VisibleCourseViews *views)
{
    HorizonPassNormalTiles(views, 11, RelFileSection11, 0x44, 0);
}

/* func_8006C880 */
static void HorizonPassC7b(const VisibleCourseViews *views)
{
    HorizonPassNormalTiles(views, 12, RelFileOffsetPtr12, 0x48, 1);
}

/* func_8006AD04: HorizonEmitLitGrid with one colour (corner 0) and FT4
 * pieces; only positions and texture coordinates are interpolated. */
static void HorizonEmitFlatGridC7Levels(s32 levelU, s32 levelV, const u32 uv[4])
{
    HorizonVertex *cur = Horizon.rows[0], *prev = Horizon.rows[1];
    HorizonVertex *c = Horizon.corner;
    RroSceneScratchPrefix *scene = Horizon.scene;
    s32 stepRows = (s32)(0x1000u >> (levelU & 31));
    s32 stepCols;
    int last;
    s32 t;
    /* Host bound as in HorizonEmitLitGrid. */
    if ((levelV & 31) > 4) levelV = 4;
    stepCols = (s32)(0x1000u >> levelV);
    last = 1 << levelV;

    c[0].w.uv = (u16)uv[0];
    c[1].w.uv = (u16)uv[1];
    c[2].w.uv = (u16)uv[2];
    c[3].w.uv = (u16)uv[3];
    cur[0].w.xy = c[0].w.xy;
    cur[0].w.z = c[0].w.z;
    cur[0].w.uv = c[0].w.uv;
    cur[last].w.xy = c[2].w.xy;
    cur[last].w.z = c[2].w.z;
    cur[last].w.uv = c[2].w.uv;
    HorizonInterpRowFlat(cur, last, stepCols);
    if (!(Horizon.lineFlags & 2)) {
        if (!(Horizon.lineFlags & 1)) {
            HorizonLineF3(Horizon.sxy[0], Horizon.sxy[1], Horizon.sxy[3]);
            HorizonLineF3(Horizon.sxy[0], Horizon.sxy[2], Horizon.sxy[3]);
        }
        HorizonLineStrip(cur, last);
    }
    t = 0x1000;
    do {
        HorizonVertex *swap;
        int j;
        t -= stepRows;
        gte_mtc2(GTE_IR0, t);
        swap = cur;
        cur = prev;
        prev = swap;
        CourseIntpl(c[0].f.x, c[0].f.y, c[0].f.z, c[1].f.x, c[1].f.y, c[1].f.z);
        cur[0].f.x = (s16)gte_mfc2(GTE_IR1);
        cur[0].f.y = (s16)gte_mfc2(GTE_IR2);
        cur[0].f.z = (s16)gte_mfc2(GTE_IR3);
        cur[0].w.uv = (u16)HorizonDpcs((s16)c[1].w.uv, c[0].f.u << 4, c[0].f.v << 4, c[0].f.z);
        CourseIntpl(c[2].f.x, c[2].f.y, c[2].f.z, c[3].f.x, c[3].f.y, c[3].f.z);
        cur[last].f.x = (s16)gte_mfc2(GTE_IR1);
        cur[last].f.y = (s16)gte_mfc2(GTE_IR2);
        cur[last].f.z = (s16)gte_mfc2(GTE_IR3);
        cur[last].w.uv = (u16)HorizonDpcs((s16)c[3].w.uv, c[2].f.u << 4, c[2].f.v << 4, c[2].f.z);
        HorizonInterpRowFlat(cur, last, stepCols);
        if (!(Horizon.lineFlags & 2)) {
            HorizonLineF2(cur[0].w.xy, prev[0].w.xy);
            HorizonLineF2(cur[last].w.xy, prev[last].w.xy);
            if (t == 0) HorizonLineStrip(cur, last);
        }
        for (j = 0; j < last; j++) {
            const HorizonVertex *p0 = &prev[j], *p1 = &cur[j], *p2 = &prev[j + 1], *p3 = &cur[j + 1];
            s32 a, b, mirror;
            RR_GTE_LOAD_SXY(GTE_SXY0, &p0->w.xy);
            RR_GTE_LOAD_SXY(GTE_SXY1, &p1->w.xy);
            RR_GTE_LOAD_SXY(GTE_SXY2, &p2->w.xy);
            gte_cmd(GTE_NCLIP);
            if (QuadOutsideViewport(scene, ScreenX(p0->w.xy), ScreenX(p1->w.xy), ScreenX(p2->w.xy), ScreenX(p3->w.xy),
                                    ScreenY(p0->w.xy), ScreenY(p1->w.xy), ScreenY(p2->w.xy), ScreenY(p3->w.xy)))
                continue;
            a = gte_mfc2(GTE_MAC0);
            RR_GTE_LOAD_SXY(GTE_SXY0, &p3->w.xy);
            gte_cmd(GTE_NCLIP);
            b = gte_mfc2(GTE_MAC0);
            mirror = 0 < scene->renderFlags64;
            if (((0 < a) ^ mirror) | ((b < 0) ^ mirror)) {
                GpuPolyFT4 *p = (GpuPolyFT4 *)Horizon.cursor;
                RR_SXY_COPY(p->words.xy0, p0->w.xy);
                RR_SXY_COPY(p->words.xy1, p1->w.xy);
                RR_SXY_COPY(p->words.xy2, p2->w.xy);
                RR_SXY_COPY(p->words.xy3, p3->w.xy);
                p->words.uv0 = (s32)(((u32)Horizon.clut << 16) | p0->w.uv);
                p->words.uv1 = (s32)(((u32)Horizon.tpage << 16) | p1->w.uv);
                p->fields.u2 = (u8)p2->w.uv;
                p->fields.v2 = (u8)(p2->w.uv >> 8);
                p->fields.u3 = (u8)p3->w.uv;
                p->fields.v3 = (u8)(p3->w.uv >> 8);
                p->words.color04 = (s32)c[0].w.color;
                RR_GPU_ADD_PRIM(Horizon.ot, p, 9);
                Horizon.cursor = (u8 *)(p + 1);
            }
        }
    } while (t != 0);
}

/* func_8006A604: section 13 (RelFileOffsetPtr13) tiles of one colour, lit
 * from a single normal (NCS, RGBC 2C000000): far and undivided tiles as one
 * FT4, near tiles as a flat FT4 grid. */
static void HorizonPassC7c(const VisibleCourseViews *views)
{
    u32 view;
    for (view = 1; HorizonView(view, views); view++) {
        const CoursePrimitiveRange *range = &views->stream[view].record->ranges10[13];
        const HorizonFlatNormalTile *tile = (const HorizonFlatNormalTile *)RelFileOffsetPtr13 + range->first;
        u32 n;
        for (n = range->count; n != 0; n--, tile++) {
            s32 pos[4] = {tile->xy0, tile->xy1, tile->xy2, tile->xy3};
            u32 zw[2] = {tile->z0z1, tile->z2z3};
            s32 far, halve, levelU, levelV, subdivide;
            u32 uvs[4];
            HorizonC7Normal[0].words[0] = (u32)tile->nxy;
            HorizonC7Normal[0].words[1] = tile->nz;
            if (!HorizonC7Front(pos, zw, tile->flags2C, tile->levels2E, tile->lineColor30,
                                &HorizonC7Normal[0], &far, &halve, &levelU, &levelV, &subdivide))
                continue;
            Horizon.corner[0].w.color = (u32)gte_mfc2(GTE_RGB2);
            uvs[0] = tile->uv[0];
            uvs[1] = tile->uv[1];
            uvs[2] = tile->uv[2] & 0xFFFF;
            uvs[3] = tile->uv[2] >> 16;
            if (far) {
                if (halve) HorizonC7HalveUV(uvs);
            } else if (subdivide) {
                Horizon.lineColor = HorizonC7LineColorFromCorner();
                Horizon.clut = (u16)(uvs[0] >> 16);
                Horizon.tpage = (u16)(uvs[1] >> 16);
                HORIZON_GRID(HorizonEmitFlatGridC7Levels, levelU, levelV, uvs);
                continue;
            }
            HorizonC7WholeFT4(uvs, (s32)Horizon.corner[0].w.color);
        }
    }
}

/* Pass programs. Std/mirror lists: D_800A21B8/D_800A21E8 (courses 0..6),
 * D_800A2214/D_800A2254 (course 7), D_800A2290 (course 8). */
#define HORIZON_STD_PASSES     HorizonPassLitTiles, HorizonPass1, HorizonPass2, HorizonPass3, HorizonPass4,     HorizonPass5, HorizonPass6, HorizonPass7
static const HorizonPass HorizonProgramStd[] = {
    HORIZON_STD_PASSES, HorizonPass8, HorizonPass9, HorizonPass10, NULL};
static const HorizonPass HorizonProgramStdMirror[] = {
    HORIZON_STD_PASSES, HorizonPass9, HorizonPass10, NULL};
static const HorizonPass HorizonProgramCourse7[] = {
    HorizonPassC7a, HorizonPassC7b, HorizonPassC7c, HorizonPassC7d,
    HORIZON_STD_PASSES, HorizonPass8, HorizonPass9, HorizonPass10, NULL};
static const HorizonPass HorizonProgramCourse7Mirror[] = {
    HorizonPassC7a, HorizonPassC7b, HorizonPassC7c, HorizonPassC7d,
    HORIZON_STD_PASSES, HorizonPass9, HorizonPass10, NULL};
static const HorizonPass HorizonProgramCourse8[] = {HorizonPassC8a, HorizonPassC8b, NULL};
#undef HORIZON_STD_PASSES
static const HorizonPass *const HorizonPrograms[9] = {
    HorizonProgramStd, HorizonProgramStd, HorizonProgramStd, HorizonProgramStd,
    HorizonProgramStd, HorizonProgramStd, HorizonProgramStd,
    HorizonProgramCourse7, HorizonProgramCourse8};
static const HorizonPass *const HorizonMirrorPrograms[9] = {
    HorizonProgramStdMirror, HorizonProgramStdMirror, HorizonProgramStdMirror,
    HorizonProgramStdMirror, HorizonProgramStdMirror, HorizonProgramStdMirror,
    HorizonProgramStdMirror, HorizonProgramCourse7Mirror, HorizonProgramCourse8};

/* 80060C70: run the course's pass program (D_800A229C, or D_800A22C0 when
 * mirrored), keeping the packet cursor in Horizon and the far colour saved. */
void SubmitCourseHorizon(void *scene, VisibleCourseViews *views, s32 course)
{
    const HorizonPass *program;
    int projection = Psyz_GteSetExtendedProjection(ExtendedDrawDistance);
    Horizon.scene = (RroSceneScratchPrefix *)scene;
    Horizon.savedFarColor[0] = gte_cfc2(GTE_FC_R);
    Horizon.savedFarColor[1] = gte_cfc2(GTE_FC_G);
    Horizon.savedFarColor[2] = gte_cfc2(GTE_FC_B);
    program = (Horizon.scene->renderFlags64 == 0 ? HorizonPrograms : HorizonMirrorPrograms)[course];
    Horizon.cursor = RR_GPU_SCRATCH->cursor;
    CourseOcclusionCourseBegin(Horizon.scene);
    for (; *program != NULL; program++) {
        (*program)(views);
    }
    CourseOcclusionCourseEnd(Horizon.cursor);
    gte_ctc2(GTE_FC_R, Horizon.savedFarColor[0]);
    gte_ctc2(GTE_FC_G, Horizon.savedFarColor[1]);
    gte_ctc2(GTE_FC_B, Horizon.savedFarColor[2]);
    RR_GPU_SCRATCH->cursor = Horizon.cursor;
    FaceDepthClassEnd();
    Psyz_GteSetExtendedProjection(projection);
}

/* ------------------------------------------------------------------------ */
/* Animated track lines                                                     */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* EmitAnimatedTrackLines (8006DB0C)                                        */
/* ------------------------------------------------------------------------ */

/* Shrink a scale by 1/32 + 1/64 of itself (unsigned shifts). */
static s32 AnimatedTrackDecay(s32 scale)
{
    return scale - (s32)(((u32)scale >> 5) + ((u32)scale >> 6));
}

/* One particle position step: IR = step * scale (INTPL toward a zero far
 * colour with IR0 = 0x1000 - scale), then position += IR with 12 added to y.
 * The new position is written back to the particle and returned packed. */
/* split: 0 the whole step; at field rate 1 on the tick's field (IR - IR/2),
 * 2 on the field after it (IR/2), the two adding the tick's step. */
static s32 AnimatedTrackPart(s32 ir, int split)
{
    return split == 1 ? ir - ir / 2 : split == 2 ? ir / 2 : ir;
}

static void AnimatedTrackAdvance(Vec3sFields8 *position, const Vec3sFields8 *step,
                                 s32 *xyOut, s32 *zOut, int split)
{
    s32 x, y, z, xy;
    gte_mtc2(GTE_IR1, step->x);
    gte_mtc2(GTE_IR2, step->y);
    gte_mtc2(GTE_IR3, step->z);
    gte_cmd(GTE_INTPL);
    x = position->x + AnimatedTrackPart(gte_mfc2(GTE_IR1), split);
    y = position->y + (split ? 6 : 0xC) + AnimatedTrackPart(gte_mfc2(GTE_IR2), split);
    z = position->z + AnimatedTrackPart(gte_mfc2(GTE_IR3), split);
    xy = (s32)(((u32)x & 0xFFFF) | ((u32)y << 16));
    position->x = (s16)x;
    position->y = (s16)y;
    position->z = (s16)z;
    *xyOut = xy;
    *zOut = z;
}

/* Fill the colour and both projected ends of three prebuilt Gouraud line
 * packets per particle triple (links are left as the caller built them).
 * While the object advances (advance30 > 0) the particles move by their
 * decaying steps: the head (position00/step10) always, the tail
 * (position08/step18) only until frame14 reaches startFrame28 + 15. The
 * packet colours come from palette48/palette88 at colorOffset (a byte offset
 * of a palette word), moving one entry down (wrapping 0 -> 15) every
 * field0C triples. Returns the packet after the last one written. */
AnimatedLinePacket *EmitAnimatedTrackLines(AnimatedTrackScratch *scratch,
                                           AnimatedLinePacket *cursor,
                                           AnimatedTrackObject *object, s32 colorOffset)
{
    AnimatedTrackParticle *particle = object->particles00;
    s32 remaining = object->capacity08;
    s32 tailFrozen = object->frame14 < object->startFrame28 + 0xF;
    s32 advancing = 0 < object->advance30;
    s32 headFactor = 0x1000 - object->scale20;
    s32 tailFactor = 0x1000 - object->scale24;
    s32 colorIndex = colorOffset >> 2;
    s32 countdown = 0;
    u32 color0 = 0, color1 = 0;
    /* At field rate a tick's step over its two fields: the same scales
     * (decayed after the second), half the move each (rr/field_rate.h). */
    const int split = RaceFieldRateLive ? (RaceTickField ? 1 : 2) : 0;
    (void)scratch;

    if (advancing && split != 1) {
        object->scale20 = AnimatedTrackDecay(object->scale20);
        if (!tailFrozen) object->scale24 = AnimatedTrackDecay(object->scale24);
    }
    if (remaining <= 0) return cursor;
    if (advancing) {
        gte_ctc2(GTE_FC_R, 0);
        gte_ctc2(GTE_FC_G, 0);
        gte_ctc2(GTE_FC_B, 0);
    }
    for (;;) {
        s32 headXY[3], headZ[3], tailXY[3], tailZ[3];
        int k;
        if (advancing) {
            gte_mtc2(GTE_IR0, headFactor);
            for (k = 0; k < 3; k++)
                AnimatedTrackAdvance(&particle[k].position00, &particle[k].step10, &headXY[k], &headZ[k], split);
            if (!tailFrozen) {
                gte_mtc2(GTE_IR0, tailFactor);
                for (k = 0; k < 3; k++)
                    AnimatedTrackAdvance(&particle[k].position08, &particle[k].step18, &tailXY[k], &tailZ[k], split);
            }
        } else {
            for (k = 0; k < 3; k++) {
                headXY[k] = (s32)(((u32)(u16)particle[k].position00.x) | ((u32)particle[k].position00.y << 16));
                headZ[k] = particle[k].position00.z;
            }
        }
        if (!advancing || tailFrozen) {
            for (k = 0; k < 3; k++) {
                tailXY[k] = (s32)(((u32)(u16)particle[k].position08.x) | ((u32)particle[k].position08.y << 16));
                tailZ[k] = particle[k].position08.z;
            }
        }
        if (countdown <= 0) {
            color0 = object->palette48.words[colorIndex];
            color1 = object->palette88.words[colorIndex];
            if (--colorIndex < 0) colorIndex = 15;
            countdown = object->field0C;
        }
        gte_mtc2(GTE_VXY0, headXY[0]);
        gte_mtc2(GTE_VZ0, headZ[0]);
        gte_mtc2(GTE_VXY1, headXY[1]);
        gte_mtc2(GTE_VZ1, headZ[1]);
        gte_mtc2(GTE_VXY2, headXY[2]);
        gte_mtc2(GTE_VZ2, headZ[2]);
        gte_cmd(GTE_RTPT);
        RR_GTE_STORE_SXY(GTE_SXY0, &cursor[0].xy0);
        RR_GTE_STORE_SXY(GTE_SXY1, &cursor[1].xy0);
        RR_GTE_STORE_SXY(GTE_SXY2, &cursor[2].xy0);
        gte_mtc2(GTE_VXY0, tailXY[0]);
        gte_mtc2(GTE_VZ0, tailZ[0]);
        gte_mtc2(GTE_VXY1, tailXY[1]);
        gte_mtc2(GTE_VZ1, tailZ[1]);
        gte_mtc2(GTE_VXY2, tailXY[2]);
        gte_mtc2(GTE_VZ2, tailZ[2]);
        gte_cmd(GTE_RTPT);
        for (k = 0; k < 3; k++) {
            cursor[k].color0.word = color0;
            cursor[k].color1.word = color1;
        }
        RR_GTE_STORE_SXY(GTE_SXY0, &cursor[0].xy1);
        RR_GTE_STORE_SXY(GTE_SXY1, &cursor[1].xy1);
        RR_GTE_STORE_SXY(GTE_SXY2, &cursor[2].xy1);
        remaining -= 3;
        cursor += 3;
        if (remaining <= 0) break;
        particle += 3;
        countdown--;
    }
    return cursor;
}

/* Initial values of the globals this module owns. */
u8 *TrackRelSegment2Ptr;
u8 *RelFileOffsetPtr3;
MatrixWords32 CameraYawRotMatrix;
