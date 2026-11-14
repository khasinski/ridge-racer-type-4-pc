#include "rr/release.h"
#include "rr/overlay_module.h"
#include "rr/overlay665_visual.h"
#include "scratchpad.h"
#include "rr/overlay665_menu.h"
#include "rr/load_cursor.h"
#include "game/system/pad.h"
#include "rr/frame_storage_types.h"

#ifndef RR_PROJECTION_SCRATCH
#define RR_PROJECTION_SCRATCH ((ProjectionScratchPrefix *)SCRATCHPAD)
#endif
void ovl665_init_scene(void)
{
    Overlay665VisualState *state;
    ProjectionScratchPrefix *scratch;
    SetDispMask(0);
    SetupDrawDispEnvs(0,0,0,2);
    state=&ovl665_options_state;
    PsyqBzero(state,160);
    state->lighting4C.matrix.signedHalves[6]=4500;
    state->lighting4C.matrix.signedHalves[7]=-4096;
    state->lighting4C.matrix.signedHalves[8]=-8000;
    state->lighting4C.matrix.bytes[0]=192;state->lighting4C.matrix.bytes[1]=192;state->lighting4C.matrix.bytes[2]=192;
    state->lighting4C.matrix.signedHalves[10]=8000;state->lighting4C.matrix.signedHalves[11]=8000;state->lighting4C.matrix.signedHalves[12]=-8000;
    state->lighting4C.matrix.bytes[4]=128;state->lighting4C.matrix.bytes[5]=128;state->lighting4C.matrix.bytes[6]=128;
    state->lighting4C.matrix.signedHalves[14]=-2637;state->lighting4C.matrix.signedHalves[15]=1602;state->lighting4C.matrix.signedHalves[16]=2694;
    state->lighting4C.matrix.bytes[8]=0;state->lighting4C.matrix.bytes[9]=0;state->lighting4C.matrix.bytes[10]=0;
    state->lighting4C.ambient[0]=64;state->lighting4C.ambient[1]=64;state->lighting4C.ambient[2]=64;
    scratch=RR_PROJECTION_SCRATCH;
    scratch->pose08.position00.words[0]=0;scratch->pose08.position00.words[1]=0;scratch->pose08.position00.words[2]=0;
    scratch->pose08.angles10.xyz.x=0;scratch->pose08.angles10.xyz.y=0;scratch->pose08.angles10.xyz.z=0;
    ResetScreenCounters();
    DialogueSceneMode=2;
    ModeGroup=2;
    ModeIndex=2;
    SubmodeState=0;
}

void ovl665_init_menu(void) {
    ClearScreenOverlayState();
    ResetScreenCounters();
    MenuArenas.second=CdDirectoryBuffer+0x2D000;
    InitMenuArenas();
    LoadCursor=MenuArenas.second;
    SetupRenderer(5);
    InitUiRow(0,94,96,112,32,0,0,1,8,19);
    InitUiRow(1,94,136,112,32,0,0,1,9,19);
    /* The Japanese release's third row (screen adjust) is a type 0 row
     * with label 112. */
    InitUiRow(2,94,176,112,32,0,0,1,RR_PAL_JP(1,0),RR_PAL_JP(25,112));
    InitUiRow(3,94,216,112,32,0,0,1,1,20);
    InitUiRow(4,94,256,112,32,0,0,1,1,21);
    InitUiRow(5,94,296,112,32,0,0,1,1,18);
    InitUiRow(6,94,336,112,32,0,0,1,0,85);
    SetMenuFrameOptions(1,1,1);
    if(RaceContext.bgmViewerReturn18==1) {
        ovl665_options_state.selection08=5;
        RaceContext.bgmViewerReturn18=0;
    }
    FadeCountdown=64;
    RaceLoadReset();
    SeqStartTrack(0);
    CardDialogueStatePtr->b4=-1;
}

void ovl665_update_menu(void) {
    s32 i;
    /* Preserve the original signed two-stage range check. */
    s32 choice;
    switch(ovl665_options_state.layout0C) {
    case 0:
        if(FadeCountdown>0) {
            FadeCountdown--;
            AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[702],FadeCountdown*4,69);
        }
        if(UiRows[0].byte14==2 && FadeCountdown==0) {
            UiRows[ovl665_options_state.selection08].cells[0]=0;
            StepMenuValue(0,2,(char*)&ovl665_options_state.selection08,7,0);
            UiRows[ovl665_options_state.selection08].cells[0]=2;
            if(PlayerControllers[0].action28) {
                if(ovl665_options_state.selection08==1 && ovl665_detect_controller_kind(1)<0) {
                    PlaySfx(4);
                } else {
                    PlaySfx(1);
                    UiRows[ovl665_options_state.selection08].cells[0]=1;
                    FadeCountdown=64;
                    ovl665_options_state.layout0C=(ovl665_options_state.layout0C+1U);
                }
            } else if(PlayerControllers[0].action2C) {
                PlaySfx(2);
                ovl665_options_state.selection08=6;
                FadeCountdown=64;
                ovl665_options_state.layout0C=(ovl665_options_state.layout0C+1U);
            }
        }
        SetPresentationDrawEnabled(1);
        break;
    case 1:
        for(i=0;i<8;i++) {
            if(UiRows[i].byte14==2)UiRows[i].byte14=3;
        }
        if(ovl665_options_state.selection08>=5 || ovl665_options_state.selection08==3) {
            SeqFadeVolume(0,(s16)(FadeCountdown-4));
        }
        ovl665_options_state.layout0C=(ovl665_options_state.layout0C+1U);
        break;
    case 2:
        FadeCountdown--;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0],(64-FadeCountdown)*4,69);
        if(FadeCountdown<=0) {
            SetPresentationDrawEnabled(0);
            ovl665_options_state.exit34.word=0;
            ovl665_options_state.layout0C=0;
            ovl665_options_state.detail10=0;
            ovl665_options_state.scene00=(ovl665_options_state.scene00+1U);
            choice=ovl665_options_state.selection08;
            if(choice<2) {
                if(choice>=0)ovl665_init_controller_options();
            }
        }
        break;
    }
    if(ovl665_detect_controller_kind(1)<0) {
        UiRows[1].half20=1;
        if(ovl665_options_state.selection08==1)UiRows[1].cells[0]=4;
    } else {
        UiRows[1].half20=0;
    }
    for(i=0;i<8;i++)DrawUiRow(i);
    UpdateScreenFade(0x10066);
}

/* Module reset: the state a fresh load of the options image gave
 * (initialized data and zeroed BSS); see rr/overlay_module.h. */
extern const OverlayStateTable ovl665_audio_options_state, ovl665_controller_draw_state, ovl665_controller_options_state, ovl665_screen_adjust_state;
static const OverlayStateTable *const ovl665_state_tables[] = {&ovl665_audio_options_state, &ovl665_controller_draw_state, &ovl665_controller_options_state, &ovl665_screen_adjust_state};
static OverlayStateImage ovl665_state_image = {ovl665_state_tables, sizeof(ovl665_state_tables) / sizeof(ovl665_state_tables[0]), NULL};

void ovl665_reset_module(void) {
    OverlayStateRestore(&ovl665_state_image);
}
