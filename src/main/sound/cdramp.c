#include "rr/audio_cd_controller.h"


void CdAudioFadeStep(void)
{
    int d = SoundState.fadeLength26C6;
    int t;
    int v;
    int from;
    unsigned c;
    int to;
    if (d > 0) {
        c = (u16)SoundState.fadeTick26C8;
        if (SoundState.fadeTick26C8 < d) {
            t = c + 1;
            to = SoundState.fadeTarget26CA;
            from = SoundState.fadeStart26CC;
            v = (to - from) << 12;
            v = (v / d) * ((short) t);
            SoundState.fadeTick26C8 = t;
            if (v < 0) {
                v += 0xFFF;
            }
            v = (v >> 12) + from;
            CdAudioSetVolume(v & 0xFFFF);
            return;
        }
        SoundState.fadeLength26C6 = 0;
        CdAudioMute();
    }
}

void CdAudioPlayStep(void)
{
    int r;
    AudioState *p;
    switch (SoundState.step2678) {
    case 0:
        if (CdSync(1, 0) == 0) break;
        if (SoundState.state269C != -1) break;
        SoundState.step2678 = SoundState.step2678 + 1;
    case 1:
        if (CdControl(0x1B, 0, 0)) {
            SoundState.step2678 = SoundState.step2678 + 1;
            break;
        }
        if (CdDiskReady(1) == 0x10) {
            SoundState.flag267C = 1;
            SoundState.flag2680 = -2;
            SoundState.action2670 = -1;
            SoundState.step2678 = 0;
        }
        break;
    case 2:
        r = CdSync(1, 0);
        if (r == 2) {
            SoundState.step2678 = SoundState.step2678 + 1;
            CdAudioSetVolumeRaw(SoundState.targetVol26C0.halves.value);
        } else if (r == 5) {
            SoundState.step2678 = 1;
        }
        break;
    case 3:
        CdControl(1, 0, SoundState.status265C);
        p = &SoundState;
        if (p->status265C[0] & 0x20) {
            p->step2678 = p->step2678 + 1;
            break;
        }
        p->timer26A8 = p->timer26A8 + 1;
        if (p->timer26A8 >= 0x169) {
            p->reserved26B0 = p->reserved26B0 + 1;
            if (p->reserved26B0 >= 0x21) p->reserved26B0 = 0x20;
            p->step2678 = 0;
        }
        break;
    case 4:
        SoundState.action2670 = -1;
        SoundState.step2678 = 0;
        SoundState.state269C = 1;
        SoundState.flag2680 = 0;
        SoundState.timer26A8 = 0;
        SoundState.state26AC = 0;
        SoundState.reserved26B0 = 0;
        break;
    }
}
