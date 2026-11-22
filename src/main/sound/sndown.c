#include "types.h"
#include "psyq/libsnd/libsnd.h"
#include "rr/audio_bank_setup.h"
#include "game/sound/sndown.h"

void SlotSetupBank(AudioBankSetup *input)
{
    s32 bankCacheIndices[2];
    VagAtr toneRecord;
    s32 slotIndex;
    s32 programIndex;
    s32 bankAddress;
    s32 *bankCacheIndex;
    AudioSlot620 *slot;
    AudioState *state;
    s16 handle;

    slotIndex = input->slot0C;
    slot = &SoundState.slots38[slotIndex];
    bankCacheIndices[0] = 5;
    bankCacheIndices[1] = 6;
    bankCacheIndex = &bankCacheIndices[slotIndex];
    state = &SoundState;
    handle = state->banks24.ids[*bankCacheIndex];

    if (handle >= 0)
    {
        SsVabClose(handle);
        state->banks24.ids[*bankCacheIndex] = -1;
    }
    PsyqBzero(slot, sizeof(*slot));
    bankAddress = 0x5D000;
    if (slotIndex == 0)
    {
        bankAddress = 0x6F000;
    }
    if (input->bank00 != (const u8 *)~0UL)
    {
        handle = (s16) SsVabOpenHeadSticky(input->bank00, -1, bankAddress);
        state->banks24.ids[*bankCacheIndex] = handle;
        state->slots38[slotIndex].bank04.word = handle;
    }
    if (input->data04 != (const u16 *)~0UL)
    {
        handle = (s16) SsVabTransBody(input->data04, state->slots38[slotIndex].bank04.half.value);
        state->banks24.ids[*bankCacheIndex] = handle;
        state->slots38[slotIndex].bank04.word = handle;
        SsVabTransCompleted(1);
    }
    if (input->group08 != (const u8 *)~0UL)
    {
        SlotLoadCurves(input->group08, slotIndex);
    }
    slot->flags00 = 1;
    slot->levels20.halves.level = 0x4B;
    slot->levels20.halves.gain = 0x80;
    slot->pan24 = 0x40;
    slot->reserved26 = 0;
    if (!slotIndex)
    {
        slot->voice08 = 20;
    }
    else
    {
        slot->voice08 = 0x10;
    }
    slot->count0C = 4;
    programIndex = 0;

    do
    {
        SsUtGetVagAtr(
            SoundState.slots38[slotIndex].bank04.half.value,
            (s16)SoundState.slots38[slotIndex].programData3C.programs[0][programIndex],
            0, (u8 *)&toneRecord);
        SoundState.slots38[slotIndex].pitch5F8[0][programIndex].coarse =
            (s16) (60 - toneRecord.pbmin);
        SoundState.slots38[slotIndex].pitch5F8[0][programIndex].fine =
            (s16) ((toneRecord.pbmax + toneRecord.pbmin) << 7);
        /* The service can update bank/program fields; read the second group live. */
        SsUtGetVagAtr(
            SoundState.slots38[slotIndex].bank04.half.value,
            (s16)SoundState.slots38[slotIndex].programData3C.programs[1][programIndex],
            0, (u8 *)&toneRecord);
        SoundState.slots38[slotIndex].pitch5F8[1][programIndex].coarse =
            (s16) (60 - toneRecord.pbmin);
        SoundState.slots38[slotIndex].pitch5F8[1][programIndex].fine =
            (s16) ((toneRecord.pbmax + toneRecord.pbmin) << 7);
        programIndex++;
    }
    while (programIndex < 5);
}

void ResetPlayerVoices(void) {
    int i;
    for (i = 0; i < 2; i++) {
        SoundState.playerVoices195C[i].mode08 = 0;
        SoundState.playerVoices195C[i].bank00 = SoundState.banks24.ids[1];
        SoundState.playerVoices195C[i].voice04 = i + 12;
    }
}

void SetPlayerVoice(int a0, int a1, int a2, int a3) {
    SoundState.playerVoices195C[a3].mode08 = a0;
    if (a0 != 0) {
        SoundState.playerVoices195C[a3].pitch10 = a1;
        SoundState.playerVoices195C[a3].vol14 = a2;
    }
}
