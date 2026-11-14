/* Exercise the original course-list builder with synthetic on-disc lists:
 * extension, widescreen, shared records, the largest u16 index and resets.
 * No disc or video device is needed. */
#include <stdio.h>
#include <stddef.h>
#include "rr/relative_file.h"
#include "rr/draw_distance.h"
#include "rr/rro_model.h"
#include "rr/player_cars.h"
#include "rr/widescreen.h"
#include "game/camera/viewini.h"
#include "scratchpad.h"

extern int ViewUsesCarTrackNode;
static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); failures++; } } while (0)

static struct {
    RelativeHeader header;
    u32 slots[16];
    struct { u32 count; u16 indices[4]; } lists[16];
} archive;
static CourseViewRecord records[65536];

static void Expect(const u16 *indices, u32 count) {
    CHECK(PrimaryVisibleViews.objects.count == count);
    for (u32 i = 0; i < count; i++)
        CHECK(PrimaryVisibleViews.objects.records[i] == &records[indices[i]]);
}

int main(void) {
    const u16 original[] = {1, 2}, wide[] = {1, 2, 4};
    const u16 extended[] = {1, 2, 3, 65535}, both[] = {1, 2, 4, 3, 65535, 5};
    TrackRelFileHeaderPtr = &archive.header;
    RelFileSegmentCount = 2;
    RelFileSegmentViewSlots = archive.slots;
    RelFileViewRecords = records;
    for (int i = 0; i < 16; i++)
        archive.slots[i] = (u32)((u8 *)&archive.lists[i] - (u8 *)&archive);
    archive.lists[0].count = 2;
    archive.lists[0].indices[0] = 1;
    archive.lists[0].indices[1] = 2;
    archive.lists[8].count = 3;
    archive.lists[8].indices[0] = 2;
    archive.lists[8].indices[1] = 3;
    archive.lists[8].indices[2] = 65535;
    archive.lists[1].count = archive.lists[9].count = 1;
    archive.lists[1].indices[0] = 4;
    archive.lists[9].indices[0] = 5;
    ViewUsesCarTrackNode = 1;
    PlayerCars[0].trackNodeF0 = 0;
    ExtendedDrawDistance = WideMarginX = 0;
    BuildViewSegmentObjectList(0);
    Expect(original, 2);
    WideMarginX = 68;
    BuildViewSegmentObjectList(0);
    Expect(wide, 3);
    ExtendedDrawDistance = 1;
    WideMarginX = 0;
    BuildViewSegmentObjectList(0);
    Expect(extended, 4);
    WideMarginX = 68;
    BuildViewSegmentObjectList(0);
    Expect(both, 6);
    /* Rebuilding clears the deduplication bits, including index 65535. */
    BuildViewSegmentObjectList(0);
    Expect(both, 6);
    ExtraCourseViewTable.count = 3;
    ExtraCourseViewTable.indices[0] = 3;
    ExtraCourseViewTable.indices[1] = ExtraCourseViewTable.indices[2] = 6;
    UpdateVisibleViewRecords();
    CHECK(PrimaryVisibleViews.objects.count == 7);
    CHECK(PrimaryVisibleViews.objects.records[6] == &records[6]);
    ApplyCourseDrawDistance();
    CHECK(((RroSceneScratchPrefix *)SCRATCHPAD)->depthShift60 == 6);
    ExtendedDrawDistance = WideMarginX = 0;
    ApplyCourseDrawDistance();
    CHECK(((RroSceneScratchPrefix *)SCRATCHPAD)->depthShift60 == 5);
    BuildViewSegmentObjectList(0);
    Expect(original, 2);
    return failures ? 1 : 0;
}
