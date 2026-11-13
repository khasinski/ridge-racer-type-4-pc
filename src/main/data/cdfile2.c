#include "rr/release.h"
#include "rr/track_lighting.h"
#include "rr/menu_selection.h"
#include "rr/load_cursor.h"
#include "rr/matrix_workspace.h"
#include "rr/race_types.h"
#include "types.h"
#include "scratchpad.h"
#include "rr/renderer_setup.h"
#include "rr_asm.h"

/* CD read state machine and the course/car/VRAM resource loaders. */
#include "rr/cd.h"
#include "game/data/bankrec.h"
#include "game/data/cdfile1.h"
#include "game/gfx/poly1.h"
#include "psyq/libcd/dsfinish.h"
#include "game/car/carmove1.h"
#include "game/gfx/model.h"
#include "game/race/objdraw.h"
#include "psyq/libgpu/primatt8.h"
#include "game/gfx/sky.h"
#include "game/data/cdfile2.h"
#include "game/car/bounds.h"
#include "rr/bios_devices.h"
#include "rr/relative_file.h"
#include "game/course/trkintp2.h"
#include "rr/cd_read_request.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "rr/matrix_workspace.h"
#include "psyq/libgpu/gpuenv.h"
#include <stdlib.h>

/* Course image file: two TIM-list offsets. */
enum { COURSE_IMAGES_FIRST = 1, COURSE_IMAGES_SECOND = 2 };
/* Course data file: offset words relative to the file start. */
enum {
    COURSE_DATA_RELATIVE_FILE = 1,
    COURSE_DATA_SKY_TABLE = 2,
    COURSE_DATA_MODEL_BANK = 3,
    COURSE_DATA_MODEL_TABLE = 4,
    COURSE_DATA_TRACK_NODES = 5,
    COURSE_DATA_LIGHTS = 6,
    COURSE_DATA_LANE_LIGHTING = 7,
    COURSE_DATA_TRACK_OBJECTS = 8,
    COURSE_DATA_SEGMENT_VISIBILITY = 9,
    COURSE_DATA_OCTANT_LOOKUP = 10,
    COURSE_DATA_ANIMATED_OBJECTS = 11
};
/* Car file: offset words relative to the file start. */
enum {
    CAR_FILE_RECORD = 1,
    CAR_FILE_BOUNDS = 2,
    CAR_FILE_MODEL_BANK = 3,
    CAR_FILE_MESH_TABLE = 4,
    CAR_FILE_TEXTURE = 5
};

/* DR_MOVE as PsyZ builds it (SetDrawMove, __psyz): host tag + five words. */
typedef struct {
    GpuDmaTag tag;
    uintptr_t code[5];
} GpuMoveImagePacket;

extern s32 CdFileState;
extern s32 CdFileSyncCount;
extern s32 CdFileIndexCurrent;
extern s32 (*CdFileStateHandlers[])();
extern s32 CdFileSectorOffset;
extern s32 CdFileSectors;
extern void *CdActiveReadBuffer;
extern s32 CdFileBytesRead;
extern s32 CdFileSyncResult;
extern CdLocation CdFileLocation;
extern u8 CdCmdParamScratch;
extern CdFileEntry CdFileTable[];
extern unsigned char CarModelCategoryOffsets[];
extern unsigned char CarModelFileIndexTable[];
extern int SceneLoadState;
extern int CdBatchReadMode;
/* Per-node lane-lighting words (two 3-bit light indices per lane). */
extern const s16 *CdStreamSectorCursor;
extern int PendingCarModelFileId;
extern int PendingCarModelDestBuf;
extern int PendingCarModelTeamId;
extern int PendingCarModelTypeId;
extern int PendingCarModelStageId;
extern int PendingCarModelVariant;
extern RroModelTable *CarOffsetTables[];
/* Eight lighting levels per course, selected by the 3-bit lane indices. */
extern short LoadingTriangleBaseVerts[];
s32 CdPosToInt(const u8 *location);

s32 CdFileStateCheckShell(void);
s32 CdFileStateGetTN(void);
s32 CdFileStateReloadDirectory(void);
s32 CdFileStateSetMode(void);
s32 CdFileStateWaitSetMode(void);
s32 CdFileStateWaitShellClose(void);
s32 CdFileStateSetloc(void);
s32 CdFileStateWaitSetloc(void);
s32 CdFileStateStartRead(void);
s32 CdFileStateReadSync(void);
void CdFileFinish(void);
s32 CdFileStateRetry(void);
s32 CdFileReadStart(s32 a0, void *a1);
s32 CdFileReadRange(s32 a0, s32 a1, s32 a2, void *a3);
int ApplyCarWheelTexturePatch(int a0, int a1, int a2, int a3, int a4);
void AllocCarSlotBuffers(const CarModelHeader *a0, int a1);


s32 CdFileStateCheckShell(void) {
    s32 var_v0;

    CdControlB(1, 0, &CdCmdParamScratch);
    var_v0 = CdCmdParamScratch & 0x10;
    if (var_v0 != 0) {
        CdFileState = 5;
        var_v0 = 0;
    }
    return var_v0;
}

s32 CdFileStateGetTN(void) {
    CdControlF(0x13, 0);
    CdFileState = 2;
    return 0;
}

s32 CdFileStateReloadDirectory(void) {
    if (CdFileSyncResult == 2) {
        LoadR4Directory();
        CdFileState = 3;
    } else {
        CdFileState = 1;
    }
    return 0;
}

s32 CdFileStateSetMode(void) {
    CdCmdParamScratch = 0x80;
    CdControlF(0xE, &CdCmdParamScratch);
    CdFileState = 4;
    CdFileSyncCount = 0;
    return 0;
}

s32 CdFileStateWaitSetMode(void) {
    if (CdFileSyncResult == 2) {
        if (++CdFileSyncCount >= 4) {
            CdFileSyncCount = 0;
            if (CdFileIndexCurrent < 0) {
                CdFileState = 0;
            } else {
                CdFileState = 6;
            }
        }
    } else {
        CdFileState = 3;
    }
    return 0;
}

s32 CdFileStateWaitShellClose(void) {
    CdControlB(1, 0, &CdCmdParamScratch);
    if (!(CdCmdParamScratch & 0x10)) {
        CdFileState = 1;
    }
    return 0;
}

s32 CdFileStateSetloc(void) {
    CdControlF(2, CdFileLocation.c);
    CdFileState = 7;
    return 0;
}

s32 CdFileStateWaitSetloc(void) {
    if (CdFileSyncResult == 2) {
        CdFileState = 8;
    } else {
        CdFileState = 0xB;
    }
    return 0;
}

s32 CdFileStateStartRead(void) {
    if (CdRead(CdFileSectors, CdActiveReadBuffer, 0x80)) {
        CdFileState = 9;
    } else {
        CdFileState = 0xB;
    }
    return 0;
}

s32 CdFileStateReadSync(void) {
    s32 r = CdReadSync(1, 0);
    if (r == 0) {
        CdFileState = 0xA;
    } else if (r == -1) {
        CdFileState = 0xB;
    }
    return 0;
}

void CdFileFinish(void) {
    CdFileIndexCurrent = -1;
    CdFileState = 0;
    CdFileBytesRead = CdFileSectors * 0x800U;
}

s32 CdFileStateRetry(void)
{
    CdControlB(1, 0, &CdCmdParamScratch);
    if (CdCmdParamScratch & 0x10) {
        CdFileState = 5;
        return 0;
    }
    CdFileState = 6;
    return 0;
}

s32 CdFileStep(void) {
    if (CdFileState > 0) {
        CdFileSyncResult = CdSync(1, 0);
        if (CdFileSyncResult != 0) {
            return CdFileStateHandlers[CdFileState]();
        }
    }
    return 0;
}

s32 CdFileReadStart(s32 a0, void *a1) {
    if (CdFileState != 0) return 0;
    CdActiveReadBuffer = a1;
    CdFileIndexCurrent = a0;
    CdFileState = 6;
    CdFileSectorOffset = 0;
    CdFileLocation = CdFileTable[a0].location;
    CdFileBytesRead = 0;
    CdFileSectors = CdFileTable[CdFileIndexCurrent].sectors;
    return 1;
}

s32 CdFileReadRange(s32 a0, s32 a1, s32 a2, void *a3) {
    if (CdFileState != 0) return 0;
    CdActiveReadBuffer = a3;
    CdFileIndexCurrent = a0;
    CdFileState = 6;
    CdFileSectorOffset = a1;
    CdIntToPos((CdPosToInt(CdFileTable[a0].location.c) + (u32)a1), CdFileLocation.c);
    CdFileSectors = a2;
    CdFileBytesRead = 0;
    return 1;
}

s32 CdFileBytesLoaded(void) {
    return CdFileBytesRead;
}

/* Host decodings of wire mesh tables, keyed by the wire address and byte
 * span they were decoded from (see rr/model_table.h). */
typedef struct {
    const u8 *wire;
    size_t wireBytes;
    RroModelTable *table;
    size_t capacity;
} DecodedOffsetTable;
enum { DECODED_OFFSET_TABLE_SLOTS = 64 };
static DecodedOffsetTable DecodedOffsetTables[DECODED_OFFSET_TABLE_SLOTS];
static unsigned DecodedOffsetTableNext;

_Static_assert(offsetof(CarModelHeader, litQuads04) == offsetof(RroModelTable, entries),
    "car mesh header is a view of the decoded mesh table");

RroModelTable *RelocateOffsetTable(const void *wire) {
    const RroModelTableWire *source = wire;
    const u8 *bytes = wire;
    s32 count = source->count;
    size_t entries = count > 0 ? (size_t)count : 0;
    size_t wireBytes = sizeof(s32) + entries * RRO_MODEL_ENTRY_WORDS * sizeof(u32);
    DecodedOffsetTable *slot = NULL;
    unsigned i;

    for (i = 0; i < DECODED_OFFSET_TABLE_SLOTS; i++) {
        DecodedOffsetTable *d = &DecodedOffsetTables[i];
        if (d->wire == NULL) {
            continue;
        }
        if (d->wire == bytes) {
            slot = d;
        } else if (d->wire < bytes + wireBytes && bytes < d->wire + d->wireBytes) {
            d->wire = NULL; /* its wire bytes were replaced by this table */
        }
    }
    if (slot == NULL) {
        for (i = 0; i < DECODED_OFFSET_TABLE_SLOTS && slot == NULL; i++) {
            if (DecodedOffsetTables[i].wire == NULL) {
                slot = &DecodedOffsetTables[i];
            }
        }
        if (slot == NULL) {
            slot = &DecodedOffsetTables[DecodedOffsetTableNext++ % DECODED_OFFSET_TABLE_SLOTS];
        }
    }
    if (slot->table == NULL || slot->capacity < entries) {
        RroModelTable *grown = realloc(slot->table, sizeof(RroModelTable) + (entries ? entries : 1) * sizeof(RroEffectMesh));
        if (grown == NULL) {
            abort();
        }
        slot->table = grown;
        slot->capacity = entries ? entries : 1;
    }
    slot->wire = bytes;
    slot->wireBytes = wireBytes;
    slot->table->count = count;
    for (i = 0; i < entries; i++) {
        const u32 *words = &source->words[i * RRO_MODEL_ENTRY_WORDS];
        RroEffectMesh *mesh = &slot->table->entries[i];
        mesh->vertices = (const Vec3s8 *)RrWireAddress(wire, words[RRO_MODEL_VERTEX_WORD]);
        mesh->vertexCount = words[1];
        mesh->commands = (const u32 *)RrWireAddress(wire, words[RRO_MODEL_COMMAND_WORD]);
    }
    return slot->table;
}

void PatchModelBankTexPages(const void *bankImage, s32 textureSlot) {
    const RroModelBankWire *bank = bankImage;
    u32 remainingStreams = bank->streamCount;
    const u32 *stream = bank->streams;
    u32 slot = textureSlot;
    int textureX = ((slot << 6) + 0x180U);
    int pageX = (textureX & 0x3FF) >> 6;
    int clutX = (textureX >> 4) & 0x3F;
    int atlasU = (slot << 6);
    int coordinateMask = ~0xC0;
    int atlasClut = (slot + 0x1028U);

    while (remainingStreams-- != 0) {
        char *commands = (char *)RrWireAddress(bank, *stream++);
        unsigned int header;
        while (header = *(unsigned int *)commands, commands += 4, header != 0) {
            int recordCount = header >> 16;
            switch (header & 0xFFFF) {
            case 0:
                commands += recordCount * 16;
                break;
            case 2:
                commands += recordCount * 24;
                break;
            case 1:
                while (recordCount-- > 0) {
                    RroTextureAttributes *texture = (RroTextureAttributes *)(commands + RR_FIELD_OFFSET(RroRawTexturedQuad, uv08));
                    texture->texturePage = (texture->texturePage & ~0x1F) | pageX;
                    texture->clut = (texture->clut & ~0x403C) | clutX;
                    commands += sizeof(RroRawTexturedQuad);
                }
                break;
            case 3:
                while (recordCount-- > 0) {
                    RroEnvironmentTextureAttributes *attributes = (RroEnvironmentTextureAttributes *)(commands + RR_FIELD_OFFSET(RroEnvMapQuad, uv10));
                    if (attributes->texture.texturePage < 0) {
                        attributes->texture.texturePage = 0xA;
                        attributes->texture.firstUV = ((s16)attributes->texture.firstUV & coordinateMask) | atlasU;
                        attributes->texture.secondUV = ((s16)attributes->texture.secondUV & coordinateMask) | atlasU;
                        attributes->thirdUV = (attributes->thirdUV & coordinateMask) | atlasU;
                        attributes->fourthUV = (attributes->fourthUV & coordinateMask) | atlasU;
                        attributes->texture.clut = (attributes->texture.clut & ~0x403C) | atlasClut;
                        attributes->control |= 0x78;
                    } else {
                        attributes->texture.texturePage = (attributes->texture.texturePage & ~0x1F) | pageX;
                        attributes->texture.clut = (attributes->texture.clut & ~0x403C) | clutX;
                        attributes->control |= 0x78;
                    }
                    commands += sizeof(RroEnvMapQuad);
                }
                break;
            }
        }
    }
}

void PatchOffsetTableTexPages(RroModelTable *table, s32 textureSlot) {
    /* PAL counters and tile coordinates wrap modulo 32 bits. */
    u32 remaining = table->count;
    RroEffectMesh *mesh = table->entries;
    u32 slot = textureSlot;
    int textureX = ((slot << 6) + 0x180U);
    int pageX = (textureX & 0x3FF) >> 6;
    int clutX = (textureX >> 4) & 0x3F;
    int pageMask = ~0x1F;
    int clutMask = ~0x403C;

    while ((s32)remaining-- > 0) {
        char *commands = (char *)mesh->commands;
        unsigned int header;
        mesh++;
        while (header = *(unsigned int *)commands, commands += 4, header != 0) {
            int recordCount = header >> 16;
            switch (header & 0xFFFF) {
            case 7:
                commands += recordCount * 20;
                break;
            case 8:
                commands += recordCount * 28;
                break;
            case 1:
                while (recordCount-- > 0) {
                    RroTextureAttributes *attributes = (RroTextureAttributes *)(commands + RR_FIELD_OFFSET(RroTexturedQuadPrefix, uv0C));
                    attributes->texturePage = (attributes->texturePage & pageMask) | pageX;
                    attributes->clut = (attributes->clut & clutMask) | clutX;
                    commands += sizeof(RroTexturedQuadPrefix);
                }
                break;
            case 9:
                commands += recordCount * 20;
                break;
            case 5:
            case 10:
                commands += recordCount * 24;
                break;
            case 3:
                commands += recordCount * 20;
                break;
            case 4:
                commands += recordCount * 12;
                break;
            case 6:
                commands += recordCount * 8;
                break;
            case 11:
                while (recordCount-- > 0) {
                    RroTextureAttributes *attributes = (RroTextureAttributes *)(commands + 20);
                    attributes->texturePage = (attributes->texturePage & pageMask) | pageX;
                    attributes->clut = (attributes->clut & clutMask) | clutX;
                    commands += 0x20;
                }
                break;
            case 14:
                while (recordCount-- > 0) {
                    RroTextureAttributes *attributes = (RroTextureAttributes *)(commands + 16);
                    attributes->texturePage = (attributes->texturePage & pageMask) | pageX;
                    attributes->clut = (attributes->clut & clutMask) | clutX;
                    commands += 0x1C;
                }
                break;
            case 15:
                while (recordCount-- > 0) {
                    RroTextureAttributes *attributes = (RroTextureAttributes *)(commands + 20);
                    attributes->texturePage = (attributes->texturePage & pageMask) | pageX;
                    attributes->clut = (attributes->clut & clutMask) | clutX;
                    commands += 0x24;
                }
                break;
            }
        }
    }
}

/* Copies the selected wheel texture variant over the car's texture slot in
 * VRAM: a MoveImage packet followed by an E6 mask-reset command. */
int ApplyCarWheelTexturePatch(int teamId, int typeId, int stageId, int variant, int destSlot)
{
    GpuScratchPointers *gpu = RR_GPU_SCRATCH;
    GpuRectangle source;
    GpuMoveImagePacket *move;
    GpuCommandPacket *mask;
    int entry, row, category;

    if (teamId == 2 && typeId == 0 && stageId == 0x13)
        return 0x13;
    entry = CarModelFileIndexTable[typeId * 16 + variant];
    row = (entry >> 3) * 20;
    category = CarModelCategoryOffsets[stageId];
    if (category >= 4) {
        if (category >= 7)
            row += 0x78;
        else
            row += 0x3C;
    }
    source.rect.x = (entry & 7) * 0x10 + 0x340;
    source.rect.y = row;
    source.rect.w = 0x10;
    source.rect.h = 0x14;
    move = (GpuMoveImagePacket *)gpu->cursor;
    SetDrawMove((void *)move, (void *)&source, (destSlot << 6) + 0x1B0, 0xD8);
    RR_GPU_ADD_PRIM(gpu->order, move, 5);
    gpu->cursor += sizeof(*move);
    mask = (GpuCommandPacket *)gpu->cursor;
    mask->command = 0xE6000000;
    RR_GPU_ADD_PRIM(gpu->order, mask, 1);
    gpu->cursor += sizeof(*mask);
    return 0;
}

void AllocCarSlotBuffers(const CarModelHeader *a0, int a1)
{
    int s1;
    u8 *cursor;
    int v1;
    u32 historyBytes, primaryBytes;
    CarSlotBufferC8 *s0;
    CarTrailBuffer *p;

    s0 = &CarSlotBuffers[a1 * 4];
    cursor = LoadCursor;
    primaryBytes = ((a0->primary10.vertexCount + 2) / 3) * (3 * sizeof(RroProjectedVertex));
    historyBytes = ((a0->history34.vertexCount + 2) / 3) * (3 * sizeof(RroProjectedVertex));
    s1 = 4;
    do {
        p = s0->main14;
        v1 = 3;
        s0->model = a0;
        s0->color08 = 0;
        s0->mainCount0C = 0;
        s0->mainIndex10 = 0;
        s0->secondCount34 = 0;
        s0->secondIndex38 = 0;
        s0->phase5C = 0;
        s0->intensity60 = 0;
        s0->intensity64 = 0;
        s0->historyCount68 = 0;
        s0->historyIndex6C = 0;
        s0->overlay70 = 0;
        do {
            p->vertices = cursor;
            p++;
            v1 -= 1;
            cursor += primaryBytes;
        } while (v1 >= 0);
        p = s0->second3C;
        v1 = 3;
        do {
            p->vertices = cursor;
            p++;
            v1 -= 1;
            cursor += historyBytes;
        } while (v1 >= 0);
        p = s0->history84;
        v1 = 7;
        do {
            p->vertices = cursor;
            p++;
            v1 -= 1;
            cursor += historyBytes;
        } while (v1 >= 0);
        s0++;
        s1 -= 1;
    } while (s1 > 0);
    LoadCursor = cursor;
    if (CarBoundsTable[a1]->wheelRotationAxis == 2) {
        s0 = &CarSlotBuffers[a1 * 4];
        s1 = 4;
        do {
            s1 -= 1;
            s0->sparksC4 = LoadCursor;
            PsyqBzero(LoadCursor, sizeof(RroSpark) * 16);
            s0++;
            LoadCursor = (u8 *)LoadCursor + sizeof(RroSpark) * 16;
        } while (s1 > 0);
    }
}

int RequestSceneLoad(int a0) {
    if (SceneLoadState != 0) {
        return 0;
    }
    CdBatchReadMode = a0;
    return SceneLoadState = 1;
}

int SceneLoadStep(void)
{
    u8 *file;
    s32 bytes;
    RelativeHeader *relativeFile;
    u8 *trackObjects;

    /* Each header word is an offset from the file start. The files stay as
     * loaded; resolved addresses go to the subsystems that own them. */
    switch (SceneLoadState) {
    case 1:
        if (CdFileReadStart(GetCourseFileIndex(CdBatchReadMode), LoadCursor) == 0)
            return 0;
        SceneLoadState = 2;
    case 2:
        if (CdFileBytesLoaded() == 0)
            return 0;
        file = LoadCursor;
        LoadTimList(RrWireTableAddress(file, COURSE_IMAGES_FIRST));
        LoadTimList(RrWireTableAddress(file, COURSE_IMAGES_SECOND));
        SceneLoadState = 3;
    case 3:
        if (CdFileReadStart(GetCourseFileIndex(CdBatchReadMode) + 1, LoadCursor) == 0)
            return 0;
        SceneLoadState = 4;
    case 4:
        bytes = CdFileBytesLoaded();
        if (bytes == 0)
            return 0;
        file = LoadCursor;
        relativeFile = (RelativeHeader *)RrWireTableAddress(file, COURSE_DATA_RELATIVE_FILE);
        trackObjects = RrWireTableAddress(file, COURSE_DATA_TRACK_OBJECTS);
        if (CdBatchReadMode == 7) {
            InitAnimatedTrackObjects(RrWireTableAddress(file, COURSE_DATA_ANIMATED_OBJECTS));
        }
        LoadCursor = file + bytes;
        RelocateRelativeFile(relativeFile);
        /* The sky table stays a wire table; DrawTrackScenery resolves it. */
        TrackSkyTable = (s32 *)RrWireTableAddress(file, COURSE_DATA_SKY_TABLE);
        RegisterModelBank(RrWireTableAddress(file, COURSE_DATA_MODEL_BANK), 4);
        RelocateModelTable((RroModelTableWire *)RrWireTableAddress(file, COURSE_DATA_MODEL_TABLE));
        SetTrackNodeData((char *)RrWireTableAddress(file, COURSE_DATA_TRACK_NODES));
        SetLightTable((TrackLighting2C *)RrWireTableAddress(file, COURSE_DATA_LIGHTS));
        CdStreamSectorCursor = (const s16 *)RrWireTableAddress(file, COURSE_DATA_LANE_LIGHTING);
        LoadCursor = (u8 *)LoadCursor + LoadTrackObjects(trackObjects, LoadCursor);
        SetSegmentVisibilityTable((short *)RrWireTableAddress(file, COURSE_DATA_SEGMENT_VISIBILITY));
        SetOctantLookupTable(RrWireTableAddress(file, COURSE_DATA_OCTANT_LOOKUP));
        SceneLoadState = 0;
        return 1;
    }
    return 0;
}

int RequestCarLoad(int a0, int a1, int a2, int a3, int a4) {
    int r;
    if (SceneLoadState != 0) {
        return 0;
    }
    PendingCarModelFileId = GetCarFileIndex(a0, a1, a2, a3);
    r = 1;
    PendingCarModelDestBuf = a4;
    PendingCarModelTeamId = a1;
    PendingCarModelTypeId = a0;
    PendingCarModelStageId = a2;
    PendingCarModelVariant = a3;
    SceneLoadState = r;
    return r;
}

s32 CarLoadStep(void) {
    GpuRectangle rect;
    u8 *file;
    u8 *bank;
    u8 *texture;
    RroModelTable *meshes;

    switch (SceneLoadState) {
    case 1:
        if (CdFileReadStart(PendingCarModelFileId, LoadCursor) == 0) {
            return 0;
        }
        SceneLoadState = 2;
    case 2:
        if (CdFileBytesLoaded() == 0) {
            return 0;
        }
        file = LoadCursor;
        bank = RrWireTableAddress(file, CAR_FILE_MODEL_BANK);
        texture = RrWireTableAddress(file, CAR_FILE_TEXTURE);
        SetCarRecordPtr((CarRecord *)RrWireTableAddress(file, CAR_FILE_RECORD), PendingCarModelDestBuf);
        CarBoundsTable[PendingCarModelDestBuf] = (CarBounds *)RrWireTableAddress(file, CAR_FILE_BOUNDS);
        PatchModelBankTexPages(bank, PendingCarModelDestBuf);
        RegisterModelBank(bank, PendingCarModelDestBuf);
        meshes = RelocateOffsetTable(RrWireTableAddress(file, CAR_FILE_MESH_TABLE));
        CarOffsetTables[PendingCarModelDestBuf] = meshes;
        PatchOffsetTableTexPages(meshes, PendingCarModelDestBuf);
        rect.rect.x = (PendingCarModelDestBuf << 6) + 0x180;
        rect.rect.y = 0;
        rect.rect.w = 0x40;
        rect.rect.h = 0x100;
        LoadImage(&rect, texture);
        DrawSync(0);
        LoadCursor = texture;
        AllocCarSlotBuffers((const CarModelHeader *)CarOffsetTables[PendingCarModelDestBuf], PendingCarModelDestBuf);
        if (PendingCarModelVariant != 0) {
            ApplyCarWheelTexturePatch(PendingCarModelTeamId, PendingCarModelTypeId, PendingCarModelStageId, PendingCarModelVariant - 1, PendingCarModelDestBuf);
        }
        SceneLoadState = 0;
        return 1;
    }
    return 0;
}

int RequestVramImageLoad(void) {
    if (SceneLoadState != 0) return 0;
    return SceneLoadState = 1;
}

int VramImageLoadStep(void) {
    GpuRectangle rect;
    switch (SceneLoadState) {
    case 1:
        if (CdFileReadStart(GetR4FileIndex(0, 0), LoadCursor) == 0) return 0;
        SceneLoadState = 2;
    case 2:
        if (CdFileBytesLoaded() == 0) return 0;
        rect.rect.x = 0x340;
        rect.rect.w = 0x80;
        rect.rect.y = 0;
        rect.rect.h = 0xB4;
        LoadImage(&rect, LoadCursor);
        DrawSync(0);
        SceneLoadState = 0;
        return 1;
    default:
        return 2;
    }
}

void SetupRenderer(s32 shift) {
    RendererScratch *scratch;
    ((RendererScratch*)SCRATCHPAD)->depthShift60=shift;
    scratch=(RendererScratch*)SCRATCHPAD;
    scratch->renderFlags64=MirrorRenderFlag;
    DefaultScreenClearColor.b=128;
    DefaultScreenClearColor.g=128;
    DefaultScreenClearColor.r=128;
    DefaultScreenClearColor.code=0x2C;
    LoadingScreenClearColor.b=255;
    LoadingScreenClearColor.g=255;
    LoadingScreenClearColor.r=255;
    LoadingScreenClearColor.code=0x3C;
    scratch->viewport6C.rect.w=320;
    scratch->viewport6C.rect.x=0;
    scratch->viewport6C.rect.y=0;
    scratch->viewport6C.rect.h=GameReleaseProfile->screenHeight;
}

/* GTE INTPL of one light row: FC = far entry, IR = near entry, IR0 = blend. */
static void InterpolateLightVector(const PreviewVector8 *farVector, const PreviewVector8 *nearVector) {
    RR_GTE_SETFC(farVector->x, farVector->y, farVector->z);
    RR_GTE_MTC2_IR123_INTPL(nearVector->x, nearVector->y, nearVector->z);
}

/* GTE DPCS of one colour: FC = far colour << 4, RGB = near colour word. */
static void InterpolateLightColor(const PreviewColor4 *farColor, const PreviewColor4 *nearColor) {
    u32 nearWord;
    RR_GTE_SETFC(farColor->red << 4, farColor->green << 4, farColor->blue << 4);
    nearWord = nearColor->red | (nearColor->green << 8) | (nearColor->blue << 16) | ((u32)nearColor->pad03 << 24);
    GTE_MTC2_RGB_DPCS(nearWord);
}

int ComputeCarSurfaceLighting(const RaceVehicle *vehicle, int lightIndex) {
    int *work = (int *)(SCRATCHPAD + 0x7C);
    const s16 *lane = &CdStreamSectorCursor[vehicle->trackNodeF0];
    const s16 *levels = &LoadingTriangleBaseVerts[CourseIndex * 8];
    s32 lateral = vehicle->lateralOffset172 << 3;
    s32 shifted = lateral + 0x2000;
    s32 lateralFraction = shifted & 0xFFF;
    s32 trackFraction = vehicle->trackFraction170 << 2;
    s32 shift = (shifted / 0x1000) * 3;
    s32 currentNear = levels[(lane[0] >> shift) & 7];
    s32 currentFar = levels[(lane[0] >> (shift + 3)) & 7];
    s32 nextNear = levels[(lane[1] >> shift) & 7];
    s32 nextFar = levels[(lane[1] >> (shift + 3)) & 7];
    s32 blend;
    const PreviewLighting40 *farLight, *nearLight;
    u16 *lightColor = (u16 *)(SCRATCHPAD + 0xB4);

    work[0] = lightIndex;
    /* Bilinear lane/track blend into IR0. */
    blend = nextNear;
    RR_GTE_INTPL_TRACK_TABLE(currentFar, blend, nextFar, currentNear, lateralFraction, trackFraction);

    work[2] = (lightIndex >> 4) & 0xF;
    work[1] = lightIndex & 0xF;
    farLight = &TrackLightTable[work[2]].lighting04;
    nearLight = &TrackLightTable[work[1]].lighting04;

    /* Light directions into the three LightMatrix rows. */
    InterpolateLightVector(&farLight->matrix.fields.vectors[0], &nearLight->matrix.fields.vectors[0]);
    RR_GTE_STORE_IR_S16(&LightMatrix.halves[0]);
    InterpolateLightVector(&farLight->matrix.fields.vectors[1], &nearLight->matrix.fields.vectors[1]);
    RR_GTE_STORE_IR_S16(&LightMatrix.halves[3]);
    InterpolateLightVector(&farLight->matrix.fields.vectors[2], &nearLight->matrix.fields.vectors[2]);
    RR_GTE_STORE_IR_S16(&LightMatrix.halves[6]);

    /* Ambient into BK, light colours into the scratch light-colour matrix. */
    InterpolateLightColor((const PreviewColor4 *)farLight->ambient, (const PreviewColor4 *)nearLight->ambient);
    RR_GTE_BK_FROM_IR();
    InterpolateLightColor(&farLight->matrix.fields.colors[0], &nearLight->matrix.fields.colors[0]);
    RR_GTE_ST3_COL(lightColor);
    InterpolateLightColor(&farLight->matrix.fields.colors[1], &nearLight->matrix.fields.colors[1]);
    RR_GTE_ST3_COL(lightColor + 1);
    InterpolateLightColor(&farLight->matrix.fields.colors[2], &nearLight->matrix.fields.colors[2]);
    RR_GTE_ST3_COL(lightColor + 2);
    RR_GTE_SETLCM(lightColor);
    return work[1];
}

/* Initial values of the globals this module owns. */
s32 CdFileState;
s32 CdFileSyncCount;
int SceneLoadState;
s32 CdFileIndexCurrent = -1;
s32 (*CdFileStateHandlers[12])() = {(int (*)())CdFileStateCheckShell, (int (*)())CdFileStateGetTN, (int (*)())CdFileStateReloadDirectory, (int (*)())CdFileStateSetMode, (int (*)())CdFileStateWaitSetMode, (int (*)())CdFileStateWaitShellClose, (int (*)())CdFileStateSetloc, (int (*)())CdFileStateWaitSetloc, (int (*)())CdFileStateStartRead, (int (*)())CdFileStateReadSync, (int (*)())CdFileFinish, (int (*)())CdFileStateRetry};
unsigned char CarModelCategoryOffsets[20] = {0, 0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 7, 8, 9};
unsigned char CarModelFileIndexTable[64] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0, 1, 2, 3, 4, 5, 6, 8, 0xA, 0xB, 0xC, 0x11, 0x12, 0x14, 0x16, 0x17, 0, 2, 3, 4, 6, 7, 8, 0xC, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13, 0x14, 0x17, 0, 2, 3, 4, 5, 7, 8, 0xA, 0xB, 0xC, 0xD, 0xE, 0x11, 0x12, 0x14, 0x15};
short LoadingTriangleBaseVerts[64] = {0, 0, 2048, 4096, 4096, 4096, 4096, 4096, 0, 0, 2048, 4096, 4096, 4096, 4096, 4096, 0, 0, 4096, 4096, 4096, 4096, 4096, 4096, 0, 0, 2048, 4096, 4096, 4096, 4096, 4096, 0, 0, 2048, 4096, 4096, 4096, 4096, 4096, 0, 0, 2048, 4096, 4096, 4096, 4096, 4096, 0, 0, 2048, 4096, 4096, 4096, 4096, 4096, 0, 0, 4096, 4096, 4096, 4096, 4096, 4096};
PreviewMatrixWorkspace LightMatrix;
GpuColorCommand LoadingScreenClearColor;
GpuColorCommand DefaultScreenClearColor;
int CdBatchReadMode;
int PendingCarModelFileId;
int PendingCarModelDestBuf;
int PendingCarModelTeamId;
int PendingCarModelTypeId;
int PendingCarModelStageId;
int PendingCarModelVariant;
s32 CdFileSectorOffset;
s32 CdFileSectors;
CdLocation CdFileLocation;
RroModelTable *CarOffsetTables[4];
void *CdActiveReadBuffer;
s32 CdFileBytesRead;
s32 CdFileSyncResult;
u8 CdCmdParamScratch;
/* R4.BIN has 950 entries in the PAL release and 969 in the Japanese one. */
CdFileEntry CdFileTable[RR_R4_ENTRIES_MAX];
const s16 *CdStreamSectorCursor;
s32 *TrackSkyTable;
