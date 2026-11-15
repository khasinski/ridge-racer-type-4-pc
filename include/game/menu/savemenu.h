#ifndef RR_GAME_MENU_SAVEMENU_H
#define RR_GAME_MENU_SAVEMENU_H

#include "types.h"

/* The card save step's one-word state: step, main-data flag, retry count. */
typedef struct {
    u8 step;
    u8 isMainData;
    s16 retryCount;
} CardSaveStepState4;
extern CardSaveStepState4 CardSaveStepState;

int CardSaveStep(short initialize, int port);

extern s16 CardTransferMode;

#endif /* RR_GAME_MENU_SAVEMENU_H */
