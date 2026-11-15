#ifndef RR_FRAME_INTERP_H
#define RR_FRAME_INTERP_H
#include <stdint.h>
#include "types.h"
#include "rr/gpu.h"

struct GpuDmaTag;

/* Frame interpolation (Escape menu Graphics > Frame rate: Original/60/120).
 * A race frame still takes two original fields. PsyZ generates positions
 * between two frames for an independent output clock; MainLoop prepares
 * the completed pair through Psyz_GpuPrepareInterpolatedBlank. The
 * packets come from a linear arena, so their addresses do not pair across
 * frames: each 3D packet is tagged with a stable identity instead
 * (Psyz_GpuTagInterpolationPacket), made of
 *
 *   - the view (ActiveViewportIndex: race, mirror, split-screen half),
 *   - the object being drawn (InterpObject: a car and its part, a track
 *     object placement, the course, the scenery, an effect), set by the
 *     callers around their draws, and
 *   - a key from the renderer: the face or tile record in the loaded model
 *     data, the level of detail and the piece of a subdivided tile.
 *
 * PsyZ draws a tagged packet at the midpoint of its two poses (in view
 * space) when the previous frame drew the same identity with the same
 * texture coordinates; an identity the previous frame drew and this one
 * leaves out is drawn there as a ghost, and a new identity where ghosts go
 * is left out of the midpoint. A ghost and a newly visible tagged packet
 * move as the paired geometry next to them does; untagged HUD and 2D
 * packets keep their own pose. A camera cut (InterpCameraCut, and a jump of
 * the view's camera between frames) drops the pairs of that frame, and
 * PsyZ then shows the frame before for the midpoint's field, as it does for
 * a midpoint showing a hole or the background through a gap.
 *
 * With the option off InterpObject stays 0 and nothing is tagged. */

/* Object kinds of InterpObject. */
enum {
    INTERP_COURSE = 1,  /* course horizon tiles and road models */
    INTERP_SCENERY,     /* distant scenery (DrawTrackScenery) */
    INTERP_CAR,         /* a car: index << 8 | part */
    INTERP_CAR_FX,      /* a car's effects (shadow, lights, trails) */
    INTERP_OBJECT,      /* a track object placement */
    INTERP_EFFECT,      /* sparks, smoke and other particles */
    INTERP_SCENE        /* menu / intro 3D scenes */
};

/* The object being drawn, 0 while nothing interpolatable is drawn. */
extern u32 InterpObject;
/* One of several placements of the same model within the object (its
 * placement record), 0 for none. */
extern uintptr_t InterpInstance;
/* The option is in effect for the frame being built. */
extern s32 InterpActive;

/* Culling reaches this many pixels past the view's edges while an object
 * is drawn for interpolation: what leaves the view this frame still has its
 * pose here, so the midpoint has it where it was half a frame ago instead
 * of a gap. The drawing area clips it from the frame itself. */
#define INTERP_CULL_MARGIN 160
static inline s32 InterpMargin(void) { return InterpObject ? INTERP_CULL_MARGIN : 0; }

#define INTERP_ID(kind, index) ((u32)(kind) << 24 | ((u32)(index) & 0xFFFFFF))
/* Enter / leave an object's draws; InterpBegin returns the previous one. */
static inline u32 InterpBegin(u32 kind, u32 index) {
    u32 previous = InterpObject;
    if (InterpActive)
        InterpObject = INTERP_ID(kind, index);
    return previous;
}
static inline void InterpEnd(u32 previous) { InterpObject = previous; }
/* Switch to another object (inside an InterpBegin / InterpEnd). */
static inline void InterpSet(u32 kind, u32 index) {
    if (InterpActive)
        InterpObject = INTERP_ID(kind, index);
}

/* A car's part (wheel, body) or another piece of the current object. */
static inline void InterpPart(u32 part) {
    if (InterpObject)
        InterpObject = (InterpObject & ~0xFFu) | (part & 0xFF);
}

/* The shape pairs whatever its texture coordinates (an environment map that
 * follows the view; PsyZ PSYZ_INTERP_ANY_TEXTURE). */
#define INTERP_ANY_TEXTURE 1u
/* Drawn into the midpoint only (PSYZ_INTERP_MIDPOINT_ONLY). */
#define INTERP_MIDPOINT_ONLY 2u
/* Left out of the midpoint (PSYZ_INTERP_FRAME_ONLY). */
#define INTERP_FRAME_ONLY 4u
/* Its coordinates this frame are no projection (PSYZ_INTERP_NO_POSE). */
#define INTERP_NO_POSE 8u
/* Tag the shape whose GP0 command word is at shape with key and a piece
 * number for the current object (INTERP_ANY_TEXTURE, INTERP_MIDPOINT_ONLY, INTERP_FRAME_ONLY). */
void InterpTagShape(const void *shape, uintptr_t key, u32 piece, unsigned flags);
#define INTERP_TAG_SHAPE(shape, key, piece, flags)                                \
    do {                                                                          \
        if (InterpObject)                                                         \
            InterpTagShape((shape), (uintptr_t)(key), (u32)(piece), (flags));     \
    } while (0)
/* A packet whose payload is the shape (it starts with its GpuDmaTag). */
#define INTERP_TAG(packet, key, piece)                                            \
    INTERP_TAG_SHAPE((const GpuDmaTag *)(const void *)(packet) + 1, key, piece, 0)

/* The generation of identities of the view being drawn (ActiveViewportIndex):
 * it changes with every cut of that view. */
u32 InterpViewGeneration(void);
/* The camera cut to another shot this frame: nothing pairs with the last. */
void InterpCameraCut(void);
/* A view's camera (world position, 12-bit angles) for this frame: a jump
 * from its last frame, or a view not drawn last frame, counts as a cut. */
void InterpViewCamera(s32 view, const s32 position[3], const s32 angles[3]);

/* MainLoop: the frame starts / its ordering tables are about to be drawn
 * into clip, after the frame drawn at previousY. */
void InterpBeginFrame(void);
void InterpPollOutput(void);
void InterpCommitFrame(s32 previousY, const GpuRectangleFields *clip);

#endif
