/* Diagnostics for automated runs (use with PSYZ_HEADLESS=1
 * SDL_AUDIODRIVER=dummy). Built with -DRR4_DIAGNOSTICS=ON only (the default
 * for development builds, off for releases). The syntax of the lists is
 * parsed in diagnostics_parse.c; a malformed entry is reported on stderr and
 * skipped, and a malformed number is ignored with a message.
 *   RR4_FRAME_LIMIT=n  exit after n VSyncs
 *   RR4_CAPTURE=file   write the displayed frame as a PPM when exiting
 *   RR4_VRAM=file      write the whole VRAM as a PPM when exiting
 *   RR4_CAPTURE_AT=l   also write the displayed frame at each VSync of the
 *                      comma list l, as <RR4_CAPTURE or "capture">.<vsync>.ppm;
 *                      an entry a-b/s is every s-th VSync from a to b
 *   RR4_CAPTURE_MID=1  with frame interpolation, also write the midpoint
 *                      drawn for the displayed buffer (the in-between of the
 *                      frame shown and the one before it) at each of those
 *                      VSyncs, as <...>.<vsync>.mid.ppm. When no midpoint
 *                      was presented in the two fields before the VSync
 *                      (the frame before stayed on screen), an empty
 *                      <...>.<vsync>.held marks it.
 *   RR4_GPU_DUMP_AT=l  record the GPU commands of the frame after each
 *                      VSync of the comma list l (to the next VSync) as
 *                      <RR4_GPU_DUMP or "gpudump">.<vsync>.psxgpu, a GPUDUMP
 *                      file DuckStation replays, and the VRAM after it as
 *                      <...>.<vsync>.vram (raw 1024x512 16-bit pixels).
 *                      RR4_GPU_DUMP_LEN=n records n VSyncs instead of one
 *                      (frames that take more than one field).
 *   RR4_RASTERIZER=gpu draw with PsyZ's GPU rasterizer at 1x as well; by
 *                      default 1x frames are drawn by the CPU rasterizer.
 *                      Both match the PS1 GPU pixel for pixel; at Nx the
 *                      GPU rasterizer's first pixel of every NxN block is
 *                      the 1x pixel.
 *   RR4_TRACE=n        print the mode state every n VSyncs (with
 *                      PSYZ_FRAME_LOG also "t ms", the host time of
 *                      the log's present_ms column; with frame
 *                      interpolation "interp p/t/m mid n": the last
 *                      frame's primitives, those tagged for
 *                      interpolation, those paired with the frame
 *                      before, and the midpoints presented so far)
 *   RR4_PAD_SCRIPT=s   test input: hold pad buttons over VSync ranges.
 *                      s is a comma list of [2:]button@vsync[+frames]
 *                      (default 4 frames), e.g. "start@400,cross@900+6,
 *                      2:cross@900+6,up@1200". A "2:" prefix presses the
 *                      button on pad 2, otherwise pad 1.
 *                      Buttons: select l3 r3 start up right down left l2 r2
 *                      l1 r1 triangle circle cross square. The pressed
 *                      buttons replace the pad's receive buffer
 *                      (PadPorts[port]) right after PsyZ refreshes it at
 *                      each VSync; outside the listed ranges the real input
 *                      passes through, or an idle connected digital pad when
 *                      none is present. Any script keeps pad 1 connected;
 *                      pad 2 only when some step names it.
 *                      Instead of an absolute vsync a step can be anchored
 *                      to a mode: button@mG/I:d presses d VSyncs after each
 *                      entry into ModeGroup G / ModeIndex I (while still in
 *                      it), and a %p suffix repeats the step every p VSyncs,
 *                      e.g. "circle@m1/12:60+4%154" (dialogue) or
 *                      "start@m1/22:120" (skip a replay).
 *                      Analog steps name=value@... hold a host axis for the
 *                      same ranges, as the Escape menu's controller types
 *                      read it (other times it rests): lx ly rx ry (the
 *                      DUAL SHOCK sticks, -1..1, 0 centre), steer (neGcon
 *                      twist / JogCon dial, -1..1) and i ii l (neGcon I, II,
 *                      L, 0..1), e.g. "steer=0.5@m1/53:100+200,i=1@m1/53:0+400".
 *                      Values go through PsyZ's frame builder with its
 *                      default (DuckStation) responses.
 *   RR4_PAD_TYPE=l     controller types for pad 1[,pad 2]: digital (default),
 *                      dualshock, negcon or jogcon, e.g. "negcon" or
 *                      "digital,dualshock".
 *   RR4_WATCH=list     with RR4_TRACE, also print globals by name: a comma
 *                      list of name[:size] (size 1, 2 or 4 bytes, default 4,
 *                      signed), e.g. "TitleSceneStage:2,FadeCountdown:2".
 *                      RR4_WATCH and RR4_POKE find the names among the
 *                      executable's exported symbols: macOS builds export
 *                      them, Linux and Windows builds only when configured
 *                      with -DRR4_EXPORT_SYMBOLS=ON (otherwise no name is
 *                      found).
 *                      name+offset reads a member at a byte offset (decimal
 *                      or 0x hex) into the object, e.g. "SoundState+9904".
 *                      RR4_TRACE_RAND=1 also prints the BIOS rand() state
 *                      (PS1: word at 0x80009010; seeded from VSync(1) at
 *                      race load, so poke it on the reference to compare).
 *   RR4_POKE=list      test hook to fast-forward game state: a comma list of
 *                      name[+offset][:size]=value@vsync (store) or
 *                      name[+offset][:size]+=value@vsync (add), applied once
 *                      at that VSync (between game frames). Names and
 *                      offsets are host symbols/layouts as in RR4_WATCH,
 *                      e.g. "PlayerCars+388+=170330@3400" adds a lap's worth
 *                      of distance to the first player car; the value may
 *                      also name a 32-bit global read when applied
 *                      ("PlayerCars+388+=TrackLength@...", or name+offset
 *                      as in "PlayerCars+388=PlayerCars+1204@..."). The name
 *                      "rand" sets the BIOS rand() state instead, e.g.
 *                      "rand=12345@m1/47:600" to align random sequences
 *                      with the reference. The vsync may
 *                      be a mode anchor mG/I:d as in RR4_PAD_SCRIPT (applied
 *                      once per entry into that mode).
 *   RR4_AUTODRIVE=1    test driver for whole races: while a race runs
 *                      (RaceState 2..3) pad 1 holds accelerate (cross) and
 *                      steers left/right towards the course centre line
 *                      from the player car's track heading and lateral
 *                      offset. Combines with RR4_PAD_SCRIPT. With
 *                      RR4_AUTODRIVE_LOG=file it writes one line
 *                      "ModeFrame mask" per VSync it drives, which
 *                      tools/port/duckstation.py replays on the reference
 *                      (same ModeFrame -> same buttons) for comparison;
 *                      RR4_AUTODRIVE_REPLAY=file drives from such a log;
 *                      RR4_AUTODRIVE_HOLD=n only accelerates before
 *                      ModeFrame n (a fixed input up to the replay snapshot).
 *   RR4_AUDIO_CAPTURE=file  write the mixed audio output (44100 Hz 16-bit
 *                      stereo) as a WAV file, finished when exiting. With
 *                      PSYZ_AUDIO_CLOCK=video the samples follow the VSync
 *                      count instead of the wall clock.
 *   RR4_INTERNAL_RES=n render at n times the PlayStation resolution (1..8)
 *                      for this run, over the preferences; with
 *                      PSYZ_HEADLESS=gpu the captured frames grow with it.
 *   RR4_GRAPHICS=l     graphics options for this run, as the Escape menu
 *                      sets them: a comma list of geometry (geometry
 *                      correction), nodither (dithering off), aa
 *                      (antialiasing), filter (texture filtering),
 *                      interpolation (frame interpolation) and wide (the
 *                      Widescreen 16:9 aspect ratio: square pixels, races
 *                      at 16:9). Headless runs turn the options not named
 *                      off; interactive runs keep them as the preferences
 *                      have them. With PSYZ_HEADLESS=gpu the captures show
 *                      them.
 * Also handled elsewhere: RR4_SETTINGS_SCRIPT (settings_overlay.h), and in
 * every build RR4_DISC, RR4_VIDEO, RR4_CONFIG_HOME and the PSYZ_* variables
 * (main.c). */

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE /* RTLD_DEFAULT under -std=c11 */
#endif
/* windows.h first: PsyZ types.h defers to its byte typedef. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <psyz.h>
#include <psyz/audio.h>
#include <psyz/file.h>
#include <psyz/gpu.h>
#include <psyz/input.h>
#include <psyz/system.h>
#include <psyz/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "game/system/pad.h"

#include <SDL3/SDL.h>

#include "diagnostics.h"
#include "diagnostics_parse.h"

void Psyz_Srand(unsigned int seed);

/* Mode state of the original dispatcher, read for RR4_TRACE. */
extern unsigned short ModeGroup;
extern short ModeIndex;
extern unsigned short SubmodeState;
extern unsigned int FrameCounter;

static long frame_limit;
static long trace_every;
static int frame_log; /* PSYZ_FRAME_LOG is set */
static int trace_rand;

/* A whole number from the variable name, or fallback when unset or empty;
 * a malformed value is reported and gives fallback. */
static long env_long(const char *name, long fallback) {
    const char *text = getenv(name);
    long value;

    if (!text || !*text)
        return fallback;
    if (Rr4DiagParseLong(text, NULL, 10, &value))
        return value;
    fprintf(stderr, "%s: not a number: \"%s\"\n", name, text);
    return fallback;
}

/* Host paths are UTF-8 (SDL_getenv reads the UTF-16 environment on
 * Windows). */
static const char *path_env(const char *name) {
    const char *path = SDL_getenv(name);
    return path && *path ? path : NULL;
}

/* Address of a global for RR4_WATCH / RR4_POKE. The executable exports its
 * symbols on macOS; on Linux and Windows only a build configured with
 * -DRR4_EXPORT_SYMBOLS=ON (-rdynamic, --export-all-symbols) has them. */
static void *find_symbol(const char *name) {
#ifdef _WIN32
    return (void *)(uintptr_t)GetProcAddress(GetModuleHandleW(NULL), name);
#else
    return dlsym(RTLD_DEFAULT, name);
#endif
}

/* ---- RR4_WATCH ---- */

#define WATCH_MAX 16
static struct {
    char name[64];
    const void *addr;
    int size;
} watches[WATCH_MAX];
static int watch_count;

static void setup_watches(const char *list) {
    static Rr4WatchSpec specs[64];
    int count = Rr4DiagParseWatch(list, specs, 64);

    for (int i = 0; i < count && watch_count < WATCH_MAX; i++) {
        const char *addr = find_symbol(specs[i].name);
        if (!addr) {
            fprintf(stderr, "RR4_WATCH: no symbol %s\n", specs[i].name);
            continue;
        }
        snprintf(watches[watch_count].name, sizeof(watches[0].name), "%s", specs[i].label);
        watches[watch_count].addr = addr + specs[i].offset;
        watches[watch_count].size = specs[i].size;
        watch_count++;
    }
}

static void print_watches(void) {
    for (int i = 0; i < watch_count; i++) {
        long v = 0;
        switch (watches[i].size) {
        case 1: v = *(const signed char *)watches[i].addr; break;
        case 2: v = *(const short *)watches[i].addr; break;
        default: v = *(const int *)watches[i].addr; break;
        }
        fprintf(stderr, " %s=%ld", watches[i].name, v);
    }
    if (trace_rand)
        fprintf(stderr, " rand=%08x", Psyz_RandState());
}

/* ---- mode anchors ---- */

/* Mode anchors for RR4_PAD_SCRIPT / RR4_POKE: "mG/I:d" counts VSyncs from
 * the latest entry into mode G/I; a plain number is an absolute VSync. */
static int anchor_mode = -1;
static long anchor_entry;

static void update_anchor(long vsync) {
    int mode = (ModeGroup << 8) | (ModeIndex & 0xFF);
    if (mode != anchor_mode) {
        anchor_mode = mode;
        anchor_entry = vsync;
    }
}

/* VSyncs on the anchor's clock, or -1 while outside the anchored mode. */
static long anchor_time(int mode, long vsync) {
    if (mode < 0)
        return vsync;
    return mode == anchor_mode ? vsync - anchor_entry : -1;
}

/* ---- RR4_POKE ---- */

#define POKE_MAX 32
static struct {
    void *addr;
    int size, add, mode;
    long value, vsync;
    const int *value_from; /* value read from this global when applied */
} pokes[POKE_MAX];
static int poke_count;

static void setup_pokes(const char *list) {
    static Rr4PokeSpec specs[POKE_MAX];
    int count = Rr4DiagParsePoke(list, specs, POKE_MAX);

    for (int i = 0; i < count; i++) {
        const Rr4PokeSpec *k = &specs[i];
        pokes[poke_count].value_from = NULL;
        if (k->from[0]) {
            const char *from = find_symbol(k->from);
            if (!from) {
                fprintf(stderr, "RR4_POKE: no symbol %s\n", k->from);
                continue;
            }
            pokes[poke_count].value_from = (const int *)(from + k->from_offset);
        }
        pokes[poke_count].value = k->value;
        pokes[poke_count].mode = k->mode;
        pokes[poke_count].vsync = k->vsync;
        pokes[poke_count].size = k->size;
        pokes[poke_count].add = k->add;
        if (!strcmp(k->name, "rand")) {
            /* The BIOS rand() state (PsyZ keeps it private): store only. */
            pokes[poke_count].addr = NULL;
            poke_count++;
            continue;
        }
        pokes[poke_count].addr = find_symbol(k->name);
        if (!pokes[poke_count].addr) {
            fprintf(stderr, "RR4_POKE: no symbol %s\n", k->name);
            continue;
        }
        pokes[poke_count].addr = (char *)pokes[poke_count].addr + k->offset;
        poke_count++;
    }
}

static void apply_pokes(long vsync) {
    for (int i = 0; i < poke_count; i++) {
        long v = pokes[i].value_from ? *pokes[i].value_from : pokes[i].value;
        if (anchor_time(pokes[i].mode, vsync) != pokes[i].vsync)
            continue;
        if (!pokes[i].addr) {
            Psyz_Srand((unsigned int)v);
            continue;
        }
        switch (pokes[i].size) {
        case 1:
            *(signed char *)pokes[i].addr = (signed char)(pokes[i].add ? *(signed char *)pokes[i].addr + v : v);
            break;
        case 2:
            *(short *)pokes[i].addr = (short)(pokes[i].add ? *(short *)pokes[i].addr + v : v);
            break;
        default:
            *(int *)pokes[i].addr = (int)(pokes[i].add ? *(int *)pokes[i].addr + v : v);
            break;
        }
    }
}

/* ---- RR4_PAD_SCRIPT and RR4_AUTODRIVE ---- */

static Rr4PadStep pad_script[128];
static int pad_script_len;
static int pad_script_ports[2];

/* RR4_AUTODRIVE: decided from PS1-layout fields of PlayerCars[0] (offsets
 * below the host-only control block are those of the original). */
extern unsigned char PlayerCars[];
extern int RaceState;
extern int ModeFrame;
static int autodrive;
static long autodrive_hold = -1; /* RR4_AUTODRIVE_HOLD, or -1 */
static FILE *autodrive_log;
static unsigned short *autodrive_replay; /* RR4_AUTODRIVE_REPLAY by ModeFrame */
static int autodrive_replay_state = -1;  /* -1 not read yet, 0 none, 1 loaded */

static void load_autodrive_replay(void) {
    const char *path = path_env("RR4_AUTODRIVE_REPLAY");
    FILE *in = path ? Psyz_FileOpen(path, "r") : NULL;
    int frame;
    unsigned m;

    autodrive_replay_state = 0;
    if (!in)
        return;
    autodrive_replay = calloc(65536, sizeof(*autodrive_replay));
    if (!autodrive_replay) {
        fprintf(stderr, "RR4_AUTODRIVE_REPLAY: out of memory, driving without %s\n", path);
        fclose(in);
        return;
    }
    autodrive_replay_state = 1;
    while (fscanf(in, "%d %u", &frame, &m) == 2)
        if (frame >= 0 && frame < 65536)
            autodrive_replay[frame] = (unsigned short)m;
    fclose(in);
}

static unsigned short autodrive_mask(void) {
    short lateral, track, heading;
    int error;
    unsigned short mask;

    if (!autodrive || RaceState < 2 || RaceState > 3)
        return 0;
    /* Only in the race update modes (RaceState is stale in menus after a
     * replay): VS 1/38, GP 1/47, TA 1/53, link 1/26, Extra trial 1/82. */
    if (ModeGroup != 1 || (ModeIndex != 26 && ModeIndex != 38 && ModeIndex != 47 &&
                           ModeIndex != 53 && ModeIndex != 82))
        return 0;
    /* RR4_AUTODRIVE_REPLAY=file: drive from a RR4_AUTODRIVE_LOG instead. */
    if (autodrive_replay_state < 0)
        load_autodrive_replay();
    if (autodrive_replay_state > 0)
        return ModeFrame >= 0 && ModeFrame < 65536 ? autodrive_replay[ModeFrame] : 0;
    lateral = *(const short *)(PlayerCars + 0x172);
    track = *(const short *)(PlayerCars + 0x1EC);
    heading = (short)*(const int *)(PlayerCars + 0x54);
    error = lateral / 2;
    if (error > 400)
        error = 400;
    if (error < -400)
        error = -400;
    /* The track heading runs mirrored to the car's yaw (0x800 - yaw). */
    error = ((0x800 - track - error - heading + 0x800) & 0xFFF) - 0x800;
    mask = error > 300 || error < -300 ? 0 : 0x0040; /* cross: accelerate */
    if (error > 24)
        mask |= 0x2000; /* right */
    else if (error < -24)
        mask |= 0x8000; /* left */
    if (autodrive_hold >= 0 && ModeFrame < autodrive_hold)
        mask = 0x0040; /* straight on until that ModeFrame */
    if (autodrive_log)
        fprintf(autodrive_log, "%d %u\n", ModeFrame, mask);
    return mask;
}

/* A stick position -1..1 as the host's SDL axis reaches the frame. */
static unsigned char pad_stick_byte(float v) {
    int axis = (int)(v * 32767.0f);
    int u;
    if (axis < -32768)
        axis = -32768;
    if (axis > 32767)
        axis = 32767;
    u = (axis + 32768) >> 8;
    return (unsigned char)(u > 0xFF ? 0xFF : u);
}

static void apply_pad_port(int port, long vsync) {
    unsigned short mask = port == 0 ? autodrive_mask() : 0;
    float axes[RR4_PAD_AXES] = {0};
    int moved = 0;
    for (int i = 0; i < pad_script_len; i++) {
        long t = anchor_time(pad_script[i].mode, vsync) - pad_script[i].first;
        if (pad_script[i].port != port || t < 0)
            continue;
        if (pad_script[i].period > 0)
            t %= pad_script[i].period;
        if (t < pad_script[i].count) {
            mask |= pad_script[i].mask;
            if (pad_script[i].axis != RR4_PAD_AXIS_NONE) {
                axes[pad_script[i].axis] = pad_script[i].value;
                moved = 1;
            }
        }
    }
    /* Any script keeps pad 1 connected; pad 2 only when a step names it. */
    int scripted = port == 0 ? pad_script_len > 0 : pad_script_ports[1];
    if (!mask && !moved && (!scripted || PadPorts[port].status00 == 0))
        return;
    /* A scripted run always has a controller of the port's type (RR4_PAD_TYPE,
     * a digital pad by default) in each scripted port: between presses an
     * idle controller stays connected, as on the console, instead of the
     * host's disconnected port (the game re-detects the controller type
     * after every reconnect and ignores input meanwhile). The frame also
     * goes to PsyZ so that libpad (PadGetState) sees the same controller
     * until the next VSync. */
    {
        char frame[PSYZ_PAD_BUF_LEN];
        PsyzPadInput in = {0};
        PsyzControllerKind kind = Psyz_PadsSetKind(port, 0, PSYZ_CTRL_QUERY_KIND);
        if (kind == PSYZ_CTRL_DISCONNECTED)
            kind = PSYZ_CTRL_DIGITAL_PAD;
        in.buttons = mask;
        in.left_x = pad_stick_byte(axes[RR4_PAD_AXIS_LX]);
        in.left_y = pad_stick_byte(axes[RR4_PAD_AXIS_LY]);
        in.right_x = pad_stick_byte(axes[RR4_PAD_AXIS_RX]);
        in.right_y = pad_stick_byte(axes[RR4_PAD_AXIS_RY]);
        in.steering = axes[RR4_PAD_AXIS_STEER];
        in.pedal[0] = axes[RR4_PAD_AXIS_I];
        in.pedal[1] = axes[RR4_PAD_AXIS_II];
        in.pedal[2] = axes[RR4_PAD_AXIS_L];
        if (Psyz_PadsBuildFrame(port, kind, &in, frame) != 0)
            return;
        Psyz_PadsSet(port, frame, sizeof(frame));
        memcpy(&PadPorts[port], frame, sizeof(frame));
    }
}

/* ---- captures ---- */

static void write_ppm(const char *path, unsigned char *rgb, int w, int h) {
    FILE *f;

    if (!rgb || w <= 0 || h <= 0)
        return;
    f = Psyz_FileOpen(path, "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        fwrite(rgb, 3, (size_t)w * (size_t)h, f);
        fclose(f);
    }
    free(rgb);
}

/* RR4_AUDIO_CAPTURE: the PCM data follows a 44-byte WAV header that
 * finish_audio_capture fills in once the length is known. */
static FILE *audio_capture;
static unsigned long audio_capture_frames;

static void capture_audio(const short *samples, int frames, void *userdata) {
    (void)userdata;
    if (audio_capture && frames > 0) {
        fwrite(samples, 4, (size_t)frames, audio_capture);
        audio_capture_frames += (unsigned long)frames;
    }
}

static void put_le(unsigned char *p, unsigned long v, int n) {
    for (int i = 0; i < n; i++)
        p[i] = (unsigned char)(v >> (8 * i));
}

static void start_audio_capture(const char *path) {
    unsigned char header[44] = {0};

    audio_capture = Psyz_FileOpen(path, "wb");
    if (!audio_capture) {
        fprintf(stderr, "ridge-racer-type-4-pc: cannot write %s\n", path);
        return;
    }
    fwrite(header, 1, sizeof(header), audio_capture);
    Psyz_AudioSetCaptureCallback(capture_audio, NULL);
}

static void finish_audio_capture(void) {
    unsigned char h[44];
    unsigned long bytes;

    if (!audio_capture)
        return;
    Psyz_AudioSetCaptureCallback(NULL, NULL);
    bytes = audio_capture_frames * 4;
    memcpy(h, "RIFF", 4);
    put_le(h + 4, 36 + bytes, 4);
    memcpy(h + 8, "WAVEfmt ", 8);
    put_le(h + 16, 16, 4);
    put_le(h + 20, 1, 2);         /* PCM */
    put_le(h + 22, 2, 2);         /* stereo */
    put_le(h + 24, 44100, 4);
    put_le(h + 28, 44100 * 4, 4); /* bytes per second */
    put_le(h + 32, 4, 2);         /* bytes per frame */
    put_le(h + 34, 16, 2);
    memcpy(h + 36, "data", 4);
    put_le(h + 40, bytes, 4);
    fseek(audio_capture, 0, SEEK_SET);
    fwrite(h, 1, sizeof(h), audio_capture);
    fclose(audio_capture);
    audio_capture = NULL;
}

/* The captures taken when the run ends. */
static void finish(void) {
    const char *path;
    unsigned char *rgb;
    int w, h;

    finish_audio_capture();

    /* the frame first, then its size: argument evaluation order is
     * unspecified (GCC on x86-64 reads w and h before the call) */
    path = path_env("RR4_CAPTURE");
    if (path) {
        rgb = Psyz_VideoAllocRenderedFrame(&w, &h);
        write_ppm(path, rgb, w, h);
    }
    path = path_env("RR4_VRAM");
    if (path) {
        rgb = Psyz_VideoAllocVramDump(&w, &h);
        write_ppm(path, rgb, w, h);
    }
}

/* RR4_CAPTURE_AT entries: VSyncs first..last, every step-th. */
static Rr4VsyncRange capture_at[64];
static int capture_at_len;
static int capture_mid;
static const char *capture_base;

static void capture_at_vsync(long vsync) {
    /* midpoints presented by each of the last VSyncs */
    static unsigned long long presented[4];
    presented[vsync & 3] = Psyz_GpuGetInterpolatedPresentCount();
    for (int i = 0; i < capture_at_len; i++) {
        if (vsync >= capture_at[i].first && vsync <= capture_at[i].last &&
            (vsync - capture_at[i].first) % capture_at[i].step == 0) {
            char path[512];
            int w, h;
            snprintf(path, sizeof(path), "%s.%ld.ppm", capture_base, vsync);
            unsigned char *rgb = Psyz_VideoAllocRenderedFrame(&w, &h);
            write_ppm(path, rgb, w, h);
            if (capture_mid) {
                unsigned char *mid = Psyz_VideoAllocMidpointFrame(&w, &h);
                snprintf(path, sizeof(path), "%s.%ld.mid.ppm", capture_base, vsync);
                if (mid)
                    write_ppm(path, mid, w, h);
                if (mid && presented[vsync & 3] == presented[(vsync - 2) & 3]) {
                    FILE *held;
                    snprintf(path, sizeof(path), "%s.%ld.held", capture_base, vsync);
                    held = Psyz_FileOpen(path, "w");
                    if (held)
                        fclose(held);
                }
            }
        }
    }
}

static Rr4VsyncRange gpu_dump_at[64];
static int gpu_dump_at_len;
static long gpu_dump_open = -1;
static long gpu_dump_len = 1;
static const char *gpu_dump_base;

static void gpu_dump_vsync(long vsync) {
    char path[512];

    if (gpu_dump_open >= 0 && vsync - gpu_dump_open >= gpu_dump_len) {
        snprintf(path, sizeof(path), "%s.%ld.vram", gpu_dump_base, gpu_dump_open);
        Psyz_GpuDumpEnd(path);
        gpu_dump_open = -1;
    }
    for (int i = 0; i < gpu_dump_at_len; i++) {
        if (gpu_dump_at[i].first == vsync && gpu_dump_open < 0) {
            snprintf(path, sizeof(path), "%s.%ld.psxgpu", gpu_dump_base, vsync);
            if (Psyz_GpuDumpBegin(path) == 0)
                gpu_dump_open = vsync;
        }
    }
}

/* ---- run-wide overrides ---- */

static void apply_graphics(Rr4Prefs *prefs, const char *list, int interactive) {
    unsigned bits = Rr4DiagParseGraphics(list);

    if (interactive) {
        /* Named options on; the others as the preferences have them. */
        prefs->geometry_correction |= (bits & RR4_GRAPHICS_GEOMETRY) != 0;
        prefs->disable_dithering |= (bits & RR4_GRAPHICS_NODITHER) != 0;
        prefs->antialiasing |= (bits & RR4_GRAPHICS_AA) != 0;
        prefs->texture_filtering |= (bits & RR4_GRAPHICS_FILTER) != 0;
        prefs->frame_interpolation |= (bits & RR4_GRAPHICS_INTERPOLATION) != 0;
        if (bits & RR4_GRAPHICS_WIDE)
            prefs->aspect_ratio = RR4_ASPECT_WIDESCREEN;
    } else {
        prefs->geometry_correction = (bits & RR4_GRAPHICS_GEOMETRY) != 0;
        prefs->disable_dithering = (bits & RR4_GRAPHICS_NODITHER) != 0;
        prefs->antialiasing = (bits & RR4_GRAPHICS_AA) != 0;
        prefs->texture_filtering = (bits & RR4_GRAPHICS_FILTER) != 0;
        prefs->frame_interpolation = (bits & RR4_GRAPHICS_INTERPOLATION) != 0;
        prefs->aspect_ratio = bits & RR4_GRAPHICS_WIDE ? RR4_ASPECT_WIDESCREEN : RR4_ASPECT_CLASSIC;
    }
    Psyz_VideoSetGeometryCorrection(prefs->geometry_correction);
    Psyz_VideoSetDitheringMode(prefs->disable_dithering ? PSYZ_DITHER_OFF : PSYZ_DITHER_AUTO);
    Psyz_VideoSetAntialiasing(prefs->antialiasing);
    Psyz_VideoSetModelFiltering(prefs->texture_filtering);
    Psyz_VideoSetAspectMode(prefs->aspect_ratio == RR4_ASPECT_WIDESCREEN ? PSYZ_ASPECT_SQUARE
                                                                          : PSYZ_ASPECT_DISPLAY);
    Psyz_GpuSetFrameInterpolation(prefs->frame_interpolation);
    Rr4ApplyWidescreen(prefs->aspect_ratio == RR4_ASPECT_WIDESCREEN);
}

void Rr4DiagnosticsInit(Rr4Prefs *prefs, int interactive) {
    const char *text;

    text = getenv("RR4_RASTERIZER");
    if (text && !strcmp(text, "gpu"))
        Psyz_VideoSetNativeRasterizer(0);
    {
        long scale = env_long("RR4_INTERNAL_RES", 0);
        /* The settings menu applies the preferences again (headless with
         * RR4_SETTINGS_SCRIPT too): keep this scale there. */
        if (scale > 0 && Psyz_VideoSetInternalResolution((unsigned)scale) == 0)
            prefs->scale = (unsigned)scale;
    }
    text = getenv("RR4_GRAPHICS");
    if (text && *text)
        apply_graphics(prefs, text, interactive);
    frame_limit = env_long("RR4_FRAME_LIMIT", 0);
    trace_every = env_long("RR4_TRACE", 0);
    frame_log = SDL_getenv("PSYZ_FRAME_LOG") != NULL;
    trace_rand = getenv("RR4_TRACE_RAND") != NULL;
    setup_watches(getenv("RR4_WATCH"));
    pad_script_len = Rr4DiagParsePadScript(getenv("RR4_PAD_SCRIPT"), pad_script,
                                           (int)(sizeof(pad_script) / sizeof(pad_script[0])), pad_script_ports);
    {
        int kinds[2];
        int ports = Rr4DiagParsePadTypes(getenv("RR4_PAD_TYPE"), kinds);
        for (int port = 0; port < ports; port++) {
            prefs->pad_type[port] = Rr4PadTypeFromKind(kinds[port]);
            Psyz_PadsSetKind(port, 0, Rr4PadTypeKind(prefs->pad_type[port]));
        }
    }
    setup_pokes(getenv("RR4_POKE"));
    if (path_env("RR4_AUDIO_CAPTURE"))
        start_audio_capture(path_env("RR4_AUDIO_CAPTURE"));
    autodrive = env_long("RR4_AUTODRIVE", 0) != 0;
    autodrive_hold = env_long("RR4_AUTODRIVE_HOLD", -1);
    if (autodrive && path_env("RR4_AUTODRIVE_LOG"))
        autodrive_log = Psyz_FileOpen(path_env("RR4_AUTODRIVE_LOG"), "w");
    gpu_dump_at_len = Rr4DiagParseRanges(getenv("RR4_GPU_DUMP_AT"), 0, gpu_dump_at, 64, "RR4_GPU_DUMP_AT");
    gpu_dump_len = env_long("RR4_GPU_DUMP_LEN", 1);
    gpu_dump_base = path_env("RR4_GPU_DUMP") ? path_env("RR4_GPU_DUMP") : "gpudump";
    capture_at_len = Rr4DiagParseRanges(getenv("RR4_CAPTURE_AT"), 1, capture_at, 64, "RR4_CAPTURE_AT");
    capture_mid = getenv("RR4_CAPTURE_MID") && *getenv("RR4_CAPTURE_MID") == '1';
    capture_base = path_env("RR4_CAPTURE") ? path_env("RR4_CAPTURE") : "capture";
}

int Rr4DiagnosticsVSync(long vsync, int quitting) {
    capture_at_vsync(vsync);
    gpu_dump_vsync(vsync);
    update_anchor(vsync);
    apply_pad_port(0, vsync);
    apply_pad_port(1, vsync);
    apply_pokes(vsync);
    if (trace_every > 0 && vsync % trace_every == 0) {
        fprintf(stderr, "vsync %ld frame %u mode %u/%d sub %u", vsync,
                FrameCounter, ModeGroup, ModeIndex, SubmodeState);
        print_watches();
        if (Psyz_GpuGetFrameInterpolation()) {
            unsigned prims, tagged, paired;
            Psyz_GpuGetInterpolationStats(&prims, &tagged, &paired);
            fprintf(stderr, " interp %u/%u/%u mid %llu", prims, tagged, paired,
                    Psyz_GpuGetInterpolatedPresentCount());
        }
        /* host time on the clock of PSYZ_FRAME_LOG's present_ms */
        if (frame_log)
            fprintf(stderr, " t %.3f",
                    (double)SDL_GetPerformanceCounter() * 1000.0 /
                        (double)SDL_GetPerformanceFrequency());
        fputc('\n', stderr);
    }
    if ((frame_limit > 0 && vsync >= frame_limit) || quitting) {
        finish();
        return 1;
    }
    return 0;
}
