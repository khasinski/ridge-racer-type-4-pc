#ifndef RR_MENU_SETUP_H
#define RR_MENU_SETUP_H
#include "game/data/menuvars.h"
#include "rr/menu_choice.h"

extern u32 ScreenFrame;
extern s16 SelectedTeamIndex;
/* Clear the resident32-byte and12-byte UI work buffers. */
void ClearScreenOverlayState(void);
void InitGlyphRow(int index, int control, int glyph, int presetIndex);
void InitRect16Row(s32 index,s16 a,s16 b,s16 c,s16 d,s16 e,s16 f);
void InitBadgeRow(s32 index,s32 first,s16 x,s32 y,u16 width,u16 mode,u16 flags);
#endif
