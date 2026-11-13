#include "rr/release.h"
#include "rr/frame_interp.h"
#include "rr/widescreen.h"
#include "psyq/libgte/rotmat.h"
#include "game/gfx/drawlist.h"
#include "game/gfx/tilerect.h"
#include "psyq/libgte/rotmatz.h"
#include "rr/track_object_render.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "scratchpad.h"
#include "rr/load_cursor.h"
#include "rr/camera_matrices.h"
#include "rr/projection_state.h"
#include "game/math/rotmat.h"
#include "rr/renderer_setup.h"
#include "psyq/libgte/gtemat5.h"
#include "rr_asm.h"
#include "game/menu/mnsel.h"
#include "game/gfx/gtemodel.h"
#include "game/gfx/skyrot1.h"
#include "psyq/libgpu/gpuenv.h"
#include "rr/memory.h"
#include "rr/course_choices.h"
#include "rr/menu_grid.h"
#include "rr/work_arena.h"
#include "game/data/screenframe.h"

extern int FlashingLightColorTable[];
extern GpuRectangleFields MenuGridSkyVramRect;
extern VectorWords16 MenuGridDefaultAngles;
extern VectorWords16 MenuGridInitialOffset;

void DrawTrackObjectModel(int a0, int a1);

void DrawTrackObjectLitModel(int placementIndex, int modelIndex)
{
    TrackObjectLitWork *work = &((TrackObjectLitScratch *)SCRATCHPAD)->work7C;
    RroSceneScratchPrefix *scene = (RroSceneScratchPrefix *)SCRATCHPAD;
    const RroEffectMesh *entries = RroScene.modelEntries;
    const TrackObjectPlacement *p;
    int remaining, depthLimit, depthShift;
    u8 *packetCursor;
    GpuDmaTag *orderingBase;

    RroScene.workCommands = entries[modelIndex].commands;
    RroScene.workVertices = entries[modelIndex].vertices;
    work->rotation00.w[0] = 0x1000;
    work->rotation00.w[1] = 0;
    work->rotation00.w[2] = 0x1000;
    work->rotation00.w[3] = 0;
    work->rotation00.w[4] = 0x1000;
    RotMatrixZ(scene->view.projection.pose08.angles10.xyz.z, &work->rotation00);
    RR_GTE_SETROT(&work->rotation00);
    RR_GTE_SETLLM(CameraViewMatrix.w);
    RR_GTE_SETBK_P(CameraViewMatrix.fields.translation14);

    /* The placement stream: a command/count header word, then the records. */
    {
        const u8 *placements = (const u8 *)entries[placementIndex].commands;
        remaining = ((const u16 *)placements)[1];
        p = (const TrackObjectPlacement *)(placements + 4);
    }
    depthLimit = scene->renderFlags64 ? 0x95 : 0x12A;
    packetCursor = RR_GPU_SCRATCH->cursor;
    orderingBase = RR_GPU_SCRATCH->order + 128;
    depthShift = scene->depthShift60;

    work->input24.signedWords[0] = p->xy00;
    work->input24.fields.z = p->z04;
    while (remaining-- > 0) {
        int rawDepth, bucketDepth;
        RR_GTE14_LWC2_V0(&work->input24);
        RR_GTE_OP(0x4A4A2012);
        /* The next placement is staged before this one is emitted. */
        work->input24.signedWords[0] = p[1].xy00;
        work->input24.fields.z = p[1].z04;
        RR_GTE_SWC2_IR3(&work->depth54);
        rawDepth = work->depth54;
        bucketDepth = rawDepth >> depthShift;
        if (rawDepth > 0 && bucketDepth < depthLimit) {
            RR_GTE_TR_FROM_IR();
            InterpInstance = (uintptr_t)p;
            packetCursor = EmitModelTrisGT3(work, orderingBase + bucketDepth + p->orderingBias07,
                                            packetCursor);
            InterpInstance = 0;
        }
        p++;
    }
    RR_GPU_SCRATCH->cursor = packetCursor;
}

void DrawTrackObjectModel(int modelIndex, int flashing) {
    TrackObjectRenderWork *work = &((TrackObjectScratch *)SCRATCHPAD)->work7C;
    RroSceneScratchPrefix *sceneRoot;
    GpuPolyFT4 *packetCursor;
    const TrackObjectQuad *quadCursor;
    const Vec3s8 *vertices;
    int remaining;
    GpuDmaTag *orderingBase;
    int depthShift;
    int depth;

    work->flashing54 = flashing;
    sceneRoot = (RroSceneScratchPrefix *)SCRATCHPAD;
    packetCursor = (GpuPolyFT4 *)RR_GPU_SCRATCH->cursor;
    quadCursor = (const TrackObjectQuad *)RroScene.modelEntries[modelIndex].commands;
    vertices = RroScene.modelEntries[modelIndex].vertices;
    remaining = ((const RroBodyCommandHeader *)quadCursor)->fields.count;
    quadCursor = (const TrackObjectQuad *)((const u8 *)quadCursor + sizeof(RroBodyCommandHeader));
    {
        int xy = quadCursor->translationXY1C;
        int z = quadCursor->translationZ1A;
        work->input00.signedWords[0] = xy;
        work->input00.fields.z = z;
    }
    orderingBase = RR_GPU_SCRATCH->order + 128;
    depthShift = sceneRoot->depthShift60 + 2;
    while (remaining-- > 0) {
        RR_GTE14_LWC2_V0(&work->input00);
        RR_GTE14_NOP2();
        RR_GTE_OP(0x4A4A2012);
        {
            int nextXY = quadCursor[1].translationXY1C;
            int nextZ = quadCursor[1].translationZ1A;
            work->input00.signedWords[0] = nextXY;
            work->input00.fields.z = nextZ;
        }
        RR_GTE_READ_IR3_NOP(depth);
        if (depth > 0) {
            depth >>= depthShift;
            if (depth < 0x1C0) {
                int word0, word1, word2, word3;
                work->depth50 = depth;
                RR_GTE_TR_FROM_IR();
                word0 = quadCursor->vertices.words[0];
                word1 = quadCursor->vertices.words[1];
                word2 = word0 >> 16;
                word3 = word1 >> 16;
                word0 &= 0xFFFF;
                word1 &= 0xFFFF;
                RR_GTE_LWC2_V012_3P(&vertices[word0], &vertices[word2], &vertices[word1]);
                RR_GTE_RTPT_NOP2();
                word0 = quadCursor->uv0C;
                word2 = quadCursor->uv10;
                packetCursor->words.uv0 = word0;
                packetCursor->words.uv1 = word2;
                word0 = quadCursor->uv14;
                word2 = word0 >> 16;
                packetCursor->words.uv2 = word0;
                packetCursor->words.uv3 = word2;
                RR_GTE_SWC2_SXYZ(&work->projected58[0]);
                RR_GTE_RTPS_LDV(&vertices[word3]);
                if (work->flashing54 != 0) {
                    word0 = FlashingLightColorTable[((u32)ScreenFrame + quadCursor->flashPhase18) & 0x1F];
                } else {
                    word0 = FlashingLightColorTable[0];
                }
                packetCursor->words.color04 = word0;
                RR_SXY_COPY(packetCursor->words.xy0, work->projected58[0].word);
                RR_SXY_COPY(packetCursor->words.xy1, work->projected58[1].word);
                RR_SXY_COPY(packetCursor->words.xy2, work->projected58[2].word);
                RR_GTE_SWC2_SXY2(&work->projected58[3]);
                RR_SXY_COPY(packetCursor->words.xy3, work->projected58[3].word);
                {
                    int x0 = work->projected58[0].xy.x;
                    int x1 = work->projected58[1].xy.x;
                    int x2 = work->projected58[2].xy.x;
                    int x3 = work->projected58[3].xy.x;
                    if ((x0 >= WIDE_SCREEN_LEFT || x1 >= WIDE_SCREEN_LEFT || x2 >= WIDE_SCREEN_LEFT || x3 >= WIDE_SCREEN_LEFT) &&
                        (x0 < WIDE_SCREEN_RIGHT || x1 < WIDE_SCREEN_RIGHT || x2 < WIDE_SCREEN_RIGHT || x3 < WIDE_SCREEN_RIGHT)) {
                        int y0 = work->projected58[0].xy.y;
                        int y1 = work->projected58[1].xy.y;
                        int y2 = work->projected58[2].xy.y;
                        int y3 = work->projected58[3].xy.y;
                        if ((y0 >= 0 || y1 >= 0 || y2 >= 0 || y3 >= 0) &&
                            (y0 < RR_SCREEN_BOTTOM || y1 < RR_SCREEN_BOTTOM || y2 < RR_SCREEN_BOTTOM || y3 < RR_SCREEN_BOTTOM)) {
                            RR_GPU_ADD_PRIM(orderingBase + work->depth50 + quadCursor->orderingBias19,
                                            packetCursor, 9);
                            INTERP_TAG(packetCursor, quadCursor, 0);
                            packetCursor++;
                        }
                    }
                }
            }
        }
        quadCursor++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packetCursor;
}
void InitIntroSceneLights(void) {
    GpuRectangle sp18;
    struct IntroLightTransformWork {
        Vec3s8 angles;
        Vec3s8 direction;
        MatrixWords32 matrix;
    } *work = (struct IntroLightTransformWork *)(SCRATCHPAD + 0x7C);
    Vec3s8 *angles = &work->angles;
    Vec3s8 *direction = &work->direction;
    IntroSceneState *scene;
    int i;

    IntroSceneStatePtr = RR_ARENA_ALIGN(LoadCursor, IntroSceneState);
    LoadCursor = (u8 *)IntroSceneStatePtr + sizeof(*IntroSceneStatePtr);
    SetupRenderer(1);
    sp18.rect = MenuGridSkyVramRect;
    ClearImage2(&sp18, 0, 0, 0);
    ((ProjectionScratchPrefix *)SCRATCHPAD)->pose08.angles10 = MenuGridDefaultAngles;
    ((ProjectionScratchPrefix *)SCRATCHPAD)->pose08.position00 = IntroCameraPosKeyframes[0];
    IntroSceneStatePtr->mat20.w[1] = 0xFAAB;
    IntroSceneStatePtr->mat20.w[2] = 0xFAAB0000;
    IntroSceneStatePtr->mat20.w[0] = 0;
    IntroSceneStatePtr->mat20.w[3] = 0;
    IntroSceneStatePtr->mat20.w[4] = -0x555;
    direction->fields.z = -0x555;
    direction->fields.y = 0;
    direction->fields.x = 0;
    angles->fields.x = -0x200;
    angles->fields.y = 0x200;
    angles->fields.z = 0;
    RotMatrix(&angles->fields, &work->matrix);
    ApplyMatrixSV(&work->matrix, direction, (s16 *)&IntroSceneStatePtr->mat00.halves[0]);
    angles->fields.x = 0x400;
    angles->fields.y = 0x400;
    angles->fields.z = 0;
    RotMatrix(&angles->fields, &work->matrix);
    ApplyMatrixSV(&work->matrix, direction, (s16 *)&IntroSceneStatePtr->mat00.halves[3]);
    angles->fields.x = 0x400;
    angles->fields.y = 0x400;
    angles->fields.z = 0;
    RotMatrix(&angles->fields, &work->matrix);
    ApplyMatrixSV(&work->matrix, direction, (s16 *)&IntroSceneStatePtr->mat00.halves[6]);
    scene = IntroSceneStatePtr;
    i = 7;
    scene->s48 = 0;
    scene->s4A = 0;
    scene->s4C = 0;
    scene->s4E = 0;
    scene->s50 = 0;
    scene->s52 = 0;
    scene->s54 = 0;
    scene->s56 = 0;
    scene->s58 = 0;
    scene->s5A = 0;
    scene->s5C = 0;
    scene->s5E = 0;
    scene->s60 = 0;
    scene->s62 = 0;
    scene->s64 = 0;
    scene->s66 = 0;
    scene->s68 = 0;
    scene->s6A = 0;
    scene->s6C = 0;
    scene->s6E = 0;
    scene->s70 = 0;
    scene->s72 = 0;
    scene->s74 = 0;
    scene->s76 = 0;
    scene->s78 = 0;
    scene->s7A = 0;
    scene->s7C = 0;
    scene->s7E = 0;
    scene->s80 = 0;
    scene->s82 = 0;
    scene->s84 = 0;
    scene->s86 = 0;
    scene->s88 = 0;
    scene->s8A = 0;
    scene->s8C = 0;
    scene->s8E = 0;
    scene->s90 = 0;
    scene->s92 = 0;
    scene->s94 = 0;
    scene->s96 = 0;
    scene->s98 = 0;
    scene->s9A = 0;
    scene->s9C = 0;
    scene->s9E = 0;
    scene->sA0 = 0;
    scene->uA2 = 0;
    scene->sA4 = 0;
    scene->sA6 = 0;
    scene->uA8 = 0;
    do {
        scene->objectYawAA[i] = 0;
        i--;
    } while (i >= 0);
    AllocMenuGridRecords(SharedWorkArena, 1);
    InitMenuGrid(0, 1, 3, 0xB4, 0x60);
    PsyqMemset((u8 *)MenuGridRecords + 0x1C, 0, 0x10);
    *(VectorWords16 *)&MenuGridRecords->x0C = MenuGridInitialOffset;
}

void BuildScaledCameraMatrices(void)
{
    ViewMatrixScratch48 *scratch;
    BuildCameraMatrices();
    scratch = (ViewMatrixScratch48 *)SCRATCHPAD;
    /* Double the camera matrix's Y row, retaining halfword wraparound. */
    scratch->matrix28.halves[3] <<= 1;
    scratch->matrix28.halves[4] <<= 1;
    scratch->matrix28.halves[5] <<= 1;
}

/* Initial values of the globals this module owns. */
GpuRectangleFields MenuGridSkyVramRect = {.x = 448, .w = 128, .h = 256};
VectorWords16 MenuGridDefaultAngles = {.vector = {.x = 470, .y = 250}};
VectorWords16 MenuGridInitialOffset = {.vector = {.z = -16}};
int FlashingLightColorTable[32] = {746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746553215, 746355836, 745697906, 744711011, 743395151, 741881912, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 741881912, 743395151, 744711011, 745697906, 746355836, 746553215};
