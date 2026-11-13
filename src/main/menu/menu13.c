#include "rr/release.h"
/* RODATA 0x800105b0 0x18 */
#include "game/system/pad.h"
#include "types.h"
#include "rr/menu_selection.h"
#include "rr/menu_text_state.h"
#include "rr/badge_row_types.h"
#include "game/gfx/fontdraw.h"
#include "game/menu/menu8.h"
#include "game/menu/menuslot.h"
#include "game/menu/menu13.h"
#include "rr/badge_rows.h"
#include "rr/menu_choice.h"


void UpdateDialogue(int a0, int a1, MenuTextState *s);

void UpdateDialogue(int a0, int a1, MenuTextState *s) {
    unsigned short *p;
    int i;
    int v;
    u8 withheld;
    int w;
    unsigned short *r;

    if (s->b4 != s->b5) {
        s->st = 0;
    }
    s->b5 = s->b4;
    if (s->b4 < 0) {
        return;
    }
    switch (s->st) {
    case 0:
        p = DialogueScriptPtrs[(signed char)(s->b4 % 40)];
        s->p = p;
        s->sC = p[0] & 0xFFF;
        s->p = p + 1;
        s->bA = *((unsigned char *)p + 2);
        s->p = p + 2;
        withheld = *((unsigned char *)p + 4);
        s->st = 5;
        s->b9 = 1;
        p += 3;
        s->p = p;
        s->bB = withheld;
        s->i18 = 0;
        v = DialoguePageCounts[s->b4];
        s->s12 = 0;
        s->sE = 1;
        s->s10 = 0;
        s->b1F = 0;
        s->s14 = 0;
        s->b8 = v;
        /* The Japanese release animates up to 64 glyphs (PAL 200). */
        for (i = RR_PAL_JP(199, 63); i >= 0; i--) {
            s->animation28[i].word = 0;
        }
        break;
    case 1:
        if (s->i18 >= s->sC) {
            /* The Japanese release waits for a page with b7 2. */
            if (RR_RELEASE_JP && s->b7 != 0 && s->b7 != 1) {
                goto call1;
            }
            if (s->b7 == 1) {
                DrawDialogueContinueMark(a0 + RR_PAL_JP(0x10D, 0x10F), a1 + RR_PAL_JP(0x58, 0x32));
                if (MenuReturnSubmodeIndex == 0) {
                    goto call1;
                }
                PlaySfx(3);
            }
            s->st = 3;
            s->s12 = s->bA - s->bB;
        } else {
            s->i18++;
            if (s->sE <= s->s10) {
                if (s->s12 < s->bA - s->bB) {
                    s->s12 = s->s12 + 1;
                } else if (!RR_RELEASE_JP && s->s12 == s->bA - s->bB) {
                    if (s->b7 < 2) {
                        s->i18 = s->sC;
                    }
                }
                s->s10 = 0;
            } else {
                s->s10 = s->s10 + 1;
            }
        }
call1:
        DrawAnimatedText(a0, a1, s, 0, 0x3FFC);
        if ((signed char)s->b1C != (signed char)s->b1D) {
            s->b1F = 0;
            s->b1E = 0;
            s->b1D = s->b1C;
        }
        break;
    case 2:
        DialogueWaitEnterHook();
        DialogueWaitExitHook();
        DrawAnimatedText(a0, a1, s, 1, 0x3FFC);
        break;
    case 3:
        if ((signed char)s->b8 > s->b9) {
            r = (unsigned short *)((char *)s->p + s->bA * 2);
            s->p = r;
            s->sC = r[0] & 0xFFF;
            s->p = r + 1;
            s->bA = *((unsigned char *)r + 2);
            s->p = r + 2;
            w = *((unsigned char *)r + 4);
            r += 3;
            s->p = r;
            s->s12 = 0;
            s->bB = w;
            for (i = RR_PAL_JP(199, 63); i >= 0; i--) {
                s->animation28[i].word = 0;
            }
            s->i18 = 0;
            s->s10 = 0;
            s->b9 = s->b9 + 1;
            s->sE = 1;
            s->st = 1;
            /* The original stores through the global Dialogue even when only
             * another text state is live (the GP intro uses
             * CardDialogueStatePtr and Dialogue is still null); on the PS1
             * that write lands harmlessly at address 7. */
            if (s->b9 == (signed char)s->b8 && Dialogue != 0) {
                Dialogue->b7 = 0;
            }
            break;
        }
    case 4:
        DrawAnimatedText(a0, a1, s, 1, 0x3FFC);
        s->st = 4;
        break;
    case 5:
        if (BadgeRows[s->s16].state10 == 3) {
            s->st = 1;
        }
        break;
    }
}

void SkipDialoguePage(MenuTextState *arg0) {
    if (arg0->i18 < arg0->sC) {
        if (MenuReturnSubmodeIndex != 0) {
            if (arg0->st != 4) {
                PlaySfx(3);
                if (arg0->s12 < (arg0->bA - arg0->bB)) {
                    arg0->s12 = (s16) (arg0->bA - arg0->bB);
                    arg0->i18 = (arg0->sC - 1);
                    return;
                }
                arg0->i18 = (arg0->sC - 1);
            }
        }
    }
}

s32 DecodeCourseIndex(s32 a0) {
    ReverseCourse = a0 >= 8;
    return CoursePreviewSceneIds[a0 & 7];
}

void *EmitSpinningWheelIcon(GpuDmaTag *ordering, void *cursor, s32 x, s32 y, s32 frame) {
    s32 u = (s32)((u32)frame << 5) % 256;
    u16 clut = ((u32)frame << 6) + 0x3828U;
    return AddSpriteWithDrawMode(ordering, cursor, x, y, 0x20, 0x40, u, 0xC0, 0x9A, clut, 0, 0x80);
}

/* Initial values of the globals this module owns. */
u8 CoursePreviewSceneIds[8] = {0, 4, 2, 1, 6, 3, 5, 7};
