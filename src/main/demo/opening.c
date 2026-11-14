#include "rr/video_timing.h"
#include "game/system/pad.h"
#include "types.h"
#include "scratchpad.h"
#include "rr/line_emit.h"
#include "game/ui/hudmap2.h"
#include "game/course/trktext.h"
#include "psyq/libc/printf.h"
#include "game/demo/opening.h"
#include "rr/audio_state_types.h"
#include "rr/menu_overlay_runtime.h"
#include "game/gfx/gfxotag.h"

extern const char StrOpeningIn[];
extern const char StrOpeningBacktick[];
extern const char StrOpeningApostrophe[];
extern const char StrFmtTwoDigitsOpening[];
extern const char StrRandomPlay[];
extern char CourseNames[];
extern u8 HudSpeedUnitSprites[];
extern u8 HudViewModeIconUvs[];
extern u8 HudLapSlashSprite[];
extern u32 HudSplitSignSprites[];
extern u32 HudRecordCrownSprites[];
extern s16 HudTachRedlineAngles[];
extern s16 HudTachSweepLimitAngles[];
extern s16 *HudRankOrdinalUvs[];
extern s16 *HudTimeLabelSprites[];
extern s16 *HudBigDigitSprites[];

s32 UpdateRaceOpeningOverlay(s32 arg0);
void EmitProjectedMarkerSprt(int a0, int a1, int a2);

/* NTSC timing: the frames each banner element starts fading in at (the
 * NTSC release's HudRecordCrownSprites; its HudSplitSignSprites is the same
 * list without the 60, an element the PAL banner adds). */
static const u32 OpeningElementStartsNtsc[6] = {6, 60, 120, 180, 240, 300};

/* The Japanese release's race opening (SLPS-01800). The race banner fades
 * in five elements (date and time, stage, rank and attempt, the icon row,
 * the time labels, then the music line) and draws its stage and time
 * labels as glyph lists instead of the PAL release's course name and
 * sloped attempt line; the mode banner adds the course number and two
 * labels. */
static const u32 OpeningElementStartsJp[11] = {6, 120, 180, 240, 300, 6, 60, 120, 180, 240, 300};

static s32 OpeningRampJp(s32 count, const u32 *starts, s32 arg0, s32 *fade, s32 firstQuick) {
    s32 i, t;
    for (i = 0; i < count; i++) {
        if (starts[i] < (u32)arg0) {
            t = arg0 - starts[i];
            if (firstQuick && i == 0) {
                fade[i] = t < 15 ? (t << 7) / 15 : 0x80;
            } else {
                fade[i] = t < 30 ? (t << 7) / 30 : 0x80;
            }
        } else {
            fade[i] = 0;
        }
        if (OpeningFadeTimer >= 0) {
            fade[i] = (OpeningFadeTimer << 7) / 120;
            if (OpeningFadeTimer > 0) {
                OpeningFadeTimer--;
            } else {
                return 1;
            }
        }
    }
    return 0;
}

static void OpeningMusicLineJp(s32 brightness, char *text) {
    AddBrightnessSprite(HudSpeedUnitSprites, 0x3C, 0xC8, brightness);
    AddBrightnessSprite(HudSpeedUnitSprites + 4, 0xFC, 0xC8, brightness);
    AddBrightnessSprite(HudViewModeIconUvs, 0x4C, 0xC8, brightness);
    PsyqSprintf((s8 *)text, (const s8 *)StrFmtTwoDigitsOpening, SoundState.track28A4);
    DrawFontString(0x5A, 0xC8, (const u8 *)text, brightness, 0);
    AddBrightnessSprite(HudLapSlashSprite, 0x6C, 0xC8, brightness);
    if (SoundState.track28A4 > 0) {
        DrawFontString(0x78, 0xC8, (const u8 *)(CD_TRACK_TITLE(BgmNumberToCdTrack[SoundState.track28A4 - 1])), brightness, 0);
    } else {
        DrawFontString(0x78, 0xC8, (const u8 *)(StrRandomPlay), brightness, 0);
    }
}

static s32 UpdateRaceOpeningOverlayJp(s32 arg0) {
    s32 fade[6];
    char text[16];
    s16 line[4];
    u8 rgb[8];
    s32 done = 0;
    s32 mode, half;

    if (arg0 < 0) {
        arg0 = 0;
    }
    if (arg0 >= 0x21D) {
        done = ++OpeningSlideTimer >= 0x5B;
        if (arg0 >= 0xCA9) {
            done = 1;
        }
        arg0 = 0x21C;
    }
    if (OpeningBannerMode < 8) {
        if (OpeningRampJp(5, OpeningElementStartsJp, arg0, fade, 0)) {
            return 1;
        }
        mode = OpeningBannerMode;
        DrawGlyphList(OpeningDateGlyphLists[mode], fade[0], 0);
        DrawGlyphList(OpeningTimeGlyphLists[mode], fade[0], 0);
        DrawGlyphList(OpeningStageGlyphLists[mode], fade[1], 0);
        DrawGlyphList(HudRankOrdinalUvs[mode], fade[1], 0);
        DrawGlyphList(OpeningAttemptGlyphLists[OpeningAttemptIndex], fade[1], 0);
        EmitIconSpriteRow120(0, (u8)fade[2]);
        DrawGlyphList(HudTimeLabelSprites[mode], fade[3], 1);
        DrawGlyphList(HudTimeLabelSprites[mode] + 8, fade[3], 1);
        DrawGlyphList(HudTimeLabelSprites[mode] + 14, fade[3], 1);
        if (fade[4] >= 0x80) {
            if (Pad0Buttons & 0x8000) {
                if (OpeningBgmSelectState == 0) {
                    SoundState.track28A4 = 0;
                    OpeningBgmSelectState = 1;
                } else if (--SoundState.track28A4 < 0) {
                    SoundState.track28A4 = SoundState.count28A8 - 1;
                }
                OpeningSlideTimer = 0;
            }
            if (Pad0Buttons & 0x2000) {
                if (OpeningBgmSelectState == 0) {
                    SoundState.track28A4 = 0;
                    OpeningBgmSelectState = 1;
                } else if (++SoundState.track28A4 >= SoundState.count28A8) {
                    SoundState.track28A4 = 0;
                }
                OpeningSlideTimer = 0;
            }
            if (MenuReturnSubmodeIndex != 0) {
                done = 1;
            }
        }
        OpeningMusicLineJp(fade[4], text);
        AddSetTexpagePrim(0);
        return done;
    }
    if (OpeningRampJp(6, OpeningElementStartsJp + 5, arg0, fade, 1)) {
        return 1;
    }
    mode = OpeningBannerMode - 8;
    DrawGlyphList(OpeningModeGlyphLists[mode], fade[1], 0);
    line[0] = 0x48;
    line[1] = 0x38;
    line[3] = 0x38;
    if (OpeningFadeTimer < 0) {
        line[2] = (fade[0] * 0xB0) / 128 + 0x47;
        rgb[0] = rgb[1] = rgb[2] = 0x80;
    } else {
        line[2] = 0xF7;
        rgb[0] = rgb[1] = rgb[2] = fade[0];
    }
    if (line[0] < line[2]) {
        AddLineF2Prim(line, rgb, 1, 0);
        line[1] = 0x37;
        line[3] = 0x37;
        rgb[0] = rgb[1] = rgb[2] = rgb[0] >> 1;
        AddLineF2Prim(line, rgb, 1, 0);
        line[1] = 0x39;
        line[3] = 0x39;
        AddLineF2Prim(line, rgb, 1, 0);
    }
    DrawFontString(0x98, 0x48, (const u8 *)(StrOpeningIn), fade[2], 0);
    half = DrawFontStringCentered(0x60, 0x58, CourseNames + CourseIndex * 0x18, fade[2], 0) / 2;
    DrawFontString(0x9A - half, 0x58, (const u8 *)(StrOpeningBacktick), fade[2], 0);
    DrawFontString(0xA2 + half, 0x58, (const u8 *)(StrOpeningApostrophe), fade[2], 0);
    AddSetTexpagePrim(0);
    EmitIconSpriteRow120(mode < 3 ? 1 : mode - 1, (u8)fade[3]);
    DrawGlyphList(HudBigDigitSprites[CourseIndex], fade[4], 1);
    DrawGlyphList(HudTachRedlineAngles, fade[4], 1);
    DrawGlyphList(HudTachSweepLimitAngles, fade[4], 1);
    if (OpeningBgmSelectState >= 0) {
        if (fade[5] >= 0x80) {
            if (Pad0Buttons & 0x8000) {
                if (--SoundState.track28A4 < 0) {
                    SoundState.track28A4 = SoundState.count28A8 - 1;
                }
                OpeningSlideTimer = 0;
            }
            if (Pad0Buttons & 0x2000) {
                if (++SoundState.track28A4 >= SoundState.count28A8) {
                    SoundState.track28A4 = 0;
                }
                OpeningSlideTimer = 0;
            }
            if (MenuReturnSubmodeIndex != 0) {
                done = 1;
            }
        }
        SoundState.displayed28AC = SoundState.track28A4;
        OpeningMusicLineJp(fade[5], text);
    } else {
        half = 0x88 - DrawFontStringCenteredOffset(0x14, 0xC8, CD_TRACK_TITLE(BgmNumberToCdTrack[SoundState.track28A4 - 1]), fade[5], 0) / 2;
        AddBrightnessSprite(HudViewModeIconUvs, half, 0xC8, fade[5]);
        PsyqSprintf((s8 *)text, (const s8 *)StrFmtTwoDigitsOpening, SoundState.track28A4);
        DrawFontString(half + 0xE, 0xC8, (const u8 *)(text), fade[5], 0);
        AddBrightnessSprite(HudLapSlashSprite, half + 0x20, 0xC8, fade[5]);
        if (fade[5] >= 0x80 && MenuReturnSubmodeIndex != 0) {
            done = 1;
        }
    }
    AddSetTexpagePrim(0);
    return done;
}

s32 UpdateRaceOpeningOverlay(s32 arg0) {
    if (RR_RELEASE_JP)
        return UpdateRaceOpeningOverlayJp(arg0);
    /* Frames of a second (25, NTSC release: 30). */
    const s32 rate = RR_PAL_NTSC(25, 30);
    const u32 *starts;
    s32 sp18[6];
    char sp30[16];
    s16 sp40[4];
    u8 sp48[8];
    s16 sp50[4];
    u8 sp58[8];
    s32 i;
    s32 s2;
    s32 s4;
    s32 s0_1;
    s32 s0_2;
    s32 s0_3;

    s2 = OpeningBannerMode;
    s4 = 0;
    if (arg0 < 0) {
        arg0 = 0;
    }
    if (arg0 >= RR_PAL_NTSC(0x1C3, 0x21D)) {
        s4 = ++OpeningSlideTimer >= RR_PAL_NTSC(0x4C, 0x5B);
        if (arg0 >= RR_PAL_NTSC(0xA8D, 0xCA9)) {
            s4 = 1;
        }
        arg0 = RR_PAL_NTSC(0x1C2, 0x21C);
    }

    if (s2 < 8) {
        starts = VideoTimingNtsc ? OpeningElementStartsNtsc : HudSplitSignSprites;
        for (i = 0; i < 6; i++) {
            if (starts[i] < arg0) {
                sp18[i] = arg0 - starts[i];
                if (sp18[i] < rate) {
                    sp18[i] = (sp18[i] * 0x80) / rate;
                } else {
                    sp18[i] = 0x80;
                }
            } else {
                sp18[i] = 0;
            }
            if (OpeningFadeTimer >= 0) {
                sp18[i] = (OpeningFadeTimer * 0x80) / RR_PAL_NTSC(100, 120);
                if (OpeningFadeTimer > 0) {
                    OpeningFadeTimer--;
                } else {
                    return 1;
                }
            }
        }

        sp40[0] = 0x48;
        sp40[1] = 0x48;
        sp40[3] = 0x48;
        if (OpeningFadeTimer < 0) {
            sp40[2] = (sp18[0] * 0xB0) / 128 + 0x47;
            sp48[2] = 0x80;
            sp48[1] = 0x80;
            sp48[0] = 0x80;
        } else {
            sp40[2] = 0xF7;
            sp48[0] = sp48[1] = sp48[2] = sp18[0];
        }
        if (sp40[2] > sp40[0]) {
            AddLineF2Prim(sp40, sp48, 1, 0);
            sp40[1] = 0x47;
            sp40[3] = 0x47;
            sp48[0] = sp48[1] = sp48[2] = sp48[0] >> 1;
            AddLineF2Prim(sp40, sp48, 1, 0);
            sp40[1] = 0x49;
            sp40[3] = 0x49;
            AddLineF2Prim(sp40, sp48, 1, 0);
        }

        DrawGlyphList(OpeningDateGlyphLists[s2], sp18[1], 0);
        DrawGlyphList(OpeningTimeGlyphLists[s2], sp18[1], 0);
        DrawGlyphList(OpeningStageGlyphLists[s2], sp18[2], 0);
        DrawGlyphListSloped(OpeningAttemptGlyphLists[OpeningAttemptIndex], sp18[2], 0);
        EmitIconSpriteRow128(0, (u8)(sp18[3]));
        DrawFontString(0x9A, 0xA0, (const u8 *)(StrOpeningIn), sp18[4], 0);
        s0_1 = DrawFontStringCentered(0x60, 0xB0, CourseNames + CourseIndex * 0x18, sp18[4], 0) / 2;
        DrawFontString(0x9A - s0_1, 0xB0, (const u8 *)(StrOpeningBacktick), sp18[4], 0);
        DrawFontString(0xA2 + s0_1, 0xB0, (const u8 *)(StrOpeningApostrophe), sp18[4], 0);
        AddSetTexpagePrim(0);

        if (sp18[5] >= 0x80) {
            if (Pad0Buttons & 0x8000) {
                if (OpeningBgmSelectState == 0) {
                    SoundState.track28A4 = 0;
                    OpeningBgmSelectState = 1;
                } else {
                    SoundState.track28A4--;
                    if (SoundState.track28A4 < 0) {
                        SoundState.track28A4 = SoundState.count28A8 - 1;
                    }
                }
                OpeningSlideTimer = 0;
            }
            if (Pad0Buttons & 0x2000) {
                if (OpeningBgmSelectState == 0) {
                    SoundState.track28A4 = 0;
                    OpeningBgmSelectState = 1;
                } else {
                    SoundState.track28A4++;
                    if (SoundState.track28A4 >= SoundState.count28A8) {
                        SoundState.track28A4 = 0;
                    }
                }
                OpeningSlideTimer = 0;
            }
            if (MenuReturnSubmodeIndex != 0) {
                s4 = 1;
            }
        }

        AddBrightnessSprite(HudSpeedUnitSprites, 0x3C, 0xC8, sp18[5]);
        AddBrightnessSprite(HudSpeedUnitSprites + 4, 0xFC, 0xC8, sp18[5]);
        AddBrightnessSprite(HudViewModeIconUvs, 0x4C, 0xC8, sp18[5]);
        PsyqSprintf((s8 *)sp30, (const s8 *)StrFmtTwoDigitsOpening, SoundState.track28A4);
        DrawFontString(0x5A, 0xC8, (const u8 *)(sp30), sp18[5], 0);
        AddBrightnessSprite(HudLapSlashSprite, 0x6C, 0xC8, sp18[5]);
        if (SoundState.track28A4 <= 0) {
            DrawFontString(0x78, 0xC8, (const u8 *)(StrRandomPlay), sp18[5], 0);
        } else {
            DrawFontString(0x78, 0xC8, (const u8 *)(CD_TRACK_TITLE(BgmNumberToCdTrack[SoundState.track28A4 - 1])), sp18[5], 0);
        }
        AddSetTexpagePrim(0);
        return s4;
    } else {
        starts = VideoTimingNtsc ? OpeningElementStartsNtsc : HudRecordCrownSprites;
        for (i = 0; i < 6; i++) {
            if (starts[i] < arg0) {
                sp18[i] = arg0 - starts[i];
                if (i != 0) {
                    if (sp18[i] < rate) {
                        sp18[i] = (sp18[i] * 0x80) / rate;
                    } else {
                        sp18[i] = 0x80;
                    }
                } else {
                    if (sp18[0] < RR_PAL_NTSC(0xC, 0xF)) {
                        sp18[0] = (sp18[0] * 0x80) / RR_PAL_NTSC(12, 15);
                    } else {
                        sp18[0] = 0x80;
                    }
                }
            } else {
                sp18[i] = 0;
            }
            if (OpeningFadeTimer >= 0) {
                sp18[i] = (OpeningFadeTimer * 0x80) / RR_PAL_NTSC(100, 120);
                if (OpeningFadeTimer > 0) {
                    OpeningFadeTimer--;
                } else {
                    return 1;
                }
            }
        }

        s2 -= 8;
        DrawGlyphList(OpeningModeGlyphLists[s2], sp18[1], 0);

        sp50[0] = 0x48;
        sp50[1] = 0x38;
        sp50[3] = 0x38;
        if (OpeningFadeTimer < 0) {
            sp50[2] = (sp18[0] * 0xB0) / 128 + 0x47;
            sp58[2] = 0x80;
            sp58[1] = 0x80;
            sp58[0] = 0x80;
        } else {
            sp50[2] = 0xF7;
            sp58[0] = sp58[1] = sp58[2] = sp18[0];
        }
        if (sp50[2] > sp50[0]) {
            AddLineF2Prim(sp50, sp58, 1, 0);
            sp50[1] = 0x37;
            sp50[3] = 0x37;
            sp58[0] = sp58[1] = sp58[2] = sp58[0] >> 1;
            AddLineF2Prim(sp50, sp58, 1, 0);
            sp50[1] = 0x39;
            sp50[3] = 0x39;
            AddLineF2Prim(sp50, sp58, 1, 0);
        }

        DrawFontString(0x9A, 0x48, (const u8 *)(StrOpeningIn), sp18[2], 0);
        s0_2 = DrawFontStringCentered(0x60, 0x58, CourseNames + CourseIndex * 0x18, sp18[2], 0) / 2;
        DrawFontString(0x9A - s0_2, 0x58, (const u8 *)(StrOpeningBacktick), sp18[2], 0);
        DrawFontString(0xA2 + s0_2, 0x58, (const u8 *)(StrOpeningApostrophe), sp18[2], 0);
        AddSetTexpagePrim(0);

        if (s2 < 3) {
            EmitIconSpriteRow120(1, (u8)(sp18[3]));
        } else {
            EmitIconSpriteRow120(s2 - 1, (u8)(sp18[3]));
        }

        if (OpeningBgmSelectState >= 0) {
            if (sp18[5] >= 0x80) {
                if (Pad0Buttons & 0x8000) {
                    SoundState.track28A4--;
                    if (SoundState.track28A4 < 0) {
                        SoundState.track28A4 = SoundState.count28A8 - 1;
                    }
                    OpeningSlideTimer = 0;
                }
                if (Pad0Buttons & 0x2000) {
                    SoundState.track28A4++;
                    if (SoundState.track28A4 >= SoundState.count28A8) {
                        SoundState.track28A4 = 0;
                    }
                    OpeningSlideTimer = 0;
                }
                if (MenuReturnSubmodeIndex != 0) {
                    s4 = 1;
                }
            }
            SoundState.displayed28AC = SoundState.track28A4;
            AddBrightnessSprite(HudSpeedUnitSprites, 0x3C, 0xC8, sp18[5]);
            AddBrightnessSprite(HudSpeedUnitSprites + 4, 0xFC, 0xC8, sp18[5]);
            AddBrightnessSprite(HudViewModeIconUvs, 0x4C, 0xC8, sp18[5]);
            PsyqSprintf((s8 *)sp30, (const s8 *)StrFmtTwoDigitsOpening, SoundState.track28A4);
            DrawFontString(0x5A, 0xC8, (const u8 *)(sp30), sp18[5], 0);
            AddBrightnessSprite(HudLapSlashSprite, 0x6C, 0xC8, sp18[5]);
            if (SoundState.track28A4 > 0) {
                DrawFontString(0x78, 0xC8, (const u8 *)(CD_TRACK_TITLE(BgmNumberToCdTrack[SoundState.track28A4 - 1])), sp18[5], 0);
            } else {
                DrawFontString(0x78, 0xC8, (const u8 *)(StrRandomPlay), sp18[5], 0);
            }
        } else {
            s0_3 = 0x88 - DrawFontStringCenteredOffset(0x14, 0xC8, CD_TRACK_TITLE(BgmNumberToCdTrack[SoundState.track28A4 - 1]), sp18[5], 0) / 2;
            AddBrightnessSprite(HudViewModeIconUvs, s0_3, 0xC8, sp18[5]);
            PsyqSprintf((s8 *)sp30, (const s8 *)StrFmtTwoDigitsOpening, SoundState.track28A4);
            DrawFontString(s0_3 + 0xE, 0xC8, (const u8 *)(sp30), sp18[5], 0);
            AddBrightnessSprite(HudLapSlashSprite, s0_3 + 0x20, 0xC8, sp18[5]);
            if (sp18[5] >= 0x80 && MenuReturnSubmodeIndex != 0) {
                s4 = 1;
            }
        }
    }

    AddSetTexpagePrim(0);
    return s4;
}

void StartOpeningFadeOut(void) {
    OpeningFadeTimer = RR_PAL_NTSC(0x64, 0x78);
}

void EmitProjectedMarkerSprt(int a0, int a1, int a2)
{
    GpuScratchPointers *scratch = RR_GPU_SCRATCH;
    SpritePacket *p = (SpritePacket *)scratch->cursor;
    p->code = 0x65;
    p->w = 8;
    p->h = 0x10;
    p->u = 0x3C;
    p->v = 0x48;
    p->x = a0 - 4;
    p->y = a1 - 8;
    p->clut = 0x1B96;
    RR_GPU_ADD_PRIM(&scratch->order[a2], p, 4);
    scratch->cursor = (u8 *)(p + 1);
}

void EmitOpeningDrawModePrim(void)
{
    GpuScratchPointers *scratch = RR_GPU_SCRATCH;
    GpuCommandPacket *p = (GpuCommandPacket *)scratch->cursor;
    p->command = 0xE1000205;
    RR_GPU_ADD_PRIM(&scratch->order[703], p, 1);
    scratch->cursor = (u8 *)(p + 1);
}

/* Initial values of the globals this module owns. */
short D_800A11F8[20] = {80, 32, 1051, 2, 0, 0, 1, 5, 1058, 2, 1, 4, 9, 2, 9, 2, 9, 2, -1};
short D_800A1220[20] = {80, 32, 1051, 1, 1, 4, 5, 3, 1058, 2, 1, 4, 9, 2, 9, 2, 9, 2, -1};
short D_800A1248[20] = {80, 32, 1052, 2, 0, 0, 1, 5, 1059, 2, 1, 4, 9, 2, 9, 2, 9, 2, -1};
short D_800A1270[20] = {80, 32, 1052, 1, 1, 4, 5, 3, 1059, 2, 1, 4, 9, 2, 9, 2, 9, 2, -1};
short D_800A1298[20] = {80, 32, 1053, 3, 2, 1, 4, 3, 1060, 2, 1, 4, 9, 2, 9, 2, 9, 2, -1};
short D_800A12C0[20] = {80, 32, 1053, 3, 2, 1, 2, 4, 1061, 2, 1, 4, 9, 2, 9, 2, 9, 2, -1};
short D_800A12E8[20] = {80, 32, 1053, 3, 2, 1, 6, 4, 1062, 2, 1, 4, 9, 2, 9, 2, 9, 2, -1};
short D_800A1310[20] = {80, 32, 1053, 3, 3, 0, 1, 5, 1063, 2, 1, 4, 9, 2, 9, 2, 9, 2, -1};
short D_800A1338[16] = {116, 52, 1, 4, 1, 4, 22, 2, 3, 2, 0, 0, 20, 0, -1};
short D_800A1358[16] = {116, 52, 0, 2, 5, 2, 22, 2, 3, 2, 0, 2, 21, 0, -1};
short D_800A1378[16] = {116, 52, 0, 2, 8, 2, 22, 2, 4, 2, 5, 2, 21, 0, -1};
short D_800A1398[16] = {116, 52, 0, 2, 6, 2, 22, 2, 3, 2, 0, 0, 20, 0, -1};
short D_800A13B8[16] = {116, 52, 1, 4, 2, 2, 22, 2, 0, 2, 0, 2, 21, 0, -1};
short D_800A13D8[16] = {116, 52, 0, 2, 9, 2, 22, 2, 4, 2, 5, 0, 21, 0, -1};
short D_800A13F8[16] = {116, 52, 0, 2, 3, 2, 22, 2, 0, 2, 0, 2, 21, 0, -1};
short D_800A1418[16] = {116, 52, 1, 4, 1, 4, 22, 2, 4, 2, 5, 2, 21, 0, -1};
short D_800A1438[14] = {112, 80, 1054, 3, 1, 5, 1055, 4, 1056, 2, 1, 0, -1};
short D_800A1454[14] = {112, 80, 1054, 3, 1, 5, 1055, 4, 1056, 2, 2, 0, -1};
short D_800A1470[14] = {112, 80, 1054, 3, 2, 2, 1055, 4, 1056, 2, 1, 0, -1};
short D_800A148C[14] = {112, 80, 1054, 3, 2, 2, 1055, 4, 1056, 2, 2, 0, -1};
short D_800A14A8[12] = {112, 80, 1057, 2, 1055, 4, 1056, 2, 1, 0, -1};
short D_800A14C0[12] = {112, 80, 1057, 2, 1055, 4, 1056, 2, 2, 0, -1};
short D_800A14D8[12] = {112, 80, 1057, 2, 1055, 4, 1056, 2, 3, 0, -1};
short D_800A14F0[12] = {112, 80, 1057, 2, 1055, 4, 1056, 2, 4, 0, -1};
short D_800A1560[8] = {132, 104, 34, 0, 38, 0, -1};
short D_800A1570[8] = {132, 104, 35, 0, 38, 0, -1};
short D_800A1580[8] = {132, 104, 36, 0, 38, 0, -1};
short D_800A1590[8] = {132, 104, 37, 0, 38, 0, -1};
short D_800A15A0[8] = {132, 104, 39, 0, 38, 0, -1};
short D_800A1730[8] = {120, 38, 49, 0, 40, 0, -1};
short D_800A1740[10] = {120, 38, 49, 0, 46, 1, 48, 0, -1};
short D_800A1754[10] = {120, 38, 49, 0, 47, 0, 48, 0, -1};
short D_800A1768[10] = {120, 38, 49, 0, 41, -1, 45, 0, -1};
short D_800A177C[10] = {120, 38, 49, 0, 42, 0, 45, 0, -1};
short D_800A1790[10] = {120, 38, 49, 0, 43, 1, 45, 0, -1};
short D_800A17A4[10] = {120, 38, 49, 0, 44, -1, 45, 0, -1};
const char StrOpeningIn[4] = {73, 78};
const char StrOpeningBacktick[4] = {96};
const char StrOpeningApostrophe[4] = {39};
const char StrFmtTwoDigitsOpening[8] = {37, 48, 50, 100};
const char StrRandomPlay[12] = {82, 65, 78, 68, 79, 77, 32, 80, 76, 65, 89};
s32 BgmNumberToCdTrack[15] = {23, 14, 11, 20, 18, 12, 22, 16, 21, 19, 17, 13, 15, 24, 25};
char CourseNames[192] = {72, 69, 76, 84, 69, 82, 32, 83, 75, 69, 76, 84, 69, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 79, 85, 84, 32, 79, 70, 32, 66, 76, 85, 69, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 69, 68, 71, 69, 32, 79, 70, 32, 84, 72, 69, 32, 69, 65, 82, 84, 72, 0, 0, 0, 0, 0, 0, 0, 66, 82, 73, 71, 72, 84, 69, 83, 84, 32, 78, 73, 84, 69, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 87, 79, 78, 68, 69, 82, 72, 73, 76, 76, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 72, 69, 65, 86, 69, 78, 32, 65, 78, 68, 32, 72, 69, 76, 76, 0, 0, 0, 0, 0, 0, 0, 0, 0, 80, 72, 65, 78, 84, 79, 77, 73, 76, 69, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 83, 72, 79, 79, 84, 73, 78, 71, 32, 72, 79, 79, 80, 83};
s16 *OpeningDateGlyphListsPal[8] = {(short *)D_800A11F8, (short *)D_800A1220, (short *)D_800A1248, (short *)D_800A1270, (short *)D_800A1298, (short *)D_800A12C0, (short *)D_800A12E8, (short *)D_800A1310};
s16 *OpeningTimeGlyphListsPal[8] = {(short *)D_800A1338, (short *)D_800A1358, (short *)D_800A1378, (short *)D_800A1398, (short *)D_800A13B8, (short *)D_800A13D8, (short *)D_800A13F8, (short *)D_800A1418};
s16 *OpeningStageGlyphListsPal[8] = {(short *)D_800A1438, (short *)D_800A1454, (short *)D_800A1470, (short *)D_800A148C, (short *)D_800A14A8, (short *)D_800A14C0, (short *)D_800A14D8, (short *)D_800A14F0};
s16 *OpeningAttemptGlyphLists[5] = {(short *)D_800A1560, (short *)D_800A1570, (short *)D_800A1580, (short *)D_800A1590, (short *)D_800A15A0};
s16 *OpeningModeGlyphLists[7] = {(short *)D_800A1730, (short *)D_800A1740, (short *)D_800A1754, (short *)D_800A1768, (short *)D_800A177C, (short *)D_800A1790, (short *)D_800A17A4};
u8 HudSpeedUnitSprites[8] = {0x30, 0xD4, 8, 0xC, 0x38, 0xD4, 8, 0xC};
u8 HudViewModeIconUvs[4] = {0x40, 0xD4, 0xC, 0xC};
u8 HudLapSlashSprite[4] = {0x4C, 0xD4, 6, 0xC};
u32 HudSplitSignSprites[6] = {5, 0x32u, 0x64u, 0x96u, 0xC8u, 0xFAu};
u32 HudRecordCrownSprites[6] = {5, 0x32u, 0x64u, 0x96u, 0xC8u, 0xC8u};
s32 OpeningFadeTimer;
s32 OpeningBannerMode;
s32 OpeningAttemptIndex;
s32 OpeningBgmSelectState;
s32 OpeningSlideTimer;

/* The Japanese release's race opening lines (SLPS-01800). */
static s16 OpeningDateGlyphsJp0[31] = {80, 24, 1, 4, 9, 2, 9, 2, 9, 2, 10, 0, 0, 2, 5, 2, 11, 2, 0, 0, 1, 4, 12, 2, 16, 2, 13, 0, 17, 0, -1};
static s16 OpeningDateGlyphsJp1[31] = {80, 24, 1, 4, 9, 2, 9, 2, 9, 2, 10, 0, 0, 2, 5, 2, 11, 0, 1, 4, 5, 2, 12, 2, 16, 2, 13, 0, 17, 0, -1};
static s16 OpeningDateGlyphsJp2[31] = {80, 24, 1, 4, 9, 2, 9, 2, 9, 2, 10, 0, 0, 2, 7, 2, 11, 2, 0, 0, 1, 4, 12, 2, 16, 2, 14, -2, 17, 0, -1};
static s16 OpeningDateGlyphsJp3[31] = {80, 24, 1, 4, 9, 2, 9, 2, 9, 2, 10, 0, 0, 2, 7, 2, 11, 0, 1, 4, 5, 2, 12, 2, 16, 2, 14, -2, 17, 0, -1};
static s16 OpeningDateGlyphsJp4[31] = {80, 24, 1, 4, 9, 2, 9, 2, 9, 2, 10, 0, 0, 2, 9, 2, 11, 0, 2, 2, 4, 2, 12, 2, 16, 2, 15, -2, 17, 0, -1};
static s16 OpeningDateGlyphsJp5[31] = {80, 24, 1, 4, 9, 2, 9, 2, 9, 2, 10, 0, 1, 4, 0, 2, 11, 0, 2, 2, 2, 2, 12, 2, 16, 2, 15, -2, 17, 0, -1};
static s16 OpeningDateGlyphsJp6[31] = {80, 24, 1, 4, 9, 2, 9, 2, 9, 2, 10, 0, 1, 4, 1, 4, 11, 0, 2, 2, 6, 2, 12, 2, 16, 2, 15, -2, 17, 0, -1};
static s16 OpeningDateGlyphsJp7[31] = {80, 24, 1, 4, 9, 2, 9, 2, 9, 2, 10, 0, 1, 4, 2, 2, 11, 0, 3, 2, 1, 4, 12, 2, 16, 2, 15, -2, 17, 0, -1};
s16 *OpeningDateGlyphListsJp[8] = {OpeningDateGlyphsJp0, OpeningDateGlyphsJp1, OpeningDateGlyphsJp2, OpeningDateGlyphsJp3, OpeningDateGlyphsJp4, OpeningDateGlyphsJp5, OpeningDateGlyphsJp6, OpeningDateGlyphsJp7};
static s16 OpeningTimeGlyphsJp0[15] = {116, 40, 1, 4, 1, 4, 22, 2, 3, 2, 0, 0, 20, 0, -1};
static s16 OpeningTimeGlyphsJp1[15] = {116, 40, 0, 2, 5, 2, 22, 2, 3, 2, 0, 2, 21, 0, -1};
static s16 OpeningTimeGlyphsJp2[15] = {116, 40, 0, 2, 8, 2, 22, 2, 4, 2, 5, 2, 21, 0, -1};
static s16 OpeningTimeGlyphsJp3[15] = {116, 40, 0, 2, 6, 2, 22, 2, 3, 2, 0, 0, 20, 0, -1};
static s16 OpeningTimeGlyphsJp4[15] = {116, 40, 1, 4, 2, 2, 22, 2, 0, 2, 0, 2, 21, 0, -1};
static s16 OpeningTimeGlyphsJp5[15] = {116, 40, 0, 2, 9, 2, 22, 2, 4, 2, 5, 0, 21, 0, -1};
static s16 OpeningTimeGlyphsJp6[15] = {116, 40, 0, 2, 3, 2, 22, 2, 0, 2, 0, 2, 21, 0, -1};
static s16 OpeningTimeGlyphsJp7[15] = {116, 40, 1, 4, 1, 4, 22, 2, 4, 2, 5, 2, 21, 0, -1};
s16 *OpeningTimeGlyphListsJp[8] = {OpeningTimeGlyphsJp0, OpeningTimeGlyphsJp1, OpeningTimeGlyphsJp2, OpeningTimeGlyphsJp3, OpeningTimeGlyphsJp4, OpeningTimeGlyphsJp5, OpeningTimeGlyphsJp6, OpeningTimeGlyphsJp7};
static s16 OpeningStageGlyphsJp0[13] = {112, 56, 1, 5, 23, 6, 24, 0, 1, 5, 25, 0, -1};
static s16 OpeningStageGlyphsJp1[13] = {112, 56, 1, 5, 23, 6, 24, 0, 2, 1, 25, 0, -1};
static s16 OpeningStageGlyphsJp2[13] = {112, 56, 2, 1, 23, 6, 24, 0, 1, 5, 25, 0, -1};
static s16 OpeningStageGlyphsJp3[13] = {112, 56, 2, 1, 23, 6, 24, 0, 2, 1, 25, 0, -1};
static s16 OpeningStageGlyphsJp4[11] = {112, 56, 26, 6, 24, 0, 1, 5, 25, 0, -1};
static s16 OpeningStageGlyphsJp5[11] = {112, 56, 26, 6, 24, 0, 2, 1, 25, 0, -1};
static s16 OpeningStageGlyphsJp6[11] = {112, 56, 26, 6, 24, 0, 3, 1, 25, 0, -1};
static s16 OpeningStageGlyphsJp7[11] = {112, 56, 26, 6, 24, 0, 4, 1, 25, 0, -1};
s16 *OpeningStageGlyphListsJp[8] = {OpeningStageGlyphsJp0, OpeningStageGlyphsJp1, OpeningStageGlyphsJp2, OpeningStageGlyphsJp3, OpeningStageGlyphsJp4, OpeningStageGlyphsJp5, OpeningStageGlyphsJp6, OpeningStageGlyphsJp7};
