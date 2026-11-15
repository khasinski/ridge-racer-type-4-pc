#ifndef RR_GAME_GFX_SPRPKT_H
#define RR_GAME_GFX_SPRPKT_H

#include "types.h"

/* Intentionally unprototyped: ovl659 calls DrawRaceOverlaySprites() with 0 args
 * (GameModeType != 0 ignores a0), while ovl660 passes (s32 paused). */
void DrawRaceOverlaySprites(s32 a0);

#endif /* RR_GAME_GFX_SPRPKT_H */
