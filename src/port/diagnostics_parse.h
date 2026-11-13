#ifndef RR4_PORT_DIAGNOSTICS_PARSE_H
#define RR4_PORT_DIAGNOSTICS_PARSE_H

#include <stddef.h>

/* The syntax of the RR4_* test hooks (diagnostics.c), without the game:
 * every list is parsed here and checked by tests/port/diagnostics_test.c.
 * Bad items are reported on stderr ("RR4_POKE: bad item ...") and skipped;
 * numbers must be whole (no trailing characters). */

/* A number in base (0: decimal, 0x hex or 0 octal as strtol) that fills
 * text up to *end (or the whole string when end is NULL). Returns 1 and
 * the value, or 0 for no digits, trailing characters (end NULL) or an
 * out-of-range value. */
int Rr4DiagParseLong(const char *text, const char **end, int base, long *out);

/* A VSync time: a plain number is absolute (*mode -1); "mG/I:d" counts d
 * VSyncs from each entry into ModeGroup G / ModeIndex I (*mode
 * G << 8 | I & 0xFF; "/I" and ":d" may be left out as 0). */
int Rr4DiagParseAnchor(const char *text, const char **end, int *mode, long *time);

/* RR4_PAD_SCRIPT steps. Button masks are the game's button word,
 * ~((PadPorts[].data02 << 8) | data03). */
enum {
    RR4_PAD_AXIS_NONE = -1,
    RR4_PAD_AXIS_LX,
    RR4_PAD_AXIS_LY,
    RR4_PAD_AXIS_RX,
    RR4_PAD_AXIS_RY,
    RR4_PAD_AXIS_STEER,
    RR4_PAD_AXIS_I,
    RR4_PAD_AXIS_II,
    RR4_PAD_AXIS_L,
    RR4_PAD_AXES
};
typedef struct Rr4PadStep {
    unsigned short mask;
    int axis; /* RR4_PAD_AXIS_*, mask 0 */
    float value;
    int port, mode;
    long first, count, period;
} Rr4PadStep;
/* Parses up to max steps; ports_used[port] is set for each port a step
 * names. Returns the number of steps. */
int Rr4DiagParsePadScript(const char *script, Rr4PadStep *steps, int max, int ports_used[2]);
unsigned short Rr4DiagButtonMask(const char *name, size_t length);

/* RR4_CAPTURE_AT entries first[-last][/step]; RR4_GPU_DUMP_AT plain
 * numbers (first == last, step 1). what names the variable in messages. */
typedef struct Rr4VsyncRange {
    long first, last, step;
} Rr4VsyncRange;
int Rr4DiagParseRanges(const char *list, int ranges, Rr4VsyncRange *out, int max, const char *what);

/* RR4_WATCH name[+offset][:size]; label is the item without its size. */
typedef struct Rr4WatchSpec {
    char label[64];
    char name[80];
    long offset;
    int size; /* 1, 2 or 4 */
} Rr4WatchSpec;
int Rr4DiagParseWatch(const char *list, Rr4WatchSpec *out, int max);

/* RR4_POKE name[+offset][:size][+]=value@time: value is a number, or the
 * 32-bit global from[+from_offset] read when applied. The name "rand" is
 * the BIOS rand() state (store only). */
typedef struct Rr4PokeSpec {
    char name[96];
    long offset;
    int size, add;
    long value;
    char from[96]; /* "" for a plain value */
    long from_offset;
    int mode;
    long vsync;
} Rr4PokeSpec;
int Rr4DiagParsePoke(const char *list, Rr4PokeSpec *out, int max);

/* RR4_GRAPHICS: a comma list of exact option names. */
enum {
    RR4_GRAPHICS_GEOMETRY = 1 << 0,
    RR4_GRAPHICS_NODITHER = 1 << 1,
    RR4_GRAPHICS_AA = 1 << 2,
    RR4_GRAPHICS_FILTER = 1 << 3,
    RR4_GRAPHICS_INTERPOLATION = 1 << 4,
    RR4_GRAPHICS_WIDE = 1 << 5,
};
unsigned Rr4DiagParseGraphics(const char *list);

/* RR4_PAD_TYPE: up to two controller names (digital dualshock negcon
 * jogcon) as PsyZ PsyzControllerKind values; an unknown name is reported
 * and counts as digital. Returns the number of ports given. */
int Rr4DiagParsePadTypes(const char *list, int kinds[2]);

#endif
