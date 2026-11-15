#include <stdio.h>
#include <limits.h>
#include <string.h>
/* This header used to publish the PS1 address when included first. */
#include "rr/selection_initializer.h"
#include "rr/overlay667_packet.h"
#include "rr/serial_handles.h"
#include "rr/overlay_module.h"
#include "rr/course_choice_types.h"

extern CourseChoice4 ovl661_car_choices[2][2][9];
#include "rr/versus_names.h"
#include "game/data/cdfile2.h"
#include "game/data/cdfile1.h"
#include "rr/release.h"
#include "rr/cd.h"
extern CdFileEntry CdFileTable[RR_R4_ENTRIES_MAX];

extern s32 (*CdFileStateHandlers[12])(void);
extern s32 CdFileIndexCurrent, CdFileState, CdFileSectors, CdFileBytesRead;

void FloodFill4bpp(u8 *pixel, u32 operation);
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

static unsigned colour(const u8 *image, unsigned at) {
    return (image[at / 2] >> ((at & 1) * 4)) & 15;
}

/* Independent breadth-first reference on unpacked pixels. */
static void reference(u8 *image, unsigned start, unsigned replacement) {
    unsigned queue[4096], first = 0, count = 0;
    const unsigned previous = image[start];
    if (previous == replacement) return;
    queue[count++] = start;
    image[start] = replacement;
    while (first < count) {
        unsigned at = queue[first++], near[4], n = 0;
        if (at >= 64) near[n++] = at - 64;
        if (at < 4032) near[n++] = at + 64;
        if (at % 64) near[n++] = at - 1;
        if (at % 64 < 63) near[n++] = at + 1;
        for (unsigned i = 0; i < n; ++i) {
            unsigned next = near[i];
            if (image[next] == previous) {
                image[next] = replacement;
                queue[count++] = next;
            }
        }
    }
}

int main(void) {
    CHECK((void *)RR_PROJECTION_SCRATCH == (void *)RrScratchpad);
    memset(RrScratchpad, 0xA5, RR_SCRATCHPAD_SIZE);
    RR_PROJECTION_SCRATCH->pose08.position00.xyz.x = 12345;
    s32 scratch_word;
    memcpy(&scratch_word, RrScratchpad + 8, sizeof scratch_word);
    CHECK(scratch_word == 12345);
    CHECK(RrScratchpad[7] == 0xA5 && RrScratchpad[12] == 0xA5);
    memset(RrScratchpad, 0, RR_SCRATCHPAD_SIZE);

    CourseChoice4 initial_choices[2][2][9];
    char initial_names[24];
    memcpy(initial_choices, ovl661_car_choices, sizeof initial_choices);
    memcpy(initial_names, ovl661_player_names, sizeof initial_names);
    OverlayModulesInit();
    /* A resident card load can write these before the first versus bind. */
    memset(ovl661_car_choices, 0xA5, sizeof initial_choices);
    memset(ovl661_player_names, 'X', sizeof initial_names);
    OverlayModulesInit(); /* Restart must neither recapture nor reset live state. */
    CHECK(ovl661_player_names[0][0] == 'X');
    CHECK(((u8 *)ovl661_car_choices)[0] == 0xA5);
    CHECK(OverlayModuleBind(661) == 1);
    CHECK(!memcmp(initial_choices, ovl661_car_choices, sizeof initial_choices));
    CHECK(!memcmp(initial_names, ovl661_player_names, sizeof initial_names));
    memset(ovl661_player_names, 'Y', sizeof initial_names);
    CHECK(OverlayModuleBind(671) == 1); /* Duplicate disc entry also resets. */
    CHECK(!memcmp(initial_names, ovl661_player_names, sizeof initial_names));
    CHECK(OverlayModuleBind(664) == 0);

    u32 directory[RR_R4_ENTRIES_MAX + 2];
    CdFileEntry saved_directory[RR_R4_ENTRIES_MAX];
    const u32 count = GameReleaseProfile->r4Entries;
    directory[0] = count;
    for (u32 i = 0; i <= count; ++i) directory[i + 1] = 2 + i * 2;
    const u32 fileBytes = directory[count + 1] * 2048u;
    CHECK(ParseR4Directory(directory, count + 2, fileBytes, 150));
    CHECK(CdFileTable[0].sectors == 2 && CdFileTable[count - 1].sectors == 2);
    memcpy(saved_directory, CdFileTable, sizeof saved_directory);
    CHECK(!ParseR4Directory(NULL, count + 2, fileBytes, 150));
    CHECK(!ParseR4Directory(directory, count + 1, fileBytes, 150));
    CHECK(!ParseR4Directory(directory, count + 2, fileBytes - 2048, 150));
    CHECK(!ParseR4Directory(directory, count + 2, fileBytes, -1));
    CHECK(!ParseR4Directory(directory, count + 2, fileBytes, INT_MAX - 1));
    directory[0] = UINT_MAX;
    CHECK(!ParseR4Directory(directory, count + 2, fileBytes, 150));
    directory[0] = count;
    directory[count] = directory[count + 1] + 1;
    CHECK(!ParseR4Directory(directory, count + 2, fileBytes, 150));
    directory[count] = 2 + (count - 1) * 2;
    directory[1] = 1;
    CHECK(!ParseR4Directory(directory, count + 2, fileBytes, 150));
    CHECK(!memcmp(saved_directory, CdFileTable, sizeof saved_directory));
    RrReleaseSelect(RR_RELEASE_NTSC_J, 1);
    directory[0] = GameReleaseProfile->r4Entries;
    for (u32 i = 0; i <= directory[0]; ++i) directory[i + 1] = 2 + i * 2;
    CHECK(ParseR4Directory(directory, directory[0] + 2,
                          directory[directory[0] + 1] * 2048u, 150));
    RrReleaseSelect(RR_RELEASE_PAL, 0);

    /* Exercise the same function-pointer dispatch used after a disc read. */
    CdFileIndexCurrent = 42;
    CdFileState = 10;
    CdFileSectors = 7;
    CdFileBytesRead = -1;
    CHECK(CdFileStateHandlers[CdFileState]() == 0);
    CHECK(CdFileIndexCurrent == -1 && CdFileState == 0);
    CHECK(CdFileBytesRead == 7 * 2048);
    u8 guarded[2050], expected[4096];
    unsigned random = 123;
    for (unsigned trial = 0; trial < 300; ++trial) {
        memset(guarded, 0xA5, sizeof guarded);
        u8 *image = guarded + 1;
        for (unsigned i = 0; i < 2048; ++i) {
            random = random * 1664525u + 1013904223u;
            image[i] = trial < 4 ? 0 : (u8)(random >> 24);
        }
        for (unsigned i = 0; i < 4096; ++i) expected[i] = colour(image, i);
        unsigned start = trial < 4 ? (unsigned[]){0,63,4032,4095}[trial] : random % 4096;
        unsigned replacement = (trial % 15) + 1;
        unsigned op = start % 64 | ((start / 64) << 8) |
                      (expected[start] << 16) | (replacement << 24);
        reference(expected, start, replacement);
        FloodFill4bpp(image + start / 2, op);
        CHECK(guarded[0] == 0xA5 && guarded[2049] == 0xA5);
        for (unsigned i = 0; i < 4096; ++i) CHECK(colour(image, i) == expected[i]);
    }
    ovl667_sio_read_event = OpenEvent(0xF000000B, 0x400, 0x2000, 0);
    CHECK(ovl667_sio_read_event > 0);
    EnableEvent(ovl667_sio_read_event);
    LinkReadHandle = -1;
    for (unsigned flags = 0; flags < 256; ++flags) {
        memset(&ovl667_rx_packet_ring, 0xA5, sizeof ovl667_rx_packet_ring);
        memset(&ovl667_rx_staging_packet, 0, sizeof ovl667_rx_staging_packet);
        ovl667_rx_staging_packet.packet.flags00 = (u8)flags;
        Overlay667SerialPacket expected_packet = ovl667_rx_staging_packet.packet;
        ovl667_tx_slot_index = 0;
        ovl667_remote_error_code = 0;
        DeliverEvent(0xF000000B, 0x400);
        CHECK(ovl667_poll_race_packet() == 1);
        CHECK(ovl667_rx_slot_index == (flags & 7));
        CHECK(ovl667_active_slot_index < 8 && ovl667_tx_slot_index < 8);
        CHECK(ovl667_remote_error_code == ((flags & 128) ? ((flags & 64) ? 2 : 1) : 0));
        for (unsigned i = 0; i < 8; ++i) {
            u8 untouched[12]; memset(untouched, 0xA5, sizeof untouched);
            CHECK(!memcmp(&ovl667_rx_packet_ring.packets[i],
                          i == (flags & 7) ? (void *)&expected_packet : (void *)untouched, 12));
        }
    }
    CloseEvent(ovl667_sio_read_event);
    return 0;
}
