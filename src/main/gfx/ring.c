#include "rr/bios_devices.h"
#include "rr/scene_camera_state.h"
#include "game/camera/viewcam.h"
#include "scratchpad.h"
#include "rr/gpu_scratch_types.h"
#include "rr/gpu.h"
#include "rr/ring_renderer.h"
#include "rr/game_motion.h"
#include "game/gfx/ring.h"
#include "rr_asm.h"
#include "rr/menu_overlay_runtime.h"
#include "rr/menu_text_rows.h"
#include "game/gfx/scene1.h"

/* PsyZ libgte ScaleMatrix(MATRIX *, VECTOR *) and RotTransPers(SVECTOR *,
 * long *, long *, long *); MatrixWords32/RingVertex8 share their layouts. */
extern MatrixWords32 *ScaleMatrix(MatrixWords32 *matrix, s32 *scale);
extern s32 RotTransPers(RingVertex8 *point, s32 *screen, s32 *interpolation, s32 *flags);

static s32 project_ring_origin(s32 *position, s32 radius,
    RingScratch48 *scratch, RingVertex8 *point, s32 *output)
{
    s32 depth, innerOffset;
  point->x = position[0];
  point->y = position[1];
  point->z = position[2];
  depth = RotTransPers(point, output, output, output) >> 1;
  scratch->geometry.vectors[0].x = 0;
  scratch->geometry.vectors[0].y = radius;
  scratch->geometry.vectors[0].z = position[2];
  RR_GTE_RTPS_LDV(&scratch->geometry.vectors[0]);
  RR_GTE_SWC2_SXY2(&scratch->geometry.screen[0]);
  scratch->geometry.vectors[1].x = 0;
  innerOffset = radius - 128;
  scratch->geometry.vectors[1].y = innerOffset;
  scratch->geometry.vectors[1].z = position[2];
  RR_GTE_RTPS_LDV(&scratch->geometry.vectors[1]);
  RR_GTE_SWC2_SXY2(&scratch->geometry.screen[1]);
  return depth;
}

void DrawRing(s32 *position, s32 radius)
{
  s32 scale[3];
  SceneCameraPose18 transform;
  MatrixWords32 rotation;
  MatrixWords32 camera;
  RingVertex8 point;
  s32 transformed;
  RingScratch48 *scratch = (RingScratch48 *) (SCRATCHPAD + 0x7C);
  RingPacket24 *packet;
  s32 segment;
  s32 angle;
  s32 depth;
  s32 inner;
  GpuDmaTag *head;
  transform.position00.xyz.x = position[0];
  transform.position00.xyz.y = position[1];
  transform.position00.xyz.z = position[2];
  transform.rotation10.z = 0;
  transform.rotation10.y = 0;
  transform.rotation10.x = 0;
  SetupObjectLightMatrices(&transform, &MotionControllerView()->lighting20, &rotation, &camera);
  scale[2] = 1024;
  scale[1] = 1024;
  scale[0] = 1024;
  ScaleMatrix(&rotation, scale);
  SetGteObjectTransform(&scratch->transform, (const s16 *)&transform.position00, rotation.w);
  scratch->geometry.red = 220;
  scratch->geometry.green = 160;
  scratch->geometry.blue = 0;
  depth = project_ring_origin(position, radius, scratch, &point, &transformed);
  segment = 0;
  inner = radius - 128;
  head = RR_GPU_SCRATCH->order + depth;
  packet = (RingPacket24 *)RR_GPU_SCRATCH->cursor;
  do
  {
    angle = segment << 7;
    scratch->geometry.vectors[2].x = (rsin(angle) * radius) / 4096;
    scratch->geometry.vectors[2].y = (rcos(angle) * radius) / 4096;
    scratch->geometry.vectors[2].z = position[2];
    scratch->geometry.vectors[3].x = (rsin(angle) * inner) / 4096;
    scratch->geometry.vectors[3].y = (rcos(angle) * inner) / 4096;
    scratch->geometry.vectors[3].z = position[2];
    RR_GTE_RTPT_LDV(&scratch->geometry.vectors[2], &scratch->geometry.vectors[3], &scratch->geometry.vectors[3]);
    RING_STORE_SXY_PAIR(&scratch->geometry.screen[2], &scratch->geometry.screen[3]);
    packet->tag.length = 5;
    packet->code = 0x28;
    RrSxyCopy(packet->xy[0], &scratch->geometry.screen[0]);
    RrSxyCopy(packet->xy[1], &scratch->geometry.screen[1]);
    RrSxyCopy(packet->xy[2], &scratch->geometry.screen[2]);
    RrSxyCopy(packet->xy[3], &scratch->geometry.screen[3]);
    segment++;
    packet->red = scratch->geometry.red;
    packet->green = scratch->geometry.green;
    packet->blue = scratch->geometry.blue;
    RR_GPU_LINK(head, packet);
    RrSxyCopy(&scratch->geometry.screen[0], &scratch->geometry.screen[2]);
    RrSxyCopy(&scratch->geometry.screen[1], &scratch->geometry.screen[3]);
    packet++;
  }
  while (segment < 33);
  RR_GPU_SCRATCH->cursor = (u8 *)packet;
}

void InitScrollTrack(int selection) {
    ScrollTrack40 *scroll = &MotionControllerView()->scroll;
    PsyqBzero(scroll, sizeof(*scroll));
    scroll->disabled = 0;
    scroll->keys = HudLinearGaugeVerts[selection];
}

/* Initial values of the globals this module owns. */
ScrollKey4 D_800A1C20[11] = {{.glyph = 1, .timing = 0x106}, {.glyph = 2, .timing = 0x300}, {.glyph = 3, .timing = 0x106}, {.glyph = 4, .timing = 0x106}, {.glyph = 5, .timing = 0x200}, {.glyph = 1, .timing = 0x106}, {.glyph = 6, .timing = 0x300}, {.glyph = 7, .timing = 0x106}, {.glyph = 5, .timing = 0x106}, {.glyph = 1, .timing = 0x200}, {.glyph = -1}};
ScrollKey4 D_800A1C4C[11] = {{.glyph = 1, .timing = 0x106}, {.glyph = 2, .timing = 0x300}, {.glyph = 3, .timing = 0x106}, {.glyph = 4, .timing = 0x106}, {.glyph = 5, .timing = 0x200}, {.glyph = 1, .timing = 0x106}, {.glyph = 6, .timing = 0x300}, {.glyph = 7, .timing = 0x106}, {.glyph = 5, .timing = 0x106}, {.glyph = 1, .timing = 0x200}, {.glyph = -1}};
ScrollKey4 D_800A1C78[11] = {{.glyph = 1, .timing = 0x106}, {.glyph = 2, .timing = 0x300}, {.glyph = 3, .timing = 0x106}, {.glyph = 4, .timing = 0x106}, {.glyph = 5, .timing = 0x200}, {.glyph = 1, .timing = 0x106}, {.glyph = 6, .timing = 0x300}, {.glyph = 7, .timing = 0x106}, {.glyph = 5, .timing = 0x106}, {.glyph = 1, .timing = 0x200}, {.glyph = -1}};
ScrollKey4 D_800A1CA4[11] = {{.glyph = 8, .timing = 0x106}, {.glyph = 24, .timing = 0x300}, {.glyph = 9, .timing = 0x106}, {.glyph = 24, .timing = 0x106}, {.glyph = 10, .timing = 0x200}, {.glyph = 24, .timing = 0x106}, {.glyph = 5, .timing = 0x300}, {.glyph = 24, .timing = 0x106}, {.glyph = 11, .timing = 0x106}, {.glyph = 24, .timing = 0x200}, {.glyph = -1}};
ScrollKey4 *HudLinearGaugeVerts[4] = {(ScrollKey4 *)D_800A1C20, (ScrollKey4 *)D_800A1C4C, (ScrollKey4 *)D_800A1C78, (ScrollKey4 *)D_800A1CA4};
