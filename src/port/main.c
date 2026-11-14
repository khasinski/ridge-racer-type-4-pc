/* Host entry point. Mounts the user's PAL CUE/BIN image through PsyZ and
 * runs the original game loop (MainLoop, src/main/system/frmtask.c).
 *
 * The disc is argv[1], else RR4_DISC, else (interactive runs) the saved
 * image or one chosen in a file dialog. The European PAL release
 * (SCES-01706) and the Japanese one (SLPS-01800, which selects the game's
 * Japanese release profile, rr/release.h) are accepted (disc_check.h): an
 * interactive run explains any
 * other image or one that does not open, forgets it if it was the saved
 * one and asks again; a headless run prints why and exits with status 1.
 *
 * Interactive runs (no PSYZ_HEADLESS) read the preferences file
 * (src/port/preferences.c) and apply it, keep the memory cards
 * (memory_cards.h) and the log ridge-racer-type-4.log (host_log.h) beside
 * it, and Escape opens the settings menu (src/port/settings_overlay.c).
 * Headless runs never read or write preferences, keep the cards in the
 * working directory and never show a dialog.
 *
 * Every build reads:
 *   RR4_DISC=file      the CUE image when argv has none
 *   RR4_VIDEO=pal|ntsc video timing of the PAL release for the run, over
 *                      the preferences (the Japanese release is NTSC)
 *   RR4_CONFIG_HOME=d  the preferences directory (preferences.h)
 *   PSYZ_HEADLESS runs default to PSYZ_AUDIO_CLOCK=video: unpaced fields
 *                      would otherwise race the wall-clock sequencer tick
 *                      (PSYZ_AUDIO_CLOCK=wall keeps the host clock).
 *   PSYZ_FRAME_STATS=1 log percentiles of the present interval, host work,
 *                      present call and render passes every 500 presents
 *                      (=n every n); PSYZ_FRAME_LOG=file one line per
 *                      present. PSYZ_HEADLESS=gpu PSYZ_HEADLESS_PACED=1
 *                      PSYZ_AUDIO_CLOCK=wall measures the windowed timing
 *                      path (frame limiter, real-time clock) without a
 *                      window.
 *   PSYZ_CD_TIMING=instant  drop the CD drive's mechanical timing (seeks,
 *                      pauses, 150 sectors/s) for instant loading; the
 *                      default is the PS1's timing. PSYZ_CD_TRACE=1 prints
 *                      each drive command with its emulated time.
 *   RR4_SETTINGS_SCRIPT=s  drive the Escape menu in a headless run: s is a
 *                      comma list of key@present (see settings_overlay.h).
 * The test and diagnostic hooks (RR4_FRAME_LIMIT, RR4_CAPTURE*, RR4_TRACE,
 * RR4_PAD_SCRIPT, RR4_WATCH, RR4_POKE, RR4_AUTODRIVE, RR4_GRAPHICS, ...) are
 * listed in src/port/diagnostics.c; builds configured with
 * -DRR4_DIAGNOSTICS=OFF (the releases) have none of them. */

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE /* setenv under -std=c11 */
#endif
/* windows.h first: PsyZ types.h defers to its byte typedef. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif
#include <psyz.h>
#include <psyz/cd.h>
#include <psyz/gpu.h>
#include <psyz/system.h>
#include <psyz/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rr/video_timing.h"
#include "rr/release.h"
#include "disc_check.h"
#include "host_log.h"
#include "memory_cards.h"
#include "link.h"
#include "preferences.h"
#include "settings_overlay.h"
#include "replay_session.h"
#if RR4_DIAGNOSTICS
#include "diagnostics.h"
#endif

#include <SDL3/SDL.h>

void MainLoop(void);

static long frame_count;
static PsyzVSyncCb chained_vsync;

#ifdef _WIN32
/* main's argv converted from the UTF-16 command line, so that a CUE path
 * with characters outside the ANSI code page survives. */
static char **utf8_argv(int *argc) {
    int count = 0;
    wchar_t **wide = CommandLineToArgvW(GetCommandLineW(), &count);
    char **argv;

    if (!wide)
        return NULL;
    argv = (char **)calloc((size_t)count + 1, sizeof(*argv));
    for (int i = 0; argv && i < count; i++) {
        int bytes = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, NULL, 0, NULL, NULL);
        argv[i] = bytes > 0 ? (char *)malloc((size_t)bytes) : NULL;
        if (!argv[i] ||
            !WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, argv[i], bytes, NULL, NULL)) {
            argv = NULL; /* keep the CRT's argv; the copies are left */
            break;
        }
    }
    LocalFree(wide);
    if (argv)
        *argc = count;
    return argv;
}
#endif

/* Both the C runtime's environment (PsyZ's getenv) and SDL's copy of it:
 * SDL takes its copy at the first SDL_getenv and does not see setenv after
 * that. */
static void set_env(const char *name, const char *value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), name, value, true);
}

/* The original loop never returns; the host ends it at a VSync. */
static void host_vsync(void) {
    int quitting;

    frame_count++;
#if RR4_DIAGNOSTICS
    quitting = Rr4DiagnosticsVSync(frame_count, Psyz_QuitRequested());
#else
    quitting = Psyz_QuitRequested();
#endif
    if (quitting) {
        Rr4Log("ridge-racer-type-4-pc: exit after %ld vsyncs\n", frame_count);
        exit(0);
    }
    if (chained_vsync)
        chained_vsync();
}

/* ---- the disc (disc_check.h) for players ---- */

static int choose_disc(void *context, char *out, size_t size) {
    (void)context;
    return SettingsChooseDisc(out, size);
}

static void alert(void *context, const char *message) {
    (void)context;
    Rr4Log("ridge-racer-type-4-pc: %s\n", message);
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Ridge Racer Type 4", message, NULL);
}

/* Nothing can draw: no SDL GPU device and no SDL renderer to fall back to,
 * or the GPU failed during the run (Psyz_VideoSetFailureCallback). */
static int s_interactive;
static void video_failed(const char *message) {
    if (s_interactive)
        alert(NULL, message);
    else
        Rr4Log("ridge-racer-type-4-pc: %s\n", message);
    exit(1);
}

static Rr4DiscStatus check_disc(void *context, const char *cue, char *message, size_t size) {
    Rr4DiscStatus status;
    (void)context;
    status = Rr4DiscCheckImage(cue, message, size);
#if RR4_DIAGNOSTICS
    /* RR4_UNVERIFIED_RELEASE=1: also run a release the game has a profile
     * for but that is not verified yet (rr/release.h), for its tests. */
    if (status == RR4_DISC_OTHER_RELEASE && getenv("RR4_UNVERIFIED_RELEASE") &&
        atoi(getenv("RR4_UNVERIFIED_RELEASE"))) {
        char boot[64];
        if (Rr4DiscImageBoot(cue, boot, sizeof(boot)) == RR4_DISC_OK && RrReleaseForBootFile(boot)) {
            Rr4Log("ridge-racer-type-4-pc: running the unverified %s (%s)\n",
                   RrReleaseForBootFile(boot)->name, RrReleaseForBootFile(boot)->productCode);
            if (message && size)
                message[0] = '\0';
            status = RR4_DISC_OK;
        }
    }
#endif
    return status;
}

static int mount_disc(void *context, const char *cue) {
    (void)context;
    return Psyz_CdSetDiskPath(cue);
}

static void save_prefs(void *context, const Rr4Prefs *prefs) {
    (void)context;
    if (Rr4PrefsPath()[0] && !Rr4PrefsSave(prefs))
        Rr4Log("ridge-racer-type-4-pc: cannot save preferences to %s\n", Rr4PrefsPath());
}

/* Preferences, log and memory cards of an interactive run. */
static void open_player_files(Rr4Prefs *prefs) {
    char directory[RR4_PREFS_PATH_MAX], log[RR4_PREFS_PATH_MAX + 32];

    if (!Rr4PrefsResolve()) {
        Rr4Log("ridge-racer-type-4-pc: no preferences location, settings are not saved\n");
        return;
    }
    Rr4PrefsDirectory(directory, sizeof(directory));
    snprintf(log, sizeof(log), "%sridge-racer-type-4.log", directory);
    if (!Rr4LogOpen(log))
        Rr4Log("ridge-racer-type-4-pc: cannot write the log %s\n", log);
    Rr4Log("Ridge Racer Type 4 PC; preferences %s\n", Rr4PrefsPath());
    Rr4PrefsLoad(prefs);
    Rr4CardsUseDirectory(directory);
}

int main(int argc, char **argv) {
    const char *headless = getenv("PSYZ_HEADLESS");
    int interactive = !(headless && *headless);
    static Rr4Prefs prefs;
    static char mounted[RR4_PREFS_PATH_MAX];
    char message[RR4_DISC_MESSAGE_MAX];
    const char *requested = NULL, *cue;
    Rr4DiscStatus status;

    /* Headless runs are not paced (PsyZ limitless mode), so wall time has no
     * relation to the emulated fields. The root counters (the libsnd
     * sequencer tick, which times the music and the beat-synced front-end
     * scripts) and the SPU mix then follow the counted fields, as with
     * PSYZ_AUDIO_CLOCK=video; another PSYZ_AUDIO_CLOCK value keeps the host
     * clock. Set before any SDL call and before PsyZ reads it (kernel and
     * audio initialisation). */
    Rr4LinkPrepareEnv(); /* a link run keeps the host clock */
    if (!interactive && !getenv("PSYZ_AUDIO_CLOCK"))
        set_env("PSYZ_AUDIO_CLOCK", "video");
#ifdef _WIN32
    {
        char **wide_argv = utf8_argv(&argc);
        if (wide_argv)
            argv = wide_argv;
    }
#endif
    /* Host paths are UTF-8: SDL_getenv reads the UTF-16 environment on
     * Windows, where the CRT's getenv and main's argv use the ANSI code
     * page. */
    if (argc > 1)
        requested = argv[1];
    else if (SDL_getenv("RR4_DISC") && *SDL_getenv("RR4_DISC"))
        requested = SDL_getenv("RR4_DISC");

    /* Preferences, the log and the image dialog are for players only. */
    Rr4PrefsCaptureDefaults();
    prefs = *Rr4PrefsDefaults();
    if (interactive)
        open_player_files(&prefs);
    {
        static const Rr4DiscHost host = {NULL, choose_disc, alert, check_disc, mount_disc, save_prefs};
        cue = Rr4DiscSelect(&host, &prefs, requested, interactive, &status, message, sizeof(message));
    }
    if (!cue && status == RR4_DISC_UNREADABLE && !interactive) {
        Rr4Log("ridge-racer-type-4-pc: cannot open disc image %s\n", requested);
        return 1;
    }
    if (!cue && status != RR4_DISC_OK) {
        Rr4Log("ridge-racer-type-4-pc: %s\n", message);
        return 1;
    }
    if (!cue) {
        Rr4Log("usage: %s <Ridge Racer Type 4 (Europe).cue>\n"
               "       (or set RR4_DISC to the CUE file)\n",
               argv[0]);
        return 2;
    }
    snprintf(mounted, sizeof(mounted), "%s", cue);
    if (interactive)
        Rr4Log("ridge-racer-type-4-pc: disc %s\n", mounted);
    /* At 1x without GPU-only enhancements the CPU rasterizer draws the
     * frames bit-exactly as the PS1 GPU; higher internal resolutions and the
     * enhancements use the GPU rasterizer, which samples the same PS1 rules
     * at every target pixel (its native VRAM is the 1x result).
     * RR4_RASTERIZER=gpu (diagnostics) forces the GPU rasterizer at 1x too. */
    Psyz_VideoSetNativeRasterizer(1);
    if (interactive)
        Rr4PrefsApply(&prefs, 0);
    if (interactive || (getenv("RR4_SETTINGS_SCRIPT") && *getenv("RR4_SETTINGS_SCRIPT")))
        SettingsOverlayInstall(&prefs, mounted);
#if RR4_DIAGNOSTICS
    Rr4DiagnosticsInit(&prefs, interactive);
#endif
    {
        /* The release of the mounted disc (rr/release.h, from its
         * SYSTEM.CNF), and the video timing for the whole run
         * (rr/video_timing.h): the preference, or RR4_VIDEO=pal / ntsc (also
         * headless). NTSC timing keeps the PAL release's 256-line picture
         * whole at NTSC field timing; the Japanese release always runs at
         * NTSC timing with its own 240-line picture. */
        const char *video = getenv("RR4_VIDEO");
        int ntsc = prefs.video_timing == RR4_VIDEO_NTSC;
        char boot[64];
        const RrReleaseProfile *release = NULL;
        if (Rr4DiscImageBoot(mounted, boot, sizeof(boot)) == RR4_DISC_OK)
            release = RrReleaseForBootFile(boot);
        if (video && *video)
            ntsc = !strcmp(video, "ntsc");
        ntsc = RrReleaseSelect(release ? release->release : RR_RELEASE_PAL, ntsc);
        Psyz_GpuSetNtscPalPicture(ntsc && !RR_RELEASE_JP);
        if (interactive)
            Rr4Log("ridge-racer-type-4-pc: %s (%s)\n", GameReleaseProfile->name, GameReleaseProfile->productCode);
        Rr4LinkStart(&prefs, ntsc, GameReleaseProfile->productCode);
    }
    ReplaySessionInstall(interactive ? Rr4PrefsPath() : "");
    chained_vsync = Psyz_SetVSyncCb(host_vsync);
    s_interactive = interactive;
    Psyz_VideoSetFailureCallback(video_failed);
    MainLoop();
    return 0;
}
