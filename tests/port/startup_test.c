/* Headless checks of the startup host services, without game data:
 *   - SYSTEM.CNF parsing and the release table (disc_check.c);
 *   - the identity of synthetic CUE/BIN images read through PsyZ: the
 *     European release, the USA release, another game, an ISO without
 *     SYSTEM.CNF, a missing BIN, and that checking does not disturb the
 *     mounted disc;
 *   - the choice of disc at startup (Rr4DiscSelect) with images that fail
 *     the check or the mount, saved and chosen paths;
 *   - the memory card directory with its migration from the working
 *     directory (memory_cards.c);
 *   - the log file (host_log.c);
 *   - the preferences file rules: one file by precedence, a UTF-8 byte
 *     order mark, overlong lines.
 * The work directory is argv[1] (created); usa/usa.cue is left there for
 * the startup-other-release test. Run with PSYZ_HEADLESS=1
 * SDL_AUDIODRIVER=dummy. */

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#include <SDL3/SDL.h>
#include <psyz.h>
#include <psyz/cd.h>
#include <psyz/file.h>
#include <psyz/system.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_check.h"
#include "host_log.h"
#include "memory_cards.h"
#include "preferences.h"

static int failures;

#define CHECK(cond)                                                                  \
    do {                                                                             \
        if (!(cond)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                              \
        }                                                                            \
    } while (0)

static char s_root[4096]; /* the work directory, absolute, with a separator */

/* ---- SYSTEM.CNF and the release table ---- */

static int Boot(const char *text, char *boot) {
    return Rr4DiscParseSystemCnf(text, strlen(text), boot, 64);
}

static void TestSystemCnf(void) {
    char boot[64], message[RR4_DISC_MESSAGE_MAX];

    CHECK(Boot("BOOT = cdrom:\\SCES_017.06;1\r\nTCB = 4\r\nEVENT = 16\r\n", boot) && !strcmp(boot, "SCES_017.06"));
    CHECK(Boot("  boot=cdrom:\\scus_944.03;1\n", boot) && !strcmp(boot, "SCUS_944.03"));
    CHECK(Boot("TCB = 4\nBOOT\t=\tcdrom:\\EXE\\SLPS_018.00", boot) && !strcmp(boot, "SLPS_018.00"));
    CHECK(Boot("BOOT = cdrom:SLUS_123.45;1", boot) && !strcmp(boot, "SLUS_123.45"));
    CHECK(!Boot("BOOT2 = cdrom0:\\SLUS_200.00;1\n", boot) && !boot[0]); /* PlayStation 2 */
    CHECK(!Boot("TCB = 4\nSTACK = 801fff00\n", boot));
    CHECK(!Boot("", boot));
    CHECK(!Boot("BOOT = \n", boot));
    {
        /* the length counts, not a terminator */
        const char text[] = "BOOT = cdrom:\\SCES_017.06;1";
        CHECK(Rr4DiscParseSystemCnf(text, 6, boot, sizeof(boot)) == 0);
    }

    CHECK(Rr4DiscClassify("SCES_017.06", message, sizeof(message)) == RR4_DISC_OK && !message[0]);
    CHECK(Rr4DiscClassify("SCUS_944.03", message, sizeof(message)) == RR4_DISC_OTHER_RELEASE);
    CHECK(strstr(message, "This is the USA release") && strstr(message, "SCUS-94403") &&
          strstr(message, "this port needs the European PAL disc SCES-01706 or the Japanese disc SLPS-01800"));
    CHECK(Rr4DiscClassify("SLPS_018.00", message, sizeof(message)) == RR4_DISC_OK && !message[0]);
    CHECK(Rr4DiscClassify("SLUS_007.77", message, sizeof(message)) == RR4_DISC_OTHER_GAME);
    CHECK(strstr(message, "not Ridge Racer Type 4") && strstr(message, "SLUS-00777"));
    CHECK(Rr4DiscClassify(NULL, message, sizeof(message)) == RR4_DISC_NOT_PLAYSTATION);
    CHECK(Rr4DiscClassify("", message, sizeof(message)) == RR4_DISC_NOT_PLAYSTATION);
    CHECK(Rr4DiscFindRelease("SCES_017.06") && Rr4DiscFindRelease("SCES_017.06")->supported);
    CHECK(Rr4DiscFindRelease("SCUS_944.03") && !Rr4DiscFindRelease("SCUS_944.03")->supported);
    CHECK(!Rr4DiscFindRelease("SCES_017.07"));
}

/* ---- synthetic images ---- */

enum { SECTORS = 24, SECTOR = RR4_DISC_SECTOR };

static void Put32(unsigned char *p, unsigned v) {
    for (int i = 0; i < 4; i++) {
        p[i] = (unsigned char)(v >> (8 * i));      /* little endian */
        p[7 - i] = (unsigned char)(v >> (8 * i));  /* then big endian */
    }
}

/* A directory record at p; returns its length. */
static int Record(unsigned char *p, const char *name, unsigned lba, unsigned size, int directory) {
    size_t n = name[0] ? strlen(name) : 1; /* "\0" and "\1": the directory itself and its parent */
    int length = (int)(33 + n + (n % 2 == 0));
    memset(p, 0, (size_t)length);
    p[0] = (unsigned char)length;
    Put32(p + 2, lba);
    Put32(p + 10, size);
    p[25] = directory ? 2 : 0;
    p[32] = (unsigned char)n;
    memcpy(p + 33, name, n);
    return length;
}

/* dir/name.cue and its BIN: a Mode 2 (or Mode 1) ISO 9660 image whose
 * root holds SYSTEM.CNF with cnf (none when NULL), or no volume at all
 * (iso 0). Returns the CUE path. */
static const char *WriteImage(const char *dir, const char *name, const char *cnf, int iso, int mode) {
    static unsigned char image[SECTORS][SECTOR];
    static char cue[4096];
    char path[4096];
    int user = mode == 1 ? 16 : 24;
    FILE *f;

    memset(image, 0, sizeof(image));
    for (int i = 0; i < SECTORS; i++) {
        memset(image[i] + 1, 0xFF, 10); /* sync */
        image[i][15] = (unsigned char)mode;
    }
    if (iso) {
        unsigned char *pvd = image[16] + user, *root = image[18] + user;
        int at = 0;
        pvd[0] = 1;
        memcpy(pvd + 1, "CD001", 5);
        pvd[6] = 1;
        Record(pvd + 156, "\0", 18, 2048, 1);
        image[17][user] = 255;
        memcpy(image[17] + user + 1, "CD001", 5);
        at += Record(root + at, "\0", 18, 2048, 1);
        at += Record(root + at, "\001", 18, 2048, 1);
        at += Record(root + at, "R4.BIN;1", 20, 2048, 0);
        if (cnf) {
            at += Record(root + at, "SYSTEM.CNF;1", 19, (unsigned)strlen(cnf), 0);
            memcpy(image[19] + user, cnf, strlen(cnf));
        }
    }
    SDL_CreateDirectory(dir);
    snprintf(path, sizeof(path), "%s/%s.bin", dir, name);
    f = Psyz_FileOpen(path, "wb");
    CHECK(f != NULL);
    if (f) {
        CHECK(fwrite(image, 1, sizeof(image), f) == sizeof(image));
        fclose(f);
    }
    snprintf(cue, sizeof(cue), "%s/%s.cue", dir, name);
    f = Psyz_FileOpen(cue, "wb");
    CHECK(f != NULL);
    if (f) {
        fprintf(f, "FILE \"%s.bin\" BINARY\r\n  TRACK 01 MODE%d/2352\r\n    INDEX 01 00:00:00\r\n", name, mode);
        fclose(f);
    }
    return cue;
}

static void TestImages(void) {
    char message[RR4_DISC_MESSAGE_MAX], dir[4096], eu[4096], path[4096];
    unsigned char sector[SECTOR];

    snprintf(dir, sizeof(dir), "%seu", s_root);
    snprintf(eu, sizeof(eu), "%s", WriteImage(dir, "Ridge Racer Type 4 (Europe)",
                                               "BOOT = cdrom:\\SCES_017.06;1\r\nTCB = 4\r\n", 1, 2));
    CHECK(Rr4DiscCheckImage(eu, message, sizeof(message)) == RR4_DISC_OK && !message[0]);

    snprintf(dir, sizeof(dir), "%susa", s_root);
    snprintf(path, sizeof(path), "%s", WriteImage(dir, "usa", "BOOT = cdrom:\\SCUS_944.03;1\r\n", 1, 2));
    CHECK(Rr4DiscCheckImage(path, message, sizeof(message)) == RR4_DISC_OTHER_RELEASE);
    CHECK(strstr(message, "USA release") != NULL);

    snprintf(dir, sizeof(dir), "%sother", s_root);
    snprintf(path, sizeof(path), "%s", WriteImage(dir, "other", "BOOT=cdrom:\\SLUS_001.23;1\n", 1, 1));
    CHECK(Rr4DiscCheckImage(path, message, sizeof(message)) == RR4_DISC_OTHER_GAME); /* Mode 1 too */

    snprintf(dir, sizeof(dir), "%snocnf", s_root);
    snprintf(path, sizeof(path), "%s", WriteImage(dir, "nocnf", NULL, 1, 2));
    CHECK(Rr4DiscCheckImage(path, message, sizeof(message)) == RR4_DISC_NOT_PLAYSTATION);

    snprintf(dir, sizeof(dir), "%snoiso", s_root);
    snprintf(path, sizeof(path), "%s", WriteImage(dir, "noiso", NULL, 0, 2));
    CHECK(Rr4DiscCheckImage(path, message, sizeof(message)) == RR4_DISC_NOT_PLAYSTATION);
    CHECK(strstr(message, "not a PlayStation game disc") != NULL);

    /* A CUE without its BIN, and no CUE at all. */
    snprintf(dir, sizeof(dir), "%snobin", s_root);
    snprintf(path, sizeof(path), "%s", WriteImage(dir, "nobin", "BOOT = cdrom:\\SCES_017.06;1\r\n", 1, 2));
    {
        char bin[4096];
        snprintf(bin, sizeof(bin), "%s/nobin.bin", dir);
        CHECK(SDL_RemovePath(bin));
    }
    CHECK(Rr4DiscCheckImage(path, message, sizeof(message)) == RR4_DISC_UNREADABLE);
    CHECK(strstr(message, "Cannot open the disc image") && strstr(message, "nobin.cue"));
    snprintf(path, sizeof(path), "%smissing/missing.cue", s_root);
    CHECK(Rr4DiscCheckImage(path, message, sizeof(message)) == RR4_DISC_UNREADABLE);
    CHECK(Rr4DiscCheckImage(NULL, message, sizeof(message)) == RR4_DISC_UNREADABLE);

    /* Checking another image leaves the mounted one in the drive. */
    CHECK(Psyz_CdSetDiskPath(eu) == 0);
    snprintf(path, sizeof(path), "%susa/usa.cue", s_root);
    CHECK(Psyz_CdReadImageSector(path, 19, sector) == 0);
    CHECK(!memcmp(sector + 24, "BOOT = cdrom:\\SCUS", 18));
    CHECK(Psyz_CdReadImageSector(path, SECTORS + 100, sector) != 0); /* past the end */
    CHECK(Psyz_CdReadImageSector(path, -1, sector) != 0);
    CHECK(Psyz_CdReadImageSector(eu, 19, sector) == 0);
    CHECK(!memcmp(sector + 24, "BOOT = cdrom:\\SCES", 18));
    Psyz_CdSetDiskPath(NULL);
}

/* ---- the disc at startup ---- */

typedef struct FakeHost {
    const char *bad_check; /* paths that fail the check (substring) */
    const char *bad_mount; /* paths that fail to mount */
    const char *answers[4]; /* the dialog's answers in turn (NULL: cancel) */
    int asked, alerts, mounts, saves;
    char saved[RR4_PREFS_PATH_MAX];
    char last_alert[RR4_DISC_MESSAGE_MAX];
} FakeHost;

static int FakeChoose(void *context, char *out, size_t size) {
    FakeHost *h = context;
    const char *answer = h->asked < 4 ? h->answers[h->asked] : NULL;
    h->asked++;
    if (!answer)
        return 0;
    snprintf(out, size, "%s", answer);
    return 1;
}

static void FakeAlert(void *context, const char *message) {
    FakeHost *h = context;
    h->alerts++;
    snprintf(h->last_alert, sizeof(h->last_alert), "%s", message);
}

static Rr4DiscStatus FakeCheck(void *context, const char *cue, char *message, size_t size) {
    FakeHost *h = context;
    if (h->bad_check && strstr(cue, h->bad_check))
        return Rr4DiscClassify("SCUS_944.03", message, size);
    return Rr4DiscClassify("SCES_017.06", message, size);
}

static int FakeMount(void *context, const char *cue) {
    FakeHost *h = context;
    h->mounts++;
    return h->bad_mount && strstr(cue, h->bad_mount) ? -1 : 0;
}

static void FakeSave(void *context, const Rr4Prefs *prefs) {
    FakeHost *h = context;
    h->saves++;
    snprintf(h->saved, sizeof(h->saved), "%s", prefs->disc);
}

static const char *Select(FakeHost *fake, Rr4Prefs *prefs, const char *requested, int interactive,
                          Rr4DiscStatus *status, char *message) {
    Rr4DiscHost host = {fake, FakeChoose, FakeAlert, FakeCheck, FakeMount, FakeSave};
    return Rr4DiscSelect(&host, prefs, requested, interactive, status, message, RR4_DISC_MESSAGE_MAX);
}

static void TestSelect(void) {
    static Rr4Prefs prefs;
    char message[RR4_DISC_MESSAGE_MAX];
    Rr4DiscStatus status;
    const char *cue;

    /* Headless: the requested image, never the dialog or the preferences. */
    {
        FakeHost h = {0};
        prefs = *Rr4PrefsDefaults();
        snprintf(prefs.disc, sizeof(prefs.disc), "/saved/rr4.cue");
        cue = Select(&h, &prefs, "/arg/rr4.cue", 0, &status, message);
        CHECK(cue && !strcmp(cue, "/arg/rr4.cue") && status == RR4_DISC_OK);
        CHECK(h.mounts == 1 && !h.asked && !h.alerts && !h.saves);
        CHECK(!strcmp(prefs.disc, "/saved/rr4.cue"));
    }
    {
        FakeHost h = {.bad_check = "usa"};
        prefs = *Rr4PrefsDefaults();
        cue = Select(&h, &prefs, "/arg/usa.cue", 0, &status, message);
        CHECK(!cue && status == RR4_DISC_OTHER_RELEASE && strstr(message, "USA release"));
        CHECK(!h.mounts && !h.asked && !h.alerts && !h.saves);
    }
    {
        FakeHost h = {.bad_mount = "broken"};
        prefs = *Rr4PrefsDefaults();
        cue = Select(&h, &prefs, "/arg/broken.cue", 0, &status, message);
        CHECK(!cue && status == RR4_DISC_UNREADABLE && strstr(message, "broken.cue"));
    }
    {
        FakeHost h = {0};
        prefs = *Rr4PrefsDefaults();
        cue = Select(&h, &prefs, NULL, 0, &status, message);
        CHECK(!cue && status == RR4_DISC_OK && !message[0] && !h.asked);
    }

    /* Interactive: a saved path that no longer mounts is explained,
     * forgotten and replaced by the chosen image once that mounts. */
    {
        FakeHost h = {.bad_mount = "moved", .answers = {"/new/rr4.cue"}};
        prefs = *Rr4PrefsDefaults();
        snprintf(prefs.disc, sizeof(prefs.disc), "/moved/rr4.cue");
        cue = Select(&h, &prefs, NULL, 1, &status, message);
        CHECK(cue && !strcmp(cue, "/new/rr4.cue") && status == RR4_DISC_OK);
        CHECK(h.alerts == 1 && strstr(h.last_alert, "/moved/rr4.cue"));
        CHECK(h.asked == 1 && h.mounts == 2);
        CHECK(h.saves == 2 && !strcmp(h.saved, "/new/rr4.cue")); /* cleared, then the new one */
        CHECK(!strcmp(prefs.disc, "/new/rr4.cue"));
    }
    /* A saved wrong release, then the dialog cancelled: nothing saved. */
    {
        FakeHost h = {.bad_check = "usa"};
        prefs = *Rr4PrefsDefaults();
        snprintf(prefs.disc, sizeof(prefs.disc), "/discs/usa.cue");
        cue = Select(&h, &prefs, NULL, 1, &status, message);
        CHECK(!cue && status == RR4_DISC_OK && !message[0]);
        CHECK(h.alerts == 1 && strstr(h.last_alert, "SCES-01706"));
        CHECK(h.asked == 1 && !h.mounts && h.saves == 1 && !prefs.disc[0]);
    }
    /* A wrong chosen image is never saved; the dialog asks again. */
    {
        FakeHost h = {.bad_check = "usa", .answers = {"/discs/usa.cue", "/discs/eu.cue"}};
        prefs = *Rr4PrefsDefaults();
        cue = Select(&h, &prefs, NULL, 1, &status, message);
        CHECK(cue && !strcmp(cue, "/discs/eu.cue"));
        CHECK(h.asked == 2 && h.alerts == 1 && h.mounts == 1 && h.saves == 1);
        CHECK(!strcmp(h.saved, "/discs/eu.cue"));
    }
    /* A bad command-line image: explained, then the dialog; the saved path
     * is left alone. */
    {
        FakeHost h = {.bad_check = "usa", .answers = {NULL}};
        prefs = *Rr4PrefsDefaults();
        snprintf(prefs.disc, sizeof(prefs.disc), "/saved/eu.cue");
        cue = Select(&h, &prefs, "/arg/usa.cue", 1, &status, message);
        CHECK(!cue && h.alerts == 1 && h.asked == 1 && !h.saves);
        CHECK(!strcmp(prefs.disc, "/saved/eu.cue"));
    }
    /* A good command-line image is not saved. */
    {
        FakeHost h = {0};
        prefs = *Rr4PrefsDefaults();
        cue = Select(&h, &prefs, "/arg/eu.cue", 1, &status, message);
        CHECK(cue && !strcmp(cue, "/arg/eu.cue") && !h.saves && !prefs.disc[0]);
    }
}

/* ---- memory cards ---- */

static int ReadText(const char *path, char *out, size_t size) {
    FILE *f = Psyz_FileOpen(path, "rb");
    size_t n;
    if (!f)
        return 0;
    n = fread(out, 1, size - 1, f);
    out[n] = '\0';
    fclose(f);
    /* text files (the log) end their lines in CR LF on Windows */
    for (char *from = out, *to = out;; from++) {
        if (*from != '\r' && !(*to++ = *from))
            break;
    }
    return 1;
}

static void WriteText(const char *path, const char *text) {
    FILE *f = Psyz_FileOpen(path, "wb");
    CHECK(f != NULL);
    if (f) {
        fputs(text, f);
        fclose(f);
    }
}

static void TestCards(void) {
    char data[4096], path[4096], text[256], expect[4096];

    /* An earlier version's card in the working directory (s_root). */
    SDL_CreateDirectory("bu00");
    WriteText("bu00/BESCES-01706RR4", "old save");
    snprintf(data, sizeof(data), "%sdata", s_root);
    CHECK(Rr4CardsUseDirectory(data));
    snprintf(path, sizeof(path), "%s/bu00/BESCES-01706RR4", data);
    CHECK(ReadText(path, text, sizeof(text)) && !strcmp(text, "old save"));
    CHECK(ReadText("bu00/BESCES-01706RR4", text, sizeof(text))); /* the original stays */
    snprintf(path, sizeof(path), "%s/bu10", data);
    {
        SDL_PathInfo info;
        CHECK(SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY);
    }
    /* PsyZ's card paths now lead there. */
    Psyz_AdjustPath(path, "bu00:BESCES-01706RR4", sizeof(path));
#ifdef _WIN32
    snprintf(expect, sizeof(expect), "%s\\bu00\\BESCES-01706RR4", data);
#else
    snprintf(expect, sizeof(expect), "%s/bu00/BESCES-01706RR4", data);
#endif
    CHECK(!strcmp(path, expect));
    Psyz_AdjustPath(path, "bu10:*", sizeof(path));
    CHECK(strstr(path, "data") && strstr(path, "bu10") && (path[strlen(path) - 1] == '/' || path[strlen(path) - 1] == '\\'));
    Psyz_AdjustPath(path, "cdrom:\\R4.BIN;1", sizeof(path));
    CHECK(!strcmp(path, "cdrom:\\R4.BIN;1"));
    /* A card already in the data directory is never replaced. */
    WriteText("bu00/BESCES-01706RR4", "older save");
    CHECK(Rr4CardsUseDirectory(data));
    snprintf(path, sizeof(path), "%s/bu00/BESCES-01706RR4", data);
    CHECK(ReadText(path, text, sizeof(text)) && !strcmp(text, "old save"));
    /* The data directory as the working directory: nothing to copy. */
    {
        char trailing[4096];
        snprintf(trailing, sizeof(trailing), "%s/", data);
        CHECK(Rr4CardsUseDirectory(trailing));
        Psyz_AdjustPath(path, "bu00:X", sizeof(path));
        CHECK(strstr(path, "data") && !strstr(path, "//"));
    }
    Rr4CardsUseWorkingDirectory();
    Psyz_AdjustPath(path, "bu00:X", sizeof(path));
#ifdef _WIN32
    CHECK(!strcmp(path, "bu00\\X"));
#else
    CHECK(!strcmp(path, "bu00/X"));
#endif
    CHECK(!Rr4CardsUseDirectory(""));
}

/* ---- the log ---- */

static void TestLog(void) {
    char path[4096], text[1024];

    snprintf(path, sizeof(path), "%slog/ridge-racer-type-4.log", s_root);
    {
        char dir[4096];
        snprintf(dir, sizeof(dir), "%slog", s_root);
        SDL_CreateDirectory(dir);
    }
    WriteText(path, "an earlier run\n");
    CHECK(Rr4LogOpen(path));
    CHECK(!strcmp(Rr4LogPath(), path));
    Rr4Log("port message %d\n", 42);
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "psyz %s", "error");
    CHECK(ReadText(path, text, sizeof(text)));
    CHECK(!strstr(text, "earlier run")); /* truncated at each launch */
    CHECK(strstr(text, "port message 42\n") && strstr(text, "error: psyz error\n"));
    Rr4LogClose();
    Rr4Log("after closing\n");
    CHECK(ReadText(path, text, sizeof(text)) && !strstr(text, "after closing"));
    CHECK(!Rr4LogOpen(""));
}

/* ---- preferences file rules ---- */

static void TestPrefsRules(void) {
    char path[4096], home[4096], line[6000];
    static Rr4Prefs p;
    FILE *f;

    /* A UTF-8 byte order mark does not hide the first section. */
    snprintf(path, sizeof(path), "%sbom.toml", s_root);
    WriteText(path, "\xEF\xBB\xBF[graphics]\ninternal_resolution = 4\n");
    p = *Rr4PrefsDefaults();
    CHECK(Rr4PrefsLoadFile(&p, path) && p.scale == 4);

    /* An overlong line is one invalid line, not several. */
    snprintf(path, sizeof(path), "%slong.toml", s_root);
    f = Psyz_FileOpen(path, "wb");
    CHECK(f != NULL);
    if (f) {
        memset(line, 'x', sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';
        memcpy(line, "image = \"", 9);
        fprintf(f, "[audio]\nvolume = 40\n[disc]\n%s\n[audio]\nmute = true\n", line);
        fclose(f);
    }
    p = *Rr4PrefsDefaults();
    CHECK(Rr4PrefsLoadFile(&p, path));
    CHECK(p.volume == 40 && p.mute == 1 && !p.disc[0]);

    /* One file: with RR4_CONFIG_HOME a stray file in the working directory
     * (s_root) is not merged in. */
    WriteText("ridge-racer-type-4.toml", "[audio]\nvolume = 10\n");
    snprintf(home, sizeof(home), "%shome", s_root);
    SDL_setenv_unsafe("RR4_CONFIG_HOME", home, 1);
    snprintf(path, sizeof(path), "%s/ridge-racer-type-4.toml", home);
    SDL_RemovePath(path);
    p = *Rr4PrefsDefaults();
    CHECK(Rr4PrefsLoad(&p));
    CHECK(p.volume == 100);
    p.scale = 2;
    CHECK(Rr4PrefsSave(&p));
    p = *Rr4PrefsDefaults();
    CHECK(Rr4PrefsLoad(&p) && p.scale == 2 && p.volume == 100);
    {
        char dir[4096];
        CHECK(Rr4PrefsDirectory(dir, sizeof(dir)));
        CHECK(!strncmp(dir, home, strlen(home)) && dir[strlen(home)] == '/' && !dir[strlen(home) + 1]);
    }
    /* Without it the working directory's portable file is the one. */
    SDL_unsetenv_unsafe("RR4_CONFIG_HOME");
    p = *Rr4PrefsDefaults();
    CHECK(Rr4PrefsLoad(&p) && p.volume == 10 && p.scale == 1);
    CHECK(strstr(Rr4PrefsPath(), "ridge-racer-type-4.toml") != NULL);
    SDL_RemovePath("ridge-racer-type-4.toml");
    /* A preferences directory that cannot be created is no location. */
    snprintf(home, sizeof(home), "%sbom.toml/below", s_root); /* under a file */
    SDL_setenv_unsafe("RR4_CONFIG_HOME", home, 1);
    CHECK(!Rr4PrefsResolve());
    SDL_unsetenv_unsafe("RR4_CONFIG_HOME");
}

int main(int argc, char **argv) {
    char *cwd;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <work directory>\n", argv[0]);
        return 2;
    }
    SDL_CreateDirectory(argv[1]);
    if (chdir(argv[1]) != 0) {
        fprintf(stderr, "cannot enter %s\n", argv[1]);
        return 1;
    }
    cwd = SDL_GetCurrentDirectory(); /* with a trailing separator */
    snprintf(s_root, sizeof(s_root), "%s", cwd ? cwd : "");
    SDL_free(cwd);
    /* Leftovers of an earlier run. */
    SDL_RemovePath("data/bu00/BESCES-01706RR4");
    SDL_RemovePath("bu00/BESCES-01706RR4");

    Rr4PrefsCaptureDefaults();
    TestSystemCnf();
    TestImages();
    TestSelect();
    TestCards();
    TestLog();
    TestPrefsRules();
    if (failures) {
        fprintf(stderr, "startup_test: %d failures\n", failures);
        return 1;
    }
    printf("startup_test: ok\n");
    return 0;
}
