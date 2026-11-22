#include "types.h"
#include "rr/audio_state_types.h"
#include "game/sound/cdstart.h"
#include "game/sound/sndbgm3.h"
#include "game/sound/sndevt.h"
#include "game/sound/sndloop2.h"
#include "game/sound/sndown.h"
#include "game/sound/sndrst2.h"
#include "game/sound/sndslot.h"
#include "game/sound/sndvab.h"
#include "psyq/libcd/s_2.h"
#include "psyq/libsnd/ssmvol.h"
#include "psyq/libsnd/utrev3.h"
#include "psyq/libsnd/vmsel.h"
#include "psyq/libsnd/vmvinit.h"
#include "rr/library.h"
#include "rr/overlay665_audio_options.h"
#include "game/sound/sndctrl.h"
#include "rr/audio_bank_setup.h"

void SoundReset(int a0, int a1) {
    int a[3] = { 2, 3, 7 };
    int i;
    SsUtAllKeyOff(0);
    if (SoundState.seqHandle2864 >= 0) {
        SsSeqStop((short)SoundState.seqHandle2864);
        SsSeqClose((short)SoundState.seqHandle2864);
        SoundState.seqHandle2864 = -1;
    }
    for (i = 0; i < 3; i++) {
        if (SoundState.banks24.ids[a[i]] >= 0) {
            SsVabClose(SoundState.banks24.ids[a[i]]);
            SoundState.banks24.ids[a[i]] = -1;
        }
    }
    SoundMasterVolumeReset();
    ResetPlayerVoices();
    InitLoopVoices(a0);
    ResetSoundChannelVolLR();
    SoundQueueClear();
    SoundLoopReset();
    ReverbReset();
    if (a0 == 0) {
        SoundListenerInit(a1 & 0xFFFF, 0);
    } else {
        SoundListenerInit(a1 & 0xFFFF, 1);
    }
    EnableEngineSoundUpdates();
    SsUtAllKeyOff(0);
    SoundState.unk1A08 = 0;
    i = 6;
    do {
        SoundState.unk1A0C[i] = 1;
        i--;
    } while (i >= 0);
    SoundState.unk1A30 = 0;
    SoundState.unk1A2C = 0;
    SoundState.seqTrack2868 = -1;
}

void SoundReverbOffAll(void) {
    SsUtAllKeyOff(0);
    SsUtReverbOff();
    SsUtSetReverbType(0);
    SsUtSetReverbDepth(0, 0);
}

void SoundPause(void) {
    s32 slot;

    SoundState.muted04 = 1;
    CdAudioRequestPause();
    for (slot = 0; slot < 4; slot++) {
        SlotSetPause(slot, 1);
    }
}

void SoundResume(void) {
    s32 slot;

    SoundState.muted04 = 0;
    CdAudioRequestPause();
    for (slot = 0; slot < 4; slot++) {
        SlotSetPause(slot, 0);
    }
}

void SetCdMixMode(int a0) {
    u8 b[4];
    u8 *p;
    if (a0 == 0) {
        SsSetStereo();
        p = b;
        b[0] = 0x80;
        b[1] = 0;
        b[2] = 0x80;
        b[3] = 0;
    } else {
        SsSetMono();
        p = b;
        b[0] = 0x64;
        b[1] = 0x64;
        b[2] = 0x64;
        b[3] = 0x64;
    }
    CdMix(p);
}

void SoundStopAll(void) {
    int i;
    for (i = 0; i < 4; i++) {
        if (SoundState.slots38[i].flags00 & 1) {
            SlotKeyOffAll(i);
        }
    }
    for (i = 0; i < 2; i++) {
        SetPlayerVoice(0, 0, 0, i);
    }
    for (i = 0; i < 2; i++) {
        SetLoopVoiceVolumes(i, 0, 0);
    }
    SetSoundChannelVolLR(0, 0, 0);
    SoundLoopSet(5, 0x1E00, 0, 0x40);
    CdAudioMute();
    SoundReverbOffAll();
    DisableEngineSoundUpdates();
    SoundState.muted04 = 1;
}

void SoundMasterVolumeReset(void) {
    SoundState.gain18 = 0x80;
    SoundState.fadeLength1A = 0;
    SoundState.fadeTick1C = 0;
    SoundState.fadeTarget1E = 0;
    SoundState.fadeStart20 = 0;
    SoundState.fadeReserved22 = 0;
}

void SoundSetMasterVolume(short arg0, short arg1) {
    if (arg1 >= 2) {
        SoundState.fadeLength1A = arg1;
        SoundState.fadeTick1C = 0;
        SoundState.fadeTarget1E = arg0;
        SoundState.fadeStart20 = *(u16 *)&SoundState.gain18;
    } else {
        SoundState.gain18 = arg0;
        SoundState.fadeLength1A = 0;
        SoundState.fadeTick1C = 0;
        SoundState.fadeTarget1E = 0;
        SoundState.fadeStart20 = 0;
        if (arg0 == 0) {
            SoundStopAll();
        }
    }
    CdAudioFade(arg0 & 0xFFFF, arg1);
}
