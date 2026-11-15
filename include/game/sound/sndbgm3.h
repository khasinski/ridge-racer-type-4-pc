#ifndef RR_GAME_SOUND_SNDBGM3_H
#define RR_GAME_SOUND_SNDBGM3_H

#include "types.h"

void InitLoopVoices(int a0);
void SetLoopVoiceVolumes(int a0, int a1, int a2);
void UpdateLoopVoices(void);
void ResetSoundChannelVolLR(void);
void SetSoundChannelVolLR(int a0, s16 a1, s16 a2);

#endif /* RR_GAME_SOUND_SNDBGM3_H */
