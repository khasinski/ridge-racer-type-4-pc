#include "rr/widescreen.h"
#include "rr/draw_distance.h"
#include "rr/rro_model.h"
#include "types.h"
#include "scratchpad.h"
#include "game/gfx/model.h"
#include "game/gfx/poly1.h"
#include "rr/menu_overlay_runtime.h"
#include "rr/overlay668_camera.h"
#include "game/data/globals.h"
#include "game/camera/viewini.h"
#include "rr/relative_file.h"



typedef struct {
    u8 reserved00[0x5E];
    s16 h5E;
    u8 reserved60[0x78 - 0x60];
    s32 w78;
    u8 reserved7C[0x128 - 0x7C];
    s32 w128;
} ViewInitScratch;

extern int CourseFarColorTable[];
extern int ViewUsesCarTrackNode;

void InitViewLighting(void)
{
    int *t;
    int *b;
    int i;
    ColorMatrix = TitleLogoWaveOffsets;
    LightSourceAngles[0] = MenuBgScrollSpeedX;
    LightSourceAngles[1] = MenuBgScrollSpeedY;
    LightSourceAngles[2] = MenuBgTileAnimFrames;
    LightSourceIntensities[0] = 0x1000;
    LightSourceIntensities[1] = 0x1000;
    LightSourceIntensities[2] = 0x1000;
    BackColorR = 0x13;
    BackColorG = 0x1D;
    BackColorB = 0x1C;
    b = CourseFarColorTable;
    i = CourseIndex;
    t = b + i * 3;
    FarColorR = t[0];
    FarColorG = t[1];
    FarColorB = t[2];
}

void InitCourseViewState(void) {
    int i;
    char *p;
    ViewInitScratch *g = (ViewInitScratch *)SCRATCHPAD;
    InitViewLighting();
    ViewUsesCarTrackNode = 1;
    DefaultSegmentVisibilityMask = 0x1400;
    g->w78 = 0;
    g->h5E = 0;
    if (CourseIndex == 7) {
        for (i = 0; i < 5; i++) {
            SetAnimatedTrackObjectState(i, 6, 0, 0);
        }
    }
    i = 0x1F;
    p = (char *)&ExtraCourseViewTable + i;
    do {
        *p-- = 0;
        i--;
    } while (i >= 0);
}

/* Widescreen: also the records of the neighbouring octant's list
 * (GetWideNeighbourViewTable) that the list does not have, after its own,
 * as UpdateVisibleViewRecords adds the extra records. */
static void AddWideNeighbourViews(int player) {
    static u8 listed[0x10000 / 8];
    const CourseViewIndices *own = TrackCourseViewIndices;
    const CourseViewIndices *more = GetWideNeighbourViewTable(player);
    u32 count = PrimaryVisibleViews.objects.count;
    u32 i;

    for (i = 0; i < count; i++)
        listed[own->indices[i] >> 3] |= (u8)(1u << (own->indices[i] & 7));
    for (i = 0; i < more->count && i < 0x100 && count < COURSE_ORIGINAL_VISIBLE_CAPACITY; i++) {
        u16 index = more->indices[i];
        if (listed[index >> 3] & (1u << (index & 7)))
            continue;
        listed[index >> 3] |= (u8)(1u << (index & 7));
        PrimaryVisibleViews.objects.records[count++] = &RelFileViewRecords[index];
    }
    for (i = 0; i < count; i++) {
        size_t index = (size_t)(PrimaryVisibleViews.objects.records[i] - RelFileViewRecords);
        listed[index >> 3] = 0;
    }
    PrimaryVisibleViews.objects.count = count;
}

/* The retail lists stop at the PS1 horizon. Collect the same viewing
 * octant from the rest of the loaded course; normal facing, viewport and
 * depth tests in SubmitCourseHorizon discard geometry outside the extended
 * frustum. Keep the original list first (the mirror uses its prefix), and
 * add each record once even when many segments share it. */
static void AddExtendedViews(void) {
    static u8 listed[0x10000 / 8];
    const ProjectionView32 *view = &((RroSceneScratchPrefix *)SCRATCHPAD)->view.projection.pose08;
    int octant = YawToOctant(view);
    int neighbour = octant;
    u32 count = PrimaryVisibleViews.objects.count;

    if (WideMarginX) {
        int offset = (((view->angles10.xyz.y & 0xFFF) + 0x100) & 0x1FF) - 0x100;
        neighbour = (octant + (offset < 0 ? 7 : 1)) & 7;
    }
    for (u32 i = 0; i < count; i++) {
        size_t index = (size_t)(PrimaryVisibleViews.objects.records[i] - RelFileViewRecords);
        listed[index >> 3] |= (u8)(1u << (index & 7));
    }
    for (int segment = 0; segment < RelFileSegmentCount; segment++) {
        for (int side = 0; side < (WideMarginX ? 2 : 1); side++) {
            const CourseViewIndices *more = (const CourseViewIndices *)RelFileSegmentView(
                segment * 8 + (side ? neighbour : octant));
            for (u32 i = 0; i < more->count && count < COURSE_VISIBLE_CAPACITY; i++) {
                u16 index = more->indices[i];
                if (listed[index >> 3] & (1u << (index & 7)))
                    continue;
                listed[index >> 3] |= (u8)(1u << (index & 7));
                PrimaryVisibleViews.objects.records[count++] = &RelFileViewRecords[index];
            }
        }
    }
    for (u32 i = 0; i < count; i++) {
        size_t index = (size_t)(PrimaryVisibleViews.objects.records[i] - RelFileViewRecords);
        listed[index >> 3] = 0;
    }
    PrimaryVisibleViews.objects.count = count;
}

void BuildViewSegmentObjectList(int player) {
    /* The serialized cursor passes its word count before halfword indices. */
    u32 *source;
    unsigned int i;
    ActiveVisibleViews = &PrimaryVisibleViews;
    source = (u32 *)GetSegmentViewTable(player);
    TrackCourseViewIndices = (CourseViewIndices *)source;
    PrimaryVisibleViews.stream[0].count = *source;
    if (PrimaryVisibleViews.stream[0].count >= 0x100) {
        PrimaryVisibleViews.stream[0].count = COURSE_ORIGINAL_VISIBLE_CAPACITY;
    }
    source++;
    i = 0;
    if (PrimaryVisibleViews.stream[0].count != 0) {
        do {
            PrimaryVisibleViews.stream[i+1].record = &RelFileViewRecords[((u16 *)source)[i]];
            i++;
        } while (i < PrimaryVisibleViews.stream[0].count);
    }
    if (WideMarginX)
        AddWideNeighbourViews(player);
    if (ExtendedDrawDistance)
        AddExtendedViews();
}

void UpdateVisibleViewRecords(void) {
    /* This cursor changes from the counted owner to its pointer-array tail. */
    union {VisibleCourseViews *list;CourseViewRecord **next;} cursor;
    CourseViewRecord *record;
    int extra;
    unsigned int i;
    unsigned int count;

    cursor.list = ActiveVisibleViews;
    count = cursor.list->objects.count;
    cursor.next = cursor.list->objects.records + count;
    for (extra = 0; extra < ExtraCourseViewTable.count; extra++) {
        record = &RelFileViewRecords[ExtraCourseViewTable.indices[extra]];
        for (i = 0; i < count; i++) {
            if (record == ActiveVisibleViews->objects.records[i]) {
                break;
            }
        }
        if (i == count && count < COURSE_VISIBLE_CAPACITY) {
            *cursor.next = record;
            cursor.next++;
            count++;
        }
    }
    ActiveVisibleViews->objects.count = count;
}

void CourseViewInitNoop(void) {
}

void ResetViewScratchMask(void) {
    ((ViewInitScratch *)SCRATCHPAD)->w128 = 0x1F;
}

/* Initial values of the globals this module owns. */
MatrixWords32 TitleLogoWaveOffsets = {.w = {69731368, 99288104, 99288555, 101975572, 1556}};
Vec3sFields8 MenuBgScrollSpeedX = {.x = 682};
Vec3sFields8 MenuBgScrollSpeedY = {.x = 682, .y = 1365};
Vec3sFields8 MenuBgTileAnimFrames = {.x = 3900, .y = 3122};
int CourseFarColorTable[27] = {50, 60, 70, 55, 57, 91, 10, 10, 5, 40, 20, 15, 30, 30, 30, 30, 30, 30, 22, 27, 22, 70, 70, 60};
int DefaultSegmentVisibilityMask;
u16 LightSourceIntensities[3];
CourseViewIndices *TrackCourseViewIndices;
CourseViewRecord *RelFileViewRecords;
Vec3sFields8 LightSourceAngles[3];
AdditionalCourseViews ExtraCourseViewTable;
VisibleCourseViews PrimaryVisibleViews;
VisibleCourseViews *ActiveVisibleViews;
