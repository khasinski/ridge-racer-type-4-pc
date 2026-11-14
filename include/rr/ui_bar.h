#ifndef RR_UI_BAR_H
#define RR_UI_BAR_H
#include "rr/gpu.h"
#include "rr/release.h"

typedef struct {u8 u,v,w,h;} UiUvRect;
typedef struct {s16 glyph,count;} UiBarDescriptor;
/* UI font glyph UVs. One PS1 table (0x8009FA1C..0x8009FC14) indexed as a
 * whole by glyph id (labels such as "Save" are ids 89..125); the original
 * names of its sub-ranges are views into it. */
extern UiUvRect UiFontGlyphsPal[126];
extern const UiUvRect UiFontGlyphsJp[126];
/* The glyph widths and four labels differ in the Japanese release. */
#define UiFontGlyphs RR_PAL_JP(UiFontGlyphsPal, UiFontGlyphsJp)
#define RecordsHeaderSpriteUvs (UiFontGlyphs + 33)
#define SaveSlotStatusUvs (UiFontGlyphs + 89)
#define MenuStepArrowUv (UiFontGlyphs[116])
#define SaveSlotTrophyUvs (UiFontGlyphs + 124)
extern UiUvRect SelectionBarGlyphUvs[];
extern UiBarDescriptor SelectionBarDescriptors[];

/* Emits a glyph, left cap, count-dependent middle pieces, right cap and white
 * underline through the existing GPU helpers. Style and glyph indices must
 * name accessible records. Count is signed and re-read after each helper.
 * Coordinates use wrapping32 arithmetic; helper stores narrow to wire16.
 * The supplied cursor is threaded through helper return values; the final
 * helper result is returned to the caller. */
SpritePacket *DrawBadgeTitleBar(GpuDmaTag *ordering,SpritePacket *cursor,s32 x,s32 y,s32 style);
RR_ASSERT_SIZE(UiUvRect, 4);
RR_ASSERT_SIZE(UiBarDescriptor, 4);
RR_ASSERT_OFFSET(UiBarDescriptor, count, 2);
#endif
