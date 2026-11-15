#include "rr/overlay_module.h"
#include "rr/menu_arena_state.h"
#include "rr/overlay662_init.h"
#include "rr/overlay662_arena_init.h"
#include "rr/load_cursor.h"
#include "rr/overlay662_garage.h"
#include "rr/diagnostics.h"
#include "rr/menu_setup.h"
#include "rr/menu_transition.h"
#include "rr/overlay662_transition.h"
#include "rr/overlay662_frame_step.h"

extern const char ovl662_str_enter_editor_clean[];
void InitUiRow(s32,s32,s32,s32,s32,s32,s32,s32,s32,s32,...);
extern const char ovl662_str_enter_editor_restore[];

void ovl662_reset_editor_state(void){
 EmblemEditFrames=0;
 ovl662_editor_workspace.fade00=-1;
 ovl662_editor_workspace.config1C=2;
 ovl662_editor_workspace.word04=0;
 ovl662_editor_workspace.reset08[0]=0;
 ovl662_editor_workspace.reset08[1]=0;
 ovl662_editor_workspace.reset08[2]=0;
 ovl662_editor_workspace.reset08[3]=0;
 ovl662_editor_workspace.reset08[4]=0;
 ovl662_editor_workspace.reset08[5]=0;
 ovl662_editor_workspace.reset08[6]=0;
 ovl662_editor_workspace.reset08[7]=0;
 ovl662_editor_workspace.reset08[8]=0;
 ovl662_editor_workspace.reset08[9]=0;
 ovl662_editor_workspace.config1D=1;
 ovl662_editor_workspace.config1E=0;
 ovl662_editor_workspace.config1F=0;
 ovl662_editor_workspace.config20=0;
 ovl662_editor_workspace.active21=0;
 ovl662_editor_workspace.config22=1;
 ovl662_editor_workspace.layer23=0;
 ovl662_editor_workspace.halfC8=0;
 ovl662_editor_workspace.halfCA=0;
 ovl662_editor_workspace.phaseCC=0;
 ovl662_editor_workspace.shadeD0=0;
 ovl662_editor_workspace.flagsD1=0;
 ovl662_editor_workspace.stateD2=0;
 ovl662_editor_cursor_state.words[0]=0;
 ovl662_editor_cursor_state.words[1]=48;
 ovl662_editor_cursor_state.words[2]=0;
 ovl662_editor_cursor_state.words[3]=0;
 ovl662_editor_cursor_state.words[4]=0;
 ovl662_editor_cursor_state.halves[10]=47;
 ovl662_editor_cursor_state.halves[11]=77;
 ovl662_editor_cursor_state.halves[13]=77;
 ovl662_editor_cursor_state.halves[24]=13;
 ovl662_editor_cursor_state.halves[25]=63;
 ovl662_editor_cursor_state.halves[26]=294;
 ovl662_editor_cursor_state.halves[12]=47;
 ovl662_editor_cursor_state.halves[14]=0;
 ovl662_editor_cursor_state.halves[15]=0;
 ovl662_editor_cursor_state.halves[16]=0;
 ovl662_editor_cursor_state.halves[17]=0;
 ovl662_editor_cursor_state.halves[18]=0;
 ovl662_editor_cursor_state.halves[19]=0;
 ovl662_editor_cursor_state.words[10]=0;
 ovl662_editor_cursor_state.words[11]=0;
 ovl662_editor_cursor_state.halves[27]=346;
 ovl662_editor_cursor_state.words[14]=0;
}

void ovl662_init_editor_arenas(void)
{
    s32 stage,i;
    union {
        u8 *ptr;
        u32 *words;
        GarageTimBytes *tim;
        void *raw;
    } base, alloc;
    u32 *clear;
    u8 *second;
    GarageSelectionPrefix *owner;
    stage=(ovl662_arena_init_stage+1U);
    ovl662_arena_init_stage=stage;
    switch (stage) {
    case 1:
        ovl662_emit_indexed_sprite(-1,0,0,0);
        ovl662_dispatch_editor_actions(-1);
        ovl662_init_option_groups(-1);
        ovl662_reset_editor_state();
        base.ptr=MenuArenas.second+4;
        owner=&ovl662_editor_workspace;
        base.ptr-=((GarageArenaAddress)MenuArenas.second&3U);
        second=base.ptr+0x840;
        /* Retain each publication of the shared arena cursor. */
        LoadCursor = base.ptr;
        *(void *volatile*)&LoadCursor=second;
        owner->buffers24[1]=*(void **)&LoadCursor=(u32*)(base.ptr+0x1080);
        alloc.ptr = base.ptr + 0x18C0;
        LoadCursor = alloc.ptr;
        owner->tim2C[1]=alloc.tim;
        owner->scratch34=*(void **)&LoadCursor=(u32*)(base.ptr+0x2100);
        owner->paletteScratch58[0]=*(void **)&LoadCursor=(u32*)(base.ptr+0x2900);
        owner->paletteScratch58[1]=*(void **)&LoadCursor=(u32*)(base.ptr+0x2920);
        *(void **)&LoadCursor=base.ptr+0x2940;
        owner->image38[0]=(u32*)(base.ptr+0x40);
        owner->image40[0]=(u32*)(base.ptr+0x880);
        owner->palette48[0]=(u32*)(base.ptr+0x14);
        owner->paletteBackup50[0]=(u32*)(base.ptr+0x854);
        owner->image38[1]=(u32*)(base.ptr+0x10C0);
        owner->image40[1]=(u32*)(base.ptr+0x1900);
        owner->palette48[1]=(u32*)(base.ptr+0x1094);
        owner->paletteBackup50[1]=(u32*)(base.ptr+0x18D4);
        GarageSavedDecalTimA=(EmblemTim840 *)base.ptr;
        owner->buffers24[0]=(u32*)base.ptr;
        owner->tim2C[0]=(GarageTimBytes*)second;
        GarageSavedDecalTimB=(EmblemTim840 *)second;
        break;
    case 2:
        StoreImage(&ovl662_vram_emblem_image_rect,ovl662_editor_workspace.image38[0]);
        StoreImage(&ovl662_vram_emblem_clut_rect,ovl662_editor_workspace.palette48[0]);
        clear=ovl662_editor_workspace.image40[0];
        for(i=511;i>=0;i--)*clear++=0;
        break;
    case 3:
        ovl662_arena_init_stage=0;
        ovl662_backup_layer_and_palette();
        ovl662_editor_workspace.dirtyD3=0;
        ClearScreenOverlayState();
        InitBadgeRow(0,12,304,296,88,0,0);
        InitUiRow(0,140,136,28,32,0,0,1,0,104);
        InitUiRow(1,140,176,28,32,0,0,1,0,105);
        ModeGroup=stage;
        ModeIndex=4;
        SubmodeState=0;
        break;
    }
}

void ovl662_setup_editor_buffers(s32 mode){
 u16 x,y;
 ovl662_editor_workspace.fade00=-1;
 ovl662_editor_workspace.word04=0;
 ovl662_editor_workspace.flagsD1=0;
 ovl662_editor_workspace.stateD2=0;
 ovl662_editor_cursor_state.words[0]=0;
 x=ovl662_editor_cursor_state.halves[10];y=ovl662_editor_cursor_state.halves[11];
 ovl662_editor_cursor_state.words[1]=48;
 ovl662_editor_cursor_state.halves[24]=13;
 ovl662_editor_cursor_state.halves[25]=63;
 ovl662_editor_cursor_state.halves[26]=294;
 ovl662_editor_cursor_state.words[2]=0;
 ovl662_editor_cursor_state.words[3]=0;
 ovl662_editor_cursor_state.words[4]=0;
 ovl662_editor_cursor_state.halves[14]=0;
 ovl662_editor_cursor_state.halves[15]=0;
 ovl662_editor_cursor_state.halves[16]=0;
 ovl662_editor_cursor_state.halves[17]=0;
 ovl662_editor_cursor_state.halves[18]=0;
 ovl662_editor_cursor_state.halves[19]=0;
 ovl662_editor_cursor_state.words[10]=0;
 ovl662_editor_cursor_state.words[11]=0;
 ovl662_editor_cursor_state.halves[27]=346;
 ovl662_editor_cursor_state.halves[12]=x;
 ovl662_editor_cursor_state.halves[13]=y;
 if(mode){
 ovl662_editor_workspace.config1C=2;
 ovl662_editor_workspace.config1D=1;
 ovl662_editor_workspace.config22=1;
 ovl662_editor_workspace.reset08[0]=0;
 ovl662_editor_workspace.reset08[1]=0;
 ovl662_editor_workspace.reset08[2]=0;
 ovl662_editor_workspace.reset08[3]=0;
 ovl662_editor_workspace.reset08[4]=0;
 ovl662_editor_workspace.reset08[5]=0;
 ovl662_editor_workspace.reset08[6]=0;
 ovl662_editor_workspace.reset08[7]=0;
 ovl662_editor_workspace.reset08[8]=0;
 ovl662_editor_workspace.reset08[9]=0;
 ovl662_editor_workspace.config1E=0;
 ovl662_editor_workspace.config1F=0;
 ovl662_editor_workspace.config20=0;
 ovl662_editor_workspace.active21=0;
 ovl662_editor_workspace.halfC8=0;
 ovl662_editor_workspace.halfCA=0;
 ovl662_editor_workspace.phaseCC=0;
 ovl662_editor_workspace.shadeD0=0;
 ovl662_editor_cursor_state.halves[10]=67;
 ovl662_editor_cursor_state.halves[11]=149;
 ovl662_editor_cursor_state.halves[12]=67;
 ovl662_editor_cursor_state.halves[13]=149;
 ovl662_editor_cursor_state.words[14]=0;
 }
}

void ovl662_enter_editor_clean(void) {
    DebugPrintStub(ovl662_str_enter_editor_clean);
    ovl662_setup_editor_buffers(0);
    ovl662_editor_workspace.dirtyD3 = 0;
    ClearScreenOverlayState();
    InitBadgeRow(0, 12, 304, 296, 88, 0, 0);
    InitUiRow(0, 140, 136, 28, 32, 0, 0, 1, 0, 104);
    InitUiRow(1, 140, 176, 28, 32, 0, 0, 1, 0, 105);
    ModeGroup = 3;
    ModeIndex = 4;
    SubmodeState = 0;
}

void ovl662_enter_editor_restore(void) {
    DebugPrintStub(ovl662_str_enter_editor_restore);
    ovl662_setup_editor_buffers(1);
    ovl662_init_option_groups(-1);
    ovl662_backup_layer_and_palette();
    ovl662_editor_workspace.dirtyD3 = 0;
    ClearScreenOverlayState();
    InitBadgeRow(0, 12, 304, 296, 88, 0, 0);
    InitUiRow(0, 140, 136, 28, 32, 0, 0, 1, 0, 104);
    InitUiRow(1, 140, 176, 28, 32, 0, 0, 1, 0, 105);
    ModeGroup = 3;
    ModeIndex = 4;
    SubmodeState = 0;
}

void ovl662_draw_editor_transition(void) {
    ovl662_draw_editor_panel_fades();
    ovl662_init_editor_draw_env();
    ovl662_update_editor_overlay();
    {
    union {u32 bits;s32 value;} count;
    count.bits=ovl662_editor_workspace.word04+1U;
    ovl662_editor_workspace.word04=count.value;
    if (count.value >= 28) {
        ModeGroup = 3;
        ModeIndex = 5;
        SubmodeState = 0;
    }
    }
}

void ovl662_step_editor_frame(void) {
    ScreenFrame=(ScreenFrame+1U);
    if (++EmblemEditFrames > 0x014996FFU) {
        EmblemEditFrames = 0x014996FFU;
    }
    ovl662_update_cursor_pulse();
    ovl662_update_editor_cursor();
    ovl662_dispatch_editor_actions(0);
    ovl662_draw_editor_cursor();
    ovl662_draw_editor_panel_fades();
    ovl662_init_editor_draw_env();
    ovl662_update_editor_overlay();
    ovl662_upload_editor_textures();
    GarageActiveDecalTimA = (EmblemTim840 *)ovl662_editor_workspace.buffers24[ovl662_editor_workspace.active21];
    GarageActiveDecalTimB = ovl662_editor_workspace.tim2C[ovl662_editor_workspace.active21];
}

/* Initial values of the overlay globals this module owns. */
const char ovl662_str_enter_editor_clean[16] = {101, 110, 116, 101, 114, 32, 83, 97, 118, 101, 66, 97, 99, 107, 10};
const char ovl662_str_enter_editor_restore[152] = {101, 110, 116, 101, 114, 32, 76, 111, 97, 100, 66, 97, 99, 107, 10, 0, -32, -75, 17, -128, 40, -74, 17, -128, 116, -74, 17, -128, -12, -74, 17, -128, -88, -73, 17, -128, 0, 0, 0, 0, -60, -61, 17, -128, 124, -60, 17, -128, 52, -59, 17, -128, 116, -59, 17, -128, -76, -59, 17, -128, -20, -28, 17, -128, -12, -28, 17, -128, -4, -28, 17, -128, 4, -27, 17, -128, 12, -27, 17, -128, 0, 0, 0, 0, -92, 8, 18, -128, -100, 8, 18, -128, -100, 8, 18, -128, -92, 8, 18, -128, -80, 8, 18, -128, 0, 0, 0, 0, -52, 13, 18, -128, -24, 13, 18, -128, 8, 14, 18, -128, 56, 14, 18, -128, -104, 14, 18, -128, 0, 0, 0, 0, -24, 73, 18, -128, -8, 73, 18, -128, 8, 74, 18, -128, 24, 74, 18, -128, 40, 74, 18, -128};
GpuRectangle ovl662_vram_emblem_image_rect = {.rect = {.x = 640, .w = 16, .h = 64}};
GpuRectangle ovl662_vram_emblem_clut_rect = {.rect = {.x = 640, .y = 64, .w = 16, .h = 1}};
s32 ovl662_arena_init_stage;

/* Writable state of this unit; ovl662_reset_module restores it. */
static const OverlayStateRegion ovl662_editor_lifecycle_regions[] = {
    RR_OVERLAY_STATE(ovl662_arena_init_stage),
    RR_OVERLAY_STATE(ovl662_vram_emblem_clut_rect),
    RR_OVERLAY_STATE(ovl662_vram_emblem_image_rect)};
const OverlayStateTable ovl662_editor_lifecycle_state = {ovl662_editor_lifecycle_regions, sizeof(ovl662_editor_lifecycle_regions) / sizeof(ovl662_editor_lifecycle_regions[0])};

/* Initial values of the globals this module owns. */
u32 EmblemEditFrames;
