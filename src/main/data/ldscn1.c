#include "rr/release.h"
#include "rr/emblem_images.h"
#include "rr/menu_arena_state.h"
#include "rr/runtime_settings.h"
#include "rr/menu_choice_types.h"
#include "rr/menu_selection.h"
#include "rr/save_slots.h"
#include "rr/menu_state_types.h"
#include "rr/work_arena.h"
#include "rr/load_cursor.h"
#include "game/camera/campath.h"
#include "game/data/bankrec.h"
#include "game/data/cdfile2.h"
#include "game/data/trkld1.h"
#include "game/data/trkld6.h"
#include "game/menu/mntrow.h"
#include "game/sound/sndcmd.h"
#include "game/car/carmove1.h"
#include "game/menu/menuslot.h"
#include "game/data/ldscn1.h"
#include "game/data/ldscn4.h"
#include "game/menu/mnrows.h"
#include "game/race/raceinfo.h"
#include "game/data/cdfile1.h"
#include "rr/audio_loop.h"
#include "rr/menu_text_state.h"
#include "rr/replay_types.h"
#include "game/course/trkintp2.h"
#include "game/gfx/clut.h"
#include "game/sound/fxplay.h"
#include "rr/overlay_module.h"

extern unsigned char LoadState;
#include "rr/cd_directory_buffer.h"
#include "rr/menu_arena_base.h"
#include "game/data/selection.h"
#include "rr/scene_camera_state.h"
extern int DialogueSceneMode;
extern char SceneOffsetLookupBuf[];
extern int TrackNodeCount;
extern char MenuSoundVabBuffer[];

int LoadDefaultRaceStep(void);
int LoadModeMenuAssetsStep(void);
int LoadMenuSoundStep(void);
int LoadSceneStep(void);
int LoadAllCarsStep(void);
int LoadCarRangeStep(int arg0, int arg1);
int LoadIndexedTimStep(void);
int LoadResultsSceneStep(void);
int LoadCarClutStep(void);
int LoadGlyphRowsStep(void);
int LoadMenuCarModelsStep(void);
int LoadTrackNodesStep(void);

int LoadDefaultRaceStep(void) {
    int r = 0;
    int i;
    switch (LoadState) {
    case 0:
        LoadState = 1;
        LoadIndex = 0;
        RaceLoadReset();
    case 1:
        if (LoadRaceSoundAndTrackStep() == 1) {
            LoadState = 2;
    case 2:
            i = 0;
            do {
                RaceContext.startGridOrder00[i] = i + 1;
                i++;
            } while (i < 7);
            RaceContext.startGridOrder00[7] = 0;
            LoadState = 3;
    case 3:
            r = 1;
        }
        break;
    }
    return r;
}

int LoadModeMenuAssetsStep(void) {
    int r = 0;
    int v;
    switch (LoadState) {
    case 0:
        if (GameModeType == 4) {
            r = 1;
            MenuArenas.first = MenuArenaBase;
            LoadCursor = MenuArenaBase;
            break;
        }
        if (GameModeType == 5 && MainMenuChoice == 6) {
            r = 1;
            LoadCursor = MenuArenaBase;
            break;
        }
        LoadState = 1;
        LoadCursor = CdDirectoryBuffer;
        LoadIndex = *(unsigned char *)&GameModeType;
        if (*(unsigned char *)&GameModeType >= 5) {
            LoadIndex = 0;
        }
    case 1:
        /* The mode overlay is linked, but its image is still read: the load
         * takes its disc time and arena space. Loading resets the module. */
        if (CdFileReadStart(GetR4FileIndex(1, LoadIndex + 0xB), LoadCursor)) {
            LoadState = 2;
        }
        break;
    case 2:
        v = CdFileBytesLoaded();
        if (v) {
            LoadCursor = (u8 *)LoadCursor + (v);
            OverlayModuleBind(GetR4FileIndex(1, LoadIndex + 0xB));
            LoadState = 3;
    case 3:
            r = 1;
            MenuArenaBase = LoadCursor;
        }
        break;
    }
    return r;
}

int LoadOpeningMovieTimStep(void) {
    int r = 0;
    int v;
    switch (LoadState) {
    case 0:
        LoadState = 1;
        LoadCursor = CdDirectoryBuffer;
        LoadIndex = MainMenuChoice;
    case 1:
        /* The overlay is linked, but its image is still read (disc time,
         * arena space); loading resets the module. */
        if (CdFileReadStart(GetR4FileIndex(1, LoadIndex + 1), LoadCursor)) {
            LoadState = 2;
        }
        break;
    case 2:
        v = CdFileBytesLoaded();
        if (v) {
            u8 *t;
            OverlayModuleBind(GetR4FileIndex(1, LoadIndex + 1));
            LoadState = 3;
            t = (u8 *)LoadCursor + v;
            LoadCursor = t;
            MenuArenaBase = t;
    case 3:
            r = 1;
        }
        break;
    }
    return r;
}

int LoadMenuSoundStep(void) {
    int r = 0;
    switch (LoadState) {
    case 0:
        LoadState = 1;
        LoadCursor = MenuArenaBase;
        RaceLoadReset();
    case 1:
        if (LoadMenuSoundVabStep()) {
            MenuArenaBase = LoadCursor;
            SoundCloseVabsAndReset();
            LoadState = 2;
            InitMenuArenas();
            LoadCursor = MenuArenas.second;
    case 2:
            r = 1;
        }
        break;
    }
    return r;
}

int LoadSceneStep(void) {
    int r = 0;
    switch (LoadState) {
    case 0:
        LoadState = 1;
        LoadIndex = 0;
    case 1:
        if (RequestSceneLoad(CourseIndex)) {
            LoadState = 2;
        }
        break;
    case 2:
        if (SceneLoadStep()) r = 1;
        break;
    case 3:
        r = 1;
        break;
    }
    return r;
}

int LoadAllCarsStep(void) {
    int s0;
    int s1 = 0;
    int r;
    int a1;
    int t;
    char *s2;
    switch (LoadState) {
    case 0:
        LoadState = 3;
        LoadIndex = 0;
    case 3:
        s0 = LoadIndex;
        if (s0 < 0) goto l5;
        if (RequestCarLoad(SelectionRows.entries[s0].variant, SelectionRows.entries[s0].selection, SelectionRows.entries[s0].parameter % 20, PlayerCourseChoices[s0].first, s0) == 1) {
            LoadState = 4;
        }
        break;
    case 4:
        s0 = LoadIndex;
        r = CarLoadStep();
        if (r != 1) break;
        a1 = PlayerCourseChoices[s0].second;
        if (a1 >= 2) {
            s2 = PresetEmblemTimPtr;
            a1 -= 2;
        } else {
            if (a1 == r) {
                s2 = CustomEmblemBuffers.bytes + (s0 & 1) * sizeof(EmblemTim840);
                a1 = 0;
            } else {
                a1 = -1;
            }
        }
        UploadCarClut(s2, a1, s0);
        LoadIndex++;
        s1 = -LoadIndex;
        if (LoadIndex != 4) {
            LoadState = 3;
        } else {
l5:
            LoadState = 5;
        }
        break;
    case 5:
        s1 = 1;
        if (GameModeType == 4) MenuArenas.first = LoadCursor;
        break;
    default:
        break;
    }
    return s1;
}

int LoadCarRangeStep(int arg0, int arg1) {
    int r;
    int q4 = 0;
    int a1;
    char *q3;
    switch (LoadState) {
    case 0:
        LoadState = 1;
        LoadIndex = 0;
        break;
    case 1:
        arg0 += LoadIndex;
        if (RequestCarLoad(SelectionRows.entries[arg0].variant, SelectionRows.entries[arg0].selection, SelectionRows.entries[arg0].parameter % 20, PlayerCourseChoices[arg0].first, arg0) == 1) {
            LoadState = 2;
        }
        break;
    case 2:
        r = CarLoadStep();
        if (r != 1) break;
        arg0 += LoadIndex;
        a1 = PlayerCourseChoices[arg0].second;
        if (a1 >= 2) {
            q3 = PresetEmblemTimPtr;
            a1 -= 2;
        } else {
            if (a1 == r) {
                q3 = CustomEmblemBuffers.bytes + (arg0 & 1) * sizeof(EmblemTim840);
                a1 = 0;
            } else {
                a1 = -1;
            }
        }
        if (GameModeType != 1) {
            UploadCarClut(q3, a1, arg0);
        }
        LoadIndex++;
        if (LoadIndex == arg1) {
            LoadState = 3;
        } else {
            LoadState = 1;
        }
        break;
    case 3:
        q4 = 1;
        break;
    default:
        break;
    }
    return q4;
}

int LoadIndexedTimStep(void) {
    int r = 0;
    switch (LoadState) {
    case 0:
        LoadCursor = SharedWorkArena;
        LoadState = 1;
        LoadIndex = 0;
    case 1:
        if (CdFileReadStart(GetR4FileIndex(1, LoadIndex + 0x11), LoadCursor)) {
            LoadState = 2;
        }
        break;
    case 2:
        if (CdFileBytesLoaded()) {
            LoadTimList(LoadCursor);
            LoadState = 3;
    case 3:
            r = 1;
        }
        break;
    }
    return r;
}

int LoadResultsSceneStep(void) {
    int r = 0;
    switch (LoadState) {
    case 0:
        LoadState = 1;
        SetPresentationDrawEnabled(0);
        LoadIndex = 0;
    case 1:
        if (CdFileReadStart(GetR4FileIndex(1, 0xF), LoadCursor)) {
            LoadState = 2;
        }
        break;
    case 2:
        if (CdFileBytesLoaded()) {
            LoadTimList(LoadCursor);
            LoadState = 3;
            LoadCursor = MenuBaseLoadCursor;
            RaceLoadReset();
        }
        break;
    case 3:
        if (LoadTrackVabsStep()) {
            InitMotionController();
            SoundCloseVabsAndReset();
            if (GameModeType == 1) {
                goto s7;
            }
            LoadState = 5;
        }
        break;
    case 5:
        if (RequestVramImageLoad()) {
            LoadState = 6;
        }
        break;
    case 6:
        if (VramImageLoadStep()) {
s7:
            LoadState = 7;
        }
        break;
    case 7:
        r = 1;
        break;
    }
    return r;
}

int LoadCarClutStep(void) {
    int r = 0;
    switch (LoadState) {
    case 0:
        if (GameModeType == 1) {
            LoadState = 3;
        } else {
            LoadState = 1;
        }
        LoadDestCursor = CdDirectoryBuffer + 0xAF000;
    case 1:
        if (CdFileReadStart(GetR4FileIndex(1, 0x12), LoadDestCursor)) {
            LoadState = 2;
        }
        break;
    case 2:
        if (CdFileBytesLoaded()) {
            LoadState = 3;
            PresetEmblemTimPtr = LoadDestCursor;
    case 3:
            r = 1;
        }
        break;
    }
    return r;
}

int LoadTimListStep(void) {
    int r = 0;
    switch (LoadState) {
    case 0:
        LoadIndex = 0;
        LoadState = 1;
        LoadCursor = MenuArenas.second;
    case 1:
        if (CdFileReadStart(GetR4FileIndex(1, 0x13), LoadCursor)) {
            LoadState = 2;
        }
        break;
    case 2:
        if (CdFileBytesLoaded()) {
            LoadTimList(LoadCursor);
            LoadState = 3;
    case 3:
            r = 1;
        }
        break;
    }
    return r;
}

int LoadGlyphRowsStep(void) {
    int r = 0;
    int v;
    switch (LoadState) {
    case 0:
        LoadIndex = 0;
        LoadState = 1;
        GlyphRowCount = 4;
    case 1:
        if (CdFileReadRange(GetR4FileIndex(1, RR_PAL_JP(0x40, 0x60)), 0, 0xC0, LoadCursor)) {
            LoadState = 2;
        }
        break;
    case 2:
        v = CdFileBytesLoaded();
        if (v) {
            SetGlyphRowImageBase(LoadCursor, 0);
            LoadState = 3;
            LoadCursor = (u8 *)LoadCursor + (v);
        }
        break;
    case 3:
        r = 1;
        break;
    }
    return r;
}

int LoadDialogueStep(void) {
    int s3 = 0;
    int s0, s1, s2, a0, k;
    u8 *v0;

    switch (LoadState) {
    case 0:
        k = 0xAF000;
        *(unsigned char *)&LoadIndex = 0;
        LoadDestCursor = CdDirectoryBuffer + k;
        v0 = LoadDestCursor;
        LoadDestCursor = v0;
        Dialogue = (void *)v0;
        v0 += sizeof(MenuTextState);
        LoadDestCursor = v0;
        if (DialogueSceneMode == 1 && SelectionState.fields.phase17 == 7 && SelectionState.fields.mode1A == 4) {
            SeqStartTrack(1);
        }
        InitDialogueState(Dialogue);
        LoadState = 1;
        break;
    case 1:
        if (RR_RELEASE_JP) {
            /* Each scene's dialogue is a whole entry: 17 entries a team
             * from 20 (two a Grand Prix stage), 97 outside the story. */
            s2 = DialogueSceneMode == 1
                     ? SelectionState.fields.copies[0].bytes[0] * 0x11 + SelectionState.fields.phase17 * 2 + 0x14
                     : 0x61;
            if (CdFileReadStart(GetR4FileIndex(1, s2), LoadDestCursor)) {
                LoadState = 2;
            }
            break;
        }
        if (DialogueSceneMode == 1) {
            s2 = 0x41;
            s1 = 0x14;
            s2 = SelectionState.fields.copies[0].bytes[0] * 9 + (SelectionState.fields.phase17 + s1);
        } else {
            s2 = 0x41;
            s1 = 4;
        }
        if (CdFileReadRange(GetR4FileIndex(1, s2), s1 * Options.storyBranch3C, s1, LoadDestCursor)) {
            LoadState = 2;
        }
        break;
    case 2:
        s2 = CdFileBytesLoaded();
        if (s2) {
            SetupDialogueTable(LoadDestCursor);
            LoadState = 3;
            LoadDestCursor += s2;
            Dialogue->images20 = LoadDestCursor;
        }
        break;
    case 3:
        /* The Japanese release reads the scene's image list (the entry
         * after its dialogue, 98 outside the story); the PAL one a single
         * image. */
        s2 = 0x42;
        if (RR_RELEASE_JP)
            s2 = DialogueSceneMode == 1
                     ? SelectionState.fields.copies[0].bytes[0] * 0x11 + SelectionState.fields.phase17 * 2 + 0x15
                     : 0x62;
        if (CdFileReadStart(GetR4FileIndex(1, s2), LoadDestCursor)) {
            LoadState = 4;
        }
        break;
    case 4:
        if (CdFileBytesLoaded()) {
            if (RR_RELEASE_JP)
                LoadTimList((int *)LoadDestCursor);
            else
                LoadTimImage((int *)LoadDestCursor);
            LoadState = 5;
        }
        break;
    case 5:
        s2 = RR_RELEASE_JP ? SelectionState.fields.copies[0].bytes[0] * 0x11 + 0x24
                           : SelectionState.fields.copies[0].bytes[0] * 9 + 0x1C;
        a0 = LoadIndex;
        s0 = DialogueAssetTable[Dialogue->b4 * 8 + a0];
        if (s0 != -1) {
            if (DialogueSceneMode == 0) {
                s1 = 8;
                s0 <<= 3;
            } else if (DialogueSceneMode == 1) {
                s1 = 8;
                if (a0 == 0) {
                    s1 = 0x2E;
                    s0 = s0 * s1;
                } else {
                    s0 <<= 3;
                    s0 += 0xE6;
                }
            } else {
                s1 = 8;
                s0 <<= 3;
            }
            if (CdFileReadRange(GetR4FileIndex(1, s2), s0, s1, LoadDestCursor)) {
                LoadState = 6;
            }
        } else {
            LoadState = 7;
        }
        break;
    case 6:
        s2 = CdFileBytesLoaded();
        if (s2) {
            if (LoadIndex == 0) {
                LoadTimList((int *)LoadDestCursor);
            } else {
                LoadDestCursor = LoadDestCursor + s2;
            }
            LoadIndex = LoadIndex + 1;
            s0 = DialogueAssetTable[Dialogue->b4 * 8 + LoadIndex];
            if (LoadIndex == 8 || s0 == -1) {
                LoadState = 7;
            } else {
                LoadState = 5;
            }
        }
        break;
    case 7:
        s3 = 1;
        break;
    }
    return s3;
}

int LoadMenuCarModelsStep(void) {
    int r = 0;
    char *s;
    int a1;
    switch (LoadState) {
    case 0:
        LoadState = 1;
        LoadIndex = 0;
    case 1:
        /* The Japanese release has an image per dialogue scene (entries
         * 100, 102, 104) where the PAL one has a single image. */
        if (RR_RELEASE_JP)
            a1 = DialogueSceneMode == 2 ? 0x64 : DialogueSceneMode == 4 ? 0x68 : 0x66;
        else
            a1 = 0x42;
        if (CdFileReadStart(GetR4FileIndex(1, a1), SharedWorkArena)) {
            LoadState = 2;
        }
        break;
    case 2:
        if (CdFileBytesLoaded()) {
            s = SharedWorkArena;
            LoadTimImage(s);
            s += 0x4E20;
            LoadState = 3;
            CardDialogueStatePtr = (MenuTextState *)s;
            InitDialogueState((MenuTextState *)s);
    case 3:
            if (RR_RELEASE_JP) {
                /* The scene's whole dialogue table (entries 101, 103, 105);
                 * the PAL release reads the language's offset from 68..70. */
                a1 = DialogueSceneMode == 2 ? 0x65 : DialogueSceneMode == 4 ? 0x69 : 0x67;
                if (CdFileReadStart(GetR4FileIndex(1, a1), SceneOffsetLookupBuf)) {
                    LoadState = 4;
                }
                break;
            }
            if (DialogueSceneMode == 2) {
                a1 = 0x44;
            } else if (DialogueSceneMode == 4) {
                a1 = 0x46;
                CardDialogueStatePtr->b7 = 2;
            } else {
                a1 = 0x45;
            }
            if (CdFileReadRange(GetR4FileIndex(1, a1), Options.storyBranch3C << 2, 4, SceneOffsetLookupBuf)) {
                LoadState = 4;
            }
        }
        break;
    case 4:
        if (CdFileBytesLoaded()) {
            SetupDialogueTable(SceneOffsetLookupBuf);
            LoadState = 5;
    case 5:
            r = 1;
        }
        break;
    }
    return r;
}

int LoadTrackNodesStep(void) {
    int r = 0;
    switch (LoadState) {
    case 0:
        LoadIndex = 0;
        LoadState = 1;
        TrackPreviewTick = 0;
    case 1:
        if (CdFileReadStart(GetR4FileIndex(1, CourseIndex + RR_PAL_JP(0x38, 0x58)), LoadCursor) != 0) {
            LoadState = 2;
        }
        break;
    case 2:
        if (CdFileBytesLoaded() != 0) {
            SetTrackNodeData(LoadCursor);
            LoadState = 3;
            TrackNodesEnd = (char *)(TrackNodes + TrackNodeCount);
    case 3:
            r = 1;
        }
        break;
    }
    return r;
}

int LoadSplashAssetsStep(void) {
    int r = 0;
    int t;
    switch (LoadState) {
    case 0:
        LoadState = 1;
        LoadIndex = 0;
    case 1:
        if (CdFileReadStart(GetR4FileIndex(1, 0), LoadCursor) != 0) {
            LoadState = 2;
        }
        break;
    case 2:
        if (CdFileBytesLoaded() != 0) {
            LoadTimList(LoadCursor);
            RaceLoadReset();
            LoadState = 3;
        }
        break;
    case 3:
        if (LoadVabPairStep(MenuSoundVabBuffer) != 0) {
            SoundCloseVabsAndReset();
            LoadState = 4;
            LoadCursor = CdDirectoryBuffer;
        }
        break;
    case 4:
        /* movie_playback is linked, but entry 668 is still read (disc time,
         * arena space); loading resets the module. */
        if (CdFileReadStart(GetR4FileIndex(1, 10), LoadCursor) != 0) {
            LoadState = 5;
        }
        break;
    case 5:
        t = CdFileBytesLoaded();
        if (t != 0) {
            OverlayModuleBind(GetR4FileIndex(1, 10));
            LoadCursor = (u8 *)LoadCursor + (t);
            LoadState = 6;
            MenuArenaBase = LoadCursor;
            case 6:
            r = 1;
        }
        break;
    }
    return r;
}

/* Initial values of the globals this module owns. */
#include "rr/menu_overlay_runtime.h"
/* PS1 free RAM above the executable: the overlay window 0x80114878 up to the
 * stack, about 0xEB000 bytes, where menus and race loads place overlay
 * images, models, textures and sound banks. Host tables and packets are
 * wider, so the arena is twice that. */
static _Alignas(16) u8 FreeRamArena[0x200000];
u8 *CdDirectoryBuffer = FreeRamArena;
char LoadIndex;
u8 *LoadDestCursor;
signed char *DialogueAssetTable;
void *LoadCursor;
void *MenuBaseLoadCursor;
char *TrackNodesEnd;
void *PresetEmblemTimPtr;
int DialogueSceneMode;
void *MenuArenaBase;
unsigned char LoadState;
/* PS1: 0x5168 bytes. The host layouts placed in it are larger: the dialogue
 * state LoadMenuCarModelsStep puts at +0x4E20 and the replay storage. (The
 * PS1 two-player replay storage, 0x60E8 bytes, already ran on into
 * SceneOffsetLookupBuf; that buffer is reloaded before every dialogue
 * scene, so keeping the two apart changes nothing observable.) */
#define SHARED_WORK_ARENA_MAX(a, b) ((a) > (b) ? (a) : (b))
u8 SharedWorkArena[SHARED_WORK_ARENA_MAX(
    SHARED_WORK_ARENA_MAX(0x5168, 0x4E20 + sizeof(MenuTextState)), sizeof(ReplayStorage))];
/* 0x80104DB0: LoadMenuCarModelsStep reads four sectors (0x2000 bytes) of
 * dialogue tables here. The PS1 map's next labels, DuelRaceVehicles
 * (+0x1DA8) and GrandPrixRaceVehicles, are only ComCars - 2 / - 1 pointer
 * bases (see InitCarTable); the extent runs to LinkPlayerCount (+0x23C8). */
char SceneOffsetLookupBuf[0x2000];
char MenuSoundVabBuffer[8192];
