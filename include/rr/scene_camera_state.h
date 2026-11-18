#ifndef RR_SCENE_CAMERA_STATE_H
#define RR_SCENE_CAMERA_STATE_H

#include "rr/scene_camera_types.h"
#include "rr/ps1_low_ram.h"

extern SceneRaceContext RaceContext;

/* PS1 MotionController: the presentation controller once
 * InitMotionController has bound it, and before that the null pointer's
 * view of low RAM (rr/ps1_low_ram.h), as the original's accesses were. */
static inline PresentationController *MotionControllerView(void) {
    return RaceContext.controllerSlot3D8 != 0 ? &RaceContext.presentation250 : &Ps1LowRamImage.presentation;
}

#endif
