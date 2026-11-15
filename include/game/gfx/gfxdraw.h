#ifndef RR_GAME_GFX_GFXDRAW_H
#define RR_GAME_GFX_GFXDRAW_H

#include "types.h"

void UpdateRaceCamera(int arg0, void *arg1);
#include "rr/matrix_words.h"

/* object: showcase object record (position, angles, light rows, ambient). */
void BuildObjectLightAndRotMatrices(const void *object, MatrixWords32 *rotation);
/* Takes no arguments; some callers still pass two ignored PS1 arguments. */
int RotTransPersOrigin(void);
void InitRaceContextBuffers(void);
u8 *EmitSpotlightConePolys(s32 a0);
void InitTrackStartPos(void);


#endif /* RR_GAME_GFX_GFXDRAW_H */
