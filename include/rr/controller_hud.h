#ifndef RR_CONTROLLER_HUD_H
#define RR_CONTROLLER_HUD_H
#include "game/race/racegrid.h"
#include "game/system/pad.h"
#include "rr/sprite_emit.h"
#include "rr/badge_rows.h"
#include "rr/animated_sprite_grid.h"
#include "rr/tables.h"
#include "rr/record_banks.h"
#include "rr/frame_storage_types.h"
typedef union {s16 half[6];s32 word[3];} HudTriangle;
RR_ASSERT_SIZE(HudTriangle,12);
extern u32 ScreenFrame;
extern s16 HudLapFlashToggles[2];
extern u8 HudRotatingQuadVerts[6],HudIndicatorTriColor[3];
extern UiUvRect HudCheckeredFlagUv;
s32 GetControllerWarning(s32 player);
/* Layout1 selects port0 at Y112; layout2 selects port1 at Y240.
 * Other layouts select port0 at Y240. Status2 permits the warning and
 * optional steering needle. Button0x100 toggles that port's needle. */
void DrawSteeringIndicator(GpuDmaTag *ordering,s32 layout);
/* Layout0..2: layout0 uses the first frame ordering table; other layouts use the second.
 * Layout1 shifts the Y origin by128 pixels. Both full frame arenas and
 * count record-pointer slots must be accessible. Nonpositive counts skip
 * record markers; bit0x10 selects visible map records. Captures the frame
 * head, then reads fresh record fields/counts after each emitter callback.
 * CPU pointers remain full width; coordinates narrow only in GPU packets. */
void DrawRaceMinimap(s32 layout);
#endif
