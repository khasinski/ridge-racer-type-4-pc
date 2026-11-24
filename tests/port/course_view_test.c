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
#include "rr/course_occlusion.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"

extern int ViewUsesCarTrackNode;
static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); failures++; } } while (0)

static struct {
    RelativeHeader header;
    u32 slots[16];
    struct { u32 count; u16 indices[4]; } lists[16];
} archive;
static CourseViewRecord records[65536];

void Psyz_GteDataWrite(unsigned reg, unsigned int value);

/* A quad's projection as the GTE leaves it: corners (x, y) and depths. */
static void Project(s32 sxy[4], const s16 xy[8], const u16 z[4]) {
    for (int i = 0; i < 4; i++) {
        sxy[i] = (s32)(((u32)(u16)xy[2 * i]) | ((u32)(u16)xy[2 * i + 1] << 16));
        Psyz_GteDataWrite(16 + i, z[i]);
    }
}

typedef struct { GpuDmaTag tag; u32 words[9]; } TestPacket;

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
    /* Course occlusion (extended draw distance): an opaque wall tile linked
     * behind the cars (entry 300, the faces at 250) though nearer hides the
     * faces wholly behind it, and a car with faces reaching past its edge
     * (showing over the wall) moves behind the wall's entry whole. A car
     * with a near face is left as it is. A see-through (semi-transparent)
     * wall hides nothing, and nothing changes without the extended view. */
    {
        RroSceneScratchPrefix *scene = (RroSceneScratchPrefix *)SCRATCHPAD;
        static GpuDmaTag table[704];
        static TestPacket arena[16];
        const s16 wall[8] = {40, 40, 200, 40, 40, 200, 200, 200};
        const u16 wallZ[4] = {2000, 2000, 2000, 2000};
        const s16 behind[8] = {80, 80, 120, 80, 80, 120, 120, 120};
        const s16 edge[8] = {180, 80, 230, 80, 180, 120, 230, 120};
        const u16 farZ[4] = {6000, 6000, 6100, 6100}, nearZ[4] = {1500, 1500, 1500, 1500};
        GpuDmaTag *faceEntry = &table[250], *wallEntry = &table[300];
        s32 sxy[4];
        int k;
        WideMarginX = 0;
        scene->viewport6C.rect.x = 0;
        scene->viewport6C.rect.y = 0;
        scene->viewport6C.rect.w = 319;
        scene->viewport6C.rect.h = 255;
        scene->renderFlags64 = 0;
        RR_GPU_SCRATCH->order = table;
        for (int pass = 0; pass < 3; pass++) {
            /* pass 0: opaque flat wall; 1: semi-transparent wall; 2: off */
            ExtendedDrawDistance = pass != 2;
            CourseOcclusionFrame();
            for (k = 0; k < 704; k++) table[k].address = RR_GPU_END_LINK;
            for (k = 0; k < 16; k++) arena[k].tag.length = 5;
            /* the cars come first (main view): one behind and past the wall's
             * edge (a two-packet face, a shadow face, a body face), one near */
            CourseOcclusionCarBegin(scene);
            Project(sxy, behind, farZ);
            RR_GPU_ADD_PRIM(faceEntry, &arena[1], 5);
            RR_GPU_ADD_PRIM(faceEntry, &arena[0], 5);
            CourseOcclusionFace(faceEntry, &arena[0].tag, &arena[1].tag, &arena[1].tag, sxy, 4);
            Project(sxy, edge, farZ);
            RR_GPU_ADD_PRIM(faceEntry, &arena[2], 5);
            CourseOcclusionCarShadow(1);
            CourseOcclusionFace(faceEntry, &arena[2].tag, &arena[2].tag, NULL, sxy, 4);
            CourseOcclusionCarShadow(0);
            Project(sxy, edge, farZ);
            RR_GPU_ADD_PRIM(faceEntry, &arena[5], 5);
            CourseOcclusionFace(faceEntry, &arena[5].tag, &arena[5].tag, NULL, sxy, 4);
            CourseOcclusionCarEnd();
            CourseOcclusionCarBegin(scene);
            Project(sxy, behind, nearZ);
            RR_GPU_ADD_PRIM(faceEntry, &arena[3], 5);
            CourseOcclusionFace(faceEntry, &arena[3].tag, &arena[3].tag, NULL, sxy, 4);
            Project(sxy, behind, farZ);
            RR_GPU_ADD_PRIM(faceEntry, &arena[6], 5);
            CourseOcclusionFace(faceEntry, &arena[6].tag, &arena[6].tag, NULL, sxy, 4);
            CourseOcclusionCarEnd();
            CourseOcclusionCourseBegin(scene);
            Project(sxy, wall, wallZ);
            CourseOcclusionTile(sxy, wallEntry, (const u8 *)&arena[8]);
            arena[8].words[0] = pass == 1 ? 0x2A000000u : 0x28000000u; /* F4 */
            RR_GPU_ADD_PRIM(wallEntry, &arena[8], 5);
            CourseOcclusionCourseEnd((const u8 *)&arena[9]);
            CHECK((arena[0].tag.length == 0) == (pass == 0));
            CHECK((arena[1].tag.length == 0) == (pass == 0));
            CHECK(arena[2].tag.length == 5);
            CHECK(arena[5].tag.length == 5);
            CHECK(arena[3].tag.length == 5);
            CHECK(arena[6].tag.length == 5); /* the near car's: as it is */
            if (pass == 0) {
                /* the car's showing faces head entry 301 in their order */
                CHECK(table[301].address == (uintptr_t)&arena[5]);
                CHECK(arena[5].tag.address == (uintptr_t)&arena[2]);
                CHECK(arena[2].tag.address == RR_GPU_END_LINK);
                CHECK(faceEntry->address == (uintptr_t)&arena[6]);
                CHECK(arena[6].tag.address == (uintptr_t)&arena[3]);
                CHECK(arena[3].tag.address == (uintptr_t)&arena[0]);
                CHECK(arena[1].tag.address == RR_GPU_END_LINK);
            } else {
                CHECK(table[301].address == RR_GPU_END_LINK);
                CHECK(arena[3].tag.address == (uintptr_t)&arena[5]);
            }
            /* the mirror's cars come after its course: done at once */
            if (pass == 0) {
                CourseOcclusionCarBegin(scene);
                Project(sxy, behind, farZ);
                RR_GPU_ADD_PRIM(faceEntry, &arena[4], 5);
                CourseOcclusionFace(faceEntry, &arena[4].tag, &arena[4].tag, NULL, sxy, 4);
                CourseOcclusionCarEnd();
                CHECK(arena[4].tag.length == 0);
            }
        }
        ExtendedDrawDistance = 0;
    }
    return failures ? 1 : 0;
}
