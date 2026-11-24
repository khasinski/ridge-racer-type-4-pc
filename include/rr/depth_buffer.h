#ifndef RR_DEPTH_BUFFER_H
#define RR_DEPTH_BUFFER_H

#include "rr/gpu.h"
#include "types.h"

/* Desktop preference, off by default (preference depth_buffer, environment
 * RR4_DEPTH_BUFFER). The 3D world of each view is depth-tested per pixel
 * instead of only by its ordering table (PsyZ Psyz_VideoSetDepthBuffer): cars
 * stay hidden behind walls and crests at any distance and large polygons no
 * longer sort through each other. Off, the packet stream is the original. */
extern int DepthBuffer;

/* Marks the start (begin != 0) or end of a depth-tested view at the head of
 * an ordering-table entry: a host GP0 command drawn in ordering-table order.
 * Returns the cursor after the packet (unchanged while the option is off).
 * SetupViewport marks each view after its drawing area; the frame marks the
 * end of both ordering tables. */
u8 *DepthViewMark(GpuDmaTag *entry, u8 *cursor, s32 begin);

#endif
