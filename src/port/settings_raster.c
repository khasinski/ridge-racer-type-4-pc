#include "settings_raster.h"

#include <psyz/file.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"

#include "fonts/ibm_plex_sans.ttf.inc"

typedef struct Color {
    float r, g, b, a;
} Color;

/* The palette of the Rage Racer and Parasite Eve panels. */
static const Color kPanel = {0.075f, 0.082f, 0.102f, 0.96f};
static const Color kBorder = {1.0f, 1.0f, 1.0f, 0.10f};
static const Color kAccent = {0.27f, 0.62f, 1.0f, 1.0f};
static const Color kText = {0.93f, 0.95f, 0.98f, 1.0f};
static const Color kMuted = {0.60f, 0.65f, 0.73f, 1.0f};
static const Color kDanger = {1.0f, 0.45f, 0.42f, 1.0f};

static Color WithAlpha(Color c, float a) {
    c.a = a;
    return c;
}

/* ---- surface ---- */

int SettingsSurfaceResize(SettingsSurface *s, int width, int height) {
    size_t bytes = (size_t)width * (size_t)(height + 1) * 4u;

    if (width == s->w && height + 1 == s->h && s->pixels)
        return 1;
    free(s->pixels);
    s->pixels = (uint8_t *)calloc(1, bytes);
    s->w = s->pixels ? width : 0;
    s->h = s->pixels ? height + 1 : 0;
    return s->pixels != NULL;
}

void SettingsSurfaceRelease(SettingsSurface *s) {
    free(s->pixels);
    memset(s, 0, sizeof(*s));
}

static float Clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

static void BlendPixel(SettingsSurface *s, int x, int y, Color c, float coverage) {
    uint8_t *p;
    float a = c.a * coverage;
    float keep = 1.0f - a;

    /* The last row is the solid texel, not part of the panel. */
    if (x < 0 || y < 0 || x >= s->w || y >= s->h - 1 || a <= 0.0f)
        return;
    p = s->pixels + ((size_t)y * (size_t)s->w + (size_t)x) * 4u;
    p[0] = (uint8_t)(c.r * a * 255.0f + p[0] * keep + 0.5f);
    p[1] = (uint8_t)(c.g * a * 255.0f + p[1] * keep + 0.5f);
    p[2] = (uint8_t)(c.b * a * 255.0f + p[2] * keep + 0.5f);
    p[3] = (uint8_t)(a * 255.0f + p[3] * keep + 0.5f);
}

/* ---- shapes ---- */

static float RoundRectDistance(float px, float py, float x, float y, float w, float h, float r) {
    float cx = x + w * 0.5f, cy = y + h * 0.5f;
    float qx = fabsf(px - cx) - (w * 0.5f - r);
    float qy = fabsf(py - cy) - (h * 0.5f - r);
    float ox = qx > 0.0f ? qx : 0.0f, oy = qy > 0.0f ? qy : 0.0f;
    float inside = qx > qy ? qx : qy;

    return sqrtf(ox * ox + oy * oy) + (inside < 0.0f ? inside : 0.0f) - r;
}

static void FillRoundRect(SettingsSurface *s, float x, float y, float w, float h, float r, Color c) {
    int x0 = (int)floorf(x) - 1, x1 = (int)ceilf(x + w) + 1;
    int y0 = (int)floorf(y) - 1, y1 = (int)ceilf(y + h) + 1;

    for (int iy = y0; iy < y1; iy++)
        for (int ix = x0; ix < x1; ix++)
            BlendPixel(s, ix, iy, c,
                       Clamp01(0.5f - RoundRectDistance((float)ix + 0.5f, (float)iy + 0.5f, x, y, w, h, r)));
}

static void DrawSegment(SettingsSurface *s, float ax, float ay, float bx, float by, float width, Color c) {
    float min_x = (ax < bx ? ax : bx) - width - 1.0f, max_x = (ax > bx ? ax : bx) + width + 1.0f;
    float min_y = (ay < by ? ay : by) - width - 1.0f, max_y = (ay > by ? ay : by) + width + 1.0f;
    float ex = bx - ax, ey = by - ay, length2 = ex * ex + ey * ey;

    for (int iy = (int)floorf(min_y); iy < (int)ceilf(max_y); iy++) {
        for (int ix = (int)floorf(min_x); ix < (int)ceilf(max_x); ix++) {
            float px = (float)ix + 0.5f - ax, py = (float)iy + 0.5f - ay;
            float t = Clamp01(length2 > 0.0f ? (px * ex + py * ey) / length2 : 0.0f);
            float qx = px - ex * t, qy = py - ey * t;
            BlendPixel(s, ix, iy, c, Clamp01(0.5f - (sqrtf(qx * qx + qy * qy) - width * 0.5f)));
        }
    }
}

static void DrawChevron(SettingsSurface *s, float cx, float cy, float size, int direction, float width, Color c) {
    float tip = cx + (float)direction * size * 0.5f, back = cx - (float)direction * size * 0.5f;

    DrawSegment(s, back, cy - size, tip, cy, width, c);
    DrawSegment(s, back, cy + size, tip, cy, width, c);
}

/* ---- text ---- */

static stbtt_fontinfo s_font;
static int s_font_state;

int SettingsFontReady(void) {
    int offset;

    if (s_font_state)
        return s_font_state > 0;
    offset = stbtt_GetFontOffsetForIndex(ibm_plex_sans_ttf, 0);
    if (offset < 0 || (size_t)offset >= ibm_plex_sans_ttf_len ||
        !stbtt_InitFont(&s_font, ibm_plex_sans_ttf, offset)) {
        s_font_state = -1;
        return 0;
    }
    s_font_state = 1;
    return 1;
}

static float TextWidth(const char *text, float em) {
    float scale = stbtt_ScaleForMappingEmToPixels(&s_font, em), width = 0.0f;

    for (; *text; text++) {
        int advance, bearing;
        stbtt_GetCodepointHMetrics(&s_font, (unsigned char)*text, &advance, &bearing);
        width += (float)advance * scale;
    }
    return width;
}

typedef enum TextAlign { ALIGN_LEFT, ALIGN_CENTRE, ALIGN_RIGHT } TextAlign;

static void DrawText(SettingsSurface *s, float x, float baseline, float em, const char *text, Color c,
                     TextAlign align) {
    enum { GLYPH_MAX = 256 };
    static uint8_t glyph[GLYPH_MAX * GLYPH_MAX];
    float scale = stbtt_ScaleForMappingEmToPixels(&s_font, em);
    float width = TextWidth(text, em);

    x -= align == ALIGN_CENTRE ? width * 0.5f : align == ALIGN_RIGHT ? width : 0.0f;
    for (; *text; text++) {
        int codepoint = (unsigned char)*text;
        int advance, bearing, x0, y0, x1, y1;

        stbtt_GetCodepointHMetrics(&s_font, codepoint, &advance, &bearing);
        stbtt_GetCodepointBitmapBox(&s_font, codepoint, scale, scale, &x0, &y0, &x1, &y1);
        if (x1 - x0 > 0 && y1 - y0 > 0 && x1 - x0 <= GLYPH_MAX && y1 - y0 <= GLYPH_MAX) {
            int gw = x1 - x0, gh = y1 - y0;
            int ox = (int)floorf(x + 0.5f) + x0, oy = (int)floorf(baseline + 0.5f) + y0;

            stbtt_MakeCodepointBitmap(&s_font, glyph, gw, gh, gw, scale, scale, codepoint);
            for (int gy = 0; gy < gh; gy++)
                for (int gx = 0; gx < gw; gx++)
                    if (glyph[gy * gw + gx])
                        BlendPixel(s, ox + gx, oy + gy, c, (float)glyph[gy * gw + gx] / 255.0f);
        }
        x += (float)advance * scale;
    }
}

/* Shortens text with "..." to fit width pixels. */
static const char *FitText(const char *text, float em, float width, char *out, size_t size) {
    size_t n;

    snprintf(out, size, "%s", text);
    if (TextWidth(out, em) <= width)
        return out;
    n = strlen(out);
    while (n > 0) {
        out[--n] = '\0';
        if (n + 3 < size) {
            char trial[512];
            snprintf(trial, sizeof(trial), "%s...", out);
            if (TextWidth(trial, em) <= width) {
                snprintf(out, size, "%s", trial);
                return out;
            }
        }
    }
    return out;
}

/* ---- the panel ---- */

typedef struct Painter {
    SettingsSurface *surface;
    const SettingsModel *model;
    float s; /* design units to pixels */
} Painter;

static void PaintFrame(const Painter *p, float width, float height) {
    FillRoundRect(p->surface, 0.0f, 0.0f, width, height, 16.0f * p->s, kBorder);
    FillRoundRect(p->surface, 1.0f, 1.0f, width - 2.0f, height - 2.0f, 15.0f * p->s, kPanel);
}

static void PaintHeader(const Painter *p) {
    const float s = p->s;

    DrawText(p->surface, SETTINGS_PAD * s, 40.0f * s, 26.0f * s, "Settings", kText, ALIGN_LEFT);
    DrawText(p->surface, (SETTINGS_PANEL_W - SETTINGS_PAD) * s, 38.0f * s, 14.0f * s, "GAME PAUSED", kMuted,
             ALIGN_RIGHT);
    for (int page = 0; page < SETTINGS_PAGE_COUNT; page++) {
        SettingsRect tab = SettingsTabRect(page);
        int current = page == p->model->page;

        if (current)
            FillRoundRect(p->surface, tab.x * s, tab.y * s, tab.w * s, tab.h * s, 8.0f * s,
                          WithAlpha(kAccent, 0.18f));
        DrawText(p->surface, (tab.x + tab.w * 0.5f) * s, (tab.y + tab.h * 0.5f + 5.5f) * s, 15.0f * s,
                 SettingsPageName(page), current ? kText : kMuted, ALIGN_CENTRE);
    }
    FillRoundRect(p->surface, SETTINGS_PAD * s, (SETTINGS_HEADER_H - 12) * s,
                  (SETTINGS_PANEL_W - 2 * SETTINGS_PAD) * s, 1.0f, 0.0f, WithAlpha(kText, 0.10f));
}

static void PaintRow(const Painter *p, int row) {
    const float s = p->s;
    float top = SettingsRowTop(p->model, row);
    float mid, baseline, centre = SettingsValueCentre() * s;
    float reach = (SETTINGS_VALUE_W * 0.5f - 14.0f) * s;
    int selected = row == p->model->row;
    Color label;
    SettingsRowView v;
    char fit[256];

    if (top < 0.0f)
        return;
    top *= s;
    mid = top + SETTINGS_ROW_H * 0.5f * s;
    baseline = mid + 6.0f * s;
    label = selected ? kText : WithAlpha(kText, 0.82f);
    SettingsModelRow(p->model, row, &v);
    if (v.disabled)
        label = WithAlpha(kMuted, 0.55f);
    if (selected) {
        FillRoundRect(p->surface, (SETTINGS_PAD / 2) * s, top + 3.0f * s, (SETTINGS_PANEL_W - SETTINGS_PAD) * s,
                      (SETTINGS_ROW_H - 6) * s, 9.0f * s, WithAlpha(kAccent, 0.16f));
        FillRoundRect(p->surface, (SETTINGS_PAD / 2) * s + 4.0f * s, mid - 10.0f * s, 3.0f * s, 20.0f * s,
                      1.5f * s, kAccent);
    }
    DrawText(p->surface, SETTINGS_PAD * s, baseline, 17.0f * s, v.label ? v.label : "",
             v.danger ? kDanger : label, ALIGN_LEFT);
    if (!v.value)
        return;
    FitText(v.value, 17.0f * s, (SETTINGS_VALUE_W - 40) * s, fit, sizeof(fit));
    if (v.disabled) {
        DrawText(p->surface, centre, baseline, 17.0f * s, fit, WithAlpha(kMuted, 0.55f), ALIGN_CENTRE);
    } else if (v.kind == 1) {
        Color chevron = selected ? kAccent : WithAlpha(kMuted, 0.75f);
        DrawChevron(p->surface, centre - reach, mid, 5.0f * s, -1, 2.0f * s, chevron);
        DrawChevron(p->surface, centre + reach, mid, 5.0f * s, 1, 2.0f * s, chevron);
        DrawText(p->surface, centre, baseline, 17.0f * s, fit, selected ? kText : WithAlpha(kText, 0.9f),
                 ALIGN_CENTRE);
    } else if (v.kind == 2) {
        if (v.capturing)
            FillRoundRect(p->surface, centre - SETTINGS_VALUE_W * 0.5f * s, top + 7.0f * s, SETTINGS_VALUE_W * s,
                          (SETTINGS_ROW_H - 14) * s, 7.0f * s, WithAlpha(kAccent, 0.30f));
        DrawText(p->surface, centre, baseline, 17.0f * s, fit,
                 v.capturing ? kText : selected ? kAccent : WithAlpha(kText, 0.9f), ALIGN_CENTRE);
    } else {
        DrawText(p->surface, centre, baseline, 17.0f * s, fit, kMuted, ALIGN_CENTRE);
    }
}

static void PaintScrollbar(const Painter *p) {
    const float s = p->s;
    int content = SettingsContentRows(p->model), visible = SettingsVisibleContentRows(p->model);
    float track_top, track_h, thumb_h, thumb_top;

    if (content <= visible)
        return;
    track_top = (SETTINGS_HEADER_H + 4) * s;
    track_h = (visible * SETTINGS_ROW_H - 8) * s;
    thumb_h = track_h * (float)visible / (float)content;
    thumb_top = track_top + (track_h - thumb_h) * (float)p->model->scroll / (float)(content - visible);
    FillRoundRect(p->surface, (SETTINGS_PANEL_W - 12) * s, track_top, 4.0f * s, track_h, 2.0f * s,
                  WithAlpha(kText, 0.10f));
    FillRoundRect(p->surface, (SETTINGS_PANEL_W - 12) * s, thumb_top, 4.0f * s, thumb_h, 2.0f * s,
                  WithAlpha(kAccent, 0.65f));
}

static void PaintFooter(const Painter *p) {
    const float s = p->s;
    float footer_top = (float)(SettingsDesignHeight(p->model) - SETTINGS_FOOTER_H) * s;
    float width = (SETTINGS_PANEL_W - 2 * SETTINGS_PAD) * s;
    const char *help = SettingsModelHelp(p->model);
    char line[2][256];
    size_t n = 0, cut;

    FillRoundRect(p->surface, SETTINGS_PAD * s, footer_top, width, 1.0f, 0.0f, WithAlpha(kText, 0.10f));
    /* Help wraps to two lines at a word. */
    snprintf(line[0], sizeof(line[0]), "%s", help);
    line[1][0] = '\0';
    if (TextWidth(line[0], 14.0f * s) > width) {
        n = strlen(line[0]);
        while (n > 0 && TextWidth(line[0], 14.0f * s) > width) {
            cut = n;
            while (cut > 0 && line[0][cut - 1] != ' ')
                cut--;
            if (cut == 0)
                break;
            line[0][cut - 1] = '\0';
            n = cut - 1;
        }
        snprintf(line[1], sizeof(line[1]), "%s", help + strlen(line[0]) + (help[strlen(line[0])] == ' '));
    }
    if (line[1][0]) {
        char fit[256];
        DrawText(p->surface, SETTINGS_PAD * s, footer_top + 22.0f * s, 14.0f * s, line[0], kMuted, ALIGN_LEFT);
        DrawText(p->surface, SETTINGS_PAD * s, footer_top + 40.0f * s, 14.0f * s,
                 FitText(line[1], 14.0f * s, width, fit, sizeof(fit)), kMuted, ALIGN_LEFT);
    } else {
        DrawText(p->surface, SETTINGS_PAD * s, footer_top + 28.0f * s, 14.0f * s, line[0], kMuted, ALIGN_LEFT);
    }
    DrawText(p->surface, SETTINGS_PAD * s, footer_top + 62.0f * s, 13.0f * s,
             "Up/Down select   Left/Right change   Tab/PgUp/PgDn page   Esc resume",
             WithAlpha(kMuted, 0.7f), ALIGN_LEFT);
}

void SettingsPaint(SettingsSurface *surface, const SettingsModel *model, const SettingsLayout *layout) {
    Painter painter = {surface, model, layout->scale};
    uint8_t *solid;

    memset(surface->pixels, 0, (size_t)surface->w * (size_t)surface->h * 4u);
    if (SettingsFontReady()) {
        PaintFrame(&painter, SETTINGS_PANEL_W * layout->scale, (float)SettingsDesignHeight(model) * layout->scale);
        PaintHeader(&painter);
        for (int row = 0; row < SettingsRowCount(model); row++)
            PaintRow(&painter, row);
        PaintScrollbar(&painter);
        PaintFooter(&painter);
    }
    solid = surface->pixels + (size_t)(surface->h - 1) * (size_t)surface->w * 4u;
    solid[3] = 255;
}

int SettingsSurfaceDump(const SettingsSurface *surface, const char *path) {
    FILE *file = Psyz_FileOpen(path, "wb");

    if (!file)
        return 0;
    fprintf(file, "P6\n%d %d\n255\n", surface->w, surface->h - 1);
    for (int y = 0; y < surface->h - 1; y++) {
        for (int x = 0; x < surface->w; x++) {
            const uint8_t *px = surface->pixels + ((size_t)y * (size_t)surface->w + (size_t)x) * 4u;
            int keep = 255 - px[3];
            uint8_t rgb[3] = {(uint8_t)(px[0] + 40 * keep / 255), (uint8_t)(px[1] + 40 * keep / 255),
                              (uint8_t)(px[2] + 40 * keep / 255)};
            fwrite(rgb, 1, 3, file);
        }
    }
    fclose(file);
    return 1;
}
