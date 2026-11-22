#include "types.h"
#include "rr/audio_cd_controller.h"
#include "psyq/libsnd/ssmvol.h"
#include "psyq/libsnd/ssvolset.h"
#include "game/sound/cdmain.h"
#include "rr/overlay668_stream.h"
#include "rr/library.h"
#include "rr/bios_devices.h"

void SeqSetVolume(s16 a0);
void SeqFadeVolume(int a0, int a1);

void CdAudioUpdate(void) {
    s32 ready;
    AudioState *phaseState = &SoundState;
    u8 filter[2];
    CdLocation location;
    AudioState *var_v0;
    s32 temp_v1_2;
    s32 temp_a0;
    s32 temp_a1;
    s32 temp_v0;
    s32 temp_v0_2;
    s32 temp_v0_3;
    s32 rate;
    s32 temp_v0_4;
    s32 temp_v0_6;
    s32 temp_v1;
    s32 temp_v1_3;

    if (SoundState.sector2664 < 0) {
        switch (SoundState.action2670) {
        case 1:
            CdAudioPlayStep();
            break;
        case 2:
            CdAudioPauseStep();
            break;
        case 3:
            CdAudioResumeStep();
            break;
        case 4:
            CdAudioStopStep();
            break;
        case 5:
            CdAudioLoopStep();
            break;
        case 6:
            CdAudioFilterStep();
            break;
        }
    } else {
        switch (SoundState.phase2674) {
        case 0:
            if (CdSync(1, 0) == 0) {
                goto timeout;
            }
            SoundState.state269C = 0;
            SoundState.timer26A8 = 0;
            SoundState.phase2674 = (SoundState.phase2674 + 1U);
        case 1:
            CdIntToPos(SoundState.sector2664, location.c);
            CdControlF(0x15, location.c);
            SoundState.phase2674 = (SoundState.phase2674 + 1U);
            break;
        case 2:
            ready = 2;
            temp_v0_2 = CdSync(1, 0);
            if (temp_v0_2 == ready) {
                filter[0] = 1;
                filter[1] = (u8) SoundState.filter266C;
                if (CdControlF(0xD, filter) != 0) {
                    SoundState.phase2674 = (SoundState.phase2674 + 1U);
                }
            } else if (temp_v0_2 == 5) {
                SoundState.phase2674 = 1;
                if (CdDiskReady(1) == 0x10) {
                    ready = -1;
                    SoundState.flag267C = 1;
                    goto block_27;
                }
            } else {
timeout:
                temp_v0_3 = phaseState->timer26A8 + 1U;
                phaseState->timer26A8 = temp_v0_3;
                if (temp_v0_3 >= 0x169) {
                    ready = -1;
block_27:
                    phaseState->flag2680 = ready;
                    CdFlush();
                    phaseState->sector2664 = ready;
                    phaseState->phase2674 = 0;
                }
            }
            break;
        case 3:
            ready = 2;
            temp_v0 = CdSync(1, 0);
            if (temp_v0 == ready) {
                temp_v1 = SoundState.sector2664;
                SoundState.sector2664 = -1;
                SoundState.phase2674 = 0;
                SoundState.state269C = -1;
                SoundState.flag267C = 0;
                SoundState.flag2680 = 0;
                SoundState.sector2668 = temp_v1;
                SoundState.position2688 = temp_v1;
                SoundState.index2698 = SoundState.filter266C;
                if (SoundState.state26AC == temp_v0) {
                    SoundState.state26AC = 0;
                }
            } else if (temp_v0 == 5) {
                SoundState.phase2674 = 1;
            }
            break;
        }
    }
    var_v0 = &SoundState;
    temp_a0 = var_v0->request26B4;
    if (temp_a0 != 0 && var_v0->action2670 == -1) {
        switch (temp_a0) {
        case 1:CdAudioRequestPause();SoundState.request26B4=0;break;
        case 2:CdAudioMute();SoundState.request26B4=0;break;
        default:SoundState.request26B4=0;break;
        }
    }
    if (SoundState.state269C == 1) {
        temp_v0_4 = SoundState.toc28C8[SoundState.bankA8CC][SoundState.index2698].position;
        SoundState.previous26DC = SoundState.tick26D8;
        SoundState.base26F0 = temp_v0_4;
        rate = SoundState.rate26F4;
        if (rate != 0) {
            SoundState.tick26D8 = (((temp_v0_4 + 10U) * rate) / 600000U);
        } else {
            SoundState.tick26D8 = 0;
        }
        SoundState.tick26D0 = 0;
        SoundState.half26D2 = 0;
        temp_v1_2 = (u32) SoundState.tick26D8 - SoundState.previous26DC;
        SoundState.quarter26D4 = 0;
        if (temp_v1_2 == 1) {
            temp_a1 = (u32) SoundState.tick26D8 + SoundState.offset26E8;
            SoundState.tick26D0 = temp_v1_2;
            if (!(temp_a1 & 1)) {
                SoundState.half26D2 = temp_v1_2;
            }
            if (!(temp_a1 & 3)) {
                SoundState.quarter26D4 = temp_v1_2;
            }
        }
        if (SoundState.position2688 > SoundState.end2690) {
            if (SoundState.flag26A0 == 0) {
                SoundState.action2670 = 4;
                SoundState.step2678 = 0;
            } else if (SoundState.action2670 != 5) {
                SoundState.action2670 = 5;
                SoundState.timer26A4 = 0;
                DebugPrintStub(XaRepeatMsg, &SoundState);
            }
            CdAudioSetVolumeRaw(0);
        }
            if (SoundState.previous268C != SoundState.position2688) {
            SoundState.timer26A8 = 0;
        } else {
            SoundState.timer26A8 = (SoundState.timer26A8 + 1U);
        }
        if (SoundState.timer26A8 >= 0x1E) {
            SoundState.action2670 = 4;
            SoundState.step2678 = 0;
            SoundState.timer26A8 = 0;
        }
    }
    CdAudioFadeStep();
    SoundState.previous268C = SoundState.position2688;
    if ((SoundState.state269C - 1U) < 2U) {
        temp_v0_2 = CdSync(1, SoundState.status265C);
        switch (temp_v0_2) {
        case 0:
        case 1:
            break;
        case 2:
            if (CdLastCom() == 0x11) {
                temp_v0_6 = CdPosToInt(&SoundState.status265C[5]);
                if (temp_v0_6 > 0) {
                    SoundState.position2688 = temp_v0_6;
                }
            }
            CdControlF(0x11, 0);
            return;
        case 5:
            if (SoundState.state269C == 2) {
                CdAudioSetVolume(SoundState.volume26CE);
            }
            temp_v1_3 = SoundState.state269C;
            SoundState.state269C = -1;
            SoundState.action2670 = -1;
            SoundState.step2678 = 0;
            SoundState.state26AC = temp_v1_3;
            break;
        }
    }
}

/* Data-ready: copy the TOC sector header (32 words) into the next of the
 * sixteen 2 KB TOC banks and publish the bank that was filled. */
void CdAudioReadyCallback(u8 status, u8 *result) {
    (void)result;
    if (status == 1) {
        s32 bank = SoundState.reservedA8C8;
        CdGetSector((u8 *)SoundState.toc28C8[bank], 0x20);
        SoundState.bankA8CC = bank;
        SoundState.reservedA8C8 = (bank + 1) & 0xF;
    }
}

void ResetSeqFadeState(void) {
    SoundState.seqFadeLen288A = 0;
    SoundState.seqFadeTick288C = 0;
    SoundState.seqFadeTarget288E = 0;
    SoundState.seqFadeStart2890 = 0;
}

void SeqSetVolume(s16 a0) {
    AudioState *p = &SoundState;
    s16 v;
    p->seqVol2888 = a0;
    v = (a0 * Options.bgmVolume30) / 16;
    SsSeqSetVol((s16)p->seqHandle2864, v, v);
}

void SeqFadeVolume(int a0, int a1) {
    if ((s16)a1 >= 2) {
        u16 v = SoundState.seqVol2888;
        SoundState.seqFadeLen288A = a1;
        SoundState.seqFadeTick288C = 0;
        SoundState.seqFadeTarget288E = a0;
        SoundState.seqFadeStart2890 = v;
    } else {
        int s = (s16)a0;
        SeqSetVolume(s);
        SoundState.seqFadeLen288A = 0;
        SoundState.seqFadeTick288C = 0;
        SoundState.seqFadeTarget288E = 0;
        SoundState.seqFadeStart2890 = 0;
        if (s == 0) {
            if (SoundState.seqHandle2864 >= 0) {
                SsSeqStop((s16)SoundState.seqHandle2864);
                SsSeqClose((s16)SoundState.seqHandle2864);
                PsyqBzero(&SoundState.seqTick286C, RR_FIELD_SPAN(AudioState, seqTick286C, seqVol2888));
            }
            SoundState.seqHandle2864 = -1;
            SoundState.seqTrack2868 = -1;
        }
    }
}

/* Initial values of the globals this module owns. */
char XaRepeatMsg[64] = {88, 97, 82, 101, 112, 101, 97, 116, 10};
AudioState SoundState;
