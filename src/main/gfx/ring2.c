#include "rr/cpu_work.h"
#include "rr/cpu_time.h"
#include "rr/scene_camera_state.h"
#include "game/camera/viewcam.h"
#include "scratchpad.h"
#include "rr/presentation.h"
#include "game/gfx/meshquad.h"
#include "game/gfx/ring2.h"
#include "rr/ring_renderer.h"
#include "game/gfx/scene1.h"
#include "game/gfx/ring.h"
/* PsyZ libgte ScaleMatrix(MATRIX *, VECTOR *). */
extern MatrixWords32 *ScaleMatrix(MatrixWords32 *matrix, s32 *scale);

void DrawRingEffects(void) {
    if (!RrWorkFrame)
        RR_CPU_CYCLES(RR_CYCLES_RING_EFFECTS);
    int i;
    SceneCameraPose18 pose;
    s32 scale[3];
    MatrixWords32 light, rotation;
    ScrollTrack40 *scroll = &MotionControllerView()->scroll;
    for (i = 0; i < 2; i++) {
        if (scroll->channel[i].glyph > 0) {
            pose.position00.xyz.y = 0;
            pose.position00.xyz.x = 0;
            pose.position00.xyz.z = (s16)scroll->channel[i].offset;
            pose.rotation10.z = 0;
            pose.rotation10.y = 0;
            pose.rotation10.x = 0;
            SetupObjectLightMatrices(&pose, &MotionControllerView()->lighting20, &light, &rotation);
            scale[2] = 0x800;
            scale[1] = 0x800;
            scale[0] = 0x800;
            ScaleMatrix(&light, scale);
            if (scroll->channel[i].glyph < 0x18) {
                SetGteObjectTransform((ObjectTransform38 *)(SCRATCHPAD + 0x7C), (const s16 *)&pose.position00, light.w);
                DrawMeshPrimList(RaceContext.soundPayload564, scroll->channel[i].glyph + 1, 0);
            } else {
                DrawRing(pose.position00.words, 0x300);
            }
        }
    }
}
