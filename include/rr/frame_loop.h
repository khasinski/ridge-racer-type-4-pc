#ifndef RR_FRAME_LOOP_H
#define RR_FRAME_LOOP_H
#include "game/data/menuvars.h"
#include "rr/frame_environment.h"

#include "rr/frame_storage_types.h"
typedef void (*FrameStep)(void);
extern FrameStorage DrawBuffers[2];
extern FrameStep *ModeFuncTable[];
extern u32 FrameCounter;
extern s32 VSyncThreshold, DrawSubOtFlag, DrawBufIndex;

/* Initializes once, then runs indefinitely. Helpers may replace the current
 * context, frame counter, dispatch selectors and frame-wait threshold.
 * The threshold is captured once per frame after DrawSync. All later context
 * arguments are read afresh. Dispatch selectors require accessible table rows. */
void MainLoopStartupHook(void);
void MainLoop(void);
#endif
