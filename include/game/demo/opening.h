#ifndef RR_GAME_DEMO_OPENING_H
#define RR_GAME_DEMO_OPENING_H

#include "types.h"
#include "rr/release.h"

typedef struct {
    s32 volumes[38];
    s32 timing[64][3];
} CdTrackTableSet;
extern CdTrackTableSet CdTrackTables;
/* PS1 CdTrackTitleStrings (0x800A095C) + track * 32, within CdTrackTables. */
#define CD_TRACK_TITLE(track) ((char *)&CdTrackTables + 22 * 4 + ((track) << 5))
extern s16 *OpeningDateGlyphListsPal[], *OpeningDateGlyphListsJp[];
extern s16 *OpeningTimeGlyphListsPal[], *OpeningTimeGlyphListsJp[];
extern s16 *OpeningStageGlyphListsPal[], *OpeningStageGlyphListsJp[];
/* The race opening's date, time and stage lines (glyph lists, see
 * DrawGlyphList): the Japanese release has its own. */
#define OpeningDateGlyphLists RR_PAL_JP(OpeningDateGlyphListsPal, OpeningDateGlyphListsJp)
#define OpeningTimeGlyphLists RR_PAL_JP(OpeningTimeGlyphListsPal, OpeningTimeGlyphListsJp)
#define OpeningStageGlyphLists RR_PAL_JP(OpeningStageGlyphListsPal, OpeningStageGlyphListsJp)
void StartOpeningFadeOut(void);
void EmitProjectedMarkerSprt(int a0, int a1, int a2);
void EmitOpeningDrawModePrim(void);
s32 UpdateRaceOpeningOverlay(s32 arg0);


#endif /* RR_GAME_DEMO_OPENING_H */
