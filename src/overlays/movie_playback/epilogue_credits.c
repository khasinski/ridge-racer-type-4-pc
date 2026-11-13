#include "rr/release.h"
#include "rr/overlay_module.h"
#include "rr/overlay668_camera.h"
#include "game/system/pad.h"
#include "rr/overlay668_replay_controller.h"
#include "rr/frame_storage_types.h"
#include "rr/video_timing.h"

/* Remainder of the NTSC-timing credit scroll, in sixths of a unit. */
static s32 ovl668_credit_scroll_sixths;

void ovl668_init_epilogue_credits(void) {
    s32 i;
    TrackSample3C *p;

    SetDispMask(0);
    SetupDrawDispEnvs(0, 0, 0, 1);
    VSyncThreshold = 384;
    InitCourseViewState();
    SetupRenderer(5);
    BuildCameraMatrices();
    p = TrackNodes;
    RaceContext.work444.epilogue.counter444 = 0;
    RaceContext.work444.epilogue.state44C = 0;
    RaceContext.work444.epilogue.counter4A4 = 0;
    RaceContext.work444.epilogue.mode4A8 = 1;
    RaceContext.work444.epilogue.phase4AC = 0;
    RaceContext.work444.epilogue.position450[0] = (p[320].road.x * 4U);
    RaceContext.work444.epilogue.position450[1] = p[320].road.height - 120;
    RaceContext.work444.epilogue.position450[2] = (p[320].road.z * 4U);
    RaceContext.work444.epilogue.pitch460 = 0;
    RaceContext.work444.epilogue.yaw462 = (s16)(0U-(u16)p[320].road.heading);
    RaceContext.work444.epilogue.roll464 = 0;
    RaceContext.work444.epilogue.offset488[0] = 0;
    RaceContext.work444.epilogue.offset488[1] = 0;
    RaceContext.work444.epilogue.offset488[2] = 0;
    RaceContext.work444.epilogue.angle490 = -96;
    ViewUsesCarTrackNode = 0;
    RaceContext.work444.epilogue.timer492 = 0;
    RaceContext.work444.epilogue.timer494 = 0;
    RaceContext.work444.epilogue.node498 = 320;
    ovl668_project_camera_to_track(RaceContext.work444.epilogue.position450);
    for (i = 0; i < 16; i++) {
        ovl668_step_epilogue_camera_yaw();
    }
    ovl668_step_epilogue_camera_pitch();
    ovl668_apply_epilogue_camera();
    InitSceneOverlayState();
    SetSceneOverlayColor(0, 144, 144, 144);
    SetSceneOverlayEnabled(1);
    MaskBitSetting = 1;
    if (RR_RELEASE_JP)
        return;
    EpilogueCreditScrollY = 16000;
    ovl668_credit_scroll_sixths = 0;
    EpilogueCreditScrollStep = ovl668_credit_scroll_speeds[(Options.storyBranch3C * 4U + SelectedTeamIndex)];
}

void ovl668_draw_credit_page(s32 page, s32 brightness);

void ovl668_update_epilogue_credits(void) {
    s32 temp_a1;
    s32 temp_s1;
    s32 temp_v0;
    s32 temp_v0_2;
    s32 temp_v1;
    s32 var_a0;
    s32 var_s0;
EpilogueCameraWork78 *ctx;

    switch (RaceContext.work444.epilogue.counter444) {                          
    case 0:
        ovl668_init_epilogue_credits();
        RaceContext.work444.epilogue.counter444=RaceContext.work444.epilogue.counter444+1U;
        break;
    case 4:
        SetDispMask(1);
    case 1: case 2: case 3:
        RaceContext.work444.epilogue.counter444=RaceContext.work444.epilogue.counter444+1U;
        break;
    case 5:                                        
        var_a0 = -1;
        for(var_s0=0;ovl668_epilogue_events[var_s0].key>=0;var_s0++){
          /* Frames at 25 a second; the NTSC release's table holds 6/5 of
           * each (100, 1800 -> 120, 2160 frames at 30 a second). */
          s32 key = ovl668_epilogue_events[var_s0].key;
          if (VideoTimingNtsc) key = key * 6 / 5;
          if(key==RaceContext.work444.epilogue.state44C){var_a0=ovl668_epilogue_events[var_s0].action;break;}
        }
        switch (var_a0) {                          
        case 0:                                    
            RaceContext.work444.epilogue.phase4AC = 1;
            RaceContext.work444.epilogue.counter4B4 = 0;
            CdAudioBeginPlay(0);
            break;
        case 1:                                    
            RaceContext.work444.epilogue.mode4A8 = 2;
            break;
        }
        ovl668_step_epilogue_camera_yaw();
        ctx=&RaceContext.work444.epilogue;
        ctx->position450[0] = ((u32)ctx->position450[0] + (ctx->delta478[0] / 6));
        temp_a1 = ctx->delta478[2] / 6;
        ctx->position450[2] = ((u32)ctx->position450[2] + temp_a1);
        ovl668_project_camera_to_track(ctx->position450);
        ovl668_step_epilogue_camera_pitch();
        ovl668_apply_epilogue_camera();
        ovl668_emit_letterbox_pass(2);
        temp_s1 = ctx->mode4A8;
        switch (temp_s1) {                         
        case 1:                                    
            temp_v0 = ctx->counter4A4 + 4U;
            ctx->counter4A4 = temp_v0;
            if (temp_v0 >= 0xFF) {
                ctx->counter4A4 = 0xFF;
                ctx->mode4A8 = 0;
}
            AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (255U - ctx->counter4A4), 0x45);
            break;
        case 2:                                    
            temp_v0_2 = ctx->counter4A4 - 4U;
            ctx->counter4A4 = temp_v0_2;
            if (temp_v0_2 <= 0) {
                ctx->counter4A4 = 0;
                ctx->mode4A8 = 0;
                CdAudioFade(0, 0);
                ModeGroup = (s16) temp_s1;
                ModeIndex = 3;
                RaceContext.streamResourceB0 = 1;
                SubmodeState = 0;
            }
            AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (255U - ctx->counter4A4), 0x45);
            break;
        }
        temp_v1 = RaceContext.work444.epilogue.phase4AC;
        if (RR_RELEASE_JP) {
            /* Credit pages: fade in 9 a frame to 160, hold 134 frames, fade
             * out 10 a frame and wait to 150 frames, twelve pages. */
            EpilogueCameraWork78 *e = &RaceContext.work444.epilogue;
            switch (temp_v1) {
            case 1:
                e->reserved4B0 += 9;
                e->counter4B4++;
                if (e->reserved4B0 >= 0xA0) {
                    e->reserved4B0 = 0xA0;
                    e->counter4B4 = 0;
                    e->phase4AC++;
                }
                break;
            case 2:
                if (++e->counter4B4 >= 0x86) {
                    e->phase4AC++;
                    if (e->reserved4B8 == 0xB)
                        e->phase4AC = 5;
                }
                break;
            case 3:
                e->counter4B4++;
                e->reserved4B0 -= 10;
                if (e->reserved4B0 <= 0) {
                    e->reserved4B0 = 0;
                    if (e->counter4B4 >= 0x96) {
                        e->counter4B4 = 0;
                        e->phase4AC = 1;
                        e->reserved4B8++;
                    }
                    if (e->reserved4B8 >= 0xC)
                        e->phase4AC = 0;
                }
                break;
            case 4:
                if (++e->counter4B4 >= 0) {
                    e->counter4B4 = 0;
                    e->phase4AC = 1;
                }
                break;
            }
            if (temp_v1 >= 1 && temp_v1 <= 5 && e->reserved4B0 > 0)
                ovl668_draw_credit_page(e->reserved4B8, e->reserved4B0);
            temp_v1 = 0;
        }
        if (temp_v1 != 0) {
            if (temp_v1 == 1) {
                var_s0 = 0;
                if (VideoTimingNtsc) {
                    /* The scrolling credits are the PAL release's own: at 30
                     * frames a second they move 5/6 of the step a frame, so
                     * they keep their place in the music and end with it. */
                    ovl668_credit_scroll_sixths += EpilogueCreditScrollStep * 5;
                    EpilogueCreditScrollY = (u32)EpilogueCreditScrollY - ovl668_credit_scroll_sixths / 6;
                    ovl668_credit_scroll_sixths %= 6;
                } else {
                    EpilogueCreditScrollY = (u32)EpilogueCreditScrollY - EpilogueCreditScrollStep;
                }
                for(;var_s0<62;var_s0++)ovl668_draw_credit_band(var_s0,160);
            }
        }
        if ((RR_RELEASE_JP ? (Pad0Buttons & 0x800) : (PlayerControllers[0].edges08.fields.pressed & 0x800)) &&
            (RaceContext.work444.epilogue.mode4A8 == 0)) {
            RaceContext.work444.epilogue.mode4A8 = 2;
            CdAudioFade(0, RR_PAL_NTSC(0x15, 0x1A));
        }
        SetupPlayerView(0);
        DrawTrackSceneryForPlayer(0);
        RaceContext.work444.epilogue.state44C = (RaceContext.work444.epilogue.state44C + 1U);
        ovl668_emit_letterbox_pass(1);
        ovl668_emit_letterbox_pass(0);
        /* fallthrough */
    default:                                       
        return;
    }
}

/* Initial values of the overlay globals this module owns. */
ReplayEvent ovl668_epilogue_events[3] = {{.key = 100}, {.key = 1800, .action = 1}, {.key = -1}};
s32 ovl668_credit_scroll_speeds[20] = {28, 21, 24, 28, 31, 25, 28, 31, 30, 23, 31, 32, 29, 22, 25, 30, 27, 22, 26, 30};

/* Writable state of this unit; ovl668_reset_module restores it. */
static const OverlayStateRegion ovl668_epilogue_credits_regions[] = {
    RR_OVERLAY_STATE(ovl668_credit_scroll_speeds),
    RR_OVERLAY_STATE(ovl668_epilogue_events),
    RR_OVERLAY_STATE(ovl668_credit_scroll_sixths)};
const OverlayStateTable ovl668_epilogue_credits_state = {ovl668_epilogue_credits_regions, sizeof(ovl668_epilogue_credits_regions) / sizeof(ovl668_epilogue_credits_regions[0])};

/* Initial values of the globals this module owns. */
s32 EpilogueCreditScrollY;
s32 EpilogueCreditScrollStep;
