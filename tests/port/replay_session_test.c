/* Compile the real session implementation so malformed private blocks can be
 * tested without exposing their representation as a production API. */
#include "../../src/port/replay_session.c"
#include <limits.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

static ReplayData source, decoded;
static CarModelHeader model;
static unsigned char vertices[16][24];
static RroSpark sparks[CAR_SPARKS];

static void setup_value(const char *name, const void *value) {
    for (unsigned i = 0; i < SETUP_COUNT; ++i)
        if (!strcmp(kSetup[i].name, name)) {
            memcpy(source.setup[i], value, kSetup[i].size);
            return;
        }
    abort();
}

static void storage_fixture(int mode, int cars) {
    s16 game_mode = mode, car_count = cars;
    setup_value("GameModeType", &game_mode);
    setup_value("NumCars", &car_count);
    source.info.mode = mode;
    source.info.cars = cars;
    source.replay_length = mode == 2 || mode == 3 ? 2700 : 3600;
    for (int i = 0; i < SNAPSHOTS_MAX; ++i) {
        source.snapshot_offset[i] = i < cars ?
            (s32)((mode == 2 || mode == 3 ? offsetof(ReplayStorage, two.snapshots) :
                   offsetof(ReplayStorage, single.snapshots)) + i * sizeof(RaceVehicle)) : -1;
        source.snapshot_links[i] = 0;
        if (i < cars) {
            int players = mode == 2 || mode == 3 ? 2 : 1;
            RaceVehicle *v = mode == 2 || mode == 3 ?
                &source.storage.two.snapshots[i] : &source.storage.single.snapshots[i];
            memset(v, 0, sizeof(*v));
            if (i < players) {
                v->flags1E4 = 0x110 + i;
                v->control.playerFlags24 = i;
                v->control.previewSample84.halves.high = 1;
                v->control.dynamics94.steering.count22 = 1;
                source.snapshot_links[i] = LINK_RECORD | LINK_HANDLING;
            } else {
                v->flags1E4 = i - players + 1;
                v->control.gearCountFC = 6;
                v->control.span11A = v->control.divisor11C = 1;
            }
        }
    }
}

static void fixture(void) {
    memset(&source, 0, sizeof(source));
    memset(CarSlotBuffers, 0, sizeof(CarSlotBuffers));
    model.primary10.vertexCount = model.history34.vertexCount = 3;
    CarSlotBufferC8 *e = &CarSlotBuffers[0];
    e->model = &model;
    e->mainCount0C = 4; e->mainIndex10 = 3;
    e->secondCount34 = 4; e->secondIndex38 = 3;
    e->historyCount68 = 8; e->historyIndex6C = 7;
    e->phase5C = 2;
    e->sparksC4 = sparks;
    for (unsigned i = 0; i < 16; ++i)
        memset(vertices[i], i + 1, sizeof(vertices[i]));
    memset(sparks, 0x34, sizeof(sparks));
    for (unsigned i = 0; i < 4; ++i) {
        e->main14[i].vertices = vertices[i];
        e->second3C[i].vertices = vertices[4 + i];
    }
    for (unsigned i = 0; i < 8; ++i)
        e->history84[i].vertices = vertices[8 + i];
    SaveTrails(&source);
    source.info.ntsc = VideoTimingNtsc ? REPLAY_TIMING_NTSC : REPLAY_TIMING_PAL;
    storage_fixture(0, 1);
    s32 laps = 3;
    setup_value("NumLaps", &laps);
}

static int accepted(void) {
    ReplayFileImage image = {0};
    char error[256];
    image.info = source.info;
    image.count = ToBlocks(&source, image.blocks);
    return FromImage(&image, &decoded, error, sizeof(error));
}

static int encoded_accepted(void) {
    ReplayBlock blocks[REPLAY_BLOCKS_MAX];
    ReplayFileImage image = {0};
    char error[256];
    size_t size;
    int count = ToBlocks(&source, blocks);
    unsigned char *bytes = ReplayFileEncode(&source.info, blocks, count, &size);
    if (!bytes || !ReplayFileDecode(bytes, size, source.info.layout, &image, error, sizeof(error)))
        abort();
    int valid = FromImage(&image, &decoded, error, sizeof(error));
    free(bytes);
    return valid;
}

static int unchanged_on_rejection(void) {
    CarSlotBufferC8 before[CAR_SLOT_ENTRIES];
    unsigned char previous_vertices[sizeof(vertices)], previous_sparks[sizeof(sparks)];
    memcpy(before, CarSlotBuffers, sizeof(before));
    memcpy(previous_vertices, vertices, sizeof(vertices));
    memcpy(previous_sparks, sparks, sizeof(sparks));
    CHECK(!accepted());
    ApplyTrails(&source);
    CHECK(!memcmp(before, CarSlotBuffers, sizeof(before)));
    CHECK(!memcmp(previous_vertices, vertices, sizeof(vertices)));
    CHECK(!memcmp(previous_sparks, sparks, sizeof(sparks)));
    return 0;
}

static int storage_bounds(void) {
    fixture();
    CHECK(accepted());
    CHECK(encoded_accepted());
    source.snapshot_offset[0] = 0;
    CHECK(!encoded_accepted()); /* Both CRCs are valid for this bad state. */
    /* These offsets fit the storage union, but name inputs, misaligned data,
     * another car, or missing live snapshots rather than this car's slot. */
    const s32 bad_offsets[] = {0, 1, -1, -2, INT32_MAX,
        offsetof(ReplayStorage, single.snapshots) + 1,
        offsetof(ReplayStorage, single.snapshots) + sizeof(RaceVehicle),
        offsetof(ReplayStorage, two.snapshots)};
    for (unsigned i = 0; i < sizeof(bad_offsets) / sizeof(bad_offsets[0]); ++i) {
        fixture();
        source.snapshot_offset[0] = bad_offsets[i];
        CHECK(!accepted());
    }
    fixture();
    source.snapshot_offset[1] = source.snapshot_offset[0];
    CHECK(!accepted());
    fixture(); source.snapshot_links[0] = 4; CHECK(!accepted());
    fixture(); source.snapshot_links[1] = 1; CHECK(!accepted());
    fixture();
    source.storage.single.snapshots[0].control.record18 = (void *)&model;
    CHECK(!accepted());
    fixture();
    source.storage.single.snapshots[0].control.handling1C = (void *)&model;
    CHECK(!accepted());
    const int modes[] = {0, 1, 2, 3, 6};
    const int counts[] = {1, 8, 2, 4, 2};
    for (unsigned i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        fixture(); storage_fixture(modes[i], counts[i]);
        CHECK(accepted());
        source.info.frames = source.replay_length;
        CHECK(accepted());
        ++source.info.frames; CHECK(!accepted());
        source.info.frames = 0;
        ++source.replay_length; CHECK(!accepted());
        source.replay_length = -1; CHECK(!accepted());
        source.replay_length = 0; CHECK(!accepted());
        fixture(); storage_fixture(modes[i], counts[i]);
        s16 wrong_mode = 4; setup_value("GameModeType", &wrong_mode);
        CHECK(!accepted());
        source.info.mode = 4; CHECK(!accepted());
        fixture(); storage_fixture(modes[i], counts[i] + 1); CHECK(!accepted());
    }
    fixture(); storage_fixture(3, 2); CHECK(accepted());
    fixture(); storage_fixture(3, 3); CHECK(accepted());
    fixture(); storage_fixture(3, 1); CHECK(!accepted());
    fixture(); source.info.mode = 1; CHECK(!accepted());
    fixture(); source.info.cars = 8; CHECK(!accepted());
    fixture(); source.info.course = 1; CHECK(!accepted());
    fixture(); source.info.reverse = 1; CHECK(!accepted());
    CHECK(!encoded_accepted());
    for (s16 invalid = -1; invalid <= 8; invalid += 9) {
        fixture(); setup_value("CourseIndex", &invalid); source.info.course = invalid;
        CHECK(!accepted());
    }
    for (s16 invalid = -1; invalid <= 2; invalid += 3) {
        fixture(); setup_value("ReverseCourse", &invalid); source.info.reverse = invalid;
        CHECK(!accepted());
    }
    /* The capture path must reject malformed live pointers before casting
     * and touching a snapshot. No GPU or disc is needed for this refusal. */
    GameModeType = 0; NumCars = 1;
    CarSnapshots[0] = (RaceVehicle *)(SharedWorkArena + offsetof(ReplayStorage, single.snapshots) + 1);
    CHECK(!Capture(&decoded));
    CarSnapshots[0] = (RaceVehicle *)SharedWorkArena;
    CHECK(!Capture(&decoded));
    CarSnapshots[0] = NULL;
    return 0;
}

static int asset_bounds(void) {
    MenuGridPoint choices[4] = {0};
    CurrentSelection selections = {0};
    for (int slot = 0; slot < 4; ++slot) {
        fixture();
        choices[slot].first = 17;
        setup_value("PlayerCourseChoices", choices);
        CHECK(!encoded_accepted());
        choices[slot].first = -1;
        setup_value("PlayerCourseChoices", choices); CHECK(!accepted());
        choices[slot].first = 16;
        setup_value("PlayerCourseChoices", choices); CHECK(accepted());
        choices[slot].first = 0;
        choices[slot].second = 27;
        setup_value("PlayerCourseChoices", choices); CHECK(!accepted());
        choices[slot].second = -1;
        setup_value("PlayerCourseChoices", choices); CHECK(!accepted());
        choices[slot].second = 26;
        setup_value("PlayerCourseChoices", choices); CHECK(accepted());
        choices[slot].second = 0;
        setup_value("PlayerCourseChoices", choices);
        selections.entries[slot].selection = 4;
        setup_value("SelectionRows", &selections); CHECK(!accepted());
        selections.entries[slot].selection = 3;
        selections.entries[slot].variant = 4;
        setup_value("SelectionRows", &selections); CHECK(!accepted());
        selections.entries[slot].variant = 3;
        selections.entries[slot].parameter = 255; /* Loader takes modulo 20. */
        setup_value("SelectionRows", &selections); CHECK(accepted());
        memset(&selections, 0, sizeof(selections));
    }
    for (s16 value = -1; value <= 4; value += 5) {
        fixture(); setup_value("SelectedTeamIndex", &value); CHECK(!accepted());
        fixture(); setup_value("SelectedMakerIndex", &value); CHECK(!accepted());
    }
    for (s32 laps = 1; laps <= 10; ++laps) {
        fixture(); storage_fixture(2, 2); setup_value("NumLaps", &laps);
        CHECK(accepted() == (laps >= 2 && laps < 10));
    }
    SavedMenuSettings state = {0};
    for (int phase = -1; phase <= 8; ++phase) {
        fixture(); storage_fixture(1, 8);
        state.fields.phase17 = phase;
        setup_value("SelectionState", &state);
        CHECK(accepted() == (phase >= 0 && phase < 8));
    }
    fixture(); storage_fixture(1, 8);
    choices[0].second = 2;
    setup_value("PlayerCourseChoices", choices);
    CHECK(!accepted()); /* GP does not load the preset TIM bank. */
    memset(choices, 0, sizeof(choices));

    CustomEmblemImages1080 emblems = {0};
    for (int i = 0; i < 2; ++i) {
        EmblemTimLayout840 *tim = &emblems.images[i].layout;
        tim->header[0] = 16; tim->header[1] = 8; tim->header[2] = 44;
        tim->header[3] = UINT32_MAX; /* UploadCarClut replaces the origin. */
        tim->header[4] = 0x10010;
        tim->imageHeader[0] = 2060; tim->imageHeader[1] = UINT32_MAX;
        tim->imageHeader[2] = 0x400010;
    }
    const unsigned words[] = {0, 1, 2, 4, 13, 15};
    for (int slot = 0; slot < 4; ++slot) {
        fixture(); choices[slot].second = 1;
        setup_value("PlayerCourseChoices", choices);
        setup_value("CustomEmblemBuffers", &emblems); CHECK(encoded_accepted());
        for (unsigned j = 0; j < sizeof(words) / sizeof(words[0]); ++j) {
            u32 *word = &emblems.images[slot & 1].words[words[j]], before = *word;
            *word = UINT32_MAX;
            setup_value("CustomEmblemBuffers", &emblems); CHECK(!encoded_accepted());
            *word = 0;
            setup_value("CustomEmblemBuffers", &emblems); CHECK(!accepted());
            *word = before;
        }
        choices[slot].second = 0;
    }
    fixture(); /* Unselected custom TIMs may be empty/stale. */
    CHECK(accepted());
    return 0;
}

static int vehicle_bounds(void) {
    fixture(); storage_fixture(1, 8);
    RaceVehicle *ai = &source.storage.single.snapshots[1];
    ai->control.gear68 = 6;
    CHECK(!encoded_accepted());
    const s16 gears[] = {-1, 6, INT16_MAX};
    for (unsigned i = 0; i < sizeof(gears) / sizeof(gears[0]); ++i) {
        fixture(); storage_fixture(1, 8); ai->control.gear68 = gears[i]; CHECK(!accepted());
        fixture(); source.storage.single.snapshots[0].control.previewSample84.halves.high = gears[i] == 6 ? 7 : gears[i];
        CHECK(!accepted());
    }
    const s32 counts[] = {-1, 0, 7, INT32_MAX};
    for (unsigned i = 0; i < sizeof(counts) / sizeof(counts[0]); ++i) {
        fixture(); storage_fixture(1, 8); ai->control.gearCountFC = counts[i]; CHECK(!accepted());
    }
    fixture(); storage_fixture(1, 8); ai->control.shiftDir62 = -1; CHECK(!accepted());
    ai->control.shiftDir62 = 1; ai->control.gear68 = 5; CHECK(!accepted());
    ai->control.gear68 = 4; CHECK(accepted());
    ai->control.shiftDir62 = 2; CHECK(!accepted());
    fixture(); storage_fixture(1, 8); ai->control.span11A = 0; CHECK(!accepted());
    fixture(); storage_fixture(1, 8); ai->control.divisor11C = 0; ai->control.boostTimer44 = 1; CHECK(!accepted());
    ai->control.boostTimer44 = 0; CHECK(accepted());
    for (int i = 0; i < 2; ++i) {
        fixture(); storage_fixture(2, 2);
        RaceVehicle *player = &source.storage.two.snapshots[i];
        player->control.playerFlags24 = 4; CHECK(!encoded_accepted());
        player->control.playerFlags24 = i;
        source.snapshot_links[i] = LINK_RECORD; CHECK(!accepted());
        source.snapshot_links[i] = LINK_RECORD | LINK_HANDLING;
        player->slotIndex1E6 = 8; CHECK(!accepted());
        player->slotIndex1E6 = -1; CHECK(!accepted());
        player->slotIndex1E6 = 7;
        player->control.dynamics94.steering.count22 = -1; CHECK(!accepted());
        player->control.dynamics94.steering.count22 = 11; CHECK(!accepted());
        player->control.dynamics94.steering.count22 = 10; CHECK(accepted());
        player->flags1E4 = 15; CHECK(!accepted());
        player->flags1E4 = 0xFFFF; CHECK(accepted()); /* Lost-track sentinel. */
    }
    return 0;
}

int main(int argc, char **argv) {
    CHECK(!vehicle_bounds());
    CHECK(!asset_bounds());
    CHECK(!storage_bounds());
    fixture();
    u32 at = 16, original = at;
    CHECK(!TrailGet(&source, &at, NULL, UINT32_MAX));
    CHECK(at == original);
    CHECK(!TrailPut(&source, &at, NULL, UINT32_MAX));
    CHECK(at == original);
    at = UINT32_MAX;
    CHECK(!TrailGet(&source, &at, NULL, 1));
    CHECK(!TrailPut(&source, &at, NULL, 1));

    CHECK(source.trails_size > 0 && accepted());
    CarSlotBufferC8 expected = CarSlotBuffers[0];
    memset(vertices, 0, sizeof(vertices));
    memset(sparks, 0, sizeof(sparks));
    CarSlotBuffers[0].mainCount0C = 0;
    ApplyTrails(&source);
    CHECK(!memcmp(&expected, &CarSlotBuffers[0], sizeof(expected)));
    for (unsigned i = 0; i < sizeof(vertices); ++i)
        CHECK(((unsigned char *)vertices)[i] == i / 24 + 1);
    for (unsigned i = 0; i < sizeof(sparks); ++i)
        CHECK(((unsigned char *)sparks)[i] == 0x34);

    const u32 bytes = source.trails_size;
    /* Every truncated prefix must fail before even the first slot changes. */
    for (u32 n = 1; n < bytes; ++n) {
        source.trails_size = n;
        CarSlotBuffers[0].mainCount0C = 0;
        CHECK(!unchanged_on_rejection());
    }
    source.trails_size = bytes + 1;
    CHECK(!unchanged_on_rejection());
    const u32 invalid_headers[][2] = {{0, 2}, {4, UINT32_MAX}, {8, UINT32_MAX}, {12, 2}, {0, 0},
        {4, 25}, {8, 25}, {4, UINT32_MAX - 15}};
    for (unsigned i = 0; i < sizeof(invalid_headers) / sizeof(invalid_headers[0]); ++i) {
        fixture();
        memcpy(source.trails + invalid_headers[i][0], &invalid_headers[i][1], 4);
        CHECK(!unchanged_on_rejection());
    }
    const size_t fields[] = {offsetof(CarSlotBufferC8, mainCount0C), offsetof(CarSlotBufferC8, mainIndex10),
        offsetof(CarSlotBufferC8, secondCount34), offsetof(CarSlotBufferC8, secondIndex38),
        offsetof(CarSlotBufferC8, historyCount68), offsetof(CarSlotBufferC8, historyIndex6C),
        offsetof(CarSlotBufferC8, phase5C)};
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        for (int j = 0; j < 2; ++j) {
            fixture();
            s32 bad = j ? INT32_MAX : -1;
            memcpy(source.trails + 16 + fields[i], &bad, sizeof(bad));
            CHECK(!unchanged_on_rejection());
        }
    }
    for (s32 phase = 0; phase < 2; ++phase) {
        fixture(); /* Eight samples cannot be idle or still accumulating. */
        memcpy(source.trails + 16 + offsetof(CarSlotBufferC8, phase5C), &phase, sizeof(phase));
        CHECK(!unchanged_on_rejection());
    }
    fixture();
    source.trails_size = 0; /* Writer's documented allocation/size fallback. */
    CHECK(accepted());
    ApplyTrails(&source);
    CHECK(!memcmp(&expected, &CarSlotBuffers[0], sizeof(expected)));
    fixture();
    CarSlotBuffers[0].model = NULL; /* A different load cannot accept this slot. */
    CarSlotBufferC8 absent = CarSlotBuffers[0];
    ApplyTrails(&source);
    CHECK(!memcmp(&absent, &CarSlotBuffers[0], sizeof(absent)));
    fixture();
    ++model.primary10.vertexCount;
    CarSlotBuffers[0].mainCount0C = 0;
    CarSlotBufferC8 different = CarSlotBuffers[0];
    ApplyTrails(&source);
    CHECK(!memcmp(&different, &CarSlotBuffers[0], sizeof(different)));
    char title[128];
    source.info.course = INT32_MAX;
    Title(&source.info, title, sizeof(title));
    CHECK(strstr(title, "course 2147483648"));

    /* Optional local game-recorded fixtures; no retail data is needed by CI. */
    for (int i = 1; i < argc; ++i) {
        ReplayFileImage image = {0};
        char error[256];
        CHECK(ReplayFileRead(argv[i], ReplaySessionLayout(), &image, error, sizeof(error)));
        VideoTimingNtsc = image.info.ntsc != 0;
        int valid = FromImage(&image, &decoded, error, sizeof(error));
        if (!valid) fprintf(stderr, "%s: %s\n", argv[i], error);
        CHECK(valid);
        ReplayFileFree(&image);
        printf("Accepted recorded replay: %s\n", argv[i]);
    }
    puts("Replay session trail bounds and atomic restore passed.");
    return 0;
}
