#include "rr/overlay_module.h"
#include "rr/overlay662_name_init.h"
#include "game/system/pad.h"
#include "rr/overlay662_name_controller.h"
#include "rr/frame_storage_types.h"

void ovl662_init_name_entry(void) {
    ModeFrame = 0;
    ScreenFrame = 0;
    ovl662_name_grid_col = 0;
    ovl662_name_grid_row = 0;
    ovl662_name_entry_cursor = 0;
    ovl662_name_grid_index = 0;
    CopyStringClearTail((char *)ovl662_name_entry_buffer, (char *)&SaveProgress);
    ClearScreenOverlayState();
    InitBadgeRow(0, 4, 172, 312, 216, 0, 0x806);
    BadgeRows[0].half02 = 1;
    InitBadgeRow(1, 208, 84, 108, 80, 0, 0x905);
    BadgeRows[1].half02 = 1;
    SetMenuFrameOptions(0, 1, 1);
    ModeGroup = 1;
    ModeIndex = 28;
    SubmodeState = 0;
}

void ovl662_update_name_entry(void) {
    s16 fadeStep;
    s32 cancel;
    s32 phase;
    u8 normalizedRow;

    phase = (u16)SubmodeState;
    ModeFrame=ModeFrame+1U;
    cancel = 0;
    ScreenFrame=ScreenFrame+1U;
    if (phase == 0) {
        SetMenuFrameOptions(1, 1, 1);
        if (PlayerControllers[0].action2C != 0) {cancel=1;goto normalizeColumn;}
        if (PlayerControllers[0].edges08.fields.repeat & 0x1000) {
            PlaySfx(0);ovl662_name_grid_row--;goto normalizeRow;
        }else if (PlayerControllers[0].edges08.fields.repeat & 0x4000) {
            PlaySfx(0);ovl662_name_grid_row++;goto normalizeRow;
        }else if (PlayerControllers[0].edges08.fields.repeat & 0x8000) {
            PlaySfx(0);ovl662_name_grid_col--;goto normalizeColumn;
        }else if (PlayerControllers[0].edges08.fields.repeat & 0x2000) {
            PlaySfx(0);ovl662_name_grid_col++;goto normalizeColumn;
        }else goto normalizeColumn;
normalizeRow:
        normalizedRow=((s8)ovl662_name_grid_row+5)%5;
        ovl662_name_grid_row=normalizedRow;
        if((s8)ovl662_name_grid_col>=3){if((s8)normalizedRow==4)ovl662_name_grid_col=2;}
normalizeColumn:
        if((s8)ovl662_name_grid_row>=4)ovl662_name_grid_col=((s8)ovl662_name_grid_col+3)%3;
        else ovl662_name_grid_col=((s8)ovl662_name_grid_col+10)%10;
        ovl662_name_grid_index = (s8) ovl662_name_grid_col + ((s8) ovl662_name_grid_row * 0xA);
        if (MenuReturnSubmodeIndex != 0) {
            PlaySfx(1);
            if(ovl662_name_grid_index<41){
                if (ovl662_name_entry_cursor < 8) {
                    PlaySfx(1);
                    if (ovl662_name_grid_index == 0x28) {
                        ovl662_name_entry_buffer[ovl662_name_entry_cursor] = NameCharset[0];
                    } else {
                        ovl662_name_entry_buffer[ovl662_name_entry_cursor] = NameCharset[ovl662_name_grid_index+1];
                    }
                    if (ovl662_name_entry_cursor < 7) {
                        ovl662_name_entry_cursor = (u8) ovl662_name_entry_cursor + 1;
                    }
                }
}else if(ovl662_name_grid_index==41){
                if (ovl662_name_entry_cursor > 0) {
                    ovl662_name_entry_cursor = (u8) ovl662_name_entry_cursor - 1;
                }
}else if(ovl662_name_grid_index==42){
                FadeCountdown = 0x40;
                SubmodeState = 1;
                CopyStringClearTail(SaveProgress.prefix.progress.prefix00.fields.name,(char*)ovl662_name_entry_buffer);
            }
        } else if (cancel == 1) {
            PlaySfx(2);
            ovl662_name_entry_buffer[ovl662_name_entry_cursor] = NameCharset[0];
            if (ovl662_name_entry_cursor > 0) {
                ovl662_name_entry_cursor = (u8) ovl662_name_entry_cursor - 1;
            }
        }
    } else if (phase == 1) {
        SetMenuFrameOptions(0, 1, 1);
        fadeStep = (u16)FadeCountdown - 1;
        FadeCountdown = (u16) fadeStep;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x40 - fadeStep) * 4, 0x45);
        if ((s16) FadeCountdown <= 0) {
            CardMenuOperation = 5;
            ModeGroup = phase;
            ModeIndex = 0x47;
            SubmodeState = 0;
        }
    }
    UpdateBadgeRowAnim(0);
    UpdateBadgeRowAnim(1);
    UpdateScreenFade(0x1008B);
}

/* Initial values of the overlay globals this module owns. */
s8 ovl662_name_entry_cursor;
s8 ovl662_name_grid_col;
s8 ovl662_name_grid_row;
u8 ovl662_name_entry_buffer[12];
s32 ovl662_name_grid_index;

/* Writable state of this unit; ovl662_reset_module restores it. */
static const OverlayStateRegion ovl662_name_entry_regions[] = {
    RR_OVERLAY_STATE(ovl662_name_entry_buffer),
    RR_OVERLAY_STATE(ovl662_name_entry_cursor),
    RR_OVERLAY_STATE(ovl662_name_grid_col),
    RR_OVERLAY_STATE(ovl662_name_grid_index),
    RR_OVERLAY_STATE(ovl662_name_grid_row)};
const OverlayStateTable ovl662_name_entry_state = {ovl662_name_entry_regions, sizeof(ovl662_name_entry_regions) / sizeof(ovl662_name_entry_regions[0])};
