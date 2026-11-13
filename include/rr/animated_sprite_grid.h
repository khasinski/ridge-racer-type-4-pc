#ifndef RR_ANIMATED_SPRITE_GRID_H
#define RR_ANIMATED_SPRITE_GRID_H
#include "rr/gpu.h"

#include "rr/gpu_scratch_types.h"
typedef struct {u8 pad00[14];u16 tick0E;} SpriteGridClock;
typedef struct {GpuDmaTag tag;u32 command;} GpuSingleCommandPacket;

/* Requires aligned pointer slots, a 4-byte-aligned cursor with space for 15
 * sprite packets and one 8-byte command, and accessible heads after each order
 * pointer reload. Adds one modulo 16-bit tick, then advances phase by 5 for each
 * sprite. Raw intensity bits are OR-packed without clamping. Each list insertion
 * clears the ordering-head high byte. Finishes with command E100022A. */
void EmitSpriteGrid(u32 intensity,SpriteGridClock *state);
/* EmitSpriteGrid with the clock at tick (the Japanese release's frame task
 * keeps it at +0x2C and draws the grid 4 pixels further left). */
void EmitSpriteGridTick(u32 intensity,u16 *tick);

RR_ASSERT_SIZE(SpriteGridClock, 16);
RR_ASSERT_OFFSET(SpriteGridClock, tick0E, 14);
RR_ASSERT_SIZE(GpuSingleCommandPacket, 8);
#endif
