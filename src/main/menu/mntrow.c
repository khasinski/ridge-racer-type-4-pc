#include "rr/release.h"
#include "game/system/pad.h"
#include "rr/menu_text_state.h"
#include "types.h"
#include "scratchpad.h"
#include "rr/gpu.h"
#include "rr/ui.h"
#include "game/system/modeslot.h"
#include "game/menu/mnsel.h"
#include "game/ui/barsel1.h"
#include "game/gfx/fontdraw.h"
#include "game/menu/mntrow.h"
#include "rr/frame_environment.h"
#include "rr/menu_background.h"
#include "rr/menu_text_rows.h"
#include "rr/menu_state_types.h"
#include "rr/badge_row_types.h"
#include "game/data/selection.h"
#include "rr/frame_storage_types.h"


#include "rr/sprite_emit.h"
#include "rr/gpu_scratch_types.h"
#include "rr/trig.h"
#include "game/ui/uitext.h"
#include "game/ui/uibar.h"
#include "game/ui/uidraw.h"
#include "game/sound/fxplay.h"
extern unsigned short MenuTextRowClutTable[];
extern s32 VSyncThreshold;
extern s16 ModeIndex;
extern s16 ModeGroup;
extern s16 SubmodeState;
extern short FadeCountdown;
extern short ActiveSceneId;
extern short SelectedTeamIndex;
extern u32 ModeFrame;
extern short SelectedMakerIndex;
#include "rr/badge_rows.h"
extern int MenuStepDirection;

void DrawResultsRankIcons(void);
void SetMenuTextRowStyle();
void ModeSelectMenuEnter(void);
void InitModeSelectMenu(void);
void ModeSelectMenuUpdate();

void DrawMenuTextRow(s32 a0) {
    SpritePacket *s4;
    s32 fp;
    s32 sp10;
    s32 sp14;
    s32 sp18;
    u8 *sp1C;
    u16 s5;
    u16 sp20;
    s32 s2;
    s32 s0;
    s32 a3;
    s32 s1;
    s32 d34;
    s32 v1;

    s4 = (SpritePacket *)RR_GPU_SCRATCH->cursor;
    fp = MenuTextRows[a0].rendering.x;
    sp1C = MenuTextRows[a0].rendering.text;
    sp10 = MenuTextRows[a0].rendering.y;
    s5 = MenuTextRows[a0].rendering.clut;
    for (s2 = 0; *sp1C != 0; fp += 0xD, s2++) {
        s0 = *sp1C - 0x20;
        sp1C++;
        if (s0 != 0) {
            s0 = FontAtlasTileCoords[s0 * 2] + (FontAtlasTileCoords[s0 * 2 + 1] << 5);
            a3 = 0;
            sp14 = (s0 % 21) * 12;
            sp18 = (s0 / 21) * 24;
            s1 = MenuTextRows[a0].rendering.revealed[s2];
            d34 = (s16)MenuTextRows[a0].state.d34;
            {
                if (MenuTextRows[a0].rendering.visible >= s2) {
                    if (d34 == 1) {
                        if (s2 & 1) {
                            s5 = 0x2638;
                            sp20 = 0x2639;
                        } else {
                            sp20 = 0x2638;
                            s5 = 0x2639;
                        }
                        if (rsin(s1 << 6) > 0) {
                            a3 = s1 * 4;
                        } else {
                            a3 = 0xFF - (s1 - 0x20) * 4;
                        }
                        if (s1 >= 0x40) {
                            MenuTextRows[a0].rendering.revealed[s2] = 0;
                        }
                        MenuTextRows[a0].rendering.revealed[s2]++;
                    } else if (d34 == 3) {
                        sp20 = 0x2639;
                        s5 = 0x2638;
                        a3 = (rcos(((s1 << 5) - s2 * 0x61) & 0xFFF) * 0x4A) / 4096 + 0x4A;
                        if (s1 >= 0x80) {
                            MenuTextRows[a0].rendering.revealed[s2] = 0;
                        }
                        MenuTextRows[a0].rendering.revealed[s2]++;
                    } else if (d34 == 2) {
                        s5 = 0x2638;
                        a3 = (rcos(((s1 << 5) - s2 * 0x61) & 0xFFF) * 0x4A) / 4096 + 0x4A;
                        if (s1 >= 0x80) {
                            MenuTextRows[a0].rendering.revealed[s2] = 0;
                        }
                        MenuTextRows[a0].rendering.revealed[s2]++;
                    } else if (d34 >= 0) {
                        s0 = d34 >> 8;
                        v1 = rsin(MenuTextRows[a0].rendering.tick << 6) / 256;
                        a3 = v1 + 0x80;
                        if (s0 - 1 == s2) {
                            a3 = v1 + 0xE4;
                            s5 = 0x2638;
                        } else {
                            s5 = MenuTextRows[a0].rendering.clut;
                            if (s1 < 0x15) {
                                a3 = (rsin((s1 * 0x555) / 20) * 0x94) / 4096;
                                MenuTextRows[a0].rendering.revealed[s2]++;
                            }
                        }
                    } else {
                        break;
                    }
                }
            }
            for (s0 = 0; s0 < (MenuTextRows[a0].state.d34 & 1) + 1; s0++) {
                s4->code = 0x64;
                s4->x = fp;
                s4->y = sp10;
                s4->w = 0xC;
                s4->h = 0x18;
                s4->u = sp14;
                s4->v = sp18;
                if (s0 == 1) {
                    s4->clut = sp20;
                    a3 = 0x94 - a3;
                } else {
                    s4->clut = s5;
                }
                s4->r = a3;
                s4->g = a3;
                s4->b = a3;
                s4->code |= 2;
                RR_GPU_ADD_PRIM(&CurrentDrawBuf->buffer.ordering70.tags[0], s4, 4);
                s4++;
            }
        }
    }
    RR_GPU_SCRATCH->cursor = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], (GpuDrawModePacket *)s4, 0x2E);
}

void DrawResultsRankIcons(void)
{
    SpritePacket *sprite;
    int a3;
    FrameStorage *d;
    sprite = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[10], (GpuDrawModePacket *)RR_GPU_SCRATCH->cursor, 5);
    a3 = 0;
    do {
        sprite->code = 0x64;
        sprite->w = 0xC;
        sprite->h = 0x18;
        sprite->u = 0xF4;
        sprite->v = 0x68;
        sprite->clut = 0x2678;
        sprite->code |= 1;
        sprite->x = (a3 % 22) * 13 + 0x11;
        sprite->y = (a3 / 22) * 0x1C + 0x60;
        d = CurrentDrawBuf;
        a3++;
        RR_GPU_ADD_PRIM(&d->buffer.ordering70.tags[10], sprite, 4);
        sprite++;
    } while (a3 < 0xF2);
    RR_GPU_SCRATCH->cursor = AddDrawModePrim(&d->buffer.ordering70.tags[10], (GpuDrawModePacket *)sprite, 0xE);
}


void SetMenuTextRowStyle(int a0, int a1, int a2) {
  MenuTextRows[a0].rendering.clut = MenuTextRowClutTable[a1];
  if (a2 < (-0xFF))
  {
    MenuTextRows[a0].state.d34 = -a2;
    return;
  }
  switch (a2)
  {
    case 0:
      MenuTextRows[a0].rendering.text[0] = 0x20;
      return;

    case 1:
      MenuTextRows[a0].rendering.text[0] = 0x3E;
      return;

    case -1:
      return;

  }

}


void SetGlyphRowImageBase(u8 *imageBase, s32 firstRow) {
    s32 i;
    for (i = 0; i < 4; i++) {
        GlyphRows[firstRow + i].imageBase18 = imageBase + i * 0xC000;
    }
}
void ModeSelectMenuEnter(void) {
    SetupDrawDispEnvs(0, 0, 0, 2);
    VSyncThreshold = 0x80;
    ModeGroup = 1;
    ModeIndex = 8;
    SubmodeState = 0;
}

void InitModeSelectMenu(void) {
    int i;
    SetupDrawDispEnvs(0, 0, 0, 2);
    VSyncThreshold = 0x80;
    for (i = 0; i < 4; i++) {
        InitGlyphRow(i, -2, 0, 0);
    }
    SelectedTeamIndex = 2;
    SelectionRows.bytes[0] = 2;
    SelectedMakerIndex = 0;
    SeqStartTrack(0);
    InitRect16Row(GlyphRows->glyph07, GlyphRows->x08, GlyphRows->y0A, GlyphRows->w0C, GlyphRows->h0E, 0, 0);
    InitBadgeRow(0, 0xC, 0x130, 0x128, 0x58, 0, 0);
    InitUiRow(0, 0xC, 0x48, 0xF0, 0x20, 0, 0, 4, 2, 0);
    BadgeRows->half02 = 1;
    Dialogue->b4 = 1;
    Dialogue->b7 = 0;
    ModeFrame = 0;
    ActiveSceneId = 0x108;
    FadeCountdown = 0;
}

void ModeSelectMenuUpdate(void) {
    int a2;
    ModeFrame += 1;
    if (Dialogue->b4 >= 0x22) {
        DrawUiRow(0);
        UpdateScreenFade(0x10002);
    } else {
        UpdateScreenFade(0x10003);
    }
    if (UpdateModeSelectBarAnim(MenuStepDirection) != 0) {
        if (BadgeRows->state10 == 3) {
            StepModeSelectChoice();
            return;
        }
    }
    if (Dialogue->b4 == 1) {
        if (MenuReturnSubmodeIndex == 0) {
            if (ModeFrame < RR_PAL_JP(0x1F5, 0x12D)) return;
        }
        a2 = Dialogue->st;
        if (a2 == 4) {
            Dialogue->b4 = SelectedTeamIndex + 0x22;
            UiRows->byte16 = SelectedTeamIndex;
            BadgeRows->state10 = a2;
        }
    }
}

/* Initial values of the globals this module owns. */
unsigned short MenuTextRowClutTable[2] = {0x2638, 0x2639};
