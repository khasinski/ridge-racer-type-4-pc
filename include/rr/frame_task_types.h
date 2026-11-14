#ifndef RR_FRAME_TASK_TYPES_H
#define RR_FRAME_TASK_TYPES_H
#include "rr/animated_sprite_grid.h"
typedef struct {
    s16 frame;
    s16 intensity[3];
    s16 ramp[3];
    u16 spriteTick;
} FrameTaskFields;
typedef union {
    FrameTaskFields fields;
    SpriteGridClock grid;
    s16 words[8];
} FrameTaskState;
RR_ASSERT_SIZE(FrameTaskFields, 16);
RR_ASSERT_SIZE(FrameTaskState, 16);
RR_ASSERT_OFFSET(FrameTaskFields, intensity, 2);
RR_ASSERT_OFFSET(FrameTaskFields, ramp, 8);
RR_ASSERT_OFFSET(FrameTaskFields, spriteTick, 14);

/* The Japanese release's frame task (0x2E bytes): eight preset sprites
 * fading in one after another, the sprite grid and a fade tile. */
typedef struct {
    s16 frame;
    s16 intensity[9];
    s16 ramp[9];
    s16 fade26, fade28, fade2A;
    u16 spriteTick;
} FrameTaskFieldsJp;
#endif
