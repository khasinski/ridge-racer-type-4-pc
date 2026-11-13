#ifndef RR4_PORT_DISC_CHECK_H
#define RR4_PORT_DISC_CHECK_H

#include <stddef.h>

#include "preferences.h"

/* Which disc a CUE image holds, and choosing the one the game starts with.
 *
 * The port runs the European PAL release (SCES-01706). Other releases have
 * different files at different places in R4.BIN and on the disc, which shows
 * as broken textures or a hang, so an image is identified by the boot file
 * its SYSTEM.CNF names ("BOOT = cdrom:\SCES_017.06;1") before it is mounted.
 * The releases the check knows are a table keyed by that file
 * (disc_check.c): support for another release is one more entry marked
 * supported. */

enum { RR4_DISC_SECTOR = 2352, RR4_DISC_MESSAGE_MAX = 512 };

typedef enum Rr4DiscStatus {
    RR4_DISC_OK = 0,           /* a supported release */
    RR4_DISC_UNREADABLE,       /* the CUE or its BIN cannot be read */
    RR4_DISC_NOT_PLAYSTATION,  /* no ISO 9660 volume or no SYSTEM.CNF boot file */
    RR4_DISC_OTHER_RELEASE,    /* Ridge Racer Type 4, but not a supported release */
    RR4_DISC_OTHER_GAME,       /* another PlayStation disc */
} Rr4DiscStatus;

typedef struct Rr4DiscRelease {
    const char *boot;  /* boot file as SYSTEM.CNF names it, upper case: "SCES_017.06" */
    const char *name;  /* "European PAL release" */
    int supported;
} Rr4DiscRelease;

/* Reads the raw 2352-byte sector lba of an image into out; 0 on success. */
typedef int (*Rr4DiscSectorReader)(void *context, int lba, unsigned char *out);

/* The boot file of SYSTEM.CNF text ("BOOT = cdrom:\SCES_017.06;1" gives
 * "SCES_017.06"): the BOOT line's file name without device, directories
 * and version, in upper case. Returns 0 without a BOOT line. */
int Rr4DiscParseSystemCnf(const char *text, size_t length, char *boot, size_t size);

/* The known release with that boot file, or NULL. */
const Rr4DiscRelease *Rr4DiscFindRelease(const char *boot);

/* Classifies a boot file ("" or NULL: no SYSTEM.CNF) and writes the
 * message a player sees for anything but RR4_DISC_OK. */
Rr4DiscStatus Rr4DiscClassify(const char *boot, char *message, size_t size);

/* Finds SYSTEM.CNF in the image's ISO 9660 root directory and returns its
 * boot file (as Rr4DiscParseSystemCnf). RR4_DISC_UNREADABLE when sector 16
 * cannot be read, RR4_DISC_NOT_PLAYSTATION without a volume or a boot file,
 * RR4_DISC_OK otherwise (the release is not judged here). */
Rr4DiscStatus Rr4DiscReadBoot(Rr4DiscSectorReader read, void *context, char *boot, size_t size);

/* Identifies the image of a CUE file through PsyZ without mounting it (the
 * drive keeps its disc and position). message gets what a player sees for
 * anything but RR4_DISC_OK. */
Rr4DiscStatus Rr4DiscCheckImage(const char *cue, char *message, size_t size);

/* The boot file of a CUE image's SYSTEM.CNF ("" when it has none), read as
 * Rr4DiscCheckImage reads it; the game's release profile follows it
 * (rr/release.h RrReleaseForBootFile). */
Rr4DiscStatus Rr4DiscImageBoot(const char *cue, char *boot, size_t size);

/* ---- the image the game starts with ---- */

typedef struct Rr4DiscHost {
    void *context;
    /* The image file dialog: 1 and the path in out, 0 when cancelled. */
    int (*choose)(void *context, char *out, size_t size);
    /* Tells the player why an image cannot be used (a message box). */
    void (*alert)(void *context, const char *message);
    /* Rr4DiscCheckImage */
    Rr4DiscStatus (*check)(void *context, const char *cue, char *message, size_t size);
    /* Mounts the image in the drive (Psyz_CdSetDiskPath); 0 on success. */
    int (*mount)(void *context, const char *cue);
    /* Writes the preferences; prefs.disc changed. */
    void (*save)(void *context, const Rr4Prefs *prefs);
} Rr4DiscHost;

/* Checks and mounts the image to play: requested (the command line or
 * RR4_DISC) if given, else the saved prefs->disc, else one the player
 * chooses. Interactive runs never give up on a bad image: they explain it
 * (alert), forget a saved path that failed (cleared and saved) and ask
 * again, until a good image mounts or the dialog is cancelled. A chosen
 * image is saved as prefs->disc only once it has mounted. A headless run
 * (interactive 0) neither asks nor saves: the first image decides, and its
 * problem is left in message and status. Returns the mounted path
 * (requested, or prefs->disc), or NULL: then message and status say why
 * ("" and RR4_DISC_OK when the player cancelled or nothing was given). */
const char *Rr4DiscSelect(const Rr4DiscHost *host, Rr4Prefs *prefs, const char *requested,
                          int interactive, Rr4DiscStatus *status, char *message, size_t size);

#endif
