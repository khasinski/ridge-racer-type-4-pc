#include "rr/release.h"
#include "rr/scene_camera_state.h"
#include "rr/runtime_settings.h"
#include "rr/menu_selection.h"
#include "game/gfx/scene1.h"
#include "game/data/cdfile2.h"
#include "rr/load_cursor.h"
#include "game/gfx/motion1.h"
#include "game/sound/cdpos.h"
#include "psyq/libcd/s_1.h"
#include "psyq/libspu/s_i.h"
#include "game/data/trkld1.h"
#include "game/data/trkld8.h"
#include "game/course/trkres2.h"
#include "game/data/cdfile1.h"
#include "rr/course_choices.h"
#include "rr/sequencer_init.h"
#include "rr/spu_tick.h"
#include "rr/bios_devices.h"
#include "rr/audio_state_types.h"
#include "game/course/trkintp2.h"
#include "rr/overlay_module.h"
extern short SelectedTeamIndex;
#include "rr/menu_arena_base.h"
extern void SsSetSerialAttr(signed char a0, signed char a1, signed char a2);
extern int SsSetSerialVol(signed char a0, int a1, int a2);

int LoadIntroSceneStep(void) {
    int v;
    int t;
    int u;
    char *w;
    int g;
    int h;
    switch (RaceLoadStepIndex) {
    case 0:
        /* movie_playback is linked, but entry 668 is still read (disc time,
         * arena space); loading resets the module. */
        v = CdFileReadStart(GetR4FileIndex(1, 10), RaceLoadCursor);
        if (v != 0) {
            RaceLoadStepIndex += 1;
    case 1:
            t = CdFileBytesLoaded();
            if (t != 0) {
                OverlayModuleBind(GetR4FileIndex(1, 10));
                u = (int)AlignUp4(t);
                w = RaceLoadCursor + u;
                CourseIndex = 8;
                LoadCursor = w;
                RaceLoadCursor = w;
                MenuArenaBase = w;
                RaceLoadStepIndex += 1;
            }
    default:
            v = 0;
        }
        return v;
    case 2:
        v = RequestSceneLoad(CourseIndex);
        if (v != 0) {
            RaceLoadStepIndex += 1;
    case 3:
            if (SceneLoadStep() != 0) {
                RaceLoadStepIndex += 1;
            }
            v = 0;
        }
        return v;
    case 4:
        /* The PAL release has a team image set per language. */
        g = RR_RELEASE_JP ? 0 : Options.storyBranch3C << 2;
        h = SelectedTeamIndex + 0x97;
        v = CdFileReadStart(GetR4FileIndex(2, g + h), LoadCursor);
        if (v != 0) {
            RaceLoadStepIndex += 1;
    case 5:
            if (CdFileBytesLoaded() != 0) {
                LoadTimList(LoadCursor);
                RaceLoadStepIndex = 0;
                return 1;
            }
            v = 0;
        }
        return v;
    }
    return 0;
}

void SoundSystemInit(void) {
    SequenceChannel *table;
    AudioState *state;
    int i;
    s32 emptySlot;
    s16 emptyBank;
    CdReset(1);
    SsSetSerialAttr(0, 0, 1);
    SsSetSerialVol(0, 0x7F, 0x7F);
    table = SoundState.sequences;
    SsSetTableSize(table, 2, 1);
    SsSetTickMode(1);
    state = &SoundState;
    SsStart();
    SsSetMVol(0x7F, 0x7F);
    PsyqBzero(state, sizeof(*state));
    PsyqBzero(&RaceContext, sizeof(RaceContext));
    InitSceneOverlayState();
    ResetSlotOrder();
    state->master08 = 0x80;
    emptySlot = -1;
    for (i = 3; i >= 0; i--)
        state->slots38[i].bank04.word = emptySlot;
    emptyBank = -1;
    for (i = 7; i >= 0; i--)
        SoundState.banks24.ids[i] = emptyBank;
    SoundState.seqInit2860 = -1;
    SoundState.seqInit2862 = -1;
    CdAudioInit();
    CdAudioFindStreamFile();
}
