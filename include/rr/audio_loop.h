#ifndef RR_AUDIO_LOOP_H
#define RR_AUDIO_LOOP_H

#include "game/data/menuvars.h"
#include "rr/audio_state_types.h"
#include "rr/channel_pitch.h"
#include "rr/runtime_settings.h"

extern AudioState SoundState;

s32 SsUtSetVVol(s32 voice,s32 left,s32 right);
/* Voice uses signed low16. Volume products wrap in32 bits before arithmetic
 * shifting and narrowing to signed16, including full-width master values. */
void SetVoiceVolumeScaled(s32 voice,s32 left,s32 right);
s32 SsUtKeyOnV(s32 voice,s32 bank,s32 program,s32 mode,
                 s32 group,s32 unused,s32 left,s32 right);
/* Captures scaled levels and pitch, then updates channels6/7. Bank/program and
 * active state are re-read after helpers; their return values are ignored. */
void SoundLoopUpdate(void);


#endif
