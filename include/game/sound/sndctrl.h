#ifndef RR_GAME_SOUND_SNDCTRL_H
#define RR_GAME_SOUND_SNDCTRL_H

#include "types.h"

void SoundReset(int a0, int a1);
void SoundReverbOffAll(void);
void SoundPause(void);
void SoundResume(void);
void SoundStopAll(void);
void SoundMasterVolumeReset(void);
void SetCdMixMode(int a0);
void SoundSetMasterVolume(short arg0, short arg1);


#endif /* RR_GAME_SOUND_SNDCTRL_H */
