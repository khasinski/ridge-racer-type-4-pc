#include <string.h>
#include "types.h"
#include "rr/audio_bank_setup.h"
#include "rr/runtime_settings.h"
#include "game/sound/sndchan.h"
#include "rr/channel_pitch.h"
#include "rr/audio_loop.h"
#include "rr/audio_pan.h"

extern u16 MenuCarTextureFileOffsets[];

void UpdateSoundSlot(int slotIndex) {
    AudioSlot620 *slot = &SoundState.slots38[slotIndex];
    u16 spread = 0;
    int voice, programIndex;
    u16 pitchOffset;
    int activeLevel;
    int curvePosition;
    int filterIndex;

    if (slot->program14 != slot->previous18) {
        voice = slot->voice08;
        for (programIndex = 0; programIndex < slot->count0C; programIndex++, voice++) {
            if (slot->programData3C.programs[0][programIndex] != slot->programData3C.programs[1][programIndex]) {
                SsUtKeyOnV((s16)voice, slot->bank04.half.value, (s16)slot->programData3C.programs[slot->program14][programIndex], 0, slot->group28, 0, 0, 0);
            }
        }
        slot->previous18 = slot->program14;
    }

    if ((slot->flags00 & 4) == 0) {
        activeLevel = slot->levels20.halves.level;
    } else {
        activeLevel = 0;
    }

    curvePosition = (slot->level10 * 0x2800) / slot->programData3C.duration;
    voice = slot->voice08;
    if (curvePosition > 0x2800) {
        curvePosition = 0x2800;
    }

    filterIndex = 0x11A800 / (0x2350 - slot->filter30);
    filterIndex -= 0x4D;
    if (filterIndex < 0) {
        filterIndex = 0;
    }
    if (filterIndex >= 0x120) {
        filterIndex = 0x11F;
    }
    pitchOffset = MenuCarTextureFileOffsets[filterIndex];

    for (programIndex = 0; programIndex < slot->count0C; programIndex++, voice++) {
        int pitchCurve = SlotInterpCurve(programIndex * 2, curvePosition, slot->program14, slotIndex);
        int leftVolume = SlotInterpCurve(programIndex * 2 + 1, curvePosition, slot->program14, slotIndex) * activeLevel;
        int rightVolume;
        if (leftVolume < 0) {
            leftVolume += 0x7F;
        }
        leftVolume >>= 7;
        rightVolume = (PanGainTable[slot->pan24] * leftVolume) / 16129;
        leftVolume = (PanGainTable[(s16)(0x7F - slot->pan24)] * leftVolume) / 16129;

        if (slotIndex >= 2 && programIndex == 2) {
            int spatialGain = PanGainTable[slot->spatial2C.halves.value];
            spread = slot->programData3C.spread;
            rightVolume = (spatialGain * rightVolume) / 16129;
            leftVolume = (spatialGain * leftVolume) / 16129;
            rightVolume = -rightVolume;
            leftVolume = -leftVolume;
        }

        rightVolume = (rightVolume * slot->levels20.halves.level * slot->levels20.halves.gain) / 16384;
        leftVolume = (leftVolume * slot->levels20.halves.level * slot->levels20.halves.gain) / 16384;
        rightVolume = (rightVolume * slot->programData3C.gain) / 128;
        leftVolume = (leftVolume * slot->programData3C.gain) / 128;

        if (Options.mono32 != 0) {
            rightVolume = (rightVolume + leftVolume) / 2;
            leftVolume = rightVolume;
        }

        leftVolume = (leftVolume * SoundState.gain18) / 128;
        rightVolume = (rightVolume * SoundState.gain18) / 128;
        rightVolume = (rightVolume * Options.volume31) / 16;
        leftVolume = (leftVolume * Options.volume31) / 16;

        SetVoiceVolumeScaled(voice, leftVolume, rightVolume);

        {
            s16 pitch;
            int coarseOffset;
            s16 fine;
            int coarse;
            pitch = (slot->pitch5F8[slot->program14][programIndex].fine * pitchCurve) / 128;
            pitch += pitchOffset;
            pitch += spread;
            fine = pitch % 128;
            coarseOffset = pitch / 128;
            coarse = slot->pitch5F8[slot->program14][programIndex].coarse + coarseOffset;
            SsUtChangePitch((s16)voice, slot->bank04.half.value, (s16)slot->programData3C.programs[slot->program14][programIndex], slot->group28, 0, (s16)coarse, fine);
        }
    }
}

/* A curve is keys[9] then values[9]. Past the last key the original reads
 * keys[9] / values[9]: the first value of this curve and the first key of the
 * next one (the program payload has no host pointers, so it keeps the PS1
 * layout); read those words from the payload instead of indexing past the
 * arrays. */
static s32 SlotCurveWord(const AudioProgramData *data, int curve, int word) {
    s32 value;
    memcpy(&value, (const char *)data + (curve * 18 + word) * sizeof(s32), sizeof(value));
    return value;
}

int SlotInterpCurve(int curveIndex, int position, int programGroup, int slotIndex) {
    const AudioProgramData *data = &SoundState.slots38[slotIndex].programData3C;
    int curve = programGroup * 10 + curveIndex;
    int upperIndex, lowerIndex, lowerValue, upperValue, lowerKey, upperKey;
    for (upperIndex = 1; upperIndex < 9; upperIndex++) {
        if (position < data->curves[programGroup][curveIndex].keys[upperIndex]) break;
    }
    lowerIndex = upperIndex - 1;
    lowerValue = SlotCurveWord(data, curve, 9 + lowerIndex);
    upperValue = SlotCurveWord(data, curve, 9 + upperIndex);
    lowerKey = SlotCurveWord(data, curve, lowerIndex);
    upperKey = SlotCurveWord(data, curve, upperIndex);
    return (upperValue - lowerValue) * (position - lowerKey) / (upperKey - lowerKey) + lowerValue;
}

/* Initial values of the globals this module owns. */
s16 PanGainTable[128] = {0, 204, 408, 612, 816, 1020, 1224, 1427, 1631, 1834, 2037, 2239, 2441, 2643, 2844, 3045, 3246, 3446, 3645, 3844, 4042, 4240, 4437, 4633, 4828, 5023, 5217, 5410, 5603, 5794, 5985, 6175, 6363, 6551, 6738, 6923, 7108, 7291, 7474, 7655, 7835, 8013, 8191, 8367, 8542, 8716, 8888, 9059, 9228, 9396, 9563, 9728, 9892, 10054, 10214, 10373, 10530, 10686, 10840, 10992, 11143, 11292, 11439, 11584, 11584, 11728, 11869, 12009, 12147, 12283, 12417, 12550, 12680, 12808, 12935, 13059, 13181, 13301, 13420, 13536, 13650, 13762, 13871, 13979, 14084, 14188, 14289, 14387, 14484, 14578, 14670, 14760, 14848, 14933, 15016, 15096, 15174, 15250, 15323, 15394, 15463, 15529, 15593, 15655, 15714, 15770, 15824, 15876, 15925, 15972, 16016, 16058, 16097, 16134, 16168, 16200, 16229, 16255, 16279, 16301, 16320, 16337, 16351, 16362, 16371, 16377, 16381, 16383};
u16 MenuCarTextureFileOffsets[292] = {0xFB9A, 0xFBB7, 0xFBD3, 0xFBEF, 0xFC0B, 0xFC26, 0xFC41, 0xFC5B, 0xFC75, 0xFC8F, 0xFCA9, 0xFCC2, 0xFCDB, 0xFCF4, 0xFD0C, 0xFD25, 0xFD3D, 0xFD54, 0xFD6C, 0xFD83, 0xFD9A, 0xFDB1, 0xFDC7, 0xFDDD, 0xFDF4, 0xFE09, 0xFE1F, 0xFE34, 0xFE4A, 0xFE5F, 0xFE73, 0xFE88, 0xFE9C, 0xFEB1, 0xFEC5, 0xFED9, 0xFEEC, 0xFF00, 0xFF13, 0xFF26, 0xFF39, 0xFF4C, 0xFF5F, 0xFF71, 0xFF84, 0xFF96, 0xFFA8, 0xFFBA, 0xFFCC, 0xFFDE, 0xFFEF, 0, 0x11, 0x22, 0x33, 0x44, 0x54, 0x65, 0x75, 0x86, 0x96, 0xA6, 0xB6, 0xC6, 0xD6, 0xE6, 0xF5, 0x105, 0x114, 0x123, 0x132, 0x141, 0x150, 0x15F, 0x16E, 0x17C, 0x18B, 0x199, 0x1A8, 0x1B6, 0x1C4, 0x1D2, 0x1E0, 0x1EE, 0x1FC, 0x20A, 0x217, 0x225, 0x232, 0x240, 0x24D, 0x25A, 0x267, 0x274, 0x281, 0x28E, 0x29B, 0x2A8, 0x2B5, 0x2C1, 0x2CE, 0x2DA, 0x2E7, 0x2F3, 0x2FF, 0x30B, 0x318, 0x324, 0x330, 0x33C, 0x348, 0x353, 0x35F, 0x36B, 0x376, 0x382, 0x38E, 0x399, 0x3A4, 0x3B0, 0x3BB, 0x3C6, 0x3D1, 0x3DC, 0x3E8, 0x3F3, 0x3FD, 0x408, 0x413, 0x41E, 0x429, 0x433, 0x43E, 0x449, 0x453, 0x45E, 0x468, 0x472, 0x47D, 0x487, 0x491, 0x49B, 0x4A6, 0x4B0, 0x4BA, 0x4C4, 0x4CE, 0x4D8, 0x4E1, 0x4EB, 0x4F5, 0x4FF, 0x509, 0x512, 0x51C, 0x525, 0x52F, 0x538, 0x542, 0x54B, 0x555, 0x55E, 0x567, 0x570, 0x57A, 0x583, 0x58C, 0x595, 0x59E, 0x5A7, 0x5B0, 0x5B9, 0x5C2, 0x5CB, 0x5D4, 0x5DD, 0x5E5, 0x5EE, 0x5F7, 0x600, 0x608, 0x611, 0x619, 0x622, 0x62A, 0x633, 0x63B, 0x644, 0x64C, 0x654, 0x65D, 0x665, 0x66D, 0x675, 0x67E, 0x686, 0x68E, 0x696, 0x69E, 0x6A6, 0x6AE, 0x6B6, 0x6BE, 0x6C6, 0x6CE, 0x6D6, 0x6DE, 0x6E6, 0x6ED, 0x6F5, 0x6FD, 0x705, 0x70C, 0x714, 0x71B, 0x723, 0x72B, 0x732, 0x73A, 0x741, 0x749, 0x750, 0x758, 0x75F, 0x766, 0x76E, 0x775, 0x77C, 0x784, 0x78B, 0x792, 0x799, 0x7A0, 0x7A8, 0x7AF, 0x7B6, 0x7BD, 0x7C4, 0x7CB, 0x7D2, 0x7D9, 0x7E0, 0x7E7, 0x7EE, 0x7F5, 0x7FC, 0x803, 0x80A, 0x810, 0x817, 0x81E, 0x825, 0x82B, 0x832, 0x839, 0x840, 0x846, 0x84D, 0x853, 0x85A, 0x861, 0x867, 0x86E, 0x874, 0x87B, 0x881, 0x888, 0x88E, 0x895, 0x89B, 0x8A1, 0x8A8, 0x8AE, 0x8B5, 0x8BB, 0x8C1, 0x8C7, 0x8CE, 0x8D4, 0x8DA, 0x8E0, 0x8E7, 0x8ED, 0x8F3, 0x8F9, 0x8FF, 0x905, 0x90B, 0x505, 0x505, 0x606, 0x505};
