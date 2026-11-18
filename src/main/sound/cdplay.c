#include "rr/video_timing.h"
#include "types.h"
#include "game/sound/cdplay.h"
#include "rr/audio_cd_controller.h"
#include "rr/frame_loop.h"
#include "rr/bios_devices.h"

#include "game/demo/opening.h"
#include "psyq/libc/rand.h"


/* The NTSC release's frame counts (timing[0]) of the 24 timed tracks
 * (rr/video_timing.h); the rest of each triple is the same. */
enum { CD_TRACK_TIMED_NTSC = 24 };
static const s32 CdTrackFramesNtsc[CD_TRACK_TIMED_NTSC] = {
    1071, 1241, 1090, 1352, 1636, 1323, 1250, 1250, 1250, 1058, 1267, 1250,
    1176, 1417, 1125, 1250, 1176, 1417, 1125, 1250, 1176, 1417, 1125, 1250};

void CdAudioSelectTrack(int bank, int track) {
    AudioState *state = &SoundState;
    u32 sector;
    u8 *table;
    int filter;
    int offset, end;
    const s32 *timing;
    table = state->streamBanks[bank].table;
    if (table != AUDIO_STREAM_TABLE_UNAVAILABLE) {
        sector = ((AudioStreamEntry *)table)[track].position;
        filter = sector >> 28;
        sector &= 0x0FFFFFFF;
        sector += state->streamBanks[bank].sector;
        CdAudioSetTarget(sector, filter, 0);
        state->end2690 = (((AudioStreamEntry *)state->streamBanks[bank].table)[track].sectors - 2U) * 8;
        PsyqBzero(&state->tick26D0, RR_FIELD_SPAN(AudioState, tick26D0, reserved26EC));
        { const s32 *base = CdTrackTables.timing[0]; timing = base + track * 3; }
        state->reserved26EC = 0;
        state->base26F0 = 0;
        state->reserved26E0 = VideoTimingNtsc && (unsigned)track < CD_TRACK_TIMED_NTSC ? CdTrackFramesNtsc[track] : timing[0];
        state->rate26F4 = timing[1];
        end = state->end2690;
        offset = timing[2];
        state->timer26A8 = 0;
        state->end2690 = end + sector;
        state->offset26E8 = offset;
        state->targetVol26C0.word = CdTrackTables.volumes[track];
        CdAudioSetVolumeRaw(0);
    }
    PsyqSrand(FrameCounter);
}

void CdAudioSetTarget(int a0, int a1, int a2) {
    SoundState.sector2664 = a0;
    SoundState.filter266C = a1;
    SoundState.phase2674 = 0;
    if (a2 == 0) SoundState.reserved2684 = a0;
    SoundState.action2670 = -1;
    SoundState.step2678 = 0;
    if (SoundState.state269C == 2) CdAudioSetVolume(SoundState.volume26CE);
}

/* Initial values of the globals this module owns. */
/* 0x800A0904..0x800A0C9B, one object: per-track CD volumes (the PS1 map's
 * StageUnlockMovieIds; entries 22..37 are its CdTrackTitleStrings label) and
 * the per-track timing triples (its RaceOverlayFilePath, 0x800A099C). The
 * opening and title screens read "titles" at volumes[22] + track * 32
 * (CD_TRACK_TITLE), which runs on into the timing words. */
CdTrackTableSet CdTrackTables = {
    .volumes = {127, 127, 127, 72, 72, 72, 72, 72, 72, 72, 72, 127, 127, 112, 127, 110, 120, 127, 127, 127, 127, 118, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127},
    .timing = {{892, 16800, 1}, {1034, 14500}, {909, 16500}, {1127, 13305}, {1363, 11000}, {1102, 13600}, {1041, 14400}, {1041, 14400}, {1041, 14400}, {882, 17000}, {1056, 14200, 1}, {1041, 14400, 1}, {980, 15300}, {1181, 12700}, {937, 16000}, {1041, 14400}, {980, 15300}, {1181, 12700}, {937, 16000}, {1041, 14400}, {980, 15300}, {1181, 12700}, {937, 16000}, {1041, 14400}, {1381322585, 1112102432, 69}, {0}, {0, 0, 1212959059}, {1413829967, 1142965588, 1162038849}, {0}, {0, 1163284301, 4541728}, {0}, {0}, {1162559822, 1279729732, 22351}, {0}, {0, 0, 541411412}, {1162103122}, {0}, {0, 1280722258, 1414090057}, {1314211360, 75}, {0}, {541411412, 1162494543, 1447646275}, {69}, {0, 0, 1431455828}, {0}, {0}, {0, 1330925389, 1347625042}, {1162429253, 83}, {0}, {1229149516, 1213341764, 1296585817}, {83}, {0, 0, 1162433873}, {1430462548, 1397053010}, {0}, {0, 1314018626, 539446857}, {1111643474, 21061}, {0}, {1380009296, 1279402060, 1394623829}, {5002575}, {0, 0, 1230393165}, {1226843982, 1229135950, 1162625874}, {83}, {0, 542392645, 541934887}, {2183253}},
};
