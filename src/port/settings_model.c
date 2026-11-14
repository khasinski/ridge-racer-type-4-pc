#include "settings_model.h"

#include <stdio.h>
#include <string.h>

#define COUNT_OF(a) ((int)(sizeof(a) / sizeof((a)[0])))

/* ---- choice settings ---- */

typedef struct ChoiceSetting {
    const char *label;
    const char *help;
    int count;
    const char *const *names;
    int (*get)(const Rr4Prefs *p);
    void (*set)(Rr4Prefs *p, int index);
    int clamp; /* stops at the ends instead of wrapping (volume) */
    int next_start; /* takes effect when the game starts again */
    unsigned feature; /* SETTINGS_FEATURE_* the renderer must have */
} ChoiceSetting;

#define ACCESSORS(Name, read, write)                               \
    static int Get##Name(const Rr4Prefs *p) { return (read); }     \
    static void Set##Name(Rr4Prefs *p, int i) { write; }
#define SETTING(label, help, names, Name) \
    {label, help, COUNT_OF(names), names, Get##Name, Set##Name, 0, 0, 0}
#define GPU_SETTING(label, help, names, Name, feature) \
    {label, help, COUNT_OF(names), names, Get##Name, Set##Name, 0, 0, feature}

static const char *const kSwitchNames[] = {"Off", "On"};

static const unsigned kScales[] = {1, 2, 4, 6, 8};
static const char *const kScaleNames[] = {"1x (original)", "2x", "4x", "6x", "8x"};
static int ScaleIndex(unsigned scale) {
    for (int i = 0; i < COUNT_OF(kScales); i++)
        if (scale <= kScales[i])
            return i;
    return COUNT_OF(kScales) - 1;
}
ACCESSORS(Scale, ScaleIndex(p->scale), p->scale = kScales[i])

static const char *const kWindowNames[] = {"Windowed", "Fullscreen", "Fullscreen 50 Hz"};
ACCESSORS(Fullscreen, p->fullscreen ? 1 + (p->fullscreen_field_rate != 0) : 0,
          p->fullscreen = i != 0; if (i) p->fullscreen_field_rate = i == 2)

static const char *const kVsyncNames[] = {"Auto", "On", "Off (limiter)"};
ACCESSORS(Vsync, p->vsync >= 0 && p->vsync <= 2 ? p->vsync : 0, p->vsync = i)

static const char *const kVideoTimingNames[] = {"PAL 50 Hz (original)", "NTSC 60 Hz"};
ACCESSORS(VideoTiming, p->video_timing == RR4_VIDEO_NTSC,
          p->video_timing = i ? RR4_VIDEO_NTSC : RR4_VIDEO_PAL)

static const char *const kAspectNames[] = {"Classic 4:3", "Widescreen 16:9"};
ACCESSORS(Aspect, p->aspect_ratio == RR4_ASPECT_WIDESCREEN,
          p->aspect_ratio = i ? RR4_ASPECT_WIDESCREEN : RR4_ASPECT_CLASSIC)

static const char *const kDitherNames[] = {"Original", "Off"};
ACCESSORS(Dither, p->disable_dithering != 0, p->disable_dithering = i)

static const char *const kAntialiasNames[] = {"Off", "4x SSAA"};
ACCESSORS(Antialiasing, p->antialiasing != 0, p->antialiasing = i)

static const char *const kFilterNames[] = {"Original", "Bilinear"};
ACCESSORS(Filtering, p->texture_filtering != 0, p->texture_filtering = i)

ACCESSORS(Geometry, p->geometry_correction != 0, p->geometry_correction = i)

static const char *const kInterpolationNames[] = {"Off (original)", "60 Hz"};
ACCESSORS(Interpolation, p->frame_interpolation != 0, p->frame_interpolation = i)

static const char *const kDrawDistanceNames[] = {"Original", "Extended (2x)"};
ACCESSORS(DrawDistance, p->extended_draw_distance != 0, p->extended_draw_distance = i)

static const ChoiceSetting kGraphics[] = {
    GPU_SETTING("Internal resolution", "Renders 3D at a multiple of the PlayStation resolution. Higher values cost GPU time.",
            kScaleNames, Scale,
            SETTINGS_FEATURE_UPSCALE),
    SETTING("Display mode", "Window or fullscreen (F4). Fullscreen 50 Hz sets a display that has the mode (most 120 Hz ones) to the game's own rate (60 Hz with NTSC video timing) for the smoothest motion.",
            kWindowNames, Fullscreen),
    SETTING("VSync", "Auto uses display sync on 60 Hz screens. Off paces the game's fields (50 Hz PAL, 60 Hz NTSC) with a limiter.",
            kVsyncNames, Vsync),
    {"Video timing", "PAL: this release, races at 25 fps. NTSC: 60 Hz fields and the NTSC release's timing, races at 30 fps and faster. Used from the next start.",
     COUNT_OF(kVideoTimingNames), kVideoTimingNames, GetVideoTiming, SetVideoTiming, 0, 1},
    GPU_SETTING("Aspect ratio", "Classic 4:3 matches a TV. Widescreen 16:9 shows more of the race to the sides; menus stay 4:3.",
            kAspectNames, Aspect,
            SETTINGS_FEATURE_WIDESCREEN),
    SETTING("Draw distance", "Doubles the course view depth and draws more track scenery. Costs CPU and GPU time. Original keeps the PlayStation visibility.",
            kDrawDistanceNames, DrawDistance),
    SETTING("Dithering", "Original: PlayStation 15-bit colour, dithered on shaded polygons. Off: smooth full-colour shading.",
            kDitherNames, Dither),
    GPU_SETTING("Antialiasing", "Smoother edges: twice each axis, four samples per pixel. Costs GPU time and memory.",
            kAntialiasNames, Antialiasing,
            SETTINGS_FEATURE_ANTIALIASING),
    GPU_SETTING("Texture filtering", "Experimental. Bilinear filtering for 3D textures.",
            kFilterNames, Filtering,
            SETTINGS_FEATURE_FILTERING),
    GPU_SETTING("Geometry correction", "Experimental. Subpixel vertices and perspective-correct textures.",
            kSwitchNames, Geometry,
            SETTINGS_FEATURE_GEOMETRY),
    GPU_SETTING("Frame interpolation", "Experimental. Presents in-between frames; game timing stays original.",
            kInterpolationNames, Interpolation,
            SETTINGS_FEATURE_INTERPOLATION),
};

static const char *const kVolumeNames[] = {
    "Off", "5%",  "10%", "15%", "20%", "25%", "30%", "35%", "40%", "45%", "50%",
    "55%", "60%", "65%", "70%", "75%", "80%", "85%", "90%", "95%", "100%"};
ACCESSORS(Volume, (int)((p->volume + 2) / 5 > 20 ? 20 : (p->volume + 2) / 5),
          p->volume = (unsigned)i * 5u)
ACCESSORS(Mute, p->mute != 0, p->mute = i)

static const ChoiceSetting kAudio[] = {
    {"Master volume", "Overall volume of music, voices and sound effects. Music and effect levels are in the game's own options.",
     COUNT_OF(kVolumeNames), kVolumeNames, GetVolume, SetVolume, 1, 0},
    SETTING("Mute", "Silence all output without losing the volume setting.", kSwitchNames, Mute),
};

/* Controls: the controller each port emulates and how the host's analog
 * inputs drive it (the Rage Racer port's choices), then the bindings. */
static int Pad1Type(const Rr4Prefs *p) { return p->pad_type[0]; }
static void SetPad1Type(Rr4Prefs *p, int i) { p->pad_type[0] = i; }
static int Pad2Type(const Rr4Prefs *p) { return p->pad_type[1]; }
static void SetPad2Type(Rr4Prefs *p, int i) { p->pad_type[1] = i; }
static const char *const kPadTypeNames[RR4_PAD_TYPES] = {"Digital pad", "DualShock", "neGcon", "JogCon"};

static const float kLinearity[] = {0.0f, 0.5f, 1.0f, 1.5f, 2.0f};
static const char *const kLinearityNames[] = {"0 (linear)", "0.5", "1.0", "1.5", "2.0"};
static int NearestLinearity(float v) {
    int best = 0;
    for (int i = 1; i < COUNT_OF(kLinearity); i++) {
        float d = kLinearity[i] - v, b = kLinearity[best] - v;
        if (d * d < b * b)
            best = i;
    }
    return best;
}
ACCESSORS(Linearity, NearestLinearity(p->axes[RR4_AXIS_STEERING].linearity),
          p->axes[RR4_AXIS_STEERING].linearity = kLinearity[i])
ACCESSORS(ForceFeedback, p->force_feedback != 0, p->force_feedback = i)

static const ChoiceSetting kControls[] = {
    {"Pad 1 controller", "Digital pad: the original pad. DualShock: the left stick steers. neGcon: the stick twists, right / left trigger press I / II. JogCon: the stick turns the dial.",
     COUNT_OF(kPadTypeNames), kPadTypeNames, Pad1Type, SetPad1Type, 0, 0},
    {"Pad 2 controller", "The second gamepad, as for pad 1. The game's own controller options set it up further.",
     COUNT_OF(kPadTypeNames), kPadTypeNames, Pad2Type, SetPad2Type, 0, 0},
    SETTING("Steering linearity", "neGcon twist and JogCon dial from the left stick: higher values steer more gently around the centre. 0 is linear.",
            kLinearityNames, Linearity),
    SETTING("Force feedback", "Gamepad rumble for the DualShock's and the JogCon's motors (the game's vibration option).",
            kSwitchNames, ForceFeedback),
};

enum {
    CONTROL_ROW_DEVICE = COUNT_OF(kControls), /* Keyboard / Controller list */
    CONTROL_ROW_BINDINGS,                     /* first binding row */
};

/* Controls rows after the device row, in the order a racer reads them. */
static const int kControlOrder[RR4_PREFS_BUTTONS] = {
    12 /* Up */, 14 /* Down */, 15 /* Left */, 13 /* Right */,
    6 /* Cross */, 5 /* Circle */, 7 /* Square */, 4 /* Triangle */,
    2 /* L1 */, 3 /* R1 */, 0 /* L2 */, 1 /* R2 */,
    11 /* Start */, 8 /* Select */, 9 /* L3 */, 10 /* R3 */};

enum {
    DISC_ROW_MOUNTED = 0,
    DISC_ROW_NEXT = 1,
    DISC_ROWS = 2,
};

/* Link cable: the cable's end, the port, the host to join, the connection. */
enum {
    LINK_ROW_MODE = 0,
    LINK_ROW_PORT = 1,
    LINK_ROW_ADDRESS = 2,
    LINK_ROW_STATUS = 3,
    LINK_ROWS = 4,
};

static const char *const kLinkModeNames[] = {"Off", "Host", "Join"};

static const char *const kPageNames[SETTINGS_PAGE_COUNT] = {"Graphics", "Controls", "Audio", "Disc",
                                                            "Link cable", "Replays"};

const char *SettingsPageName(int page) {
    return page >= 0 && page < SETTINGS_PAGE_COUNT ? kPageNames[page] : "";
}

static const ChoiceSetting *PageChoices(int page, int *count) {
    if (page == SETTINGS_PAGE_GRAPHICS) {
        *count = COUNT_OF(kGraphics);
        return kGraphics;
    }
    if (page == SETTINGS_PAGE_AUDIO) {
        *count = COUNT_OF(kAudio);
        return kAudio;
    }
    if (page == SETTINGS_PAGE_CONTROLS) {
        *count = COUNT_OF(kControls);
        return kControls;
    }
    *count = 0;
    return NULL;
}

/* Replays page: Save replay, then one row per file (or a note when none). */
static int ReplayRows(const SettingsModel *m) {
    return 1 + (m->replay_count > 0 ? m->replay_count : 1);
}

static void ActivateReplayRow(SettingsModel *m) {
    if (!m->replays_available)
        return;
    if (m->row == 0) {
        if (m->replay_can_save) {
            m->replay_request = SETTINGS_REPLAY_SAVE;
        } else {
            snprintf(m->status, sizeof(m->status), "%s", m->replay_save_help);
        }
    } else if (m->row - 1 < m->replay_count) {
        m->replay_index = m->row - 1;
        m->replay_request = SETTINGS_REPLAY_LOAD;
    }
    m->dirty = 1;
}

static void ReplayRowView(const SettingsModel *m, int row, SettingsRowView *v) {
    if (row == 0) {
        v->kind = 0;
        v->label = "Save replay";
        v->help = !m->replays_available ? "Replays are not available in this build."
                  : m->replay_can_save   ? "Save the last race's replay to the replays folder."
                                         : m->replay_save_help;
        return;
    }
    if (m->replay_count <= 0) {
        v->kind = 3;
        v->label = "No saved replays";
        v->value = "";
        v->help = "Saved replays appear here. Enter plays one.";
        return;
    }
    v->kind = 0;
    v->label = m->replays[row - 1].label;
    v->value = m->replays[row - 1].value;
    v->help = m->replays[row - 1].help;
    v->danger = !m->replays[row - 1].ok;
}

int SettingsContentRows(const SettingsModel *m) {
    int count;
    switch (m->page) {
    case SETTINGS_PAGE_CONTROLS:
        return CONTROL_ROW_BINDINGS + RR4_PREFS_BUTTONS;
    case SETTINGS_PAGE_DISC:
        return DISC_ROWS;
    case SETTINGS_PAGE_LINK:
        return LINK_ROWS;
    case SETTINGS_PAGE_REPLAYS:
        return ReplayRows(m);
    default:
        PageChoices(m->page, &count);
        return count;
    }
}

int SettingsRowCount(const SettingsModel *m) { return SettingsContentRows(m) + SETTINGS_ACTION_ROWS; }

int SettingsVisibleContentRows(const SettingsModel *m) {
    int n = SettingsContentRows(m);
    return n < SETTINGS_VISIBLE_ROWS ? n : SETTINGS_VISIBLE_ROWS;
}

/* ---- state ---- */

void SettingsModelOpen(SettingsModel *m, const Rr4Prefs *prefs, const char *mounted) {
    memset(m, 0, sizeof(*m));
    m->prefs = *prefs;
    m->mounted = mounted;
    m->open = 1;
    m->dirty = 1;
}

void SettingsModelClose(SettingsModel *m) {
    m->capturing = 0;
    m->editing = 0;
    m->open = 0;
    m->dirty = 1;
}

static void Changed(SettingsModel *m) {
    m->changed = 1;
    m->edited = 1;
    m->dirty = 1;
}

int SettingsModelTakeChanged(SettingsModel *m) {
    int changed = m->changed;
    m->changed = 0;
    return changed;
}

static void KeepVisible(SettingsModel *m) {
    int content = SettingsContentRows(m), visible = SettingsVisibleContentRows(m);
    int max = content - visible;

    if (m->row < content) {
        if (m->row < m->scroll)
            m->scroll = m->row;
        if (m->row >= m->scroll + visible)
            m->scroll = m->row - visible + 1;
    }
    if (m->scroll > max)
        m->scroll = max;
    if (m->scroll < 0)
        m->scroll = 0;
}

void SettingsModelSelect(SettingsModel *m, int row) {
    if (row < 0 || row >= SettingsRowCount(m) || row == m->row)
        return;
    m->row = row;
    m->status[0] = '\0';
    KeepVisible(m);
    m->dirty = 1;
}

void SettingsModelSwitchPage(SettingsModel *m, int page) {
    page = (page % SETTINGS_PAGE_COUNT + SETTINGS_PAGE_COUNT) % SETTINGS_PAGE_COUNT;
    if (page == m->page)
        return;
    m->page = page;
    m->row = 0;
    m->scroll = 0;
    m->capturing = 0;
    m->editing = 0;
    m->status[0] = '\0';
    m->dirty = 1;
}

static int *DeviceBindings(Rr4Prefs *p, int device) {
    return device == SETTINGS_DEVICE_CONTROLLER ? p->buttons : p->keys;
}

static void ResetPage(SettingsModel *m) {
    const Rr4Prefs *d = Rr4PrefsDefaults();
    Rr4Prefs before = m->prefs;
    Rr4Prefs *p = &m->prefs;

    if (m->page == SETTINGS_PAGE_REPLAYS) {
        snprintf(m->status, sizeof(m->status), "Nothing to restore: saved replays stay in the replays folder.");
        m->dirty = 1;
        return;
    }
    switch (m->page) {
    case SETTINGS_PAGE_GRAPHICS:
        p->scale = d->scale;
        p->fullscreen = d->fullscreen;
        p->fullscreen_field_rate = d->fullscreen_field_rate;
        p->vsync = d->vsync;
        p->video_timing = d->video_timing;
        p->aspect_ratio = d->aspect_ratio;
        p->disable_dithering = d->disable_dithering;
        p->antialiasing = d->antialiasing;
        p->texture_filtering = d->texture_filtering;
        p->geometry_correction = d->geometry_correction;
        p->extended_draw_distance = d->extended_draw_distance;
        p->frame_interpolation = d->frame_interpolation;
        break;
    case SETTINGS_PAGE_CONTROLS:
        memcpy(DeviceBindings(p, m->device), DeviceBindings((Rr4Prefs *)d, m->device),
               sizeof(p->keys));
        memcpy(p->pad_type, d->pad_type, sizeof(p->pad_type));
        p->force_feedback = d->force_feedback;
        memcpy(p->axes, d->axes, sizeof(p->axes));
        break;
    case SETTINGS_PAGE_AUDIO:
        p->volume = d->volume;
        p->mute = d->mute;
        break;
    case SETTINGS_PAGE_DISC:
        p->disc[0] = '\0';
        break;
    case SETTINGS_PAGE_LINK:
        p->link_mode = d->link_mode;
        p->link_port = d->link_port;
        snprintf(p->link_address, sizeof(p->link_address), "%s", d->link_address);
        break;
    default:
        break;
    }
    snprintf(m->status, sizeof(m->status), "%s restored to defaults.",
             m->page == SETTINGS_PAGE_CONTROLS
                 ? (m->device == SETTINGS_DEVICE_CONTROLLER ? "Controller bindings" : "Keyboard bindings")
                 : SettingsPageName(m->page));
    if (memcmp(&before, p, sizeof(before)))
        Changed(m);
    m->dirty = 1;
}

static void StepChoice(SettingsModel *m, int row, int direction) {
    int count;
    const ChoiceSetting *settings = PageChoices(m->page, &count);
    const ChoiceSetting *s;
    int index, next;

    if (!settings || row < 0 || row >= count)
        return;
    s = &settings[row];
    if (s->feature & m->unavailable) {
        snprintf(m->status, sizeof(m->status), "%s", m->unavailable_note ? m->unavailable_note : "Not available.");
        m->dirty = 1;
        return;
    }
    index = s->get(&m->prefs);
    if (s->clamp) {
        next = index + direction;
        if (next < 0)
            next = 0;
        if (next >= s->count)
            next = s->count - 1;
    } else {
        next = (index + direction + s->count) % s->count;
    }
    if (next == index)
        return;
    s->set(&m->prefs, next);
    m->status[0] = '\0';
    if (s->next_start)
        snprintf(m->status, sizeof(m->status), "%s: %s from the next start.", s->label, s->names[next]);
    Changed(m);
}

static void StepLinkMode(SettingsModel *m, int direction) {
    int mode = m->prefs.link_mode >= RR4_LINK_OFF && m->prefs.link_mode <= RR4_LINK_JOIN ? m->prefs.link_mode : 0;
    mode = (mode + direction + 3) % 3;
    m->prefs.link_mode = mode;
    m->status[0] = '\0';
    if (mode == RR4_LINK_JOIN && !m->prefs.link_address[0])
        snprintf(m->status, sizeof(m->status), "Enter the host's address below to join it.");
    Changed(m);
}

/* Enter on a text row: keep a valid port or address. */
static void CommitEdit(SettingsModel *m) {
    m->editing = 0;
    m->dirty = 1;
    if (m->row == LINK_ROW_PORT) {
        unsigned long port = 0;
        const char *c = m->edit;
        for (; *c >= '0' && *c <= '9' && port <= 65535; c++)
            port = port * 10 + (unsigned long)(*c - '0');
        if (*c || port < 1 || port > 65535) {
            snprintf(m->status, sizeof(m->status), "A port is a number from 1 to 65535.");
            return;
        }
        if (m->prefs.link_port != (unsigned)port) {
            m->prefs.link_port = (unsigned)port;
            Changed(m);
        }
        return;
    }
    if (strcmp(m->prefs.link_address, m->edit)) {
        snprintf(m->prefs.link_address, sizeof(m->prefs.link_address), "%s", m->edit);
        Changed(m);
    }
}

static void Activate(SettingsModel *m) {
    int content = SettingsContentRows(m);

    if (m->row == content) {
        ResetPage(m);
        return;
    }
    if (m->row == content + 1) {
        SettingsModelClose(m);
        return;
    }
    if (m->row == content + 2) {
        m->quit = 1;
        SettingsModelClose(m);
        return;
    }
    switch (m->page) {
    case SETTINGS_PAGE_CONTROLS:
        if (m->row < CONTROL_ROW_DEVICE) {
            StepChoice(m, m->row, 1);
        } else if (m->row == CONTROL_ROW_DEVICE) {
            m->device = !m->device;
            m->dirty = 1;
        } else {
            m->capturing = 1;
            m->status[0] = '\0';
            m->dirty = 1;
        }
        break;
    case SETTINGS_PAGE_REPLAYS:
        ActivateReplayRow(m);
        break;
    case SETTINGS_PAGE_DISC:
        if (m->row == DISC_ROW_NEXT) {
            m->choose_disc = 1;
            snprintf(m->status, sizeof(m->status), "Choose the image's CUE file.");
            m->dirty = 1;
        }
        break;
    case SETTINGS_PAGE_LINK:
        if (m->row == LINK_ROW_MODE) {
            StepLinkMode(m, 1);
        } else if (m->row == LINK_ROW_PORT || m->row == LINK_ROW_ADDRESS) {
            m->editing = 1;
            if (m->row == LINK_ROW_PORT)
                snprintf(m->edit, sizeof(m->edit), "%u", m->prefs.link_port);
            else
                snprintf(m->edit, sizeof(m->edit), "%s", m->prefs.link_address);
            m->status[0] = '\0';
            m->dirty = 1;
        }
        break;
    default:
        StepChoice(m, m->row, 1);
        break;
    }
}

static void Step(SettingsModel *m, int direction) {
    if (m->row >= SettingsContentRows(m))
        return;
    if (m->page == SETTINGS_PAGE_CONTROLS) {
        if (m->row < CONTROL_ROW_DEVICE) {
            StepChoice(m, m->row, direction);
        } else if (m->row == CONTROL_ROW_DEVICE) {
            m->device = !m->device;
            m->dirty = 1;
        }
        return;
    }
    if (m->page == SETTINGS_PAGE_DISC) {
        if (m->row == DISC_ROW_NEXT)
            Activate(m);
        return;
    }
    if (m->page == SETTINGS_PAGE_LINK) {
        if (m->row == LINK_ROW_MODE)
            StepLinkMode(m, direction);
        return;
    }
    if (m->page == SETTINGS_PAGE_REPLAYS)
        return;
    StepChoice(m, m->row, direction);
}

static void ClearBinding(SettingsModel *m) {
    int button, *bindings;
    const int *defaults;

    if (m->page != SETTINGS_PAGE_CONTROLS || m->row < CONTROL_ROW_BINDINGS ||
        m->row >= CONTROL_ROW_BINDINGS + RR4_PREFS_BUTTONS)
        return;
    button = kControlOrder[m->row - CONTROL_ROW_BINDINGS];
    bindings = DeviceBindings(&m->prefs, m->device);
    defaults = DeviceBindings((Rr4Prefs *)Rr4PrefsDefaults(), m->device);
    if (bindings[button] == defaults[button])
        return;
    /* As a rebind to the default: whoever holds it gets ours. */
    for (int i = 0; i < RR4_PREFS_BUTTONS; i++)
        if (i != button && bindings[i] == defaults[button])
            bindings[i] = bindings[button];
    bindings[button] = defaults[button];
    snprintf(m->status, sizeof(m->status), "%s restored to its default.", Rr4PrefsButtonLabel(button));
    Changed(m);
}

void SettingsModelDo(SettingsModel *m, SettingsAction action) {
    int rows = SettingsRowCount(m);

    if (!m->open)
        return;
    if (m->editing) {
        if (action == SETTINGS_ACTIVATE) {
            CommitEdit(m);
        } else if (action == SETTINGS_BACK) {
            m->editing = 0;
            m->dirty = 1;
        } else if (action == SETTINGS_CLEAR && m->edit[0]) {
            m->edit[strlen(m->edit) - 1] = '\0';
            m->dirty = 1;
        }
        return;
    }
    if (m->capturing) {
        if (action == SETTINGS_BACK) {
            m->capturing = 0;
            m->dirty = 1;
        }
        return;
    }
    switch (action) {
    case SETTINGS_UP:
        SettingsModelSelect(m, (m->row + rows - 1) % rows);
        break;
    case SETTINGS_DOWN:
        SettingsModelSelect(m, (m->row + 1) % rows);
        break;
    case SETTINGS_LEFT:
        Step(m, -1);
        break;
    case SETTINGS_RIGHT:
        Step(m, 1);
        break;
    case SETTINGS_PAGE_PREV:
        SettingsModelSwitchPage(m, m->page - 1);
        break;
    case SETTINGS_PAGE_NEXT:
        SettingsModelSwitchPage(m, m->page + 1);
        break;
    case SETTINGS_ACTIVATE:
        Activate(m);
        break;
    case SETTINGS_BACK:
        SettingsModelClose(m);
        break;
    case SETTINGS_CLEAR:
        ClearBinding(m);
        break;
    case SETTINGS_NONE:
        break;
    }
}

static int Capture(SettingsModel *m, int device, int binding) {
    int button, *bindings, previous;

    if (!m->open || !m->capturing || m->device != device || m->page != SETTINGS_PAGE_CONTROLS ||
        m->row < CONTROL_ROW_BINDINGS || m->row >= CONTROL_ROW_BINDINGS + RR4_PREFS_BUTTONS)
        return 0;
    button = kControlOrder[m->row - CONTROL_ROW_BINDINGS];
    bindings = DeviceBindings(&m->prefs, device);
    previous = bindings[button];
    m->capturing = 0;
    m->dirty = 1;
    /* Swap a duplicate, so every pad button stays reachable. */
    for (int i = 0; i < RR4_PREFS_BUTTONS; i++) {
        if (i != button && bindings[i] == binding) {
            bindings[i] = previous;
            snprintf(m->status, sizeof(m->status), "Swapped with %s.", Rr4PrefsButtonLabel(i));
        }
    }
    if (previous != binding) {
        bindings[button] = binding;
        Changed(m);
    }
    return 1;
}

int SettingsModelCaptureKey(SettingsModel *m, int scancode) {
    /* Escape belongs to the menu (PsyZ refuses it as a binding too). */
    if (scancode <= 0 || scancode == 41 /* SDL_SCANCODE_ESCAPE */ || scancode >= 512)
        return 0;
    return Capture(m, SETTINGS_DEVICE_KEYBOARD, scancode);
}

int SettingsModelCaptureButton(SettingsModel *m, int binding) {
    if (binding < 0)
        return 0;
    return Capture(m, SETTINGS_DEVICE_CONTROLLER, binding);
}

static const char *BaseName(const char *path) {
    const char *name = path;
    for (const char *c = path; c && *c; c++)
        if (*c == '/' || *c == '\\')
            name = c + 1;
    return name;
}

static int EndsWithCue(const char *path) {
    size_t n = strlen(path);
    if (n < 4)
        return 0;
    path += n - 4;
    return path[0] == '.' && (path[1] | 32) == 'c' && (path[2] | 32) == 'u' && (path[3] | 32) == 'e';
}

void SettingsModelDiscChosen(SettingsModel *m, const char *path) {
    m->choose_disc = 0;
    m->dirty = 1;
    if (!path || !*path) {
        snprintf(m->status, sizeof(m->status), "No image chosen.");
        return;
    }
    if (!EndsWithCue(path)) {
        snprintf(m->status, sizeof(m->status), "Choose the CUE file of the image, not its BIN.");
        return;
    }
    if (strlen(path) >= sizeof(m->prefs.disc)) {
        snprintf(m->status, sizeof(m->status), "That path is too long.");
        return;
    }
    if (m->check_disc && !m->check_disc(path, m->status, sizeof(m->status)))
        return;
    if (strcmp(m->prefs.disc, path)) {
        snprintf(m->prefs.disc, sizeof(m->prefs.disc), "%s", path);
        Changed(m);
    }
    snprintf(m->status, sizeof(m->status), "%.120s is used from the next start.", BaseName(path));
}

int SettingsModelTextInput(SettingsModel *m, const char *text) {
    size_t n;
    if (!m->open || !m->editing || !text)
        return 0;
    n = strlen(m->edit);
    for (; *text; text++) {
        char c = *text;
        int ok = m->row == LINK_ROW_PORT ? (c >= '0' && c <= '9')
                                         : ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
                                            (c >= 'A' && c <= 'Z') || c == '.' || c == '-' || c == ':' ||
                                            c == '[' || c == ']' || c == '_' || c == '%');
        if (ok && n + 1 < (m->row == LINK_ROW_PORT ? 6 : sizeof(m->edit)))
            m->edit[n++] = c;
    }
    m->edit[n] = '\0';
    m->dirty = 1;
    return 1;
}

void SettingsModelSetLinkStatus(SettingsModel *m, const char *brief, const char *status) {
    if (!brief || !status || (!strcmp(m->link_brief, brief) && !strcmp(m->link_status, status)))
        return;
    snprintf(m->link_brief, sizeof(m->link_brief), "%s", brief);
    snprintf(m->link_status, sizeof(m->link_status), "%s", status);
    if (m->page == SETTINGS_PAGE_LINK)
        m->dirty = 1;
}

/* ---- views ---- */

void SettingsModelRow(const SettingsModel *m, int row, SettingsRowView *v) {
    static char help[256];
    int content = SettingsContentRows(m);

    memset(v, 0, sizeof(*v));
    if (row >= content) {
        v->kind = 0;
        if (row == content) {
            v->label = "Restore page defaults";
            v->help = m->page == SETTINGS_PAGE_CONTROLS
                          ? "Restore the controllers and the default bindings of the listed device."
                          : m->page == SETTINGS_PAGE_DISC ? "Forget the saved image; the next start asks for one."
                                                          : "Restore this page's settings only.";
        } else if (row == content + 1) {
            v->label = "Resume game";
            v->help = "Close the menu and continue. Settings are saved.";
        } else {
            v->label = "Quit game";
            v->help = "Save the settings and close the game.";
            v->danger = 1;
        }
        return;
    }
    switch (m->page) {
    case SETTINGS_PAGE_CONTROLS:
        if (row < CONTROL_ROW_DEVICE) {
            int index = kControls[row].get(&m->prefs);
            if (index < 0 || index >= kControls[row].count)
                index = 0;
            v->kind = 1;
            v->label = kControls[row].label;
            v->value = kControls[row].names[index];
            v->help = kControls[row].help;
        } else if (row == CONTROL_ROW_DEVICE) {
            v->kind = 1;
            v->label = "Input device";
            v->value = m->device == SETTINGS_DEVICE_CONTROLLER ? "Controller" : "Keyboard";
            v->help = "Choose whose bindings are listed. Pad 1 reads both.";
        } else {
            int button = kControlOrder[row - CONTROL_ROW_BINDINGS];
            int keyboard = m->device == SETTINGS_DEVICE_KEYBOARD;
            const Rr4Prefs *d = Rr4PrefsDefaults();
            int binding = keyboard ? m->prefs.keys[button] : m->prefs.buttons[button];
            int fallback = keyboard ? d->keys[button] : d->buttons[button];

            v->kind = 2;
            v->label = Rr4PrefsButtonLabel(button);
            v->capturing = m->capturing && row == m->row;
            v->value = v->capturing ? (keyboard ? "Press a key..." : "Press a button...")
                       : keyboard  ? Rr4PrefsKeyName(binding)
                                   : Rr4PrefsGamepadName(binding);
            snprintf(help, sizeof(help), "Default: %s. Enter rebinds, Backspace restores the default.",
                     keyboard ? Rr4PrefsKeyName(fallback) : Rr4PrefsGamepadName(fallback));
            if (v->capturing)
                snprintf(help, sizeof(help), "%s",
                         keyboard ? "Press the new key. Escape cancels."
                                  : "Press the new button or trigger. Escape cancels.");
            v->help = help;
        }
        return;
    case SETTINGS_PAGE_LINK: {
        static char value[160];
        int mode = m->prefs.link_mode >= RR4_LINK_OFF && m->prefs.link_mode <= RR4_LINK_JOIN ? m->prefs.link_mode : 0;
        v->kind = 1;
        if (row == LINK_ROW_MODE) {
            v->label = "Link cable";
            v->value = kLinkModeNames[mode];
            v->help = "The two-console link battle over the network. Host waits for the other player, "
                      "Join connects to the host. It shows in the main menu once both are there.";
        } else if (row == LINK_ROW_PORT || row == LINK_ROW_ADDRESS) {
            int port = row == LINK_ROW_PORT;
            v->kind = 2; /* typed, not stepped */
            v->label = port ? "Port" : "Host address";
            v->capturing = m->editing && row == m->row;
            if (v->capturing)
                snprintf(value, sizeof(value), "%.150s_", m->edit);
            else if (port)
                snprintf(value, sizeof(value), "%u", m->prefs.link_port);
            else
                snprintf(value, sizeof(value), "%.150s", m->prefs.link_address[0] ? m->prefs.link_address : "None");
            v->value = value;
            v->help = v->capturing ? "Type, then Enter keeps it and Escape cancels. Ctrl+V pastes."
                      : port ? "TCP port the host waits on and the other player connects to. Enter edits."
                             : "Name or IP address of the host to join. Enter edits.";
        } else {
            v->kind = 3;
            v->label = "Status";
            v->value = m->link_brief[0] ? m->link_brief : "Off";
            snprintf(help, sizeof(help), "%.240s", m->link_status[0] ? m->link_status : "Off: no cable.");
            v->help = help;
        }
        return;
    }
    case SETTINGS_PAGE_DISC:
        v->kind = row == DISC_ROW_NEXT ? 1 : 3;
        if (row == DISC_ROW_MOUNTED) {
            v->label = "In the drive";
            v->value = m->mounted && *m->mounted ? BaseName(m->mounted) : "No image";
            snprintf(help, sizeof(help), "%.240s", m->mounted && *m->mounted ? m->mounted : "No image is mounted.");
        } else {
            v->label = "Image at start";
            v->value = m->prefs.disc[0] ? BaseName(m->prefs.disc) : "Ask / command line";
            snprintf(help, sizeof(help), "%.240s",
                     m->prefs.disc[0] ? m->prefs.disc
                                      : "Enter chooses a CUE file. A path on the command line or in RR4_DISC wins.");
        }
        v->help = help;
        return;
    case SETTINGS_PAGE_REPLAYS:
        ReplayRowView(m, row, v);
        return;
    default: {
        int count;
        const ChoiceSetting *s = PageChoices(m->page, &count);
        int index;
        if (!s || row >= count)
            return;
        index = s[row].get(&m->prefs);
        if (index < 0)
            index = 0;
        if (index >= s[row].count)
            index = s[row].count - 1;
        v->kind = 1;
        v->label = s[row].label;
        v->value = s[row].names[index];
        v->help = s[row].help;
        if (s[row].feature & m->unavailable) {
            v->disabled = 1;
            v->help = m->unavailable_note ? m->unavailable_note : "Not available.";
        }
        return;
    }
    }
}

const char *SettingsModelHelp(const SettingsModel *m) {
    SettingsRowView v;
    if (m->status[0])
        return m->status;
    SettingsModelRow(m, m->row, &v);
    return v.help ? v.help : "";
}

/* ---- geometry ---- */

static int ShownRows(const SettingsModel *m) {
    return SettingsVisibleContentRows(m) + SETTINGS_ACTION_ROWS;
}

int SettingsDesignHeight(const SettingsModel *m) {
    return SETTINGS_HEADER_H + ShownRows(m) * SETTINGS_ROW_H + SETTINGS_ACTION_GAP + SETTINGS_FOOTER_H;
}

void SettingsComputeLayout(const SettingsModel *m, int window_w, int window_h, SettingsLayout *l) {
    float scale = (float)window_h / (float)SETTINGS_REFERENCE_H;
    float height_limit = (float)(window_h - 24) / (float)SettingsDesignHeight(m);
    float width_limit = (float)window_w * 0.94f / (float)SETTINGS_PANEL_W;

    if (scale > height_limit)
        scale = height_limit;
    if (scale > width_limit)
        scale = width_limit;
    if (scale > 2.5f)
        scale = 2.5f;
    if (scale < 0.5f)
        scale = 0.5f;
    scale = (float)(int)(scale * 16.0f) / 16.0f; /* no repaint on every resize pixel */
    l->scale = scale;
    l->w = (int)((float)SETTINGS_PANEL_W * scale + 0.999f);
    l->h = (int)((float)SettingsDesignHeight(m) * scale + 0.999f);
    l->x = (window_w - l->w) / 2;
    l->y = (window_h - l->h) / 2;
}

float SettingsRowTop(const SettingsModel *m, int row) {
    int content = SettingsContentRows(m), visible = SettingsVisibleContentRows(m);
    int slot;

    if (row < content) {
        slot = row - m->scroll;
        if (slot < 0 || slot >= visible)
            return -1.0f;
        return (float)(SETTINGS_HEADER_H + slot * SETTINGS_ROW_H);
    }
    slot = visible + (row - content);
    return (float)(SETTINGS_HEADER_H + slot * SETTINGS_ROW_H + SETTINGS_ACTION_GAP);
}

SettingsRect SettingsTabRect(int page) {
    float width = (float)(SETTINGS_PANEL_W - 2 * SETTINGS_PAD) / (float)SETTINGS_PAGE_COUNT;
    SettingsRect r = {(float)SETTINGS_PAD + width * (float)page, (float)SETTINGS_TAB_TOP, width,
                      (float)SETTINGS_TAB_H};
    return r;
}

float SettingsValueCentre(void) {
    return (float)(SETTINGS_PANEL_W - SETTINGS_PAD) - (float)SETTINGS_VALUE_W * 0.5f;
}

/* ---- pointer ---- */

void SettingsModelPointer(SettingsModel *m, float x, float y, int click) {
    int rows = SettingsRowCount(m);

    if (!m->open || m->capturing || m->editing)
        return;
    if (x < 0.0f || y < 0.0f || x > (float)SETTINGS_PANEL_W || y > (float)SettingsDesignHeight(m)) {
        if (click)
            SettingsModelClose(m);
        return;
    }
    for (int page = 0; page < SETTINGS_PAGE_COUNT; page++) {
        SettingsRect t = SettingsTabRect(page);
        if (x >= t.x && x < t.x + t.w && y >= t.y && y < t.y + t.h) {
            if (click)
                SettingsModelSwitchPage(m, page);
            return;
        }
    }
    for (int row = 0; row < rows; row++) {
        float top = SettingsRowTop(m, row);
        if (top < 0.0f || y < top || y >= top + (float)SETTINGS_ROW_H)
            continue;
        SettingsModelSelect(m, row);
        if (!click)
            return;
        if (row < SettingsContentRows(m) && m->page != SETTINGS_PAGE_DISC && m->page != SETTINGS_PAGE_REPLAYS &&
            (m->page != SETTINGS_PAGE_LINK || row == LINK_ROW_MODE) &&
            (m->page != SETTINGS_PAGE_CONTROLS || row < CONTROL_ROW_DEVICE))
            Step(m, x < SettingsValueCentre() ? -1 : 1);
        else
            Activate(m);
        return;
    }
}

void SettingsModelWheel(SettingsModel *m, int step) {
    int max = SettingsContentRows(m) - SettingsVisibleContentRows(m);
    int next = m->scroll + step;

    if (next < 0)
        next = 0;
    if (next > max)
        next = max;
    if (next != m->scroll) {
        m->scroll = next;
        m->dirty = 1;
    }
}
