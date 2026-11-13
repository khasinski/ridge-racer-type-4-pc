#include "host_log.h"

#include <SDL3/SDL.h>
#include <psyz/file.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static FILE *s_file;
static char s_path[4096];
static SDL_LogOutputFunction s_sdl_output;
static void *s_sdl_userdata;

static void WriteFile(const char *text, size_t length) {
    if (!s_file)
        return;
    fwrite(text, 1, length, s_file);
    /* Every line reaches the disk at once: the log is read after crashes. */
    fflush(s_file);
}

void Rr4Log(const char *format, ...) {
    char line[2048];
    va_list args;
    int n;

    va_start(args, format);
    n = vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    if (n < 0)
        return;
    if ((size_t)n >= sizeof(line))
        n = (int)sizeof(line) - 1;
    fputs(line, stderr);
    WriteFile(line, (size_t)n);
}

static void SDLCALL LogOutput(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    static const char *const kNames[SDL_LOG_PRIORITY_COUNT] = {"", "trace", "verbose", "debug",
                                                               "info", "warn", "error", "critical"};
    (void)userdata;
    if (s_sdl_output)
        s_sdl_output(s_sdl_userdata, category, priority, message);
    if (s_file && message) {
        const char *name = priority >= 0 && priority < SDL_LOG_PRIORITY_COUNT ? kNames[priority] : "";
        char line[2048];
        int n = snprintf(line, sizeof(line), "%s%s%s\n", name, *name ? ": " : "", message);
        if (n > 0)
            WriteFile(line, (size_t)n < sizeof(line) ? (size_t)n : sizeof(line) - 1);
    }
}

int Rr4LogOpen(const char *path) {
    FILE *file = path && *path ? Psyz_FileOpen(path, "w") : NULL;

    if (!file)
        return 0;
    Rr4LogClose();
    s_file = file;
    snprintf(s_path, sizeof(s_path), "%s", path);
    SDL_GetLogOutputFunction(&s_sdl_output, &s_sdl_userdata);
    if (s_sdl_output == LogOutput) {
        s_sdl_output = SDL_GetDefaultLogOutputFunction();
        s_sdl_userdata = NULL;
    }
    SDL_SetLogOutputFunction(LogOutput, NULL);
    return 1;
}

const char *Rr4LogPath(void) { return s_path; }

void Rr4LogClose(void) {
    if (!s_file)
        return;
    SDL_SetLogOutputFunction(s_sdl_output ? s_sdl_output : SDL_GetDefaultLogOutputFunction(), s_sdl_userdata);
    fclose(s_file);
    s_file = NULL;
    s_path[0] = '\0';
}
