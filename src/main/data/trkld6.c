#include "rr/course_occlusion.h"
#include "rr/scene_camera_state.h"
#include "rr/menu_selection.h"
#include "rr/race_audio_resources.h"
#include "game/data/cdfile2.h"
#include "rr/load_cursor.h"
#include "game/data/trkld1.h"
#include "game/data/trkld5.h"
#include "game/data/trkld6.h"
#include "game/course/trkres2.h"
#include "game/data/cdfile1.h"
#include "game/sound/sndcmd.h"
#include "rr/controller_hud.h"
#include "rr/course_choices.h"
#include "game/gfx/model.h"
#include "rr/audio_state_types.h"
#include "rr/audio_loop.h"
#include "rr/sequence_queue.h"
#include "game/course/trkintp2.h"
#include "game/sound/sndvab.h"
#include "rr/relative_file.h"
extern short SelectedMakerIndex;
extern unsigned short SavedCourseIndex;


void SetupRaceSoundCarConfig(void) {
    switch (GameModeType) {
    case 6:
        SoundState.sequenceVariant19CC = 1;
        RaceContext.sound.sfxVabId = 5;
        RaceContext.sound.carVariants[0] = GetPlayerCarVariantIndex(0);
        RaceContext.sound.carVariants[1] = -1;
        SoundState.split10 = 0;
        RaceContext.sound.comBank = SelectedMakerIndex + 1;
        break;
    case 0:
        SoundState.sequenceVariant19CC = 1;
        RaceContext.sound.sfxVabId = 5;
        RaceContext.sound.carVariants[0] = GetPlayerCarVariantIndex(0);
        RaceContext.sound.carVariants[1] = -1;
        RaceContext.sound.comBank = -1;
        SoundState.split10 = 0;
        break;
    default:
    case 1:
    case 5:
        SoundState.sequenceVariant19CC = 0;
        RaceContext.sound.sfxVabId = 4;
        RaceContext.sound.carVariants[0] = GetPlayerCarVariantIndex(0);
        RaceContext.sound.carVariants[1] = -1;
        RaceContext.sound.comBank = 0;
        SoundState.split10 = 0;
        break;
    case 2:
    case 3:
        SoundState.sequenceVariant19CC = 1;
        RaceContext.sound.sfxVabId = 5;
        RaceContext.sound.carVariants[0] = GetPlayerCarVariantIndex(0);
        RaceContext.sound.carVariants[1] = GetPlayerCarVariantIndex(1);
        RaceContext.sound.comBank = -1;
        SoundState.split10 = 1;
        break;
    case 4:
        SoundState.sequenceVariant19CC = 1;
        RaceContext.sound.sfxVabId = 5;
        if (LinkHostFlag != 0) {
            RaceContext.sound.carVariants[0] = GetPlayerCarVariantIndex(0);
            RaceContext.sound.carVariants[1] = GetPlayerCarVariantIndex(1);
        } else {
            RaceContext.sound.carVariants[0] = GetPlayerCarVariantIndex(2);
            RaceContext.sound.carVariants[1] = GetPlayerCarVariantIndex(3);
        }
        RaceContext.sound.comBank = -1;
        SoundState.split10 = 1;
        break;
    }
}

int LoadRaceSoundAndTrackStep(void) {
    switch (RaceSoundTrackLoadState) {
    case 0:
        SetupRaceSoundCarConfig();
        RaceSoundTrackLoadState += 1;
    case 1:
        if (LoadRaceSoundStep() != 0) {
            RaceLoadStepIndex = 0;
            RaceLoadState = 0;
            RaceLoadSubState = 0;
            RaceLoadCursor = LoadCursor;
            SavedCourseIndex = CourseIndex;
            if (CourseIndex >= 8) {
                CourseIndex = 7;
            }
            RaceSoundTrackLoadState += 1;
        }
        return 0;
    case 2:
        if (LoadTrackStep() != 0) {
            CourseOcclusionCourseLoaded();
            RaceSoundTrackLoadState += 1;
            CourseIndex = SavedCourseIndex;
        }
        return 0;
    case 3:
        return 1;
    default:
        return 0;
    }
}

int LoadTrackVabsStep(void)
{
    s32 bank, started, result;
    VabLoadConfig *configs;
    char *resource;
    switch (RaceLoadStepIndex) {
    case 0:
        result = started = CdFileReadStart(GetR4FileIndex(2, 1), RaceLoadCursor);
        if (started == 0) break;
        RaceLoadStepIndex = RaceLoadStepIndex + 1;
    case 1:
        if (CdFileBytesLoaded() == 0) { result = 0; break; }
        resource = (char *)(RaceLoadCursor + ((const TrackAudioResourceHeader *)RaceLoadCursor)->modelBankOffset);
        RegisterModelBank(resource, 7);
        bank = 0;
        configs = &CourseVabLoadConfigs[1];
        RaceContext.soundPayload564 = RelocateOffsetTable(RaceLoadCursor + ((const TrackAudioResourceHeader *)RaceLoadCursor)->relocationOffset);
        do {
            VabLoadArgs.header = (u8 *)(RaceLoadCursor + *(((const TrackAudioResourceHeader *)RaceLoadCursor)->bankHeaderOffsets + bank));
            VabLoadArgs.samples = (const u16 *)(RaceLoadCursor + *(((const TrackAudioResourceHeader *)RaceLoadCursor)->bankSampleOffsets + bank));
            LoadVab(&VabLoadArgs, configs[bank].open_mode, configs[bank].bank_slot);
            SoundState.seqAddr2858[bank] = (u8 *)(RaceLoadCursor + *(((const TrackAudioResourceHeader *)RaceLoadCursor)->sequenceOffsets + bank));
            bank++;
        } while (bank < 2);
        LoadTimList((int *)(RaceLoadCursor + ((const TrackAudioResourceHeader *)RaceLoadCursor)->imageListOffset));
        resource = (char *)(((uintptr_t)(RaceLoadCursor + ((const TrackAudioResourceHeader *)RaceLoadCursor)->bankSampleOffsets[0]) + 3) & ~(uintptr_t)3);
        LoadCursor = resource;
        RaceLoadCursor = resource;
        RaceLoadStepIndex = 0;
        SoundCloseVabsAndReset();
        result = 1;
        break;
    default:
        result = 0;
        break;
    }
    return result;
}

int LoadTrophyModelStep(void) {
    int v;
    switch (RaceLoadStepIndex) {
    case 0:
        v = CdFileReadStart(GetR4FileIndex(2, 2), RaceLoadCursor);
        if (v != 0) {
            RaceLoadStepIndex += 1;
    case 1:
            if (CdFileBytesLoaded() != 0) {
                RegisterModelBank(RaceLoadCursor + ((const TrophyResourceHeader *)RaceLoadCursor)->modelBankOffset, 8);
                LoadTimList((int *)(RaceLoadCursor + ((const TrophyResourceHeader *)RaceLoadCursor)->imageListOffset));
                RaceLoadStepIndex = 0;
                return 1;
            }
            v = 0;
        }
        return v;
    default:
        return 0;
    }
}

int LoadAttractRaceStep(void) {
    int r;
    VabLoadConfig *q;
    switch (RaceLoadStepIndex) {
    case 0:
        r = CdFileReadStart(GetR4FileIndex(2, 0), RaceLoadCursor);
        if (r != 0) {
            RaceLoadStepIndex++;
    case 1:
            if (CdFileBytesLoaded()) {
                q = &CourseVabLoadConfigs[6];
                VabLoadArgs.header = (u8 *)(RaceLoadCursor + ((const AttractAudioResourceHeader *)RaceLoadCursor)->bankHeaderOffset);
                VabLoadArgs.samples = (const u16 *)(RaceLoadCursor + ((const AttractAudioResourceHeader *)RaceLoadCursor)->bankSamplesOffset);
                LoadVab(&VabLoadArgs, q->open_mode, q->bank_slot);
                LoadTimList((int *)(RaceLoadCursor + ((const AttractAudioResourceHeader *)RaceLoadCursor)->imageListOffset));
                RaceLoadStepIndex = 0;
                RaceLoadCursor = RaceLoadCursor + ((const AttractAudioResourceHeader *)RaceLoadCursor)->bankSamplesOffset;
                LoadCursor = (void *)RaceLoadCursor;
                return 1;
            }
            r = 0;
        }
        break;
    default:
        r = 0;
    }
    return r;
}

/* Initial values of the globals this module owns. */
s32 RaceSoundTrackLoadState;
s32 RaceLoadStepIndex;
unsigned short SavedCourseIndex;
