#include "rr/video_timing.h"
#include "game/car/carbound.h"
#include "rr/rro_model.h"
#include "game/gfx/model.h"
#include "psyq/libgte/gtemat4.h"
#include "game/car/carwheel.h"
#include "rr/frame_storage_types.h"
#include "game/camera/viewcam.h"
#include "types.h"
#include "rr/scene_camera_state.h"
#include "rr/player_cars.h"
#include "scratchpad.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "rr/geometry.h"
#include "game/gfx/gfxdraw.h"
#include "game/gfx/meshquad.h"
#include "game/gfx/scene1.h"
#include "game/camera/camview.h"
#include "rr/race_types.h"
#include "rr/matrix_workspace.h"
#include "rr/frame_storage_types.h"
#include "game/math/rotmat.h"


void DrawGarageFloorReflection(void)
{
    struct {
        MatrixWords32 m18, m38;
        /* The existing work capacity beyond this matrix remains unresolved. */
        union { MatrixWords32 matrix; u8 storage[0x28]; } m58;
        VectorWords16 v80;
        PreviewColor color90;
        u8 reserved94[24];
    } L;
    RroSceneScratchPrefix *g = (RroSceneScratchPrefix *)SCRATCHPAD;
    s32 s6;
    int a1;
    GpuDmaTag *secondary;
    GpuDmaTag *entry;
    int t;

    s6 = g->depthShift60;
    g->depthShift60 = 3;
    SetupObjectLightMatrices(&RaceContext.camera24C->pose00, &RaceContext.camera24C->lighting78, &L.m18, &L.m38);
    SelectModelBank(8);
    SetGteObjectTransform((ObjectTransform38 *)(SCRATCHPAD + 0x7C), (const s16 *)&RaceContext.camera24C->pose00.position00, (s32 *)&L.m18);
    g->draw74.draw.counter = 0;
    SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, ModelBankFormat < 1, 0, 0);
    SetGteObjectTransform((ObjectTransform38 *)(SCRATCHPAD + 0x7C), (const s16 *)&RaceContext.camera24C->pose00.position00, (s32 *)&L.m18);
    DrawMeshPrimList(RaceContext.model560, 0, 0);
    TransformPoint(&L.m38, &RaceContext.camera24C->pose00.position00.xyz, &RaceContext.camera24C->parts18[0].position00.xyz, &L.v80.xyz, &L.m58.matrix);
    BuildRotMatrixY(&L.m58.matrix, -RaceContext.camera24C->parts18[0].rotation10.y);
    MulMatrix2((const MatrixWords32 *)(&L.m18), &L.m58.matrix);
    SetGteObjectTransform((ObjectTransform38 *)(SCRATCHPAD + 0x7C), (const s16 *)&L.v80, L.m58.matrix.w);
    g->draw74.draw.counter = 0;
    SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, 1, 0, 0);
    TransformPoint(&L.m38, &RaceContext.camera24C->pose00.position00.xyz, &RaceContext.camera24C->parts18[1].position00.xyz, &L.v80.xyz, &L.m58.matrix);
    BuildRotMatrixY(&L.m58.matrix, -RaceContext.camera24C->parts18[1].rotation10.y);
    MulMatrix2((const MatrixWords32 *)(&L.m18), &L.m58.matrix);
    SetGteObjectTransform((ObjectTransform38 *)(SCRATCHPAD + 0x7C), (const s16 *)&L.v80, L.m58.matrix.w);
    a1 = 1;
    g->draw74.draw.counter = 0;
    if (ModelBankFormat >= 3)
        a1 = 2;
    SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1, 0, 0);
    TransformPoint(&L.m38, &RaceContext.camera24C->pose00.position00.xyz, &RaceContext.camera24C->parts18[3].position00.xyz, &L.v80.xyz, &L.m58.matrix);
    BuildRotMatrixX(&L.m58.matrix, -RaceContext.camera24C->parts18[3].rotation10.x);
    MulMatrix2((const MatrixWords32 *)(&L.m18), &L.m58.matrix);
    SetGteObjectTransform((ObjectTransform38 *)(SCRATCHPAD + 0x7C), (const s16 *)&L.v80, L.m58.matrix.w);
    DrawMeshPrimList(RaceContext.model560, 2, 0);
    TransformPoint(&L.m38, &RaceContext.camera24C->pose00.position00.xyz, &RaceContext.camera24C->parts18[2].position00.xyz, &L.v80.xyz, &L.m58.matrix);
    BuildRotMatrixY(&L.m58.matrix, -RaceContext.camera24C->parts18[2].rotation10.y);
    MulMatrix2((const MatrixWords32 *)(&L.m18), &L.m58.matrix);
    SetGteObjectTransform((ObjectTransform38 *)(SCRATCHPAD + 0x7C), (const s16 *)&L.v80, L.m58.matrix.w);
    DrawMeshPrimList(RaceContext.model560, RaceContext.camera24C->mirrorA4 * 2 | 1, 0);
    g->depthShift60 = s6;
    L.color90.channels.r = 0x80;
    L.color90.channels.g = 0x80;
    L.color90.channels.b = 0x80;
    RR_GPU_SCRATCH->order = &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0];
    UpdateCarWorldTransform(PlayerCars);
    DrawPreviewCar(PlayerCars, L.color90.argument, 0x80, 1);
    t = RotTransPersOrigin();
    t = (t >> 3) + 0x64;
    /* Splice the reflected car's secondary table in at its floor depth. */
    secondary = RR_GPU_SCRATCH->order;
    entry = &CurrentDrawBuf->buffer.ordering70.tags[t];
    secondary[0].address = entry->address;
    entry->address = (uintptr_t)&secondary[703];
    RR_GPU_SCRATCH->order = &CurrentDrawBuf->buffer.ordering70.tags[0];
}

/* Curve keys count units of 10 frames (NTSC release: 12 frames). */
#define CURVE_KEY_FRAMES RR_PAL_NTSC(10, 12)

s32 FindCurveSegment(PairS16 *p, s32 a1) {
    int t = a1 / CURVE_KEY_FRAMES;
    int i = 0;
    while (p[i].b >= 0) {
        if (t < p[i].b) break;
        i++;
    }
    return i;
}

s32 InterpCurveLinear(PairS16 *t, s32 idx, s32 x) {
    PairS16 *p;
    if (idx == 0) return t[0].a;
    p = &t[idx];
    if (p->b < 0) return p->a;
    return (p->a - p[-1].a) * (x - p[-1].b * CURVE_KEY_FRAMES) / ((p->b - p[-1].b) * CURVE_KEY_FRAMES) + p[-1].a;
}

s32 InterpCurveEase(s32 a0)
{
  s32 unused[2];
  int i;
  int r;
  int q;
  short *p;
  int v1;
  int a1;
  int a2;
  int a3;
  int d;
  int h;
  i = FindCurveSegment(CarCameraHeightOffsets, a0);
  if (i == 0)
  {
    return CarCameraHeightOffsets[0].a;
  }
  p = (short *) &CarCameraHeightOffsets[i];
  if (p[1] < 0)
  {
    return p[0];
  }
  v1 = p[-1];
  a3 = p[0];
  a1 = p[1] - v1;
  a1 = a1 * CURVE_KEY_FRAMES;
  d = a0 - (v1 * CURVE_KEY_FRAMES);
  a2 = a3 - p[-2];
  h = a1 >> 1;
  if (d < h)
  {
    r = (((d * d) * a2) / 2) / (h * h);
  }
  else
  {
    d = a1 - d;
    q = (((d * d) * a2) / 2) / (h * h);
    r = a3 - q;
  }
  q = r;
  return q;
}

/* Initial values of the globals this module owns. */
PairS16 CarCameraHeightOffsets[3] = {{.b = 10}, {.a = 1536, .b = 40}, {.a = 1536, .b = -1}};
