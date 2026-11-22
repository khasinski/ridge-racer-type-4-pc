#include "types.h"
#include "rr/audio_loop.h"
#include "game/sound/sndbgm3.h"
#include "rr/channels.h"
#include "game/gfx/color.h"

void SetLoopVoiceVolumes(int a0, int a1, int a2);
void SetSoundChannelVolLR(int a0, s16 a1, s16 a2);

void InitLoopVoices(int a0) {
    int k, i;
    if (a0 == 0) {
        k = 0xB;
        SoundState.loopVoices1994[0].enabled00 = 1;
        SoundState.loopVoices1994[1].enabled00 = 0;
    } else {
        k = 0xE;
        SoundState.loopVoices1994[0].enabled00 = 1;
        SoundState.loopVoices1994[1].enabled00 = 1;
    }
    for (i = 0; i < 2; i++) {
        SoundState.loopVoices1994[i].vol10[1] = 0;
        SoundState.loopVoices1994[i].vol10[0] = 0;
        SoundState.loopVoices1994[i].active04 = 0;
        SoundState.loopVoices1994[i].bank08.word = SoundState.banks24.ids[1];
        SoundState.loopVoices1994[i].voice0C.word = k + i;
    }
}

void SetLoopVoiceVolumes(int a0, int a1, int a2) {
    int i;
    if (Options.mono32 == 0) {
        SoundState.loopVoices1994[a0].vol10[0] = a1;
        SoundState.loopVoices1994[a0].vol10[1] = a2;
    } else {
        SoundState.loopVoices1994[a0].vol10[0] = (a1 + a2) / 2;
        SoundState.loopVoices1994[a0].vol10[1] = (a1 + a2) / 2;
    }
    for (i = 0; i < 2; i++) {
        if (SoundState.loopVoices1994[a0].vol10[i] < 2) SoundState.loopVoices1994[a0].vol10[i] = 0;
    }
}

void UpdateLoopVoices(void) {
    int i;
    int s0, s1;
    for (i = 0; i < 2; i++) {
        if (SoundState.loopVoices1994[i].enabled00 == 0) continue;
        s0 = SoundState.loopVoices1994[i].vol10[0];
        s1 = SoundState.loopVoices1994[i].vol10[1];
        if (SoundState.muted04 != 0) {
            s1 = 0;
            s0 = 0;
        }
        s1 = (s1 * SoundState.gain18) / 128;
        s0 = (s0 * SoundState.gain18) / 128;
        s1 = (s1 * Options.volume31) / 16;
        s0 = (s0 * Options.volume31) / 16;
        if (s0 + s1 > 0) {
            SetVoiceVolumeScaled(SoundState.loopVoices1994[i].voice0C.word, s0, s1);
            if (SoundState.loopVoices1994[i].active04 == 0) {
                SsUtKeyOnV(SoundState.loopVoices1994[i].voice0C.halves.value, SoundState.loopVoices1994[i].bank08.halves.value, 7, 0, 0x3C, 0, (short)s0, (short)s1);
                SoundState.loopVoices1994[i].active04 = 1;
            }
        } else if (SoundState.loopVoices1994[i].active04 != 0) {
            SsUtKeyOffV(SoundState.loopVoices1994[i].voice0C.halves.value);
            SoundState.loopVoices1994[i].active04 = 0;
        }
    }
}

void ResetSoundChannelVolLR(void) {
    int i;
    for (i = 0; i < 2; i++) {
        SoundState.chVolL19BC[i] = 0;
        SoundState.chVolR19C0[i] = 0;
    }
    SoundState.chActive19C4 = 0;
    SoundState.chProg19C8.word = 5;
}

void SetSoundChannelVolLR(int a0, s16 a1, s16 a2) {
    SoundState.chVolL19BC[a0] = a1;
    SoundState.chVolR19C0[a0] = a2;
}
