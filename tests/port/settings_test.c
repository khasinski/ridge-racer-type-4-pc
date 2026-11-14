/* Headless checks of the Escape settings menu: preferences file round trip
 * and precedence, the model's input state machine and option application,
 * and the panel raster (written as PPMs into the directory argv[1]).
 * Run with PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy. */

#include <SDL3/SDL.h>
#include <psyz.h>
#include <psyz/audio.h>
#include <psyz/overlay_sdl3.h>
#include <psyz/video.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "link.h"
#include "preferences.h"
#include "rr/draw_distance.h"
#include "settings_model.h"
#include "settings_raster.h"

static int failures;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                      \
        }                                                                    \
    } while (0)

enum { BTN_L2 = 0, BTN_CROSS = 6, BTN_UP = 12 };
/* Controls page: four controller rows, the device row, the bindings. */
enum { CONTROLS_DEVICE = 4, CONTROLS_BINDINGS = 5 };

static void Do(SettingsModel *m, SettingsAction a, int times) {
    while (times-- > 0)
        SettingsModelDo(m, a);
}

static void TestDefaults(void) {
    const Rr4Prefs *d = Rr4PrefsDefaults();
    CHECK(d->scale == 1);
    CHECK(d->volume == 100);
    CHECK(!d->mute && !d->fullscreen && !d->frame_interpolation &&
          d->aspect_ratio == RR4_ASPECT_CLASSIC);
    CHECK(d->vsync == RR4_VSYNC_AUTO);
    CHECK(d->video_timing == RR4_VIDEO_PAL);
    CHECK(!d->extended_draw_distance);
    CHECK(d->pad_type[0] == RR4_PAD_DIGITAL && d->pad_type[1] == RR4_PAD_DIGITAL);
    CHECK(d->force_feedback == 1);
    CHECK(d->axes[RR4_AXIS_STEERING].linearity == 0.5f && d->axes[RR4_AXIS_THROTTLE].linearity == 0.0f);
    for (int i = 0; i < RR4_AXES; i++)
        CHECK(d->axes[i].deadzone == 0.0f && d->axes[i].saturation == 1.0f && d->axes[i].scaling == 1.0f);
    for (int i = 0; i < RR4_PREFS_BUTTONS; i++) {
        CHECK(d->keys[i] == Psyz_GetKeyboardBinding_SDL3((unsigned)i));
        CHECK(d->buttons[i] == Psyz_GetGamepadBinding_SDL3((unsigned)i));
    }
}

static void TestGraphicsPage(void) {
    SettingsModel m;
    SettingsModelOpen(&m, Rr4PrefsDefaults(), "/discs/rr4.cue");
    CHECK(m.open && m.page == SETTINGS_PAGE_GRAPHICS && m.row == 0);
    Do(&m, SETTINGS_RIGHT, 3); /* 1x -> 2x -> 4x -> 6x */
    CHECK(m.prefs.scale == 6);
    CHECK(SettingsModelTakeChanged(&m) == 1);
    CHECK(SettingsModelTakeChanged(&m) == 0);
    Do(&m, SETTINGS_RIGHT, 2); /* 8x, wraps to 1x */
    CHECK(m.prefs.scale == 1);
    Do(&m, SETTINGS_LEFT, 1);
    CHECK(m.prefs.scale == 8);
    Do(&m, SETTINGS_DOWN, 1);
    Do(&m, SETTINGS_ACTIVATE, 1); /* display mode */
    CHECK(m.prefs.fullscreen == 1);
    Do(&m, SETTINGS_DOWN, 1);
    Do(&m, SETTINGS_RIGHT, 2);
    CHECK(m.prefs.vsync == RR4_VSYNC_OFF);
    /* Video timing: used from the next start, the status says so. */
    Do(&m, SETTINGS_DOWN, 1);
    Do(&m, SETTINGS_RIGHT, 1);
    CHECK(m.prefs.video_timing == RR4_VIDEO_NTSC);
    CHECK(strstr(SettingsModelHelp(&m), "next start") != NULL);
    Do(&m, SETTINGS_UP, 1);
    /* Up from the first row wraps to Quit game. */
    Do(&m, SETTINGS_UP, 3);
    CHECK(m.row == SettingsRowCount(&m) - 1);
    {
        SettingsRowView v;
        SettingsModelRow(&m, m.row, &v);
        CHECK(v.danger && !strcmp(v.label, "Quit game"));
    }
    /* Draw distance is available to both rasterizers and applies live. */
    SettingsModelSelect(&m, 5);
    Do(&m, SETTINGS_RIGHT, 1);
    CHECK(m.prefs.extended_draw_distance == 1);
    /* Restore page defaults. */
    SettingsModelSelect(&m, SettingsContentRows(&m));
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.prefs.scale == 1 && !m.prefs.fullscreen && m.prefs.vsync == RR4_VSYNC_AUTO &&
          m.prefs.video_timing == RR4_VIDEO_PAL && !m.prefs.extended_draw_distance);
    CHECK(m.open);
    /* Back closes. */
    Do(&m, SETTINGS_BACK, 1);
    CHECK(!m.open && !m.quit);
}

/* A renderer without the GPU enhancements greys their rows out: they keep
 * their value, do not change and say why. */
static void TestUnavailable(void) {
    SettingsModel m;
    SettingsRowView v;
    Rr4Prefs prefs = *Rr4PrefsDefaults();

    prefs.scale = 4;
    SettingsModelOpen(&m, &prefs, NULL);
    m.unavailable = SETTINGS_FEATURE_UPSCALE | SETTINGS_FEATURE_INTERPOLATION;
    m.unavailable_note = "Needs the SDL GPU renderer.";
    SettingsModelRow(&m, 0, &v); /* internal resolution */
    CHECK(v.disabled && !strcmp(v.value, "4x") && !strcmp(v.help, m.unavailable_note));
    Do(&m, SETTINGS_RIGHT, 1);
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.prefs.scale == 4 && !SettingsModelTakeChanged(&m));
    CHECK(!strcmp(SettingsModelHelp(&m), m.unavailable_note));
    Do(&m, SETTINGS_DOWN, 1); /* display mode: no GPU feature */
    SettingsModelRow(&m, m.row, &v);
    CHECK(!v.disabled);
    Do(&m, SETTINGS_RIGHT, 1);
    CHECK(m.prefs.fullscreen == 1);
    SettingsModelRow(&m, SettingsContentRows(&m) - 1, &v); /* frame interpolation */
    CHECK(v.disabled && !strcmp(v.label, "Frame interpolation"));
    SettingsModelRow(&m, SettingsContentRows(&m) - 2, &v); /* geometry correction */
    CHECK(!v.disabled);
}

static void TestControlsPage(void) {
    SettingsModel m;
    const Rr4Prefs *d = Rr4PrefsDefaults();
    SettingsRowView v;

    SettingsModelOpen(&m, d, NULL);
    Do(&m, SETTINGS_PAGE_NEXT, 1);
    CHECK(m.page == SETTINGS_PAGE_CONTROLS);
    CHECK(SettingsContentRows(&m) == CONTROLS_BINDINGS + RR4_PREFS_BUTTONS);
    /* Controller types: digital by default, left / right cycles. */
    SettingsModelRow(&m, 0, &v);
    CHECK(v.kind == 1 && !strcmp(v.label, "Pad 1 controller") && !strcmp(v.value, "Digital pad"));
    Do(&m, SETTINGS_RIGHT, 2);
    CHECK(m.prefs.pad_type[0] == RR4_PAD_NEGCON);
    Do(&m, SETTINGS_LEFT, 3);
    CHECK(m.prefs.pad_type[0] == RR4_PAD_JOGCON);
    SettingsModelRow(&m, 0, &v);
    CHECK(!strcmp(v.value, "JogCon"));
    Do(&m, SETTINGS_DOWN, 1);
    Do(&m, SETTINGS_ACTIVATE, 1); /* Enter steps a choice */
    CHECK(m.prefs.pad_type[1] == RR4_PAD_DUALSHOCK);
    Do(&m, SETTINGS_DOWN, 1);
    SettingsModelRow(&m, m.row, &v);
    CHECK(!strcmp(v.label, "Steering linearity") && !strcmp(v.value, "0.5"));
    Do(&m, SETTINGS_LEFT, 1);
    CHECK(m.prefs.axes[RR4_AXIS_STEERING].linearity == 0.0f);
    Do(&m, SETTINGS_DOWN, 1);
    Do(&m, SETTINGS_LEFT, 1);
    CHECK(!m.prefs.force_feedback);
    CHECK(SettingsModelTakeChanged(&m));
    /* Restore page defaults also restores the controllers. */
    SettingsModelSelect(&m, SettingsContentRows(&m));
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.prefs.pad_type[0] == RR4_PAD_DIGITAL && m.prefs.pad_type[1] == RR4_PAD_DIGITAL);
    CHECK(m.prefs.force_feedback && m.prefs.axes[RR4_AXIS_STEERING].linearity == 0.5f);
    SettingsModelSelect(&m, CONTROLS_DEVICE);
    Do(&m, SETTINGS_DOWN, 1); /* Up binding */
    SettingsModelRow(&m, m.row, &v);
    CHECK(v.kind == 2 && !strcmp(v.label, "Up"));
    CHECK(strstr(v.help, "Default:") != NULL);
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.capturing);
    /* Navigation is ignored while capturing; gamepad input for the keyboard
     * list too. */
    Do(&m, SETTINGS_DOWN, 1);
    CHECK(m.row == CONTROLS_BINDINGS && m.capturing);
    CHECK(SettingsModelCaptureButton(&m, SDL_GAMEPAD_BUTTON_SOUTH) == 0);
    /* Escape cancels the capture, not the menu. */
    Do(&m, SETTINGS_BACK, 1);
    CHECK(!m.capturing && m.open);
    /* Rebind Up to the key of L2: the two swap. */
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(SettingsModelCaptureKey(&m, SDL_SCANCODE_ESCAPE) == 0);
    CHECK(m.capturing);
    CHECK(SettingsModelCaptureKey(&m, d->keys[BTN_L2]) == 1);
    CHECK(!m.capturing);
    CHECK(m.prefs.keys[BTN_UP] == d->keys[BTN_L2]);
    CHECK(m.prefs.keys[BTN_L2] == d->keys[BTN_UP]);
    CHECK(SettingsModelTakeChanged(&m));
    /* Backspace restores the default, swapping back. */
    Do(&m, SETTINGS_CLEAR, 1);
    CHECK(m.prefs.keys[BTN_UP] == d->keys[BTN_UP]);
    CHECK(m.prefs.keys[BTN_L2] == d->keys[BTN_L2]);
    /* Controller list: bind Cross (row 5) to the right trigger. */
    SettingsModelSelect(&m, CONTROLS_DEVICE);
    Do(&m, SETTINGS_RIGHT, 1);
    CHECK(m.device == SETTINGS_DEVICE_CONTROLLER);
    SettingsModelSelect(&m, CONTROLS_BINDINGS + 4);
    SettingsModelRow(&m, CONTROLS_BINDINGS + 4, &v);
    CHECK(!strcmp(v.label, "Cross"));
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(SettingsModelCaptureKey(&m, SDL_SCANCODE_A) == 0);
    CHECK(SettingsModelCaptureButton(&m, SDL_GAMEPAD_BUTTON_COUNT + 1) == 1);
    CHECK(m.prefs.buttons[BTN_CROSS] == SDL_GAMEPAD_BUTTON_COUNT + 1);
    SettingsModelRow(&m, CONTROLS_BINDINGS + 4, &v);
    CHECK(!strcmp(v.value, "Right trigger"));
    /* Scrolling keeps the selection visible. */
    SettingsModelSelect(&m, CONTROLS_BINDINGS + RR4_PREFS_BUTTONS - 1);
    CHECK(SettingsRowTop(&m, m.row) >= 0.0f);
    CHECK(SettingsRowTop(&m, 0) < 0.0f);
    /* Restore page defaults resets the listed device only. */
    m.prefs.keys[BTN_UP] = SDL_SCANCODE_I;
    SettingsModelSelect(&m, SettingsContentRows(&m));
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.prefs.buttons[BTN_CROSS] == d->buttons[BTN_CROSS]);
    CHECK(m.prefs.keys[BTN_UP] == SDL_SCANCODE_I);
}

static void TestAudioAndDisc(void) {
    SettingsModel m;
    SettingsRowView v;

    SettingsModelOpen(&m, Rr4PrefsDefaults(), "/discs/Ridge Racer Type 4 (Europe).cue");
    SettingsModelSwitchPage(&m, SETTINGS_PAGE_AUDIO);
    Do(&m, SETTINGS_RIGHT, 1); /* clamps at 100% */
    CHECK(m.prefs.volume == 100);
    Do(&m, SETTINGS_LEFT, 3);
    CHECK(m.prefs.volume == 85);
    Do(&m, SETTINGS_LEFT, 30);
    CHECK(m.prefs.volume == 0);
    Do(&m, SETTINGS_DOWN, 1);
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.prefs.mute == 1);

    Do(&m, SETTINGS_PAGE_NEXT, 1);
    CHECK(m.page == SETTINGS_PAGE_DISC);
    SettingsModelRow(&m, 0, &v);
    CHECK(!strcmp(v.value, "Ridge Racer Type 4 (Europe).cue"));
    Do(&m, SETTINGS_DOWN, 1);
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.choose_disc);
    SettingsModelDiscChosen(&m, "/x/track.bin");
    CHECK(!m.choose_disc && m.prefs.disc[0] == '\0');
    SettingsModelDiscChosen(&m, NULL);
    CHECK(m.prefs.disc[0] == '\0');
    SettingsModelDiscChosen(&m, "/y/RR4.CUE");
    CHECK(!strcmp(m.prefs.disc, "/y/RR4.CUE"));
    SettingsModelRow(&m, 1, &v);
    CHECK(!strcmp(v.value, "RR4.CUE"));
    /* The mounted image is unaffected. */
    SettingsModelRow(&m, 0, &v);
    CHECK(!strcmp(v.value, "Ridge Racer Type 4 (Europe).cue"));
    /* Then Link cable. */
    Do(&m, SETTINGS_PAGE_NEXT, 1);
    CHECK(m.page == SETTINGS_PAGE_LINK);
    /* Replays: without a replay session the page says so and asks for
     * nothing; with replays listed, Enter asks the host to save or play. */
    Do(&m, SETTINGS_PAGE_NEXT, 1);
    CHECK(m.page == SETTINGS_PAGE_REPLAYS);
    CHECK(SettingsContentRows(&m) == 2);
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.replay_request == 0);
    m.replays_available = 1;
    m.replay_can_save = 0;
    snprintf(m.replay_save_help, sizeof(m.replay_save_help), "No replay yet.");
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.replay_request == 0 && !strcmp(m.status, "No replay yet."));
    m.replay_can_save = 1;
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.replay_request == SETTINGS_REPLAY_SAVE);
    m.replay_request = 0;
    m.replay_count = 2;
    snprintf(m.replays[1].label, sizeof(m.replays[1].label), "Time Attack, course 1");
    m.replays[1].ok = 1;
    CHECK(SettingsContentRows(&m) == 3);
    Do(&m, SETTINGS_DOWN, 2);
    SettingsModelRow(&m, m.row, &v);
    CHECK(!strcmp(v.label, "Time Attack, course 1"));
    Do(&m, SETTINGS_RIGHT, 1); /* no choice to step */
    CHECK(m.replay_request == 0);
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.replay_request == SETTINGS_REPLAY_LOAD && m.replay_index == 1);
    m.replay_request = 0;
    /* Next page wraps to Graphics; Quit game closes and quits. */
    Do(&m, SETTINGS_PAGE_NEXT, 1);
    CHECK(m.page == SETTINGS_PAGE_GRAPHICS);
    SettingsModelSelect(&m, SettingsRowCount(&m) - 1);
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(!m.open && m.quit && m.edited);
}

/* Link cable: Off / Host / Join, the port and address typed in, the
 * connection status from the host; RR4_LINK values. */
static void TestLinkPage(const char *dir) {
    SettingsModel m;
    SettingsRowView v;
    Rr4Prefs q;
    char path[1024], address[64];
    int mode;
    unsigned port;

    SettingsModelOpen(&m, Rr4PrefsDefaults(), NULL);
    SettingsModelSwitchPage(&m, SETTINGS_PAGE_LINK);
    CHECK(SettingsContentRows(&m) == 4);
    SettingsModelRow(&m, 0, &v);
    CHECK(!strcmp(v.label, "Link cable") && !strcmp(v.value, "Off"));
    SettingsModelRow(&m, 1, &v);
    CHECK(!strcmp(v.value, "47400"));
    Do(&m, SETTINGS_RIGHT, 1);
    CHECK(m.prefs.link_mode == RR4_LINK_HOST && SettingsModelTakeChanged(&m));
    Do(&m, SETTINGS_RIGHT, 1);
    CHECK(m.prefs.link_mode == RR4_LINK_JOIN && strstr(m.status, "address"));
    Do(&m, SETTINGS_LEFT, 2);
    CHECK(m.prefs.link_mode == RR4_LINK_OFF);
    /* the port: digits only, kept on Enter, refused when out of range */
    Do(&m, SETTINGS_DOWN, 1);
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.editing && !strcmp(m.edit, "47400"));
    Do(&m, SETTINGS_CLEAR, 5);
    CHECK(SettingsModelTextInput(&m, "12x34"));
    CHECK(!strcmp(m.edit, "1234"));
    SettingsModelRow(&m, 1, &v);
    CHECK(v.capturing && !strcmp(v.value, "1234_"));
    Do(&m, SETTINGS_DOWN, 1); /* typing: the selection stays */
    CHECK(m.row == 1);
    SettingsModelTakeChanged(&m);
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(!m.editing && m.prefs.link_port == 1234 && SettingsModelTakeChanged(&m));
    Do(&m, SETTINGS_ACTIVATE, 1);
    Do(&m, SETTINGS_CLEAR, 4);
    SettingsModelTextInput(&m, "0");
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.prefs.link_port == 1234 && strstr(m.status, "65535"));
    /* the address: Escape drops the typing */
    Do(&m, SETTINGS_DOWN, 1);
    Do(&m, SETTINGS_ACTIVATE, 1);
    SettingsModelTextInput(&m, "192.168.1.20 ");
    Do(&m, SETTINGS_BACK, 1);
    CHECK(m.open && !m.editing && m.prefs.link_address[0] == '\0');
    Do(&m, SETTINGS_ACTIVATE, 1);
    SettingsModelTextInput(&m, "racer.local");
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(!strcmp(m.prefs.link_address, "racer.local"));
    /* the status row shows what the host reports */
    SettingsModelSetLinkStatus(&m, "Connected, 0.4 ms", "Connected to 192.168.1.20, round trip 0.4 ms");
    SettingsModelRow(&m, 3, &v);
    CHECK(v.kind == 3 && !strcmp(v.value, "Connected, 0.4 ms") && strstr(v.help, "round trip"));
    /* the page's defaults */
    SettingsModelSelect(&m, SettingsContentRows(&m));
    Do(&m, SETTINGS_ACTIVATE, 1);
    CHECK(m.prefs.link_mode == RR4_LINK_OFF && m.prefs.link_port == 47400 && !m.prefs.link_address[0]);

    /* [link] round trip */
    q = *Rr4PrefsDefaults();
    q.link_mode = RR4_LINK_JOIN;
    q.link_port = 47999;
    snprintf(q.link_address, sizeof(q.link_address), "fe80::1%%en0");
    snprintf(path, sizeof(path), "%s/link.toml", dir);
    CHECK(Rr4PrefsSaveFile(&q, path));
    m.prefs = *Rr4PrefsDefaults();
    CHECK(Rr4PrefsLoadFile(&m.prefs, path));
    CHECK(m.prefs.link_mode == RR4_LINK_JOIN && m.prefs.link_port == 47999 &&
          !strcmp(m.prefs.link_address, "fe80::1%en0"));

    CHECK(Rr4LinkParse("host", &mode, address, sizeof(address), &port) && mode == RR4_LINK_HOST && port == 47400);
    CHECK(Rr4LinkParse("host:5000", &mode, address, sizeof(address), &port) && port == 5000);
    CHECK(!Rr4LinkParse("host:0", &mode, address, sizeof(address), &port));
    CHECK(Rr4LinkParse("join:127.0.0.1:47411", &mode, address, sizeof(address), &port) &&
          mode == RR4_LINK_JOIN && !strcmp(address, "127.0.0.1") && port == 47411);
    CHECK(Rr4LinkParse("join:racer.local", &mode, address, sizeof(address), &port) &&
          !strcmp(address, "racer.local") && port == 47400);
    CHECK(Rr4LinkParse("join:[::1]:6000", &mode, address, sizeof(address), &port) &&
          !strcmp(address, "::1") && port == 6000);
    CHECK(Rr4LinkParse("join:fe80::2", &mode, address, sizeof(address), &port) && !strcmp(address, "fe80::2"));
    CHECK(Rr4LinkParse("off", &mode, address, sizeof(address), &port) && mode == RR4_LINK_OFF);
    CHECK(!Rr4LinkParse("join:", &mode, address, sizeof(address), &port));
    CHECK(!Rr4LinkParse("client", &mode, address, sizeof(address), &port));
}

static void TestPointer(void) {
    SettingsModel m;
    SettingsRect tab = SettingsTabRect(SETTINGS_PAGE_AUDIO);
    float top;

    SettingsModelOpen(&m, Rr4PrefsDefaults(), NULL);
    SettingsModelPointer(&m, tab.x + 4, tab.y + 4, 1);
    CHECK(m.page == SETTINGS_PAGE_AUDIO);
    top = SettingsRowTop(&m, 0);
    SettingsModelPointer(&m, SettingsValueCentre() - 60, top + 10, 1); /* left of value: down */
    CHECK(m.prefs.volume == 95);
    SettingsModelPointer(&m, -10, -10, 0); /* hover outside: nothing */
    CHECK(m.open);
    SettingsModelPointer(&m, -10, -10, 1);
    CHECK(!m.open);
}

static void TestPrefsFile(const char *dir) {
    char path[1024];
    Rr4Prefs p = *Rr4PrefsDefaults(), q = *Rr4PrefsDefaults();
    FILE *f;

    snprintf(path, sizeof(path), "%s/roundtrip.toml", dir);
    p.scale = 4;
    p.fullscreen = 1;
    p.vsync = RR4_VSYNC_ON;
    p.antialiasing = 1;
    p.aspect_ratio = RR4_ASPECT_WIDESCREEN;
    p.video_timing = RR4_VIDEO_NTSC;
    p.extended_draw_distance = 1;
    p.volume = 35;
    p.mute = 1;
    p.keys[BTN_UP] = SDL_SCANCODE_I;
    p.buttons[BTN_CROSS] = SDL_GAMEPAD_BUTTON_COUNT;
    p.pad_type[0] = RR4_PAD_NEGCON;
    p.pad_type[1] = RR4_PAD_JOGCON;
    p.force_feedback = 0;
    p.axes[RR4_AXIS_STEERING].deadzone = 0.1f;
    p.axes[RR4_AXIS_STEERING].linearity = -1.25f;
    p.axes[RR4_AXIS_THROTTLE].saturation = 0.8f;
    p.axes[RR4_AXIS_BRAKE].scaling = 2.5f;
    snprintf(p.disc, sizeof(p.disc), "%s", "/games/rr4 \"PAL\"/Ridge Racer Type 4.cue");
    CHECK(Rr4PrefsSaveFile(&p, path));
    CHECK(Rr4PrefsLoadFile(&q, path));
    CHECK(!memcmp(&p, &q, sizeof(p)));

    /* Invalid values are skipped, the rest still loads. */
    snprintf(path, sizeof(path), "%s/partial.toml", dir);
    f = fopen(path, "w");
    CHECK(f != NULL);
    if (f) {
        fputs("[graphics]\ninternal_resolution = 99\nfullscreen = true\nvsync = \"sometimes\"\n"
              "internal_resolution = 3\n[keyboard]\nup = 41\ncross = 7\nnonsense\n[future]\nx = 1\n",
              f);
        fclose(f);
    }
    q = *Rr4PrefsDefaults();
    CHECK(Rr4PrefsLoadFile(&q, path));
    CHECK(q.scale == 4); /* 3 rounds up to an offered scale */
    CHECK(q.fullscreen == 1 && q.vsync == RR4_VSYNC_AUTO);
    CHECK(q.keys[BTN_UP] == Rr4PrefsDefaults()->keys[BTN_UP]); /* Escape refused */
    CHECK(q.keys[BTN_CROSS] == 7);
    CHECK(!Rr4PrefsLoadFile(&q, "/nonexistent/rr4.toml"));

    /* [input]: unknown controllers and out-of-range axes are skipped. */
    snprintf(path, sizeof(path), "%s/input.toml", dir);
    f = fopen(path, "w");
    CHECK(f != NULL);
    if (f) {
        fputs("[input]\ncontroller_1 = \"dualshock\"\ncontroller_2 = \"wheel\"\n"
              "force_feedback = false\nsteering_deadzone = 1.5\nsteering_linearity = 1\n"
              "throttle_saturation = 0.9\nbrake_scaling = abc\n",
              f);
        fclose(f);
    }
    q = *Rr4PrefsDefaults();
    CHECK(Rr4PrefsLoadFile(&q, path));
    CHECK(q.pad_type[0] == RR4_PAD_DUALSHOCK && q.pad_type[1] == RR4_PAD_DIGITAL);
    CHECK(!q.force_feedback);
    CHECK(q.axes[RR4_AXIS_STEERING].deadzone == 0.0f && q.axes[RR4_AXIS_STEERING].linearity == 1.0f);
    CHECK(q.axes[RR4_AXIS_THROTTLE].saturation == 0.9f && q.axes[RR4_AXIS_BRAKE].scaling == 1.0f);

    /* Earlier aspect keys: square pixels alone is classic, widescreen 16:9. */
    snprintf(path, sizeof(path), "%s/aspect.toml", dir);
    f = fopen(path, "w");
    CHECK(f != NULL);
    if (f) {
        fputs("[graphics]\nsquare_pixels = true\n", f);
        fclose(f);
    }
    q = *Rr4PrefsDefaults();
    CHECK(Rr4PrefsLoadFile(&q, path) && q.aspect_ratio == RR4_ASPECT_CLASSIC);
    f = fopen(path, "w");
    CHECK(f != NULL);
    if (f) {
        fputs("[graphics]\nsquare_pixels = true\nwidescreen = true\n", f);
        fclose(f);
    }
    CHECK(Rr4PrefsLoadFile(&q, path) && q.aspect_ratio == RR4_ASPECT_WIDESCREEN);
    f = fopen(path, "w");
    CHECK(f != NULL);
    if (f) {
        fputs("[graphics]\naspect_ratio = \"sideways\"\n", f);
        fclose(f);
    }
    CHECK(Rr4PrefsLoadFile(&q, path) && q.aspect_ratio == RR4_ASPECT_CLASSIC);

    /* RR4_CONFIG_HOME selects the user file and wins over other layers. */
    snprintf(path, sizeof(path), "%s/home/ridge-racer-type-4.toml", dir);
    SDL_RemovePath(path);
    snprintf(path, sizeof(path), "%s/home", dir);
    SDL_setenv_unsafe("RR4_CONFIG_HOME", path, 1);
    CHECK(Rr4PrefsLoad(&q));
    CHECK(strstr(Rr4PrefsPath(), "/home/ridge-racer-type-4.toml") != NULL);
    CHECK(q.scale == 1); /* fresh: defaults */
    q.scale = 2;
    CHECK(Rr4PrefsSave(&q));
    p = *Rr4PrefsDefaults();
    CHECK(Rr4PrefsLoad(&p));
    CHECK(p.scale == 2);
    SDL_unsetenv_unsafe("RR4_CONFIG_HOME");
}

static void TestApply(void) {
    Rr4Prefs p = *Rr4PrefsDefaults();

    p.volume = 40;
    p.extended_draw_distance = 1;
    p.keys[BTN_UP] = SDL_SCANCODE_I;
    p.buttons[BTN_CROSS] = SDL_GAMEPAD_BUTTON_COUNT + 1;
    p.pad_type[0] = RR4_PAD_NEGCON;
    p.axes[RR4_AXIS_THROTTLE].deadzone = 0.25f;
    Rr4PrefsApply(&p, 0);
    CHECK(ExtendedDrawDistance == 1);
    CHECK(Psyz_AudioGetOutputVolume() == 40);
    CHECK(Psyz_PadsSetKind(0, 0, PSYZ_CTRL_QUERY_KIND) == PSYZ_CTRL_NEGCON);
    CHECK(Psyz_PadsSetKind(1, 0, PSYZ_CTRL_QUERY_KIND) == PSYZ_CTRL_DIGITAL_PAD);
    {
        PsyzAxisResponse r;
        CHECK(Psyz_PadsGetAxisResponse(PSYZ_PAD_AXIS_I, &r) == 0);
        CHECK(r.deadzone == 0.25f && r.full == 0x6A);
        CHECK(Psyz_PadsGetAxisResponse(PSYZ_PAD_AXIS_STEERING, &r) == 0 && r.linearity == 0.5f);
    }
    CHECK(Psyz_GetKeyboardBinding_SDL3(BTN_UP) == SDL_SCANCODE_I);
    CHECK(Psyz_GetGamepadBinding_SDL3(BTN_CROSS) == SDL_GAMEPAD_BUTTON_COUNT + 1);
    p.mute = 1;
    Rr4PrefsApply(&p, 0);
    CHECK(Psyz_AudioGetOutputVolume() == 0);
    Rr4PrefsApply(Rr4PrefsDefaults(), 0);
    CHECK(ExtendedDrawDistance == 0);
    CHECK(Psyz_AudioGetOutputVolume() == 100);
    CHECK(Psyz_PadsSetKind(0, 0, PSYZ_CTRL_QUERY_KIND) == PSYZ_CTRL_DIGITAL_PAD);
    CHECK(Psyz_GetKeyboardBinding_SDL3(BTN_UP) == Rr4PrefsDefaults()->keys[BTN_UP]);
}

static void TestRaster(const char *dir) {
    SettingsModel m;
    SettingsSurface surface = {0};
    SettingsLayout layout;
    char path[1024];

    CHECK(SettingsFontReady());
    SettingsModelOpen(&m, Rr4PrefsDefaults(), "/discs/Ridge Racer Type 4 (Europe).cue");
    for (int page = 0; page < SETTINGS_PAGE_COUNT; page++) {
        size_t opaque = 0;
        SettingsModelSwitchPage(&m, page);
        if (page == SETTINGS_PAGE_CONTROLS) {
            SettingsModelSelect(&m, CONTROLS_BINDINGS + 1);
            SettingsModelDo(&m, SETTINGS_ACTIVATE); /* shows the capture state */
        }
        SettingsComputeLayout(&m, 1280, 960, &layout);
        CHECK(layout.w > 0 && layout.h > 0 && layout.h <= 960 && layout.x >= 0 && layout.y >= 0);
        CHECK(SettingsSurfaceResize(&surface, layout.w, layout.h));
        SettingsPaint(&surface, &m, &layout);
        for (size_t i = 0; i < (size_t)layout.w * (size_t)layout.h; i++)
            opaque += surface.pixels[i * 4 + 3] > 200;
        CHECK(opaque > (size_t)layout.w * (size_t)layout.h / 2);
        snprintf(path, sizeof(path), "%s/panel-%s.ppm", dir, SettingsPageName(page));
        CHECK(SettingsSurfaceDump(&surface, path));
        if (page == SETTINGS_PAGE_CONTROLS) {
            CHECK(m.capturing);
            SettingsModelDo(&m, SETTINGS_BACK); /* ends the capture only */
            CHECK(m.open && !m.capturing);
        }
    }
    SettingsSurfaceRelease(&surface);
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : "settings-test";

    SDL_CreateDirectory(dir);
    Rr4PrefsCaptureDefaults();
    TestDefaults();
    TestGraphicsPage();
    TestUnavailable();
    TestControlsPage();
    TestAudioAndDisc();
    TestLinkPage(dir);
    TestPointer();
    TestPrefsFile(dir);
    TestApply();
    TestRaster(dir);
    if (failures) {
        fprintf(stderr, "settings_test: %d failures\n", failures);
        return 1;
    }
    printf("settings_test: ok (panels in %s)\n", dir);
    return 0;
}
