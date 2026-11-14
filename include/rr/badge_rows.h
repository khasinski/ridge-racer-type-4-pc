#ifndef RR_BADGE_ROWS_H
#define RR_BADGE_ROWS_H

#include "rr/ui_bar.h"
#include "rr/release.h"
#include "rr/icon_sprites.h"

#include "rr/badge_row_types.h"

extern BadgeRow1C *BadgeRows;
extern s8 BadgeStripPatternsPal[];
extern UiUvRect BadgeGlyphUvTablePal[];
extern const s8 BadgeStripPatternsJp[152];
extern const UiUvRect BadgeGlyphUvTableJp[37];
/* The Japanese release's badge strips and glyphs. */
#define BadgeStripPatterns RR_PAL_JP(BadgeStripPatternsPal, BadgeStripPatternsJp)
#define BadgeGlyphUvTable RR_PAL_JP(BadgeGlyphUvTablePal, BadgeGlyphUvTableJp)


/* The helpers thread full packet pointers. Coordinate arguments remain wide
 * until their wire16 stores, including sums of two signed16 row fields. */
void *AddTilePrim(GpuDmaTag *, void *, s32, s32, s32, s32, s32, s32, s32);
void *AddTileGrid2x2(GpuDmaTag *, void *, s32, s32, s32, s32, s32, s32, s32);
void *AddSpriteWithDrawMode(GpuDmaTag *, void *, s32, s32, s32, s32, s32, s32,
                    u16, u16, s32, s32);
void *AddDrawAreaPrim(GpuDmaTag *, void *, s32, s32, s32, s32);

/* Emits five row underlines/areas and four textured badges. Style selects an
 * eight-byte pattern group, with nine readable bytes; its first four signed
 * glyph indices must name accessible UV rows. Index selects five consecutive
 * records and the byte offsets must fit signed32. Output helpers determine each
 * next cursor. Captures glyph/shift before the first helper, then reloads the
 * row/context pointers after every helper; UV bytes are read after the first.
 * Each helper input cursor and returned cursor must point into writable arenas. */
void *EmitBadgePatternSprites(void *cursor, s32 style, s32 index);

RR_ASSERT_SIZE(BadgeRow1C, 0x1C);
RR_ASSERT_OFFSET(BadgeRow1C, status00, 0);
RR_ASSERT_OFFSET(BadgeRow1C, half02, 2);
RR_ASSERT_OFFSET(BadgeRow1C, x04, 4);
RR_ASSERT_OFFSET(BadgeRow1C, y06, 6);
RR_ASSERT_OFFSET(BadgeRow1C, width0C, 0xC);
RR_ASSERT_OFFSET(BadgeRow1C, x08, 8);
RR_ASSERT_OFFSET(BadgeRow1C, y0A, 0xA);
RR_ASSERT_OFFSET(BadgeRow1C, extent0E, 0xE);
RR_ASSERT_OFFSET(BadgeRow1C, state10, 0x10);
RR_ASSERT_OFFSET(BadgeRow1C, animationTick12, 0x12);
RR_ASSERT_OFFSET(BadgeRow1C, x14, 0x14);
RR_ASSERT_OFFSET(BadgeRow1C, y16, 0x16);
RR_ASSERT_OFFSET(BadgeRow1C, w18, 0x18);
RR_ASSERT_OFFSET(BadgeRow1C, h1A, 0x1A);

#endif
