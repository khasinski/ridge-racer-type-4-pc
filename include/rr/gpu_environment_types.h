#ifndef RR_GPU_ENVIRONMENT_TYPES_H
#define RR_GPU_ENVIRONMENT_TYPES_H

#include <stdint.h>
#include "rr/gpu.h"

typedef void (*GpuCompletionCallback)(void);

/* SDK DR_ENV as PsyZ (__psyz) lays it out: the 16-byte host packet tag and
 * fifteen u_long (pointer-width) command cells. PsyZ reads state packets as
 * u_long words, so SetDrawEnv/PutDrawEnv fill one cell per GPU command. */
typedef struct {
    GpuDmaTag tag;
    uintptr_t code[15];
} GpuDrawEnvironmentPacket;

/* SDK DRAWENV: drawing state and its embedded DR_ENV command backing. It does
 * not own a display descriptor. background enables a clear/fill command; it
 * does not select RGB24 output. Layout matches PsyZ DRAWENV (the packet starts
 * at 0x1C on 32-bit hosts and 0x20 on 64-bit hosts). */
typedef struct {
    GpuRectangleFields clip;
    s16 offset_x,offset_y;
    GpuRectangleFields window;
    u16 page;
    u8 dither,draw_display,background,r,g,b;
    GpuDrawEnvironmentPacket draw_packet1C;
} DrawEnvironment;

/* Submission compares all four trailing bytes as one cached word. mode is
 * the video-mode byte;8 marks a mode change before timing is rebuilt. */
typedef union {
    u32 word;
    struct {u8 interlace,rgb24,mode,reserved;} bytes;
} DisplayFlags;

/* SDK DISPENV is a separate twenty-byte descriptor (same on PS1 and PsyZ). */
typedef struct {
    GpuRectangleFields display,screen;
    DisplayFlags flags10;
} DisplayEnvironment;

/* Native PsyZ layout on both 32-bit and 64-bit hosts.
 * src/main/psyq/sdk_abi.c also compares the fields against PsyZ's DRAWENV. */
_Static_assert(sizeof(GpuDrawEnvironmentPacket) == 17 * sizeof(uintptr_t), "DR_ENV size");
_Static_assert(RR_FIELD_OFFSET(DrawEnvironment, background) == 0x18, "DRAWENV.isbg");
_Static_assert(RR_FIELD_OFFSET(DrawEnvironment, draw_packet1C) ==
                   ((0x1C + _Alignof(uintptr_t) - 1) & ~(_Alignof(uintptr_t) - 1)),
               "DRAWENV.dr_env");
_Static_assert(sizeof(DisplayEnvironment) == 0x14, "DISPENV size");

RR_ASSERT_OFFSET(DrawEnvironment,window,0xC);
RR_ASSERT_OFFSET(DrawEnvironment,offset_x,8);
RR_ASSERT_OFFSET(DrawEnvironment,page,0x14);
RR_ASSERT_OFFSET(DrawEnvironment,background,0x18);
RR_ASSERT_OFFSET(DrawEnvironment,r,0x19);
RR_ASSERT_OFFSET(DrawEnvironment, draw_packet1C, RR_ALIGN_UP(0x1C, _Alignof(GpuDrawEnvironmentPacket)));
RR_ASSERT_SIZE(DisplayEnvironment,0x14);
RR_ASSERT_OFFSET(DisplayEnvironment,screen,8);
RR_ASSERT_SIZE(DisplayFlags,4);
RR_ASSERT_OFFSET(DisplayFlags,bytes,0);
RR_ASSERT_OFFSET(DisplayEnvironment,flags10,0x10);

#endif
