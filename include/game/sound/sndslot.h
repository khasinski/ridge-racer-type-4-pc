#ifndef RR_GAME_SOUND_SNDSLOT_H
#define RR_GAME_SOUND_SNDSLOT_H

#include "types.h"

void SoundUpdate(void);
/* The field between two ticks of a field-rate race (rr/field_rate.h). */
void SoundUpdateField(void);
void SlotKeyOffAll(int arg0);
void SlotSetPause(int a0, int a1);
void SoundMasterFadeStep(void);
void SetVoiceVolumeScaled(s32 voice, s32 left, s32 right);
void SlotsStartForRace(void);
void SlotStart(int a0);
void SlotSetLevelProgram(int a0, int a1, int a2);
void SlotKeyOn(int a0);


#endif /* RR_GAME_SOUND_SNDSLOT_H */
