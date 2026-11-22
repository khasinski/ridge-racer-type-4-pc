#include "rr/gpu_scratch_types.h"
#include "rr/video_timing.h"
#include "game/system/pad.h"
#include "game/car/carbound.h"
#include "rr_asm.h"
#include "rr/gpu.h"

/* Candidate module: mesh quad packet builders (GT4, G4, F4), 80057A44..80058038. */
#include "types.h"
#include "rr/scene_camera_state.h"
#include "rr/player_cars.h"
#include "game/car/bounds.h"
#include "scratchpad.h"
#include "game/camera/camview.h"
#include "game/effect/spark.h"
#include "game/gfx/meshquad.h"
#include "rr/overlay665_render.h"
#include "game/gfx/gfxotag.h"
#include "game/effect/carfx.h"
#include "rr/race_types.h"
#define RTPT() RR_GTE_RTPT_MEM()
#define RTPS() RR_GTE_RTPS_MEM()
#define NCLIP() RR_GTE_NCLIP_MEM()
#define AVSZ4() RR_GTE_AVSZ4_MEM()

/* Per-call projection work. The PS1 kept it at scratchpad 0x7C including the
 * vertex-pool and ordering-table pointers; on the host those are pointers, so
 * the work area is an ordinary local passed to the emitters. */
typedef struct {
    const u8 *vertices;
    s32 count;
    s32 sxy[3];   /* SXY0..SXY2 after RTPT */
    s32 sxy3;     /* SXY2 after RTPS of the fourth vertex */
    s32 winding;  /* NCLIP of the first triangle */
    s32 winding2; /* NCLIP of the second triangle (F4 only) */
    s32 depth;    /* OTZ, then the depth-shifted bucket */
    s32 bias;
    GpuDmaTag *ordering;
} MeshQuadWork;

static const MeshQuadGT4 *DrawMeshQuadGT4(const MeshQuadGT4 *p, MeshQuadWork *s);
static const MeshQuadG4 *DrawMeshQuadG4(const MeshQuadG4 *p, MeshQuadWork *s);
static const MeshQuadF4 *DrawMeshQuadF4(const MeshQuadF4 *p, MeshQuadWork *s);

int StepCarShowcaseScene(int a0)
{
  int sp10[2];
  int s1;
  s1 = 4;
  {
    u32 extent = RaceContext.camera24C->extentA0;
    RaceContext.camera24C->distance100 = extent - s1 - (CarBoundsTable[0]->unk08 + PlayerCars[0].halfSpan10E / 3);
  }
  s1 = StepCarShowcasePhysics(&PlayerCars[0], RaceContext.camera24C->frame104, a0, sp10);
  UpdateSparkEffects();
  ScaleMatrixRow1(2, (MatrixWords32 *)SCRATCH_ROT_MATRIX);
  DrawGarageFloorReflection();
  RaceContext.camera24C->frame104 = RaceContext.camera24C->frame104 + 1U;
  RaceContext.camera24C->phase108 = RaceContext.camera24C->phase108 + 1U;
  if (RaceContext.camera24C->frame104 >= RR_PAL_NTSC(0x322, 0x3C2))
  {
    RaceContext.camera24C->frame104 = RR_PAL_NTSC(0x321, 0x3C1);
  }
  if (RaceContext.camera24C->phase108 >= RR_PAL_NTSC(0x516, 0x61A))
  {
    RaceContext.camera24C->phase108 = RR_PAL_NTSC(0x515, 0x619);
  }
  if (Pad0Buttons & 0x800)
  {
    if (RaceContext.camera24C->frame104 >= RR_PAL_NTSC(0x33, 0x3D))
    {
      s1 = 1;
    }
  }
  return s1;
}

/* Face color words already carry their GPU command code (0x3C/0x38/0x28);
 * the PS1 code byte store preceding them was overwritten by the word store. */
static GpuDmaTag *MeshQuadOrderEntry(const MeshQuadWork *s, s32 otBias) {
    return s->ordering + (s->bias + otBias + s->depth + 0x80);
}

static const MeshQuadGT4 *DrawMeshQuadGT4(const MeshQuadGT4 *p, MeshQuadWork *s) {
    s32 sh = ((RroSceneScratchPrefix *)SCRATCHPAD)->depthShift60;
    const u8 *verts = s->vertices;
    s32 n = s->count;
    PolyGT4WordPacket *t1 = (PolyGT4WordPacket *)RR_GPU_SCRATCH->cursor;
    while (n-- > 0) {
        RR_GTE_LWC2_V012_3P(verts + (p->v0 << 3), verts + (p->v1 << 3), verts + (p->v2 << 3));
        RTPT();
        t1->color04 = p->color08;
        t1->color10 = p->color0C;
        NCLIP();
        RR_GTE14_SWC2_MAC0(&s->winding);
        RR_GTE14_SWC2_SXYZ(s->sxy);
        if (s->winding > 0) {
            RR_GTE_LWC2_PAIR_AT(0, 0, 1, 4, verts + (p->v3 << 3));
            RTPS();
            RR_SXY_COPY(t1->xy0, s->sxy[0]);
            RR_SXY_COPY(t1->xy1, s->sxy[1]);
            RR_GTE_SWC2_AT(14, 0, &s->sxy3);
            AVSZ4();
            RR_GTE14_SWC2_OTZ(&s->depth);
            s->depth >>= sh;
            if (s->depth < 0x1C1) {
                RR_SXY_COPY(t1->xy2, s->sxy[2]);
                RR_SXY_COPY(t1->xy3, s->sxy3);
                t1->uvClut0C = p->uvClut18;
                t1->uvTpage18 = p->uvTpage1C;
                t1->uv2 = p->uv2;
                t1->uv3 = p->uv3;
                t1->color1C = p->color10;
                t1->color28 = p->color14;
                RR_GPU_ADD_PRIM(MeshQuadOrderEntry(s, (s8)p->otBias25), t1, 0xC);
                t1++;
            }
        }
        p++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)t1;
    return p;
}

static const MeshQuadG4 *DrawMeshQuadG4(const MeshQuadG4 *p, MeshQuadWork *s) {
    s32 sh = ((RroSceneScratchPrefix *)SCRATCHPAD)->depthShift60;
    const u8 *verts = s->vertices;
    s32 n = s->count;
    PolyG4WordPacket *t1 = (PolyG4WordPacket *)RR_GPU_SCRATCH->cursor;
    while (n-- > 0) {
        RR_GTE_LWC2_V012_3P(verts + (p->v0 << 3), verts + (p->v1 << 3), verts + (p->v2 << 3));
        RTPT();
        t1->color04 = p->color08;
        t1->color0C = p->color0C;
        NCLIP();
        RR_GTE14_SWC2_MAC0(&s->winding);
        RR_GTE14_SWC2_SXYZ(s->sxy);
        if (s->winding > 0) {
            RR_GTE_LWC2_PAIR_AT(0, 0, 1, 4, verts + (p->v3 << 3));
            RTPS();
            RR_SXY_COPY(t1->xy0, s->sxy[0]);
            RR_SXY_COPY(t1->xy1, s->sxy[1]);
            RR_GTE_SWC2_AT(14, 0, &s->sxy3);
            AVSZ4();
            RR_GTE14_SWC2_OTZ(&s->depth);
            s->depth >>= sh;
            if (s->depth < 0x1C1) {
                RR_SXY_COPY(t1->xy2, s->sxy[2]);
                RR_SXY_COPY(t1->xy3, s->sxy3);
                t1->color14 = p->color10;
                t1->color1C = p->color14;
                RR_GPU_ADD_PRIM(MeshQuadOrderEntry(s, (s8)p->otBias19), t1, 8);
                t1++;
            }
        }
        p++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)t1;
    return p;
}

static const MeshQuadF4 *DrawMeshQuadF4(const MeshQuadF4 *p, MeshQuadWork *s) {
    s32 sh = ((RroSceneScratchPrefix *)SCRATCHPAD)->depthShift60;
    const u8 *verts = s->vertices;
    s32 n = s->count;
    PolyF4Packet *t1 = (PolyF4Packet *)RR_GPU_SCRATCH->cursor;
    while (n-- > 0) {
        RR_GTE_LWC2_V012_3P(verts + (p->v0 << 3), verts + (p->v1 << 3), verts + (p->v2 << 3));
        RTPT();
        t1->colorCode04 = p->color08;
        NCLIP();
        RR_GTE14_SWC2_MAC0(&s->winding);
        RR_GTE14_SWC2_SXYZ(s->sxy);
        RR_GTE_LWC2_PAIR_AT(0, 0, 1, 4, verts + (p->v3 << 3));
        RTPS();
        RR_SXY_COPY(t1->xy0, s->sxy[0]);
        RR_SXY_COPY(t1->xy1, s->sxy[1]);
        NCLIP();
        RR_GTE_SWC2_AT(14, 0, &s->sxy3);
        RR_GTE_SWC2_AT(24, 0, &s->winding2);
        if (s->winding > 0 || s->winding2 < 0) {
            AVSZ4();
            RR_GTE14_SWC2_OTZ(&s->depth);
            s->depth >>= sh;
            if (s->depth < 0x1C1) {
                RR_SXY_COPY(t1->xy2, s->sxy[2]);
                RR_SXY_COPY(t1->xy3, s->sxy3);
                RR_GPU_ADD_PRIM(MeshQuadOrderEntry(s, (s8)p->otBias0D), t1, 5);
                t1++;
            }
        }
        p++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)t1;
    return p;
}

/* Draws mesh `index` of a decoded model table. Its command stream holds
 * (count << 16 | kind) headers, each followed by count face records, and
 * ends with a zero word. */
void DrawMeshPrimList(const RroModelTable *table, int index, int bias) {
    MeshQuadWork work;
    const u32 *a = table->entries[index].commands;
    u32 v;
    work.bias = bias;
    work.ordering = RR_GPU_SCRATCH->order;
    work.vertices = (const u8 *)table->entries[index].vertices;
    while ((v = *a++) != 0) {
        work.count = (s32)v >> 16;
        switch (v & 0xFFFF) {
        case 0:
            a = (const u32 *)DrawMeshQuadF4((const MeshQuadF4 *)a, &work);
            break;
        case 12:
            a = (const u32 *)DrawMeshQuadGT4((const MeshQuadGT4 *)a, &work);
            break;
        case 8:
            a = (const u32 *)DrawMeshQuadG4((const MeshQuadG4 *)a, &work);
            break;
        }
    }
}
