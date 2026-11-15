#ifndef RR_PLAYER_STATUS_H
#define RR_PLAYER_STATUS_H
#include "rr/layout.h"
#include "game/race/racegrid.h"

extern s32 RaceTimeRemaining;
extern u8 StrPlayerLabel[12],StrNowAcceptingEntries[28],EntryAcceptedStr[20];
extern u8 StrStarPleaseWait[16],StrNoPlayer[12],SecondsUntilCloseText[28],StrLinkProgressBarGlyph[4];
void DrawBigFontString(s32 x,s32 y,u8 *text);
/* Negative modes draw nothing; active modes0/1 select the two status pairs.
 * The renderer reads a terminated string and may change later global state. */
void DrawLinkEntryScreen(s32 mode);
typedef char rr_player_status_native_address_width[
    sizeof(uintptr_t)>=sizeof(void*)?1:-1];
#endif
