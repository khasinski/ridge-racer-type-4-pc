#include "rr/cpu_time.h"
#include "rr/scene_camera_state.h"
#include "types.h"
#include "rr/audio_loop.h"
#include "game/sound/fxramp1.h"
#include "game/sound/sndbgm3.h"
#include "game/sound/sndchan.h"
#include "game/sound/sndcmd.h"
#include "game/sound/sndctrl.h"
#include "game/sound/sndmain.h"
#include "game/sound/sndrst2.h"
#include "game/sound/sndvol2.h"
#include "game/data/trkld6.h"
#include "game/sound/sndslot.h"
#include "rr/memory.h"
#include "rr/menu_overlay_runtime.h"
#include "rr/audio_cd_controller.h"

void SoundMasterFadeStep(void) {
    AudioState *p = &SoundState;
    int n = p->fadeLength1A;
    int v = 0x80;
    int c;
    short c1;
    if (n > 0) {
        c = (unsigned short)p->fadeTick1C;
        if (p->fadeTick1C < n) {
            n = ((p->fadeTarget1E - p->fadeStart20) << 12) / n;
            c1 = c + 1;
            p->fadeTick1C = c1;
            v = (unsigned short)p->fadeStart20 + n * c1 / 4096;
        } else {
            p->fadeLength1A = 0;
            SoundStopAll(n, p, c);
            return;
        }
    }
    p->gain18 = v;
}

void SoundUpdate(void) {
    RR_CPU_CYCLES(RR_CYCLES_SOUND_UPDATE);

    int slotIndex;
    AudioSlot620 *slot;
    u32 beatPhase;
    if (SoundState.seqPrevBeat2878 != SoundState.seqBeat2874) {
        beatPhase = SoundState.seqBeat2874 - 1;
        SoundState.seqTick286C = 1;
        if ((beatPhase & 1) == 0) SoundState.seqHalf286E = 1;
        if ((beatPhase & 3) == 0) SoundState.seqQuarter2870 = 1;
    } else {
        SoundState.seqTick286C = 0;
        SoundState.seqHalf286E = 0;
        SoundState.seqQuarter2870 = 0;
    }
    slotIndex = 0;
    SoundState.seqPrevBeat2878 = SoundState.seqBeat2874;
    CdAudioUpdate();
    SeqFadeStep();
    SoundMasterFadeStep();
    SoundListenerUpdate();
    for (slot = SoundState.slots38; slotIndex < 4; slotIndex++, slot++) {
        if (slot->flags00 & 1) {
            UpdateSoundSlot(slotIndex);
        }
    }
    UpdatePlayerVoices();
    UpdateLoopVoices();
    UpdateMasterSoundVolumes();
    SoundQueueUpdate();
    SoundLoopUpdate();
}

void SetVoiceVolumeScaled(s32 voice, s32 left, s32 right) {
    /* Hardware volume products wrap at 32 bits before the logical shift. */
    u32 scale = SoundState.master08;
    u32 leftProduct = left * scale;
    u32 rightProduct = right * scale;
    SsUtSetVVol((s16)voice, (s16)(leftProduct >> 7), (s16)(rightProduct >> 7));
}

void SlotsStartForRace(void) {
    SlotStart(0);
    if (SoundState.split10 == 0) {
        if (RaceContext.sound.comBank >= 0) {
            SlotStart(2);
            SlotStart(3);
        }
    } else {
        SlotStart(1);
    }
}

void SlotStart(int index) {
    SoundState.slots38[index].flags00 |= 1;
    SlotKeyOn(index);
    SoundState.slots38[index].previous18 = SoundState.slots38[index].program14;
    SlotSetPause(index, 0);
}

void SlotKeyOffAll(int slotIndex) {
    int channel;
    int voice;
    AudioSlot620 *slot = &SoundState.slots38[slotIndex];
    voice = slot->voice08;
    channel = 0;
    while (channel < slot->count0C) {
        SsUtKeyOffV((short)voice);
        channel++;
        voice++;
    }
    slot->flags00 = 0;
}

void SlotSetPause(int slotIndex, int paused) {
    u32 *flags = &SoundState.slots38[slotIndex].flags00;
    if (paused == 1) {
        *flags |= 4;
    } else {
        *flags &= ~4;
    }
}

void SlotSetLevelProgram(int a0, int a1, int a2) {
    SoundState.slots38[a2].level10 = a0;
    SoundState.slots38[a2].program14 = a1;
}

void SlotKeyOn(int slotIndex) {
    AudioUnsignedWord4 groups[4];
    AudioSlot620 *slot = &SoundState.slots38[slotIndex];
    AudioUnsignedWord4 *group;
    int channel;
    int voice;
    int noteGroup;
    PsyqMemset(groups, 0, sizeof(groups));
    groups[3].word = 2;
    voice = slot->voice08;
    channel = 0;
    if (slot->count0C > 0) {
        group = &groups[slotIndex];
        do {
            SsUtKeyOnV((short)voice, slot->bank04.half.value, (s16)slot->programData3C.programs[slot->program14][channel], 0, (short)(group->halves.value + 0x3C), 0, 0, 0);
            voice++;
            channel++;
        } while (channel < slot->count0C);
    }
    noteGroup = groups[slotIndex].halves.value;
    slot->reserved2A = 0;
    slot->group28 = noteGroup + 0x3C;
}

