#ifndef RR_GAME_CAMERA_VIEWCAM_H
#define RR_GAME_CAMERA_VIEWCAM_H

#include "types.h"
#include "rr/gpu.h"
#include "rr/object_transform_types.h"
#include "rr/geometry.h"

/* Position supplies the signed low half of each of three word components.
 * Rotation supplies the eight-word libgte matrix storage. */
void SetGteObjectTransform(ObjectTransform38 *output, const s16 *position,
                           s32 *rotation);
/* The same for a full-word position at any distance from the camera. */
void SetGteObjectTransformWide(ObjectTransform38 *output, const Vector *position,
                               s32 *rotation);

void BuildLightMatrix(void);
void SetupViewLighting(void);
void SetupPlayerView(int player);
void RenderMirrorCourseView(int player);
/* Two draw-buffer banks, each with seven viewport rectangles. */
extern GpuRectangleFields CourseSelectPreviewUvs[2][7];
void SetupViewport(s32 viewport);
int GetViewTrackNode(const s32 *position, s32 player);
void SetupHorizonViewTransform(int *a0, int *a1);
void SetupViewMatrix(void);
void RenderPlayerCourseView(int player);


#endif /* RR_GAME_CAMERA_VIEWCAM_H */
