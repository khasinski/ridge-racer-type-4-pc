#ifndef RR_COURSE_OCCLUSION_H
#define RR_COURSE_OCCLUSION_H

#include "types.h"
#include "rr/gpu.h"
#include "rr/rro_model.h"

/* Extended draw distance: opponent faces hidden behind the course.
 *
 * The course and the cars are ordered by ordering-table slot, a whole course
 * tile at the average depth of its corners plus a per-tile bias (which the
 * extended view's twice-as-deep slots double in distance). Inside the
 * original's car range that mostly works; past it (the extended view draws
 * opponents, their wheels and shadows to its end) a car sorted in front of
 * the walls, crests and tunnel roofs between it and the camera. While the
 * course of a view is submitted, each drawn tile's projection (screen
 * corners and depths, as the GTE gives them) is kept, unless its packets are
 * semi-transparent or its texels include the transparent colour; then the
 * tiles are rasterized in the GPU's order into a one-pixel buffer of the
 * depth of the course where it shows. An opponent face whose every pixel
 * (and the pixels around it) shows course nearer than the face's nearest
 * vertex is not drawn: its packets keep their links with no payload. When
 * other faces of the car show over course linked in front of them, the
 * whole car moves back in the table behind that course, in its own order,
 * if it stays in front of the course behind it (else only its shadow
 * moves). The main view submits its cars before its course, so they wait
 * for the view's course; the mirror's come after it. Only with
 * ExtendedDrawDistance, and not with the depth buffer (rr/depth_buffer.h),
 * which hides what lies behind the course per pixel. */

/* Frame start: forget the buffers and any faces still waiting. */
void CourseOcclusionFrame(void);

/* SubmitCourseHorizon: begin and end a view's course; End tests the faces
 * that waited for it. */
void CourseOcclusionCourseBegin(const RroSceneScratchPrefix *scene);
void CourseOcclusionCourseEnd(const u8 *cursor);

/* A course tile just projected (RTPT + RTPS: SZ0..SZ3 are its corners), its
 * ordering entry and the packet cursor: its packets follow. */
void CourseOcclusionTile(const s32 sxy[4], const GpuDmaTag *ot, const u8 *cursor);

/* A course was loaded: its textures are read again (transparent texels). */
void CourseOcclusionCourseLoaded(void);

/* DrawOpponentCar: faces submitted between Begin and End are tested. */
void CourseOcclusionCarBegin(const RroSceneScratchPrefix *scene);
void CourseOcclusionCarEnd(void);
/* The faces submitted next are the car's shadow (nonzero) or not. */
void CourseOcclusionCarShadow(int shadow);

/* A car face just emitted: the ordering entry it is linked in, its packets
 * (first..last, a run of the entry's list; second, when set, a second
 * drawing packet among them), screen points, and the depths the GTE holds in
 * SZ0..SZ3 (count 3 or 4 vertices). */
extern int CourseOcclusionCarActive;
void CourseOcclusionFace(GpuDmaTag *entry, GpuDmaTag *first, GpuDmaTag *last, GpuDmaTag *second,
                         const s32 *sxy, int count);

#endif
