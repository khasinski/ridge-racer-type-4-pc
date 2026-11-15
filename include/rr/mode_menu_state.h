#ifndef RR_MODE_MENU_STATE_H
#define RR_MODE_MENU_STATE_H
#include "game/menu/savemenu.h"
#include "game/menu/menutra21.h"
#include "game/menu/menu8.h"
#include "game/data/menuvars.h"
#include "rr/save_slots.h"

#include "rr/menu_setup.h"
#include "rr/frame_environment.h"
#include "rr/driver_control.h"

extern char CardSaveFileName[], CardTargetFileName[24];
extern s32 ActivePlayerIndex, LoadMenuStatusPromptId, DialogueSceneMode, GarageExtraDefaultTimes[3];
extern s16 MenuActiveControllerIndex, MenuStateIndex;
extern s16 LinkState, CardActivePort;
extern s16 CardReplayLoadedFlag, CardPromptIndex;

void CopyStringClearTail(char *destination, char *source);
s32 UpdateLinkHandshake(s32);
void SeqStartTrack(s32 mode);
s32 LoadMenuCarModelsStep(void);
void UpdateScreenFade(s32 layout);

/* The initial choice index is0..2. The name terminates within23 bytes and
 * the text destination has24 bytes. Badge helpers may replace the live root;
 * each replacement covers two complete BadgeRow1C records. Both settings
 * arrays contain three complete records. Only the full word result1 from
 * the readiness service completes state1. */
void CardMenuModeEnter(void);

#endif
