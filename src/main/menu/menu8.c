#include "game/ui/uipart1.h"
#include "game/menu/menutra21.h"
#include "game/data/menuvars.h"
#include "rr/menu_arena_state.h"
#include "rr/menu_choice_types.h"
#include "rr/vehicle_table.h"
#include "rr/menu_frame_state.h"
#include "game/sound/sndvab.h"
#include "rr/save_slots.h"
/* RODATA 0x80010550 0x60 */
#include "types.h"
#include "rr/frame_storage_types.h"
#include "rr/load_cursor.h"
#include "scratchpad.h"
#include "rr/menu_state_types.h"
#include "rr/selection_types.h"
#include "game/menu/mnrows.h"
#include "game/data/selection.h"
#include "rr/ui.h"
#include "rr/badge_row_types.h"
#include "rr/menu_text_state.h"
#include "game/data/ldscn1.h"
#include "game/gfx/gfxotag.h"
#include "game/menu/menutran.h"
#include "game/menu/menu8.h"
#include "game/system/pad.h"
#include "game/ui/barsel0.h"
#include "game/data/globals.h"
#include "rr/menu_background.h"
#include "game/ui/badge1.h"
#include "rr/badge_rows.h"
#include "game/course/trkintp2.h"
#include "rr/screen_tiles.h"
#include "game/gfx/sprefct.h"
#include "game/ui/uidraw.h"
#include "game/ui/uibar.h"

#include "rr/sprite_effect.h"
extern SpriteEffectSource MenuJitterSpritePresetA;
extern SpriteEffectSource MenuJitterSpritePresetB;

extern int MenuAutoAdvanceTimer;
extern s16 MenuCursor;
extern s16 CardActivePort;

void SelectDialogueScript(void);
void GpSaveSlotMenuEnter(void);
void GpSaveSlotMenuUpdate(void);
void StepReplaySavePrompt(void);
void YesNoPromptModeEnter(void);

void SelectDialogueScript(void) {
    int sw;
    int idx, a1, a3, t;
    const MenuSettings2C *settings;
    MenuTextState *P;
    int u;
    int i;

    sw = SelectionState.fields.mode1A;
    Dialogue->b7 = 1;
    Dialogue->s16 = 0;
    Dialogue->b1E = -2;
    switch (sw) {
    case 0:
        Dialogue->b7 = 0;
    case 1:
        Dialogue->b4 = 0;
        PendingModeId = 0x109;
        SelectionState.fields.mode1A = 1;
        break;
    case 2:
        Dialogue->b4 = SelectionState.fields.copies[0].bytes[1] + 0x18;
        PendingModeId = 0x10F;
        break;
    case 4:
        idx = SelectionState.fields.phase17;
        if (idx < 4) {
            a3 = SelectionState.fields.results00[idx];
        } else {
            a3 = 3 - SelectionState.fields.retries08[idx];
            if (idx == 7) a3 = 0;
        }
        switch (idx) {
        case 0:
            a1 = SelectionState.fields.retries08[idx] < 3;
            break;
        case 1:
        case 3:
            a1 = 4;
            if (SelectionState.fields.retries08[idx] >= 3) a1 = SelectionState.fields.results00[idx - 1];
            break;
        case 2:
            a1 = 4;
            if (SelectionState.fields.retries08[idx] >= 3) a1 = SelectionState.fields.reserved10[4];
            break;
        case 4:
            a1 = SelectionState.fields.reserved10[4];
            break;
        case 5:
            a1 = 3 - SelectionState.fields.retries08[idx - 1];
            break;
        case 6:
        case 7:
            a1 = 0;
            i = 0;
            settings = &SelectionState.fields;
            do {
                a1 += settings->retries08[i + 4];
                i++;
            } while (i < 4);
            if (a1 == 0xC) {
                a1 = 0;
            } else if (idx == 7) {
                a1 = 1;
            } else {
                a1 = 4 - SelectionState.fields.retries08[idx - 1];
            }
            break;
        }
        P = Dialogue;
        u = a3 * 5;
        t = a1 + 4;
        P->b4 = u + t;
        switch (SelectionState.fields.phase17) {
        case 1:
        case 3:
        case 6:
            SelectionState.fields.mode1A = 6;
            UpdateSelectionRank();
            PendingModeId = 0x12A;
            break;
        default:
            PendingModeId = 0x10F;
            break;
        }
        if (SelectionState.fields.phase17 == 7) PendingModeId = 0x205;
        break;
    case 6:
        Dialogue->b4 = 0x25 - SelectionState.fields.reserved10[4];
        PendingModeId = 0x111;
        CardMenuOperation = 3;
        break;
    }
}

void UpdateDialoguePanel(void) {
    void *var_a1;
    s32 temp_v0;
    s32 temp_s2;

    temp_v0 = Dialogue->b1E;
    temp_s2 = Dialogue->b1F;
    switch (temp_v0) {
    case -2:
        Dialogue->b1F = 0;
        Dialogue->b1E = -4;
        Dialogue->b1D = -1;
        Dialogue->b1C = -1;
        break;
    case -4:
        InitRect16Row(0, 0x78, 0x60, 0x50, 0xB4, 0, 0);
        Dialogue->b1E = -5;
        break;
    case -5:
        if (UpdateRect16Row(0) != 0) {
            Dialogue->b1E = -1;
        }
        break;
    case -3:
        break;
    case -1:
        {
            void *a1 = RR_GPU_SCRATCH->cursor;
            RR_GPU_SCRATCH->cursor = AddDrawModePrim(
                &CurrentDrawBuf->buffer.ordering70.tags[1],
                AddTileSemiTransPrim(&CurrentDrawBuf->buffer.ordering70.tags[1], a1, 0x78, 0x60, 0x50, 0xB4, (u16)temp_s2, (u16)temp_s2, (u16)temp_s2),
                0x45);
            if ((u16)temp_s2 >= 0x7C) {
                Dialogue->b1E = 2;
                Dialogue->b1F = 0;
                BadgeRows[Dialogue->s16].half02 = 1;
            } else {
                Dialogue->b1F += 8;
            }
            UpdateRect16Row(0);
        }
        break;
    case 0:
        var_a1 = RR_GPU_SCRATCH->cursor;
        if ((u16)temp_s2 >= 3) {
            var_a1 = AddJitterSprites(&CurrentDrawBuf->buffer.ordering70.tags[0], var_a1, &MenuJitterSpritePresetA, 1, 0xFF, 2);
        }
        RR_GPU_SCRATCH->cursor = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[1], var_a1, 0x78, 0x60, 0x50, 0xB4, 8, 8, 8);
        if ((u16)temp_s2 >= 0x15) {
            Dialogue->b1E = 1;
            Dialogue->b1F = 0;
        } else {
            Dialogue->b1F += 1;
        }
        if (temp_s2 == 1) {
            u8 temp_a0 = Dialogue->b1C;
            if ((temp_a0 - 1) < 5U) {
                LoadTimImage((int *)(Dialogue->images20 + (((s8)temp_a0 - 1) << 14)));
            }
        }
        break;
    case 1:
        {
            s32 two = 2;
            RR_GPU_SCRATCH->cursor = AddTilePrim(
                &CurrentDrawBuf->buffer.ordering70.tags[3],
                AddJitterSprites(
                    &CurrentDrawBuf->buffer.ordering70.tags[0],
                    AddJitterSprites(&CurrentDrawBuf->buffer.ordering70.tags[0], RR_GPU_SCRATCH->cursor, &MenuJitterSpritePresetB, 1, temp_s2, 1),
                    &MenuJitterSpritePresetA, 1, (0x80 - temp_s2) * 2, two),
                0x78, 0x60, 0x50, 0xB4, 8, 8, 8);
            if (temp_s2 >= 0x80U) {
                Dialogue->b1F = 0;
                Dialogue->b1E = two;
            } else {
                Dialogue->b1F += 8;
            }
        }
        break;
    case 2:
    default:
        var_a1 = RR_GPU_SCRATCH->cursor;
        if (Dialogue->b1C >= 0) {
            var_a1 = AddJitterSprites(&CurrentDrawBuf->buffer.ordering70.tags[0], var_a1, &MenuJitterSpritePresetB, 0, 0x80, 1);
        }
        RR_GPU_SCRATCH->cursor = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[1], var_a1, 0x78, 0x60, 0x50, 0xB4, 8, 8, 8);
        break;
    }
}

void UpdateDialogueBox(void) {
    BadgeRow1C *temp_v0;

    if (BadgeRows[Dialogue->s16].state10 == 3) {
        SetMenuFrameOptions(1, 1, 0);
        SkipDialoguePage(Dialogue);
        temp_v0 = &BadgeRows[Dialogue->s16];
        UpdateDialogue(temp_v0->x08 + 0xA, temp_v0->y0A + 0x28, (MenuTextState *)Dialogue);
    }
    UpdateBadgeRowAnim(Dialogue->s16);
}

void DialogueWaitEnterHook(void) {
}

void DialogueWaitExitHook(void) {
}

void ResetRankEntries(void) {
    int i = 0;
    RankEntry8 **q = RankTable;
    RankEntry8 *p = RankEntries;
    do {
        p->score00 = 0;
        p->carIndex04 = i;
        *q++ = p;
        i++;
        p++;
    } while (i < 8);
}

void ResetRankEntriesForCars(void) {
    int i = 0;
    RankEntry8 *e;
    RankEntry8 **t;
    int n;
    if (NumCars > 0) {
        n = NumCars;
        t = RankTable;
        e = RankEntries;
        do {
            e->score00 = 0;
            e->carIndex04 = i;
            *t = e;
            t++;
            i++;
            e++;
        } while (i < n);
    }
}

/* Default variant per selection, and the seven row selections for each
 * primary selection (main-executable data tables). */
extern const u8 D_8009F420[4];
extern const u8 D_8009F424[][8];

void InitSelectionRows(void) {
    SelectionRows32 *b = &SelectionRows.selection;
    const MenuSettings2C *config = &SelectionState.fields;
    SelectionRow *row;
    s32 a2;
    u8 t;

    for (a2 = 0; a2 < 7; a2++) {
        row = &b->rows[a2];
        t = D_8009F424[b->primary][a2];
        row->selection = t;
        if (t == b->primary) {
            row->variant = b->variant;
        } else {
            row->variant = D_8009F420[row->selection];
        }
        if (row->selection != b->primary) {
            if (row->variant == b->variant) {
                row->variant = D_8009F420[b->primary];
            }
        }
        row->parameter = config->copies[0].bytes[2];
    }
}

void GpSaveSlotMenuEnter(void) {
    ClearScreenOverlayState();
    ScreenFadeState.phase = 2;
    GpSaveSlotCursor = 0;
    InitBadgeRow(1, 0x80, 0x58, 0xA8, 0x80, 0, 0x701);
    BadgeRows[1].half02 = 1;
    InitBadgeRow(0, 0xC, 0x130, 0x128, 0x58, 0, 0);
    BadgeRows->half02 = 1;
    SetMenuFrameOptions(0, 1, 1);
    ModeGroup = 1;
    ModeIndex = 0xE;
    SubmodeState = 0;
}

void GpSaveSlotMenuUpdate(void) {
    int s0;
    switch (SubmodeState) {
    case 0:
        UiRows[GpSaveSlotCursor].cells[0] = 0;
        StepMenuValue(0, 2, (char *)&GpSaveSlotCursor, 3, 0);
        UiRows[GpSaveSlotCursor].cells[0] = 2;
        if (BadgeRows->state10 == 3) {
            s0 = CardDialogueStatePtr->st;
            if (s0 == 4) {
                SetMenuFrameOptions(1, 1, 1);
                if (PlayerControllers[0].action28 != 0) {
                    if (SaveSlotSelections[GpSaveSlotCursor].fields.phase17 > 0) {
                        PlaySfx(1);
                        SubmodeState = 1;
                        FadeCountdown = 0x40;
                        ResetScreenCounters();
                        LoadCursor = MenuArenas.second;
                        BadgeRows->state10 = s0;
                        BadgeRows[1].state10 = s0;
                    } else {
                        PlaySfx(4);
                    }
                } else if (PlayerControllers[0].action2C != 0) {
                    PlaySfx(2);
                    SubmodeState = 2;
                    FadeCountdown = 0x40;
                    ResetScreenCounters();
                    LoadCursor = MenuArenas.second;
                    LoadTimListStep();
                    BadgeRows->state10 = s0;
                    BadgeRows[1].state10 = s0;
                }
            }
        }
        break;
    case 1:
        SetMenuFrameOptions(0, 1, 1);
        if (FadeCountdown > 0) FadeCountdown = FadeCountdown - 1;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x40 - FadeCountdown) * 4, 0x45);
        if (FadeCountdown <= 0) {
            if (LoadTimListStep() == 1) {
                SelectionState.fields = SaveSlotSelections[GpSaveSlotCursor].fields;
                ModeGroup = 1;
                ModeIndex = 0xF;
                SubmodeState = 0;
                return;
            }
        }
        break;
    case 2:
        SetMenuFrameOptions(0, 1, 1);
        if (FadeCountdown > 0) FadeCountdown = FadeCountdown - 1;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x40 - FadeCountdown) * 4, 0x45);
        if (FadeCountdown <= 0) {
            UpdateGrandPrixStageProgress();
            MenuScriptId = 1;
            ModeGroup = 1;
            ModeIndex = 0x23;
            SubmodeState = 0;
            return;
        }
    default:
        break;
    }
    UpdateBadgeRowAnim(0);
    UpdateBadgeRowAnim(1);
    if (BadgeRows->state10 == 3) {
        CardDialogueStatePtr->b4 = 0x1C;
        UpdateDialogue(BadgeRows->x08 + 0xA, BadgeRows->y0A + 0x28, CardDialogueStatePtr);
    }
    UpdateScreenFade(0x10086);
}

void StepReplaySavePrompt(void) {
    if (MenuSelectedCarSlot & 0x1000) {
        MenuAutoAdvanceTimer = MenuAutoAdvanceTimer - 1;
    } else if (MenuSelectedCarSlot & 0x4000) {
        MenuAutoAdvanceTimer = MenuAutoAdvanceTimer + 1;
    }
    if (MenuAutoAdvanceTimer < 0) {
        MenuAutoAdvanceTimer = 1;
    } else if (MenuAutoAdvanceTimer >= 2) {
        MenuAutoAdvanceTimer = 0;
    }
    if (Pad0Buttons & 0x800) {
        ModeGroup = 1;
        ModeIndex = 0xF;
        SubmodeState = 0;
    }
}

void YesNoPromptModeEnter(void) {
    ModeFrame = 0;
    CardActivePort = 0;
    CardMenuOperation = 3;
    MenuCursor = 0;
    InitUiRow(0, 0x8C, 0x88, 0x1C, 0x20, 0, 0, 1, 0, 0x68);
    InitUiRow(1, 0x8C, 0xB0, 0x1C, 0x20, 0, 0, 1, 0, 0x69);
    InitBadgeRow(0, 0xC, 0x130, 0x128, 0x58, 0, 0);
    ModeGroup = 1;
    ModeIndex = 0x2B;
    SubmodeState = 0;
    BadgeRows->half02 = 1;
    UpdateScreenFade(0x10086);
}

/* Initial values of the globals this module owns. */
SpriteEffectSource MenuJitterSpritePresetA = {.x = 120, .y = 178, .page = 0xAD, .clut = 0x3A68, .uv = {.u = 0x30, .v = 0xB4, .w = 0x50, .h = 0x10}};
SpriteEffectSource MenuJitterSpritePresetB = {.x = 120, .y = 96, .page = 0xAD, .clut = 0x3A68, .uv = {.u = 0x30, .w = 0x50, .h = 0xB4}};
const u8 D_8009F420[4] = {1, 3, 2};
const u8 D_8009F424[68][8] = {{3, 2, 1, 0, 3, 2, 1}, {3, 2, 0, 1, 3, 2, 0, 1}, {3, 1, 0, 2, 3, 1, 0, 2}, {2, 1, 0, 3, 2, 1, 0, 3}, {0x81, 0x42, 0x81, 0x49, 0x82, 0x50, 0x82, 0x51}, {0x82, 0x52, 0x82, 0x53, 0x82, 0x54, 0x82, 0xA2}, {0x82, 0xA4, 0x82, 0xA5, 0x82, 0xA6, 0x82, 0xA9}, {0x82, 0xAB, 0x82, 0xAD, 0x82, 0xAF, 0x82, 0xB1}, {0x82, 0xB5, 0x82, 0xB7, 0x82, 0xC5, 0x82, 0xCD}, {0x82, 0xD0, 0x82, 0xD3, 0x82, 0xD6, 0x82, 0xD9}, {0x82, 0xE6, 0x82, 0xEB, 0x89, 0xEB, 0x8C, 0xC3}, {0x8F, 0x72, 0x97, 0xD1, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x69, 0x83, 0x81, 0x83, 0x82, 0x83, 0x8A}, {0x81, 0x5B, 0x83, 0x4A, 0x81, 0x5B, 0x83, 0x68}, {0x82, 0xCD, 0x94, 0xB2, 0x82, 0xA9, 0x82, 0xC8}, {0x82, 0xA2, 0x82, 0xC5, 0x81, 0x49, 0x81, 0x6A}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x82, 0x78, 0x82, 0x64}, {0x82, 0x72, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x82, 0x6D, 0x82, 0x6E}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x83, 0x66, 0x81, 0x5B}, {0x83, 0x5E, 0x82, 0xF0, 0x83, 0x8D, 0x81, 0x5B}, {0x83, 0x68, 0x82, 0xB5, 0x82, 0xDC, 0x82, 0xB7}, {0x82, 0xA9, 0x81, 0x48, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x83, 0x6F, 0x83, 0x62}, {0x83, 0x4E, 0x83, 0x41, 0x83, 0x62, 0x83, 0x76}, {0x82, 0xCD, 0x96, 0x59, 0x82, 0xEA, 0x82, 0xB8}, {0x82, 0xC9, 0x81, 0x49, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x83, 0x66, 0x81, 0x5B}, {0x83, 0x5E, 0x8F, 0xC1, 0x82, 0xA6, 0x82, 0xC4}, {0x82, 0xE0, 0x82, 0xB5, 0x82, 0xE7, 0x82, 0xF1}, {0x82, 0xC5, 0x81, 0x49, 0x81, 0x40, 0x81, 0x40}, {0x82, 0x78, 0x82, 0x64, 0x82, 0x72, 0x81, 0x46}, {0x81, 0x9B, 0x83, 0x7B, 0x83, 0x5E, 0x83, 0x93}, {0x81, 0x40, 0x82, 0x6D, 0x82, 0x6E, 0x81, 0x46}, {0x81, 0x7E, 0x83, 0x7B, 0x83, 0x5E, 0x83, 0x93}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x9D}, {0x83, 0x6E, 0x83, 0x93, 0x83, 0x68, 0x83, 0x8B}, {0x82, 0xCC, 0x92, 0xB2, 0x90, 0xAE, 0x81, 0x9D}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x9E, 0x8A, 0x65, 0x8E, 0xED}, {0x83, 0x52, 0x81, 0x5B, 0x83, 0x58, 0x83, 0x43}, {0x83, 0x78, 0x83, 0x93, 0x83, 0x67, 0x82, 0xCC}, {0x90, 0xDD, 0x92, 0xE8, 0x81, 0x9E, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x9C, 0x90, 0x81, 0x82, 0xAF}, {0x82, 0xA0, 0x82, 0xAA, 0x82, 0xE8, 0x82, 0xC6}, {0x89, 0xF1, 0x93, 0x5D, 0x90, 0x94, 0x82, 0xCC}, {0x8A, 0xD6, 0x8C, 0x57, 0x81, 0x9C, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x9A, 0x82, 0xB7}, {0x82, 0xD7, 0x82, 0xC4, 0x82, 0xCC, 0x83, 0x4D}, {0x83, 0x84, 0x82, 0xCC, 0x93, 0xC1, 0x90, 0xAB}, {0x81, 0x48, 0x81, 0x9A, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0xA1, 0x89, 0xF1}, {0x93, 0x5D, 0x90, 0x94, 0x82, 0xC6, 0x95, 0x89}, {0x89, 0xD7, 0x82, 0xCC, 0x8A, 0xD6, 0x8C, 0x57}, {0x81, 0x48, 0x81, 0xA1, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}, {0x81, 0x40, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40}};
SavedMenuSettings SelectionState;
s16 GpSaveSlotCursor;
MenuTextState *Dialogue;
int MenuAutoAdvanceTimer;
