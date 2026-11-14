#ifndef RR_RELEASE_H
#define RR_RELEASE_H

#include "types.h"
#include "rr/video_timing.h"

/* The release of the mounted disc (docs/REGIONS.md).
 *
 * The PAL release (SCES-01706, boot file SCES_017.06) is the Japanese one
 * (SLPS-01800, SLPS_018.00) rebuilt: the same functions in the same order,
 * with the frame-rate conversions, a 256-line picture, languages, DualShock
 * steering, other card names and a re-laid-out R4.BIN. The port runs the
 * PAL code with the Japanese release's code next to it where the two
 * differ, chosen by the profile of the disc that was mounted:
 *
 *   RR_RELEASE_JP          the Japanese release is running
 *   RR_PAL_JP(pal, jp)     the PAL release's value or the Japanese one's
 *
 * The release follows the disc (the host identifies it from SYSTEM.CNF
 * before MainLoop) and is not changed while the game runs. The Japanese
 * release always runs at NTSC timing (VideoTimingNtsc): the frame-rate
 * values behind RR_PAL_NTSC are already the Japanese ones. The PAL release
 * runs at either timing (RR4_VIDEO); its picture, data and logic stay PAL.
 *
 * With the PAL disc every value is the original one. */

typedef enum RrRelease {
    RR_RELEASE_PAL = 0, /* Europe, SCES-01706 */
    RR_RELEASE_NTSC_J,  /* Japan and Asia, SLPS-01800 (also the JogCon bundle) */
    RR_RELEASE_COUNT
} RrRelease;

typedef struct RrReleaseProfile {
    RrRelease release;
    const char *bootFile;    /* as SYSTEM.CNF names it: "SCES_017.06" */
    const char *productCode; /* "SCES-01706" */
    const char *name;        /* "European PAL release" */
    s32 ntsc;                /* SetVideoMode(MODE_NTSC) and 30 race frames a second */
    s32 screenHeight;        /* display and draw environments: 256 or 240 lines */
    /* GetR4FileIndex: the first R4.BIN entry of groups 0..4. */
    s16 r4GroupBase[5];
    s16 r4Entries; /* entries in the R4.BIN directory */
} RrReleaseProfile;

/* Entries of the largest R4.BIN directory (the Japanese release's). */
#define RR_R4_ENTRIES_MAX 969

/* The running release (RR_RELEASE_PAL until the host selects one). */
extern RrRelease GameRelease;
extern const RrReleaseProfile *GameReleaseProfile;

#define RR_RELEASE_JP (GameRelease == RR_RELEASE_NTSC_J)
#define RR_PAL_JP(pal, jp) (RR_RELEASE_JP ? (jp) : (pal))

/* The bottom of the screen, for "some vertex on screen" culling tests: 256
 * lines in the PAL release, 240 in the Japanese one. */
#define RR_SCREEN_BOTTOM RR_PAL_JP(0x100, 0xF0)

/* The profile of a release, or of the release a boot file starts ("" or an
 * unknown file: NULL). The match ignores case. */
const RrReleaseProfile *RrReleaseProfileOf(RrRelease release);
const RrReleaseProfile *RrReleaseForBootFile(const char *bootFile);

/* Selects the release before MainLoop: GameRelease, GameReleaseProfile and,
 * for an NTSC release, VideoTimingNtsc. ntscTiming asks for NTSC timing on
 * the PAL release (RR4_VIDEO=ntsc); an NTSC release ignores it. Returns
 * whether the release runs at NTSC timing. */
s32 RrReleaseSelect(RrRelease release, s32 ntscTiming);

#endif
