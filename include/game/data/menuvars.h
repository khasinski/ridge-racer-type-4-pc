#ifndef RR_GAME_DATA_MENUVARS_H
#define RR_GAME_DATA_MENUVARS_H

#include "types.h"

/* Shared dispatcher/menu state. Definitions stay in their original modules.
 * FadeCountdown is signed: negative values are used as sentinels. */

extern u16 ModeGroup;
extern u16 SubmodeState;
extern s16 ModeIndex;
extern s32 ModeFrame;
extern s16 FadeCountdown;
extern u16 ActiveSceneId;
extern u16 MenuScriptId;
extern u16 MenuTransitionOverlayMode;
extern s16 GameModeType;

#endif
