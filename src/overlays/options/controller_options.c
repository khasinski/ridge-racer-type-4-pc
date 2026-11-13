#include "rr/release.h"
#include "rr/overlay_module.h"
#include "rr/runtime_settings.h"
#include "rr/menu_frame_state.h"
#include "rr/overlay665_strokes.h"
#include "rr/menu_text_state.h"
#include "game/car/carstat.h"
#include "rr/line_emit.h"
#include "rr/frame_storage_types.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "types.h"
#include "game/system/pad.h"
#include "rr/ui.h"
#include "rr/badge_rows.h"

/* RaceVehicle / course setup controller (one player). */

typedef struct {
    s16 x00, y02;
    s32 words04[4];
} Ovl665View14;

typedef struct {
    s32 scene00, reserved04, selection08;
    s32 step0C, detail10, next14, kind18;
    s32 angle1C, phase20, fade24;
    u16 buttons28;
    s16 course2A;
    s32 ready2C;
    u8 pad30[8];
    Ovl665View14 view38;
    u8 pad4C[0x3A];
    u16 rotation86;
    u8 pad88[4];
    s16 offset8C, offset8E, offset90;
    u8 pad92[2];
    s16 offset94, offset96, offset98;
    u8 pad9A[6];
} Ovl665CarState;





typedef GpuCommandPacket Ovl665Packet;

typedef struct {
    u8 a, pad1, b, pad3;
} Ovl665Colour4;

typedef struct {
    s16 x0, y0, x1, y1;
} Ovl665Line;

extern Ovl665CarState ovl665_options_state;
extern s32 ovl665_saved_stereo_mode;
extern s16 FadeCountdown;
extern s32 ActivePlayerIndex;
extern Ovl665View14 ovl665_controller_camera_poses[];
extern Ovl665Colour4 ovl665_controller_colors_type0[][5];
extern Ovl665Colour4 ovl665_controller_colors_type1[][5];
extern Ovl665Colour4 ovl665_controller_colors_type2[][5];
/* Callout stroke paths: five strokes for each of the eight selections. The
 * step lists they address are unnamed original data between the tables. */
extern StrokeStep4 ovl665_callout_steps_type0[28], ovl665_callout_steps_type1[46], ovl665_callout_steps_type2[52];
extern const StrokeStep4 *ovl665_callout_paths_type0[8][5];
extern const StrokeStep4 *ovl665_callout_paths_type1[8][5];
extern const StrokeStep4 *ovl665_callout_paths_type2[8][5];
extern const StrokeStep4 *ovl665_callout_paths_type3[8][5];
extern s32 ovl665_controller_row_counts[];
extern s32 ovl665_controller_model_scales[];
extern u8 ovl665_twist_needle_color[];
extern UiUvRect ovl665_screen_arrow_uv_rects[2];
extern s16 TwistRangeLimits[];
extern s16 PadRangeLimits[];
extern s32 WheelDeadzoneTable[];

extern s32 PadGetState(s32);
extern s32 PadSetMainMode(s32, s32, s32);
extern s32 LoadTrophyModelStep(void);
extern s32 ovl665_detect_controller_kind(s32);
extern void SetPresentationDrawEnabled(s32);
extern void StepMenuValue(s32, s32, void *, s32, s32);
/* Sound cue; several call sites pass the freshly computed offset as a
 * second (ignored) argument, so it is declared without a prototype. */
extern void PlaySfx();
extern void *AddSpritePrim(void *, void *, s32, s32, void *, s32, s32);
extern void ovl665_draw_controller_model(void);
extern void DrawUiRow(s32);
#include "rr/screen_tiles.h"
extern void ovl665_init_menu(void);
extern void ClearScreenOverlayState();
extern void InitBadgeRow(s32, s32, s32, s32, s32, s32, s32);
extern void SetMenuFrameOptions(s32, s32, s32);
extern s32 rsin(s32);
extern void PadSetRumbleFlags(s32, s32);
extern void ovl665_draw_twist_gauge_needle(s32);
extern void UpdateBadgeRowAnim(s32);
extern void UpdateScreenFade(s32);

#define ST ovl665_options_state

#define SCRATCH RR_GPU_SCRATCH

#define CLOSE_ROWS(n)                                   \
    BadgeRows->state10 = 4;                            \
    for (i = 0; i < (n); i++) {                         \
        if (UiRows[i].byte14 == 2) {                \
            UiRows[i].byte14 = 3;                   \
        }                                               \
    }

#define COPY_COLOURS(src)                               \
    for (i = 0; i < 5; i++) {                           \
        UiRows[i].byte15 = (src)->a;                \
        UiRows[i].byte16 = (src)->b;                \
        (src)++;                                        \
    }

/* Links a texture-page command packet into the frame ordering list. */
#define LINK_PACKET()                                                       \
    {                                                                       \
        packet = (Ovl665Packet *)SCRATCH->cursor;                           \
        packet->command = 0xE1000259;                                       \
        RR_GPU_ADD_PRIM(&CurrentDrawBuf->buffer.ordering70.tags[0], packet, 1); \
        packet++;                                                           \
        SCRATCH->cursor = (u8 *)packet;                                     \
    }

void ovl665_update_controller_options(s32 player) {
    Ovl665Colour4 *src;
    PadPort28 *entry;
    s32 changed;
    s32 i;
    s32 kind;
    s32 index;
    s32 old;
    s32 value;
    s32 phase;
    s32 wave;
    s32 arg;
    s16 timer;
    void *cursor;
    Ovl665Line line;
    Ovl665Packet *packet;

    ST.buttons28 = PlayerControllers[player].edges08.fields.pressed;
    entry = &PadPorts[player];
    changed = 0;
    switch (ST.step0C) {
    case 0:
        if (RR_RELEASE_JP) {
            /* No analog-mode wait in the Japanese release: a neGcon-class
             * pad is switched once and the page opens at once. */
            if (PlayerControllers[player].flag40 == 1 && ST.ready2C == 0) {
                if (PlayerControllers[player].input00.fields.type == 0xE3) {
                    ST.ready2C = 1;
                } else if (PadGetState(player << 4) == 6) {
                    PadSetMainMode(player << 4, 1, 0);
                }
            }
        } else if (ovl665_saved_stereo_mode == 0) {
            if (PlayerControllers[player].flag40 == 1) {
                if (ST.ready2C == 0) {
                    if (PlayerControllers[player].input00.fields.type != 0xE3) {
                        if (PadGetState(player << 4) == 6) {
                            PadSetMainMode(player << 4, 1, 0);
                        }
                    } else {
                        ST.ready2C = 1;
                        ovl665_saved_stereo_mode = 1;
                    }
                }
            } else {
                ovl665_saved_stereo_mode = 1;
            }
        }
        ActivePlayerIndex = player;
        if (LoadTrophyModelStep() == 0) {
            break;
        }
        if (!RR_RELEASE_JP && ovl665_saved_stereo_mode == 0) {
            break;
        }
        kind = ovl665_detect_controller_kind(player);
        ST.kind18 = kind;
        if (kind >= 0) {
            switch (kind) {
            case 0:
            case 1:
                index = Options.pulse[player].buttonLayout00;
                src = ovl665_controller_colors_type0[index];
                index <<= 12;
                ST.rotation86 = index;
                break;
            case 2:
                index = Options.pad[player].buttonLayout0C;
                src = ovl665_controller_colors_type1[index];
                index <<= 12;
                ST.rotation86 = index;
                break;
            case 3:
                if (PlayerControllers[player].input00.fields.type != 0xE3) {
                    index = Options.pulse[player].buttonLayout00;
                    src = ovl665_controller_colors_type0[index];
                    index <<= 12;
                ST.rotation86 = index;
                } else {
                    index = Options.wheel[player].buttonLayout03;
                    src = ovl665_controller_colors_type2[index];
                    index <<= 12;
                ST.rotation86 = index;
                }
                ST.course2A = PlayerControllers[player].input00.fields.type;
                break;
            }
            ST.view38 = ovl665_controller_camera_poses[ST.kind18];
            COPY_COLOURS(src);
        } else {
            CLOSE_ROWS(8);
            FadeCountdown = 64;
            ST.step0C = 2;
        }
        ST.detail10 = 0;
        ST.angle1C = 0;
        ST.fade24 = 0;
        ST.offset8C = 0;
        ST.offset8E = 0;
        ST.offset90 = 0;
        ST.offset94 = 0;
        ST.offset96 = 0;
        ST.offset98 = 0;
        SetPresentationDrawEnabled(0);
        ST.step0C++;
        break;
    case 1:
        if (BadgeRows->state10 == 3 && UiRows[0].byte14 == 2) {
            if (ovl665_detect_controller_kind(player) < 0) {
                CLOSE_ROWS(8);
                FadeCountdown = 64;
                ST.step0C = 2;
                ST.next14 = 0;
            }
            switch (ST.kind18) {
            case 1:
                if (RR_RELEASE_JP || PlayerControllers[player].kind1C != 0) {
                    UiRows[5].cells[0] = 1;
                    UiRows[Options.pulse[player].disabled02 + 6].cells[0] = 0;
                    StepMenuValue(player * 4, 2, &Options.pulse[player].disabled02, 2, 0);
                    UiRows[Options.pulse[player].disabled02 + 6].cells[0] = 2;
                }
            case 0:
                old = Options.pulse[player].buttonLayout00;
                StepMenuValue(player * 4 | 1, 2, &Options.pulse[player].buttonLayout00, 8, 0);
                if (old != Options.pulse[player].buttonLayout00) {
                    if (Options.pulse[player].buttonLayout00 < old) {
                        ST.angle1C -= 0x1000;
                    } else {
                        ST.angle1C += 0x1000;
                    }
                    src = ovl665_controller_colors_type0[Options.pulse[player].buttonLayout00];
                    COPY_COLOURS(src);
                    ST.fade24 = 0;
                    SetPadButtonMapLayout(player, Options.pulse[player].buttonLayout00, Options.pad[player].buttonLayout0C,
                                  Options.wheel[player].buttonLayout03);
                }
                if (PlayerControllers[player].action28 != 0) {
                    PlaySfx(1);
                    CLOSE_ROWS(5);
                    FadeCountdown = 64;
                    ST.step0C = 2;
                    ST.next14 = 0;
                }
                ovl665_draw_callout_strokes(ovl665_callout_paths_type0[Options.pulse[player].buttonLayout00], ST.fade24);
                cursor = SCRATCH->cursor;
                if (Options.pulse[player].buttonLayout00 > 0) {
                    cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 56, 280, &ovl665_screen_arrow_uv_rects[0], 0x7F38, 255);
                }
                if (Options.pulse[player].buttonLayout00 < 7) {
                    cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 254, 280, &ovl665_screen_arrow_uv_rects[1], 0x7F38, 255);
                }
                SCRATCH->cursor = cursor;
                break;
            case 2:
                old = Options.pad[player].buttonLayout0C;
                StepMenuValue(player * 4 | 1, 2, &Options.pad[player].buttonLayout0C, 8, 0);
                if (old != Options.pad[player].buttonLayout0C) {
                    if (Options.pad[player].buttonLayout0C < old) {
                        ST.angle1C -= 0x1000;
                    } else {
                        ST.angle1C += 0x1000;
                    }
                    src = ovl665_controller_colors_type1[Options.pad[player].buttonLayout0C];
                    COPY_COLOURS(src);
                    ST.fade24 = 0;
                    SetPadButtonMapLayout(player, Options.pulse[player].buttonLayout00, Options.pad[player].buttonLayout0C,
                                  Options.wheel[player].buttonLayout03);
                }
                value = PlayerControllers[player].steering20 * 3800 / 256 - 1900;
                ST.offset8C = value / 2;
                ST.offset94 = -value / 2;
                if (PlayerControllers[player].action28 != 0) {
                    PlaySfx(1, value);
                    CLOSE_ROWS(5);
                    FadeCountdown = 64;
                    ST.step0C = 2;
                    ST.next14 = 3;
                }
                ovl665_draw_callout_strokes(ovl665_callout_paths_type1[Options.pad[player].buttonLayout0C], ST.fade24);
                cursor = SCRATCH->cursor;
                if (Options.pad[player].buttonLayout0C > 0) {
                    cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 56, 280, &ovl665_screen_arrow_uv_rects[0], 0x7F38, 255);
                }
                if (Options.pad[player].buttonLayout0C < 7) {
                    cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 254, 280, &ovl665_screen_arrow_uv_rects[1], 0x7F38, 255);
                }
                SCRATCH->cursor = cursor;
                break;
            case 3:
                if (ST.course2A != PlayerControllers[player].input00.fields.type) {
                    changed = 1;
                    ST.course2A = PlayerControllers[player].input00.fields.type;
                }
                if (PlayerControllers[player].input00.fields.type != 0xE3) {
                    old = Options.pulse[player].buttonLayout00;
                    StepMenuValue(player * 4 | 1, 1, &Options.pulse[player].buttonLayout00, 8, 0);
                    if (old != Options.pulse[player].buttonLayout00) {
                        if (Options.pulse[player].buttonLayout00 < old) {
                            ST.angle1C -= 0x1000;
                        } else {
                            ST.angle1C += 0x1000;
                        }
                        changed = 1;
                        SetPadButtonMapLayout(player, Options.pulse[player].buttonLayout00, Options.pad[player].buttonLayout0C,
                                      Options.wheel[player].buttonLayout03);
                    }
                    if (changed) {
                        src = ovl665_controller_colors_type0[Options.pulse[player].buttonLayout00];
                        COPY_COLOURS(src);
                        ST.fade24 = 0;
                    }
                    if (PlayerControllers[player].action28 != 0) {
                        PlaySfx(1);
                        CLOSE_ROWS(5);
                        FadeCountdown = 64;
                        ST.step0C = 2;
                        ST.next14 = 0;
                    }
                    ovl665_draw_callout_strokes(ovl665_callout_paths_type2[Options.pulse[player].buttonLayout00], ST.fade24);
                    cursor = SCRATCH->cursor;
                    if (Options.pulse[player].buttonLayout00 > 0) {
                        cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 56, 280, &ovl665_screen_arrow_uv_rects[0], 0x7F38, 255);
                    }
                    if (Options.pulse[player].buttonLayout00 < 7) {
                        cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 254, 280, &ovl665_screen_arrow_uv_rects[1], 0x7F38, 255);
                    }
                    SCRATCH->cursor = cursor;
                } else {
                    old = Options.wheel[player].buttonLayout03;
                    StepMenuValue(player * 4 | 1, 1, &Options.wheel[player].buttonLayout03, 8, 0);
                    if (old != Options.wheel[player].buttonLayout03) {
                        if (Options.wheel[player].buttonLayout03 < old) {
                            ST.angle1C -= 0x1000;
                        } else {
                            ST.angle1C += 0x1000;
                        }
                        changed = 1;
                        SetPadButtonMapLayout(player, Options.pulse[player].buttonLayout00, Options.pad[player].buttonLayout0C,
                                      Options.wheel[player].buttonLayout03);
                    }
                    if (changed) {
                        src = ovl665_controller_colors_type2[Options.wheel[player].buttonLayout03];
                        COPY_COLOURS(src);
                        ST.fade24 = 0;
                    }
                    value = (s8)PlayerControllers[player].position41 * 16;
                    ST.offset90 = value;
                    if (PlayerControllers[player].action28 != 0) {
                        PlaySfx(1, value);
                        CLOSE_ROWS(5);
                        FadeCountdown = 64;
                        ST.step0C = 2;
                        ST.next14 = 4;
                    }
                    ovl665_draw_callout_strokes(ovl665_callout_paths_type3[Options.wheel[player].buttonLayout03], ST.fade24);
                    cursor = SCRATCH->cursor;
                    if (Options.wheel[player].buttonLayout03 > 0) {
                        cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 56, 280, &ovl665_screen_arrow_uv_rects[0], 0x7F38, 255);
                    }
                    if (Options.wheel[player].buttonLayout03 < 7) {
                        cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 254, 280, &ovl665_screen_arrow_uv_rects[1], 0x7F38, 255);
                    }
                    SCRATCH->cursor = cursor;
                }
                break;
            }
            ST.fade24++;
            if (ST.fade24 > 16) {
                ST.fade24 = 16;
            }
        }
        ST.angle1C = ST.angle1C * 15 / 16;
        ST.rotation86 = ST.angle1C;
        if ((ST.angle1C + 128) > 256U) {
            ST.fade24 = 0;
        }
        LINK_PACKET();
        ovl665_draw_controller_model();
        for (i = 0; i < 5; i++) {
            DrawUiRow(i);
        }
        if (ST.kind18 == 1 && (RR_RELEASE_JP || PlayerControllers[player].kind1C != 0)) {
            for (i = 5; i < 8; i++) {
                DrawUiRow(i);
            }
        }
        break;
    case 2:
        timer = FadeCountdown - 1;
        FadeCountdown = timer;
        AddScreenFadeTiles(&CurrentDrawBuf->buffer.ordering70.tags[0], (64 - timer) * 4, 69);
        if (FadeCountdown <= 0) {
            if (ST.next14 == 0) {
                ovl665_init_menu();
                ST.step0C = 0;
                ActivePlayerIndex = 0;
                ST.scene00--;
                return;
            }
            ST.step0C = ST.next14;
            ST.detail10 = 0;
            ST.next14 = 0;
        }
        ovl665_draw_controller_model();
        for (i = 0; i < 5; i++) {
            DrawUiRow(i);
        }
        break;
    case 3:
        switch (ST.detail10) {
        case 0:
            ClearScreenOverlayState();
            ScreenFadeState.phase = 2;
            InitBadgeRow(0, 12, 304, 296, 88, 0, 0);
            BadgeRows->half02 = 1;
            SetMenuFrameOptions(1, 0, 0);
            ST.view38 = ovl665_controller_camera_poses[4];
            ST.rotation86 = 0;
            ST.angle1C = 0;
            CardDialogueStatePtr->b4 = 9;
            ST.detail10++;
            break;
        case 1:
            value = PlayerControllers[player].steering20 * 3800 / 256 - 1900;
            ST.offset8C = value / 2;
            ST.offset94 = -value / 2;
            if (PlayerControllers[player].edges08.fields.pressed & 0x800) {
                PlaySfx(1, value);
                Options.pad[player].center00 = entry->analog04.bytes[0] - 128;
                Options.pad[player].offset02 = entry->analog04.bytes[1];
                Options.pad[player].offset04 = entry->analog04.bytes[2];
                Options.pad[player].offset06 = entry->analog04.bytes[3];
                CardDialogueStatePtr->b4 = Options.pad[player].deadzone08 + 10;
                ST.view38 = ovl665_controller_camera_poses[5];
                ST.phase20 = 0;
                ST.detail10++;
                SetMenuFrameOptions(1, 1, 0);
            }
            break;
        case 2:
            old = Options.pad[player].deadzone08;
            StepMenuValue(player * 4 | 1, 1, &Options.pad[player].deadzone08, 4, 0);
            if (old != Options.pad[player].deadzone08) {
                CardDialogueStatePtr->b4 = Options.pad[player].deadzone08 + 10;
            }
            value = rsin(ST.phase20) * ovl665_controller_row_counts[Options.pad[player].deadzone08] / 512;
            ST.offset8C = value;
            ST.offset94 = -value;
            phase = ST.phase20;
            ST.phase20 = phase + 32;
            if (ST.phase20 >= 4096) {
                ST.phase20 = phase - 4064;
            }
            if (PlayerControllers[player].edges08.fields.pressed & 0x840) {
                PlaySfx(1, value);
                CardDialogueStatePtr->b4 = Options.pad[player].range0A + 14;
                ST.view38 = ovl665_controller_camera_poses[5];
                ST.phase20 = 0;
                ST.detail10++;
            }
            line.x0 = 148;
            line.x1 = 172;
            line.y0 = line.y1 = ST.view38.y02;
            AddLineF2Prim((const s16 *)&line, ovl665_twist_needle_color, 0, 0);
            if (Options.pad[player].deadzone08 > 0) {
                line.y0 = line.y1 = ST.view38.y02 + ovl665_controller_model_scales[Options.pad[player].deadzone08 - 1];
                AddLineF2Prim((const s16 *)&line, ovl665_twist_needle_color, 0, 0);
                line.y0 = line.y1 = ST.view38.y02 - ovl665_controller_model_scales[Options.pad[player].deadzone08 - 1];
                AddLineF2Prim((const s16 *)&line, ovl665_twist_needle_color, 0, 0);
            }
            cursor = SCRATCH->cursor;
            if (Options.pad[player].deadzone08 > 0) {
                cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 56, 182, &ovl665_screen_arrow_uv_rects[0], 0x7F38, 255);
            }
            if (Options.pad[player].deadzone08 < 3) {
                cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 254, 182, &ovl665_screen_arrow_uv_rects[1], 0x7F38, 255);
            }
            SCRATCH->cursor = cursor;
            break;
        case 3:
            old = Options.pad[player].range0A;
            StepMenuValue(player * 4 | 1, 1, &Options.pad[player].range0A, 4, 0);
            if (old != Options.pad[player].range0A) {
                CardDialogueStatePtr->b4 = Options.pad[player].range0A + 14;
            }
            value = rsin(ST.phase20) * TwistRangeLimits[Options.pad[player].range0A] / 512;
            ST.offset8C = value;
            ST.offset94 = -value;
            phase = ST.phase20;
            ST.phase20 = phase + 32;
            if (ST.phase20 >= 4096) {
                ST.phase20 = phase - 4064;
            }
            if (PlayerControllers[player].edges08.fields.pressed & 0x840) {
                PlaySfx(1, value);
                CLOSE_ROWS(5);
                FadeCountdown = 64;
                ST.step0C = 2;
            }
            cursor = SCRATCH->cursor;
            if (Options.pad[player].range0A > 0) {
                cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 56, 182, &ovl665_screen_arrow_uv_rects[0], 0x7F38, 255);
            }
            if (Options.pad[player].range0A < 3) {
                cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 254, 182, &ovl665_screen_arrow_uv_rects[1], 0x7F38, 255);
            }
            SCRATCH->cursor = cursor;
            break;
        }
        if (ovl665_detect_controller_kind(player) < 0) {
            CLOSE_ROWS(8);
            FadeCountdown = 64;
            ST.step0C = 2;
            ST.next14 = 0;
        }
        LINK_PACKET();
        ovl665_draw_controller_model();
        break;
    case 4:
        switch (ST.detail10) {
        case 0:
            ClearScreenOverlayState();
            ScreenFadeState.phase = 2;
            InitBadgeRow(0, 12, 304, 296, 88, 0, 0);
            BadgeRows->half02 = 1;
            SetMenuFrameOptions(1, 0, 0);
            ST.view38 = ovl665_controller_camera_poses[6];
            ST.rotation86 = 0;
            ST.angle1C = 0;
            CardDialogueStatePtr->b4 = 8;
            ST.detail10++;
            break;
        case 1:
            value = (s8)PlayerControllers[player].position41 * 16;
            ST.offset90 = value;
            if (PlayerControllers[player].edges08.fields.pressed & 0x800) {
                PlaySfx(1, value);
                PadSetRumbleFlags(player, 3);
                CardDialogueStatePtr->b4 = Options.wheel[player].deadzone00 + 10;
                ST.phase20 = 0;
                ST.detail10++;
                SetMenuFrameOptions(1, 1, 0);
            }
            break;
        case 2:
            old = Options.wheel[player].deadzone00;
            StepMenuValue(player * 4 | 1, 1, &Options.wheel[player].deadzone00, 4, 0);
            if (old != Options.pad[player].deadzone08) {
                CardDialogueStatePtr->b4 = Options.wheel[player].deadzone00 + 10;
            }
            value = rsin(ST.phase20) * WheelDeadzoneTable[Options.wheel[player].deadzone00] / 128;
            ST.offset90 = value;
            phase = ST.phase20;
            ST.phase20 = phase + 32;
            if (ST.phase20 >= 4096) {
                ST.phase20 = phase - 4064;
            }
            if (PlayerControllers[player].action28 != 0) {
                PlaySfx(1, value);
                CardDialogueStatePtr->b4 = Options.wheel[player].range01 + 18;
                ST.phase20 = 0;
                ST.detail10++;
            }
            if (Options.wheel[player].deadzone00 > 0) {
                ovl665_draw_twist_gauge_needle(WheelDeadzoneTable[Options.wheel[player].deadzone00] << 5);
                ovl665_draw_twist_gauge_needle(-WheelDeadzoneTable[Options.wheel[player].deadzone00] << 5);
            }
            cursor = SCRATCH->cursor;
            if (Options.wheel[player].deadzone00 > 0) {
                cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 56, 182, &ovl665_screen_arrow_uv_rects[0], 0x7F38, 255);
            }
            if (Options.wheel[player].deadzone00 < 3) {
                cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 254, 182, &ovl665_screen_arrow_uv_rects[1], 0x7F38, 255);
            }
            SCRATCH->cursor = cursor;
            break;
        case 3:
            old = Options.wheel[player].range01;
            StepMenuValue(player * 4 | 1, 1, &Options.wheel[player].range01, 5, 0);
            if (old != Options.wheel[player].range01) {
                CardDialogueStatePtr->b4 = Options.wheel[player].range01 + 18;
            }
            value = rsin(ST.phase20) * PadRangeLimits[Options.wheel[player].range01] / 256;
            ST.offset90 = value;
            phase = ST.phase20;
            ST.phase20 = phase + 32;
            if (ST.phase20 >= 4096) {
                ST.phase20 = phase - 4064;
            }
            if (PlayerControllers[player].action28 != 0) {
                PlaySfx(1, value);
                CardDialogueStatePtr->b4 = Options.wheel[player].rumble02 + 23;
                ST.phase20 = 0;
                ST.detail10++;
            }
            cursor = SCRATCH->cursor;
            if (Options.wheel[player].range01 > 0) {
                cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 56, 182, &ovl665_screen_arrow_uv_rects[0], 0x7F38, 255);
            }
            if (Options.wheel[player].range01 < 4) {
                cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 254, 182, &ovl665_screen_arrow_uv_rects[1], 0x7F38, 255);
            }
            SCRATCH->cursor = cursor;
            break;
        case 4:
            old = Options.wheel[player].rumble02;
            StepMenuValue(player * 4 | 1, 1, &Options.wheel[player].rumble02, 3, 0);
            if (old != Options.wheel[player].rumble02) {
                CardDialogueStatePtr->b4 = Options.wheel[player].rumble02 + 23;
            }
            wave = rsin(ST.phase20) * 21;
            value = wave * Options.wheel[player].rumble02 / 256;
            ST.offset90 = value;
            phase = ST.phase20;
            ST.phase20 = phase + 32;
            if (ST.phase20 >= 4096) {
                ST.phase20 = phase - 4064;
            }
            if (PlayerControllers[player].action28 != 0) {
                PlaySfx(1, value);
                CLOSE_ROWS(5);
                FadeCountdown = 64;
                ST.step0C = 2;
            }
            cursor = SCRATCH->cursor;
            if (Options.wheel[player].rumble02 > 0) {
                cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 56, 182, &ovl665_screen_arrow_uv_rects[0], 0x7F38, 255);
            }
            if (Options.wheel[player].rumble02 < 2) {
                cursor = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], cursor, 254, 182, &ovl665_screen_arrow_uv_rects[1], 0x7F38, 255);
            }
            SCRATCH->cursor = cursor;
            break;
        }
        if (ovl665_detect_controller_kind(player) < 0 || PlayerControllers[player].input00.fields.type != 0xE3) {
            CLOSE_ROWS(8);
            FadeCountdown = 64;
            ST.step0C = 2;
            ST.next14 = 0;
        }
        LINK_PACKET();
        ovl665_draw_controller_model();
        break;
    }
    if (ST.step0C < 3) {
        UpdateBadgeRowAnim(0);
        MenuFrameState.titlePlayer08 = player + 1;
        arg = 0x10072;
    } else {
        UpdateBadgeRowAnim(0);
        if (BadgeRows->state10 == 3) {
            UpdateDialogue(BadgeRows->x04 + 10, BadgeRows->y06 + 40, CardDialogueStatePtr);
        }
        arg = 0x83;
    }
    UpdateScreenFade(arg);
}

/* Initial values of the overlay globals this module owns. */
/* Left and right arrows: screen adjust draws both as one two-rect table,
 * the controller screens the left one ([0]) and the right one ([1]). */
UiUvRect ovl665_screen_arrow_uv_rects[2] = {{.u = 0xE8, .v = 0x58, .w = 8, .h = 0x10}, {.u = 0xF0, .v = 0x58, .w = 8, .h = 0x10}};
Ovl665Colour4 ovl665_controller_colors_type0[8][5] = {{{.a = 1, .b = 0x17}, {.b = 0x6D}, {.a = 1, .b = 0x16}, {.b = 0x6E}, {.b = 0x6F}}, {{.b = 0x6D}, {.a = 1, .b = 0x17}, {.a = 1, .b = 0x16}, {.b = 0x6E}, {.b = 0x6F}}, {{.a = 1, .b = 0x17}, {.b = 0x6D}, {.a = 1, .b = 0x16}, {.b = 0x6F}, {.b = 0x6E}}, {{.b = 0x6D}, {.a = 1, .b = 0x17}, {.a = 1, .b = 0x16}, {.b = 0x6F}, {.b = 0x6E}}, {{.a = 1, .b = 0x17}, {.b = 0x6D}, {.a = 1, .b = 0x16}, {.b = 0x6E}, {.b = 0x6F}}, {{.b = 0x6D}, {.a = 1, .b = 0x17}, {.a = 1, .b = 0x16}, {.b = 0x6E}, {.b = 0x6F}}, {{.a = 1, .b = 0x17}, {.b = 0x6D}, {.a = 1, .b = 0x16}, {.b = 0x6F}, {.b = 0x6E}}, {{.b = 0x6D}, {.a = 1, .b = 0x17}, {.a = 1, .b = 0x16}, {.b = 0x6F}, {.b = 0x6E}}};
Ovl665Colour4 ovl665_controller_colors_type1[8][5] = {{{.a = 1, .b = 0x16}, {.a = 1, .b = 0x17}, {.b = 0x6D}, {.b = 0x6E}, {.b = 0x6F}}, {{.a = 1, .b = 0x16}, {.a = 1, .b = 0x17}, {.b = 0x6D}, {.b = 0x6F}, {.b = 0x6E}}, {{.a = 1, .b = 0x17}, {.b = 0x6E}, {.b = 0x6F}, {.a = 1, .b = 0x16}, {.b = 0x6D}}, {{.a = 1, .b = 0x17}, {.b = 0x6E}, {.b = 0x6D}, {.a = 1, .b = 0x16}, {.b = 0x6F}}, {{.a = 1, .b = 0x17}, {.b = 0x6E}, {.b = 0x6F}, {.a = 1, .b = 0x16}, {.b = 0x6D}}, {{.a = 1, .b = 0x17}, {.b = 0x6D}, {.a = 1, .b = 0x16}, {.b = 0x6E}, {.b = 0x6F}}, {{.a = 1, .b = 0x17}, {.b = 0x6D}, {.a = 1, .b = 0x16}, {.b = 0x6F}, {.b = 0x6E}}, {{.a = 1, .b = 0x17}, {.b = 0x6F}, {.a = 1, .b = 0x16}, {.b = 0x6D}, {.b = 0x6E}}};
Ovl665Colour4 ovl665_controller_colors_type2[8][5] = {{{.b = 0x6E}, {.a = 1, .b = 0x17}, {.b = 0x6F}, {.a = 1, .b = 0x16}, {.b = 0x6D}}, {{.b = 0x6E}, {.b = 0x6D}, {.b = 0x6F}, {.a = 1, .b = 0x17}, {.a = 1, .b = 0x16}}, {{.a = 1, .b = 0x17}, {.b = 0x6E}, {.b = 0x6F}, {.a = 1, .b = 0x16}, {.b = 0x6D}}, {{.b = 0x6E}, {.a = 1, .b = 0x17}, {.a = 1, .b = 0x16}, {.b = 0x6F}, {.b = 0x6D}}, {{.b = 0x6E}, {.b = 0x6F}, {.b = 0x6D}, {.a = 1, .b = 0x16}, {.a = 1, .b = 0x17}}, {{.b = 0x6D}, {.a = 1, .b = 0x17}, {.a = 1, .b = 0x16}, {.b = 0x6E}, {.b = 0x6F}}, {{.b = 0x6D}, {.b = 0x6E}, {.b = 0x6F}, {.a = 1, .b = 0x17}, {.a = 1, .b = 0x16}}, {{.a = 1, .b = 0x17}, {.b = 0x6D}, {.a = 1, .b = 0x16}, {.b = 0x6E}, {.b = 0x6F}}};
StrokeStep4 ovl665_callout_steps_type0[28] = {{.direction = 1, .length = 32}, {.direction = 3, .length = 22}, {.direction = -1}, {.direction = 1, .length = 94}, {.direction = 3, .length = 120}, {.direction = -1}, {.direction = 1, .length = 94}, {.direction = 3, .length = 48}, {.direction = -1}, {.direction = 1, .length = 22}, {.direction = 3, .length = 46}, {.direction = -1}, {.direction = 1, .length = 32}, {.direction = 2, .length = 24}, {.direction = -1}, {.length = 48}, {.direction = 3, .length = 68}, {.direction = -1}, {.length = 24}, {.direction = 3, .length = 82}, {.direction = -1}, {.length = 24}, {.direction = 3, .length = 10}, {.direction = -1}, {.length = 12}, {.direction = 3, .length = 26}, {.length = 38}, {.direction = -1}};
const StrokeStep4 *ovl665_callout_paths_type0[8][5] = {{(const StrokeStep4 *)&ovl665_callout_steps_type0[0], (const StrokeStep4 *)&ovl665_callout_steps_type0[6], (const StrokeStep4 *)&ovl665_callout_steps_type0[12], (const StrokeStep4 *)&ovl665_callout_steps_type0[15], (const StrokeStep4 *)&ovl665_callout_steps_type0[21]}, {(const StrokeStep4 *)&ovl665_callout_steps_type0[3], (const StrokeStep4 *)&ovl665_callout_steps_type0[9], (const StrokeStep4 *)&ovl665_callout_steps_type0[12], (const StrokeStep4 *)&ovl665_callout_steps_type0[15], (const StrokeStep4 *)&ovl665_callout_steps_type0[21]}, {(const StrokeStep4 *)&ovl665_callout_steps_type0[0], (const StrokeStep4 *)&ovl665_callout_steps_type0[6], (const StrokeStep4 *)&ovl665_callout_steps_type0[12], (const StrokeStep4 *)&ovl665_callout_steps_type0[15], (const StrokeStep4 *)&ovl665_callout_steps_type0[21]}, {(const StrokeStep4 *)&ovl665_callout_steps_type0[3], (const StrokeStep4 *)&ovl665_callout_steps_type0[9], (const StrokeStep4 *)&ovl665_callout_steps_type0[12], (const StrokeStep4 *)&ovl665_callout_steps_type0[15], (const StrokeStep4 *)&ovl665_callout_steps_type0[21]}, {(const StrokeStep4 *)&ovl665_callout_steps_type0[0], (const StrokeStep4 *)&ovl665_callout_steps_type0[6], (const StrokeStep4 *)&ovl665_callout_steps_type0[12], (const StrokeStep4 *)&ovl665_callout_steps_type0[18], (const StrokeStep4 *)&ovl665_callout_steps_type0[24]}, {(const StrokeStep4 *)&ovl665_callout_steps_type0[3], (const StrokeStep4 *)&ovl665_callout_steps_type0[9], (const StrokeStep4 *)&ovl665_callout_steps_type0[12], (const StrokeStep4 *)&ovl665_callout_steps_type0[18], (const StrokeStep4 *)&ovl665_callout_steps_type0[24]}, {(const StrokeStep4 *)&ovl665_callout_steps_type0[0], (const StrokeStep4 *)&ovl665_callout_steps_type0[6], (const StrokeStep4 *)&ovl665_callout_steps_type0[12], (const StrokeStep4 *)&ovl665_callout_steps_type0[18], (const StrokeStep4 *)&ovl665_callout_steps_type0[24]}, {(const StrokeStep4 *)&ovl665_callout_steps_type0[3], (const StrokeStep4 *)&ovl665_callout_steps_type0[9], (const StrokeStep4 *)&ovl665_callout_steps_type0[12], (const StrokeStep4 *)&ovl665_callout_steps_type0[18], (const StrokeStep4 *)&ovl665_callout_steps_type0[24]}};
StrokeStep4 ovl665_callout_steps_type1[46] = {{.direction = 1, .length = 118}, {.direction = 3, .length = 16}, {.direction = -1}, {.direction = 1, .length = 84}, {.direction = 3, .length = 16}, {.direction = -1}, {.direction = 1, .length = 40}, {.direction = 3, .length = 12}, {.direction = -1}, {.direction = 1, .length = 84}, {.direction = 2, .length = 54}, {.direction = -1}, {.direction = 1, .length = 40}, {.direction = 2, .length = 60}, {.direction = -1}, {.direction = 1, .length = 70}, {.direction = 3, .length = 48}, {.direction = -1}, {.direction = 1, .length = 98}, {.direction = 2, .length = 10}, {.direction = -1}, {.direction = 1, .length = 70}, {.direction = 2, .length = 22}, {.direction = -1}, {.direction = 1, .length = 40}, {.direction = 2, .length = 12}, {.direction = -1}, {.length = 68}, {.direction = 3, .length = 68}, {.direction = -1}, {.length = 50}, {.direction = 2, .length = 20}, {.direction = -1}, {.length = 38}, {.direction = 3, .length = 10}, {.direction = -1}, {.length = 66}, {.direction = 2, .length = 2}, {.direction = -1}, {.length = 16}, {.direction = 3, .length = 26}, {.length = 52}, {.direction = -1}, {.length = 98}, {.direction = 3, .length = 12}, {.direction = -1}};
const StrokeStep4 *ovl665_callout_paths_type1[8][5] = {{(const StrokeStep4 *)&ovl665_callout_steps_type1[0], (const StrokeStep4 *)&ovl665_callout_steps_type1[9], (const StrokeStep4 *)&ovl665_callout_steps_type1[21], (const StrokeStep4 *)&ovl665_callout_steps_type1[27], (const StrokeStep4 *)&ovl665_callout_steps_type1[33]}, {(const StrokeStep4 *)&ovl665_callout_steps_type1[0], (const StrokeStep4 *)&ovl665_callout_steps_type1[9], (const StrokeStep4 *)&ovl665_callout_steps_type1[21], (const StrokeStep4 *)&ovl665_callout_steps_type1[27], (const StrokeStep4 *)&ovl665_callout_steps_type1[33]}, {(const StrokeStep4 *)&ovl665_callout_steps_type1[3], (const StrokeStep4 *)&ovl665_callout_steps_type1[12], (const StrokeStep4 *)&ovl665_callout_steps_type1[24], (const StrokeStep4 *)&ovl665_callout_steps_type1[30], (const StrokeStep4 *)&ovl665_callout_steps_type1[43]}, {(const StrokeStep4 *)&ovl665_callout_steps_type1[3], (const StrokeStep4 *)&ovl665_callout_steps_type1[12], (const StrokeStep4 *)&ovl665_callout_steps_type1[21], (const StrokeStep4 *)&ovl665_callout_steps_type1[30], (const StrokeStep4 *)&ovl665_callout_steps_type1[36]}, {(const StrokeStep4 *)&ovl665_callout_steps_type1[3], (const StrokeStep4 *)&ovl665_callout_steps_type1[15], (const StrokeStep4 *)&ovl665_callout_steps_type1[18], (const StrokeStep4 *)&ovl665_callout_steps_type1[30], (const StrokeStep4 *)&ovl665_callout_steps_type1[36]}, {(const StrokeStep4 *)&ovl665_callout_steps_type1[6], (const StrokeStep4 *)&ovl665_callout_steps_type1[15], (const StrokeStep4 *)&ovl665_callout_steps_type1[24], (const StrokeStep4 *)&ovl665_callout_steps_type1[27], (const StrokeStep4 *)&ovl665_callout_steps_type1[33]}, {(const StrokeStep4 *)&ovl665_callout_steps_type1[6], (const StrokeStep4 *)&ovl665_callout_steps_type1[15], (const StrokeStep4 *)&ovl665_callout_steps_type1[24], (const StrokeStep4 *)&ovl665_callout_steps_type1[27], (const StrokeStep4 *)&ovl665_callout_steps_type1[33]}, {(const StrokeStep4 *)&ovl665_callout_steps_type1[6], (const StrokeStep4 *)&ovl665_callout_steps_type1[15], (const StrokeStep4 *)&ovl665_callout_steps_type1[24], (const StrokeStep4 *)&ovl665_callout_steps_type1[27], (const StrokeStep4 *)&ovl665_callout_steps_type1[39]}};
StrokeStep4 ovl665_callout_steps_type2[52] = {{.direction = 1, .length = 42}, {.direction = 3, .length = 18}, {.direction = -1}, {.direction = 1, .length = 32}, {.direction = 3, .length = 18}, {.direction = -1}, {.direction = 1, .length = 94}, {.direction = 3, .length = 120}, {.direction = -1}, {.direction = 1, .length = 94}, {.direction = 3, .length = 48}, {.direction = -1}, {.direction = 1, .length = 32}, {.direction = 3, .length = 52}, {.direction = -1}, {.direction = 1, .length = 98}, {.direction = 2, .length = 50}, {.direction = -1}, {.direction = 1, .length = 42}, {.direction = 2, .length = 52}, {.direction = -1}, {.direction = 1, .length = 42}, {.direction = 2, .length = 18}, {.direction = -1}, {.direction = 1, .length = 94}, {.direction = 2, .length = 22}, {.direction = -1}, {.length = 50}, {.direction = 3, .length = 70}, {.direction = -1}, {.length = 26}, {.direction = 3, .length = 84}, {.direction = -1}, {.length = 36}, {.direction = 2, .length = 14}, {.direction = -1}, {.length = 126}, {.direction = 2, .length = 16}, {.direction = -1}, {.length = 26}, {.direction = 3, .length = 12}, {.direction = -1}, {.length = 12}, {.direction = 3, .length = 28}, {.length = 38}, {.direction = -1}, {.length = 74}, {.direction = 3, .length = 12}, {.direction = -1}, {.length = 68}, {.direction = 2, .length = 86}, {.direction = -1}};
const StrokeStep4 *ovl665_callout_paths_type2[8][5] = {{(const StrokeStep4 *)&ovl665_callout_steps_type2[0], (const StrokeStep4 *)&ovl665_callout_steps_type2[9], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[27], (const StrokeStep4 *)&ovl665_callout_steps_type2[39]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[6], (const StrokeStep4 *)&ovl665_callout_steps_type2[12], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[27], (const StrokeStep4 *)&ovl665_callout_steps_type2[39]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[0], (const StrokeStep4 *)&ovl665_callout_steps_type2[9], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[27], (const StrokeStep4 *)&ovl665_callout_steps_type2[39]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[6], (const StrokeStep4 *)&ovl665_callout_steps_type2[12], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[27], (const StrokeStep4 *)&ovl665_callout_steps_type2[39]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[0], (const StrokeStep4 *)&ovl665_callout_steps_type2[9], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[30], (const StrokeStep4 *)&ovl665_callout_steps_type2[42]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[6], (const StrokeStep4 *)&ovl665_callout_steps_type2[12], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[30], (const StrokeStep4 *)&ovl665_callout_steps_type2[42]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[0], (const StrokeStep4 *)&ovl665_callout_steps_type2[9], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[30], (const StrokeStep4 *)&ovl665_callout_steps_type2[42]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[6], (const StrokeStep4 *)&ovl665_callout_steps_type2[12], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[30], (const StrokeStep4 *)&ovl665_callout_steps_type2[42]}};
const StrokeStep4 *ovl665_callout_paths_type3[8][5] = {{(const StrokeStep4 *)&ovl665_callout_steps_type2[0], (const StrokeStep4 *)&ovl665_callout_steps_type2[15], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[33], (const StrokeStep4 *)&ovl665_callout_steps_type2[46]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[0], (const StrokeStep4 *)&ovl665_callout_steps_type2[9], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[27], (const StrokeStep4 *)&ovl665_callout_steps_type2[39]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[3], (const StrokeStep4 *)&ovl665_callout_steps_type2[12], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[36], (const StrokeStep4 *)&ovl665_callout_steps_type2[46]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[3], (const StrokeStep4 *)&ovl665_callout_steps_type2[12], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[36], (const StrokeStep4 *)&ovl665_callout_steps_type2[46]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[3], (const StrokeStep4 *)&ovl665_callout_steps_type2[18], (const StrokeStep4 *)&ovl665_callout_steps_type2[24], (const StrokeStep4 *)&ovl665_callout_steps_type2[33], (const StrokeStep4 *)&ovl665_callout_steps_type2[49]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[6], (const StrokeStep4 *)&ovl665_callout_steps_type2[12], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[27], (const StrokeStep4 *)&ovl665_callout_steps_type2[39]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[6], (const StrokeStep4 *)&ovl665_callout_steps_type2[12], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[27], (const StrokeStep4 *)&ovl665_callout_steps_type2[39]}, {(const StrokeStep4 *)&ovl665_callout_steps_type2[0], (const StrokeStep4 *)&ovl665_callout_steps_type2[9], (const StrokeStep4 *)&ovl665_callout_steps_type2[21], (const StrokeStep4 *)&ovl665_callout_steps_type2[27], (const StrokeStep4 *)&ovl665_callout_steps_type2[39]}};
Ovl665View14 ovl665_controller_camera_poses[7] = {{.x00 = 160, .y02 = 287, .words04 = {1024, 500, -290, 7000}}, {.x00 = 160, .y02 = 287, .words04 = {1024, 500, -290, 7000}}, {.x00 = 160, .y02 = 294, .words04 = {1024, 500, 0, 7000}}, {.x00 = 160, .y02 = 358, .words04 = {1024, 500, -440, 7000}}, {.x00 = 160, .y02 = 190, .words04 = {1024, 500, 0, 7000}}, {.x00 = 160, .y02 = 190, .words04 = {1024, 500, -1024, 7000}}, {.x00 = 160, .y02 = 220, .words04 = {1024, 556, 0, 7000}}};
s32 ovl665_controller_model_scales[6] = {5, 8, 11, 5, 8, 11};
u8 ovl665_twist_needle_color[4] = {0x40, 0x80, 0xFF};
s32 ovl665_controller_row_counts[5] = {0, 6, 10, 14};
s32 ovl665_saved_stereo_mode;
Ovl665CarState ovl665_options_state;

/* Writable state of this unit; ovl665_reset_module restores it. */
static const OverlayStateRegion ovl665_controller_options_regions[] = {
    RR_OVERLAY_STATE(ovl665_callout_paths_type0),
    RR_OVERLAY_STATE(ovl665_callout_paths_type1),
    RR_OVERLAY_STATE(ovl665_callout_paths_type2),
    RR_OVERLAY_STATE(ovl665_callout_paths_type3),
    RR_OVERLAY_STATE(ovl665_callout_steps_type0),
    RR_OVERLAY_STATE(ovl665_callout_steps_type1),
    RR_OVERLAY_STATE(ovl665_callout_steps_type2),
    RR_OVERLAY_STATE(ovl665_controller_camera_poses),
    RR_OVERLAY_STATE(ovl665_controller_colors_type0),
    RR_OVERLAY_STATE(ovl665_controller_colors_type1),
    RR_OVERLAY_STATE(ovl665_controller_colors_type2),
    RR_OVERLAY_STATE(ovl665_controller_model_scales),
    RR_OVERLAY_STATE(ovl665_controller_row_counts),
    RR_OVERLAY_STATE(ovl665_options_state),
    RR_OVERLAY_STATE(ovl665_saved_stereo_mode),
    RR_OVERLAY_STATE(ovl665_screen_arrow_uv_rects),
    RR_OVERLAY_STATE(ovl665_twist_needle_color)};
const OverlayStateTable ovl665_controller_options_state = {ovl665_controller_options_regions, sizeof(ovl665_controller_options_regions) / sizeof(ovl665_controller_options_regions[0])};

/* Initial values of the globals this module owns. */
s32 WheelDeadzoneTable[4] = {0, 4, 6, 8};
RuntimeSettings Options;
