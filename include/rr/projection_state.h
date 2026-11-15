#ifndef RR_PROJECTION_STATE_H
#define RR_PROJECTION_STATE_H
#include "rr/animated_sprite_grid.h"
#include "rr/geometry.h"
#include "rr/matrix_words.h"
#include "scratchpad.h"
/* Camera position and full-word Euler angles are one contiguous input view.
 * The fourth word of each vector belongs to the SDK storage. */
typedef struct {
    VectorWords16 position00;
    VectorWords16 angles10;
} ProjectionView32;
/* The first eight PS1 scratch bytes held the packet cursor and ordering
 * table; on the host they are RR_GPU_SCRATCH and this is a placeholder. */
typedef struct {
    u8 gpu_slot00[8];
    ProjectionView32 pose08;
} ProjectionScratchPrefix;
/* Every projection view shares the native scratchpad, independent of which
 * overlay header the caller includes first. */
#define RR_PROJECTION_SCRATCH ((ProjectionScratchPrefix *)(void *)RrScratchpad)
typedef struct {
    ProjectionScratchPrefix projection;
    MatrixWords32 matrix28;
} ViewMatrixScratch48;
RR_ASSERT_SIZE(ProjectionView32,0x20);
RR_ASSERT_OFFSET(ProjectionView32,angles10,0x10);
/* Scalar-only scratch views: PS1 offsets hold on every host. */
_Static_assert(sizeof(ProjectionScratchPrefix) == 0x28, "projection scratch");
_Static_assert(RR_FIELD_OFFSET(ProjectionScratchPrefix, pose08) == 8, "pose08");
_Static_assert(sizeof(ViewMatrixScratch48) == 0x48, "view scratch");
_Static_assert(RR_FIELD_OFFSET(ViewMatrixScratch48, matrix28) == 0x28, "matrix28");
#endif
