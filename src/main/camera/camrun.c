#include "rr/geometry.h"
#include "game/car/carbound.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "types.h"
#include "rr/camera_motion.h"
#include "rr/game_motion.h"
#include "scratchpad.h"
#include "game/camera/cammove.h"
#include "game/camera/campath.h"
#include "game/gfx/motion1.h"
#include "game/gfx/ring2.h"
#include "game/gfx/ring.h"
#include "psyq/libgpu/gpudisp.h"
#include "game/camera/camrun.h"
#include "rr/bios_devices.h"
#include "rr/frame_storage_types.h"
#include "rr/race_setup.h"
#include "rr/rro_model.h"
#include "game/camera/viewcam.h"


void StepCameraScript(void) {
    RroSceneScratchPrefix *scratch = (RroSceneScratchPrefix *)SCRATCHPAD;
    s32 scriptChanged = 0;
    MotionClock20 *clock;

    if (VSyncThreshold == 0x80) {
        RaceContext.presentation250.scale4C = 2;
    } else {
        RaceContext.presentation250.scale4C = 1;
    }
    if (RaceContext.presentation250.alternateClock5C == 0) {
        clock = &SoundState.primaryMotionClock;
    } else {
        clock = &SoundState.secondaryMotionClock;
    }
    if (clock->active != 0) {
        if (++RaceContext.presentation250.scriptTimer6A >= RaceContext.presentation250.script64[RaceContext.presentation250.scriptIndex68].duration) {
            RaceContext.presentation250.scriptIndex68++;
            if (RaceContext.presentation250.script64[RaceContext.presentation250.scriptIndex68].operation < 0) {
                RaceContext.presentation250.scriptIndex68 = 0;
            }
            RaceContext.presentation250.scriptOperation6C = RaceContext.presentation250.script64[RaceContext.presentation250.scriptIndex68].operation;
            RaceContext.presentation250.scriptArgument70 = RaceContext.presentation250.script64[RaceContext.presentation250.scriptIndex68].argument;
            scriptChanged = 1;
            RaceContext.presentation250.scriptTimer6A = 0;
        }
    }
    switch (RaceContext.presentation250.scriptOperation6C) {
    case 0:
        if (scriptChanged != 0) {
            InitCameraMotion(RaceContext.presentation250.scriptArgument70);
        }
        UpdateCameraMotion(clock);
        break;
    case 1:
        if (scriptChanged != 0) {
            InitMotionPanel(RaceContext.presentation250.scriptArgument70);
        }
        UpdateMotionPanel(clock);
        break;
    case 2:
        if (scriptChanged != 0) {
            InitScrollTrack(RaceContext.presentation250.scriptArgument70);
        }
        UpdateScrollTrack(clock);
        break;
    case 3:
        if (scriptChanged != 0) {
            InitMotionTrack(RaceContext.presentation250.scriptArgument70);
        }
        UpdateMotionTrack(clock);
        break;
    }
    if (RaceContext.presentation250.drawEnabled54 != 0) {
        scratch->view.projection.pose08.position00.vector = RaceContext.presentation250.position;
        scratch->view.projection.pose08.angles10.vector = RaceContext.presentation250.target;
        BuildCameraMatrices();
        SetGeomScreen(RaceContext.presentation250.viewDistance48);
        if (RaceContext.presentation250.fullViewport50 == 0) {
            SetupViewport(6);
            SetGeomOffset(0xA0, 0xE8);
            ScaleMatrixRow1(2, (MatrixWords32 *)SCRATCH_ROT_MATRIX);
        }
        scratch->depthShift60 = 5;
        RR_GPU_SCRATCH->order = &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0];
        if (RaceContext.presentation250.drawEnabled54 != 0) {
            switch (RaceContext.presentation250.scriptOperation6C) {
            case 0:
                DrawPresentationMotionModel();
                break;
            case 1:
                DrawMotionControllerPanel();
                break;
            case 2:
                DrawRingEffects();
                break;
            case 3:
                DrawPresentationTrackModel();
                break;
            }
        }
        if (RaceContext.presentation250.stencilEnabled58 == 0) {
            /* Splice the secondary table (entries 0..703) in front of entry 702. */
            RR_GPU_SCRATCH->order[0].address = CurrentDrawBuf->buffer.ordering70.tags[702].address;
            CurrentDrawBuf->buffer.ordering70.tags[702].address = (uintptr_t)&RR_GPU_SCRATCH->order[703];
        } else {
            GpuDmaTag *top = &CurrentDrawBuf->buffer.ordering70.tags[703];
            GpuDrawModePacket *packet = (GpuDrawModePacket *)RR_GPU_SCRATCH->cursor;
            GpuDrawModePacket *maskPacket;
            GpuTilePacket *tile;

            SetDrawStp(packet, 0);
            packet->command[0] |= 2;
            RR_GPU_LINK(top, packet);
            packet++;
            RR_GPU_SCRATCH->cursor = (u8 *)packet;
            RR_GPU_SCRATCH->order[0].address = top->address;
            top->address = (uintptr_t)&RR_GPU_SCRATCH->order[703];
            packet = (GpuDrawModePacket *)RR_GPU_SCRATCH->cursor;
            SetDrawStp(packet, 0);
            RR_GPU_LINK(top, packet);
            tile = (GpuTilePacket *)(packet + 1);
            tile->tag.length = 3;
            tile->code = 0x60;
            tile->r = 0;
            tile->g = 0;
            tile->b = 0;
            tile->x = 0;
            tile->y = 0;
            tile->w = 0x140;
            tile->h = 0xF0;
            RR_GPU_LINK(top, tile);
            maskPacket = (GpuDrawModePacket *)(tile + 1);
            SetDrawStp(maskPacket, 1);
            RR_GPU_LINK(top, maskPacket);
            RR_GPU_SCRATCH->cursor = (u8 *)(maskPacket + 1);
        }
        RR_GPU_SCRATCH->order = &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0];
    }
}

void InitCameraMotion(int selection) {
    PsyqBzero(&RaceContext.presentation250.motion74, sizeof(CameraMotion70));
    if (selection < 4) {
        RaceContext.presentation250.motion74.mode04 = 0;
        RaceContext.presentation250.motion74.keys0C = HudTachMeterSegments[selection];
    } else {
        selection -= 4;
        RaceContext.presentation250.motion74.mode04 = 1;
        RaceContext.presentation250.motion74.keys0C = HudGaugeArcVertices[selection];
    }
    RaceContext.presentation250.motion74.modelScale6C = 0x2D0;
    RaceContext.presentation250.motion74.start10[0] =
        RaceContext.presentation250.motion74.current40[0] =
        RaceContext.presentation250.motion74.keys0C->x;
    RaceContext.presentation250.motion74.start10[1] =
        RaceContext.presentation250.motion74.current40[1] =
        RaceContext.presentation250.motion74.keys0C->y;
    RaceContext.presentation250.motion74.start10[2] =
        RaceContext.presentation250.motion74.current40[2] =
        RaceContext.presentation250.motion74.keys0C->z;
    RaceContext.presentation250.motion74.key08 = 1;
}

/* Initial values of the globals this module owns. */
CameraKey16 HudTachMeterSegments[4][3] = {{{0}, {.x = -409600, .y = 204800}, {.x = -819200, .y = 409600}}, {{0}, {.x = 409600, .y = 204800, .z = 204800}, {.x = 819200, .y = 409600, .z = 409600}}, {{0}, {.x = 204800, .y = 409600}, {.x = 409600, .y = 819200}}, {{0}, {.x = -204800, .z = 204800}, {.x = -409600, .z = 409600}}};
CameraKey16 HudGaugeArcVertices[6][2] = {{{0}, {.x = -204800, .y = 102400}}, {{0}, {.x = 204800, .y = 102400, .z = 102400}}, {{0}, {.x = 102400, .y = 204800}}, {{0}, {.x = -102400, .z = 102400}}, {0}, {{.x = -1280}, {.x = 1280}}};
