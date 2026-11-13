#ifndef RR4_PORT_PREFERENCES_H
#define RR4_PORT_PREFERENCES_H

#include <stddef.h>

/* Desktop preferences: presentation, audio, input bindings and the disc
 * image, kept as TOML in ridge-racer-type-4.toml. The file is chosen as in
 * the Parasite Eve port:
 *   1. RR4_CONFIG_HOME/ridge-racer-type-4.toml when RR4_CONFIG_HOME is set;
 *   2. a portable ridge-racer-type-4.toml next to the executable (or in the
 *      working directory) when one already exists;
 *   3. the per-user configuration directory (XDG_CONFIG_HOME or
 *      ~/.config/ridge-racer-type-4 on Linux, the SDL preference path on
 *      macOS and Windows).
 * The first of these is the one file read and written; the others are
 * ignored. Its directory also holds the memory cards (memory_cards.h) and
 * the log (host_log.h). Headless runs never read or write preferences. */

enum {
    RR4_PREFS_BUTTONS = 16, /* PsyZ pad bit order: L2 R2 L1 R1 ... Left */
    RR4_PREFS_PATH_MAX = 4096,
};

typedef enum Rr4VsyncPref {
    RR4_VSYNC_AUTO = 0,
    RR4_VSYNC_ON = 1,
    RR4_VSYNC_OFF = 2, /* PsyZ frame limiter */
} Rr4VsyncPref;

typedef enum Rr4AspectPref {
    RR4_ASPECT_CLASSIC = 0,    /* the PAL display aspect, everything 4:3 */
    RR4_ASPECT_WIDESCREEN = 1, /* square pixels; races 16:9, the rest 4:3 */
} Rr4AspectPref;

typedef enum Rr4VideoTimingPref {
    RR4_VIDEO_PAL = 0,  /* the PAL release: 49.75 Hz fields, races at 25 fps */
    RR4_VIDEO_NTSC = 1, /* the NTSC release's timing: 59.82 Hz, races at 30 fps */
} Rr4VideoTimingPref;

/* What each controller port emulates (PsyZ PsyzControllerKind). The PAL
 * release reads all four: the digital pad, the DUAL SHOCK in analog mode
 * (left stick steers), the neGcon (twist steers, analog I / II / L) and the
 * JogCon (dial steers, force feedback). */
typedef enum Rr4PadType {
    RR4_PAD_DIGITAL = 0,
    RR4_PAD_DUALSHOCK = 1,
    RR4_PAD_NEGCON = 2,
    RR4_PAD_JOGCON = 3,
    RR4_PAD_TYPES
} Rr4PadType;

/* Host axis response for an emulated analog input, DuckStation's terms:
 * dead zone and saturation as fractions of the travel, the travelled part
 * raised to e^linearity (0 linear), then times scaling. */
/* The link cable over the network (PsyZ psyz/link.h). */
typedef enum Rr4LinkPref {
    RR4_LINK_OFF = 0,
    RR4_LINK_HOST = 1, /* wait for the other player on link_port */
    RR4_LINK_JOIN = 2, /* connect to link_address:link_port */
} Rr4LinkPref;

enum { RR4_LINK_DEFAULT_PORT = 47400, RR4_LINK_ADDRESS_MAX = 256 };

typedef struct Rr4AxisPref {
    float deadzone, saturation, linearity, scaling;
} Rr4AxisPref;

enum {
    RR4_AXIS_STEERING = 0, /* neGcon twist / JogCon dial from the left stick */
    RR4_AXIS_THROTTLE = 1, /* neGcon I from the right trigger */
    RR4_AXIS_BRAKE = 2,    /* neGcon II from the left trigger */
    RR4_AXES = 3,
};

typedef struct Rr4Prefs {
    /* [graphics] */
    unsigned scale; /* internal resolution: 1, 2, 4, 6 or 8 */
    int fullscreen;
    int fullscreen_field_rate; /* fullscreen at 50 Hz when the display can */
    int vsync; /* Rr4VsyncPref */
    int disable_dithering;
    int antialiasing;
    int texture_filtering;
    int geometry_correction;
    int frame_interpolation;
    int aspect_ratio; /* Rr4AspectPref */
    int video_timing; /* Rr4VideoTimingPref, used from the next start */
    /* [audio] */
    unsigned volume; /* 0..100 */
    int mute;
    /* [input] */
    int pad_type[2];  /* Rr4PadType of pad 1 and pad 2 */
    int force_feedback; /* gamepad rumble for the game's motors */
    Rr4AxisPref axes[RR4_AXES];
    /* [link] */
    int link_mode; /* Rr4LinkPref */
    unsigned link_port;
    char link_address[RR4_LINK_ADDRESS_MAX];
    /* [disc] */
    char disc[RR4_PREFS_PATH_MAX];
    /* [keyboard] SDL scancodes, [controller] SDL gamepad buttons (plus
     * SDL_GAMEPAD_BUTTON_COUNT / +1 for the left / right trigger). */
    int keys[RR4_PREFS_BUTTONS];
    int buttons[RR4_PREFS_BUTTONS];
} Rr4Prefs;

/* Psyz_VideoSetWidescreen for RR4: buffers already drawn stay 4:3. */
void Rr4ApplyWidescreen(int enabled);

/* Lower-case pad button names, the TOML keys ("l2" ... "left"). */
const char *Rr4PrefsButtonKey(int button);
/* Display names ("L2" ... "Left"). */
const char *Rr4PrefsButtonLabel(int button);

/* Records the bindings PsyZ starts with as the defaults. Call before
 * anything changes them; later calls do nothing. */
void Rr4PrefsCaptureDefaults(void);
/* The defaults: original presentation, full volume, PsyZ's bindings. */
const Rr4Prefs *Rr4PrefsDefaults(void);

/* Reads one TOML file over prefs. A missing file is not an error; invalid
 * lines are reported on stderr and skipped. Returns 1 if the file existed. */
int Rr4PrefsLoadFile(Rr4Prefs *prefs, const char *path);
/* Writes prefs atomically (temporary file and rename). Returns 1 on success. */
int Rr4PrefsSaveFile(const Rr4Prefs *prefs, const char *path);

/* Chooses the preferences file as described above, creating its
 * directory. The path is kept for Rr4PrefsSave and Rr4PrefsPath; returns 0
 * (and logs why) if no location could be determined or created. */
int Rr4PrefsResolve(void);
/* Resolves the file (Rr4PrefsResolve) and loads the defaults, then the
 * file, into prefs. Returns 0 if there is no location. */
int Rr4PrefsLoad(Rr4Prefs *prefs);
/* The resolved file's directory with its trailing separator, or "" (and 0)
 * before a location is resolved. */
int Rr4PrefsDirectory(char *out, size_t size);
int Rr4PrefsSave(const Rr4Prefs *prefs);
const char *Rr4PrefsPath(void);

/* Applies prefs to PsyZ. Window-dependent options (fullscreen) only take
 * effect when window_ready is non-zero. */
void Rr4PrefsApply(const Rr4Prefs *prefs, int window_ready);

/* Controller types: PsyZ kind, TOML name ("digital" ...), display name. */
int Rr4PadTypeKind(int type);
int Rr4PadTypeFromKind(int kind);
const char *Rr4PadTypeKey(int type);
const char *Rr4PadTypeLabel(int type);

/* Human-readable binding names, never NULL. */
const char *Rr4PrefsKeyName(int scancode);
const char *Rr4PrefsGamepadName(int binding);

#endif
