#include "rr/overlay_module.h"
#include "rr/audio_state_types.h"
#include "rr/overlay_menu_init.h"
#include "game/system/pad.h"
#include "rr/overlay665_audio_options.h"
#include "rr/frame_storage_types.h"

void ovl665_init_audio_options(void)
{
    ClearScreenOverlayState();
    InitUiRow(0,12,72,68,32,0,0,1,0,86);
    InitUiRow(1,12,112,68,32,0,0,1,0,87);
    InitUiRow(2,12,152,68,32,0,0,1,0,88);
    InitUiRow(3,12,192,68,32,0,0,1,0,85);
    InitUiRow(4,100,72,98,32,0,1,1,10,89);
    InitUiRow(5,100,112,98,32,0,1,1,11,89);
    InitUiRow(6,100,152,68,32,0,1,1,0,91);
    InitUiRow(7,100,192,68,32,0,1,1,0,92);
    SetMenuFrameOptions(1,1,1);
}

void ovl665_update_audio_options(void) {
    s32 row;
    s16 closingTimer;
    u8 monoState;
    u8 musicState;
    u8 effectsState;
    u8 detailState;

    switch (ovl665_options_state.visual.layout0C) {
    case 0:
        ovl665_init_audio_options();
        FadeCountdown = 0x40;
        ovl665_options_state.visual.layout0C = (ovl665_options_state.visual.layout0C + 1U);
        break;
    case 1:
        if (FadeCountdown > 0) {
            FadeCountdown--;
            AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[702], FadeCountdown * 4, 0x45);
        }
        SetPresentationDrawEnabled(1);
        if (UiRows[0].byte14 == 2 && FadeCountdown == 0) {
            ovl665_options_state.visual.layout0C = (ovl665_options_state.visual.layout0C + 1U);
        }
        break;
    case 2:
        UiRows[ovl665_options_state.visual.exit34.word].cells[0] = 0;
        StepMenuValue(0, 2, (char *)&ovl665_options_state.visual.exit34.halves[0], 4, 0);
        UiRows[ovl665_options_state.visual.exit34.word].cells[0] = 2;
        if (PlayerControllers[0].action28 != 0) {
            PlaySfx(1);
            UiRows[ovl665_options_state.visual.exit34.word].cells[0] = 1;
            ovl665_options_state.visual.layout0C = (ovl665_options_state.visual.exit34.word + 3);
            SetMenuFrameOptions(1, 1, 0);
        } else if (PlayerControllers[0].action2C != 0) {
            PlaySfx(2);
            FadeCountdown = 0x40;
            ovl665_options_state.visual.layout0C = 6;
        }
        break;
    case 3:
        musicState = UiRows[4].byte14;
        if (musicState == 2) {
            UiRows[4].cells[0] = musicState;
            StepMenuValue(1, -1, (char *)&Options.bgmVolume30, 0x11, 0);
            ovl665_saved_bgm_volume = Options.bgmVolume30;
            SeqSetVolume(SoundState.seqVol2888);
            if (MenuReturnSubmodeIndex != 0) {
                PlaySfx(1);
                UiRows[4].cells[0] = 1U;
                ovl665_options_state.visual.layout0C = 7U;
                SetMenuFrameOptions(1, 1, 1);
            }
        }
        DrawUiRow(4);
        break;
    case 4:
        effectsState = UiRows[5].byte14;
        if (effectsState == 2) {
            UiRows[5].cells[0] = effectsState;
            StepMenuValue(1, -1, (char *)&Options.volume31, 0x11, 0);
            ovl665_saved_sfx_volume = Options.volume31;
            if (MenuReturnSubmodeIndex != 0) {
                PlaySfx(1);
                UiRows[5].cells[0] = 1U;
                ovl665_options_state.visual.layout0C = 7U;
                SetMenuFrameOptions(1, 1, 1);
            }
        }
        DrawUiRow(5);
        break;
    case 5:
        monoState = UiRows[6].byte14;
        if (monoState == 2) {
            UiRows[Options.mono32 + 6].cells[0] = 0;
            StepMenuValue(0, 2, (char *)&Options.mono32, 2, 0);
            UiRows[Options.mono32 + 6].cells[0] = monoState;
            if (MenuReturnSubmodeIndex != 0) {
                PlaySfx(1);
                UiRows[Options.mono32 + 6].cells[0] = 1;
                ovl665_options_state.visual.layout0C = 7U;
                SetMenuFrameOptions(1, 1, 1);
                SetCdMixMode(Options.mono32);
            }
        }
        DrawUiRow(6);
        DrawUiRow(7);
        break;
    case 6:
        for (row = 0; row < 4; row++) {
            if (UiRows[row].byte14 == 2) {
                UiRows[row].byte14 = 3U;
            }
        }
        FadeCountdown = 0x40;
        ovl665_options_state.visual.layout0C = 9U;
        break;
    case 7:
        for (row = 4; row < 8; row++) {
            detailState = UiRows[row].byte14;
            if (detailState == 2) {
                UiRows[row].byte14 = 3U;
            } else if (detailState == 6) {
                ovl665_options_state.visual.layout0C = 8;
            }
            if ((u8)UiRows[row].byte14 >= 2U) {
                DrawUiRow(row);
            }
        }
        break;
    case 8:
        for (row = 4; row < 8; row++) {
            if (UiRows[row].byte14 != 0) {
                UiRows[row].byte14 = 0U;
            }
        }
        ovl665_options_state.visual.layout0C = 1U;
        break;
    case 9:
        closingTimer = (u16)FadeCountdown - 1;
        FadeCountdown = closingTimer;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (0x40 - closingTimer) * 4, 0x45);
        if (FadeCountdown <= 0) {
            ovl665_init_menu();
            ovl665_options_state.visual.layout0C = 0;
            ovl665_options_state.visual.scene00 = (ovl665_options_state.visual.scene00 - 1U);
        }
        break;
    }
    for (row = 0; row < 4; row++) {
        DrawUiRow(row);
    }
    UpdateScreenFade(0x1006A);
}

/* Initial values of the overlay globals this module owns. */
s32 ovl665_saved_bgm_volume;
s32 ovl665_saved_sfx_volume;

/* Writable state of this unit; ovl665_reset_module restores it. */
static const OverlayStateRegion ovl665_audio_options_regions[] = {
    RR_OVERLAY_STATE(ovl665_saved_bgm_volume),
    RR_OVERLAY_STATE(ovl665_saved_sfx_volume)};
const OverlayStateTable ovl665_audio_options_state = {ovl665_audio_options_regions, sizeof(ovl665_audio_options_regions) / sizeof(ovl665_audio_options_regions[0])};

/* Initial values of the globals this module owns. */
