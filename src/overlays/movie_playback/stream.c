#include "rr/overlay_module.h"
#include "rr/overlay668_resource.h"
#include "rr/overlay668_playback_init.h"
#include "rr/load_cursor.h"
#include "rr/overlay668_dispatch.h"
#include "rr/overlay668_stream.h"
#include "game/system/pad.h"
#include "rr/overlay668_playback_step.h"
#include "rr/overlay668_decode.h"
#include "rr/cd.h"
#include "rr/cpu_time.h"
#include "rr/overlay668_display.h"
#include "rr/overlay668_frame_submit.h"
#include "rr/overlay668_frame_acquire.h"

void ovl668_resolve_stream_file(s32 arg0, StrCdFileInfo *arg1, u32 *arg2) {
    /* Existing PAL compiler frame reserves this area; it carries no data. */
    u8 reserved_stack10[8];
    /* Emit 0x0FFFFFFF mask in $3 before &SoundState and offset math */
    u32 mask = 0x0FFFFFFF;
    /* Hold &SoundState in $2 across offset calculation */
    AudioState *base;
    /* Keep offset addition in $4 before loading archive2638 */
    s32 offset;
    StrArchiveEntry *entry;
    u32 v0;
    (void)reserved_stack10;
    base = &SoundState;
    offset = (arg0 * 8U + 0x130U);
    entry = (StrArchiveEntry *)(base->streamBanks[0].table + offset);
    CdIntToPos(((entry->position & mask) + base->streamBanks[0].sector), arg1->location.c);
    arg1->byteSize = entry->sectors << 11;
    *arg2 = entry->sectors;
    RaceContext.streamStartBC = arg1->location;
    RaceContext.streamCurrentC0 = RaceContext.streamStartBC;
    v0 = (*arg2 / 6) - 20;
    RaceContext.streamFrameLimitC8 = v0;
    RaceContext.streamFrameBudgetCC = v0 * 2;
}

void ovl668_init_stream_playback(void)
{
    StrCdFileInfo file;
    u32 sectors;
    StrPlaybackState *state;

    SetDispMask(0);
    SetupDrawDispEnvs(0, 0, 0, 5);
    state = RR_ARENA_ALIGN(MenuArenaBase, StrPlaybackState);
    LoadCursor = state;
    RaceContext.streamStateB8 = state;
    LoadCursor = (u8 *)state + sizeof(*state);
    PsyqBzero(state, sizeof(*state));
    ResetGraph(3);
    CdSync(0, 0);
    CdControl(9, 0, 0);
    RaceContext.streamPhaseB4 = 0;
    RaceContext.streamStopD0 = 0;
    ovl668_resolve_stream_file(RaceContext.streamResourceB0, &file, &sectors);
    ModeGroup = 2;
    ModeIndex = 4;
    SubmodeState = 0;
}

void ovl668_update_stream_mode(void) {
    switch (RaceContext.streamPhaseB4) {
    case 0:
        ovl668_start_stream_playback(LoadCursor);
        break;
    case 1:
        ovl668_step_stream_playback();
        break;
    case 2:
        ovl668_stop_stream_playback();
        break;
    }
}

void ovl668_start_stream_playback(u8 *arg0) {
    s32 vol;
    SetDispMask(0);
    ((StrPlaybackState *)RaceContext.streamStateB8)->width00 = 320;
    ((StrPlaybackState *)RaceContext.streamStateB8)->height04 = 240;
    SsSetSerialAttr(0, 0, 1);
    vol = (Options.bgmVolume30 * 127) / 16;
    SsSetSerialVol(0, vol, vol);
    VSyncThreshold = 128;
    ovl668_init_stream_buffers(arg0);
    ovl668_init_stream_display(&((StrPlaybackState *)RaceContext.streamStateB8)->display24, 0, 24, 0, 264);
    ovl668_setup_stream_ring(ovl668_on_mdec_slice_ready);
    while (ovl668_decode_next_frame(&((StrPlaybackState *)RaceContext.streamStateB8)->display24) == -1) {
        ovl668_seek_and_read_stream(RaceContext.streamCurrentC0.c);
    }
    RaceContext.streamPhaseB4 = 1;
    RaceContext.streamFrameC4 = 0;
    ModeFrame = 0;
    VSync(0);
}

void ovl668_step_stream_playback(void)
{
    s32 timer;
    u32 frame;
    CdLocation *location;
    SceneRaceContext *context;
    s32 pixels;
    SceneRaceContext *initialContext;

    timer = (ModeFrame + 1U);
    ModeFrame = timer;
    if (timer == 4) SetDispMask(1);
    initialContext = &RaceContext;
    ovl668_mdec_in(((StrPlaybackState *)initialContext->streamStateB8)->commands10[((StrPlaybackState *)initialContext->streamStateB8)->display24.word00], 3);
    pixels = ((StrPlaybackState *)initialContext->streamStateB8)->display24.rectangles[2].w * ((StrPlaybackState *)initialContext->streamStateB8)->display24.rectangles[2].h;
    context = initialContext;
    location = &context->streamCurrentC0;
    ovl668_mdec_out(((StrPlaybackState *)initialContext->streamStateB8)->buffers18[((StrPlaybackState *)initialContext->streamStateB8)->display24.word08], pixels / 2);
retry:
    if (ovl668_decode_next_frame(&((StrPlaybackState *)context->streamStateB8)->display24) == -1) {
        frame = StGetBackloc(location);
        DebugPrintStub(ovl668_backloc_debug_fmt, frame);
        if (frame >= context->streamFrameLimitC8) {
            context->streamStopD0 = 1;
            goto complete;
        }
        if ((s32)frame <= 0) context->streamCurrentC0 = context->streamStartBC;
        ovl668_seek_and_read_stream(location->c);
        goto retry;
    }
complete:
    ovl668_wait_frame_ready(&((StrPlaybackState *)RaceContext.streamStateB8)->display24, 0);
    DrawSync(0);
    ovl668_mdec_reset(1);
    if (RaceContext.streamStopD0 == 1) {
        RaceContext.streamPhaseB4 = 2;
        SsSetSerialVol(0, 0, 0);
    }
    if (PlayerControllers[0].edges08.fields.pressed & 0x800) {
        SsSetSerialVol(0, 0, 0);
        RaceContext.streamPhaseB4 = 2;
    }
}

void StUnSetRing(void);
void ovl668_stop_stream_playback(void){
 ovl668_mdec_out_callback(0);
 StUnSetRing();
 CdControlB(9,0,0);
 ModeGroup=1;ModeIndex=3;SubmodeState=0;
}

void ovl668_init_stream_buffers(u8 *base){
 ((StrPlaybackState*)RaceContext.streamStateB8)->commands10[0]=(StrCommandHeader*)base;
 base+=0xa000;
 ((StrPlaybackState*)RaceContext.streamStateB8)->commands10[1]=(StrCommandHeader*)base;
 base+=0xa000;
 ((StrPlaybackState*)RaceContext.streamStateB8)->buffers18[0]=base;
 base+=0x2d00;
 ((StrPlaybackState*)RaceContext.streamStateB8)->buffers18[1]=base;
 base+=0x2d00;
 ((StrPlaybackState*)RaceContext.streamStateB8)->queue20=(LibQueueRecord32*)base;
}

void ovl668_init_stream_display(StrDisplayConfig*p,s32 x,s32 y,s32 second_x,s32 second_y){
 p->rectangles[0].w=480;
 p->rectangles[1].w=480;
 p->rectangles[0].x=x;
 p->rectangles[0].y=y;
 p->rectangles[0].h=240;
 p->rectangles[1].x=second_x;
 p->rectangles[1].h=240;
 p->rectangles[2].x=x;
 p->rectangles[2].y=y;
 p->rectangles[2].w=24;
 p->rectangles[2].h=240;
 p->rectangles[1].y=second_y;
 p->alternate04=~DrawBufIndex&1;
}

void ovl668_setup_stream_ring(LibInterruptCallback callback) {
    ovl668_mdec_reset(0);
    ovl668_mdec_out_callback(callback);
    StSetRing(((StrPlaybackState *)RaceContext.streamStateB8)->queue20, 32);
    StSetStream(1, 1, -1, 0, 0);
}

void ovl668_on_mdec_slice_ready(void) {
    GpuRectangle sp10;
    u32 prev2C;

    if (StCdIntrFlag != 0) {
        StCdInterrupt();
        StCdIntrFlag = 0;
    }
    prev2C = ((StrPlaybackState *)RaceContext.streamStateB8)->display24.word08;
    sp10.rect = ((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[2];
    ((StrPlaybackState *)RaceContext.streamStateB8)->display24.word08 = (((StrPlaybackState *)RaceContext.streamStateB8)->display24.word08 == 0);
    ((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[2].x += ((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[2].w;
    if (((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[2].x < ((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[((StrPlaybackState *)RaceContext.streamStateB8)->display24.alternate04].x + ((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[((StrPlaybackState *)RaceContext.streamStateB8)->display24.alternate04].w) {
        ovl668_mdec_out(
            ((StrPlaybackState *)RaceContext.streamStateB8)->buffers18[((StrPlaybackState *)RaceContext.streamStateB8)->display24.word08],
            (((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[2].w * ((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[2].h) / 2
        );
    } else {
        ((StrPlaybackState *)RaceContext.streamStateB8)->display24.ready0C = 1;
        ((StrPlaybackState *)RaceContext.streamStateB8)->display24.alternate04 = (((StrPlaybackState *)RaceContext.streamStateB8)->display24.alternate04 == 0);
        ((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[2].x = ((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[((StrPlaybackState *)RaceContext.streamStateB8)->display24.alternate04].x;
        ((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[2].y = ((StrPlaybackState *)RaceContext.streamStateB8)->display24.rectangles[((StrPlaybackState *)RaceContext.streamStateB8)->display24.alternate04].y;
    }
    LoadImage(&sp10, ((StrPlaybackState *)RaceContext.streamStateB8)->buffers18[prev2C]);
}

/* The PS1 decodes a frame's VLC in software while the MDEC works on the
 * previous one: about 1.32 ms plus 2.55 us per MDEC word it writes
 * (DuckStation, 320x176 frames of 1,400..9,100 words). The frame's ring
 * slots are freed after it, and the MDEC callbacks run meanwhile. */
#define VLC_FRAME_NS 1320000u
#define VLC_WORD_NS 2550u

static void ovl668_spend_vlc_time(const StrCommandHeader *commands) {
    Psyz_SpendTime(VLC_FRAME_NS + VLC_WORD_NS * ovl668_mdec_buf_size(commands));
}

s32 ovl668_decode_next_frame(StrDisplayConfig *config) {
    u8 *data;
    s32 remaining = 2000;
    u32 *index;

    while (1) {
        data = ovl668_acquire_stream_frame(config);
        if (data != 0) {
            break;
        }
        if (--remaining == 0) {
            return -1;
        }
    }
    index = &config->word00;
    *index = *index < 1U;
    ovl668_mdec_decode_vlc(data, ((StrPlaybackState *)RaceContext.streamStateB8)->commands10[*index]);
    ovl668_spend_vlc_time(((StrPlaybackState *)RaceContext.streamStateB8)->commands10[*index]);
    return (s32)StFreeRing(data);
}

u8 *ovl668_acquire_stream_frame(StrDisplayConfig *config) {
 GpuRectangle rectangle;
 u8 *data;LibQueueRecord32 *header;
 s32 retry=2000;StrPlaybackState *playback;
 while(StGetNext(&data,&header)!=0) {if(--retry==0)return 0;}
 if(header->value08>=RaceContext.streamFrameLimitC8)RaceContext.streamStopD0=1;
 if(RaceContext.streamFrameC4>=RaceContext.streamFrameBudgetCC)RaceContext.streamStopD0=1;
 if(((StrPlaybackState*)RaceContext.streamStateB8)->width00!=header->width10 || ((StrPlaybackState*)RaceContext.streamStateB8)->height04!=header->height12) {
  ((StrPlaybackState*)RaceContext.streamStateB8)->width00=header->width10;
  ((StrPlaybackState*)RaceContext.streamStateB8)->height04=header->height12;
  playback=RaceContext.streamStateB8;
  rectangle.rect.x=0;rectangle.rect.y=0;
  rectangle.rect.w=(s32)(playback->width00*3U)/2;
  rectangle.rect.h=512;
  ClearImage(&rectangle,0,0,0);
 }
 config->rectangles[0].y=DrawBuffers[0].environment.display5C.display.y+(s32)(256U-((StrPlaybackState*)RaceContext.streamStateB8)->height04)/2;
 config->rectangles[1].y=DrawBuffers[1].environment.display5C.display.y+(s32)(256U-((StrPlaybackState*)RaceContext.streamStateB8)->height04)/2;
 config->rectangles[1].w=(s32)(((StrPlaybackState*)RaceContext.streamStateB8)->width00*3U)/2;
 config->rectangles[0].w=config->rectangles[1].w;
 config->rectangles[1].h=((StrPlaybackState*)RaceContext.streamStateB8)->height04;
 config->rectangles[0].h=config->rectangles[1].h;
 config->rectangles[2].h=((StrPlaybackState*)RaceContext.streamStateB8)->height04;
 DrawBuffers[1].environment.display5C.display.w=((StrPlaybackState*)RaceContext.streamStateB8)->width00;
 DrawBuffers[0].environment.display5C.display.w=DrawBuffers[1].environment.display5C.display.w;
 DrawBuffers[1].environment.display5C.display.h=((StrPlaybackState*)RaceContext.streamStateB8)->height04;
 DrawBuffers[0].environment.display5C.display.h=DrawBuffers[1].environment.display5C.display.h;
 return data;
}

/* PS1 time of one iteration of the wait below (two uncached loads and a
 * store): the MDEC callbacks, timed on the emulated clock, set ready0C
 * while it spins. */
#define WAIT_FRAME_ITERATION_NS 470

void ovl668_wait_frame_ready(StrDisplayConfig *config,s32 mode)
{
    u32 ready=config->ready0C;
    volatile u32 counter=0x800000;
    if(ready==0) do {
        Psyz_SpendTime(WAIT_FRAME_ITERATION_NS);
        if (--counter==0) {
            config->ready0C=1;
            config->alternate04=!config->alternate04;
            config->rectangles[2].x=config->rectangles[config->alternate04].x;
            config->rectangles[2].y=config->rectangles[config->alternate04].y;
        }
    } while(config->ready0C==0);
    config->ready0C=0;
}

extern s32 CdRead2(s32);

void ovl668_seek_and_read_stream(u8 *location) {
    u8 mode = 0x80;
    do {
        while (CdControl(2, location, 0) == 0) {
        }
        while (CdControl(14, &mode, 0) == 0) {
        }
        VSync(3);
    } while (CdRead2(480) == 0);
}

/* Initial values of the overlay globals this module owns. */
const char ovl668_backloc_debug_fmt[28] = {116, 105, 109, 101, 32, 111, 117, 116, 32, 105, 110, 32, 115, 116, 114, 78, 101, 120, 116, 40, 41, 32, 37, 100, 10};

/* Module reset: the state a fresh load of the movie_playback image gave
 * (initialized data and zeroed BSS); see rr/overlay_module.h. */
extern const OverlayStateTable ovl668_epilogue_credits_state, ovl668_mdec_state;
static const OverlayStateTable *const ovl668_state_tables[] = {&ovl668_epilogue_credits_state, &ovl668_mdec_state};
static OverlayStateImage ovl668_state_image = {ovl668_state_tables, sizeof(ovl668_state_tables) / sizeof(ovl668_state_tables[0]), NULL};

void ovl668_reset_module(void) {
    OverlayStateRestore(&ovl668_state_image);
}

/* Initial values of the globals this module owns. */
FrameStorage DrawBuffers[2];
