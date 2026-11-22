#include "psyq/libc/exit.h"
#include "types.h"
#include "rr/audio_state_types.h"
#include "rr/audio_pan.h"
#include "psyq/libsnd/utrev3.h"
#include "game/sound/sndvab.h"
#include "rr/library.h"
#include "rr/menu_choice.h"
#include "rr/audio_bank_setup.h"
#include "rr/audio_cd_controller.h"

extern char VabOpenHeadErrorStr[];
extern char VabTransBodyErrorStr[];

void LoadVab(const VabData *data, s32 mode, s32 bankIndex) {
    s32 opened;
    s32 transferred;
    if (SoundState.banks24.ids[bankIndex] >= 0) {
        SsVabClose(SoundState.banks24.ids[bankIndex]);
        SoundState.banks24.ids[bankIndex] = -1;
    }
    opened = SsVabOpenHeadSticky(data->header, -1, mode);
    SoundState.banks24.ids[bankIndex] = opened;
    if ((s16)opened == -1) {
        DebugPrintStub(VabOpenHeadErrorStr);
        PsyqExit(1);
    }
    transferred = SsVabTransBody(data->samples, (s16)opened);
    SoundState.banks24.ids[bankIndex] = transferred;
    if ((s16)transferred == -1) {
        DebugPrintStub(VabTransBodyErrorStr);
        PsyqExit(1);
    }
    SsVabTransCompleted(1);
}

void ReverbReset(void) {
    SoundState.revType2894 = 0;
    SoundState.revDepthL2898 = 0;
    SoundState.revDepthR289C = 0;
    SoundState.revMask28A0 = 0xFFFFFF;
    SsUtReverbOff();
    SsUtSetReverbType(2);
    SsUtReverbOn();
    SsUtSetReverbDepth(0, 0);
}

void SetReverbDepthBoth(s32 arg0) {
    SoundState.revDepthL2898 = arg0;
    SsUtSetReverbDepth((s16) arg0, (s16) arg0);
}

void SetReverb(int a0, int a1) {
    SsUtReverbOff();
    if (a0 > 0) {
        SoundState.revType2894 = a0;
        SoundState.revDepthL2898 = a1;
        SoundState.revDepthR289C = a1;
        SsUtSetReverbType((s16)a0);
        SsUtReverbOn();
    } else {
        SoundState.revType2894 = 0;
        SoundState.revDepthL2898 = 0;
        SoundState.revDepthR289C = 0;
    }
}

void TriggerSfx(s32 programId, s32 level, s32 pan, s32 pitch, s32 voiceIndex) {
    s16 rightPanned;
    s16 automaticFine;
    s16 fixedFine;
    s16 leftPanned;
    s16 rightLevel;
    s16 leftLevel;
    s32 bankProgram;
    s32 panIndex;
    s32 rightVolume;
    s32 leftVolume;
    s32 pitchOffset;
    s32 monoLevel;
    s32 leftGainProduct;
    s32 rightVolumeProduct;
    s32 leftVolumeProduct;
    s32 rightScaled, leftScaled;
    s32 volume;
    s16 *table;
    s16 note;
    s32 rightGainProduct;
    u16 bankId;
    bankProgram = programId;
    pitchOffset = (pitch + 0x3CU);
    if (bankProgram < 0x1E) {
        bankId = SoundState.banks24.ids[0];
    } else if (bankProgram < 0x3C) {
        bankId = SoundState.banks24.ids[1];
        bankProgram -= 0x1E;
    } else {
        bankId = SoundState.banks24.ids[7];
        bankProgram -= 0x3C;
    }
    table = PanGainTable;

    panIndex = pan & 0x7F;
    rightPanned = (table[panIndex] * (s16)level) / 16129;
    rightLevel = rightPanned;
    leftPanned = (table[0x7F-panIndex] * (s16)level) / 16129;
    leftLevel = leftPanned;
    if (Options.mono32 != 0) {
        monoLevel = (leftPanned + rightPanned) / 2;
        rightLevel = (s16) monoLevel;
        leftLevel = (s16) monoLevel;
    }
    rightGainProduct = rightLevel * SoundState.gain18;
    if (rightGainProduct < 0) {
        rightGainProduct += 0x7F;
    }

    leftGainProduct = leftLevel * SoundState.gain18;
    rightScaled = rightGainProduct >> 7;
    if (leftGainProduct < 0) {
        leftGainProduct += 0x7F;
    }

    rightScaled = (s16)rightScaled;

    volume = Options.volume31;
    rightVolumeProduct = rightScaled * volume;
    leftScaled = leftGainProduct >> 7;
    if (rightVolumeProduct < 0) {
        rightVolumeProduct += 0xF;
    }

    leftVolumeProduct = (s16)leftScaled * volume;
    rightVolume = rightVolumeProduct >> 4;
    leftVolume = leftVolumeProduct / 16;
    if (voiceIndex == -1) {
        s16 bank = bankId;
        s16 program = bankProgram;
        SsUtKeyOn(bank, program, 0, note = pitchOffset / 128, automaticFine = pitchOffset % 128, (s16) leftVolume, (s16) rightVolume);
        SsUtKeyOn(bank, program, 1, note, automaticFine, (s16) leftVolume, (s16) rightVolume);
        return;
    }
    {
        u32 voice = voiceIndex * 2U;
        s16 bank = bankId;
        s16 program = bankProgram;
        SsUtKeyOnV((s16)voice, bank, program, 0, note = pitchOffset / 128, fixedFine = pitchOffset % 128, (s16) leftVolume, (s16) rightVolume);
        SsUtKeyOnV((s16)(voice + 1U), bank, program, 1, note, fixedFine, (s16) leftVolume, (s16) rightVolume);
    }
}

void PlaySfxVolPan(int a0, s16 a1, s16 a2, int a3) {
    TriggerSfx(a0, a1, a2, 0x1E00, a3);
}

void PlaySfx(int a0) {
    PlaySfxVolPan(a0, 0x7F, 0x40, -1);
}

void PlaySfxBank2(s32 arg0) {
    PlaySfxVolPan(arg0 + 0x3C, 0x7F, 0x40, -1);
}

void SoundQueueClear(void) {
    s16 m = -1;
    int i = RR_SOUND_SEQUENCE_CAPACITY - 1;
    SoundState.sequence19D0.active = 0;
    SoundState.sequence19D0.readIndex = 0;
    SoundState.sequence19D0.unknown0C = 0;
    do { SoundState.sequence19D0.programs[i] = m; i--; } while (i >= 0);
}

/* Initial values of the globals this module owns. */
char VabOpenHeadErrorStr[24] = {83, 115, 86, 97, 98, 79, 112, 101, 110, 72, 101, 97, 100, 32, 69, 114, 114, 111, 114, 33, 10};
char VabTransBodyErrorStr[52] = {83, 115, 86, 97, 98, 84, 114, 97, 110, 115, 66, 111, 100, 121, 32, 69, 114, 114, 111, 114, 10};
