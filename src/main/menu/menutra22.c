#include "rr/release.h"
#include "types.h"
#include "rr/frame_storage_types.h"
#include "scratchpad.h"
#include "rr/ui.h"
#include "game/race/ranking.h"
#include "game/menu/menutra22.h"
#include "game/ui/uitext.h"
#include "rr/badge_rows.h"

extern short CardMenuOperation;
extern char StrName[];
extern char TeamLabel[];
extern char StrMaker[];

void UpdateCardSlotUiRows(int a0, int a1);
void CardSlotUiNoop(void);
void DrawCardMenuCommandRows(int a0, int a1);
void MainLoopStartupHook(void);

void UpdateCardSlotUiRows(int a0, int a1) {
    int i = 0;
    UiRow24 *p, *q;
    int v;
    do {
        if (a1 == 1) {
            p = &UiRows[i + 0x26];
            if (p->byte14 == 5) { p->byte14 = 0; goto tail; }
        }
        if (a1 == 0) {
            q = &UiRows[i + 0x26];
            v = q->byte14;
            if (v == 2) {
                q->byte14 = 3;
            } else if (v == 0) {
                return;
            }
        }
tail:
        DrawUiRow(i + 0x26);
        if (i == a0) UiRows[i + 0x26].cells[0] = 2;
        else UiRows[i + 0x26].cells[0] = 0;
        i++;
    } while (i < 2);
}

void CardSlotUiNoop(void) {
}

void DrawCardMenuCommandRows(int a0, int a1) {
 int n, i;
 if (CardMenuOperation == 0) n = 4; else if (CardMenuOperation == 6) n = 3; else n = 2;
 i = 0;
 if (n != 0) {
  do {
   DrawUiRow(i + 0x22);
   if (a1 == 1) {
    if (i == a0) UiRows[i + 0x22].cells[0] = 2;
    else UiRows[i + 0x22].cells[0] = 0;
   } else {
    UiRow24 *p = &UiRows[i + 0x22];
    if ((s8)p->cells[0] == 2) p->cells[0] = 1;
   }
   i++;
  } while (i < n);
 }
}

void DrawSaveSlotTableHeader(int a0)
{
  DrawSmallFontString(0x28, 0xA0, StrName, RR_PAL_JP(0x414, 0x7800));
  DrawSmallFontString(0x68, 0xA0, TeamLabel, RR_PAL_JP(0x414, 0x7800));
  DrawSmallFontString(0xC2, 0xA0, StrMaker, RR_PAL_JP(0x414, 0x7800));
  RR_GPU_SCRATCH->cursor = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[1], RR_GPU_SCRATCH->cursor, 0x20, (a0 << 5) + 0xB2, 0x104, 0xC, 0x80, 0x20, 0x20);
}

void MainLoopStartupHook(void) {

}

/* Initial values of the globals this module owns. */
char StrName[8] = {78, 65, 77, 69};
char TeamLabel[8] = {84, 69, 65, 77};
char StrMaker[8] = {77, 65, 75, 69, 82};
