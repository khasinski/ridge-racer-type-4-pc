#include "rr/video_timing.h"
#include "game/sound/fxplay.h"
#include "types.h"
#include "rr/audio_state_types.h"
#include "game/camera/campath.h"
#include "game/sound/sndvab.h"
#include "psyq/libsnd/ssopenq.h"
#include "game/sound/fxramp2.h"
#include "psyq/libsnd/sspause.h"
#include "psyq/libsnd/sspause2.h"
#include "rr/bios_devices.h"
#include "rr/overlay665_audio_options.h"
#include "game/sound/cdmain.h"

typedef struct { s32 a, b, c; } ReplayCamTargetDist;
extern ReplayCamTargetDist ReplayCamTargetDistTable[];
extern const s32 ReplayCamTargetDistNtscFrames[];
#include "psyq/libspu/smalloc2.h"

void SeqStartTrack(int a0);
void SeqMusicPause(void);
void SeqMusicResume(void);

void SeqStartTrack(int a0) {
    int arr[2] = {0x6C, 0x7F};
    if (SoundState.seqTrack2868 != a0) {
        SeqMuteVolume();
        SoundState.seqTrack2868 = a0;
        SoundState.seqHandle2864 = (s16)SsSeqOpen(SoundState.seqAddr2858[a0], SoundState.banks24.ids[a0 + 2]);
        SsSetMarkCallback((s16)SoundState.seqHandle2864, 0, SeqMarkCallback);
        ReverbReset();
        SetReverbDepthBoth(0x32);
        SsSeqPlay((s16)SoundState.seqHandle2864, 1, 0);
        SeqSetVolume((s16)arr[a0]);
        /* The sequence beat clock, seqMark2880 and seqReserved2884. */
        PsyqBzero(&SoundState.seqTick286C, RR_FIELD_SPAN(AudioState, seqTick286C, seqVol2888));
        SoundState.seqTempo287C = RR_PAL_NTSC(ReplayCamTargetDistTable[a0].a, ReplayCamTargetDistNtscFrames[a0]);
        RestartMotionSequence();
    }
}

void SeqMuteVolume(void) {
    SeqFadeVolume(0, 0);
}

void SeqMusicPause(void) {
    SsSeqPause(SoundState.seqHandle2864);
}

void SeqMusicResume(void) {
    SsSeqReplay(SoundState.seqHandle2864);
}

/* Initial values of the globals this module owns. */
ReplayCamTargetDist ReplayCamTargetDistTable[2] = {{.a = 842, .b = 17800}, {.a = 1271, .b = 11800}};
/* The NTSC release's frame counts (.a) of the two sequences (rr/video_timing.h). */
const s32 ReplayCamTargetDistNtscFrames[2] = {1011, 1525};
