#ifndef RR_OVERLAY668_DISPLAY_H
#define RR_OVERLAY668_DISPLAY_H
#include "rr/gpu.h"
#include "rr/cd.h"

typedef struct {
    u32 word00, alternate04;
    u32 word08;
    volatile u32 ready0C;
    GpuRectangleFields rectangles[3];
} StrDisplayConfig;
#include "rr/scene_camera_state.h"
extern s32 DrawBufIndex;

/* Preserve the other configuration fields and store low16 coordinate bits. */
void ovl668_init_stream_display(StrDisplayConfig *config,s32 x,s32 y,s32 second_x,s32 second_y);
/* Base covers both command/slice buffers and 32 ring sectors (0x29A00 bytes).
 * The ring begins at offset 0x19A00. */
void ovl668_init_stream_buffers(u8 *base);

/* Consume readiness; on timeout toggle the buffer and select its coordinates. */
/* The playback caller supplies mode0. This implementation ignores the mode. */
void ovl668_wait_frame_ready(StrDisplayConfig *config,s32 mode);

RR_ASSERT_SIZE(StrDisplayConfig,0x28);
RR_ASSERT_OFFSET(StrDisplayConfig,ready0C,0x0C);
RR_ASSERT_OFFSET(StrDisplayConfig,rectangles,0x10);
#endif
