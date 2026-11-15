#ifndef RR_OVERLAY668_FRAME_ACQUIRE_H
#define RR_OVERLAY668_FRAME_ACQUIRE_H
#include "rr/overlay668_stream.h"
#include "rr/frame_storage_types.h"
#include "psyq/libgpu/gpuenv.h"
extern FrameStorage DrawBuffers[2];
/* Poll at most 2000 times. Return NULL when no frame is ready. A returned
 * frame stays owned until StFreeRing; *bytes is its validated compressed
 * extent, or zero for malformed dimensions/extent. Invalid metadata never
 * changes the display config. Valid dimensions fit the fixed slice buffers. */
u8 *ovl668_acquire_stream_frame(StrDisplayConfig *config, size_t *bytes);
#endif
