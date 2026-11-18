#include "types.h"
#include "rr/audio_cd_controller.h"
#include "game/sound/cdstop.h"
#include "rr/bios_devices.h"

void CdAudioStopStep(void)
{
    int r;
    switch (SoundState.step2678) {
    case 0:
        SoundState.state269C = 4;
        SoundState.playActive26BC = 0;
        CdAudioSetVolumeRaw(0);
        SoundState.step2678 = SoundState.step2678 + 1;
    case 1:
        if (CdSync(1, 0) == 0) break;
        SoundState.step2678 = SoundState.step2678 + 1;
    case 2:
        if (CdControl(9, 0, 0) == 0) break;
        goto inc;
    case 3:
        r = CdSync(1, 0);
        if (r == 2) {
    inc:
            SoundState.step2678 = SoundState.step2678 + 1;
        } else if (r == 5) {
            SoundState.step2678 = 1;
        }
        break;
    case 4:
        SoundState.state269C = -1;
        SoundState.action2670 = -1;
        SoundState.step2678 = 0;
        PsyqBzero(&SoundState.tick26D0, RR_FIELD_SPAN(AudioState, tick26D0, reserved26EC));
        break;
    }
}

int PollCdAudioStatus(void) {
    if (SoundState.state269C == 1) return SoundState.base26F0;
    return -1;
}
