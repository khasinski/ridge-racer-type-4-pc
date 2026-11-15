#include "rr/geometry.h"
#include "rr/video_timing.h"
#include "game/system/pad.h"
#include "rr/audio_state_types.h"
#include "rr/save_slots.h"
#include "rr/menu_selection.h"
#include "rr/rro_model.h"
#include "game/gfx/model.h"
#include "psyq/libgte/gtemat5.h"
#include "rr/frame_storage_types.h"
#include "game/camera/viewcam.h"
#include "types.h"
#include "scratchpad.h"
#include "rr/projection_state.h"
#include "game/course/trkintp2.h"
#include "game/demo/opening.h"
#include "game/gfx/gfxdraw.h"
#include "game/sound/sndbgm3.h"
#include "game/sound/sndslot.h"
#include "psyq/libgpu/gpuenv.h"
#include "game/gfx/gfxotag.h"
#include "game/ui/hudmap2.h"
#include "rr/hud_number.h"
#include "rr/bios_devices.h"
#include "rr/race_setup.h"
#include "rr/race_types.h"
#include "rr/gpu.h"
#include "rr/frame_storage_types.h"
#include "rr/audio_loop.h"
#include "rr/screen_tiles.h"
#include "game/sound/cdstart.h"
#include "rr/menu_transition.h"
#include "psyq/libc/rand.h"
#include "game/sound/cdplay.h"
#include "rr/scene_camera_state.h"
#include "rr/gpu_scratch_types.h"
#include "rr/runtime_settings.h"


typedef struct {
    short x, y, z, w;
} V8;

typedef struct {
    Vector f0;
    V8 f10;
    int f18;
    int f1C;
    int f20;
    int f24;
} Elem28;


extern short SelectedTeamIndex;
extern int ViewUsesCarTrackNode;

void RenderSubOtObject(void);
int FindCameraTrackTrigger(RaceVehicle *a0, int a1, int a2);
int DrawLensFlareAndSun(RaceVehicle *arg0);

/* Renders the showcase object into its sub-ordering table and splices that
 * table into the frame ordering table at the object's depth. */
static void RenderSubOtModel(u8 *object, GpuDmaTag *subOrdering, s32 bank, s32 depthShift) {
    MatrixWords32 sp10;
    RroSceneScratchPrefix *scene = (RroSceneScratchPrefix *)SCRATCHPAD;
    ObjectTransform38 *transform = &((RroModelScratch *)SCRATCHPAD)->work7C.transform;
    int s1;
    BuildObjectLightAndRotMatrices(object, &sp10);
    SetGteObjectTransform(transform, (const s16 *)object, sp10.w);
    s1 = RotTransPersOrigin();
    if (s1 > 0) {
        ClearOTagR(subOrdering, 0x15F);
        RR_GPU_SCRATCH->order = subOrdering;
        scene->depthShift60 = s1 >> 7;
        if (s1 >> 7 >= 6) {
            scene->depthShift60 = 5;
        }
        SelectModelBank(bank);
        SetGteObjectTransform(transform, (const s16 *)object, sp10.w);
        scene->draw74.draw.counter = 0;
        SubmitModel(scene, ModelBankFormat < 1, 0, 0);
        s1 >>= depthShift;
        s1 += 0x6E;
        if (s1 > 0) {
            GpuDmaTag *entry = &CurrentDrawBuf->buffer.ordering70.tags[s1];
            EmitSpotlightConePolys(s1 + 1);
            /* Reverse-cleared table: entry 0x15E is the head, entry 0 the tail. */
            subOrdering[0].address = entry->address;
            entry->address = (uintptr_t)&subOrdering[0x15E];
        }
    }
}

void RenderSubOtObject(void) {
    RroSceneScratchPrefix *scene = (RroSceneScratchPrefix *)SCRATCHPAD;
    s32 s5 = scene->depthShift60;
    u8 *object = RaceContext.workspace55C + (SelectedTeamIndex << 7);
    RenderSubOtModel(object, RaceContext.subOrdering50[0][DrawBufIndex], 8, s5);
    if (CourseSkyboxTypeTable[CourseIndex] == 7) {
        RenderSubOtModel(object + 0x40, RaceContext.subOrdering50[1][DrawBufIndex], 9, s5);
    }
    scene->depthShift60 = s5;
    RR_GPU_SCRATCH->order = &CurrentDrawBuf->buffer.ordering70.tags[0];
}

void ApplyCameraState(const SceneCameraView24 *state) {
    MatrixWords32 matrix;
    Vec3s8 angles;
    Vec3s8 offset;
    Vec3i transformed;
    ProjectionScratchPrefix *scratch = (ProjectionScratchPrefix *)SCRATCHPAD;
    scratch->pose08.angles10.xyz.x = -state->pose00.rotation10.x;
    scratch->pose08.angles10.xyz.y = state->pose00.rotation10.y;
    scratch->pose08.angles10.xyz.z = state->pose00.rotation10.z;
    angles.fields.x = state->pose00.rotation10.x;
    angles.fields.y = state->pose00.rotation10.y;
    angles.fields.z = 0;
    offset.fields.x = 0;
    offset.fields.y = 0;
    offset.fields.z = -(u16)state->distance1C;
    RotMatrixZYX((const Vec3sFields8 *)angles.halves, &matrix);
    ApplyMatrix(&matrix, &offset, &transformed);
    {
        s32 origin = state->pose00.position00.xyz.x;
        u32 displacement = transformed.x;
        scratch->pose08.position00.xyz.x = origin + displacement;
    }
    {
        s32 origin = state->pose00.position00.xyz.y;
        u32 displacement = transformed.y;
        scratch->pose08.position00.xyz.y = origin + displacement;
    }
    {
        s32 origin = state->pose00.position00.xyz.z;
        u32 displacement = transformed.z;
        scratch->pose08.position00.xyz.z = origin + displacement;
    }
    SetGeomScreen(state->projection20);
    BuildCameraMatrices();
}
int FindCameraTrackTrigger(RaceVehicle *a0, int a1, int a2) {
    int unused10[4];
    Vector sp20;
    Vector sp30;
    int s0;
    int s3;
    int i;
    Elem28 *s2;
    int t1;
    int t2;

    s0 = -1;
    s2 = ((Elem28 (*)[16])RaceContext.workspace554)[a2];
    for (i = 1; i < 0x10; i++) {
        if (s2[i].f24 * RR_PAL_NTSC(5, 6) >= a1) {
            s0 = i;
            s3 = s0 - 1;
            break;
        }
    }
    if (s0 < 0) {
        return 1;
    }
    if (RaceContext.opening88.triggerA4 != s3) {
        ResetCarSlotBuffers();
        RaceContext.opening88.triggerA4 = s3;
    }
    /* Key times in units of 5 frames (NTSC release: 6). */
    t1 = (s2[s0].f24 - s2[s3].f24) * RR_PAL_NTSC(5, 6);
    t2 = a1 - s2[s3].f24 * RR_PAL_NTSC(5, 6);
    if (s2[s3].f18 == 0) {
        sp20.x = a0->worldPosition10.words[0];
        sp20.y = a0->worldPosition10.words[1];
        sp20.z = a0->worldPosition10.words[2];
    } else {
        sp20 = s2[s3].f0;
    }
    if (s2[s0].f18 == 0) {
        sp30.x = a0->worldPosition10.words[0];
        sp30.y = a0->worldPosition10.words[1];
        sp30.z = a0->worldPosition10.words[2];
    } else {
        sp30 = s2[s0].f0;
    }
    if (t1 > 0) {
        RaceContext.view60.pose00.position00.vector.x = ((sp30.x - sp20.x) * t2) / t1 + sp20.x;
        RaceContext.view60.pose00.position00.vector.y = ((sp30.y - sp20.y) * t2) / t1 + sp20.y;
        RaceContext.view60.pose00.position00.vector.z = ((sp30.z - sp20.z) * t2) / t1 + sp20.z;
        RaceContext.view60.pose00.rotation10.x = ((s2[s0].f10.x - s2[s3].f10.x) * t2) / t1 + s2[s3].f10.x;
        RaceContext.view60.pose00.rotation10.y = ((s2[s0].f10.y - s2[s3].f10.y) * t2) / t1 + s2[s3].f10.y;
        RaceContext.view60.pose00.rotation10.z = ((s2[s0].f10.z - s2[s3].f10.z) * t2) / t1 + s2[s3].f10.z;
        RaceContext.view60.distance1C = ((s2[s0].f1C - s2[s3].f1C) * t2) / t1 + s2[s3].f1C;
        RaceContext.view60.projection20 = ((s2[s0].f20 - s2[s3].f20) * t2) / t1 + s2[s3].f20;
    } else {
        RaceContext.view60.pose00.position00.vector = sp30;
        RaceContext.view60.pose00.rotation10.x = s2[s0].f10.x;
        RaceContext.view60.pose00.rotation10.y = s2[s0].f10.y;
        RaceContext.view60.pose00.rotation10.z = s2[s0].f10.z;
        RaceContext.view60.pose00.rotation10.pad06 = s2[s0].f10.w;
        RaceContext.view60.distance1C = s2[s0].f1C;
        RaceContext.view60.projection20 = s2[s0].f20;
    }
    ApplyCameraState(&RaceContext.view60);
    if (GameModeType == 1) {
        InitRaceContextBuffers();
        RenderSubOtObject();
    }
    return 0;
}

void ResetLensFlareState(void) {
    PsyqBzero(&RaceContext.opening88, sizeof(RaceContext.opening88));
    RaceContext.opening88.triggerA4 = -1;
}

int DrawLensFlareAndSun(RaceVehicle *arg0)
{
    int i;
    int s0;
    Elem28 *r;
    GpuTilePacket *pk;
    GpuCommandPacket *mode;
    int v;
    int v1;
    int v2;
    SceneRaceContext *g;
    SceneRaceContext *g2;
    SceneRaceContext *g1;
    SceneRaceContext *gB;
    SceneRaceContext *gC;
    SceneRaceContext *gL;
    SceneRaceContext *gF;
    SceneRaceContext *gD;
    SceneRaceContext *gS;
    SceneRaceContext *gT;
    s0 = 0;
    do {
        SlotSetPause(s0, 1);
        s0++;
    } while (s0 < 4);
    g1 = &RaceContext;
    if (g1->opening88.fadeActive90 != 0) {
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], g1->opening88.fadeFrame8C * 10, 0x25);
        v1 = g1->opening88.volumeA0 - 8;
        g1->opening88.volumeA0 = v1;
        if (v1 < 0) {
            g1->opening88.volumeA0 = 0;
        }
        SetSoundChannelVolLR(0, (short)g1->opening88.volumeA0, (short)g1->opening88.volumeA0);
        g1->opening88.fadeFrame8C = g1->opening88.fadeFrame8C + 1;
    } else {
        v2 = g1->opening88.volumeA0 + 8;
        g1->opening88.volumeA0 = v2;
        if (v2 >= 0x61) {
            g1->opening88.volumeA0 = 0x60;
        }
        SetSoundChannelVolLR(0, (short)g1->opening88.volumeA0, (short)g1->opening88.volumeA0);
    }
    gB = &RaceContext;
    s0 = gB->opening88.frame88;
    if (s0 < RR_PAL_NTSC(0xC, 0xF)) {
        if (s0 == 1) {
            StartOpeningFadeOut();
            if (gB->opening88.mode94 == s0) {
                CdAudioBeginPlay(0);
            }
            for (s0 = 0; s0 < 2; s0++) {
                gB->introSavedRumbleA8[s0] = Options.wheel[s0].rumble02;
                Options.wheel[s0].rumble02 = 2;
            }
        }
        {
            int x, d;
            x = RaceContext.opening88.frame88 << 8;
            d = x / RR_PAL_NTSC(12, 15);
            s0 = 0x100;
            s0 -= d;
        }
        UpdateRaceOpeningOverlay(RR_PAL_NTSC(0x1C2, 0x21C));
        pk = (GpuTilePacket *)RR_GPU_SCRATCH->cursor;
        pk->code = 0x62;
        pk->r = s0;
        pk->g = s0;
        pk->b = s0;
        pk->x = 0;
        pk->y = 0;
        pk->w = 0x140;
        pk->h = 0xF0;
        RR_GPU_ADD_PRIM(&CurrentDrawBuf->buffer.ordering70.tags[0], pk, 3);
        pk++;
        mode = (GpuCommandPacket *)pk;
        mode->command = 0xE1000245;
        RR_GPU_ADD_PRIM(&CurrentDrawBuf->buffer.ordering70.tags[0], mode, 1);
        RR_GPU_SCRATCH->cursor = (u8 *)(mode + 1);
    }
    gC = &RaceContext;
    if (gC->opening88.mode94 == 0) {
        if ((GameModeType == 1) && (SelectionState.signedBytes[SelectionState.signedBytes[0x17] + 8] >= 3)) {
            s0 = 1;
            gL = gC;
            gL->opening88.endFrame98 = 0;
            r = &((Elem28 (*)[16])gL->workspace554)[SelectedTeamIndex][1];
            do {
                v = r->f24;
                if (gL->opening88.endFrame98 < v) {
                    gL->opening88.endFrame98 = v;
                }
                s0++;
                r++;
            } while (s0 < 0x10);
            gF = &RaceContext;
            gF->opening88.mode94 = 1;
            gF->opening88.endFrame98 = gF->opening88.endFrame98 * RR_PAL_NTSC(5, 6);
        } else {
            gC->opening88.mode94 = 2;
        }
    }
    gD = &RaceContext;
    switch (gD->opening88.mode94) {
    case 1:
        FindCameraTrackTrigger(arg0, gD->opening88.frame88, SelectedTeamIndex);
        if (Pad0Buttons & 0x800) {
            if (gD->opening88.frame88 >= RR_PAL_NTSC(0x1A, 0x1F)) {
                if (gD->opening88.fadeActive90 == 0) {
                    gD->opening88.endFrame98 = gD->opening88.frame88 + RR_PAL_NTSC(0x19, 0x1E);
                }
            }
        }
        gS = &RaceContext;
        if ((gS->opening88.endFrame98 - gS->opening88.frame88) < RR_PAL_NTSC(0x1A, 0x1F)) {
            if (gS->opening88.fadeActive90 == 0) {
                gS->opening88.fadeActive90 = 1;
                CdAudioFade(0, RR_PAL_NTSC(8, 10));
            }
        }
        gT = &RaceContext;
        gT->opening88.frame88++;
        if (gT->opening88.frame88 >= gT->opening88.endFrame98) {
            gT->opening88.frame88 = gT->opening88.endFrame98;
            gT->opening88.done9C = 1;
        }
        ViewUsesCarTrackNode = 0;
        break;

    case 2:
        gD->opening88.frame88++;
        if (gD->opening88.frame88 >= RR_PAL_NTSC(0x49, 0x58)) {
            gD->opening88.done9C = 1;
            arg0->flags1E4 = arg0->flags1E4 | 0x100;
        }
        if (gD->opening88.done9C == 0) {
            RaceContext.cameraFollowOffset30.fields.x = 0;
            RaceContext.cameraFollowOffset30.fields.y = 0;
            RaceContext.cameraFollowOffset30.fields.z = -0x100;
            CarLocalToWorld(arg0, &RaceContext.cameraFollowOffset30, &RaceContext.view60.pose00.position00.vector);
            RaceContext.view60.pose00.rotation10.z = 0;
            RaceContext.view60.projection20 = 0x1D6;
            /* A 73-frame swing (NTSC release: 88 frames). */
            const int swing = RR_PAL_NTSC(73, 88);
            RaceContext.view60.pose00.rotation10.x = ((RaceContext.opening88.frame88 * 0x190) / swing) - 0x280;
            v = (RaceContext.view60.pose00.rotation10.y = (-(RaceContext.opening88.frame88 * 0xB00) / swing) + 0x600);
            RaceContext.view60.distance1C = (-(RaceContext.opening88.frame88 << 9) / swing) + 0x400;
            if (((short)v) < (-0x400)) {
                RaceContext.view60.pose00.rotation10.y = -0x400;
            }
        } else {
            RaceContext.cameraFollowOffset30.fields.x = 0;
            RaceContext.cameraFollowOffset30.fields.y = 0;
            RaceContext.cameraFollowOffset30.fields.z = -0x100;
            CarLocalToWorld(arg0, &RaceContext.cameraFollowOffset30, &RaceContext.view60.pose00.position00.vector);
            RaceContext.view60.pose00.rotation10.x = -0xF0;
            RaceContext.view60.pose00.rotation10.y = -0x400;
            RaceContext.view60.distance1C = 0x200;
            RaceContext.view60.pose00.rotation10.z = 0;
            RaceContext.view60.projection20 = 0x1D6;
        }
        ApplyCameraState(&RaceContext.view60);
        g2 = &RaceContext;
        if (g2->opening88.frame88 >= RR_PAL_NTSC(0x33, 0x3D)) {
            g2->opening88.fadeActive90 = 1;
        }
        break;
    }

    g = &RaceContext;
    if (g->opening88.done9C != 0) {
        SlotsStartForRace();
        v = g->opening88.done9C;
        DrawSubOtFlag = 1;
        if (v == 1) {
            if (g->opening88.mode94 == v) {
                if (SoundState.track28A4 == 0) {
                    CdAudioSelectTrack(0, (PsyqRand() % 14) + 0xB);
                } else {
                    CdAudioSelectTrack(0, BgmNumberToCdTrack[SoundState.track28A4 - 1]);
                }
            }
            RaceContext.opening88.done9C = 2;
        }
        for (s0 = 0; s0 < 2; s0++) {
            Options.wheel[s0].rumble02 = (s8)RaceContext.introSavedRumbleA8[s0];
        }
    } else {
        DrawSubOtFlag = 0;
    }
    return RaceContext.opening88.done9C;
}

/* Initial values of the globals this module owns. */
s32 DrawSubOtFlag;
