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
#include "game/camera/viewcam.h"
#include "rr/projection_state.h"
#include "rr/matrix_workspace.h"
#include "game/gfx/poly1.h"
#include "game/data/globals.h"
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
    /* The far depth (scratch 0x5C) past which course tiles lose their
     * shading comes from the segment table, or the default for a negative
     * entry; the extended draw distance keeps every tile in full detail. */
    {
        static short farDepths[16] = {2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000,
                                      -1, -1, -1, -1, -1, -1, -1, -1};
        short *farDepth = &((short *)SCRATCHPAD)[0x2E];
        SetSegmentVisibilityTable(farDepths);
        DefaultSegmentVisibilityMask = 0x1400;
        ApplySegmentVisibilityMask();
        CHECK(*farDepth == 2000);
        farDepths[0] = farDepths[1] = farDepths[2] = farDepths[3] = -1;
        farDepths[4] = farDepths[5] = farDepths[6] = farDepths[7] = -1;
        ApplySegmentVisibilityMask();
        CHECK(*farDepth == 0x1400);
        ExtendedDrawDistance = 1;
        ApplySegmentVisibilityMask();
        CHECK(*farDepth == 0x7FFF);
        ExtendedDrawDistance = 0;
    }
    /* Object transforms: within 32767 units of the camera the full-word
     * variant is the original's; past it the original's 16-bit difference
     * wraps (a car 40000 units ahead drawn 25536 behind) and the full-word
     * one does not. */
    {
        ProjectionScratchPrefix *view = (ProjectionScratchPrefix *)SCRATCHPAD;
        MatrixWords32 *m = (MatrixWords32 *)SCRATCH_ROT_MATRIX;
        ObjectTransform38 a, b;
        MatrixWords32 rotation = {{0}};
        Vector near = {0x12345 + 1000, 0x200 - 300, 0x54321 - 32767, 0};
        Vector far = {0x12345 + 40000, 0x200, 0x54321 - 50000, 0};
        *m = rotation;
        m->fields.rotation00[0][0] = m->fields.rotation00[1][1] = 0x4000;
        m->fields.rotation00[2][2] = 0x4000;
        m->fields.rotation00[0][2] = 0x0800;
        rotation.fields.rotation00[0][0] = rotation.fields.rotation00[1][1] = 0x1000;
        rotation.fields.rotation00[2][2] = 0x1000;
        view->pose08.position00.words[0] = 0x12345;
        view->pose08.position00.words[1] = 0x200;
        view->pose08.position00.words[2] = 0x54321;
        SetGteObjectTransform(&a, (const s16 *)&near, rotation.w);
        SetGteObjectTransformWide(&b, &near, rotation.w);
        CHECK(a.fields.matrix18.fields.translation14[0] == b.fields.matrix18.fields.translation14[0]);
        CHECK(a.fields.matrix18.fields.translation14[1] == b.fields.matrix18.fields.translation14[1]);
        CHECK(a.fields.matrix18.fields.translation14[2] == b.fields.matrix18.fields.translation14[2]);
        CHECK(b.fields.matrix18.fields.translation14[2] == -32767 * 4);
        SetGteObjectTransformWide(&b, &far, rotation.w);
        CHECK(b.fields.matrix18.fields.translation14[0] == 40000 * 4 + (-50000 / 2));
        CHECK(b.fields.matrix18.fields.translation14[1] == 0);
        CHECK(b.fields.matrix18.fields.translation14[2] == -50000 * 4);
        CHECK(b.fields.transformed08.z == -50000 * 4);
    }
    return failures ? 1 : 0;
}
