#include "rr/gpu_address.h"
#include "game/ui/uipart1.h"
#include "game/sound/cdpos.h"
#include "game/gfx/drawlist.h"
/* RODATA 0x800102cc 0x24 */
#include "types.h"
#include "rr_asm.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"
#include "scratchpad.h"
#include "rr/frame_loop.h"
#include "rr/widescreen.h"
#include "rr/video_timing.h"
#include "rr/release.h"
#include "rr/frame_interp.h"
#include <psyz/gpu.h>
#include <stdlib.h>
#include "game/data/globals.h"
#include "rr/gpu_scratch_types.h"
#include "game/data/cdfile2.h"
#include "game/system/frmenv.h"
#include "game/system/pad.h"
#include "game/effect/spfade.h"
#include "psyq/libgpu/gpuenv.h"
#include "psyq/libgpu/gpudisp.h"
#include "game/system/frmtask.h"
#include "game/data/cdfile1.h"
#include "game/sound/sndslot.h"
#include "rr/course_choices.h"
#include "rr/menu_background.h"
#include "rr/driver_control.h"
#include "rr/overlay668_stream.h"
#include "game/sound/cdstart.h"
#include "game/course/trkintp2.h"
#include "rr/screen_tiles.h"
#include "game/sound/cdplay.h"

extern s16 FadeCountdown;

/* Host scratchpad buffer and the GPU packet cursor/ordering pointers that
 * occupied its first two words on the PS1 (include/scratchpad.h). */
_Alignas(16) u8 RrScratchpad[RR_SCRATCHPAD_SIZE];
GpuScratchPointers GpuScratch;

void FrmTaskModeEnter(void);
void FrmTaskModeUpdate(void);

void FrmTaskModeEnter(void) {
    FrmTaskState = FRAME_TASK_READ_START;
    FadeCountdown = -1;
    ModeGroup = 0;
    ModeIndex = 6;
    SubmodeState = 0;
}


void ResetFrmTaskState(void) {
    FrmTaskState = FRAME_TASK_READ_START;
    FadeCountdown = -1;
}

void FrmTaskModeUpdate(void) {
    FrameTaskFields *rampState;
    FrameTaskState *initialState;
    FrameTaskFields *revealState;
    FrameTaskState *animationState;
    GpuScratchPointers *scratch;
    GpuCommandPacket *packet;
    int fileIndex;

    switch (FrmTaskState) {
    case FRAME_TASK_READ_START:
        fileIndex = GetR4FileIndex(0, 0x15);
        if (CdFileReadStart(fileIndex, LoadCursor)) {
            FrmTaskState = FRAME_TASK_READ_WAIT;
        }
        return;
    case FRAME_TASK_READ_WAIT:
        if (CdFileBytesLoaded()) {
            FrmTaskState = FRAME_TASK_SETUP;
        }
        return;
    case FRAME_TASK_SETUP:
        CdAudioInit();
        CdAudioSelectTrack(0, 0x24);
        CdAudioBeginPlay(0);
        LoadTimList((s32 *)LoadCursor);
        FrmTaskState = FRAME_TASK_REVEAL;
        initialState = (FrameTaskState *)LoadCursor;
        ActiveFrameTaskScript = initialState;
        LoadCursor = (u8 *)(initialState + 1);
        initialState->fields.intensity[0] = 0;
        initialState->fields.intensity[1] = 0;
        initialState->fields.intensity[2] = 0;
        initialState->fields.ramp[0] = 0;
        initialState->fields.ramp[1] = 0;
        initialState->fields.ramp[2] = 8;
        initialState->fields.spriteTick = 0;
        initialState->fields.frame = 0;
    case FRAME_TASK_REVEAL:
        revealState = &ActiveFrameTaskScript->fields;
        if (revealState->frame < 0x40) {
            if (revealState->frame == 8) {
                revealState->ramp[0] = 4;
            } else if (revealState->frame == 0x20) {
                revealState->ramp[1] = 4;
            }
            break;
        }
        FrmTaskState = FRAME_TASK_FADE;
    case FRAME_TASK_FADE:
        SetMenuFrameOptions(1, 1, 0);
        if (FadeCountdown > 0) {
            FadeCountdown--;
            AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x40 - FadeCountdown) * 4, 0x45);
            if (FadeCountdown == 0) {
                ModeGroup = 1;
                ModeIndex = 3;
                SubmodeState = 0;
            }
        } else if (MenuReturnSubmodeIndex) {
            FadeCountdown = 0x40;
            CdAudioFade(0, 0x40);
        }
        break;
    default:
        break;
    }
    /* Accumulators wrap through the corresponding unsigned halfword view. */
    rampState = &ActiveFrameTaskScript->fields;
    RampValueToLimit((u16 *)&rampState->intensity[0], &rampState->ramp[0], 0x80);
    rampState = &ActiveFrameTaskScript->fields;
    RampValueToLimit((u16 *)&rampState->intensity[1], &rampState->ramp[1], 0x80);
    rampState = &ActiveFrameTaskScript->fields;
    RampValueToLimit((u16 *)&rampState->intensity[2], &rampState->ramp[2], 0x80);
    EmitPresetSprite(ActiveFrameTaskScript->fields.intensity[0], 0x2F);
    EmitPresetSprite(ActiveFrameTaskScript->fields.intensity[1], 0x30);
    animationState = ActiveFrameTaskScript;
    EmitSpriteGrid(animationState->fields.intensity[2], &animationState->grid);
    scratch = RR_GPU_SCRATCH;
    packet = (GpuCommandPacket *)scratch->cursor;
    packet->command = 0xE100022A;
    RR_GPU_ADD_PRIM(scratch->order, packet, 1);
    scratch->order->length = 0;
    scratch->cursor += sizeof(GpuCommandPacket);
    UpdateScreenFade(0x1000C);
    ActiveFrameTaskScript->fields.frame++;
}


/* rr/video_timing.h: chosen by the host before MainLoop. */
int VideoTimingNtsc;

void MainLoop(void) {
    s32 threshold;
    u32 bank;
    FrameStorage *frame;
    MainLoopStartupHook();
    /* MODE_PAL, or MODE_NTSC for NTSC timing; the Japanese release sets
     * MODE_NTSC in SystemInit. */
    if (!RR_RELEASE_JP)
        SetVideoMode(RR_PAL_NTSC(1, 0));
    ResetCallback();
    SystemInit();
    VSyncThreshold=128;
    SetDispMask(0);
    ModeFrame=0;
    ModeGroup=1;
    ModeIndex=1;
    SubmodeState=0;
    FrameCounter=0;
    Psyz_GteSetPgxpLibraryStores(1);
    Psyz_GteSetPgxpNclip(1);
    Psyz_GteSetPgxpIntpl(1);
    for (;;) {
        /* The previous frame's ordering table went to the GPU (DrawOTag
         * below, which copies its precise vertices along). */
        Psyz_PgxpNextFrame();
        {
            s32 length=0x2C0;
            FrameStorage *frames;
            GpuScratchPointers *scratch;
            GpuDmaTag *ordering;
            frames=DrawBuffers;
            bank=FrameCounter;
            bank &=1;
            frame=&frames[bank];
            ordering=frame->buffer.ordering70.tags;
            CurrentDrawBuf=frame;
            DrawBufIndex=bank;
            scratch=RR_GPU_SCRATCH;
            scratch->order=ordering;
            scratch->cursor=frame->buffer.packets1670;
            ClearOTagR(ordering,length);
        }
        ClearOTagR(CurrentDrawBuf->buffer.secondaryOrderingB70.tags,0x2C0);
        CdFileStep();
        SoundUpdate();
        WideBeginFrame();
        InterpBeginFrame();
        ModeFuncTable[ModeGroup][ModeIndex]();
        DrawSync(0);
        /* The frame drawn last is shown at the next VSync(0); a frame of two
         * fields shows the midpoint to it at the blank between. */
        if (InterpActive)
            Psyz_GpuPrepareInterpolatedBlank(DrawBuffers[bank ^ 1].buffer.environment00.draw00.clip.y);
        threshold=VSyncThreshold;
        do {} while (VSync(1)<threshold);
        VSync(0);
        PutDispEnv(&CurrentDrawBuf->buffer.environment00.display5C);
        PutDrawEnv(&CurrentDrawBuf->buffer.environment00.draw00);
        InterpCommitFrame(DrawBuffers[bank ^ 1].buffer.environment00.draw00.clip.y,
                          &CurrentDrawBuf->buffer.environment00.draw00.clip);
        WideCommitFrame(CurrentDrawBuf->buffer.environment00.draw00.clip.y,
                        CurrentDrawBuf->buffer.environment00.draw00.clip.w,
                        CurrentDrawBuf->buffer.environment00.draw00.clip.h,
                        &CurrentDrawBuf->buffer.ordering70.tags[703],
                        DrawSubOtFlag ? &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[703] : NULL);
        DrawOTag(&CurrentDrawBuf->buffer.ordering70.tags[703]);
        if(DrawSubOtFlag) DrawOTag(&CurrentDrawBuf->buffer.secondaryOrderingB70.tags[703]);
        PadUpdate();
        FrameCounter++;
    }
}

/* Frame interpolation identities (rr/frame_interp.h). */
u32 InterpObject;
uintptr_t InterpInstance;
s32 InterpActive;
/* A cut starts a new generation of identities: globally (InterpEpoch) or for
 * one view's camera (InterpViewEpoch; the mirror follows view 0). */
static u32 InterpEpoch;
static u32 InterpViewEpoch[8];
static struct {
    u32 frame; /* FrameCounter + 1 of the last pose, 0 for none */
    s32 position[3], angles[3];
} InterpViewPose[8];
static s32 InterpLastGroup = -1, InterpLastIndex = -1;

/* Camera jumps between two frames that count as a cut: world units and
 * 12-bit angle units (192 = 17 degrees). */
enum { INTERP_CUT_MOVE = 768, INTERP_CUT_TURN = 192 };

static inline u64 InterpMix(u64 x) {
    x ^= x >> 30;
    x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27;
    x *= 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

_Static_assert(INTERP_ANY_TEXTURE == PSYZ_INTERP_ANY_TEXTURE, "PsyZ tag flag");
_Static_assert(INTERP_MIDPOINT_ONLY == PSYZ_INTERP_MIDPOINT_ONLY, "PsyZ tag flag");
_Static_assert(INTERP_FRAME_ONLY == PSYZ_INTERP_FRAME_ONLY, "PsyZ tag flag");
_Static_assert(INTERP_NO_POSE == PSYZ_INTERP_NO_POSE, "PsyZ tag flag");

u32 InterpViewGeneration(void) {
    u32 view = (u32)ActiveViewportIndex & 7;
    return InterpEpoch + InterpViewEpoch[view == 1 ? 0 : view];
}

void InterpTagShape(const void *shape, uintptr_t key, u32 piece, unsigned flags) {
    u32 view = (u32)ActiveViewportIndex & 7;
    u32 epoch = InterpViewGeneration();
    u64 id = InterpMix((u64)key ^ InterpMix((u64)InterpInstance));
    id = InterpMix(id ^ ((u64)InterpObject << 32 | piece));
    id = InterpMix(id ^ ((u64)epoch << 8 | view));
    Psyz_GpuTagInterpolationPacketFlags(shape, id, flags);
}

void InterpCameraCut(void) {
    InterpEpoch++;
}

/* Wrapped difference of two 12-bit angles. */
static s32 InterpAngleDelta(s32 a, s32 b) {
    s32 d = (a - b) & 0xFFF;
    return d >= 0x800 ? 0x1000 - d : d;
}

void InterpViewCamera(s32 view, const s32 position[3], const s32 angles[3]) {
    s32 slot = (view & 7) == 1 ? 0 : (view & 7);
    s32 move = 0, turn = 0;
    if (!InterpActive || (view & 7) == 1)
        return;
    if (InterpViewPose[slot].frame == FrameCounter + 1)
        return; /* already seen this frame */
    for (int i = 0; i < 3; i++) {
        s32 d = abs(position[i] - InterpViewPose[slot].position[i]);
        s32 a = InterpAngleDelta(angles[i], InterpViewPose[slot].angles[i]);
        move = d > move ? d : move;
        turn = a > turn ? a : turn;
        InterpViewPose[slot].position[i] = position[i];
        InterpViewPose[slot].angles[i] = angles[i];
    }
    /* A chase camera at top speed moves ~300 units and turns a few
     * degrees per frame; a replay or intro cut jumps far more. */
    if (InterpViewPose[slot].frame != FrameCounter || move > INTERP_CUT_MOVE || turn > INTERP_CUT_TURN)
        InterpViewEpoch[slot]++;
    InterpViewPose[slot].frame = FrameCounter + 1;
}

void InterpBeginFrame(void) {
    InterpActive = Psyz_GpuGetFrameInterpolation();
    InterpObject = 0;
    if (ModeGroup != InterpLastGroup || ModeIndex != InterpLastIndex)
        InterpCameraCut();
    InterpLastGroup = ModeGroup;
    InterpLastIndex = ModeIndex;
}

void InterpCommitFrame(s32 previousY, const GpuRectangleFields *clip) {
    InterpObject = 0;
    Psyz_GpuBeginInterpolationFrame();
    if (InterpActive)
        Psyz_GpuInterpolationBackdrop(clip->x, previousY, clip->y, clip->w, clip->h);
}

/* Initial values of the globals this module owns. */
u32 FrameCounter;
FrameTaskState *ActiveFrameTaskScript;
s32 FrmTaskState;
