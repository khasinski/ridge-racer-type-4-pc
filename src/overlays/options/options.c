#include "game/data/ldscn1.h"
#include "rr/release.h"
#include "rr/overlay665_dispatch.h"

void ovl665_update_options(void) {
 switch(ovl665_options_state.scene00) {
 case 0:
  SetDispMask(0);
  /* The Japanese release loads the menu dialogue image first. */
  if(RR_RELEASE_JP && LoadMenuCarModelsStep()!=1)break;
  ovl665_init_menu();
  ovl665_options_state.scene00=(ovl665_options_state.scene00+1U);
  InitMotionController();
  SetPresentationDrawEnabled(1);
  SeqStartTrack(0);
  break;
 case 1:ovl665_update_menu();break;
 case 2:
  switch(ovl665_options_state.selection08) {
  case 0:ovl665_update_controller_options(0);break;
  case 1:ovl665_update_controller_options(1);break;
  case 2:ovl665_update_audio_options();break;
  case 3:ovl665_update_screen_adjust();break;
  case 4:ovl665_update_brightness_panel();break;
  case 5:StartAttractDemo(1);break;
  case 6:ModeGroup=1;
  ModeIndex=3;
  SubmodeState=0;
  break;
  }
  break;
 }
 StepCameraScript();
}
