#ifndef RR_MENU_TEXT_ROWS_H
#define RR_MENU_TEXT_ROWS_H
#include "rr/layout.h"
#include "rr/work_arena.h"

/* Full text-row storage. The renderer addresses rows with a60-byte stride. */
/* Signed animation bytes and the two marquee spans share the real row owner. */
typedef struct {
 u8 tick, style;
 s8 phase, visible;
 u8 revealed[24];
 u8 text[24];
 u16 flags, clut;
 s16 x, y;
} MenuTextFields60;

typedef union {
 MenuTextFields60 rendering;
 u8 bytes[60];
 s8 signedBytes[60];
 struct {
     u8 prefix[4];
     u8 at4[24];
     u8 at28[32];
 } marquee;
 struct {
     u8 prefix[0x34];
     s16 d34;
     u16 e36;
     u8 suffix[4];
 } state;
} MenuTextRow60;
extern MenuTextRow60 *MenuTextRows;
extern s32 MenuTextRowCount;
/* Publish the full storage pointer, clear60*count bytes and return that captured
 * byte count. Count is in0..35791394 and the supplied owner covers all rows. */
s32 InitMenuTextRows(MenuTextRow60 *rows);
/* BIOS A0/28 bzero. Count bytes belong to the supplied allocation. */
void PsyqBzero(void *destination,s32 bytes);
void SetMenuTextRow(s32 row,s32 position,s32 group,s32 mode,s32 label);
RR_ASSERT_SIZE(MenuTextFields60,60);
RR_ASSERT_OFFSET(MenuTextFields60,text,0x1C);
RR_ASSERT_OFFSET(MenuTextFields60,flags,0x34);
RR_ASSERT_OFFSET(MenuTextFields60,clut,0x36);
RR_ASSERT_OFFSET(MenuTextFields60,x,0x38);
RR_ASSERT_OFFSET(MenuTextFields60,y,0x3A);
RR_ASSERT_SIZE(MenuTextRow60,60);
RR_ASSERT_OFFSET(MenuTextRow60,marquee,0);
RR_ASSERT_OFFSET(MenuTextRow60,state,0);
#endif
