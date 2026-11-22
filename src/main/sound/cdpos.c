#include "rr/release.h"
#include "types.h"
#include "rr/audio_cd_controller.h"
#include "game/sound/cdmain.h"
#include "psyq/libcd/cdfile.h"
#include "game/sound/cdpos.h"
#include "rr/overlay668_stream.h"
#include "rr/menu_overlay_runtime.h"

#include "psyq/libcd/s_1.h"
extern char R4StrPath[];
extern char CdAudioTrackTable[];
static const char CdAudioTrackTableJp[320];

void CdAudioFindStreamFile(void) {
    struct {
        u8 b[8];
        CdlFILE c;
    } L;
    SoundState.streamBanks[0].table = AUDIO_STREAM_TABLE_UNAVAILABLE;
    SoundState.streamBanks[1].table = AUDIO_STREAM_TABLE_UNAVAILABLE;
    if (CdControlB(1, 0, L.b) != 0) {
        u32 v = L.b[0];
        if ((v & 0x10) == 0) {
            if ((v & 1) == 0) {
                CdSearchFile(&L.c, R4StrPath);
                SoundState.streamBanks[0].sector = CdPosToInt(L.c.pos) & 0xFFFFFF;
                SoundState.streamBanks[0].table = (u8 *)RR_PAL_JP(CdAudioTrackTable, CdAudioTrackTableJp);
            }
        }
    }
}

void CdAudioInit(void) {
    SoundState.cdMode2658 = 0xC8;
    CdControlB(0xE, &SoundState.cdMode2658, 0);
    SsSetSerialVol(0, 0x7F, 0x7F);
    CdReadyCallback(CdAudioReadyCallback);
    SoundState.sector2664 = -1;
    SoundState.action2670 = -1;
    SoundState.phase2674 = 0;
    SoundState.step2678 = 0;
    SoundState.state269C = -1;
    SoundState.timer26A8 = 0;
    SoundState.pausedState26B8 = 0;
    SoundState.playActive26BC = 0;
    SoundState.currentVol26C4 = 0;
    SoundState.fadeLength26C6 = 0;
    SoundState.fadeTick26C8 = 0;
    SoundState.fadeTarget26CA = 0;
    CdAudioSetVolume(0x7F);
    SoundState.seqTrack2868 = -1;
    CdPlaybackTrackIndex = 0;
}

/* Initial values of the globals this module owns. */
char R4StrPath[64] = {92, 82, 52, 46, 83, 84, 82, 59, 49, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 92, 82, 52, 46, 83, 84, 82, 59, 49};
char CdAudioTrackTable[320] = {-94, 54, 2, 32, -81, 6, 0, 0, -92, 54, 2, 64, 107, 1, 0, 0, -111, -77, 1, 16, -28, 19, 0, 0, -43, 64, 2, 80, 111, 0, 0, 0, -114, 70, 2, 96, 102, 0, 0, 0, -20, 66, 2, 64, 109, 0, 0, 0, 101, 60, 2, 80, 112, 0, 0, 0, 38, 66, 2, 96, 111, 0, 0, 0, 61, 69, 2, 80, 107, 0, 0, 0, -123, 73, 2, 80, 96, 0, 0, 0, 68, 71, 2, 64, 99, 0, 0, 0, 0, 0, 0, 0, 27, 27, 0, 0, 1, 0, 0, 16, 27, 27, 0, 0, 2, 0, 0, 32, 27, 27, 0, 0, 3, 0, 0, 48, 27, 27, 0, 0, 4, 0, 0, 64, 27, 27, 0, 0, 5, 0, 0, 80, 27, 27, 0, 0, 6, 0, 0, 96, 27, 27, 0, 0, -56, -39, 0, 0, 27, 27, 0, 0, -55, -39, 0, 16, 27, 27, 0, 0, -54, -39, 0, 32, 27, 27, 0, 0, -53, -39, 0, 48, 27, 27, 0, 0, -52, -39, 0, 64, 27, 27, 0, 0, -51, -39, 0, 80, 27, 27, 0, 0, -112, -77, 1, 0, 22, 27, 0, 0, -50, -39, 0, 96, 27, 27, 0, 0, -110, -77, 1, 32, -48, 8, 0, 0, -109, -77, 1, 48, -48, 8, 0, 0, -108, -77, 1, 64, -48, 8, 0, 0, -107, -77, 1, 80, -48, 8, 0, 0, -106, -77, 1, 96, -53, 8, 0, 0, -34, -6, 1, 96, -53, 8, 0, 0, 2, -5, 1, 32, 86, 7, 0, 0, 3, -5, 1, 48, 86, 7, 0, 0, 4, -5, 1, 64, 86, 7, 0, 0, 5, -5, 1, 80, 86, 7, 0, 0, -91, 54, 2, 80, -102, 0, 0, 0, -93, 54, 2, 48, -70, 4, 0, 0, 56, -115, 2, 0, -32, 76, 0, 0, -113, -39, 2, 0, 105, -108};
/* The Japanese release's XA tracks (SLPS-01800): the last two differ. */
static const char CdAudioTrackTableJp[320] = {-0x5E, 0x36, 2, 0x20, -0x51, 6, 0, 0, -0x5C, 0x36, 2, 0x40, 0x6B, 1, 0, 0, -0x6F, -0x4D, 1, 0x10, -0x1C, 0x13, 0, 0, -0x2B, 0x40, 2, 0x50, 0x6F, 0, 0, 0, -0x72, 0x46, 2, 0x60, 0x66, 0, 0, 0, -0x14, 0x42, 2, 0x40, 0x6D, 0, 0, 0, 0x65, 0x3C, 2, 0x50, 0x70, 0, 0, 0, 0x26, 0x42, 2, 0x60, 0x6F, 0, 0, 0, 0x3D, 0x45, 2, 0x50, 0x6B, 0, 0, 0, -0x7B, 0x49, 2, 0x50, 0x60, 0, 0, 0, 0x44, 0x47, 2, 0x40, 0x63, 0, 0, 0, 0, 0, 0, 0, 0x1B, 0x1B, 0, 0, 1, 0, 0, 0x10, 0x1B, 0x1B, 0, 0, 2, 0, 0, 0x20, 0x1B, 0x1B, 0, 0, 3, 0, 0, 0x30, 0x1B, 0x1B, 0, 0, 4, 0, 0, 0x40, 0x1B, 0x1B, 0, 0, 5, 0, 0, 0x50, 0x1B, 0x1B, 0, 0, 6, 0, 0, 0x60, 0x1B, 0x1B, 0, 0, -0x38, -0x27, 0, 0, 0x1B, 0x1B, 0, 0, -0x37, -0x27, 0, 0x10, 0x1B, 0x1B, 0, 0, -0x36, -0x27, 0, 0x20, 0x1B, 0x1B, 0, 0, -0x35, -0x27, 0, 0x30, 0x1B, 0x1B, 0, 0, -0x34, -0x27, 0, 0x40, 0x1B, 0x1B, 0, 0, -0x33, -0x27, 0, 0x50, 0x1B, 0x1B, 0, 0, -0x70, -0x4D, 1, 0, 0x16, 0x1B, 0, 0, -0x32, -0x27, 0, 0x60, 0x1B, 0x1B, 0, 0, -0x6E, -0x4D, 1, 0x20, -0x30, 8, 0, 0, -0x6D, -0x4D, 1, 0x30, -0x30, 8, 0, 0, -0x6C, -0x4D, 1, 0x40, -0x30, 8, 0, 0, -0x6B, -0x4D, 1, 0x50, -0x30, 8, 0, 0, -0x6A, -0x4D, 1, 0x60, -0x35, 8, 0, 0, -0x22, -6, 1, 0x60, -0x35, 8, 0, 0, 2, -5, 1, 0x20, 0x56, 7, 0, 0, 3, -5, 1, 0x30, 0x56, 7, 0, 0, 4, -5, 1, 0x40, 0x56, 7, 0, 0, 5, -5, 1, 0x50, 0x56, 7, 0, 0, -0x5B, 0x36, 2, 0x50, -0x66, 0, 0, 0, -0x5D, 0x36, 2, 0x30, -0x46, 4, 0, 0, 0x38, -0x73, 2, 0, 0x10, 0x4D, 0, 0, -0x41, -0x27, 2, 0, 0x3E, -0x6C, 0, 0};
int CdPlaybackTrackIndex;
