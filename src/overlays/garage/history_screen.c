#include "rr/overlay_module.h"
#include "rr/overlay662_setup.h"
#include "rr/menu_arenas.h"
#include "rr/load_cursor.h"
#include "game/system/pad.h"
#include "rr/overlay662_progress_controller.h"
#include "rr/frame_storage_types.h"
#include "rr/overlay_menu.h"
#include "rr/menu_ui_bank.h"

void ovl662_setup_history_arenas(void);
void UpdateScreenFade(s32 command);

void ovl662_init_history_subpage(s32 arg0) {
    s32 i;
    ovl662_history_arrow_phase = 0;
    SelectedMakerIndex = 0;
    SelectionRows.choice.bytes[1] = 0;
    for (i = 0; i < 4; i++) {
        GlyphRows[arg0 + i].imageBase18 = (GlyphRows[i].imageBase18 + 0x30000);
        InitGlyphRow(arg0 + i, -2, 0, 1);
        InitRect16Row(
            GlyphRows[arg0 + i].glyph07,
            GlyphRows[arg0 + i].x08,
            GlyphRows[arg0 + i].y0A,
            GlyphRows[arg0 + i].w0C,
            GlyphRows[arg0 + i].h0E,
            0, 0
        );
    }
    InitBadgeRow(0, 108, 336, 112, 34, 0, 0x401);
    InitUiRow(0, 40, 72, 192, 32, 0, 0, 4, 3, SelectedMakerIndex + 4);
    BadgeRows[0].half02 = 1;
}

void ovl662_init_history_overview(void)
{
    s32 i;
    ModeFrame=0;
    ScreenFrame=0;
    ovl662_menu_substate=0;
    ovl662_history_arrow_phase=0;
    SelectedTeamIndex=2;
    SelectionRows.choice.bytes[0]=2;
    SelectedMakerIndex=0;
    for(i=0;i<4;i++)InitGlyphRow(i,-2,0,1);
    InitRect16Row(GlyphRows->glyph07,GlyphRows->x08,GlyphRows->y0A,GlyphRows->w0C,GlyphRows->h0E,0,0);
    InitUiRow(0,12,72,240,32,0,0,4,2,SelectedTeamIndex);
    InitBadgeRow(0,36,336,248,34,0,0x301);
    BadgeRows->half02=1;
}

void ovl662_setup_history_arenas(void){
 ResetScreenCounters();
 MenuArenas.second=CdDirectoryBuffer+0x2D000;
 InitMenuArenas();
 LoadCursor=MenuArenas.second;
 SetMenuFrameOptions(0,1,1);
 ovl662_init_history_overview();
}

void ovl662_enter_history_screen(void)
{
    ovl662_setup_history_arenas();
    ModeGroup=1;
    ModeIndex=66;
    SubmodeState=0;
    UpdateScreenFade(0x10002);
}

void ovl662_update_history_screen(void)
{
    s32 ready, changed, state, i, flags;

    ModeFrame = (ModeFrame + 1U);
    ScreenFrame = (ScreenFrame + 1U);
    switch ((u16)SubmodeState) {
    case 0:
        ready = LoadGlyphRowsStep();
        if (ready == 1) {
            SetMenuFrameOptions(1, 1, 1);
            SubmodeState = ready;
        }
        flags = 0x10002;
        break;
    case 1:
        changed = ovl662_draw_history_arrows(ovl662_history_arrow_phase, 0);
        state = BadgeRows->state10;
        if (state == 3) {
            if (changed) {
                ovl662_step_selected_team();
            }
            if (PlayerControllers[0].action28) {
                if ((s16)ovl662_team_unlocked_counts[SelectionRows.choice.bytes[0]] > 0) {
                    PlaySfx(1);
                    ModeFrame = 0;
                    SubmodeState = 2;
                    UiRows->byte14 = state;
                    BadgeRows->state10 = 4;
                } else {
                    PlaySfx(4);
                }
            } else if (PlayerControllers[0].action2C) {
                PlaySfx(2);
                SubmodeState = 8;
                FadeCountdown = 64;
                UiRows->byte14 = state;
                BadgeRows->state10 = 4;
            }
        }
        flags = 0x10002;
        break;
    case 2:
        if (BadgeRows->state10 == 11 && UiRows->byte14 == 5) {
            ovl662_init_history_subpage(0);
            SubmodeState = 3;
        }
        flags = 0x10006;
        break;
    case 3:
        changed = ovl662_draw_history_arrows(ovl662_history_arrow_phase, 1);
        state = BadgeRows->state10;
        if (state == 3) {
            if (changed) {
                ovl662_step_selected_maker();
            }
            if (PlayerControllers[0].action28) {
                if ((s16)ovl662_team_maker_car_counts[SelectionRows.choice.bytes[0]][SelectionRows.choice.bytes[1]] > 0) {
                    PlaySfx(1);
                    ModeFrame = 0;
                    SubmodeState = 4;
                    UiRows->byte14 = state;
                    BadgeRows->state10 = 4;
                } else {
                    PlaySfx(4);
                }
            } else if (PlayerControllers[0].action2C) {
                PlaySfx(2);
                ovl662_menu_substate = 1;
                SubmodeState = 4;
                UiRows->byte14 = state;
                BadgeRows->state10 = 4;
            }
        }
        flags = 0x10006;
        break;
    case 4:
        if (BadgeRows->state10 == 11 && UiRows->byte14 == 5) {
            i = 0;
            if (ovl662_menu_substate == 0) {
                ovl662_init_available_cars();
                return;
            }
            for (; i < 4; i++) {
                GlyphRows[i].imageBase18 = (GlyphRows[i].imageBase18 - 0x30000);
            }
            ovl662_init_history_overview();
            SubmodeState = 1;
            ovl662_menu_substate = 0;
        }
        flags = 0x10006;
        break;
    case 8:
        FadeCountdown--;
        SetMenuFrameOptions(0, 1, 1);
        AddScreenFadeTiles(CurrentDrawBuf->buffer.ordering70.tags,
                     (64 - FadeCountdown) * 4, 69);
        if (FadeCountdown <= 0) {
            ovl662_init_team_car_layout();
            ModeGroup = 1;
            ModeIndex = 64;
            SubmodeState = 0;
        }
        flags = 0x10002;
        break;
    }
    DrawUiRow(0);
    UpdateBadgeRowAnim(0);
    UpdateScreenFade(flags);
}

s32 ovl662_draw_history_arrows(u32 unused,s32 mode){
 s32 selected;
 if(mode)selected=SelectionRows.choice.bytes[1];else selected=SelectionRows.choice.bytes[0];
 DrawMenuStepArrows(160,204,48,0,selected,0,4);
 return UpdateGlyphRow(selected);
}

void ovl662_step_selected_team(void)
{
    SelectedTeamIndex=SelectionRows.choice.bytes[0];
    {
        UiRow24 *ui=UiRows;
        s32 selected=SelectedTeamIndex;
        ((MenuUiBank *)ui)->cell_view.cells0C[selected]=0;
    }
    if ((MenuSelectedCarSlot & 0x2000) && SelectedTeamIndex < 3) {
        SelectedTeamIndex++;
        MenuStepDirection = 2;
    } else if ((MenuSelectedCarSlot & 0x8000) && SelectedTeamIndex > 0) {
        SelectedTeamIndex--;
        MenuStepDirection = 1;
    } else {
        MenuStepDirection = 0;
        goto skip_step;
    }
    PlaySfx(0);
    if (SelectedTeamIndex<0)SelectedTeamIndex=0;
    else if (SelectedTeamIndex>=4)SelectedTeamIndex=3;
    SelectionRows.choice.bytes[0]=(u8)SelectedTeamIndex;
    GlyphRows[SelectedTeamIndex].control02=-6;
    GlyphRows[SelectedTeamIndex].frameIndex00=0;
skip_step:;
    UiRows->byte16=(u8)SelectedTeamIndex;
    {
        UiRow24 *ui=UiRows;
        s32 selected=SelectedTeamIndex;
        ((MenuUiBank *)ui)->cell_view.cells0C[selected]=2;
    }
}

void ovl662_step_selected_maker(void)
{
    SelectedMakerIndex=SelectionRows.choice.bytes[1];
    {
        UiRow24 *ui=UiRows;
        s32 selected=SelectedMakerIndex;
        ((MenuUiBank *)ui)->cell_view.cells0C[selected]=0;
    }
    if ((MenuSelectedCarSlot & 0x2000) && SelectedMakerIndex < 3) {
        SelectedMakerIndex++;
        MenuStepDirection = 2;
    } else if ((MenuSelectedCarSlot & 0x8000) && SelectedMakerIndex > 0) {
        SelectedMakerIndex--;
        MenuStepDirection = 1;
    } else {
        MenuStepDirection = 0;
        goto skip_step;
    }
    PlaySfx(0);
    if (SelectedMakerIndex<0)SelectedMakerIndex=0;
    else if (SelectedMakerIndex>=4)SelectedMakerIndex=3;
    SelectionRows.choice.bytes[1]=(u8)SelectedMakerIndex;
    GlyphRows[SelectedMakerIndex].control02=-6;
skip_step:;
    UiRows->byte16=(u8)SelectedMakerIndex+4;
    {
        UiRow24 *ui=UiRows;
        s32 selected=SelectedMakerIndex;
        ((MenuUiBank *)ui)->cell_view.cells0C[selected]=2;
    }
}

/* Initial values of the overlay globals this module owns. */
s32 ovl662_history_arrow_phase;

/* Writable state of this unit; ovl662_reset_module restores it. */
static const OverlayStateRegion ovl662_history_screen_regions[] = {
    RR_OVERLAY_STATE(ovl662_history_arrow_phase)};
const OverlayStateTable ovl662_history_screen_state = {ovl662_history_screen_regions, sizeof(ovl662_history_screen_regions) / sizeof(ovl662_history_screen_regions[0])};

/* Initial values of the globals this module owns. */
s32 MenuStepDirection;
