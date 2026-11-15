#ifndef RR_OVERLAY668_CAMERA_H
#define RR_OVERLAY668_CAMERA_H
#include "game/gfx/scene1.h"
#include "rr/runtime_settings.h"
#include "scratchpad.h"
#include "game/data/cdfile2.h"
#include "rr/track_sample_types.h"
#include "rr/track_lateral.h"
#include "rr/race_types.h"
#include "rr/overlay665_matrix.h"
#include "psyq/libgte/gtecord.h"
#include "psyq/libgte/gtemat5.h"
#include "rr/scene_camera_state.h"
extern s32 TrackNodeCount;
int Atan2Table(int a0, int a1);
/* Node and its single-wrapped neighbours select accessible track samples.
 * Position and interpolation results use wrapping32 arithmetic. Fraction is
 *0..1023; sample count is at least4. Services may replace the sample table and
 * camera state. The angle service returns an angle in-1024..3072. */
void ovl668_step_epilogue_camera_yaw(void);
void ovl668_step_epilogue_camera_pitch(void);
#include "rr/player_cars.h"
#include "psyq/libgte/rotmat.h"
void BuildCameraMatrices(void);
void ovl668_apply_epilogue_camera(void);
/* Explicit legacy position argument, ignored by the current body. */
void ovl668_project_camera_to_track(s32 *position);
/* The track root after setup services covers at least321 samples. Services
 * may change roots and state; the final wrapped selection index must address
 * one of the20 lookup entries. Initializes from captured sample320, advances
 * the camera sixteen times, then publishes the post-service lookup value. */
void ovl668_init_epilogue_credits(void);
extern s32 VSyncThreshold,ViewUsesCarTrackNode,MaskBitSetting,EpilogueCreditScrollY,EpilogueCreditScrollStep;
extern s16 SelectedTeamIndex;
extern s32 ovl668_credit_scroll_speeds[20];
void SetDispMask(s32);
void SetupDrawDispEnvs(s32,s32,s16,u32);
void InitCourseViewState(void);

#ifndef RR_REPLAY_SCRATCH_WORDS
#define RR_REPLAY_SCRATCH_WORDS ((s32 *)SCRATCHPAD)
#endif
#endif
