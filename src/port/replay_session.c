/* Race replays saved as files: the game side (replay_session.h). */

#include "replay_session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rr/current_selection_types.h"
#include "rr/gpu.h"
#include "rr/emblem_tim_types.h"
#include "rr/menu_selection.h"
#include "rr/record_banks.h"
#include "rr/replay_host.h"
#include "rr/replay_types.h"
#include "rr/save_slot_types.h"
#include "rr/scene_camera_types.h"
#include "rr/ui.h"
#include "rr/car_render_types.h"
#include "scratchpad.h"
#include "rr/vehicle_table.h"
#include "rr/work_arena.h"
#include "game/race/objrec.h"

#include "replay_file.h"
#include "settings_replays.h"

/* PsyZ (psyz/system.h, psyz/gpu.h; the game headers above define the SDK
 * types). */
unsigned int Psyz_RandState(void);
void Psyz_Srand(unsigned int seed);
int Psyz_GpuSetVramCopy(int enabled);
const unsigned short *Psyz_GpuGetVramCopy(const unsigned char **known);
int LoadImage(GpuRectangle *rectangle, const void *pixels);
int DrawSync(int mode);

/* Game state the replay is set up from (owners in src/main). */
extern s16 GameModeType, CourseIndex, ReverseCourse, SelectedTeamIndex, SelectedMakerIndex,
    SelectedCourseChoice, LinkHostFlag, LinkPlayerCount;
extern s32 NumLaps, TotalRaceTime, ModeFrame, RaceState, TrackLength, RaceTimeRemaining, RaceVoiceFlags,
    CarCollisionGraceTimers, ObjSimLastActiveFrame[30], TrackObjVisibleFrame[32];
extern s16 PlayerDisplayRpm[2], PlayerThrottleLoad[2];
extern ParticleSystem32C ParticleSystem;
extern CarSlotBufferC8 CarSlotBuffers[16];
extern u32 ScreenFrame;
extern CurrentSelection SelectionRows;
extern MenuGridPoint PlayerCourseChoices[4];
extern SavedMenuSettings SelectionState;
extern CustomEmblemImages1080 CustomEmblemBuffers;
extern s8 LinkEntryState[2][2];
extern SceneRaceContext RaceContext;
extern S_ECB4 *TrackObjectStates;
extern int TrackObjectCount;
extern int VideoTimingNtsc;
extern u16 ModeGroup, SubmodeState, MenuScriptId, ActiveSceneId;
extern s16 ModeIndex;
void SeqMuteVolume(void);
void StopRaceAudioAndRumble(void);

/* ---- what a replay is made of ---- */

typedef struct ReplayGlobal {
    const char *name;
    void *data;
    size_t size;
} ReplayGlobal;

#define GLOBAL(name) {#name, &(name), sizeof(name)}

/* The race setup the load script reads: mode, course, the cars and their
 * colours and emblems, the Grand Prix stage, the VS options. Applied before
 * the load; the player's own values come back when the replay ends. */
static const ReplayGlobal kSetup[] = {
    GLOBAL(GameModeType),        GLOBAL(CourseIndex),        GLOBAL(ReverseCourse),
    GLOBAL(NumLaps),             GLOBAL(NumCars),            GLOBAL(SelectionRows),
    GLOBAL(PlayerCourseChoices), GLOBAL(SelectionState),     GLOBAL(SelectedTeamIndex),
    GLOBAL(SelectedMakerIndex),  GLOBAL(SelectedCourseChoice), GLOBAL(CustomEmblemBuffers),
    GLOBAL(LinkEntryState),      GLOBAL(LinkPlayerCount),
};
enum { SETUP_COUNT = sizeof(kSetup) / sizeof(kSetup[0]) };

/* Race state the replay reads besides the car snapshots, as it was when
 * the replay started: restored over what the load set up. The scratchpad
 * holds scalars only (the VS race leaves its course level-of-detail table
 * selected at 0x5E, for one). */
static const ReplayGlobal kContext[] = {
    GLOBAL(ScreenFrame),          GLOBAL(ModeFrame),           GLOBAL(TotalRaceTime),
    GLOBAL(RaceTimeRemaining),    GLOBAL(RaceVoiceFlags),      GLOBAL(CarCollisionGraceTimers),
    GLOBAL(PlayerDisplayRpm),     GLOBAL(PlayerThrottleLoad),  GLOBAL(ObjSimLastActiveFrame),
    GLOBAL(TrackObjVisibleFrame), GLOBAL(ParticleSystem),       GLOBAL(RrScratchpad),
};
enum { CONTEXT_COUNT = sizeof(kContext) / sizeof(kContext[0]) };

/* The pointer-free parts of RaceContext the race and its replay use: grid
 * order, car and player counts and the race camera's follow state; the
 * secondary view camera; the scene overlay (the replay's blur, toggled with
 * Triangle) and its title-screen switch; the fade sprite state. */
static const struct {
    size_t begin, end;
} kRaceContextParts[] = {
    {0, offsetof(SceneRaceContext, subOrdering50)},
    {offsetof(SceneRaceContext, view60), offsetof(SceneRaceContext, streamResourceB0)},
    {offsetof(SceneRaceContext, overlayEnabledD4), offsetof(SceneRaceContext, standings)},
    {offsetof(SceneRaceContext, controllerSlot3D8), offsetof(SceneRaceContext, work444)},
};
enum { RACE_CONTEXT_PARTS = sizeof(kRaceContextParts) / sizeof(kRaceContextParts[0]) };
#define CONTEXT_HEAD_SIZE                                                                         \
    (offsetof(SceneRaceContext, subOrdering50) +                                                  \
     (offsetof(SceneRaceContext, streamResourceB0) - offsetof(SceneRaceContext, view60)) +        \
     (offsetof(SceneRaceContext, standings) - offsetof(SceneRaceContext, overlayEnabledD4)) +     \
     (offsetof(SceneRaceContext, work444) - offsetof(SceneRaceContext, controllerSlot3D8)))

/* Textures the race shows that the menus put in VRAM before the race load
 * (car bodies, colours, emblems and wheel variants, menu-loaded pages):
 * every texel outside the display buffers that a transfer wrote, as runs
 * of u16 y, x, count and the RGB5551 texels. */
enum {
    VRAM_W = 1024,
    VRAM_H = 512,
    VRAM_FIRST_X = 320, /* the display buffers are 320 wide */
    VRAM_BLOB_MAX = (VRAM_W - VRAM_FIRST_X) * VRAM_H * 2 + VRAM_H * 176 * 6,
};

/* The cars' light trail, motion history and spark rings (CarSlotBuffers and
 * the vertex rings AllocCarSlotBuffers gave them), carried from the race
 * into its replay: per slot entry the entry with its pointers cleared, then
 * the ring contents. */
enum {
    CAR_SLOT_ENTRIES = 16,
    CAR_SPARKS = 16,
    TRAILS_BLOB_MAX = 1 << 20,
};

enum {
    SNAPSHOTS_MAX = 8,
    TRACK_OBJECTS_MAX = 1024,
    /* Pointer links of a snapshot: which live pointers the car had. */
    LINK_RECORD = 1,
    LINK_HANDLING = 2,
};

/* One replay as blocks: the recorded storage (snapshot pointers cleared),
 * the snapshot links, setup and context. */
typedef struct ReplayData {
    int valid;
    ReplayFileInfo info;
    ReplayStorage storage;
    s32 saved_race_state, replay_length; /* SavedRaceState, ReplayLength */
    s32 snapshot_offset[SNAPSHOTS_MAX]; /* CarSnapshots[i] in storage, -1 unused */
    u32 snapshot_links[SNAPSHOTS_MAX];
    u32 rand_state;
    u8 context_head[CONTEXT_HEAD_SIZE];
    s32 track_objects;
    S_ECB4 track_object_states[TRACK_OBJECTS_MAX];
    unsigned char setup[SETUP_COUNT][sizeof(CustomEmblemImages1080)];
    s32 overlay_toggle; /* RaceContext.overlayToggle3DC with the setup */
    unsigned char context[CONTEXT_COUNT][RR_SCRATCHPAD_SIZE];
    u32 vram_size;
    u8 vram[VRAM_BLOB_MAX];
    u32 trails_size;
    u8 trails[TRAILS_BLOB_MAX];
} ReplayData;

static ReplayData s_last;     /* the replay recorded (or loaded) last */
static ReplayData s_loaded;   /* the file being loaded or played */
static ReplayData s_player;   /* the player's setup while a loaded replay runs */
static int s_load_pending;    /* s_loaded waits for the title screen */
static int s_load_active;     /* s_loaded is being loaded or played */
static int s_replay_starting; /* ReplayModeEnter ran, the first frame is next */
static char s_directory[1024];
static char s_prefs_path[1024];
#if RR4_DIAGNOSTICS
static const char *s_auto_save;
#endif

uint32_t ReplaySessionLayout(void) {
    /* Host sizes of every copied structure, and their order. */
    uint32_t sizes[] = {
        (uint32_t)sizeof(RaceVehicle), (uint32_t)sizeof(ReplayStorage), (uint32_t)sizeof(S_ECB4),
        (uint32_t)CONTEXT_HEAD_SIZE,   (uint32_t)SETUP_COUNT,          (uint32_t)CONTEXT_COUNT,
        (uint32_t)offsetof(RaceVehicle, control.record18),
        (uint32_t)offsetof(RaceVehicle, control.handling1C),
        (uint32_t)sizeof(CarSlotBufferC8), (uint32_t)sizeof(RroSpark), (uint32_t)sizeof(RroProjectedVertex),
    };
    uint32_t crc = 0;
    for (size_t i = 0; i < SETUP_COUNT; i++)
        crc = ReplayCrc32(&kSetup[i].size, sizeof(kSetup[i].size), crc);
    for (size_t i = 0; i < CONTEXT_COUNT; i++)
        crc = ReplayCrc32(&kContext[i].size, sizeof(kContext[i].size), crc);
    return ReplayCrc32(sizes, sizeof(sizes), crc);
}

static void CopyGlobals(const ReplayGlobal *table, int count, unsigned char (*out)[sizeof(CustomEmblemImages1080)],
                        size_t stride) {
    for (int i = 0; i < count; i++)
        memcpy((unsigned char *)out + (size_t)i * stride, table[i].data, table[i].size);
}

static void ApplyGlobals(const ReplayGlobal *table, int count, const unsigned char *in, size_t stride) {
    for (int i = 0; i < count; i++)
        memcpy(table[i].data, in + (size_t)i * stride, table[i].size);
}

static void SaveTrails(ReplayData *d);
static void ApplyTrails(const ReplayData *d);

static void SaveSetup(ReplayData *d) {
    CopyGlobals(kSetup, SETUP_COUNT, d->setup, sizeof(d->setup[0]));
    d->overlay_toggle = RaceContext.overlayToggle3DC;
}

/* The race setup; the player's own also brings back their replay-blur
 * switch, which the loaded replay's context replaced. */
static void ApplySetup(const ReplayData *d, int player) {
    ApplyGlobals(kSetup, SETUP_COUNT, (const unsigned char *)d->setup, sizeof(d->setup[0]));
    if (player)
        RaceContext.overlayToggle3DC = d->overlay_toggle;
}

static void SaveContext(ReplayData *d) {
    for (int i = 0; i < CONTEXT_COUNT; i++)
        memcpy(d->context[i], kContext[i].data, kContext[i].size);
    d->rand_state = Psyz_RandState();
    {
        size_t at = 0;
        for (int i = 0; i < RACE_CONTEXT_PARTS; i++) {
            size_t n = kRaceContextParts[i].end - kRaceContextParts[i].begin;
            memcpy(d->context_head + at, (const u8 *)&RaceContext + kRaceContextParts[i].begin, n);
            at += n;
        }
    }
    d->track_objects = TrackObjectCount;
    if (d->track_objects < 0 || d->track_objects > TRACK_OBJECTS_MAX || !TrackObjectStates)
        d->track_objects = 0;
    if (d->track_objects)
        memcpy(d->track_object_states, TrackObjectStates, (size_t)d->track_objects * sizeof(S_ECB4));
    SaveTrails(d);
}

static void ApplyContext(const ReplayData *d) {
    for (int i = 0; i < CONTEXT_COUNT; i++)
        memcpy(kContext[i].data, d->context[i], kContext[i].size);
    Psyz_Srand(d->rand_state);
    {
        size_t at = 0;
        for (int i = 0; i < RACE_CONTEXT_PARTS; i++) {
            size_t n = kRaceContextParts[i].end - kRaceContextParts[i].begin;
            memcpy((u8 *)&RaceContext + kRaceContextParts[i].begin, d->context_head + at, n);
            at += n;
        }
    }
    if (d->track_objects == TrackObjectCount && TrackObjectStates)
        memcpy(TrackObjectStates, d->track_object_states, (size_t)d->track_objects * sizeof(S_ECB4));
    ApplyTrails(d);
}

/* Ring sizes of a slot entry, as AllocCarSlotBuffers laid them out. */
static void TrailSizes(const CarSlotBufferC8 *e, u32 *primary, u32 *history) {
    *primary = ((e->model->primary10.vertexCount + 2) / 3) * (3 * sizeof(RroProjectedVertex));
    *history = ((e->model->history34.vertexCount + 2) / 3) * (3 * sizeof(RroProjectedVertex));
}

static int TrailPut(ReplayData *d, u32 *at, const void *data, u32 size) {
    if (*at + size > sizeof(d->trails))
        return 0;
    if (size)
        memcpy(d->trails + *at, data, size);
    *at += size;
    return 1;
}

static void SaveTrails(ReplayData *d) {
    u32 at = 0;

    d->trails_size = 0;
    for (int i = 0; i < CAR_SLOT_ENTRIES; i++) {
        const CarSlotBufferC8 *e = &CarSlotBuffers[i];
        CarSlotBufferC8 copy = *e;
        u32 present = e->model != NULL, primary = 0, history = 0, sparks = 0, ok = 1;

        if (present)
            TrailSizes(e, &primary, &history);
        sparks = present && e->sparksC4 != NULL;
        copy.model = NULL;
        copy.sparksC4 = NULL;
        for (int k = 0; k < 4; k++)
            copy.main14[k].vertices = copy.second3C[k].vertices = NULL;
        for (int k = 0; k < 8; k++)
            copy.history84[k].vertices = NULL;
        ok = TrailPut(d, &at, &present, 4) && TrailPut(d, &at, &primary, 4) && TrailPut(d, &at, &history, 4) &&
             TrailPut(d, &at, &sparks, 4) && TrailPut(d, &at, &copy, sizeof(copy));
        for (int k = 0; ok && present && k < 4; k++)
            ok = TrailPut(d, &at, e->main14[k].vertices, primary) && TrailPut(d, &at, e->second3C[k].vertices, history);
        for (int k = 0; ok && present && k < 8; k++)
            ok = TrailPut(d, &at, e->history84[k].vertices, history);
        if (ok && sparks)
            ok = TrailPut(d, &at, e->sparksC4, sizeof(RroSpark) * CAR_SPARKS);
        if (!ok)
            return; /* too large: leave the rings as the load made them */
    }
    d->trails_size = at;
}

static int TrailGet(const ReplayData *d, u32 *at, void *out, u32 size) {
    if (*at + size > d->trails_size)
        return 0;
    if (out && size)
        memcpy(out, d->trails + *at, size);
    *at += size;
    return 1;
}

/* Over the entries the load allocated for the same models. */
static void ApplyTrails(const ReplayData *d) {
    u32 at = 0;

    for (int i = 0; i < CAR_SLOT_ENTRIES && at < d->trails_size; i++) {
        CarSlotBufferC8 *e = &CarSlotBuffers[i], copy;
        u32 present, primary, history, sparks, live_primary = 0, live_history = 0;
        int fits;

        if (!TrailGet(d, &at, &present, 4) || !TrailGet(d, &at, &primary, 4) || !TrailGet(d, &at, &history, 4) ||
            !TrailGet(d, &at, &sparks, 4) || !TrailGet(d, &at, &copy, sizeof(copy)))
            return;
        if (e->model)
            TrailSizes(e, &live_primary, &live_history);
        fits = present && e->model && live_primary == primary && live_history == history &&
               (!sparks || e->sparksC4);
        if (fits) {
            CarSlotBufferC8 live = *e;
            *e = copy;
            e->model = live.model;
            e->sparksC4 = live.sparksC4;
            for (int k = 0; k < 4; k++) {
                e->main14[k].vertices = live.main14[k].vertices;
                e->second3C[k].vertices = live.second3C[k].vertices;
            }
            for (int k = 0; k < 8; k++)
                e->history84[k].vertices = live.history84[k].vertices;
        }
        for (int k = 0; present && k < 4; k++) {
            if (!TrailGet(d, &at, fits ? e->main14[k].vertices : NULL, primary) ||
                !TrailGet(d, &at, fits ? e->second3C[k].vertices : NULL, history))
                return;
        }
        for (int k = 0; present && k < 8; k++)
            if (!TrailGet(d, &at, fits ? e->history84[k].vertices : NULL, history))
                return;
        if (sparks && !TrailGet(d, &at, fits ? e->sparksC4 : NULL, sizeof(RroSpark) * CAR_SPARKS))
            return;
    }
}

static int Saveable(int mode) {
    return mode == 0 || mode == 1 || mode == 2 || mode == 3 || mode == 6;
}

static void Title(const ReplayFileInfo *info, char *out, size_t size) {
    snprintf(out, size, "%s, course %d%s", ReplayModeName(info->mode), info->course + 1,
             info->reverse ? " reverse" : "");
}

static void PutU16(u8 *p, int v) {
    p[0] = (u8)v;
    p[1] = (u8)(v >> 8);
}

static int GetU16(const u8 *p) { return p[0] | p[1] << 8; }

static void SaveVram(ReplayData *d) {
    const unsigned char *known = NULL;
    const unsigned short *vram = Psyz_GpuGetVramCopy(&known);
    u32 at = 0;

    d->vram_size = 0;
    if (!vram || !known)
        return;
    for (int y = 0; y < VRAM_H; y++) {
        int x = VRAM_FIRST_X;
        while (x < VRAM_W) {
            int start, count;
            while (x < VRAM_W && !known[y * VRAM_W + x])
                x++;
            start = x;
            while (x < VRAM_W && known[y * VRAM_W + x])
                x++;
            count = x - start;
            if (!count)
                continue;
            if (at + 6 + (u32)count * 2 > sizeof(d->vram))
                return;
            PutU16(d->vram + at, y);
            PutU16(d->vram + at + 2, start);
            PutU16(d->vram + at + 4, count);
            memcpy(d->vram + at + 6, vram + y * VRAM_W + start, (size_t)count * 2);
            at += 6 + (u32)count * 2;
        }
    }
    d->vram_size = at;
}

static void ApplyVram(const ReplayData *d) {
    u32 at = 0;
    while (at + 6 <= d->vram_size) {
        GpuRectangle r;
        int count = GetU16(d->vram + at + 4);
        r.rect.x = (s16)GetU16(d->vram + at + 2);
        r.rect.y = (s16)GetU16(d->vram + at);
        r.rect.w = (s16)count;
        r.rect.h = 1;
        if (at + 6 + (u32)count * 2 > d->vram_size)
            break;
        LoadImage(&r, d->vram + at + 6);
        at += 6 + (u32)count * 2;
    }
    DrawSync(0);
}

static int VramValid(const u8 *data, u32 size) {
    u32 at = 0;
    while (at < size) {
        int y, x, count;
        if (size - at < 6)
            return 0;
        y = GetU16(data + at);
        x = GetU16(data + at + 2);
        count = GetU16(data + at + 4);
        if (y >= VRAM_H || x < VRAM_FIRST_X || count <= 0 || x + count > VRAM_W ||
            size - at - 6 < (u32)count * 2)
            return 0;
        at += 6 + (u32)count * 2;
    }
    return 1;
}

/* The replay the game is about to play, from its own memory. */
static int Capture(ReplayData *d) {
    int i;

    memset(d, 0, sizeof(*d));
    memcpy(&d->storage, SharedWorkArena, sizeof(d->storage));
    d->saved_race_state = SavedRaceState;
    d->replay_length = ReplayLength;
    for (i = 0; i < SNAPSHOTS_MAX; i++)
        d->snapshot_offset[i] = -1;
    for (i = 0; i < NumCars && i < SNAPSHOTS_MAX; i++) {
        uintptr_t address = (uintptr_t)CarSnapshots[i];
        uintptr_t base = (uintptr_t)SharedWorkArena;
        size_t offset;
        RaceVehicle *snapshot;
        if (address < base || address - base > sizeof(d->storage) - sizeof(RaceVehicle))
            return 0;
        offset = (size_t)(address - base);
        d->snapshot_offset[i] = (s32)offset;
        snapshot = (RaceVehicle *)((u8 *)&d->storage + offset);
        if (snapshot->control.record18 && snapshot->control.record18 == CarTable[i]->control.record18)
            d->snapshot_links[i] |= LINK_RECORD;
        if (snapshot->control.handling1C && snapshot->control.handling1C == CarTable[i]->control.handling1C)
            d->snapshot_links[i] |= LINK_HANDLING;
        snapshot->control.record18 = NULL;
        snapshot->control.handling1C = NULL;
    }
    SaveSetup(d);
    SaveVram(d);
    d->info.layout = ReplaySessionLayout();
    d->info.ntsc = VideoTimingNtsc != 0;
    d->info.mode = GameModeType;
    d->info.course = CourseIndex;
    d->info.reverse = ReverseCourse;
    d->info.car = (SelectionRows.entries[0].selection & 0xFF) << 8 | (SelectionRows.entries[0].variant & 0xFF);
    d->info.cars = NumCars;
    d->info.race_time = GameModeType < 2 ? TotalRaceTime : -1;
    d->info.frames = (u32)ReplayFrame;
    Title(&d->info, d->info.title, sizeof(d->info.title));
    /* valid once the context is taken, at the replay's first frame */
    return 1;
}

/* The loaded replay over the race the load script set up. */
static void Install(const ReplayData *d) {
    memcpy(SharedWorkArena, &d->storage, sizeof(d->storage));
    SavedRaceState = d->saved_race_state;
    ReplayLength = d->replay_length;
    ApplyVram(d);
    for (int i = 0; i < NumCars && i < SNAPSHOTS_MAX; i++) {
        RaceVehicle *snapshot;
        if (d->snapshot_offset[i] < 0)
            continue;
        snapshot = (RaceVehicle *)(SharedWorkArena + d->snapshot_offset[i]);
        CarSnapshots[i] = snapshot;
        snapshot->control.record18 = d->snapshot_links[i] & LINK_RECORD ? CarTable[i]->control.record18 : NULL;
        snapshot->control.handling1C =
            d->snapshot_links[i] & LINK_HANDLING ? CarTable[i]->control.handling1C : NULL;
    }
}

/* ---- file blocks ---- */

static int ToBlocks(const ReplayData *d, ReplayBlock *blocks) {
    int n = 0;

#define BLOCK(label, ptr, bytes)                                              \
    do {                                                                      \
        snprintf(blocks[n].name, sizeof(blocks[n].name), "%s", label);        \
        blocks[n].data = (ptr);                                               \
        blocks[n].size = (uint32_t)(bytes);                                   \
        n++;                                                                  \
    } while (0)
    BLOCK("ReplayStorage", &d->storage, sizeof(d->storage));
    BLOCK("SavedRaceState", &d->saved_race_state, sizeof(d->saved_race_state));
    BLOCK("ReplayLength", &d->replay_length, sizeof(d->replay_length));
    BLOCK("SnapshotOffsets", d->snapshot_offset, sizeof(d->snapshot_offset));
    BLOCK("SnapshotLinks", d->snapshot_links, sizeof(d->snapshot_links));
    BLOCK("rand", &d->rand_state, sizeof(d->rand_state));
    BLOCK("RaceContext", d->context_head, sizeof(d->context_head));
    BLOCK("TrackObjectStates", d->track_object_states, (size_t)d->track_objects * sizeof(S_ECB4));
    BLOCK("Vram", d->vram, d->vram_size);
    BLOCK("CarSlotBuffers", d->trails, d->trails_size);
    for (int i = 0; i < SETUP_COUNT; i++)
        BLOCK(kSetup[i].name, d->setup[i], kSetup[i].size);
    for (int i = 0; i < CONTEXT_COUNT; i++)
        BLOCK(kContext[i].name, d->context[i], kContext[i].size);
#undef BLOCK
    return n;
}

static int Take(const ReplayFileImage *image, const char *name, void *out, size_t size, int exact) {
    const ReplayBlock *b = ReplayFileFind(image, name);
    if (!b || (exact ? b->size != size : b->size > size))
        return 0;
    memcpy(out, b->data, b->size);
    return 1;
}

static int FromImage(const ReplayFileImage *image, ReplayData *d, char *err, size_t size) {
    const ReplayBlock *objects;
    int ok;

    memset(d, 0, sizeof(*d));
    d->info = image->info;
    ok = Take(image, "ReplayStorage", &d->storage, sizeof(d->storage), 1) &&
         Take(image, "SavedRaceState", &d->saved_race_state, sizeof(d->saved_race_state), 1) &&
         Take(image, "ReplayLength", &d->replay_length, sizeof(d->replay_length), 1) &&
         Take(image, "SnapshotOffsets", d->snapshot_offset, sizeof(d->snapshot_offset), 1) &&
         Take(image, "SnapshotLinks", d->snapshot_links, sizeof(d->snapshot_links), 1) &&
         Take(image, "rand", &d->rand_state, sizeof(d->rand_state), 1) &&
         Take(image, "RaceContext", d->context_head, sizeof(d->context_head), 1);
    for (int i = 0; ok && i < SETUP_COUNT; i++)
        ok = Take(image, kSetup[i].name, d->setup[i], kSetup[i].size, 1);
    for (int i = 0; ok && i < CONTEXT_COUNT; i++)
        ok = Take(image, kContext[i].name, d->context[i], kContext[i].size, 1);
    {
        const ReplayBlock *vram = ReplayFileFind(image, "Vram");
        if (ok && vram && vram->size <= sizeof(d->vram) && VramValid(vram->data, vram->size)) {
            d->vram_size = vram->size;
            memcpy(d->vram, vram->data, vram->size);
        } else {
            ok = 0;
        }
    }
    {
        const ReplayBlock *trails = ReplayFileFind(image, "CarSlotBuffers");
        if (ok && trails && trails->size <= sizeof(d->trails)) {
            d->trails_size = trails->size;
            memcpy(d->trails, trails->data, trails->size);
        } else {
            ok = 0;
        }
    }
    objects = ReplayFileFind(image, "TrackObjectStates");
    if (ok && objects && objects->size % sizeof(S_ECB4) == 0 &&
        objects->size <= sizeof(d->track_object_states)) {
        d->track_objects = (s32)(objects->size / sizeof(S_ECB4));
        memcpy(d->track_object_states, objects->data, objects->size);
    } else {
        ok = 0;
    }
    for (int i = 0; ok && i < SNAPSHOTS_MAX; i++)
        if (d->snapshot_offset[i] >= 0 &&
            (size_t)d->snapshot_offset[i] + sizeof(RaceVehicle) > sizeof(d->storage))
            ok = 0;
    if (!ok) {
        snprintf(err, size, "The replay file is incomplete.");
        return 0;
    }
    if (!Saveable(d->info.mode)) {
        snprintf(err, size, "This kind of replay cannot be played.");
        return 0;
    }
    if (d->info.ntsc != (VideoTimingNtsc != 0)) {
        snprintf(err, size, "The replay was recorded with %s video timing; switch Video timing to play it.",
                 d->info.ntsc ? "NTSC" : "PAL");
        return 0;
    }
    d->valid = 1;
    return 1;
}

/* ---- the game's hooks ---- */

static int AtTitle(void) { return ModeGroup == 1 && ModeIndex == 4; }
static int InReplay(void) { return ModeGroup == 1 && (ModeIndex == 21 || ModeIndex == 22); }

/* Back from a loaded replay: the player's setup, then the title screen (the
 * game's own way out of a race to the title, as the Grand Prix pause menu
 * quits). */
static void LeaveLoadedReplay(void) {
    ApplySetup(&s_player, 1);
    s_load_active = 0;
    StopRaceAudioAndRumble();
    ModeGroup = 1;
    ModeIndex = 3;
    SubmodeState = 0;
}


static void AutoSave(void) {
#if RR4_DIAGNOSTICS
    char status[256];
    ReplayBlock blocks[REPLAY_BLOCKS_MAX];
    int count;

    if (!s_auto_save || !*s_auto_save || !s_last.valid)
        return;
    count = ToBlocks(&s_last, blocks);
    s_last.info.saved_at = (int64_t)time(NULL);
    if (!ReplayFileWrite(s_auto_save, &s_last.info, blocks, count, status, sizeof(status)))
        fprintf(stderr, "ridge-racer-type-4-pc: %s\n", status);
    else
        fprintf(stderr, "ridge-racer-type-4-pc: replay saved to %s\n", s_auto_save);
#endif
}

/* RR4_REPLAY_CAPTURE=base[,step]: the frame each replay frame drew, keyed
 * by ReplayFrame (base.<frame>.ppm), independent of how many fields the
 * frames took. Read at the start of the next game frame: the frame drawn
 * last is complete and on display. */
unsigned char *Psyz_VideoAllocRenderedFrame(int *w, int *h);
static void CaptureReplayFrame(void) {
#if RR4_DIAGNOSTICS
    static int last = -1;
    const char *base = getenv("RR4_REPLAY_CAPTURE");
    const char *comma;
    int step = 1, w = 0, h = 0, frame = ReplayFrame - 1;
    unsigned char *rgb;
    char path[1200];
    FILE *f;

    if (!base || !*base || !(ModeGroup == 1 && ModeIndex == 22) || s_replay_starting || frame < 0 ||
        frame == last)
        return;
    comma = strchr(base, ',');
    if (comma)
        step = atoi(comma + 1) > 0 ? atoi(comma + 1) : 1;
    last = frame;
    if (frame % step)
        return;
    rgb = Psyz_VideoAllocRenderedFrame(&w, &h);
    snprintf(path, sizeof(path), "%.*s.%d.ppm", comma ? (int)(comma - base) : (int)strlen(base), base, frame);
    f = rgb ? fopen(path, "wb") : NULL;
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        fwrite(rgb, 3, (size_t)w * (size_t)h, f);
        fclose(f);
    }
    free(rgb);
#endif
}

static void HostFrame(void) {
#if RR4_DIAGNOSTICS
    static int was_replaying;
    int replaying = ModeGroup == 1 && ModeIndex == 22;

    CaptureReplayFrame();
    /* RR4_REPLAY_EXIT=1: end a headless run when a replay has played. */
    if (was_replaying && !replaying && getenv("RR4_REPLAY_EXIT") && atoi(getenv("RR4_REPLAY_EXIT"))) {
        fprintf(stderr, "ridge-racer-type-4-pc: replay ended after %d frames\n", ReplayFrame);
        exit(0);
    }
    was_replaying = replaying;
#else
    CaptureReplayFrame();
#endif
    if (s_replay_starting && ModeGroup == 1 && ModeIndex == 22) {
        /* ReplayModeEnter has run (it reseeds rand() from FrameCounter when
         * it picks the music): the replay's first frame starts here. */
        s_replay_starting = 0;
        if (s_load_active) {
            ApplyContext(&s_loaded);
            s_last = s_loaded;
        } else {
            SaveContext(&s_last);
            s_last.valid = Saveable(s_last.info.mode);
            AutoSave();
        }
    }
    if (!s_load_pending)
        return;
    if (InReplay() && s_load_active) {
        /* Leave this saved replay first; the load starts at the title. */
        if (s_load_active)
            ApplySetup(&s_player, 1);
        s_load_active = 0;
        s_replay_starting = 0;
        StopRaceAudioAndRumble();
        ModeGroup = 1;
        ModeIndex = 3;
        SubmodeState = 0;
        return;
    }
    if (!AtTitle())
        return;
    s_load_pending = 0;
    s_load_active = 1;
    SaveSetup(&s_player);
    ApplySetup(&s_loaded, 0);
    SeqMuteVolume();
    /* The full race load (mode overlay, all cars, course, SetupRace), with
     * the replay mode as the scene it ends in. */
    MenuScriptId = 10;
    ActiveSceneId = 0x115;
    ModeGroup = 1;
    ModeIndex = 0x23;
    SubmodeState = 0;
}

static void HostEnter(void) {
    s_replay_starting = 1;
    if (s_load_active) {
        Install(&s_loaded);
        return;
    }
    if (!Capture(&s_last))
        s_replay_starting = 0;
}

static int HostFinish(void) {
    if (!s_load_active)
        return 0;
    LeaveLoadedReplay();
    return 1;
}

static const ReplayHostHooks kHooks = {HostFrame, HostEnter, HostFinish};

/* ---- menu actions ---- */

const char *ReplaySessionDirectory(void) {
    if (!s_directory[0] && !ReplayDirectory(s_prefs_path, s_directory, sizeof(s_directory))) {
        s_directory[0] = '\0';
        return NULL;
    }
    return s_directory;
}

int ReplaySessionCanSave(char *why, size_t size) {
    if (!s_last.valid) {
        snprintf(why, size, "No replay yet: finish a race and its replay can be saved.");
        return 0;
    }
    return 1;
}

int ReplaySessionSave(char *status, size_t size) {
    char name[256], path[1400];
    ReplayBlock blocks[REPLAY_BLOCKS_MAX];
    const char *dir;
    int count;

    if (!ReplaySessionCanSave(status, size))
        return 0;
    dir = ReplaySessionDirectory();
    if (!dir) {
        snprintf(status, size, "The replays folder cannot be created.");
        return 0;
    }
    s_last.info.saved_at = (int64_t)time(NULL);
    Title(&s_last.info, s_last.info.title, sizeof(s_last.info.title));
    ReplayFileName(&s_last.info, name, sizeof(name));
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    count = ToBlocks(&s_last, blocks);
    if (!ReplayFileWrite(path, &s_last.info, blocks, count, status, size))
        return 0;
    snprintf(status, size, "Saved %s", name);
    return 1;
}

int ReplaySessionCanLoad(char *why, size_t size) {
    if (s_load_pending || (s_load_active && !InReplay())) {
        snprintf(why, size, "A replay is being loaded.");
        return 0;
    }
    if (InReplay() && !s_load_active) {
        /* The race results (records, Grand Prix progress) follow it. */
        snprintf(why, size, "Let this replay finish: the race results come next. Replays load from the title screen.");
        return 0;
    }
    if (!AtTitle() && !InReplay()) {
        snprintf(why, size, "Replays load from the title screen or while a saved replay plays.");
        return 0;
    }
    return 1;
}

int ReplaySessionLoad(const char *path, char *status, size_t size) {
    static ReplayData data;
    ReplayFileImage image;

    if (!ReplayFileRead(path, ReplaySessionLayout(), &image, status, size))
        return 0;
    if (!FromImage(&image, &data, status, size)) {
        ReplayFileFree(&image);
        return 0;
    }
    ReplayFileFree(&image);
    s_loaded = data;
    s_load_pending = 1;
    snprintf(status, size, "Loading the replay.");
    return 1;
}

static const SettingsReplayOps kMenuOps = {
    ReplaySessionCanSave, ReplaySessionSave, ReplaySessionCanLoad,
    ReplaySessionLoad,    ReplaySessionDirectory, ReplaySessionLayout,
};

void ReplaySessionInstall(const char *prefs_path) {
#if RR4_DIAGNOSTICS
    const char *load = getenv("RR4_REPLAY_LOAD");
#endif

    snprintf(s_prefs_path, sizeof(s_prefs_path), "%s", prefs_path ? prefs_path : "");
#if RR4_DIAGNOSTICS
    s_auto_save = getenv("RR4_REPLAY_SAVE");
#endif
    /* The texels transfers write, for SaveVram (host memory only: the
     * emulated machine does not see it). */
    Psyz_GpuSetVramCopy(1);
    ReplayHost = &kHooks;
    SettingsReplaysSetOps(&kMenuOps);
#if RR4_DIAGNOSTICS
    if (load && *load) {
        char status[256];
        if (!ReplaySessionLoad(load, status, sizeof(status)))
            fprintf(stderr, "ridge-racer-type-4-pc: %s: %s\n", load, status);
    }
#endif
}
