#ifndef RR_GAME_MENU_MENUMODE_H
#define RR_GAME_MENU_MENUMODE_H

#include "types.h"
#include "rr/layout.h"

typedef struct {
    u16 textId00;
    s8 kind02;
    u8 handlerIdx03;
    u8 tableIdx04;
    u8 flags05;
    u8 promptIdx06;
    s8 defaultChoice07;
} MenuStateEntry8;
RR_ASSERT_SIZE(MenuStateEntry8, 0x8);

extern MenuStateEntry8 MenuStateTable[];
/* RrReleaseSelect: the release's card file names. */
void MenuModeSelectRelease(void);
extern const MenuStateEntry8 MenuStateTableJp[68]; /* the Japanese release's values */
extern unsigned char CardPollTransitionTable[];
extern char *CardTargetFileNames[3];
extern unsigned short SavedMenuStateIndex;
extern short CardMainSaveExists;
extern short CardPdaModeFlag;
void StepCardFileSlotSelection(void);
void StepCardPortSelection(void);
void UpdateCardMenuRowStates(void);


#endif /* RR_GAME_MENU_MENUMODE_H */
