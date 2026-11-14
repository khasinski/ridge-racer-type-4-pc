#include "rr/release.h"
#include "rr/overlay_module.h"
#include "rr/overlay665_calibration.h"
#include "game/system/pad.h"
#include "rr/frame_storage_types.h"
#include "rr/overlay665_panels.h"

void ovl665_init_screen_adjust(void)
{
    ClearScreenOverlayState();
    MenuCursor = 0;
    InitBadgeRow(0, 12, 304, 296, 88, 0, 0);
    BadgeRows[0].half02 = 1;
    SetMenuFrameOptions(1, 1, 0);
    if (!RR_RELEASE_JP)
        DebugPrintStub(ovl665_screen_offset_debug_fmt, Options.screenX34, Options.screenY38);
}

void ovl665_update_screen_adjust(void) {
    s32 i;
    void *cursor;
    GpuSingleCommandPacket *packet;
    s16 timer;
    FrameStorage *context;
    switch(ovl665_options_state.layout0C) {
    case 0:
        ovl665_init_screen_adjust();
        SetPresentationDrawEnabled(0);
        ovl665_options_state.layout0C=(ovl665_options_state.layout0C+1U);
        break;
    case 1:
        if(BadgeRows[0].state10==3)ovl665_options_state.layout0C=2;
        break;
    case 2:
        if(PlayerControllers[0].edges08.fields.repeat&0x8000)Options.screenX34=(Options.screenX34-1U);
        if(PlayerControllers[0].edges08.fields.repeat&0x2000)Options.screenX34=(Options.screenX34+1U);
        if(PlayerControllers[0].edges08.fields.repeat&0x1000)Options.screenY38=(Options.screenY38-2U);
        if(PlayerControllers[0].edges08.fields.repeat&0x4000)Options.screenY38=(Options.screenY38+2U);
        /* The Japanese release allows x -15..15 and y 0..16, default 0. */
        if(Options.screenX34 < RR_PAL_JP(-10,-15))Options.screenX34=RR_PAL_JP(-10,-15);
        if(Options.screenX34 >=RR_PAL_JP(11,16))Options.screenX34=RR_PAL_JP(10,15);
        if(Options.screenY38<0)Options.screenY38=0;
        if(Options.screenY38>=RR_PAL_JP(33,17))Options.screenY38=RR_PAL_JP(32,16);
        if(PlayerControllers[0].edges08.fields.pressed&0x800) {
            Options.screenX34=0;
            Options.screenY38=RR_PAL_JP(16,0);
        } else if(PlayerControllers[0].action30) {
            PlaySfx(1);
            BadgeRows[0].state10=4;
            FadeCountdown=64;
            ovl665_options_state.layout0C=(ovl665_options_state.layout0C+1U);
        }
        for(i=0;i<2;i++) {
            DrawBuffers[i].environment.display5C.screen.x=(s16)Options.screenX34;
            DrawBuffers[i].environment.display5C.screen.y=(s16)Options.screenY38;
        }
        break;
    case 3:
        timer=(u16)FadeCountdown-1;
        FadeCountdown=timer;
        AddScreenFadeTiles(CurrentDrawBuf->buffer.ordering70.tags,(64-timer)*4,69);
        if(FadeCountdown<=0 && BadgeRows[0].state10==11) {
            ovl665_init_menu();
            ovl665_options_state.layout0C=0;
            ovl665_options_state.scene00=(ovl665_options_state.scene00-1U);
            return;
        }
        break;
    }
    cursor=RR_GPU_SCRATCH->cursor;
    for(i=0;i<2;i++) {
        cursor=AddSpritePrim(CurrentDrawBuf->buffer.ordering70.tags,cursor,160,128+i*96,&ovl665_screen_frame_uv_rects[i],0x7F38,255);
    }
    for(i=0;i<2;i++) {
        cursor=AddSpritePrim(CurrentDrawBuf->buffer.ordering70.tags,cursor,136+i*48,176,&ovl665_screen_arrow_uv_rects[i],0x7F38,255);
    }
    packet=(GpuSingleCommandPacket *)cursor;
    RR_GPU_SCRATCH->cursor=(u8 *)packet;
        context=CurrentDrawBuf;
    packet->command=0xE1000259;
    RR_GPU_ADD_PRIM(&context->buffer.ordering70.tags[0],packet,1);
    packet++;
    RR_GPU_SCRATCH->cursor=(u8 *)packet;
    CardDialogueStatePtr->b4=26;
    UpdateBadgeRowAnim(0);
    if(BadgeRows[0].state10==3)UpdateDialogue(BadgeRows[0].x04+10,BadgeRows[0].y06+40,CardDialogueStatePtr);
    UpdateScreenFade(0x1007A);
}

void ovl665_init_brightness_panel(void)
{
    ClearScreenOverlayState();
    ScreenFadeState.phase = 2;
    InitBadgeRow(0, 12, 304, 296, 88, 0, 0);
    BadgeRows[0].half02 = 1;
    SetMenuFrameOptions(1, 1, 0);
}

void ovl665_update_brightness_panel(void) {
    s32 i,x;
    const UiUvRect *glyph;
    void *cursor;
    GpuSingleCommandPacket *packet;
    switch(ovl665_options_state.layout0C) {
    case 0:
        ovl665_init_brightness_panel();
        SetPresentationDrawEnabled(0);
        ovl665_options_state.layout0C=(ovl665_options_state.layout0C+1U);
        break;
    case 1:
        if(BadgeRows[0].state10==3)ovl665_options_state.layout0C=2;
        break;
    case 2:
        if(MenuReturnSubmodeIndex) {
            PlaySfx(1);
            BadgeRows[0].state10=4;
            FadeCountdown=64;
            ovl665_options_state.layout0C=(ovl665_options_state.layout0C+1U);
        }
        break;
    case 3:
        FadeCountdown--;
        AddScreenFadeTiles(CurrentDrawBuf->buffer.ordering70.tags,(64-FadeCountdown)*4,69);
        if(FadeCountdown<=0 && BadgeRows[0].state10==11) {
            ovl665_init_menu();
            ovl665_options_state.layout0C=0;
            ovl665_options_state.scene00=(ovl665_options_state.scene00-1U);
            return;
        }
        break;
    }
    cursor=RR_GPU_SCRATCH->cursor;
    i=0;
    glyph=&ovl665_brightness_swatch_uv_rect;
    x=24;
    for(;i<8;x+=36,i++) {
        cursor=AddSpritePrim(CurrentDrawBuf->buffer.ordering70.tags,cursor,x,144,glyph,0x7F38,((7-i)*223)/7+32);
    }
    packet=(GpuSingleCommandPacket*)cursor;
    RR_GPU_SCRATCH->cursor=(u8*)packet;
        {
        FrameStorage *context=CurrentDrawBuf;
        packet->command=0xE1000239U;
        RR_GPU_ADD_PRIM(&context->buffer.ordering70.tags[0],packet,1);
        packet++;
        RR_GPU_SCRATCH->cursor=(u8*)packet;
    }
    for(i=0;i<8;i++) {
        DrawHudDigit(0,28+i*36,192,128);
        DrawHudDigit(i+1,34+i*36,192,128);
    }
    packet=(GpuSingleCommandPacket*)RR_GPU_SCRATCH->cursor;
        {
        FrameStorage *context=CurrentDrawBuf;
        packet->command=0xE1000238U;
        RR_GPU_ADD_PRIM(&context->buffer.ordering70.tags[0],packet,1);
        packet++;
        RR_GPU_SCRATCH->cursor=(u8*)packet;
    }
    CardDialogueStatePtr->b4=27;
    UpdateBadgeRowAnim(0);
    if(BadgeRows[0].state10==3) {
        UpdateDialogue(BadgeRows[0].x04+10,BadgeRows[0].y06+40,CardDialogueStatePtr);
    }
    UpdateScreenFade(0x1007C);
}

/* Initial values of the overlay globals this module owns. */
const char ovl665_screen_offset_debug_fmt[16] = {83, 99, 114, 101, 101, 110, 58, 37, 100, 44, 32, 37, 100, 10};
UiUvRect ovl665_screen_frame_uv_rects[2] = {{.u = 0xD8, .v = 0x58, .w = 8, .h = 0x10}, {.u = 0xE0, .v = 0x58, .w = 8, .h = 0x10}};
UiUvRect ovl665_brightness_swatch_uv_rect = {.u = 0x28, .v = 0x78, .w = 0x14, .h = 0x28};

/* Writable state of this unit; ovl665_reset_module restores it. */
static const OverlayStateRegion ovl665_screen_adjust_regions[] = {
    RR_OVERLAY_STATE(ovl665_brightness_swatch_uv_rect),
    RR_OVERLAY_STATE(ovl665_screen_frame_uv_rects)};
const OverlayStateTable ovl665_screen_adjust_state = {ovl665_screen_adjust_regions, sizeof(ovl665_screen_adjust_regions) / sizeof(ovl665_screen_adjust_regions[0])};
