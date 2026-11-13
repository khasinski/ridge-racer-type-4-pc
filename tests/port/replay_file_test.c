/* Replay files (src/port/replay_file.c) without game data: a round trip
 * through a file in the replays folder, the listing, and the files a build
 * must refuse (another format version, another layout, damage, truncation,
 * foreign files). The working directory is argv[1] (created). */

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "replay_file.h"

static int failures;

#define CHECK(cond)                                                                \
    do {                                                                           \
        if (!(cond)) {                                                             \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                            \
        }                                                                          \
    } while (0)

enum { LAYOUT = 0x12345678u };

static ReplayFileInfo Info(void) {
    ReplayFileInfo info;
    memset(&info, 0, sizeof(info));
    info.layout = LAYOUT;
    info.mode = 0;
    info.course = 3;
    info.reverse = 1;
    info.car = 2 << 8 | 1;
    info.cars = 1;
    info.race_time = 83456;
    info.frames = 2957;
    info.saved_at = 1794600000;
    snprintf(info.title, sizeof(info.title), "Time Attack, course 4 reverse");
    return info;
}

static void WriteBytes(const char *path, const unsigned char *data, size_t size) {
    FILE *f = fopen(path, "wb");
    CHECK(f != NULL);
    if (f) {
        CHECK(fwrite(data, 1, size, f) == size);
        fclose(f);
    }
}

static void TestRoundTrip(const char *dir) {
    unsigned char storage[3000], small[4] = {1, 2, 3, 4};
    ReplayBlock blocks[3];
    ReplayFileInfo info = Info();
    ReplayFileImage image;
    char name[256], path[1024], err[256];
    const ReplayBlock *b;

    for (size_t i = 0; i < sizeof(storage); i++)
        storage[i] = (unsigned char)(i * 7);
    memset(blocks, 0, sizeof(blocks));
    snprintf(blocks[0].name, sizeof(blocks[0].name), "ReplayStorage");
    blocks[0].data = storage;
    blocks[0].size = sizeof(storage);
    snprintf(blocks[1].name, sizeof(blocks[1].name), "rand");
    blocks[1].data = small;
    blocks[1].size = sizeof(small);
    snprintf(blocks[2].name, sizeof(blocks[2].name), "Empty");
    blocks[2].size = 0;

    ReplayFileName(&info, name, sizeof(name));
    CHECK(!strncmp(name, "TA_course4R_car2-1_1-23-456_", 28));
    CHECK(strstr(name, REPLAY_FILE_EXTENSION) != NULL);
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    CHECK(ReplayFileWrite(path, &info, blocks, 3, err, sizeof(err)));

    CHECK(ReplayFileRead(path, LAYOUT, &image, err, sizeof(err)));
    CHECK(image.count == 3);
    CHECK(image.info.version == REPLAY_FILE_VERSION && image.info.course == 3 && image.info.reverse == 1);
    CHECK(image.info.race_time == 83456 && image.info.frames == 2957 && image.info.saved_at == 1794600000);
    CHECK(!strcmp(image.info.title, "Time Attack, course 4 reverse"));
    b = ReplayFileFind(&image, "ReplayStorage");
    CHECK(b && b->size == sizeof(storage) && !memcmp(b->data, storage, sizeof(storage)));
    b = ReplayFileFind(&image, "Empty");
    CHECK(b && b->size == 0);
    CHECK(ReplayFileFind(&image, "missing") == NULL);
    ReplayFileFree(&image);

    /* Another build's layout is refused cleanly, header first. */
    CHECK(!ReplayFileRead(path, LAYOUT + 1, &image, err, sizeof(err)));
    CHECK(strstr(err, "different version") != NULL);
}

static void TestRefusals(const char *dir) {
    unsigned char payload[64] = {0};
    ReplayBlock block;
    ReplayFileInfo info = Info();
    ReplayFileImage image;
    unsigned char *data, copy[REPLAY_HEADER_SIZE + 128];
    size_t size = 0;
    char err[256], path[1024];
    uint32_t crc;

    memset(&block, 0, sizeof(block));
    snprintf(block.name, sizeof(block.name), "Block");
    block.data = payload;
    block.size = sizeof(payload);
    data = ReplayFileEncode(&info, &block, 1, &size);
    CHECK(data && size <= sizeof(copy));
    if (!data || size > sizeof(copy))
        return;
    CHECK(ReplayFileDecode(data, size, LAYOUT, &image, err, sizeof(err)));

    /* A damaged payload byte. */
    memcpy(copy, data, size);
    copy[size - 1] ^= 1;
    CHECK(!ReplayFileDecode(copy, size, LAYOUT, &image, err, sizeof(err)));
    CHECK(strstr(err, "data checksum") != NULL);
    /* A damaged header byte. */
    memcpy(copy, data, size);
    copy[40] ^= 1;
    CHECK(!ReplayFileDecode(copy, size, LAYOUT, &image, err, sizeof(err)));
    CHECK(strstr(err, "header checksum") != NULL);
    /* Truncated. */
    CHECK(!ReplayFileDecode(data, size - 3, LAYOUT, &image, err, sizeof(err)));
    CHECK(!ReplayFileDecode(data, 20, LAYOUT, &image, err, sizeof(err)));
    /* A newer format version (with a valid header checksum). */
    memcpy(copy, data, size);
    copy[8] = REPLAY_FILE_VERSION + 1;
    crc = ReplayCrc32(copy, REPLAY_HEADER_SIZE - 4, 0);
    for (int i = 0; i < 4; i++)
        copy[REPLAY_HEADER_SIZE - 4 + i] = (unsigned char)(crc >> (8 * i));
    CHECK(!ReplayFileDecode(copy, size, LAYOUT, &image, err, sizeof(err)));
    CHECK(strstr(err, "format 2 is not supported") != NULL);
    snprintf(path, sizeof(path), "%s/newer" REPLAY_FILE_EXTENSION, dir);
    WriteBytes(path, copy, size);
    /* Not a replay at all. */
    CHECK(!ReplayFileDecode((const unsigned char *)"hello", 5, LAYOUT, &image, err, sizeof(err)));
    snprintf(path, sizeof(path), "%s/other.txt", dir);
    WriteBytes(path, (const unsigned char *)"not listed", 10);
    /* Invalid blocks are not encoded. */
    snprintf(block.name, sizeof(block.name), "%s", "");
    CHECK(ReplayFileEncode(&info, &block, 1, &size) == NULL);
    free(data);
}

static void TestList(const char *dir) {
    ReplayListEntry list[8];
    int n = ReplayFileList(dir, LAYOUT, list, 8);
    int ok = 0, bad = 0;

    CHECK(n == 2);
    for (int i = 0; i < n; i++) {
        if (list[i].ok) {
            ok++;
            CHECK(list[i].info.course == 3 && strstr(list[i].label, "1:23.456") != NULL);
        } else {
            bad++;
            CHECK(strstr(list[i].label, "newer") != NULL);
        }
    }
    CHECK(ok == 1 && bad == 1);
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : "replay-file-test";
    char replays[1024];

    SDL_CreateDirectory(dir);
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "RR4_REPLAY_DIR", "", true);
    snprintf(replays, sizeof(replays), "%s/prefs.toml", dir);
    CHECK(ReplayDirectory(replays, replays, sizeof(replays)));
    CHECK(strstr(replays, "/replays") != NULL);
    TestRoundTrip(replays);
    TestRefusals(replays);
    TestList(replays);
    if (failures) {
        fprintf(stderr, "replay_file_test: %d failures\n", failures);
        return 1;
    }
    printf("replay_file_test: ok\n");
    return 0;
}
