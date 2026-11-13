#include "rr/rro_model.h"
#include "psyq/libgte/gtemat7.h"
#include "psyq/libgte/gtemat4.h"
#include "psyq/libgte/gtemat3.h"
#include "psyq/libgte/gtemat.h"
#include "rr/matrix_workspace.h"
#include "rr/cpu_time.h"
#include "types.h"
#include "scratchpad.h"
#include "psyq/libgte/libgte.h"
#include "rr_asm.h"
#include "game/camera/viewcam.h"
#include "game/course/trkclip.h"
#include "rr/overlay662_palette_refresh.h"
#include "rr/race_types.h"
#include "game/gfx/model.h"
#include "game/car/bounds.h"
#include "rr/relative_file.h"
#include "game/math/rotmat.h"
#include "psyq/libgpu/gpuenv.h"
#include <stdlib.h>

static inline const u8 *RroModelBankAddress(const RroModelBank *bank,u32 relative) {
    return RrWireAddress(bank, relative);
}

#define G ((RroSceneScratchPrefix *)SCRATCHPAD)

extern int ViewUsesCarTrackNode;
extern short ModelTrackSegmentResult[2];
extern int ActiveModelVertexCount;
/* VRAM image slots: each holds the pixel data uploaded to the lens-flare
 * sprite rectangle by LoadVramImageSlot. */
extern const void *ModelBoneMatrixTable[];
extern GpuRectangle OpeningLensFlareSprites[];

void SetFogNearFar(s32 a0, s32 a1, s32 a2);
s32 DrawArticulatedCarModel(RaceVehicle *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6);
int GetViewTrackSegment(const ProjectionView32 *view, s32 player) {
    int node, yaw;
    if (ViewUsesCarTrackNode == 0) {
        node = GetViewTrackNode(view->position00.words, player);
        yaw = view->angles10.xyz.y;
        GetTrackSegmentAndYaw(node, yaw, ModelTrackSegmentResult);
        return ModelTrackSegmentResult[0];
    }
    return GetViewTrackNode(view->position00.words, player) / 5;
}

s32 YawToOctant(const ProjectionView32 *view) {
    return (((view->angles10.xyz.y & 0xFFF) + 0x100) / 0x200) & 7;
}

CourseViewIndices *GetSegmentViewTable(s32 player) {
    const ProjectionView32 *view = &G->view.projection.pose08;
    int segment = GetViewTrackSegment(view, player);
    return (CourseViewIndices *)RelFileSegmentView(segment * 8 + YawToOctant(view));
}

/* Widescreen: each octant's list is made for the 4:3 view cone around the
 * octant's yaw. The wider cone of a camera off the octant's centre also
 * reaches into the list of the next octant on that side. */
CourseViewIndices *GetWideNeighbourViewTable(s32 player) {
    const ProjectionView32 *view = &G->view.projection.pose08;
    s32 yaw = view->angles10.xyz.y & 0xFFF;
    s32 offset = ((yaw + 0x100) & 0x1FF) - 0x100; /* from the octant's centre */
    s32 octant = (YawToOctant(view) + (offset < 0 ? 7 : 1)) & 7;
    return (CourseViewIndices *)RelFileSegmentView(GetViewTrackSegment(view, player) * 8 + octant);
}

void SetFogNearFar(s32 a0, s32 a1, s32 a2) {
    double d2, d0, d1, d6, t0, d4, d2_out;
    s32 r0, r1;

    d2 = a2;
    d0 = a0;
    d1 = a1;
    d6 = d2 * 65536.0;
    d4 = 16777216.0 / (d6 * ((1.0 / d1) - (1.0 / d0)));
    t0 = d6 / d0;
    d2_out = t0 * (-d4);
    if (d4 > 32767.0 || d4 < -32768.0) {
        if (d4 > 0.0) {
            d4 = 32767.0;
        } else {
            d4 = -32768.0;
        }
        d2_out = t0 * (-d4);
        d2_out = d2_out + (4096.0 - ((d2_out + d4 * (d6 / d1)) * (1.0 / 4096.0))) * 4096.0;
    }
    if (d2_out > 2147483647.0) {
        d2_out = 2147483647.0;
    }
    if (d2_out < -2147483648.0) {
        d2_out = -2147483648.0;
    }
    r0 = d4;
    r1 = d2_out;
    GTE_CTC2_27(r0);
    GTE_CTC2_28(r1);
}
void InitTimTemplate(EmblemTimImage840 *image) {
    RR_CPU_CYCLES(RR_CYCLES_INIT_TIM_TEMPLATE);

    u32 *words = image->words;
    int n;
    *words++ = 0x10;
    *words++ = 8;
    *words++ = 0x2C;
    *words++ = 0x400280;
    *words++ = 0x10010;
    *words++ = 0x7FFF0000;
    *words++ = 0x7C1F03FF;
    *words++ = 0x7FE0001F;
    *words++ = 0x7C0003E0;
    *words++ = 0x42100000;
    *words++ = 0x40100210;
    *words++ = 0x42000010;
    *words++ = 0x40000200;
    *words++ = 0x80C;
    *words++ = 0x280;
    *words++ = 0x400010;
    n = 0x1FF;
    do {
        *words = 0;
        n--;
        words++;
    } while (n >= 0);
}

s32 DrawArticulatedCarModel(RaceVehicle *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6) {
    MatrixWords32 m10;
    MatrixWords32 m30;
    MatrixWords32 m50;
    MatrixWords32 m70;
    MatrixWords32 m90;
    MatrixWords32 mB0;
    SVECTOR svD0;
    MatrixWords32 mD8[6];
    VECTOR v198[5];
    s32 dist;
    s32 i;
    s32 s5;
    s32 a1;

    SelectModelBank(arg1);
    v198[0].vx = *(s16 *)&arg0->position20.words[0] - (s16)G->view.projection.pose08.position00.xyz.x;
    v198[0].vy = 0;
    v198[0].vz = *(s16 *)&arg0->position20.words[2] - (s16)G->view.projection.pose08.position00.xyz.z;
    if (v198[0].vx < 0) {
        v198[0].vx = -v198[0].vx;
    }
    if (v198[0].vz < 0) {
        v198[0].vz = -v198[0].vz;
    }
    dist = v198[0].vx + v198[0].vz;
    switch (arg2) {
    case 0:
        i = dist;
        break;
    case 1:
        i = arg3 - 1;
        break;
    case 2:
        i = arg4 - 1;
        break;
    case 3:
        i = arg5 - 1;
        break;
    }
    if (i < arg3) {
        s5 = 0;
        if (G->renderFlags64 != 0) {
            s5 = -0x60;
        }
        BuildRotMatrixY(&m90, -arg0->rot54);
        BuildRotMatrixX(&m50, -arg0->rot50);
        MulMatrix2(&m90, &m50);
        BuildRotMatrixZ(&m70, -arg0->rot58);
        MulMatrix2(&m50, &m70);
        MulMatrix0(&LightMatrix.matrix, &m70, &mB0);
        SetLightMatrix(&mB0);
        BuildRotMatrixY(&m10, -arg0->rot54);
        MulMatrix2((MatrixWords32 *)SCRATCH_ROT_MATRIX, &m10);
        BuildRotMatrixX(&mD8[0], arg0->wheelRotX1A8 - arg0->rot50);
        BuildRotMatrixZ(&m50, arg0->wheelRotZ1AA - arg0->rot58);
        MulMatrix(&mD8[0], &m50);
        MulMatrix2(&m10, &mD8[0]);
        v198[0] = *(VECTOR *)&arg0->bodyPosition;
        v198[1].vx = arg0->ir70[0] + arg0->ir60[0];
        v198[1].vy = arg0->ir70[1] + arg0->ir60[1];
        v198[1].vz = arg0->ir70[2] + arg0->ir60[2];
        BuildRotMatrixX(&mD8[1], -arg0->ang80);
        BuildRotMatrixZ(&m50, -arg0->ang84);
        MulMatrix(&mD8[1], &m50);
        MulMatrix2(&m10, &mD8[1]);
        v198[2].vx = arg0->worldPosition10.words[0] + arg0->ir60[0];
        v198[2].vy = arg0->worldPosition10.words[1] + arg0->ir60[1];
        v198[2].vz = arg0->worldPosition10.words[2] + arg0->ir60[2];
        BuildRotMatrixX(&m30, -arg0->rot50);
        BuildRotMatrixX(&mD8[3], arg0->wheelSteering17C);
        BuildRotMatrixZ(&mD8[2], -arg0->rot58);
        MulMatrix2(&m30, &mD8[2]);
        m30 = mD8[2];
        MulMatrix2(&m10, &mD8[2]);
        MulMatrix2(&mD8[2], &mD8[3]);
        BuildRotMatrixY(&mD8[4], arg0->lateralMotion178 / 12);
        BuildRotMatrixX(&mD8[5], arg0->wheelSteering17C);
        MulMatrix2(&m30, &mD8[4]);
        MulMatrix2(&m10, &mD8[4]);
        MulMatrix2(&mD8[4], &mD8[5]);
        MulMatrix(&m90, &m30);
        RR_GTE_SETROT(&m90);
        for (i = 0; i < 2; i++) {
            s32 x = CarBoundsTable[0]->halfWidth;
            if (i & 1) {
                x = -x;
            }
            svD0.vx = x << 2;
            svD0.vy = CarBoundsTable[0]->wheelOffsetY << 2;
            svD0.vz = -(CarBoundsTable[0]->halfLength) << 2;
            RR_GTE_MVMVA_ROT_V0_NOCV(&svD0);
            RR_GTE_STORE_IR(&v198[i + 3]);
            v198[i + 3].vx += v198[2].vx;
            v198[i + 3].vy += v198[2].vy;
            v198[i + 3].vz += v198[2].vz;
        }
        SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform, (const s16 *)&v198[0], mD8[0].w);
        a1 = 1;
        G->draw74.draw.counter = 0x05000000;
        if (ModelBankFormat >= 4) {
            a1 = 3;
        }
        SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1, 0, s5);
        SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform, (const s16 *)&v198[1], mD8[1].w);
        G->draw74.draw.counter = 0x05000000;
        SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, ModelBankFormat < 1, arg6, s5);
        SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform, (const s16 *)&v198[2], mD8[2].w);
        a1 = 1;
        G->draw74.draw.counter = 0x05000000;
        if (ModelBankFormat >= 10) {
            a1 = 9;
        }
        SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1, 0, s5);
        SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform, (const s16 *)&v198[2], mD8[3].w);
        a1 = 1;
        G->draw74.draw.counter = 0x05000000;
        if (ModelBankFormat >= 7) {
            a1 = 6;
        }
        SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1, arg6, s5);
        SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform, (const s16 *)&v198[3], mD8[4].w);
        a1 = 1;
        G->draw74.draw.counter = 0x05000000;
        if (ModelBankFormat >= 9) {
            a1 = 8;
        }
        SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1, 0, s5);
        SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform, (const s16 *)&v198[4], mD8[4].w);
        a1 = 1;
        G->draw74.draw.counter = 0x05000000;
        if (ModelBankFormat >= 8) {
            a1 = 7;
        }
        SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1, 0, s5);
        SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform, (const s16 *)&v198[3], mD8[5].w);
        a1 = 1;
        G->draw74.draw.counter = 0x05000000;
        if (ModelBankFormat >= 6) {
            a1 = 5;
        }
        SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1, arg6, s5);
        SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform, (const s16 *)&v198[4], mD8[5].w);
        a1 = 1;
        G->draw74.draw.counter = 0x05000000;
        if (ModelBankFormat >= 5) {
            a1 = 4;
        }
        SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1, arg6, s5);
    } else if (i < arg4) {
        s5 = 0;
        if (G->renderFlags64 != 0) {
            s5 = -0x60;
        }
        BuildRotMatrixY(&m90, -arg0->rot54);
        BuildRotMatrixX(&m50, -arg0->rot50);
        MulMatrix2(&m90, &m50);
        BuildRotMatrixZ(&m70, -arg0->rot58);
        MulMatrix2(&m50, &m70);
        MulMatrix0(&LightMatrix.matrix, &m70, &mB0);
        SetLightMatrix(&mB0);
        BuildRotMatrixY(&m10, -arg0->rot54);
        MulMatrix2((MatrixWords32 *)SCRATCH_ROT_MATRIX, &m10);
        BuildRotMatrixX(&mD8[0], arg0->wheelRotX1A8 - arg0->rot50);
        BuildRotMatrixZ(&m50, arg0->wheelRotZ1AA - arg0->rot58);
        MulMatrix(&mD8[0], &m50);
        MulMatrix2(&m10, &mD8[0]);
        v198[0] = *(VECTOR *)&arg0->bodyPosition;
        v198[1].vx = arg0->ir70[0] + arg0->ir60[0];
        v198[1].vy = arg0->ir70[1] + arg0->ir60[1];
        v198[1].vz = arg0->ir70[2] + arg0->ir60[2];
        BuildRotMatrixX(&mD8[1], -arg0->ang80);
        BuildRotMatrixZ(&m50, -arg0->ang84);
        MulMatrix(&mD8[1], &m50);
        MulMatrix2(&m10, &mD8[1]);
        SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform, (const s16 *)&v198[0], mD8[0].w);
        a1 = 1;
        G->draw74.draw.counter = 0;
        if (ModelBankFormat >= 4) {
            a1 = 3;
        }
        SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1, 0, s5);
        SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform, (const s16 *)&v198[1], mD8[1].w);
        G->draw74.draw.counter = 0;
        SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, 1, arg6, s5);
    } else if (i < arg5) {
        s5 = 0;
        if (G->renderFlags64 != 0) {
            s5 = -0x7F;
        }
        BuildRotMatrixY(&m50, -arg0->rot54);
        BuildRotMatrixX(&m90, -arg0->rot50);
        MulMatrix2(&m50, &m90);
        MulMatrix0(&LightMatrix.matrix, &m90, &mB0);
        SetLightMatrix(&mB0);
        BuildRotMatrixZ(&m50, -arg0->rot58);
        MulMatrix2(&m90, &m50);
        MulMatrix2((MatrixWords32 *)SCRATCH_ROT_MATRIX, &m50);
        SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform, (const s16 *)arg0->position20.words, m50.w);
        a1 = 1;
        G->draw74.draw.counter = 0;
        if (ModelBankFormat >= 3) {
            a1 = 2;
        }
        SubmitModel((RroSceneScratchPrefix *)SCRATCHPAD, a1, 0, s5);
    }
    return dist;
}

/* Host decoding of the loaded wire table. The PS1 added the table address to
 * the vertex and command words in place; a 32-bit word cannot hold a host
 * pointer, so the relocated entries are published in a host-owned table that
 * keeps the count/entries layout consumers expect behind RroScene.modelEntries. */
RroScenePointers RroScene;
static RroModelTable *DecodedModelTable;
static s32 DecodedModelTableCapacity;

void RelocateModelTable(RroModelTableWire *table) {
    const u32 *entry = table->words;
    RroEffectMesh *mesh;
    int i = 0;
    int count = table->count;
    if (DecodedModelTable == NULL || count > DecodedModelTableCapacity) {
        RroModelTable *grown = realloc(DecodedModelTable, sizeof(RroModelTable) +
                                       (size_t)(count > 0 ? count : 0) * sizeof(RroEffectMesh));
        if (grown == NULL) {
            abort();
        }
        DecodedModelTable = grown;
        DecodedModelTableCapacity = count > 0 ? count : 0;
    }
    DecodedModelTable->count = count;
    mesh = DecodedModelTable->entries;
    RroScene.modelEntries = mesh;
    ActiveModelVertexCount = count;
    while (i < count) {
        i++;
        mesh->vertices = (const Vec3s8 *)((const u8 *)table + entry[RRO_MODEL_VERTEX_WORD]);
        mesh->vertexCount = entry[1];
        mesh->commands = (const u32 *)((const u8 *)table + entry[RRO_MODEL_COMMAND_WORD]);
        entry += RRO_MODEL_ENTRY_WORDS;
        mesh++;
    }
}

static RroModelBank *ModelBankStorage[RRO_MODEL_BANK_SLOTS];
static u32 ModelBankStorageStreams[RRO_MODEL_BANK_SLOTS];

RroModelBank *RegisterModelBank(const void *image, s32 slot) {
    const RroModelBankWire *wire = image;
    RroModelBank *bank;
    u32 i;

    if (slot < 0 || slot >= RRO_MODEL_BANK_SLOTS) {
        abort();
    }
    bank = ModelBankStorage[slot];
    if (bank == NULL || ModelBankStorageStreams[slot] < wire->streamCount) {
        bank = realloc(bank, sizeof(*bank) + (wire->streamCount ? wire->streamCount : 1) * sizeof(bank->streams[0]));
        if (bank == NULL) {
            abort();
        }
        ModelBankStorage[slot] = bank;
        ModelBankStorageStreams[slot] = wire->streamCount ? wire->streamCount : 1;
    }
    bank->image = (u8 *)image;
    bank->streamCount = wire->streamCount;
    bank->vertices.pointer = (const Vec3s8 *)RrWireAddress(wire, wire->vertices);
    bank->normals.pointer = (const Vec3s8 *)RrWireAddress(wire, wire->normals);
    for (i = 0; i < wire->streamCount; i++) {
        bank->streams[i].pointer = (const u32 *)RrWireAddress(wire, wire->streams[i]);
    }
    ModelBankTable[slot] = bank;
    return bank;
}

void UnrelocateModelBank(RroModelBank *bank, const u8 *base) {
    RroModelBankWire *wire = (RroModelBankWire *)bank->image;
    u32 i;
    wire->vertices = (u32)((const u8 *)bank->vertices.pointer - base);
    wire->normals = (u32)((const u8 *)bank->normals.pointer - base);
    for (i = 0; i < bank->streamCount; i++) {
        wire->streams[i] = (u32)((const u8 *)bank->streams[i].pointer - base);
    }
}

void SelectModelBank(int slot) {
    ModelBankFormat = ModelBankTable[slot]->streamCount;
    RroScene.vertices = ModelBankTable[slot]->vertices.pointer;
    RroScene.normals = ModelBankTable[slot]->normals.pointer;
    RroScene.commands = ModelBankTable[slot]->streams;
}

void SetVramImageSlot(const void *pixels, int slot) {
    ModelBoneMatrixTable[slot] = pixels;
}

void LoadVramImageSlot(int a0) {
    LoadImage(OpeningLensFlareSprites, ModelBoneMatrixTable[a0]);
}

/* Initial values of the globals this module owns. */
GpuRectangle OpeningLensFlareSprites[9] = {{.rect = {.x = 704, .w = 64, .h = 256}}, {.rect = {.y = 256, .w = 514, .h = 770}}, {.rect = {.x = 771, .y = 512, .w = 3}}, {.rect = {.x = 12520, .y = 20496, .w = 10256, .h = 4216}}, {.rect = {.x = -24536, .y = 12304, .w = 4892, .h = -32767}}, {.rect = {.x = 4884, .y = -32767, .w = 4876, .h = -32767}}, {.rect = {.x = 4868, .y = -32767, .w = 4860, .h = -32767}}, {.rect = {.x = 4852, .y = -32767, .w = 4844, .h = -32767}}, {.rect = {.x = 4836, .y = -32767, .w = 4828, .h = -32767}}};
int ActiveModelVertexCount;
s32 ModelBankFormat;
short ModelTrackSegmentResult[2];
RroModelBank *ModelBankTable[16];
const void *ModelBoneMatrixTable[14];
