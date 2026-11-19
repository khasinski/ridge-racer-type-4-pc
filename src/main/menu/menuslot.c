#include "rr/ps1_sizes.h"
#include "game/ui/uitext.h"
#include "game/system/pad.h"
#include "rr/menu_arena_state.h"
#include "game/ui/uipart1.h"
#include "game/sound/sndvab.h"
#include "scratchpad.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"
#include "rr/ui.h"
#include "rr/badge_row_types.h"
#include "game/menu/menu8.h"
#include "game/menu/menuslot.h"
#include "rr/menu_background.h"
#include "rr/menu_text_state.h"
#include "rr/bios_devices.h"
#include "game/data/ldscn1.h"
#include "game/data/screenframe.h"
#include "game/data/menuvars.h"
#include "rr/course_choices.h"
#include "rr/screen_tiles.h"
#include "game/ui/badge1.h"
#include "rr/menu_label_render.h"
#include "rr/trig.h"
#include "game/ui/uidraw.h"

extern short MenuCursor;

extern int DialogueSceneMode;
extern UiUvRect SaveSlotPanelSpriteUv;

void YesNoPromptModeUpdate(void);

void YesNoPromptModeUpdate(void) {
    int r;
    ModeFrame += 1;
    switch (SubmodeState) {
    case 0:
        UiRows[MenuCursor].cells[0] = 0;
        StepMenuValue(0, 2, (char *)&MenuCursor, 2, 0);
        UiRows[MenuCursor].cells[0] = 2;
        if (MenuReturnSubmodeIndex != 0) {
            if (UiRows->byte14 == 2) {
                PlaySfx(1);
                UiRows->byte14 = 3;
                UiRows[1].byte14 = 3;
                BadgeRows->state10 = 4;
                UiRows[MenuCursor].cells[0] = 1;
                FadeCountdown = 0x40;
                LoadCursor = MenuArenas.second;
                ResetScreenCounters();
                if (MenuCursor == 0) {
                    SubmodeState = 3;
                } else if (MenuCursor == 1) {
                    SubmodeState = 4;
                }
            }
        }
        break;
    case 3:
        if (FadeCountdown > 0) FadeCountdown = FadeCountdown - 1;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x40 - FadeCountdown) * 4, 0x45);
        if (FadeCountdown <= 0) {
            r = LoadMenuCarModelsStep();
            if (r == 1) {
                ModeGroup = r;
                ModeIndex = 0x47;
                SubmodeState = 0;
            }
        }
        break;
    case 4:
        if (FadeCountdown > 0) FadeCountdown = FadeCountdown - 1;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x40 - FadeCountdown) * 4, 0x45);
        if (FadeCountdown <= 0) {
            r = LoadTimListStep();
            if (r == 1) {
                ModeGroup = r;
                ModeIndex = 0xF;
                SubmodeState = 0;
            }
        }
        break;
    }
    if (BadgeRows->state10 == 3) {
        Dialogue->b7 = 0;
        Dialogue->b4 = 0x26;
        UpdateDialogue(BadgeRows->x08 + 0xA, BadgeRows->y0A + 0x28, Dialogue);
    }
    UpdateBadgeRowAnim(0);
    DrawUiRow(0);
    DrawUiRow(1);
    UpdateScreenFade(0x10084);
}

void InitDialogueState(MenuTextState *a0) {
    PsyqBzeroPs1(a0, sizeof(*a0), RR_PS1_MENU_TEXT_STATE_BYTES);
    a0->b5 = -1;
    a0->b4 = 0;
    if (DialogueSceneMode == 1) {
        SelectDialogueScript();
    }
}

void DrawDialogueContinueMark(int a0, int a1) {
  int v = rsin((ScreenFrame << 6) & 0xFC0);
  void *new_var;
  if (v < 0)
  {
    v += 0x7F;
  }
  new_var = RR_GPU_SCRATCH->cursor;
  RR_GPU_SCRATCH->cursor = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], new_var, a0, a1, &SaveSlotPanelSpriteUv, 0x1B56, ((v >> 7) + 0x80) * 2), 0x25);
}

/* Initial values of the globals this module owns. */
UiUvRect SaveSlotPanelSpriteUv = {.u = 0x2C, .v = 0x48, .w = 8, .h = 0x10};
