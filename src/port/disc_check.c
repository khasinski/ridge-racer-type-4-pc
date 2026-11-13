#include "disc_check.h"

#include <psyz/cd.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define SUPPORTED_DISC "the European PAL disc SCES-01706"

/* Ridge Racer Type 4 discs by boot file. Only the European release is
 * playable; another release becomes playable by marking it supported here
 * once the port handles its data. */
static const Rr4DiscRelease kReleases[] = {
    {"SCES_017.06", "European PAL release", 1},
    {"SCUS_944.03", "USA release", 0},
    {"SLPS_018.00", "Japanese release", 0}, /* also the JogCon bundle */
};

const Rr4DiscRelease *Rr4DiscFindRelease(const char *boot) {
    for (size_t i = 0; boot && i < sizeof(kReleases) / sizeof(kReleases[0]); i++)
        if (!strcmp(kReleases[i].boot, boot))
            return &kReleases[i];
    return NULL;
}

/* "SCUS_944.03" as the label prints it, "SCUS-94403". */
static void ProductCode(const char *boot, char *out, size_t size) {
    size_t n = 0;
    for (; *boot && n + 1 < size; boot++)
        if (*boot != '.')
            out[n++] = *boot == '_' ? '-' : *boot;
    out[n] = '\0';
}

Rr4DiscStatus Rr4DiscClassify(const char *boot, char *message, size_t size) {
    const Rr4DiscRelease *release = Rr4DiscFindRelease(boot);
    char code[32];

    if (message && size)
        message[0] = '\0';
    if (!boot || !*boot) {
        snprintf(message, size, "This image is not a PlayStation game disc (it has no SYSTEM.CNF); "
                                "this port needs " SUPPORTED_DISC ".");
        return RR4_DISC_NOT_PLAYSTATION;
    }
    ProductCode(boot, code, sizeof(code));
    if (release && release->supported)
        return RR4_DISC_OK;
    if (release) {
        snprintf(message, size, "This is the %s of Ridge Racer Type 4 (%s); this port needs " SUPPORTED_DISC ".",
                 release->name, code);
        return RR4_DISC_OTHER_RELEASE;
    }
    snprintf(message, size, "This disc is not Ridge Racer Type 4 (it starts %s); this port needs " SUPPORTED_DISC ".",
             code);
    return RR4_DISC_OTHER_GAME;
}

int Rr4DiscParseSystemCnf(const char *text, size_t length, char *boot, size_t size) {
    const char *end = text + length;

    if (!boot || !size)
        return 0;
    boot[0] = '\0';
    while (text && text < end) {
        const char *line = text, *eol = line, *value, *name;
        size_t n = 0;

        while (eol < end && *eol != '\n' && *eol != '\r' && *eol != '\0')
            eol++;
        text = eol < end && *eol != '\0' ? eol + 1 : end;
        while (line < eol && isspace((unsigned char)*line))
            line++;
        if (eol - line < 4 || toupper((unsigned char)line[0]) != 'B' || toupper((unsigned char)line[1]) != 'O' ||
            toupper((unsigned char)line[2]) != 'O' || toupper((unsigned char)line[3]) != 'T')
            continue;
        value = line + 4;
        while (value < eol && (*value == ' ' || *value == '\t'))
            value++;
        if (value >= eol || *value != '=')
            continue; /* BOOT2 (a PlayStation 2 disc) or something else */
        value++;
        while (value < eol && (*value == ' ' || *value == '\t'))
            value++;
        /* cdrom:\DIR\SCES_017.06;1: the name after the device and directories */
        name = value;
        for (const char *c = value; c < eol; c++)
            if (*c == '\\' || *c == '/' || *c == ':')
                name = c + 1;
        while (name < eol && *name != ';' && *name != ' ' && *name != '\t' && n + 1 < size)
            boot[n++] = (char)toupper((unsigned char)*name++);
        boot[n] = '\0';
        return n > 0;
    }
    return 0;
}

static unsigned Le32(const unsigned char *p) {
    return (unsigned)p[0] | (unsigned)p[1] << 8 | (unsigned)p[2] << 16 | (unsigned)p[3] << 24;
}

/* User data of a raw sector: 24 bytes in for Mode 2 (subheader), 16 for Mode 1. */
static const unsigned char *UserData(const unsigned char *sector) {
    return sector + (sector[15] == 1 ? 16 : 24);
}

static int NameIs(const unsigned char *name, unsigned length, const char *want) {
    size_t n = strlen(want);
    if (length < n || (length > n && name[n] != ';'))
        return 0;
    for (size_t i = 0; i < n; i++)
        if (toupper(name[i]) != want[i])
            return 0;
    return 1;
}

Rr4DiscStatus Rr4DiscReadBoot(Rr4DiscSectorReader read, void *context, char *boot, size_t size) {
    unsigned char sector[RR4_DISC_SECTOR];
    const unsigned char *data;
    unsigned dir_lba, dir_size;

    if (boot && size)
        boot[0] = '\0';
    if (!read || read(context, 16, sector) != 0)
        return RR4_DISC_UNREADABLE;
    data = UserData(sector);
    if (data[0] != 1 || memcmp(data + 1, "CD001", 5))
        return RR4_DISC_NOT_PLAYSTATION;
    /* The root directory record of the primary volume descriptor. */
    dir_lba = Le32(data + 156 + 2);
    dir_size = Le32(data + 156 + 10);
    if (dir_size > 64 * 2048)
        dir_size = 64 * 2048;
    for (unsigned offset = 0; offset < dir_size; offset += 2048) {
        if (read(context, (int)(dir_lba + offset / 2048), sector) != 0)
            return RR4_DISC_NOT_PLAYSTATION;
        data = UserData(sector);
        for (unsigned at = 0; at + 33 < 2048 && data[at];) {
            unsigned length = data[at], name_length = data[at + 32];
            if (length < 34 || at + length > 2048 || 33 + name_length > length)
                break;
            if (!(data[at + 25] & 2) && NameIs(data + at + 33, name_length, "SYSTEM.CNF")) {
                unsigned file_lba = Le32(data + at + 2), file_size = Le32(data + at + 10);
                if (file_size > 2048)
                    file_size = 2048; /* the BOOT line comes first */
                if (read(context, (int)file_lba, sector) != 0)
                    return RR4_DISC_NOT_PLAYSTATION;
                return Rr4DiscParseSystemCnf((const char *)UserData(sector), file_size, boot, size)
                           ? RR4_DISC_OK
                           : RR4_DISC_NOT_PLAYSTATION;
            }
            at += length;
        }
    }
    return RR4_DISC_NOT_PLAYSTATION;
}

static int ReadImageSector(void *context, int lba, unsigned char *out) {
    return Psyz_CdReadImageSector((const char *)context, lba, out);
}

Rr4DiscStatus Rr4DiscCheckImage(const char *cue, char *message, size_t size) {
    char boot[64];
    Rr4DiscStatus status;

    if (message && size)
        message[0] = '\0';
    status = cue && *cue ? Rr4DiscReadBoot(ReadImageSector, (void *)cue, boot, sizeof(boot))
                         : RR4_DISC_UNREADABLE;
    if (status == RR4_DISC_UNREADABLE) {
        snprintf(message, size, "Cannot open the disc image %s. Keep the CUE file next to its BIN file.",
                 cue ? cue : "");
        return status;
    }
    return Rr4DiscClassify(status == RR4_DISC_OK ? boot : NULL, message, size);
}

Rr4DiscStatus Rr4DiscImageBoot(const char *cue, char *boot, size_t size) {
    if (boot && size)
        boot[0] = '\0';
    if (!cue || !*cue)
        return RR4_DISC_UNREADABLE;
    return Rr4DiscReadBoot(ReadImageSector, (void *)cue, boot, size);
}

/* ---- the image the game starts with ---- */

const char *Rr4DiscSelect(const Rr4DiscHost *host, Rr4Prefs *prefs, const char *requested,
                          int interactive, Rr4DiscStatus *status_out, char *message, size_t size) {
    enum { FROM_REQUEST, FROM_SAVED, FROM_DIALOG } from = FROM_REQUEST;
    static char chosen[RR4_PREFS_PATH_MAX];
    const char *cue = requested && *requested ? requested : NULL;
    Rr4DiscStatus status = RR4_DISC_OK;

    if (message && size)
        message[0] = '\0';
    if (!cue && interactive && prefs->disc[0]) {
        cue = prefs->disc;
        from = FROM_SAVED;
    }
    for (;;) {
        if (!cue) {
            if (!interactive || !host->choose(host->context, chosen, sizeof(chosen))) {
                /* Nothing to play: the usage, or the player cancelled (and
                 * has seen any earlier image's problem). */
                status = RR4_DISC_OK;
                if (message && size)
                    message[0] = '\0';
                break;
            }
            cue = chosen;
            from = FROM_DIALOG;
        }
        status = host->check(host->context, cue, message, size);
        if (status == RR4_DISC_OK && host->mount(host->context, cue) != 0) {
            status = RR4_DISC_UNREADABLE;
            snprintf(message, size, "Cannot open the disc image %s. Keep the CUE file next to its BIN file.", cue);
        }
        if (status == RR4_DISC_OK) {
            if (message && size)
                message[0] = '\0';
            if (from == FROM_DIALOG && strlen(cue) < sizeof(prefs->disc)) {
                snprintf(prefs->disc, sizeof(prefs->disc), "%s", cue);
                host->save(host->context, prefs);
                cue = prefs->disc;
            }
            if (status_out)
                *status_out = RR4_DISC_OK;
            return cue;
        }
        if (!interactive)
            break;
        host->alert(host->context, message);
        if (from == FROM_SAVED) {
            prefs->disc[0] = '\0';
            host->save(host->context, prefs);
        }
        cue = NULL;
    }
    if (status_out)
        *status_out = status;
    return NULL;
}
