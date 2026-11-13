#include "rr/release.h"
#include "rr/overlay665_menu.h"
#include "game/system/pad.h"
#include "rr/menu_pulse.h"
#include "psyq/libgte/gtemat7.h"
#include "psyq/libgte/gtemat4.h"
#include "rr/overlay665_matrix.h"

void ovl665_init_controller_options(void) {
    ClearScreenOverlayState();
    InitUiRow(0,56,160,64,32,0,0,0,1,22);
    InitUiRow(1,128,160,64,32,0,0,0,0,109);
    InitUiRow(2,200,160,64,32,0,0,0,1,23);
    InitUiRow(3,92,360,64,32,0,0,0,0,110);
    InitUiRow(4,164,360,64,32,0,0,0,0,111);
    InitUiRow(5,128,80,52,32,0,0,1,0,106);
    InitUiRow(6,200,80,28,32,0,0,1,0,107);
    InitUiRow(7,200,120,28,32,0,0,1,0,108);
    ScreenFadeState.phase=2;
    InitBadgeRow(0,16,96,76,40,0,0xD01);
    if(!RR_RELEASE_JP)ovl665_saved_stereo_mode=0;
    BadgeRows[0].half02=1;
    ovl665_options_state.view74.position00.words[2]=0;
    ovl665_options_state.view74.position00.words[1]=0;
    ovl665_options_state.view74.position00.words[0]=0;
    ovl665_options_state.view74.rotation14=0;
    ovl665_options_state.view74.rotation12=0;
    ovl665_options_state.view74.rotation10=0;
    ovl665_options_state.reset2C=0;
    SetMenuFrameOptions(1,1,0);
}

s32 ovl665_detect_controller_kind(s32 index)
{
    ControllerState5C *item=&PlayerControllers[index];
    s32 result=-1;
    switch(item->input00.fields.type){
    case 0x41:case 0x73:
        if(item->kind1C){result=3;if(item->flag40==0)result=1;}
        else result=item->input00.fields.type==0x73;
        break;
    case 0x23:result=2;break;
    case 0xE3:result=3;break;
    }
    return result;
}


void ovl665_setup_preview_light_matrix(const PreviewMatrixWorkspace *arg0, const PreviewLightInput36 *input) {
    PreviewMatrixWorkspace mat[2];
    s32 i;
    u16 *dst1,*dst2;
    const u16 *src2;
    const u8 *arg1=input->bytes;
    i=0;
    dst2=mat[1].halves;
    mat[0]=*arg0;
    dst1=LightMatrix.halves;
    src2=input->halves;
    do {
        dst1[0]=src2[6];
        dst1[1]=src2[7];
        dst1[2]=src2[8];
        i++;
        src2+=4;
        dst2[0]=arg1[0]<<4;
        dst1+=3;
        dst2[3]=arg1[1]<<4;
        dst2[6]=arg1[2]<<4;
        arg1+=4;
        dst2++;
    } while (i<3);
    MulMatrix2(&LightMatrix.matrix, &mat[0].matrix);
    SetLightMatrix(&mat[0].matrix);
}
