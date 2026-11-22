#include "rr/cpu_work.h"
#include "rr/cpu_time.h"
#include "rr/ps1_sizes.h"
#include "rr/scene_camera_state.h"
#include "game/camera/viewcam.h"
#include "rr/audio_state_types.h"
#include "types.h"
#include "scratchpad.h"
#include "rr_asm.h"
#include "rr/game_motion.h"
#include "rr/matrix_words.h"
#include "game/demo/opening.h"
#include "game/camera/camrun.h"
#include "game/gfx/motion1.h"
#include "game/gfx/ring.h"
#include "game/camera/campath.h"
#include "rr/bios_devices.h"
#include "game/gfx/scene1.h"
#include "game/math/rotmat.h"
/* PsyZ libgte ScaleMatrix(MATRIX *, VECTOR *); MatrixWords32 shares MATRIX. */
extern MatrixWords32 *ScaleMatrix(MatrixWords32 *matrix, s32 *scale);

typedef struct {
    PreviewVector8 vertex;
    Vec2s screen;
    s32 reserved0C;
    Vector scale, position;
    PreviewVector8 rotation;
    MatrixWords32 lightMatrix, viewMatrix;
    s32 flags, depth;
} MotionMarkerWork;

extern u16 HudFinalLapBannerParts[];

void DrawPresentationTrackModel(void) {
    if (!RrWorkFrame)
        RR_CPU_CYCLES(RR_CYCLES_PRESENTATION_TRACK_MODEL);
    MotionMarkerWork work;
    PresentationController *controller;
    u16 *row;
    s32 i, j, x, y, v;
    controller = MotionControllerView();
    work.position.x = 0;
    work.position.y = 0;
    work.position.z = 0;
    work.rotation.x = (u16)controller->track.position.x;
    work.rotation.y = (u16)controller->track.position.y;
    work.rotation.z = (u16)controller->track.position.z;
    SetupObjectLightMatrices((const SceneCameraPose18 *)&work.position, &controller->lighting20, &work.lightMatrix, &work.viewMatrix);
    v = MotionControllerView()->track.scale;
    work.scale.z = v;
    work.scale.y = v;
    work.scale.x = v;
    ScaleMatrix(&work.lightMatrix, &work.scale.x);
    SetGteObjectTransform(
        (ObjectTransform38 *)(SCRATCHPAD + 0x7C),
        (const s16 *)&work.position.x,
        work.lightMatrix.w);
    row = HudFinalLapBannerParts; i = 0; y = -0x438;
    do {
        j = 0;
        x = -0x438;
        do {
            if (*row & (0x8000 >> j)) {
                work.vertex.x = x;
                work.vertex.y = y;
                work.vertex.z = 0;
                RR_GTE_RTPS_LDV(&work.vertex.x);
                RR_GTE_SWC2_SXY2(&work.screen);
                RR_GTE_STORE_IR0_THEN_FLAG(&work.flags);
                RR_GTE_STORE_SZ3_SRA2(&work.depth);
                work.depth = work.depth >> 3;
                EmitProjectedMarkerSprt(work.screen.x, work.screen.y, work.depth);
            }
            j++;
            x += 0x90;
        } while (j < 16);
        y += 0x90;
        i++;
        row++;
    } while (i < 16);
    EmitOpeningDrawModePrim();
}

void InitMotionTrack(int selection) {
    MotionTrack44 *track = &MotionControllerView()->track;
    PsyqBzeroPs1(track, sizeof(*track), RR_PS1_MOTION_TRACK_BYTES);
    track->disabled = 0;
    track->key = &MotionTrackTable[selection];
    track->scale = 0x1000;
}

void UpdateMotionTrack(MotionClock20 *clock) {
    s16 counter, next;
    u32 delta;
    s32 start, code, units;
    s32 rate;
    u32 tempo;
    MotionKey32 *key;
    MotionKey32 *finishKey;
    MotionKey32 *restKey;
    MotionKey32 *scaleKey;
    MotionTrack44 *track;
    PresentationController *controller;

    controller = MotionControllerView();
    track = &controller->track;
    if (track->disabled == 0) {
        switch (controller->track.state) {
        case 0:
            if (clock->active != 0) {
                controller->track.position = track->key->from;
                track->scale = track->key->fromScale;
                code = track->key->timeCode;
                units = (code >> 8) * 12 + (code & 255);
                tempo = clock->tempo;
                units *= 2;
                track->elapsed = 0;
                track->duration = (s16)((tempo * units) / 1200U);
                controller->track.state = (controller->track.state + 1U);
                /* Fall through: the first interpolation step is immediate. */
        case 1:
                key = track->key;
                counter = track->elapsed;
                track->position.x = (u16)key->from.x +
                    ((key->to.x - key->from.x) * counter) / track->duration;
                track->position.y = (u16)key->from.y +
                    ((key->to.y - key->from.y) * counter) / track->duration;
                track->position.z = (u16)key->from.z +
                    ((key->to.z - key->from.z) * counter) / track->duration;
                scaleKey = key;
                delta = scaleKey->toScale;
                start = scaleKey->fromScale;
                delta = (s32)((delta - start) *
                              track->elapsed) / track->duration;
                next = (u16)track->elapsed + 1;
                track->elapsed = next;
                track->scale = (delta + start);
                finishKey = key;
                if (track->duration < next) {
                    /* Keep the first destination write before the key reload. */
                    track->position.x = (u16)finishKey->to.x;
                    restKey = key;
                    track->position.y = (u16)restKey->to.y;
                    track->position.z = (u16)restKey->to.z;
                    track->scale = restKey->toScale;
                    if (restKey->continueAfter == 0) {
                        track->state = 3;
                        return;
                    }
                    track->state = 2;
                    track->velocity = 800;
                    return;
                }
            } else {
                return;
            }
            break;
        case 2:
            {
                u32 current = track->scale;
                s32 rate;
                track->scale = current + track->velocity;
                rate = track->velocity * 31U;
                track->velocity = rate / 32;
            }
            track->position.x += 16;
            track->position.y += 8;
            track->position.z += 8;
            break;
        }
    }
}

void InitMotionController(void) {
    RaceContext.controllerSlot3D8 = 1; /* MotionController = &presentation250 */
    PsyqBzeroPs1(&RaceContext.presentation250, sizeof(PresentationController), RR_PS1_PRESENTATION_CONTROLLER_BYTES);
    RaceContext.presentation250.position.z = -0x1388;
    RaceContext.presentation250.viewDistance48 = 0x1B8;
    RaceContext.presentation250.lighting20.matrix.fields.vectors[0].x = 0x1C1;
    RaceContext.presentation250.lighting20.matrix.fields.vectors[0].y = -0xDF8;
    RaceContext.presentation250.lighting20.matrix.fields.vectors[0].z = -0x79A;
    RaceContext.presentation250.lighting20.matrix.fields.colors[0].red = 0xC0;
    RaceContext.presentation250.lighting20.matrix.fields.colors[0].green = 0xC0;
    RaceContext.presentation250.lighting20.matrix.fields.colors[0].blue = 0xC0;
    RaceContext.presentation250.lighting20.matrix.fields.vectors[1].x = -0x64;
    RaceContext.presentation250.lighting20.matrix.fields.vectors[1].y = 0;
    RaceContext.presentation250.lighting20.matrix.fields.vectors[1].z = 0x64;
    RaceContext.presentation250.lighting20.matrix.fields.colors[1].red = 0xC0;
    RaceContext.presentation250.lighting20.matrix.fields.colors[1].green = 0xC0;
    RaceContext.presentation250.lighting20.matrix.fields.colors[1].blue = 0xC0;
    RaceContext.presentation250.lighting20.matrix.fields.vectors[2].x = -0xA4D;
    RaceContext.presentation250.lighting20.matrix.fields.vectors[2].y = -0x642;
    RaceContext.presentation250.lighting20.matrix.fields.vectors[2].z = 0xA86;
    RaceContext.presentation250.lighting20.matrix.fields.colors[2].red = 0x20;
    RaceContext.presentation250.lighting20.matrix.fields.colors[2].green = 0x20;
    RaceContext.presentation250.lighting20.matrix.fields.colors[2].blue = 0x20;
    RaceContext.presentation250.lighting20.ambient[0] = 0;
    RaceContext.presentation250.lighting20.ambient[1] = 0;
    RaceContext.presentation250.lighting20.ambient[2] = 0;
    RaceContext.presentation250.script64 = HudCircularGaugeVerts;
    RaceContext.presentation250.scriptOperation6C = RaceContext.presentation250.script64[RaceContext.presentation250.scriptIndex68].operation;
    RaceContext.presentation250.scriptArgument70 = RaceContext.presentation250.script64[RaceContext.presentation250.scriptIndex68].argument;
    switch (RaceContext.presentation250.scriptOperation6C) {
    case 0:
        InitCameraMotion(RaceContext.presentation250.scriptArgument70);
        break;
    case 1:
        InitMotionPanel(RaceContext.presentation250.scriptArgument70);
        break;
    case 2:
        InitScrollTrack(RaceContext.presentation250.scriptArgument70);
        break;
    case 3:
        InitMotionTrack(RaceContext.presentation250.scriptArgument70);
        break;
    }
}

void RestartMotionSequence(void) {
    MotionControllerView()->scriptIndex68 = 0;
    MotionControllerView()->scriptTimer6A = 0;
    MotionControllerView()->flags60 = 0;
    MotionControllerView()->script64 = MotionSequenceTable[SoundState.seqTrack2868];
    MotionControllerView()->scriptOperation6C = MotionControllerView()->script64[MotionControllerView()->scriptIndex68].operation;
    MotionControllerView()->scriptArgument70 = MotionControllerView()->script64[MotionControllerView()->scriptIndex68].argument;
    switch (MotionControllerView()->scriptOperation6C) {
    case 0:
        InitCameraMotion(MotionControllerView()->scriptArgument70);
        break;
    case 1:
        InitMotionPanel(MotionControllerView()->scriptArgument70);
        break;
    case 2:
        InitScrollTrack(MotionControllerView()->scriptArgument70);
        break;
    case 3:
        InitMotionTrack(MotionControllerView()->scriptArgument70);
        break;
    }
}

void SetPresentationDrawEnabled(s32 arg0) {
    MotionControllerView()->drawEnabled54 = arg0;
}

void SetPresentationFullViewport(s32 arg0) {
    MotionControllerView()->fullViewport50 = arg0;
}

void SetPresentationStencilEnabled(s32 arg0) {
    MotionControllerView()->stencilEnabled58 = arg0;
}

/* Initial values of the globals this module owns. */
CameraScriptEntry8 D_800A1F78[2] = {{.argument = 4, .duration = 800}, {.operation = -1, .argument = -1}};
MotionKey32 MotionTrackTable[12] = {{.from = {.x = 512, .y = 640, .z = 256}, .fromScale = 10240, .toScale = 4096, .timeCode = 1024}, {.from = {.x = 512, .y = 2048, .z = 256}, .fromScale = 10240, .toScale = 4096, .timeCode = 1798, .continueAfter = 1}, {.from = {.x = 2048, .y = -64, .z = 512}, .fromScale = 6400, .toScale = 4096, .timeCode = 1024}, {.from = {.x = -128, .y = -2048, .z = -4096}, .fromScale = 8192, .toScale = 4096, .timeCode = 1280}, {.fromScale = 4096, .to = {.x = 256, .y = -4096, .z = -512}, .toScale = 8192, .timeCode = 1030}, {.from = {.x = 256, .y = -3072, .z = -512}, .fromScale = 8192, .toScale = 4096, .timeCode = 512}, {.fromScale = 4096, .to = {.x = -128, .y = 4096, .z = -512}, .toScale = 8192, .timeCode = 1280}, {.from = {.x = 4096}, .fromScale = 10240, .to = {.y = 256, .z = 512}, .toScale = 4096, .timeCode = 1280}, {.fromScale = 4096, .to = {.x = 4096, .y = 2048, .z = 1024}, .toScale = 8192, .timeCode = 1030}, {.from = {.x = 4096, .y = 2048, .z = 1024}, .fromScale = 8192, .toScale = 4096, .timeCode = 512}, {.from = {.x = 2048, .y = 4096, .z = 1024}, .fromScale = 8192, .toScale = 4096, .timeCode = 1024}, {.from = {.x = 4096, .y = 1600, .z = 2048}, .fromScale = 8192, .toScale = 4096, .timeCode = 1024}};
CameraScriptEntry8 HudCircularGaugeVerts[35] = {{.operation = 2, .duration = 16}, {.duration = 8}, {.operation = 3, .duration = 5}, {.operation = 1, .duration = 3}, {.operation = 2, .argument = 1, .duration = 16}, {.argument = 1, .duration = 8}, {.operation = 3, .argument = 1, .duration = 8}, {.operation = 2, .argument = 2, .duration = 16}, {.argument = 2, .duration = 8}, {.operation = 3, .argument = 3, .duration = 5}, {.operation = 1, .argument = 1, .duration = 3}, {.argument = 3, .duration = 8}, {.operation = 3, .argument = 10, .duration = 4}, {.operation = 3, .argument = 11, .duration = 4}, {.duration = 8}, {.operation = 3, .argument = 4, .duration = 5}, {.operation = 3, .argument = 5, .duration = 3}, {.operation = 2, .duration = 16}, {.duration = 8}, {.operation = 3, .argument = 6, .duration = 5}, {.operation = 1, .duration = 3}, {.operation = 2, .argument = 1, .duration = 16}, {.argument = 1, .duration = 8}, {.operation = 3, .argument = 1, .duration = 8}, {.operation = 2, .argument = 2, .duration = 16}, {.argument = 2, .duration = 8}, {.operation = 3, .argument = 7, .duration = 5}, {.operation = 1, .argument = 1, .duration = 3}, {.operation = 1, .argument = 2, .duration = 8}, {.operation = 3, .argument = 11, .duration = 4}, {.operation = 3, .argument = 10, .duration = 4}, {.duration = 8}, {.operation = 3, .argument = 8, .duration = 5}, {.operation = 3, .argument = 9, .duration = 3}, {.operation = -1, .argument = -1}};
CameraScriptEntry8 *MotionSequenceTable[2] = {(CameraScriptEntry8 *)&HudCircularGaugeVerts[0], (CameraScriptEntry8 *)D_800A1F78};
u16 HudFinalLapBannerParts[16] = {0x7E0, 0x1F8, 0x3EFC, 0x61FE, 0x67FE, 0xEBFF, 0xEDFF, 0xEEFF, 0xEFFF, 0xEFFF, 0xEFFF, 0x6FFE, 0x6FFE, 0x2FFC, 0xFF8, 0x7E0};
