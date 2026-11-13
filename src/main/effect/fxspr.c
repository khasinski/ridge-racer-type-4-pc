#include "rr/cpu_time.h"
#include "game/gfx/fontdraw.h"
#include "scratchpad.h"
#include "rr/gpu_scratch_types.h"
#include "rr/frame_storage_types.h"
#include "game/effect/fxspr.h"
#include "game/gfx/fadespr.h"
#include "rr/frame_loop.h"
#include "rr/trig.h"
#include "psyq/libc/rand.h"
#include "rr/scene_camera_state.h"

extern int SparkParticleInitVel[];
extern const char HudPauseMenuItemCounts[13][16];
extern unsigned int OverlayTextSpriteUv;
void DrawIntroRippleSprites(void)
{
    RR_CPU_CYCLES(RR_CYCLES_DRAW_INTRO_RIPPLES);

  int l[2];
  short h[2];
  int i;
  int j;
  int x;
  int y;
  int ax;
  int ay;
  int w;
  void *p;
  GpuCommandPacket *mode;
  x = (RaceContext.fade3E0.f440 / 4) + (rsin(FrameCounter * 128U) / 128);
  l[0] = (x - 16) / 4;
  y = (RaceContext.fade3E0.f440 / 2) + (rcos(FrameCounter * 128U) / 128);
  l[1] = (y - 16) / 4;
  for (i = 0; i < 2; i++)
  {
    if (l[i] <= 0)
    {
      l[i] = 1;
    }
  }

  h[0] = (PsyqRand() % 16) - 8;
  h[1] = (PsyqRand() % 6) - 3;
  p = RR_GPU_SCRATCH->cursor;
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < 4; j++)
    {
      ax = (j / 2) + 0x9E;
      ay = ((j % 2) << 1) + 0xE4;
      p = AddTextSprites(&CurrentDrawBuf->buffer.ordering70.tags[10], p, ax - SparkParticleInitVel[RaceContext.fade3E0.f438], h[i] + ay, HudPauseMenuItemCounts[RaceContext.fade3E0.f438], 0x7F38, 1, (l[i] << 1) | 1);
    }

  }

  w = OverlayTextSpriteUv;
  RR_GPU_SCRATCH->cursor = p;
  mode = p;
  mode->tag.length = 1;
  mode->command = (w & 0x9FF) | 0xE1000200;
  RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[10], mode);
  RR_GPU_SCRATCH->cursor = (u8 *)(mode + 1);
}

/* Initial values of the globals this module owns. */
unsigned int OverlayTextSpriteUv = 0x3Eu;
const char HudPauseMenuItemCounts[13][16] = {{34, 87, 69, 76, 67, 79, 77, 69, 34}, {34, 68, 69, 77, 79, 78, 83, 84, 82, 65, 84, 73, 79, 78, 34}, {34, 71, 82, 65, 78, 68, 32, 80, 82, 73, 88, 34}, {34, 84, 73, 77, 69, 32, 65, 84, 84, 65, 67, 75, 34}, {34, 86, 83, 32, 66, 65, 84, 84, 76, 69, 34}, {34, 71, 65, 82, 65, 71, 69, 34}, {34, 82, 69, 67, 79, 82, 68, 83, 34}, {34, 83, 65, 86, 69, 32, 65, 78, 68, 32, 76, 79, 65, 68, 34}, {34, 79, 80, 84, 73, 79, 78, 83, 34}, {34, 69, 88, 84, 82, 65, 32, 84, 82, 73, 65, 76, 34}, {34, 76, 73, 78, 75, 32, 66, 65, 84, 84, 76, 69, 34}, {34, 69, 80, 73, 76, 79, 71, 85, 69, 34}, {34, 77, 85, 83, 73, 67, 32, 80, 76, 65, 89, 69, 82, 34}};
int SparkParticleInitVel[13] = {27, 45, 32, 37, 31, 22, 25, 45, 24, 37, 37, 26, 41};
