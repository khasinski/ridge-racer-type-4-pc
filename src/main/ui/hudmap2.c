#include "rr/release.h"
#include "rr/save_slots.h"
#include "rr/save_progress.h"
#include "rr/menu_selection.h"
/* RODATA 0x80011200 0x1c */
#include "types.h"
#include "rr/frame_storage_types.h"
#include "scratchpad.h"
#include "psyq/libgte/libgte.h"
#include "rr/line_emit.h"
#include "rr/glyph_list.h"
#include "rr/gpu.h"
#include "rr/icon_sprites.h"
#include "game/sound/sndctrl.h"
#include "game/demo/opening.h"
#include "game/sound/cdpos.h"
#include "game/system/modeinit.h"
#include "game/ui/hudmap2.h"
#include "rr/audio_state_types.h"
#include "psyq/libgpu/gpudisp.h"
#include "rr/race_setup.h"
#include "rr/audio_loop.h"
#include "psyq/libc/rand.h"
#include "game/sound/cdplay.h"
#include "rr/track_sample_types.h"

/* The two original entries differ only in the sprite's vertical position:
 * a 24x16 banner glyph followed by draw mode E100023E on ordering entry 0. */
#define RR_ICON_SPRITE_BODY(Y) \
    SpritePacket *p; \
    GpuCommandPacket *mode; \
    GpuDmaTag *ordering; \
    p = (SpritePacket *)RR_GPU_SCRATCH->cursor; \
    p->tag.length = 4; \
    p->code = 0x66; \
    p->x = 148; \
    p->y = Y; \
    p->w = 24; \
    p->h = 16; \
    p->r = color; \
    p->g = color; \
    p->b = color; \
    p->u = (u32)index * 24U + 112U; \
    p->v = 172; \
    p->clut = OpeningBannerClutTable[index]; \
    ordering = &CurrentDrawBuf->buffer.ordering70.tags[0]; \
    RR_GPU_LINK(ordering, p); \
    mode = (GpuCommandPacket *)(p + 1); \
    mode->tag.length = 1; \
    mode->command = 0xE1000000U | 0x23EU; \
    RR_GPU_LINK(ordering, mode); \
    RR_GPU_SCRATCH->cursor = (u8 *)(mode + 1);

typedef struct {
    u8 u, v, w, h;
} UvRect;

extern s32 CarCollisionGraceTimers;
extern UvRect HudMapSpriteRects[];
extern UvRect HudSpeedUnitScaleTable;
extern UvRect HudSpeedUnitMaxTable;
extern s16 CarDrivetrainStatBias[];
extern s16 CarTireGripStatBias[];
typedef struct { u16 x, y; } CarAccelCurveThreshold;
extern CarAccelCurveThreshold CarAccelCurveThresholds[];
extern s32 CarTopSpeedGearRatios[];
extern short SelectedTeamIndex;
extern s16 HudTachRedlineAngles[];
extern s16 HudTachSweepLimitAngles[];
extern s16 *HudRankOrdinalUvs[];
extern s16 *HudTimeLabelSprites[];
extern s16 *HudBigDigitSprites[];
extern s32 HudSmallDigitSprites[];
extern s32 CourseBgmChoiceTable3[];
extern s32 HudGearDigitSprites[];
#include "game/data/selection.h"

void DrawMapBitmapIso(struct GpuDmaTag *arg0, u8 *arg1, s32 *arg2);
void DrawTrackNodeMap(s32 *arg0);
void CenterGlyphListX(short *a0, const UiUvRect *table);
void InitRaceOpeningOverlay(s32 arg0, s32 arg1);
void DrawGlyphList(s16 *stream,s32 color,s32 bank);
void AddBrightnessSprite(unsigned char *a0x, int a1, int a2, int a3);
void EmitIconSpriteRow120(s32 index,s32 color);
void EmitIconSpriteRow128(s32 index,s32 color);

void DrawMapBitmapIso(struct GpuDmaTag *arg0, u8 *arg1, s32 *arg2) {
    SpritePacketSignedClut *sprt;
    GpuDrawModePacket *mode;
    s32 i, j;
    s32 x0, y0;
    s32 base_idx;
    u32 idx;
    u8 val;
    
    base_idx = ((0x100 - (arg2[2] + 0x200) / 256) << 8) + (0x100 - (arg2[0] + 0x200) / 256);
    sprt = (SpritePacketSignedClut *)RR_GPU_SCRATCH->cursor;
    for (i = 0; i < 24; i++) {
        x0 = (i + 24) * 4 + 0x6C;
        y0 = (i - 24) * 4 + 0xD0;
        for (j = 0; j < 24; j++) {
            val = 0;
            idx = base_idx + (i - 12) * 256 + (j - 12);
            if (idx < 0x10000) val = arg1[idx];
            sprt->tag.length = 4; sprt->code = 0x64; sprt->w = 4; sprt->h = 8;
            if (val == 0) {
                idx = 1;
                sprt->u = 0x20; sprt->v = 0xE8;
                sprt->x = x0;
                sprt->y = y0;
                sprt->code = (sprt->code & ~2) | 1;
                sprt->clut = 0x7E24;
            } else {
                idx = 0;
                sprt->r = 0xE4; sprt->g = 0x40; sprt->b = 0x18;
                sprt->y = y0 - 0x18;
                sprt->u = 0x20; sprt->v = 0xE8;
                sprt->x = x0;
                sprt->code = (sprt->code & ~2) | 1;
                sprt->clut = 0x7F64;
            }
            RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[10 + idx], sprt);
            sprt++;
            x0 -= 4;
            y0 += 4;
        }
    }
    mode = (GpuDrawModePacket *)sprt;
    SetDrawMode(mode, 0, 1, 0x19, 0);
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[11], mode);
    RR_GPU_SCRATCH->cursor = (u8 *)(mode + 1);
    DrawTrackNodeMap(arg2);
}

void DrawTrackNodeMap(s32 *arg0) {
    SVECTOR sp10;
    SVECTOR sp18;
    s16 sp20[4];
    u8 sp28[4];
    GpuTilePacket *tile;
    SpritePacketSignedClut *sprt;
    GpuCommandPacket *drmode;
    s32 i;
    s16 *s1;
    UvRect *s3;
    s32 s2;
    TrackSample3C *sec;

    tile = (GpuTilePacket *)RR_GPU_SCRATCH->cursor;
    sp10.vx = CarAccelCurveThresholds[CourseIndex].x;
    sp10.vy = CarAccelCurveThresholds[CourseIndex].y;

    for (i = 0; i < TrackNodeCount; i++) {
        sp18.vx = TrackNodes[i].road.x / 384;
        sp18.vy = -TrackNodes[i].road.z / 192;
        sp18.vx += sp10.vx;
        sp18.vy += sp10.vy;
        tile->tag.length = 3;
        tile->code = 0x62;
        tile->x = sp18.vx - 1;
        tile->y = sp18.vy - 3;
        tile->w = 3;
        tile->h = 6;
        tile->r = 0xFF;
        tile->g = 0xDE;
        tile->b = 0x7F;
        RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[5], tile);
        tile++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)tile;

    if (SelectedCourseChoice < 8) {
        s1 = CarDrivetrainStatBias;
        s3 = &HudSpeedUnitScaleTable;
    } else {
        s1 = CarTireGripStatBias;
        s3 = &HudSpeedUnitMaxTable;
    }

    sec = &TrackNodes[CarTopSpeedGearRatios[CourseIndex]];
    sp18.vx = sec->road.x / 384;
    sp18.vy = -sec->road.z / 192;
    sp18.vx += sp10.vx;
    sp18.vy += sp10.vy;

    s2 = sp20[0] = s1[0] + sp18.vx;
    sp20[1] = s1[1] + sp18.vy;
    sp20[2] = s1[2] + sp18.vx;
    sp20[3] = s1[3] + sp18.vy;
    sp28[0] = 0xFF;
    sp28[1] = 0xDE;
    sp28[2] = 0x7F;

    for (i = 0; i < 4; i++) {
        sp20[2] = sp20[0] = s2 + (i & 1);
        AddLineF2Prim(sp20, sp28, 1, 5);
    }

    sprt = (SpritePacketSignedClut *)RR_GPU_SCRATCH->cursor;
    sprt->tag.length = 4;
    sprt->code = 0x64;
    sprt->u = HudMapSpriteRects[1].u;
    sprt->v = HudMapSpriteRects[1].v;
    sprt->w = HudMapSpriteRects[1].w;
    sprt->h = HudMapSpriteRects[1].h;
    sprt->x = s1[4] + sp18.vx;
    sprt->y = s1[5] + sp18.vy;
    sprt->code |= 3;
    sprt->clut = 0x7FE5;
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[5], sprt);
    sprt++;

    sprt->tag.length = 4;
    sprt->code = 0x64;
    sprt->u = s3->u;
    sprt->v = s3->v;
    sprt->w = s3->w;
    sprt->h = s3->h;
    sprt->x = s1[6] + sp18.vx;
    sprt->y = s1[7] + sp18.vy;
    sprt->code |= 3;
    sprt->clut = 0x7FE5;
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[5], sprt);
    sprt++;

    RR_GPU_SCRATCH->cursor = (u8 *)sprt;
    drmode = (GpuCommandPacket *)sprt;
    drmode->tag.length = 1;
    drmode->command = 0xE1000219;
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[5], drmode);
    drmode++;
    RR_GPU_SCRATCH->cursor = (u8 *)drmode;

    sp18.vx = arg0[0] / 384;
    sp18.vy = -arg0[2] / 192;
    sp18.vx += sp10.vx;
    sp18.vy += sp10.vy;

    CarCollisionGraceTimers &= 0xFF;
    if (CarCollisionGraceTimers & 0x80) {
        CarCollisionGraceTimers &= 0x7F;
        if (CarCollisionGraceTimers < 0x7A) {
            CarCollisionGraceTimers = (CarCollisionGraceTimers + 6) | 0x80;
        } else {
            CarCollisionGraceTimers = 0x7F;
        }
    } else {
        CarCollisionGraceTimers &= 0x7F;
        if (CarCollisionGraceTimers >= 6) {
            CarCollisionGraceTimers -= 6;
        } else {
            CarCollisionGraceTimers = 0x80;
        }
    }

    sprt = (SpritePacketSignedClut *)RR_GPU_SCRATCH->cursor;
    sprt->tag.length = 4;
    sprt->code = 0x64;
    sprt->u = HudMapSpriteRects[0].u;
    sprt->v = HudMapSpriteRects[0].v;
    sprt->w = HudMapSpriteRects[0].w;
    sprt->h = HudMapSpriteRects[0].h;
    sprt->r = CarCollisionGraceTimers & 0x7F;
    sprt->g = CarCollisionGraceTimers & 0x7F;
    sprt->b = CarCollisionGraceTimers & 0x7F;
    sprt->x = sp18.vx - 4;
    sprt->y = sp18.vy - 8;
    sprt->code = (sprt->code | 2) & ~1;
    sprt->clut = 0x7FE5;
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[4], sprt);
    sprt++;

    RR_GPU_SCRATCH->cursor = (u8 *)sprt;
    drmode = (GpuCommandPacket *)sprt;
    drmode->tag.length = 1;
    drmode->command = 0xE1000239;
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[4], drmode);
    drmode++;
    RR_GPU_SCRATCH->cursor = (u8 *)drmode;
}

void SelectReplayBgmTrack(void) {
 int r, a1; if (GameModeType == 1) { if (CourseIndex == 7) { a1 = SelectedTeamIndex + 0x20; CdAudioSelectTrack(0, a1); return; } } r = PsyqRand(); a1 = r % 6 + 0x1A; CdAudioSelectTrack(0, a1);
}

void SelectCourseBgmTrack(void) {
    int a0, a1, v;
    CdAudioInit();
    if (GameModeType == 1) {
        a0 = SelectionState.signedBytes[SelectionState.signedBytes[0x17] + 8] < 3;
    } else {
        a0 = 1;
    }
    if (SaveProgress.prefix.unlocks50.extraModeUnlocked50 != 0 && GameModeType != 1) {
        SoundState.count28A8 = 0x10;
    } else {
        SoundState.count28A8 = 0xF;
    }
    if (a0 == 0) {
        a1 = CourseIndex + 3;
        CdAudioSelectTrack(0, a1);
    } else if (SoundState.track28A4 == 0) {
        a1 = PsyqRand() % 14 + 0xB;
        CdAudioSelectTrack(0, a1);
    } else {
        a1 = BgmNumberToCdTrack[SoundState.track28A4 - 1];
        CdAudioSelectTrack(0, a1);
    }
}

/* Centres a glyph list on X 160: entries below 0x400 index the supplied UV
 * table, 0x400 and above the secondary HUD glyph table; each is followed by
 * its gap. */
void CenterGlyphListX(short *a0, const UiUvRect *table)
{
    short *p = a0 + 2;
    int sum = 0;
    short glyph;
    const UiUvRect *e;
    if (a0[2] >= 0) {
        do {
            glyph = *p++;
            /* Extended glyphs exist only in the PAL release. */
            if (glyph >= 0x400 && !RR_RELEASE_JP) {
                e = &HudGlyphUvRects[(short)(glyph - 0x400)];
            } else {
                e = &table[glyph];
            }
            sum += e->w;
            sum += *p++;
        } while (*p >= 0);
    }
    a0[0] = 0xA0 - (sum / 2);
}

void InitRaceOpeningOverlay(s32 arg0, s32 arg1) {
    s32 i;

    OpeningBannerMode = arg0;
    OpeningAttemptIndex = arg1;
    OpeningFadeTimer = -1;
    OpeningBgmSelectState = 0;
    OpeningSlideTimer = 0;
    SoundReverbOffAll();

    if (GameModeType != 1) {
        SoundState.track28A4 = 0;
    } else {
        switch (SelectedTeamIndex) {
        case 0:
            SoundState.track28A4 = HudGearDigitSprites[CourseIndex] + 1;
            break;
        case 1:
            SoundState.track28A4 = HudSmallDigitSprites[CourseIndex] + 1;
            break;
        case 2:
            SoundState.track28A4 = CourseBgmChoiceTable2[CourseIndex] + 1;
            break;
        case 3:
            SoundState.track28A4 = CourseBgmChoiceTable3[CourseIndex] + 1;
            break;
        }
    }

    switch (GameModeType) {
    case 0:
    case 2:
    case 3:
    case 6:
        SoundState.track28A4 = SoundState.displayed28AC;
        for (i = 0; i < 2; i++) {
            if (SelectionRows.entries[i].variant == 0 && SelectionRows.entries[i].selection == 2 && SelectionRows.entries[i].parameter == 0x27) {
                SoundState.track28A4 = 0xF;
            }
        }
    case 1:
    case 5:
        if (SaveProgress.prefix.unlocks50.extraModeUnlocked50 != 0 && GameModeType != 1) {
            SoundState.count28A8 = 0x10;
        } else {
            SoundState.count28A8 = 0xF;
        }
        break;
    case 4:
    default:
        OpeningBgmSelectState = -1;
        SoundState.track28A4 = CourseBgmChoiceTable2[CourseIndex] + 1;
        SoundState.count28A8 = 0;
        break;
    }

    CdPlaybackTrackIndex = SoundState.track28A4 + 0xB;

    for (i = 0; i < 8; i++) {
        CenterGlyphListX(OpeningDateGlyphLists[i], CarHandlingResponseTable);
        CenterGlyphListX(OpeningTimeGlyphLists[i], CarHandlingResponseTable);
        CenterGlyphListX(OpeningStageGlyphLists[i], CarHandlingResponseTable);
        CenterGlyphListX(HudRankOrdinalUvs[i], CarHandlingResponseTable);
    }

    for (i = 0; i < 5; i++) {
        CenterGlyphListX(OpeningAttemptGlyphLists[i], CarHandlingResponseTable);
    }

    for (i = 0; i < 8; i++) {
        CenterGlyphListX(HudTimeLabelSprites[i], HudGlyphUvRects);
        CenterGlyphListX(HudTimeLabelSprites[i] + 8, HudGlyphUvRects);
        CenterGlyphListX(HudTimeLabelSprites[i] + 14, HudGlyphUvRects);
    }

    for (i = 0; i < 7; i++) {
        CenterGlyphListX(OpeningModeGlyphLists[i], CarHandlingResponseTable);
    }

    CenterGlyphListX(HudTachRedlineAngles, HudGlyphUvRects);
    CenterGlyphListX(HudTachSweepLimitAngles, HudGlyphUvRects);

    for (i = 0; i < 8; i++) {
        CenterGlyphListX(HudBigDigitSprites[i], HudGlyphUvRects);
    }
}

void DrawGlyphList(s16 *stream,s32 color,s32 bank)
{
    u32 x;
    s32 y, defaultPage, clut, page, offset, gap;
    u32 cachedGlyph;
    
    s32 basePage;
    s32 spriteCode;
    SpritePacketSignedClut *sprite;
    GlyphCommand8 *mode;
    UiUvRect *uv;
    x = *stream++;
    y = *stream++;
    if (bank == 0) { clut = 0x7F78; defaultPage = 0x3E; } else { clut = 0x7E38; defaultPage = 0x25; }
    if (RR_RELEASE_JP) {
        /* The Japanese release has no extended glyphs (0x400 and up, on
         * texture page 0x25): one sprite a glyph, then one draw mode. */
        sprite = (SpritePacketSignedClut *)RR_GPU_SCRATCH->cursor;
        while (*stream >= 0) {
            offset = *stream++ * 4;
            if (bank == 0) uv = (UiUvRect*)((u8 *)CarHandlingResponseTable + offset);
            else uv = (UiUvRect*)((u8 *)HudGlyphUvRects + offset);
            sprite->tag.length = 4;
            sprite->code = 0x66;
            sprite->r = color; sprite->g = color; sprite->b = color;
            sprite->x = x;
            sprite->y = y;
            sprite->w = uv->w;
            sprite->h = uv->h;
            sprite->u = uv->u;
            sprite->v = uv->v;
            sprite->clut = clut;
            RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], sprite);
            sprite++;
            gap = *stream++;
            x = x + (uv->w + gap);
        }
        mode = (GlyphCommand8 *)sprite;
        mode->tag.length = 1;
        mode->command = defaultPage | 0xE1000200;
        RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], mode);
        RR_GPU_SCRATCH->cursor = (u8 *)(mode + 1);
        return;
    }
    cachedGlyph = *(u16 *)stream;
    if (*stream >= 0) {
        basePage = defaultPage & 0xFFFF;
        do {
            s32 glyph;
            sprite = (SpritePacketSignedClut *)RR_GPU_SCRATCH->cursor;
            page = defaultPage;
            if (basePage == 0x3E) {
                if ((s16)cachedGlyph >= 0x400) page = 0x25;
            }
            cachedGlyph = *(u16 *)stream;
            glyph = *stream++;
            if (glyph >= 0x400) {
                offset = (s32)((cachedGlyph - 0x400U) << 16) >> 14;
                uv = (UiUvRect*)((u8 *)HudGlyphUvRects + offset);
            } else {
                offset = glyph * 4;
                if (bank == 0) uv = (UiUvRect*)((u8 *)CarHandlingResponseTable + offset);
                else uv = (UiUvRect*)((u8 *)HudGlyphUvRects + offset);
            }
            spriteCode = 0x66;
            sprite->tag.length = 4;
            sprite->code = spriteCode;
            sprite->r = color; sprite->g = color; sprite->b = color;
            sprite->x = x;
            sprite->y = y;
            sprite->w = uv->w;
            sprite->h = uv->h;
            sprite->u = uv->u;
            sprite->v = uv->v;
            sprite->clut = clut;
            RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], sprite);
            mode = (GlyphCommand8 *)(sprite + 1);
            RR_GPU_SCRATCH->cursor = (u8 *)mode;
            mode->tag.length = 1;
            mode->command = (page & 0x9FF) | 0xE1000200;
            RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], mode);
            RR_GPU_SCRATCH->cursor = (u8 *)(mode + 1);
            gap = *stream++;
            x=(x+(uv->w+gap));
            cachedGlyph = *(u16 *)stream;
        } while (*stream >= 0);
    }
}

void DrawGlyphListSloped(s16 *t1, s32 a1, s32 a2) {
    s32 t5, s4, t6 = 0, t7, t8, t3, v, w;
    u32 a;
    
    s32 m;
    s32 k;
    SpritePacketSignedClut *p;
    GpuCommandPacket *mode;
    u8 *t0;
    t5 = *t1++;
    s4 = *t1++;
    if (a2 == 0) { t8 = 0x7F78; t7 = 0x3E; } else { t8 = 0x7E38; t7 = 0x25; }
    a = *(u16 *)t1;
    if (*t1 >= 0) {
        m = t7 & 0xFFFF;
        do {
            s32 e;
            p = (SpritePacketSignedClut *)RR_GPU_SCRATCH->cursor;
            t3 = t7;
            if (m == 0x3E) {
                if ((s16)a >= 0x400) t3 = 0x25;
            }
            a = *(u16 *)t1;
            e = *t1++;
            if (e >= 0x400) {
                v = (s32)((a - 0x400) << 16) >> 14;
                t0 = (u8 *)HudGlyphUvRects + v;
            } else {
                v = e * 4;
                if (a2 == 0) t0 = (u8 *)CarHandlingResponseTable + v;
                else t0 = (u8 *)HudGlyphUvRects + v;
            }
            k = 0x66;
            p->tag.length = 4;
            p->code = k;
            p->r = a1; p->g = a1; p->b = a1;
            p->x = t5;
            p->y = s4 + t6;
            p->w = t0[2];
            p->h = t0[3];
            p->u = t0[0];
            p->v = t0[1];
            p->clut = t8;
            t6++;
            RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], p);
            mode = (GpuCommandPacket *)(p + 1);
            RR_GPU_SCRATCH->cursor = (u8 *)mode;
            mode->tag.length = 1;
            mode->command = (t3 & 0x9FF) | 0xE1000200;
            RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], mode);
            RR_GPU_SCRATCH->cursor = (u8 *)(mode + 1);
            w = *t1++;
            t5 += t0[2] + w;
            a = *(u16 *)t1;
        } while (*t1 >= 0);
    }
}

void AddBrightnessSprite(unsigned char *a0x, int a1, int a2, int a3) {
    SpritePacketSignedClut *p;
    GpuCommandPacket *m;

    p = (SpritePacketSignedClut *)RR_GPU_SCRATCH->cursor;
    p->tag.length = 4;
    p->code = 0x64;
    if (a3 < 0xFF) {
        p->code = 0x66;
    } else {
        p->code = 0x65;
    }
    p->r = a3;
    p->g = a3;
    p->b = a3;
    p->x = a1;
    p->y = a2;
    p->w = a0x[2];
    p->h = a0x[3];
    p->u = a0x[0];
    p->v = a0x[1];
    p->clut = 0x7F78;
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], p);
    p++;
    RR_GPU_SCRATCH->cursor = (u8 *)p;
    m = (GpuCommandPacket *)p;
    m->tag.length = 1;
    m->command = 0xE100023E;
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], m);
    m++;
    RR_GPU_SCRATCH->cursor = (u8 *)m;
}

void EmitIconSpriteRow120(s32 index,s32 color) {
    RR_ICON_SPRITE_BODY(120)
}

void EmitIconSpriteRow128(s32 index,s32 color) {
    RR_ICON_SPRITE_BODY(RR_PAL_JP(128, 120))
}

/* Initial values of the globals this module owns. */
short D_800A1508[12] = {128, 80, 18, 2, 28, 8, 29, 0, 19, 0, -1};
short D_800A1520[12] = {128, 80, 18, 2, 30, 8, 31, 1, 19, 0, -1};
short D_800A1538[10] = {128, 80, 18, 6, 32, 2, 19, 0, -1};
short D_800A154C[10] = {128, 80, 18, 7, 33, 2, 19, 0, -1};
short D_800A15B0[24] = {120, 156, 8, 0, 0, 0, -1, 0, 104, 166, 18, 0, -1, 0, 88, 176, 9, 0, 12, 0, 16, 0, -1};
short D_800A15E0[24] = {120, 156, 8, 0, 1, 0, -1, 0, 104, 166, 22, 0, -1, 0, 88, 176, 9, 0, 13, -1, 16, 0, -1};
short D_800A1610[24] = {120, 156, 8, 0, 2, 0, -1, 0, 104, 166, 20, 0, -1, 0, 88, 176, 10, 0, 12, 0, 16, 0, -1};
short D_800A1640[24] = {120, 156, 8, 0, 3, 0, -1, 0, 104, 166, 19, 0, -1, 0, 88, 176, 10, 0, 13, -1, 16, 0, -1};
short D_800A1670[24] = {120, 156, 8, 0, 4, 0, -1, 0, 104, 166, 24, 0, -1, 0, 88, 176, 11, 1, 12, 0, 16, 0, -1};
short D_800A16A0[24] = {120, 156, 8, 0, 5, 0, -1, 0, 104, 166, 21, 0, -1, 0, 88, 176, 11, 1, 13, -1, 16, 0, -1};
short D_800A16D0[24] = {120, 156, 8, 0, 6, 0, -1, 0, 104, 166, 23, 0, -1, 0, 88, 176, 11, 1, 14, -1, 16, 0, -1};
short D_800A1700[24] = {120, 156, 8, 0, 7, 0, -1, 0, 104, 166, 25, 0, -1, 0, 88, 176, 11, 1, 15, -1, 16, 0, -1};
short D_800A17B8[6] = {100, 156, 18, 0, -1};
short D_800A17C4[6] = {100, 156, 19, 0, -1};
short D_800A17D0[6] = {100, 156, 20, 0, -1};
short D_800A17DC[6] = {100, 156, 21, 0, -1};
short D_800A17E8[6] = {100, 156, 22, 0, -1};
short D_800A17F4[6] = {100, 156, 23, 0, -1};
short D_800A1800[6] = {100, 156, 24, 0, -1};
short D_800A180C[6] = {100, 156, 25, 0, -1};
u16 OpeningBannerClutTable[6] = {0x7F7A, 0x7F7B, 0x7FB8, 0x7FB9, 0x7FBB, 0x7FBA};
s32 CarCollisionGraceTimers;
UvRect HudMapSpriteRects[2] = {{.u = 0x20, .v = 0xD8, .w = 8, .h = 0x10}, {.u = 0x28, .v = 0xD8, .w = 0xA, .h = 0xC}};
UvRect HudSpeedUnitScaleTable = {.u = 0x32, .v = 0xD8, .w = 0xA, .h = 0xC};
UvRect HudSpeedUnitMaxTable = {.u = 0x3C, .v = 0xD8, .w = 0xA, .h = 0xC};
s16 CarDrivetrainStatBias[8] = {0, -18, 0, 10, 2, -18, -10, 4};
s16 CarTireGripStatBias[8] = {0, -18, 0, 10, -9, -18, 0, 4};
CarAccelCurveThreshold CarAccelCurveThresholds[8] = {{.x = 0x6D, .y = 0x14A}, {.x = 0x78, .y = 0x15F}, {.x = 0x7B, .y = 0x15E}, {.x = 0x76, .y = 0x17A}, {.x = 0x79, .y = 0x172}, {.x = 0x7E, .y = 0x15B}, {.x = 0x76, .y = 0x143}, {.x = 0x74, .y = 0x171}};
s32 CarTopSpeedGearRatios[8] = {18, 24, 533, 553, 1, 1, 49, 378};
UiUvRect CarHandlingResponseTable[50] = {{.v = 0x8C, .w = 0xA, .h = 0xE}, {.u = 0xC, .v = 0x8C, .w = 6, .h = 0xE}, {.u = 0x18, .v = 0x8C, .w = 0xA, .h = 0xE}, {.u = 0x24, .v = 0x8C, .w = 0xA, .h = 0xE}, {.u = 0x30, .v = 0x8C, .w = 0xA, .h = 0xE}, {.u = 0x3C, .v = 0x8C, .w = 0xA, .h = 0xE}, {.u = 0x48, .v = 0x8C, .w = 0xA, .h = 0xE}, {.u = 0x54, .v = 0x8C, .w = 0xA, .h = 0xE}, {.u = 0x60, .v = 0x8C, .w = 0xA, .h = 0xE}, {.u = 0x6C, .v = 0x8C, .w = 0xA, .h = 0xE}, {.u = 0x78, .v = 0x8C, .w = 0xE, .h = 0xE}, {.u = 0x88, .v = 0x8C, .w = 0xC, .h = 0xE}, {.u = 0x94, .v = 0x8C, .w = 0xC, .h = 0xE}, {.u = 0xA0, .v = 0x8C, .w = 0xC, .h = 0xE}, {.u = 0xB0, .v = 0x8C, .w = 0xE, .h = 0xE}, {.u = 0xC0, .v = 0x8C, .w = 0xE, .h = 0xE}, {.u = 0xD0, .v = 0x8C, .w = 6, .h = 0xE}, {.u = 0xD8, .v = 0x8C, .w = 8, .h = 0xE}, {.u = 0xE0, .v = 0x8C, .w = 6, .h = 0xE}, {.u = 0xE8, .v = 0x8C, .w = 8, .h = 0xE}, {.v = 0x9C, .w = 0x16, .h = 0xE}, {.u = 0x18, .v = 0x9C, .w = 0x16, .h = 0xE}, {.u = 0x30, .v = 0x9C, .w = 4, .h = 0xE}, {.u = 0x34, .v = 0x9C, .w = 0x26, .h = 0xE}, {.u = 0x5C, .v = 0x9C, .w = 0xE, .h = 0xE}, {.u = 0x6C, .v = 0x9C, .w = 0xE, .h = 0xE}, {.u = 0x7C, .v = 0x9C, .w = 0x2E, .h = 0xE}, {.u = 0xAC, .v = 0x9C, .w = 0x26, .h = 0xE}, {.u = 0xD4, .v = 0x9C, .w = 0xE, .h = 0xE}, {.u = 0xE4, .v = 0x9C, .w = 0xE, .h = 0xE}, {.v = 0xAC, .w = 0xE, .h = 0xE}, {.u = 0x10, .v = 0xAC, .w = 0xC, .h = 0xE}, {.u = 0x1C, .v = 0xAC, .w = 0x1A, .h = 0xE}, {.u = 0x38, .v = 0xAC, .w = 0x1A, .h = 0xE}, {.u = 0x54, .v = 0xD4, .w = 0x10, .h = 0xC}, {.u = 0x64, .v = 0xD4, .w = 0x14, .h = 0xC}, {.u = 0x78, .v = 0xD4, .w = 0x12, .h = 0xC}, {.u = 0x8C, .v = 0xD4, .w = 0x12, .h = 0xC}, {.u = 0xC0, .v = 0xD4, .w = 0x28, .h = 0xC}, {.u = 0xA0, .v = 0xD4, .w = 0x16, .h = 0xC}, {.v = 0xE0, .w = 0x42, .h = 0xC}, {.u = 0x44, .v = 0xE0, .w = 0x30, .h = 0xC}, {.u = 0xA4, .v = 0xE0, .w = 0x22, .h = 0xC}, {.u = 0xC8, .v = 0xE0, .w = 0x24, .h = 0xC}, {.u = 0x74, .v = 0xE0, .w = 0x2E, .h = 0xC}, {.v = 0xEC, .w = 0x18, .h = 0xC}, {.u = 0x18, .v = 0xEC, .w = 0x10, .h = 0xC}, {.u = 0x28, .v = 0xEC, .w = 0x18, .h = 0xC}, {.u = 0x40, .v = 0xEC, .w = 0x20, .h = 0xC}, {.u = 0x60, .v = 0xEC, .w = 0xA, .h = 0xC}};
UiUvRect HudGlyphUvRects[40] = {{.w = 0x3C, .h = 8}, {.u = 0x3C, .w = 0x3C, .h = 8}, {.u = 0x78, .w = 0x3A, .h = 8}, {.u = 0xB4, .w = 0x3A, .h = 8}, {.v = 8, .w = 0x3A, .h = 8}, {.u = 0x3C, .v = 8, .w = 0x3A, .h = 8}, {.u = 0x78, .v = 8, .w = 0x3A, .h = 8}, {.u = 0xB4, .v = 8, .w = 0x3A, .h = 8}, {.v = 0x10, .w = 0x12, .h = 8}, {.u = 0x14, .v = 0x10, .w = 0x24, .h = 8}, {.u = 0x38, .v = 0x10, .w = 0x26, .h = 8}, {.u = 0x60, .v = 0x10, .w = 0x24, .h = 8}, {.u = 0x88, .v = 0x10, .w = 0x16, .h = 8}, {.u = 0xA0, .v = 0x10, .w = 0x18, .h = 8}, {.u = 0xB8, .v = 0x10, .w = 0x18, .h = 8}, {.u = 0xD0, .v = 0x10, .w = 0x18, .h = 8}, {.v = 0x18, .w = 0x54, .h = 8}, {.u = 0x54, .v = 0x18, .w = 0x9E, .h = 8}, {.v = 0x20, .w = 0x74, .h = 8}, {.u = 0x7C, .v = 0x28, .w = 0x6C, .h = 8}, {.v = 0x28, .w = 0x7A, .h = 8}, {.u = 0x70, .v = 0x30, .w = 0x72, .h = 8}, {.u = 0x74, .v = 0x20, .w = 0x66, .h = 8}, {.v = 0x38, .w = 0x76, .h = 8}, {.v = 0x30, .w = 0x6E, .h = 8}, {.u = 0x78, .v = 0x38, .w = 0x80, .h = 8}, {.v = 0x40, .w = 0x7E, .h = 8}, {.w = 0x3A, .h = 0x10}, {.u = 0x3C, .w = 0x3C, .h = 0x10}, {.u = 0x78, .w = 0x2A, .h = 0x10}, {.u = 0xA4, .w = 0x1E, .h = 0x10}, {.u = 0xC4, .w = 4, .h = 0x10}, {.u = 0xCC, .w = 0x20, .h = 0x10}, {.v = 0x10, .w = 0x32, .h = 0x10}, {.u = 0x34, .v = 0x10, .w = 0x1A, .h = 0x10}, {.u = 0x50, .v = 0x10, .w = 0x18, .h = 0x10}, {.u = 0x68, .v = 0x10, .w = 0x40, .h = 0x10}, {.u = 0xA8, .v = 0x10, .w = 0x30, .h = 0x10}, {.v = 0x20, .w = 0x3A, .h = 0x10}, {.u = 0x3C, .v = 0x20, .w = 0x3C, .h = 0x10}};
s16 HudTachRedlineAngles[6] = {96, 166, 26, 0, -1};
s16 HudTachSweepLimitAngles[6] = {80, 176, 17, 0, -1};
s16 *HudRankOrdinalUvs[8] = {(short *)D_800A1508, (short *)D_800A1520, (short *)D_800A1538, (short *)D_800A1508, (short *)D_800A1508, (short *)D_800A1538, (short *)D_800A1520, (short *)D_800A154C};
s16 *HudTimeLabelSprites[8] = {(short *)D_800A15B0, (short *)D_800A15E0, (short *)D_800A1610, (short *)D_800A1640, (short *)D_800A1670, (short *)D_800A16A0, (short *)D_800A16D0, (short *)D_800A1700};
s16 *HudBigDigitSprites[8] = {(short *)D_800A17B8, (short *)D_800A17C4, (short *)D_800A17D0, (short *)D_800A17DC, (short *)D_800A17E8, (short *)D_800A17F4, (short *)D_800A1800, (short *)D_800A180C};
int CourseBgmChoiceTable2[8] = {0, 3, 2, 10, 1, 12, 9, 13};
s32 HudSmallDigitSprites[8] = {0, 8, 6, 2, 1, 11, 9, 13};
s32 CourseBgmChoiceTable3[8] = {0, 3, 1, 10, 7, 12, 9, 13};
s32 HudGearDigitSprites[8] = {0, 3, 10, 7, 1, 11, 9, 13};
