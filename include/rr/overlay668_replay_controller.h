#ifndef RR_OVERLAY668_REPLAY_CONTROLLER_H
#define RR_OVERLAY668_REPLAY_CONTROLLER_H
#include "game/data/menuvars.h"
#include "game/system/pad.h"
#include "rr/frame_storage_types.h"
#include "rr/overlay668_camera.h"
#include "rr/icon_sprites.h"
#include "rr/tables.h"
typedef struct {s32 key,action;} ReplayEvent;
extern ReplayEvent ovl668_epilogue_events[3];

#include "rr/screen_tiles.h"
void CdAudioBeginPlay(s32);
void CdAudioFade(u16,s32);
void SetupPlayerView(s32);
void DrawTrackSceneryForPlayer(s32);
void ovl668_emit_letterbox_pass(s32);void ovl668_draw_credit_band(s32,s32);
/* The three-entry event table contains a negative sentinel within its owner.
 * The camera owner covers byte0..4B7, including the low scene flag atB0 and
 * event counter at4B4. Services can mutate all camera words, input, text words
 * and the projection root; each fresh root selects a complete context.
 * Counter additions, fade steps and text subtraction wrap32; division by6
 * truncates toward zero. Each active text phase submits exactly62 entries. */
void ovl668_update_epilogue_credits(void);
#endif
