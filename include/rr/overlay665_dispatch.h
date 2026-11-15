#ifndef RR_OVERLAY665_DISPATCH_H
#define RR_OVERLAY665_DISPATCH_H
#include "rr/overlay665_calibration.h"
#include "rr/menu_transition.h"
#include "game/camera/campath.h"
#include "game/camera/camrun.h"
void StartAttractDemo(int a0);
void ovl665_update_controller_options(s32);
void ovl665_update_audio_options(void);
/* Dispatch captures the scene and selection at their respective switches.
 * Scene0 increments the fresh value left by initialization, modulo32 bits. */
void ovl665_update_options(void);
#endif
