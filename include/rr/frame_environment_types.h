#ifndef RR_FRAME_ENVIRONMENT_TYPES_H
#define RR_FRAME_ENVIRONMENT_TYPES_H
#include "rr/gpu_environment_types.h"
/* The frame prefix owns distinct SDK drawing and display descriptors. */
typedef struct {
    DrawEnvironment draw00;
    DisplayEnvironment display5C;
} FrameEnvironment;
/* 704 ordering entries, each a host packet tag (OT_TYPE). */
typedef struct {
    GpuDmaTag tags[704];
} FrameOrderingTable;
/* Prefix shared by screen transitions and frame storage: the ordering
 * storage follows the environments (PS1 offset 0x70; the host environments
 * are larger, so the view repeats them instead of a fixed-size gap). */
typedef struct {
    FrameEnvironment environment00;
    FrameOrderingTable ordering70;
} FrameOrderingPrefix;
RR_ASSERT_SIZE(FrameOrderingTable, 704 * sizeof(GpuDmaTag));
RR_ASSERT_SIZE(FrameOrderingPrefix, RR_ALIGN_UP(sizeof(FrameEnvironment), _Alignof(FrameOrderingTable)) + sizeof(FrameOrderingTable));
RR_ASSERT_OFFSET(FrameOrderingPrefix, ordering70, RR_ALIGN_UP(sizeof(FrameEnvironment), _Alignof(FrameOrderingTable)));

RR_ASSERT_SIZE(FrameEnvironment, RR_ALIGN_UP(sizeof(DrawEnvironment) + sizeof(DisplayEnvironment), _Alignof(DrawEnvironment)));
RR_ASSERT_OFFSET(FrameEnvironment,draw00,0);
RR_ASSERT_OFFSET(FrameEnvironment, display5C, sizeof(DrawEnvironment));
#endif
