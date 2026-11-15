#ifndef RR_GAME_DATA_SCREENFRAME_H
#define RR_GAME_DATA_SCREENFRAME_H
#include "types.h"

/* Per-screen frame counter: zeroed when a screen initializes, +1 per frame, drives the sine/cosine
 * animations and the results marquee. Kept out of globals.h: the rr/ headers disagree on its
 * signedness (s32 / u32), so only units checked against this spelling include it. */
extern u32 ScreenFrame;

#endif
