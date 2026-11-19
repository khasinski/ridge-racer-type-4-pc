#include <string.h>
#include "game/gfx/poly1.h"
#include "rr/player_cars.h"
#include "game/math/rotmat.h"
#include "rr/timing.h"
#include "game/camera/viewini.h"
#include "rr_asm.h"
#include "rr/projection_state.h"
#include "psyq/libgte/gtemat5.h"
#include "rr/camera_matrices.h"
#include "psyq/libgte/gtemat4.h"
#include "psyq/libgte/gtemat.h"
#include "types.h"
#include "scratchpad.h"
#include "game/camera/viewcam.h"
#include "game/gfx/model.h"
#include "psyq/libgpu/addprim3.h"
#include "psyq/libgpu/addprim4.h"
#include "psyq/libgpu/primatt5.h"
#include "psyq/libgpu/primatt7.h"
#include "psyq/libgte/gtecolo3.h"
#include "psyq/libgte/gtemat3.h"
#include "psyq/libgte/gtemat7.h"
#include "rr/race_setup.h"
#include "rr/matrix_workspace.h"
#include "rr/audio_loop.h"
#include "rr/memory.h"
#include "rr/overlay661_init.h"
#include "rr/hud_number.h"
#include "rr/relative_file.h"
#include "rr/frame_environment.h"
#include "game/demo/splash.h"
#include "rr/animated_glyph.h"
#include "game/data/globals.h"
#include "game/car/carbound.h"
#include "game/race/objdraw.h"
#include "rr/driver_control.h"
#include "game/menu/mnchoice.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include <stdlib.h>




struct PreviewBytes8 { unsigned char c[8]; };

/* Host table of the counted offset table's resolved addresses. */
extern u8 **ActivePolyStripPtr;
extern s32 RearViewMirrorVisible;
extern int RaceState;
#include "psyq/libc/rand.h"
extern short PolyStripTempCoords[];
/* Host storage of the animated course 7 object pools (InitAnimatedTrackObjects). */
static AnimatedTrackArena AnimatedTrackPools;
extern s32 SquareRoot0(s32 value);
extern short *SegmentVisibilityTable;
extern unsigned char *OctantLookupTable;
extern struct PreviewBytes8 EmblemPreviewClutRect0;
extern struct PreviewBytes8 EmblemPreviewClutRect1;
extern struct PreviewBytes8 EmblemPreviewImageRect0;
extern struct PreviewBytes8 EmblemPreviewStripRect;
extern struct PreviewBytes8 EmblemPreviewImageRect1;

void RelocateCountedOffsetTable(int *a0);
int SetupRearViewMirror(void);
void RestoreMainViewAfterMirror(void);
GpuTilePacket *EmitRearViewMirrorFrameTile(GpuTilePacket *a0);
void UpdateAndDrawRearViewMirror(int arg0);
s32 GetSegmentVisibilityTableSize(void);
s32 GetOctantLookupTableSize(void);

/* The PS1 added the table address to each of the count*8 offset words in
 * place. 32-bit words cannot hold host pointers, so the resolved addresses
 * are published in a host table; the loaded image stays unmodified. */
static s32 ActivePolyStripCapacity;

void RelocateCountedOffsetTable(int *a0) {
    const u32 *p = (const u32 *)(a0 + 1);
    int i = 0;
    int n = *a0 * 8;
    if (ActivePolyStripPtr == NULL || n > ActivePolyStripCapacity) {
        u8 **grown = realloc(ActivePolyStripPtr, (size_t)(n > 0 ? n : 1) * sizeof(u8 *));
        if (grown == NULL) {
            abort();
        }
        ActivePolyStripPtr = grown;
        ActivePolyStripCapacity = n > 0 ? n : 0;
    }
    while (i < n) {
        ActivePolyStripPtr[i] = RrWireAddress(a0, p[i]);
        i++;
    }
}

void InitRearViewMirrorState(void) {
    RearViewMirrorEnabled = 1;
    RearViewMirrorY = -0x24;
    RearViewMirrorVisible = 0;
}

int SetupRearViewMirror(void) {
    MatrixWords32 rotation;
    int r = 0;
    ViewMatrixScratch48 *scratch = (ViewMatrixScratch48 *)SCRATCHPAD;
    if (RearViewMirrorVisible != 0) {
        if (RearViewMirrorEnabled != 0) {
            if (ChaseViewEnabled == 0) {
                if (GameModeType != 0) {
                    r = RaceState == 2;
                }
            }
        }
    }
    if (r != 0) {
        PolyWorldTransformMtx = scratch->matrix28;
        scratch->matrix28 = RearViewMirrorMatrix;
        scratch->projection.pose08.angles10.xyz.y = (scratch->projection.pose08.angles10.xyz.y + 0x800U);
        BuildRotMatrixY(&rotation, 0x800);
        MulMatrix0(
            &rotation,
            &CameraViewMatrix,
            &CameraViewMatrix);
        MulMatrix0(
            &rotation,
            &CameraPitchRollMatrix,
            &CameraPitchRollMatrix);
        BuildRotMatrixX(&rotation, PlayerCars[0].renderAngleX1AC * 2); /* SunLightPitchAngle 0x800AC0A4 */
        MulMatrix2(&rotation, &CameraViewMatrix);
        MulMatrix2(&rotation, &CameraPitchRollMatrix);
        CameraViewMatrix.halves[0] = -CameraViewMatrix.halves[0];
        CameraViewMatrix.halves[1] = -CameraViewMatrix.halves[1];
        CameraViewMatrix.halves[2] = -CameraViewMatrix.halves[2];
        CameraPitchRollMatrix.halves[0] = -CameraPitchRollMatrix.halves[0];
        CameraPitchRollMatrix.halves[1] = -CameraPitchRollMatrix.halves[1];
        CameraPitchRollMatrix.halves[2] = -CameraPitchRollMatrix.halves[2];
    }
    return r;
}

void RestoreMainViewAfterMirror(void) {
    ViewMatrixScratch48 *scratch = (ViewMatrixScratch48 *)SCRATCHPAD;
    scratch->projection.pose08.angles10.xyz.y = (scratch->projection.pose08.angles10.xyz.y - 0x800U);
    scratch->matrix28 = PolyWorldTransformMtx;
}

GpuTilePacket *EmitRearViewMirrorFrameTile(GpuTilePacket *a0) {
    GpuDmaTag *ordering = RR_GPU_SCRATCH->order;
    a0->code = 0x60; /* SetTile: three payload words, TILE command */
    a0->x = 0x60;
    a0->w = 0x80;
    a0->r = 0;
    a0->g = 0;
    a0->b = 0;
    a0->h = 0x20;
    a0->y = RearViewMirrorY - 2;
    RR_GPU_ADD_PRIM(ordering, a0, 3);
    return a0 + 1;
}


void UpdateAndDrawRearViewMirror(int arg0) {
    if (arg0 >= 0x169) {
        RearViewMirrorVisible = 1;
    }
    if (RearViewMirrorVisible != 0) {
        if (RearViewMirrorEnabled != 0) {
            if (RearViewMirrorY < 0x12) {
                RearViewMirrorY += 1;
            }
        } else if (RearViewMirrorY >= -0x23) {
            RearViewMirrorY -= 1;
        }
        if (SetupRearViewMirror() != 0) {
            RR_GPU_SCRATCH->cursor = (u8 *)EmitRearViewMirrorFrameTile((GpuTilePacket *)RR_GPU_SCRATCH->cursor);
            SetupViewport(1);
            DrawTrackSceneryForPlayer(0);
            RenderMirrorCourseView(0);
            DrawTrackObjects(2);
            DrawCars(1);
            RestoreMainViewAfterMirror();
        }
    }
}

void InitAnimatedTrackObjects(void *memory)
{
    AnimatedLinePacket *packet;
    int packetIndex,objectIndex;
    int buffer;
    const u32 *offsets;
    const AnimatedTrackColorKey **firstKeys,**secondKeys;
    const u8 *colorBase;
    AnimatedTrackObject *state;
    /* A fresh load of the file gives zeroed pools (rr/animated_track_types.h):
     * the PS1 calls no memset here, so this host clear takes no PS1 time
     * (the BIOS memset's 132 clocks a byte charged 17M clocks, half a
     * second, for the host pools). */
    memset(&AnimatedTrackPools, 0, sizeof(AnimatedTrackPools));
    TrackParticlePoolPtr = AnimatedTrackPools.particles;
    packet = AnimatedTrackPools.packets;
    buffer = 0;
    AnimatedTrackObjPool = AnimatedTrackPools.objects;
    do {
        objectIndex = 0;
        do {
            state = AnimatedTrackObjPool + objectIndex;
            state->headsE8[buffer] = packet;
            state->tailsF0[buffer] = 0;
            SetLineG2(packet);
            packet++;
            packetIndex = 1;
            do {
                SetLineG2(packet);
                (packet - 1)->tag.address = (uintptr_t)packet; /* CatPrim */
                packetIndex++;
                packet++;
            } while (packetIndex < 255);
            objectIndex++;
        } while (objectIndex < 5);
        buffer++;
    } while (buffer < 2);
    offsets = (const u32 *)((const u8 *)memory + RR_ANIMATED_TRACK_WIRE_KEYS);
    colorBase = (const u8 *)offsets;
    buffer = 0;
    secondKeys = TrackColorKeysSecondary;
    firstKeys = TrackColorKeysPrimary;
    do {
        *firstKeys = (const AnimatedTrackColorKey *)(colorBase + (s32)*offsets);
        offsets++;
        buffer++;
        *secondKeys = (const AnimatedTrackColorKey *)(colorBase + (s32)*offsets);
        offsets++;
        firstKeys++;
        secondKeys++;
    } while (buffer < 9);
}

void SetAnimatedTrackObjectState(int index, int flags, const s32 *position, int style) {
    AnimatedTrackObject *object = AnimatedTrackObjPool + index;
    int reset;
    int colorIndex;
    if (flags & 1) {
        if (position != 0) {
            object->positionC8.x = position[0];
            object->positionC8.y = position[1];
            object->positionC8.z = position[2];
        }
    }
    if (flags & 2) {
        if (style >= 0) object->style3C = style;
    }
    reset = flags & 4;
    if (reset) {
        object->capacity08 = 0xFF;
        object->field0C = 5;
        object->scale20 = 0x1000;
        object->scale24 = 0x1000;
        object->startFrame28 = 0x20;
        object->endFrame2C = 0xB5;
        object->advance30 = 1;
        object->activeCount04 = 0;
        object->initialized10 = 0;
        object->frame14 = 0;
        object->colorTick18 = 0;
        object->colorTick1C = 0;
        object->keyIndex34 = 0;
        object->keyIndex38 = 0;
        object->particles00 = TrackParticlePoolPtr[index];
        object->keys40 = TrackColorKeysPrimary[object->style3C];
        object->keys44 = TrackColorKeysSecondary[object->style3C];
        for (colorIndex = 0; colorIndex < 16; colorIndex++) {
            object->palette48.words[colorIndex] = 0x52000000;
            object->palette88.words[colorIndex] = 0x52000000;
        }
        object->anglesD8.x = 0;
        object->anglesD8.y = 0;
        object->anglesD8.z = 0;
        object->tailsF0[0] = 0;
        object->tailsF0[1] = 0;
        PolyStripTempCoords[0] = 0x420;
        PolyStripTempCoords[1] = 0x5D0;
        PolyStripTempCoords[2] = 0x610;
        PsyqSrand(VSync(1));
    }
}


void SetupAnimatedObjectMatrices(AnimatedTrackObject *object) {
    MatrixWords32 rotation, axisRotation;
    Vec3i transformedPosition;
    BuildRotMatrixY(&rotation, object->anglesD8.y);
    BuildRotMatrixX(&axisRotation, object->anglesD8.x);
    MulMatrix2(&axisRotation, &rotation);
    BuildRotMatrixZ(&axisRotation, object->anglesD8.z);
    MulMatrix2(&axisRotation, &rotation);
    ApplyMatrixLV(&HalfTransViewMatrix, &object->positionC8, &transformedPosition);
    rotation.w[5] = (s32)((u32)HalfTransViewMatrix.w[5] + transformedPosition.x) >> 1;
    rotation.w[6] = (s32)((u32)HalfTransViewMatrix.w[6] + transformedPosition.y) >> 1;
    rotation.w[7] = (s32)((u32)HalfTransViewMatrix.w[7] + transformedPosition.z) >> 1;
    SetTransMatrix(rotation.w);
    MulMatrix(&rotation, &HalfTransViewMatrix);
    SetRotMatrix(rotation.w);
}

void DrawAnimatedTrackObject(AnimatedTrackScratch *scratch, int objectIndex, int advance) {
    int toCamera[3];
    MatrixWords32 rotation;
    MatrixWords32 axisRotation;
    Vec3s8 direction;
    int farRed, farGreen, farBlue;
    AnimatedTrackObject *object;
    AnimatedTrackParticle *particle;
    const AnimatedTrackColorKey *key;
    int colorIndex;
    AnimatedLinePacket *cursor;
    GpuDmaTag *ordering;
    const s32 *cameraPosition = ((ProjectionScratchPrefix *)SCRATCHPAD)->pose08.position00.words;
    AnimatedLinePacket *previousTail;
    int startFrame;
    int visibleFrames;
    AnimatedLinePacket *last;
    int fraction;
    int initializedThisCall;

    PsyqMemset(&direction, 0, sizeof(direction));
    direction.fields.z = 0x100;
    object = AnimatedTrackObjPool + objectIndex;
    ordering = RR_GPU_SCRATCH->order + 0x240;
    object->advance30 = advance;
    colorIndex = (object->frame14 - object->startFrame28) & 0xF;
    if (object->initialized10 < object->capacity08) {
        initializedThisCall = 0;
        while (initializedThisCall < 10) {
            if (!(object->initialized10 < object->capacity08)) break;
            particle = object->particles00 + object->initialized10;
            particle->position00.x = 0;
            particle->position00.y = 0;
            particle->position00.z = 0;
            particle->position08.x = 0;
            particle->position08.y = 0;
            particle->position08.z = 0;
            particle->colorPhase20 = PsyqRand() & 0xF;
            BuildRotMatrixY(&rotation, PsyqRand() & 0xFFF);
            initializedThisCall++;
            BuildRotMatrixX(&axisRotation, PsyqRand() & 0xFFF);
            MulMatrix2(&axisRotation, &rotation);
            BuildRotMatrixZ(&axisRotation, PsyqRand() & 0xFFF);
            MulMatrix2(&axisRotation, &rotation);
            ApplyMatrixSV(&rotation, &direction, &particle->step10.x);
            particle->step18 = particle->step10;
            object->initialized10 = object->initialized10 + 1;
        }
    } else {
        previousTail = object->tailsF0[DrawBufIndex];
        cursor = object->headsE8[DrawBufIndex];
        if (previousTail != 0) previousTail->tag.address = (uintptr_t)(previousTail + 1); /* CatPrim */
        RR_GTE_SAVE_FC(&farRed, &farGreen, &farBlue);
        if (object->frame14 > object->endFrame2C) {
            SetAnimatedTrackObjectState(objectIndex, 4, 0, -1);
        } else {
            startFrame = object->startFrame28;
            if (object->frame14 < startFrame) {
                if (object->advance30) object->frame14 = object->frame14 + 1;
                object->activeCount04 = 0;
                object->tailsF0[DrawBufIndex] = 0;
            } else {
                visibleFrames = object->frame14 - startFrame + 1;
                object->activeCount04 = visibleFrames;
                if (visibleFrames >= 0x10) object->activeCount04 = object->capacity08;
                else object->activeCount04 = visibleFrames * (object->capacity08 / 16);
                if (object->advance30) object->frame14 = object->frame14 + 1;
                key = object->keys40 + object->keyIndex34;
                if (key->duration == -1) {
                    object->palette48.values[colorIndex] = key->color;
                } else {
                    RR_GTE_SETFC(key[1].color.r << 4, key[1].color.g << 4, key[1].color.b << 4);
                    fraction = (s32)(object->colorTick18 * 0x1000U) / key->duration;
                    DpqColor(&key->color, fraction, &object->palette48.values[colorIndex]);
                    if (object->advance30) {
                        object->colorTick18 = (object->colorTick18 + 1U);
                        if (object->colorTick18 >= key->duration) {
                            object->colorTick18 = 0;
                            object->keyIndex34 = object->keyIndex34 + 1;
                        }
                    }
                }
                key = object->keys44 + object->keyIndex38;
                if (key->duration == -1) {
                    object->palette88.values[colorIndex] = key->color;
                } else {
                    RR_GTE_SETFC(key[1].color.r << 4, key[1].color.g << 4, key[1].color.b << 4);
                    fraction = (s32)(object->colorTick1C * 0x1000U) / key->duration;
                    DpqColor(&key->color, fraction, &object->palette88.values[colorIndex]);
                    if (object->advance30) {
                        object->colorTick1C = (object->colorTick1C + 1U);
                        if (object->colorTick1C >= key->duration) {
                            object->colorTick1C = 0;
                            object->keyIndex38 = object->keyIndex38 + 1;
                        }
                    }
                }
                SetupAnimatedObjectMatrices(object);
                cursor = EmitAnimatedTrackLines(scratch, cursor, object, colorIndex << 2);
                if (cursor != object->headsE8[DrawBufIndex]) object->tailsF0[DrawBufIndex] = cursor - 1;
                else object->tailsF0[DrawBufIndex] = 0;
                if (objectIndex < 2) {
                    if (object->palette48.values[colorIndex].r * 16 > PolyStripTempCoords[0]) ColorMatrix.fields.rotation00[0][objectIndex] = (short)object->palette48.values[colorIndex].r << 4;
                    else ColorMatrix.fields.rotation00[0][objectIndex] = PolyStripTempCoords[0];
                    if (object->palette48.values[colorIndex].g * 16 > PolyStripTempCoords[1]) ColorMatrix.fields.rotation00[1][objectIndex] = (short)object->palette48.values[colorIndex].g << 4;
                    else ColorMatrix.fields.rotation00[1][objectIndex] = PolyStripTempCoords[1];
                    if (object->palette48.values[colorIndex].b * 16 > PolyStripTempCoords[2]) ColorMatrix.fields.rotation00[2][objectIndex] = (short)object->palette48.values[colorIndex].b << 4;
                    else ColorMatrix.fields.rotation00[2][objectIndex] = PolyStripTempCoords[2];
                }
            }
        }
        last = object->tailsF0[DrawBufIndex];
        if (last != 0) {
            /* AddPrims: splice the head..last chain in front of the entry. */
            last->tag.address = ordering->address;
            ordering->address = (uintptr_t)object->headsE8[DrawBufIndex];
        } else if (objectIndex < 2) {
            ColorMatrix.fields.rotation00[0][objectIndex] = PolyStripTempCoords[0];
            ColorMatrix.fields.rotation00[1][objectIndex] = PolyStripTempCoords[1];
            ColorMatrix.fields.rotation00[2][objectIndex] = PolyStripTempCoords[2];
        }
        if (objectIndex < 2) {
            toCamera[0] = (cameraPosition[0] >> 2) - 0x8000;
            toCamera[1] = cameraPosition[1] >> 2;
            toCamera[2] = (cameraPosition[2] >> 2) - 0x8000;
            toCamera[0] = (toCamera[0] >> 1) - object->positionC8.x;
            toCamera[1] = (toCamera[1] >> 1) - object->positionC8.y;
            toCamera[2] = (toCamera[2] >> 1) - object->positionC8.z;
            LightSourceAngles[objectIndex].x = -Atan2Table(SquareRoot0((toCamera[0] >> 4) * (toCamera[0] >> 4) + (toCamera[2] >> 4) * (toCamera[2] >> 4)), toCamera[1] >> 4) & 0xFFF;
            LightSourceAngles[objectIndex].y = (0x400 - Atan2Table(toCamera[0], toCamera[2])) & 0xFFF;
            LightSourceAngles[objectIndex].z = 0;
        }
        RR_GTE_SETFC(farRed, farGreen, farBlue);
    }
}

s32 GetSegmentVisibilityTableSize(void) {
    return RelFileSegmentCount * 0x10;
}

void ApplySegmentVisibilityMask(void) {
    int a, b;
    const short *p;
    short *g = (short *)SCRATCHPAD;
    a = GetViewTrackSegment(&((ProjectionScratchPrefix *)SCRATCHPAD)->pose08, 0);
    b = YawToOctant(&((ProjectionScratchPrefix *)SCRATCHPAD)->pose08);
    p = &SegmentVisibilityTable[a * 8 + b];
    if (*p >= 0) {
        g[0x2E] = *(unsigned short *)p;
    } else {
        g[0x2E] = DefaultSegmentVisibilityMask;
    }
}

void SetSegmentVisibilityTable(short *arg0) {
    SegmentVisibilityTable = arg0;
}


s32 GetOctantLookupTableSize(void) {
    return RelFileSegmentCount * 8;
}

int LookupOctantVisibility(void) {
    int a = GetViewTrackSegment(&((ProjectionScratchPrefix *)SCRATCHPAD)->pose08, 0);
    return (OctantLookupTable + a * 8)[YawToOctant(&((ProjectionScratchPrefix *)SCRATCHPAD)->pose08)];
}

void SetOctantLookupTable(unsigned char *arg0) {
    OctantLookupTable = arg0;
}


void InitSplashDispEnv(void) {
    struct PreviewBytes8 a, b, c, d, e, f;
    a = EmblemPreviewClutRect0;
    b = EmblemPreviewClutRect0;
    c = EmblemPreviewClutRect1;
    d = EmblemPreviewImageRect0;
    e = EmblemPreviewStripRect;
    f = EmblemPreviewImageRect1;
    SetDispMask(0);
    SetupDrawDispEnvs(0, 0, 0, 3);
    VSyncThreshold = 0x80;
    SplashLogoFrame = 0;
}

/* Initial values of the globals this module owns. */
struct PreviewBytes8 EmblemPreviewClutRect0 = {.c = {0, 3, 0xFF, 0, 0x10, 0, 1}};
struct PreviewBytes8 EmblemPreviewClutRect1 = {.c = {0, 3, 0xFE, 0, 0x10, 0, 1}};
struct PreviewBytes8 EmblemPreviewImageRect0 = {.c = {0, 3, 0, 0, 0x1E, 0, 0x20}};
struct PreviewBytes8 EmblemPreviewStripRect = {.c = {0, 3, 0x20, 0, 2, 0, 0x10}};
struct PreviewBytes8 EmblemPreviewImageRect1 = {.c = {0x20, 3, 0, 0, 0x1F, 0, 0x18}};
u8 **ActivePolyStripPtr;
s16 RearViewMirrorEnabled;
MatrixWords32 ColorMatrix;
s32 RelFileSegmentCount;
MatrixWords32 HalfTransViewMatrix;
s32 DrawBufIndex;
s32 RearViewMirrorY;
MatrixWords32 CameraViewMatrix;
MatrixWords32 CameraPitchRollMatrix;
/* The PS1 map also names elements 1 and 2 (PolyStripTempCoordY/Z). */
short PolyStripTempCoords[3];
MatrixWords32 PolyWorldTransformMtx;
s32 RearViewMirrorVisible;
AnimatedTrackParticle (*TrackParticlePoolPtr)[255];
const AnimatedTrackColorKey *TrackColorKeysPrimary[9];
const AnimatedTrackColorKey *TrackColorKeysPrimary[9],*TrackColorKeysSecondary[9];
AnimatedTrackObject *AnimatedTrackObjPool;
short *SegmentVisibilityTable;
unsigned char *OctantLookupTable;
