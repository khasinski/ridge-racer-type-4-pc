#ifndef RR_GAME_DATA_SAVEDAT_H
#define RR_GAME_DATA_SAVEDAT_H

#include "rr/card_header_types.h"

int ApplySaveData(int mode);

/* The Japanese release's game save has no PAL language word (Options
 * +0x3C): what follows the options lies 4 bytes lower and the checksum sums
 * 0x6A8 words into the word at 0x1AA0. These convert a game save in
 * SaveBuffer between that layout and the PAL one (GameSaveData1AA8); with
 * the PAL release they do nothing. */
void SaveDataToRelease(void);
void SaveDataFromRelease(void);
void BuildCardHeaderStatic(CardFileHeader280 *header, const char *title,
    s32 paletteX, s32 paletteY, s32 imageX, s32 imageY,
    const CardHeaderBlock128 *extra);
void BuildCardHeaderAnim(CardFileHeader280 *header, const char *title,
    s32 paletteX, s32 paletteY, s32 imageX, s32 imageY);


#endif /* RR_GAME_DATA_SAVEDAT_H */
