#include "rr/field_rate.h"
#include "rr/cpu_work.h"
#include <string.h>
#include "rr/cpu_time.h"
#include "rr/gpu_address.h"
#include "game/data/menuvars.h"
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
#include <psyz/gpu.h>
#include <stdlib.h>
#include <stdio.h>
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
#include "rr/replay_host.h"

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

/* The Japanese release's frame task (SLPS-01800): eight preset sprites
 * (0x2F..0x36) fade in at their frames, the sprite grid from the start and
 * a fade tile from frame 0x70; at frame 0x9C it waits for a button. */
static void FrmTaskModeUpdateJp(void) {
    FrameTaskFieldsJp *st;
    GpuScratchPointers *scratch;
    GpuCommandPacket *packet;
    s32 i;

    switch (FrmTaskState) {
    case FRAME_TASK_READ_START:
        if (CdFileReadStart(GetR4FileIndex(0, 0x15), LoadCursor)) {
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
        st = (FrameTaskFieldsJp *)LoadCursor;
        ActiveFrameTaskScript = (FrameTaskState *)st;
        LoadCursor = (u8 *)(st + 1);
        memset(st, 0, sizeof(*st));
        st->ramp[8] = 8;
        st->fade26 = 0x100;
        /* fallthrough */
    case FRAME_TASK_REVEAL:
        st = (FrameTaskFieldsJp *)ActiveFrameTaskScript;
        if (st->frame < 0x9C) {
            switch (st->frame) {
            case 0x08: st->ramp[0] = 4; break;
            case 0x20: st->ramp[1] = 4; break;
            case 0x34: st->ramp[2] = 4; break;
            case 0x40: st->ramp[3] = 4; break;
            case 0x4C: st->ramp[4] = 4; break;
            case 0x58: st->ramp[5] = 4; break;
            case 0x64: st->ramp[6] = 4; break;
            case 0x70: st->fade2A = 0x200; break;
            case 0x7C: st->ramp[7] = 4; break;
            }
            break;
        }
        FrmTaskState = FRAME_TASK_FADE;
        /* fallthrough */
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
        return;
    }
    st = (FrameTaskFieldsJp *)ActiveFrameTaskScript;
    for (i = 0; i < 9; i++) {
        RampValueToLimit((u16 *)&st->intensity[i], &st->ramp[i], 0x80);
    }
    RampValuePairTo4096((u16 *)&st->fade26, (u16 *)&st->fade28, &st->fade2A);
    for (i = 0; i < 8; i++) {
        EmitPresetSprite(st->intensity[i], 0x2F + i);
    }
    EmitSpriteGridTick(st->intensity[8], &st->spriteTick);
    EmitFadeTile(st->fade26, st->fade28, 0xE80020, 0xF0);
    scratch = RR_GPU_SCRATCH;
    packet = (GpuCommandPacket *)scratch->cursor;
    packet->command = 0xE100022A;
    RR_GPU_ADD_PRIM(scratch->order, packet, 1);
    scratch->order->length = 0;
    scratch->cursor += sizeof(GpuCommandPacket);
    UpdateScreenFade(0x1000C);
    st->frame++;
}

void FrmTaskModeUpdate(void) {
    FrameTaskFields *rampState;
    FrameTaskState *initialState;
    FrameTaskFields *revealState;
    FrameTaskState *animationState;
    GpuScratchPointers *scratch;
    GpuCommandPacket *packet;
    int fileIndex;

    if (RR_RELEASE_JP) {
        FrmTaskModeUpdateJp();
        return;
    }
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
    if (!InitFramePacketStorage()) {
        fprintf(stderr, "RR4: cannot protect frame packet arenas\n");
        abort();
    }
    MainLoopStartupHook();
    /* MODE_PAL, or MODE_NTSC for NTSC timing; the Japanese release sets
     * MODE_NTSC in SystemInit. */
    if (!RR_RELEASE_JP)
        SetVideoMode(RR_PAL_NTSC(1, 0));
    /* The console gets here 6.484 ms after a vblank (the BIOS's hand-over
     * and crt0) and libetc's first ResetCallback takes 62.3k clocks; that
     * phase sets where the boot's CD commands and the sequencer's root
     * counter (SsStart) fall in their fields (rr/cpu_time.h). The port's
     * fields start here. */
    RR_CPU_CYCLES(RR_CYCLES_BOOT_TO_MAIN_LOOP + RR_CYCLES_FIRST_RESET_CALLBACK);
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
        Psyz_GpuTimingClearHostPackets();
        RrWorkFrame = 0;
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
        RR_CPU_CYCLES(RR_CYCLES_FRAME_LOOP_CLEARS);
        ClearOTagR(CurrentDrawBuf->buffer.secondaryOrderingB70.tags,0x2C0);
        RR_CPU_CYCLES(RR_CYCLES_FRAME_LOOP_STEP);
        CdFileStep();
        /* the sound's timers and fades by the original tick (rr/field_rate.h) */
        if (RaceSlowTick())
            SoundUpdate();
        WideBeginFrame();
        if (ReplayHost)
            ReplayHost->frame();
        ModeFuncTable[ModeGroup][ModeIndex]();
        /* Guards stop out-of-bounds writes. Also reject an invalid final
         * cursor before GPU dispatch, even if no packet was written there. */
        {
            size_t used = (uintptr_t)RR_GPU_SCRATCH->cursor -
                          (uintptr_t)frame->buffer.packets1670;
            if (used > sizeof(frame->buffer.packets1670)) {
                fprintf(stderr, "RR4: frame packet arena overflow: %zu > %zu bytes, frame %d\n",
                        used, sizeof(frame->buffer.packets1670), FrameCounter);
                abort();
            }
            static size_t peak;
            static int stats = -1; /* read once: no environment walk a frame */
            if (stats < 0)
                stats = getenv("RR4_PACKET_STATS") != NULL;
            if (stats && used > peak) {
                peak = used;
                fprintf(stderr, "RR4: packet peak %zu / %zu bytes\n",
                        peak, sizeof(frame->buffer.packets1670));
            }
        }
        DrawSync(0);
        threshold=VSyncThreshold;
        do {} while (VSync(1)<threshold);
        VSync(0);
        PutDispEnv(&CurrentDrawBuf->buffer.environment00.display5C);
        RR_CPU_CYCLES(RR_CYCLES_FRAME_LOOP_ENV);
        PutDrawEnv(&CurrentDrawBuf->buffer.environment00.draw00);
        RR_CPU_CYCLES(RR_CYCLES_FRAME_LOOP_ENV);
        WideCommitFrame(CurrentDrawBuf->buffer.environment00.draw00.clip.y,
                        CurrentDrawBuf->buffer.environment00.draw00.clip.w,
                        CurrentDrawBuf->buffer.environment00.draw00.clip.h,
                        &CurrentDrawBuf->buffer.ordering70.tags[703],
                        DrawSubOtFlag ? &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[703] : NULL);
        DrawOTag(&CurrentDrawBuf->buffer.ordering70.tags[703]);
        if(DrawSubOtFlag) {
            RR_CPU_CYCLES(RR_CYCLES_FRAME_LOOP_SUB_OT);
            DrawOTag(&CurrentDrawBuf->buffer.secondaryOrderingB70.tags[703]);
            RR_CPU_CYCLES(RR_CYCLES_FRAME_LOOP_SUB_OT_PAD);
        } else {
            RR_CPU_CYCLES(RR_CYCLES_FRAME_LOOP_PAD);
        }
        PadUpdate();
        RaceFieldRateNextField();
        FrameCounter++;
        RR_CPU_CYCLES(RR_CYCLES_FRAME_LOOP_TOP);
    }
}

/* Initial values of the globals this module owns. */
u32 FrameCounter;
FrameTaskState *ActiveFrameTaskScript;
s32 FrmTaskState;
