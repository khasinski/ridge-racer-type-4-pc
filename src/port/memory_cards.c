#include "memory_cards.h"

#include <SDL3/SDL.h>
#include <psyz.h>
#include <psyz/system.h>

#include <stdio.h>
#include <string.h>

#include "host_log.h"

#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

static char s_directory[4096]; /* with a trailing separator, or "" */

static int IsSeparator(char c) { return c == '/' || c == '\\'; }

void Rr4CardsPath(const char *slot, char *out, size_t size) {
    snprintf(out, size, "%s%.4s" SEPARATOR, s_directory, slot);
}

/* Psyz_AdjustPathCB: "bu00:NAME" -> <directory>bu00/NAME, creating the
 * card directory as PsyZ's default mapping does. */
static int AdjustPath(char *dst, const char *src, int maxlen) {
    char card[4096];
    const char *name;
    int n;

    if (!s_directory[0] || !src || strlen(src) < 5 || src[0] != 'b' || src[1] != 'u' || src[4] != ':')
        return -1;
    name = src + 5;
    if (*name == '*')
        name = "";
    Rr4CardsPath(src, card, sizeof(card));
    SDL_CreateDirectory(card);
    n = snprintf(dst, (size_t)maxlen, "%s%s", card, name);
    return n < 0 ? -1 : (int)strlen(dst);
}

/* Regular files in a directory (count only). */
static int CountFiles(const char *directory) {
    int count = 0, files = 0;
    char **names = SDL_GlobDirectory(directory, NULL, 0, &count);

    for (int i = 0; names && i < count; i++) {
        char path[4096];
        SDL_PathInfo info;
        snprintf(path, sizeof(path), "%s%s", directory, names[i]);
        if (SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_FILE)
            files++;
    }
    SDL_free(names);
    return files;
}

/* Cards from the working directory (earlier versions) into an empty slot. */
static void Migrate(const char *slot) {
    char old_dir[4096], new_dir[4096];
    char **names;
    int count = 0, copied = 0, failed = 0;

    snprintf(old_dir, sizeof(old_dir), "%s" SEPARATOR, slot);
    Rr4CardsPath(slot, new_dir, sizeof(new_dir));
    /* Also when both are the same directory: the new one is not empty. */
    if (!CountFiles(old_dir) || CountFiles(new_dir))
        return;
    names = SDL_GlobDirectory(old_dir, NULL, 0, &count);
    for (int i = 0; names && i < count; i++) {
        char from[4096], to[4096];
        SDL_PathInfo info;
        snprintf(from, sizeof(from), "%s%s", old_dir, names[i]);
        snprintf(to, sizeof(to), "%s%s", new_dir, names[i]);
        if (!SDL_GetPathInfo(from, &info) || info.type != SDL_PATHTYPE_FILE)
            continue;
        if (SDL_CopyFile(from, to))
            copied++;
        else
            failed++;
    }
    SDL_free(names);
    {
        char *cwd = SDL_GetCurrentDirectory();
        Rr4Log("ridge-racer-type-4-pc: memory card %s: copied %d file(s) from %s%s to %s%s\n", slot, copied,
               cwd ? cwd : "", old_dir, new_dir, failed ? " (some could not be copied)" : "; the originals stay");
        SDL_free(cwd);
    }
}

int Rr4CardsUseDirectory(const char *directory) {
    static const char *const kSlots[] = {"bu00", "bu10"};
    size_t n;

    Rr4CardsUseWorkingDirectory();
    if (!directory || !*directory)
        return 0;
    n = strlen(directory);
    if (n + 8 >= sizeof(s_directory))
        return 0;
    snprintf(s_directory, sizeof(s_directory), "%s%s", directory, IsSeparator(directory[n - 1]) ? "" : SEPARATOR);
    for (int i = 0; i < 2; i++) {
        char card[4096];
        Rr4CardsPath(kSlots[i], card, sizeof(card));
        if (!SDL_CreateDirectory(card)) {
            Rr4Log("ridge-racer-type-4-pc: cannot create the memory card directory %s (%s); "
                   "the cards stay in the working directory\n",
                   card, SDL_GetError());
            Rr4CardsUseWorkingDirectory();
            return 0;
        }
    }
    for (int i = 0; i < 2; i++)
        Migrate(kSlots[i]);
    Psyz_AdjustPathCB(AdjustPath);
    return 1;
}

void Rr4CardsUseWorkingDirectory(void) {
    s_directory[0] = '\0';
    Psyz_AdjustPathCB(NULL);
}
