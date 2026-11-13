#include "rr/release.h"
#include "rr/frame_interp.h"
#include "rr/widescreen.h"
#include "game/system/pad.h"
#include "rr/save_slots.h"
#include "game/ui/uipart1.h"
#include "psyq/libgte/rotmat.h"
#include "rr/model_table.h"
#include "rr/camera_matrices.h"
#include "rr/track_object_render.h"
#include "rr/gpu.h"
/* RODATA 0x8001011c 0x20 */
#include "scratchpad.h"
#include "rr/load_cursor.h"
#include "rr/tiled_rectangle.h"
#include "rr/geometry.h"
#include "rr_asm.h"
#include "game/data/cdfile2.h"
#include "game/effect/spfade.h"
#include "game/gfx/sky.h"
#include "game/data/ldscn5.h"
#include "game/gfx/gtemodel.h"
#include "game/gfx/tilerect.h"
#include "game/data/cdfile1.h"
#include "psyq/libgpu/gpuenv.h"
#include "rr/menu_background.h"
#include "rr/sprite_preset.h"
#include "rr/memory.h"
#include "rr/controller_hud.h"
#include "rr/course_choices.h"
#include "game/math/rotmat.h"
#include "game/menu/mnrows.h"
#include "game/gfx/light1.h"
#include "game/course/trkintp2.h"

#include "rr/gpu_scratch_types.h"

typedef struct {
    MatrixWords32 matrix00;
    Vec3sFields8 unk20;
    char unk28[0x08];
    int unk30;
    int unk34;
    int w38;
    short s3C;
    short s3E;
    short s40;
    short s42;
    short s44;
    short s46;
    short s48;
    short s4A;
} S_8010BF70;

/* Queue one GP0 state command at ordering entry index. */
static void EmitTileRectCommand(s32 index, u32 command) {
    GpuCommandPacket *packet = (GpuCommandPacket *)RR_GPU_SCRATCH->cursor;
    packet->command = command;
    RR_GPU_ADD_PRIM(&RR_GPU_SCRATCH->order[index], packet, 1);
    RR_GPU_SCRATCH->cursor = (u8 *)(packet + 1);
}

extern GpuRectangleFields TileRectSkyVramRect;
extern short FadeCountdown;
extern int VSyncThreshold;
extern short ModeIndex;
extern short ModeGroup;
extern short SubmodeState;
extern int TileRectUploadPhase;
extern Vec3sFields8 TileRectDefaultAngles;
extern GpuRectangleFields TileRectCaptureVramRect;
extern int TileRectSkyHorizonY;
extern S_8010BF70 *TileRectScene;

extern int TileFadeColorTable[];
extern int TileRectFlashColors[];

void TileRectSceneModeEnter(void);
void TileRectSceneModeUpdate(void);

void EmitTiledRect(GpuRectangleFields *rectangle, s32 intensity, GpuDmaTag *head) {
    GpuRectangleFields *rect = rectangle;
    GpuDmaTag *ordering = head;
    GpuScratchPointers *scratch = RR_GPU_SCRATCH;
    GpuCommandPacket *mask;
    TiledRectanglePacket *tile;
    s32 height, y, width, x;
    s32 tile_h, tile_w;
    s32 v, u;
    s32 shade = intensity;
    u32 color, top, bits_h, vpart, ypage;
    shade = (shade < 0 ? 0 : shade) > 255 ? 255 : (shade < 0 ? 0 : shade);
    color = 0x66000000U | (shade * 0x10000U) | (shade * 0x100U) | shade;
    mask = (GpuCommandPacket *)scratch->cursor;
    mask->command = 0xE6000000U;
    RR_GPU_ADD_PRIM(ordering, mask, 1);
    tile = (TiledRectanglePacket *)(mask + 1);
    height = rect->h;
    y = rect->y;
    while (height > 0) {
        v = y & 255;
        tile_h = 256 - v;
        if (tile_h >= height) tile_h = height;
        width = rect->w;
        x = rect->x;
        top = (y - 1) * 0x10000U;
        bits_h = tile_h * 0x10000U;
        vpart = v << 8;
        ypage = (y & 256) >> 4;
        while (width > 0) {
            u = x & 63;
            tile_w = 256 - u;
            if (tile_w >= width) tile_w = width;
            tile->mode = 0xE1000200U | ypage | (((x & 1023) >> 6) | 256);
            tile->nop = RR_GPU_MERGED_PRIM_NOP;
            tile->color = color;
            tile->position = top | (x & 65535);
            tile->uv = vpart | u;
            tile->size = bits_h | tile_w;
            x += tile_w;
            width -= tile_w;
            RR_GPU_ADD_PRIM(ordering, tile, 6);
            tile++;
        }
        height -= tile_h;
        y += tile_h;
    }
    mask = (GpuCommandPacket *)tile;
    mask->command = 0xE6000001U;
    RR_GPU_ADD_PRIM(&scratch->order[703], mask, 1);
    scratch->cursor = (u8 *)(mask + 1);
}

void TileRectSceneModeEnter(void) {
    GpuRectangle sp10;
    VSyncThreshold = 0x80;
    InitMenuArenas();
    SetMenuFrameOptions(0, 1, 0);
    UpdateScreenFade(0x1000C);
    sp10.rect = TileRectSkyVramRect;
    ClearImage2(&sp10, 0, 0, 0);
    FadeCountdown = 0;
    TileRectUploadPhase = 0;
    ModeGroup = 0;
    ModeIndex = 4;
    SubmodeState = 0;
}

/* The Japanese release's scene (SLPS-01800): ten preset sprites fading in
 * one after another (0x25..0x2E) and a fade tile, in a 0x74-byte record. */
typedef struct {
    MatrixWords32 matrix00;
    Vec3sFields8 unk20;
    char unk28[0x08];
    int unk30;
    int unk34;
    int w38;
    short s3C;
    short level3E[10];
    short step52[10];
    short fade66, fade68, fade6A;
    short light6C, light6E;
    short interp70;
} TileRectSceneJp;

static void TileRectSceneModeUpdateJp(void) {
    static const s16 starts[10] = {8, 0x20, 0x34, 0x40, 0x4C, 0x58, 0x64, 0x70, 0x7C, 0x94};
    TileRectSceneJp *p;
    GpuRectangleFields sp10;
    int s1, i;

    UpdateScreenFade(0x1000C);
    if (FadeCountdown != 0) {
        FadeCountdown += 4;
        if (FadeCountdown >= 0x100) {
            ResetScreenCounters();
            ModeGroup = 1;
            ModeIndex = 0xB;
            SubmodeState = 0;
        }
    }

    switch (TileRectUploadPhase) {
    case 0:
        if (CdFileReadStart(GetR4FileIndex(0, SelectionState.fields.reserved10[2] + 1), LoadCursor) == 0) {
            return;
        }
        TileRectUploadPhase = 1;
    case 1:
        if (CdFileBytesLoaded() == 0) {
            return;
        }
        LoadTimList(LoadCursor);
        TileRectUploadPhase = 2;
    case 2:
        if (CdFileReadStart(GetR4FileIndex(0, 0x14), LoadCursor) == 0) {
            return;
        }
        TileRectUploadPhase = 3;
    case 3:
        if (CdFileBytesLoaded() == 0) {
            return;
        }
        LoadTimImage(LoadCursor);
        TileRectUploadPhase = 4;
    case 4:
        if (CdFileReadStart(GetR4FileIndex(0, SelectionState.fields.reserved10[2] + 0xA), LoadCursor) == 0) {
            return;
        }
        TileRectUploadPhase = 5;
    case 5:
        s1 = CdFileBytesLoaded();
        if (s1 == 0) {
            return;
        }
        SkyCommandListPtr = (u8 *)LoadCursor;
        TileRectScene = (S_8010BF70 *)(LoadCursor = (u8 *)LoadCursor + s1);
        LoadCursor = (char *)TileRectScene + 0x74;
        SetupRenderer(3);
        p = (TileRectSceneJp *)TileRectScene;
        p->unk20 = TileRectDefaultAngles;
        PsyqMemset(p->unk28, 0, 0x10);
        TileRectUploadPhase = 6;
        p->fade66 = 0x100;
        p->light6E = 0x40;
        p->w38 = 0;
        p->s3C = 0;
        for (i = 0; i < 10; i++) {
            p->level3E[i] = 0;
            p->step52[i] = 0;
        }
        p->fade68 = 0;
        p->fade6A = 0;
        p->light6C = 0;
        p->interp70 = 0x1000;
        p->s3C = 0;
    case 6:
        p = (TileRectSceneJp *)TileRectScene;
        if (p->s3C < 0xB4) {
            for (i = 0; i < 10; i++) {
                if (p->s3C == starts[i]) {
                    p->step52[i] = 4;
                }
            }
            if (p->s3C == 0x88) {
                p->fade6A = 0x200;
            }
            break;
        }
        SetMenuFrameOptions(1, 1, 0);
        TileRectUploadPhase = 7;
    case 7:
        p = (TileRectSceneJp *)TileRectScene;
        if (FadeCountdown == 0 && MenuReturnSubmodeIndex != 0) {
            FadeCountdown = 4;
            for (i = 9; i >= 0; i--) {
                p->step52[i] = -4;
            }
            p->fade6A = -4;
            p->light6E = -0x40;
        }
        break;
    }

    p = (TileRectSceneJp *)TileRectScene;
    p->s3C++;
    p->unk20.y -= 4;
    for (i = 0; i < 10; i++) {
        RampValueToLimit((unsigned short *)&p->level3E[i], &p->step52[i], 0x80);
    }
    RampValuePairTo4096((unsigned short *)&p->fade66, (unsigned short *)&p->fade68, &p->fade6A);

    if (p->interp70 != 0) {
        int ir2;

        if (p->interp70 / 32 != 0) {
            p->interp70 -= p->interp70 / 32;
        } else if (p->interp70 >= 3) {
            p->interp70 -= 2;
        } else {
            p->interp70 = 0;
        }
        /* INTPL as in the PAL scene: IR1 lands in the word at 0x30. */
        Psyz_GteDataWrite(8, (u16)p->interp70);
        Psyz_GteDataWrite(9, 0x720);
        Psyz_GteCtrlWrite(21, 0);
        Psyz_GteDataWrite(10, 0x90);
        Psyz_GteCtrlWrite(22, 0xFF);
        Psyz_GteCommand(0x0980011);
        p->unk30 = Psyz_GteDataRead(9);
        ir2 = Psyz_GteDataRead(10);
        TileRectSkyHorizonY = ir2;
    }

    for (i = 0; i < 10; i++) {
        EmitPresetSprite(p->level3E[i], 0x25 + i);
    }
    EmitTileRectCommand(1, 0xE1000229);
    EmitFadeTile(p->fade66, p->fade68, 0x01080020, 0xD8);
    RampValueTo4096((unsigned short *)&p->light6C, &p->light6E);

    if (p->light6C != 0) {
        SetSineLightColors(p->light6C);
        BuildCameraMatrices();
        RotMatrix(&p->unk20, &p->matrix00);
        sp10 = TileRectCaptureVramRect;
        RenderSkyToRect(p->unk28, (S_8010BF70 *)p, &sp10, TileRectSkyHorizonY);
    }
    EmitTileRectCommand(703, 0xE6000001);
}

void TileRectSceneModeUpdate(void) {
    if (RR_RELEASE_JP) {
        TileRectSceneModeUpdateJp();
        return;
    }
    GpuRectangleFields sp10;
    int s1;

    UpdateScreenFade(0x1000C);
    if (FadeCountdown != 0) {
        FadeCountdown += 4;
        if (FadeCountdown >= 0x100) {
            ResetScreenCounters();
            ModeGroup = 1;
            ModeIndex = 0xB;
            SubmodeState = 0;
        }
    }

    switch (TileRectUploadPhase) {
    case 0:
        if (CdFileReadStart(GetR4FileIndex(0, SelectionState.fields.reserved10[2] + 1), LoadCursor) == 0) {
            return;
        }
        TileRectUploadPhase = 1;
    case 1:
        if (CdFileBytesLoaded() == 0) {
            return;
        }
        LoadTimList(LoadCursor);
        TileRectUploadPhase = 2;
    case 2:
        if (CdFileReadStart(GetR4FileIndex(0, 0x14), LoadCursor) == 0) {
            return;
        }
        TileRectUploadPhase = 3;
    case 3:
        if (CdFileBytesLoaded() == 0) {
            return;
        }
        LoadTimImage(LoadCursor);
        TileRectUploadPhase = 4;
    case 4:
        if (CdFileReadStart(GetR4FileIndex(0, SelectionState.fields.reserved10[2] + 0xA), LoadCursor) == 0) {
            return;
        }
        TileRectUploadPhase = 5;
    case 5:
        s1 = CdFileBytesLoaded();
        if (s1 == 0) {
            return;
        }
        SkyCommandListPtr = (u8 *)LoadCursor; /* sky records stay wire offsets (sky.c) */
        TileRectScene = (S_8010BF70 *)(LoadCursor = (u8 *)LoadCursor + s1);
        LoadCursor = (char *)TileRectScene + 0x4C;
        SetupRenderer(3);
        TileRectScene->unk20 = TileRectDefaultAngles;
        PsyqMemset(TileRectScene->unk28, 0, 0x10);
        {
            S_8010BF70 *p = TileRectScene;
            TileRectUploadPhase = 6;
            p->s48 = 0x40;
            p->w38 = 0;
            p->s3C = 0;
            p->s3E = 0;
            p->s40 = 0;
            p->s42 = 0;
            p->s44 = 0;
            p->s46 = 0;
            p->s4A = 0x1000;
            p->s3C = 0;
        }
    case 6:
        if (TileRectScene->s3C < 0x40) {
            if (TileRectScene->s3C == 8) {
                TileRectScene->s42 = 4;
            } else if (TileRectScene->s3C == 0x20) {
                TileRectScene->s44 = 4;
            }
            break;
        }
        SetMenuFrameOptions(1, 1, 0);
        TileRectUploadPhase = 7;
    case 7:
        if (FadeCountdown == 0 && MenuReturnSubmodeIndex != 0) {
            FadeCountdown = 4;
            TileRectScene->s44 = -4;
            TileRectScene->s42 = -4;
        }
        break;
    }

    TileRectScene->s3C++;
    TileRectScene->unk20.y -= 4;
    RampValueToLimit(&TileRectScene->s3E, &TileRectScene->s42, 0x80);
    RampValueToLimit(&TileRectScene->s40, &TileRectScene->s44, 0x80);

    if (TileRectScene->s4A != 0) {
        S_8010BF70 *p;
        int ir2;

        if (TileRectScene->s4A / 32 != 0) {
            TileRectScene->s4A -= TileRectScene->s4A / 32;
        } else if (TileRectScene->s4A >= 3) {
            TileRectScene->s4A -= 2;
        } else {
            TileRectScene->s4A = 0;
        }

        p = TileRectScene;
        /* INTPL: IR = IR + IR0 * (FC - IR) with FC = (0, 0xFF), IR1 = 0x720,
         * IR2 = 0x90. IR1 lands in the scene's word at 0x30. */
        Psyz_GteDataWrite(8, (u16)p->s4A);
        Psyz_GteDataWrite(9, 0x720);
        Psyz_GteCtrlWrite(21, 0);
        Psyz_GteDataWrite(10, 0x90);
        Psyz_GteCtrlWrite(22, 0xFF);
        Psyz_GteCommand(0x0980011);
        p->unk30 = Psyz_GteDataRead(9);
        ir2 = Psyz_GteDataRead(10);
        TileRectSkyHorizonY = ir2;
    }

    EmitPresetSprite(TileRectScene->s3E, 0x25);
    EmitPresetSprite(TileRectScene->s40, 0x26);

    EmitTileRectCommand(1, 0xE1000229);
    RampValueTo4096(&TileRectScene->s46, &TileRectScene->s48);

    if (TileRectScene->s46 != 0) {
        GpuRectangleFields *a2;
        SetSineLightColors(TileRectScene->s46);
        BuildCameraMatrices();
        RotMatrix(&TileRectScene->unk20, &TileRectScene->matrix00);
        sp10 = TileRectCaptureVramRect;
        a2 = &sp10;
        RenderSkyToRect(TileRectScene->unk28, TileRectScene, a2, TileRectSkyHorizonY);
    }
    EmitTileRectCommand(703, 0xE6000001);
}

void DrawTrackObjectBillboard(int modelIndex) {
    TrackObjectBillboardWork *work = &((TrackObjectBillboardScratch *)SCRATCHPAD)->work7C;
    RroSceneScratchPrefix *scene;
    GpuPolyFT4 *packet;
    const TrackObjectQuad *quad;
    const Vec3s8 *vertices;
    int count;
    GpuDmaTag *orderingBase;
    int depthShift;
    int depthBucket;
    int word0, word1, word2, word3;
    u32 phaseOffset;

    work->rotation00.w[4] = 0x4000;
    work->rotation00.w[2] = 0x4000;
    work->rotation00.w[0] = 0x4000;
    work->rotation00.w[3] = 0;
    work->rotation00.w[1] = 0;
    RR_GTE_SETROT(&work->rotation00);
    RR_GTE_SETLLM(&((RroSceneScratchPrefix *)SCRATCHPAD)->view.matrix28);
    RR_GTE_SETBK_P(CameraViewMatrix.fields.translation14);

    scene = (RroSceneScratchPrefix *)SCRATCHPAD;
    packet = (GpuPolyFT4 *)RR_GPU_SCRATCH->cursor;
    quad = (const TrackObjectQuad *)RroScene.modelEntries[modelIndex].commands;
    vertices = RroScene.modelEntries[modelIndex].vertices;
    count = ((const RroBodyCommandHeader *)quad)->fields.count;
    quad = (const TrackObjectQuad *)((const u8 *)quad + sizeof(RroBodyCommandHeader));
    word0 = quad->translationXY1C;
    word2 = quad->translationZ1A;
    work->input2C.signedWords[0] = word0;
    work->input2C.fields.z = word2;
    orderingBase = RR_GPU_SCRATCH->order + 128;
    depthShift = scene->depthShift60 + 2;
    while (count-- > 0) {
        RR_GTE_MVMVA_LLM_BK_LDV0(&work->input2C.signedWords[0]);
        word0 = quad[1].translationXY1C;
        word2 = quad[1].translationZ1A;
        work->input2C.signedWords[0] = word0;
        work->input2C.fields.z = word2;
        RR_GTE_SWC2_IR3(&work->depth28);
        word0 = work->depth28;
        if (word0 >= 0) {
            depthBucket = word0 >> depthShift;
            if (depthBucket < 0x1C0) {
                RR_GTE_TR_FROM_IR();
                word0 = quad->vertices.words[0];
                word1 = quad->vertices.words[1];
                word2 = word0 >> 16;
                word3 = word1 >> 16;
                word0 &= 0xFFFF;
                word1 &= 0xFFFF;
                RR_GTE_LWC2_V012_3P(&vertices[word0], &vertices[word2], &vertices[word1]);
                RR_GTE14_NOP2();
                RR_GTE14_RTPT();
                word0 = quad->uv0C;
                word2 = quad->uv10;
                packet->words.uv0 = word0;
                packet->words.uv1 = word2;
                word0 = quad->uv14;
                word2 = word0 >> 16;
                packet->words.uv2 = word0;
                packet->words.uv3 = word2;
                RR_GTE_SWC2_SXYZ(&work->projected34[0].word);
                RR_GTE_RTPS_LDV(&vertices[word3]);
                word2 = work->projected34[0].word;
                word1 = work->projected34[1].word;
                word3 = work->projected34[2].word;
                word0 = TileFadeColorTable[(ScreenFrame - (phaseOffset = quad->flashPhase18 - 0x40)) & 0x3F];
                RrSxyCopy(&packet->words.xy0, &work->projected34[0].word);
                RrSxyCopy(&packet->words.xy1, &work->projected34[1].word);
                RrSxyCopy(&packet->words.xy2, &work->projected34[2].word);
                packet->words.color04 = word0;
                RR_GTE_SWC2_SXY2(&work->projected34[3].word);
                RR_SXY_COPY(packet->words.xy3, work->projected34[3].word);
                {
                    int x0 = work->projected34[0].xy.x;
                    int x1 = work->projected34[1].xy.x;
                    int x2 = work->projected34[2].xy.x;
                    int x3 = work->projected34[3].xy.x;
                    if ((x0 >= WIDE_SCREEN_LEFT || x1 >= WIDE_SCREEN_LEFT || x2 >= WIDE_SCREEN_LEFT || x3 >= WIDE_SCREEN_LEFT) &&
                        (x0 < WIDE_SCREEN_RIGHT || x1 < WIDE_SCREEN_RIGHT || x2 < WIDE_SCREEN_RIGHT || x3 < WIDE_SCREEN_RIGHT)) {
                        int y0 = work->projected34[0].xy.y;
                        int y1 = work->projected34[1].xy.y;
                        int y2 = work->projected34[2].xy.y;
                        int y3 = work->projected34[3].xy.y;
                        if ((y0 >= 0 || y1 >= 0 || y2 >= 0 || y3 >= 0) &&
                            (y0 < RR_SCREEN_BOTTOM || y1 < RR_SCREEN_BOTTOM || y2 < RR_SCREEN_BOTTOM || y3 < RR_SCREEN_BOTTOM)) {
                            RR_GPU_ADD_PRIM(&(orderingBase + depthBucket)[quad->orderingBias19], packet, 9);
                            INTERP_TAG(packet, quad, 0);
                            packet++;
                        }
                    }
                }
            }
        }
        quad++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
}

void DrawTrackObjectFlashQuads(int modelIndex) {
    TrackObjectFlashWork *work = &((TrackObjectFlashScratch *)SCRATCHPAD)->work7C;
    RroSceneScratchPrefix *scene;
    const RroTexturedQuadPrefix *quad;
    GpuPolyFT4 *packet;
    const Vec3s8 *vertices;
    int count;
    GpuDmaTag *orderingBase;
    int windingMask;
    int depthShift;
    int word0, word1, word2, word3, entryOffset, renderFlags;

    RR_GTE_SETROT(&((RroSceneScratchPrefix *)SCRATCHPAD)->view.matrix28);
    RR_GTE_SETTR(((RroSceneScratchPrefix *)SCRATCHPAD)->view.matrix28.fields.translation14);

    entryOffset = modelIndex * sizeof(RroEffectMesh);
    scene = (RroSceneScratchPrefix *)SCRATCHPAD;
    packet = (GpuPolyFT4 *)RR_GPU_SCRATCH->cursor;
    orderingBase = RR_GPU_SCRATCH->order + 128;
    quad = (const RroTexturedQuadPrefix *)RroScene.modelEntries[modelIndex].commands;
    depthShift = scene->depthShift60 + 2;
    vertices = RroScene.modelEntries[modelIndex].vertices;
    count = ((const RroBodyCommandHeader *)quad)->fields.count;
    quad = (const RroTexturedQuadPrefix *)((const u8 *)quad + sizeof(RroBodyCommandHeader));
    renderFlags = scene->renderFlags64;
    windingMask = renderFlags ? (-2147483647 - 1) : 0;
    while (count-- > 0) {
        RR_GTE_LWC2_V012_3P(&vertices[quad->vertices.indices[0]], &vertices[quad->vertices.indices[1]], &vertices[quad->vertices.indices[2]]);
        RR_GTE_RTPT_NOP2();
        word3 = quad->uv0C;
        word2 = quad->uv10;
        packet->words.uv0 = word3;
        packet->words.uv1 = word2;
        word3 = quad->uv14;
        word2 = word3 >> 16;
        packet->words.uv2 = word3;
        packet->words.uv3 = word2;
        RR_GTE14_NCLIP();
        RR_GTE_SWC2_SXYZ(&work->projected24[0].word);
        RR_GTE14_SWC2_MAC0(&work->area20);
        if ((work->area20 ^ windingMask) >= 0) {
            RR_GTE_RTPS_LDV(&vertices[quad->vertices.indices[3]]);
            word3 = TileRectFlashColors[(0U + ScreenFrame + quad->flashPhase18) & 0x3F];
            word2 = work->projected24[0].word;
            word0 = work->projected24[1].word;
            word1 = work->projected24[2].word;
            RrSxyCopy(&packet->words.xy0, &work->projected24[0].word);
            RrSxyCopy(&packet->words.xy1, &work->projected24[1].word);
            RrSxyCopy(&packet->words.xy2, &work->projected24[2].word);
            packet->words.color04 = word3;
            RR_GTE14_NOP2();
            RR_GTE14_AVSZ4();
            RR_GTE_SWC2_SXY2(&work->projected24[3].word);
            RR_GTE14_SWC2_OTZ(&work->depth1C);
            RR_SXY_COPY(packet->words.xy3, work->projected24[3].word);
            {
                int otz = work->depth1C;
                if (otz > 0) {
                    otz >>= depthShift;
                    work->depth1C = otz;
                    if (otz < 0x1C0) {
                        int x0 = work->projected24[0].xy.x;
                        int x1 = work->projected24[1].xy.x;
                        int x2 = work->projected24[2].xy.x;
                        int x3 = work->projected24[3].xy.x;
                        if ((x0 >= WIDE_SCREEN_LEFT || x1 >= WIDE_SCREEN_LEFT || x2 >= WIDE_SCREEN_LEFT || x3 >= WIDE_SCREEN_LEFT) &&
                            (x0 < WIDE_SCREEN_RIGHT || x1 < WIDE_SCREEN_RIGHT || x2 < WIDE_SCREEN_RIGHT || x3 < WIDE_SCREEN_RIGHT)) {
                            int y0 = work->projected24[0].xy.y;
                            int y1 = work->projected24[1].xy.y;
                            int y2 = work->projected24[2].xy.y;
                            int y3 = work->projected24[3].xy.y;
                            if ((y0 >= 0 || y1 >= 0 || y2 >= 0 || y3 >= 0) &&
                                (y0 < RR_SCREEN_BOTTOM || y1 < RR_SCREEN_BOTTOM || y2 < RR_SCREEN_BOTTOM || y3 < RR_SCREEN_BOTTOM)) {
                                RR_GPU_ADD_PRIM(&(orderingBase + work->depth1C)[quad->orderingBias19], packet, 9);
                                INTERP_TAG(packet, quad, 0);
                                packet++;
                            }
                        }
                    }
                }
            }
        }
        quad++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
}

u8 *EmitModelTrisGT3(const TrackObjectLitWork *work, GpuDmaTag *head, u8 *packetCursor) {
    const TrackObjectTriangle *triangle = (const TrackObjectTriangle *)RroScene.workCommands;
    const Vec3s8 *vertices = RroScene.workVertices;
    int count = ((unsigned short *)triangle)[1];
    int x0, x1, x2, y0, y1, y2;
    GpuPolyG3 *packet = (GpuPolyG3 *)packetCursor;
    GpuCommandPacket *mode;

    triangle = (const TrackObjectTriangle *)((const u8 *)triangle + 4);
    do {
        x0 = triangle->vertices.words[0];
        x2 = triangle->vertices.indices[2];
        x1 = x0 >> 16;
        x0 &= 0xFFFF;
        RR_GTE_LWC2_V012_3P(&vertices[x0], &vertices[x1], &vertices[x2]);
        RR_GTE_RTPT_NOP2();
        packet->color0 = triangle->colors08[0];
        packet->color1 = triangle->colors08[1];
        packet->color2 = triangle->colors08[2];
        x0 = Psyz_GteDataRead(12);
        x1 = Psyz_GteDataRead(13);
        x2 = Psyz_GteDataRead(14);
        RR_GTE_STORE_SXY(12, &packet->xy0);
        RR_GTE_STORE_SXY(13, &packet->xy1);
        RR_GTE_STORE_SXY(14, &packet->xy2);
        y0 = x0 >> 16;
        y1 = x1 >> 16;
        y2 = x2 >> 16;
        if ((y0 >= 0 || y1 >= 0 || y2 >= 0) &&
            (y0 < RR_SCREEN_BOTTOM || y1 < RR_SCREEN_BOTTOM || y2 < RR_SCREEN_BOTTOM)) {
            x0 = (s16)x0;
            x1 = (s16)x1;
            x2 = (s16)x2;
            if ((x0 >= WIDE_SCREEN_LEFT || x1 >= WIDE_SCREEN_LEFT || x2 >= WIDE_SCREEN_LEFT) &&
                (x0 < WIDE_SCREEN_RIGHT || x1 < WIDE_SCREEN_RIGHT || x2 < WIDE_SCREEN_RIGHT)) {
                RR_GPU_ADD_PRIM(head, packet, 6);
                INTERP_TAG(packet, triangle, 0);
                packet++;
            }
        }
        triangle++;
        count--;
    } while (count > 0);
    mode = (GpuCommandPacket *)packet;
    mode->command = 0xE1000220;
    RR_GPU_ADD_PRIM(head, mode, 1);
    return (u8 *)(mode + 1);
}

/* Initial values of the globals this module owns. */
GpuRectangleFields TileRectSkyVramRect = {.x = 448, .w = 128, .h = 320};
Vec3sFields8 TileRectDefaultAngles = {.x = 40, .y = 682};
GpuRectangleFields TileRectCaptureVramRect = {.x = 176, .y = 72, .w = 128, .h = 320};
int TileRectSkyHorizonY = 128;
int TileFadeColorTable[64] = {738197504, 739776536, 741355568, 742868807, 744118874, 745171562, 745961078, 746421629, 746619008, 746421629, 745961078, 745171562, 744118874, 742868807, 741355568, 739776536, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504, 738197504};
int TileRectFlashColors[64] = {746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746619008, 746553215, 746355836, 745697906, 744711011, 743395151, 741881912, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 740302880, 741881912, 743395151, 744711011, 745697906, 746355836, 746553215};
S_8010BF70 *TileRectScene;
int TileRectUploadPhase;
