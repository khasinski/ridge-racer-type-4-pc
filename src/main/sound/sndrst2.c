#include "types.h"
#include "rr/audio_loop.h"
#include "game/sound/sndrst2.h"
#include "rr/channels.h"
#include "rr/audio_pan.h"

void SoundLoopSet(int a0, int a1, int a2, int a3);

void UpdateMasterSoundVolumes(void)
{
    unsigned short s1, s0;
    short t1, t2;
    int v0, v1, a0, a1, i;
    if (SoundState.split10 == 0) {
        s1 = SoundState.chVolL19BC[0];
        s0 = SoundState.chVolR19C0[0];
    } else {
        s1 = SoundState.chVolL19BC[0] + SoundState.chVolL19BC[1];
        s0 = SoundState.chVolR19C0[0] + SoundState.chVolR19C0[1];
        if ((short)s1 >= 0x80) s1 = 0x7F;
        if ((short)s0 >= 0x80) s0 = 0x7F;
    }
    if (Options.mono32 != 0) {
        v0 = ((short)s1 + (short)s0) / 2;
        s1 = v0;
        s0 = v0;
    }
    if (SoundState.muted04 != 0) {
        s0 = 0;
        s1 = 0;
    }
    v1 = (short)s0 * SoundState.gain18;
    if (v1 < 0) v1 += 0x7F;
    a0 = (short)s1 * SoundState.gain18;
    t1 = v1 >> 7;
    if (a0 < 0) a0 += 0x7F;
    v1 = t1 * Options.volume31;
    t2 = a0 >> 7;
    if (v1 < 0) v1 += 0xF;
    v0 = t2 * Options.volume31;
    s0 = v1 >> 4;
    if (v0 < 0) v0 += 0xF;
    s1 = v0 >> 4;
    if (GameModeType == 5) {
        s0 = 0;
        s1 = 0;
    }
    if ((short)s1 + (short)s0 > 0) {
        i = 0;
        do {
            SetVoiceVolumeScaled(i + 8, (short)s1, (short)s0);
            i++;
        } while (i < 2);
        if (SoundState.chActive19C4 == 0) {
            SsUtKeyOnV(8, SoundState.banks24.ids[0], SoundState.chProg19C8.halves.value, 0, 0x3C, 0, (short)s1, (short)s0);
            SsUtKeyOnV(9, SoundState.banks24.ids[0], SoundState.chProg19C8.halves.value, 1, 0x3C, 0, (short)s1, (short)s0);
            SoundState.chActive19C4 = 1;
        }
    } else if (SoundState.chActive19C4 != 0) {
        SsUtKeyOffV(8);
        SsUtKeyOffV(9);
        SoundState.chActive19C4 = 0;
    }
}

void SoundLoopReset(void) {
    SoundState.left19F8 = 0;
    SoundState.right19FA = 0;
    SoundState.pitch19FC = 0;
    SoundState.active1A00 = 0;
    SoundState.program1A04.word = 0;
}

void SoundLoopSet(int a0, int a1, int a2, int a3) {
    int x = PanGainTable[(s16)a3] * (s16)a2;
    int y = PanGainTable[(s16)(0x7F - a3)] * (s16)a2;
    SoundState.pitch19FC = a1;
    SoundState.left19F8 = y / 16129;
    SoundState.right19FA = x / 16129;
    if (a0 != SoundState.program1A04.word) {
        SoundState.program1A04.word = a0;
        SoundState.active1A00 = 0;
    }
    if (a2 == 0) {
        SsUtKeyOffV(6);
        SoundState.active1A00 = 0;
    }
}
