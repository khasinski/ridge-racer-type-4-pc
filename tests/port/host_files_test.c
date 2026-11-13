/* Headless checks of the host files a player's machine gives the port, under
 * a directory whose UTF-8 name is not ASCII (as a Windows profile directory
 * such as "C:\Users\Zoë" or a localised Documents folder gives it):
 *   - a memory card save through the game's BIOS file services
 *     (src/main/psyq/libapi/file.c): create, write, reopen, seek, read back,
 *     list with firstfile and erase, with bytes a text-mode stream would
 *     translate; formatting a card;
 *   - the preferences file round trip;
 *   - mounting a small synthetic CUE/BIN disc image.
 * No game data is needed. The working directory is argv[1] (created). Run
 * with PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy. */

#include <SDL3/SDL.h>
#include <psyz.h>
#include <psyz/cd.h>
#include <psyz/file.h>
#include <psyz/system.h>
#include <libapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "preferences.h"

/* The game's BIOS file services (src/main/psyq/libapi/file.c; declared in
 * rr/bios_events.h, whose event prototypes differ from the PsyZ kernel.h
 * this test needs for firstfile and erase). */
int FileOpen(const char *name, int mode);
int FileSeek(int descriptor, int offset, int whence);
int FileRead(int descriptor, void *buffer, int count);
int FileWrite(int descriptor, const void *buffer, int count);
int FileClose(int descriptor);

/* BIOS open modes (romio.h; the host <fcntl.h> reuses some of the names). */
enum { BIOS_FREAD = 1, BIOS_FWRITE = 2, BIOS_FCREAT = 0x200 };
enum { CARD_BLOCK = 8192, SECTOR = 2352 };

/* "Zoë 日本": e diaeresis plus two CJK ideographs, outside every Windows
 * ANSI code page that has the e. */
#define UTF8_NAME "Zo\xc3\xab \xe6\x97\xa5\xe6\x9c\xac"
#define CARD_FILE "BESCES-01234RR4TEST"

static int failures;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                      \
        }                                                                    \
    } while (0)

static int EnterDirectory(const char *path) {
#ifdef _WIN32
    wchar_t *wide = (wchar_t *)SDL_iconv_string("UTF-16LE", "UTF-8", path, strlen(path) + 1);
    int ok = wide && _wchdir(wide) == 0;
    SDL_free(wide);
    return ok;
#else
    return chdir(path) == 0;
#endif
}

static void TestCardSave(void) {
    static unsigned char block[CARD_BLOCK], back[CARD_BLOCK];
    const char *name = "bu00:" CARD_FILE;
    struct DIRENTRY entry;
    int fd;

    for (int i = 0; i < CARD_BLOCK; i++)
        block[i] = (unsigned char)(i * 13 + (i >> 8));
    block[64] = '\n'; /* text-mode writes expand LF, reads stop at ^Z */
    block[65] = '\r';
    block[66] = 0x1A;

    fd = FileOpen(name, BIOS_FCREAT | (1 << 16)); /* one block, as the game */
    CHECK(fd >= 0);
    if (fd >= 0)
        CHECK(FileClose(fd) == fd);
    fd = FileOpen(name, BIOS_FWRITE);
    CHECK(fd >= 0);
    if (fd >= 0) {
        CHECK(FileWrite(fd, block, CARD_BLOCK) == CARD_BLOCK);
        CHECK(FileClose(fd) == fd);
    }

    memset(&entry, 0, sizeof(entry));
    CHECK(firstfile((char *)"bu00:", &entry) == &entry);
    CHECK(!strcmp(entry.name, CARD_FILE));
    CHECK(entry.size == CARD_BLOCK);

    fd = FileOpen(name, BIOS_FREAD);
    CHECK(fd >= 0);
    if (fd >= 0) {
        CHECK(FileRead(fd, back, CARD_BLOCK) == CARD_BLOCK);
        CHECK(!memcmp(block, back, CARD_BLOCK));
        CHECK(FileSeek(fd, 0x200, SEEK_SET) == 0x200);
        CHECK(FileRead(fd, back, 0x80) == 0x80);
        CHECK(!memcmp(block + 0x200, back, 0x80));
        CHECK(FileSeek(fd, -0x100, SEEK_CUR) == 0x180);
        CHECK(FileClose(fd) == fd);
    }

    CHECK(erase((char *)name));
    CHECK(FileOpen(name, BIOS_FREAD) < 0);
}

/* Formatting a card (the game's _card_format) empties its directory on
 * every host, Windows included. */
static void TestCardFormat(void) {
    static unsigned char block[128];
    struct DIRENTRY entry;
    const char *names[] = {"bu00:" CARD_FILE, "bu00:BESCES-01706RR4"};

    for (int i = 0; i < 2; i++) {
        int fd = FileOpen(names[i], BIOS_FCREAT | (1 << 16));
        CHECK(fd >= 0);
        if (fd >= 0)
            CHECK(FileClose(fd) == fd);
        fd = FileOpen(names[i], BIOS_FWRITE);
        CHECK(fd >= 0);
        if (fd >= 0) {
            CHECK(FileWrite(fd, block, sizeof(block)) == (int)sizeof(block));
            CHECK(FileClose(fd) == fd);
        }
    }
    CHECK(firstfile((char *)"bu00:", &entry) == &entry);
    CHECK(format((char *)"bu00:") == 1);
    CHECK(firstfile((char *)"bu00:", &entry) == NULL);
    CHECK(FileOpen(names[0], BIOS_FREAD) < 0 && FileOpen(names[1], BIOS_FREAD) < 0);
    /* the card itself (its directory) stays */
    CHECK(format((char *)"bu00:") == 1);
}

static void TestPreferences(void) {
    static Rr4Prefs saved, loaded;
    const char *path = UTF8_NAME ".toml";

    saved = *Rr4PrefsDefaults();
    saved.scale = 4;
    saved.volume = 35;
    snprintf(saved.disc, sizeof(saved.disc), "C:/Gry/" UTF8_NAME "/rr4.cue");
    CHECK(Rr4PrefsSaveFile(&saved, path));
    loaded = *Rr4PrefsDefaults();
    CHECK(Rr4PrefsLoadFile(&loaded, path));
    CHECK(loaded.scale == 4 && loaded.volume == 35);
    CHECK(!strcmp(loaded.disc, saved.disc));
    SDL_RemovePath(path);
}

static void TestDiscImage(void) {
    static unsigned char sector[SECTOR];
    FILE *f = Psyz_FileOpen(UTF8_NAME ".bin", "wb");

    CHECK(f != NULL);
    if (!f)
        return;
    for (int i = 0; i < 16; i++)
        CHECK(fwrite(sector, 1, sizeof(sector), f) == sizeof(sector));
    fclose(f);
    f = Psyz_FileOpen(UTF8_NAME ".cue", "wb");
    CHECK(f != NULL);
    if (!f)
        return;
    fputs("FILE \"" UTF8_NAME ".bin\" BINARY\r\n"
          "  TRACK 01 MODE2/2352\r\n"
          "    INDEX 01 00:00:00\r\n",
          f);
    fclose(f);
    {
        const char *cwd = SDL_GetCurrentDirectory();
        char cue[4096];
        /* An absolute path, as the file dialog and the preferences give it. */
        snprintf(cue, sizeof(cue), "%s%s", cwd ? cwd : "", UTF8_NAME ".cue");
        CHECK(Psyz_CdSetDiskPath(cue) == 0);
        Psyz_CdSetDiskPath(NULL);
        SDL_free((void *)cwd);
    }
    SDL_RemovePath(UTF8_NAME ".cue");
    SDL_RemovePath(UTF8_NAME ".bin");
}

int main(int argc, char **argv) {
    char dir[4096];

    if (argc < 2) {
        fprintf(stderr, "usage: %s <work directory>\n", argv[0]);
        return 2;
    }
    SDL_CreateDirectory(argv[1]);
    snprintf(dir, sizeof(dir), "%s/" UTF8_NAME, argv[1]);
    SDL_RemovePath(dir); /* only when a previous run left it empty */
    CHECK(SDL_CreateDirectory(dir));
    if (!EnterDirectory(dir)) {
        fprintf(stderr, "cannot enter %s\n", dir);
        return 1;
    }
    Psyz_CardSetInstantTiming(1);

    TestCardSave();
    TestCardFormat();
    TestPreferences();
    TestDiscImage();

    SDL_RemovePath("bu00");
    SDL_RemovePath("bu10");
    if (failures) {
        fprintf(stderr, "%d host file check(s) failed\n", failures);
        return 1;
    }
    printf("host files: all checks passed\n");
    return 0;
}
