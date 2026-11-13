#ifndef RR_GLYPH_LIST_H
#define RR_GLYPH_LIST_H
#include "game/ui/hudmap2.h"
#include "rr/icon_sprites.h"
#include "rr/ui_bar.h"
typedef GpuCommandPacket GlyphCommand8;
typedef struct {SpritePacketSignedClut sprite;GlyphCommand8 mode;} GlyphPacket28;
extern UiUvRect CarHandlingResponseTablePal[50],HudGlyphUvRectsPal[40];
extern const UiUvRect CarHandlingResponseTableJp[50],HudGlyphUvRectsJp[40];
/* Two glyphs of each font differ in the Japanese release, which has no
 * extended glyphs (HudGlyphUvRects 27..39 are PAL-only). */
#define CarHandlingResponseTable RR_PAL_JP(CarHandlingResponseTablePal, CarHandlingResponseTableJp)
#define HudGlyphUvRects RR_PAL_JP(HudGlyphUvRectsPal, HudGlyphUvRectsJp)
/* Readable signed16 x/y, then glyph/gap pairs and a negative terminator.
 * Bank0 selects primary glyph0..49; every nonzero bank selects0..39.
 * Glyph0x400..0x427 always selects the secondary40-entry font. Each glyph
 * needs28 writable aligned bytes in a live packet arena, disjoint from input
 * and font owners. Software cursors retain native width; only DMA wire tags
 * use low24 address bits. X arithmetic wraps32 before wire16 publication. */
#endif
