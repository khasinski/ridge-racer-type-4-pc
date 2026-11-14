#include "replay_file.h"

#include <SDL3/SDL.h>
#include <psyz/file.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char kMagic[8] = {'R', 'R', '4', 'R', 'E', 'P', 'L', 'Y'};

/* Header offsets (little-endian). */
enum {
    H_MAGIC = 0,
    H_VERSION = 8,
    H_HEADER_SIZE = 12,
    H_PAYLOAD_SIZE = 16,
    H_PAYLOAD_CRC = 20,
    H_LAYOUT = 24,
    H_NTSC = 28,
    H_MODE = 32,
    H_COURSE = 36,
    H_REVERSE = 40,
    H_CAR = 44,
    H_CARS = 48,
    H_RACE_TIME = 52,
    H_FRAMES = 56,
    H_SAVED_AT = 60,
    H_TITLE = 68,
    H_BYTE_ORDER = H_TITLE + REPLAY_TITLE_MAX, /* 116: 0x01020304 as stored by the writer */
    H_RESERVED = 120,
    H_HEADER_CRC = 124,
};
_Static_assert(H_HEADER_CRC + 4 == REPLAY_HEADER_SIZE, "replay header layout");

uint32_t ReplayCrc32(const void *data, size_t size, uint32_t crc) {
    static uint32_t table[256];
    const unsigned char *p = data;

    if (!table[1]) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++)
                c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
    }
    crc = ~crc;
    while (size--)
        crc = table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

static void Put32(unsigned char *p, uint32_t v) {
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static uint32_t Get32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void Fail(char *err, size_t size, const char *message) {
    if (err && size)
        snprintf(err, size, "%s", message);
}

unsigned char *ReplayFileEncode(const ReplayFileInfo *info, const ReplayBlock *blocks, int count,
                                size_t *size) {
    size_t payload = 0, at;
    unsigned char *out;
    uint32_t byte_order = 0x01020304u;

    if (count < 0 || count > REPLAY_BLOCKS_MAX)
        return NULL;
    for (int i = 0; i < count; i++) {
        size_t name = strlen(blocks[i].name);
        if (!name || name > REPLAY_NAME_MAX || (blocks[i].size && !blocks[i].data))
            return NULL;
        payload += 1 + name + 4 + blocks[i].size;
    }
    if (payload > REPLAY_PAYLOAD_MAX)
        return NULL;
    out = calloc(1, REPLAY_HEADER_SIZE + payload);
    if (!out)
        return NULL;
    at = REPLAY_HEADER_SIZE;
    for (int i = 0; i < count; i++) {
        size_t name = strlen(blocks[i].name);
        out[at++] = (unsigned char)name;
        memcpy(out + at, blocks[i].name, name);
        at += name;
        Put32(out + at, blocks[i].size);
        at += 4;
        if (blocks[i].size)
            memcpy(out + at, blocks[i].data, blocks[i].size);
        at += blocks[i].size;
    }
    memcpy(out + H_MAGIC, kMagic, sizeof(kMagic));
    Put32(out + H_VERSION, REPLAY_FILE_VERSION);
    Put32(out + H_HEADER_SIZE, REPLAY_HEADER_SIZE);
    Put32(out + H_PAYLOAD_SIZE, (uint32_t)payload);
    Put32(out + H_PAYLOAD_CRC, ReplayCrc32(out + REPLAY_HEADER_SIZE, payload, 0));
    Put32(out + H_LAYOUT, info->layout);
    Put32(out + H_NTSC, info->ntsc);
    Put32(out + H_MODE, (uint32_t)info->mode);
    Put32(out + H_COURSE, (uint32_t)info->course);
    Put32(out + H_REVERSE, (uint32_t)info->reverse);
    Put32(out + H_CAR, (uint32_t)info->car);
    Put32(out + H_CARS, (uint32_t)info->cars);
    Put32(out + H_RACE_TIME, (uint32_t)info->race_time);
    Put32(out + H_FRAMES, info->frames);
    Put32(out + H_SAVED_AT, (uint32_t)((uint64_t)info->saved_at & 0xFFFFFFFFu));
    Put32(out + H_SAVED_AT + 4, (uint32_t)((uint64_t)info->saved_at >> 32));
    for (int i = 0; i < REPLAY_TITLE_MAX - 1 && info->title[i]; i++)
        out[H_TITLE + i] = (unsigned char)info->title[i];
    memcpy(out + H_BYTE_ORDER, &byte_order, 4);
    Put32(out + H_HEADER_CRC, ReplayCrc32(out, H_HEADER_CRC, 0));
    *size = REPLAY_HEADER_SIZE + payload;
    return out;
}

int ReplayFileDecodeInfo(const unsigned char *data, size_t size, uint32_t layout, ReplayFileInfo *info,
                         char *err, size_t err_size) {
    uint32_t byte_order = 0x01020304u, version;
    char message[160];

    memset(info, 0, sizeof(*info));
    if (size < REPLAY_HEADER_SIZE || memcmp(data + H_MAGIC, kMagic, sizeof(kMagic))) {
        Fail(err, err_size, "Not a Ridge Racer Type 4 replay file.");
        return 0;
    }
    if (Get32(data + H_HEADER_CRC) != ReplayCrc32(data, H_HEADER_CRC, 0)) {
        Fail(err, err_size, "The replay file is damaged (header checksum).");
        return 0;
    }
    version = Get32(data + H_VERSION);
    info->version = version;
    if (version != REPLAY_FILE_VERSION || Get32(data + H_HEADER_SIZE) != REPLAY_HEADER_SIZE) {
        snprintf(message, sizeof(message), "Replay format %u is not supported (this build reads format %d).",
                 version, REPLAY_FILE_VERSION);
        Fail(err, err_size, message);
        return 0;
    }
    if (memcmp(data + H_BYTE_ORDER, &byte_order, 4)) {
        Fail(err, err_size, "The replay was saved on a machine of another byte order.");
        return 0;
    }
    info->layout = Get32(data + H_LAYOUT);
    info->ntsc = Get32(data + H_NTSC);
    info->mode = (int32_t)Get32(data + H_MODE);
    info->course = (int32_t)Get32(data + H_COURSE);
    info->reverse = (int32_t)Get32(data + H_REVERSE);
    info->car = (int32_t)Get32(data + H_CAR);
    info->cars = (int32_t)Get32(data + H_CARS);
    info->race_time = (int32_t)Get32(data + H_RACE_TIME);
    info->frames = Get32(data + H_FRAMES);
    info->saved_at = (int64_t)((uint64_t)Get32(data + H_SAVED_AT) | (uint64_t)Get32(data + H_SAVED_AT + 4) << 32);
    memcpy(info->title, data + H_TITLE, REPLAY_TITLE_MAX - 1);
    info->title[REPLAY_TITLE_MAX - 1] = '\0';
    if (info->layout != layout) {
        Fail(err, err_size, "The replay was saved by a different version of the game.");
        return 0;
    }
    return 1;
}

int ReplayFileDecode(const unsigned char *data, size_t size, uint32_t layout, ReplayFileImage *image,
                     char *err, size_t err_size) {
    size_t payload, at, end;

    memset(image->blocks, 0, sizeof(image->blocks));
    image->count = 0;
    if (!ReplayFileDecodeInfo(data, size, layout, &image->info, err, err_size))
        return 0;
    payload = Get32(data + H_PAYLOAD_SIZE);
    if (payload > REPLAY_PAYLOAD_MAX || payload != size - REPLAY_HEADER_SIZE) {
        Fail(err, err_size, "The replay file is truncated or has extra data.");
        return 0;
    }
    if (Get32(data + H_PAYLOAD_CRC) != ReplayCrc32(data + REPLAY_HEADER_SIZE, payload, 0)) {
        Fail(err, err_size, "The replay file is damaged (data checksum).");
        return 0;
    }
    at = REPLAY_HEADER_SIZE;
    end = size;
    while (at < end) {
        size_t name = data[at++];
        uint32_t block;
        ReplayBlock *b;

        if (image->count >= REPLAY_BLOCKS_MAX || !name || name > REPLAY_NAME_MAX || end - at < name + 4) {
            Fail(err, err_size, "The replay file has an invalid block list.");
            return 0;
        }
        b = &image->blocks[image->count++];
        memcpy(b->name, data + at, name);
        b->name[name] = '\0';
        if (memchr(b->name, '\0', name)) {
            Fail(err, err_size, "The replay file has an invalid block name.");
            return 0;
        }
        for (int i = 0; i < image->count - 1; i++) {
            if (!strcmp(image->blocks[i].name, b->name)) {
                Fail(err, err_size, "The replay file has duplicate blocks.");
                return 0;
            }
        }
        at += name;
        block = Get32(data + at);
        at += 4;
        if (block > end - at) {
            Fail(err, err_size, "The replay file has an invalid block size.");
            return 0;
        }
        b->size = block;
        b->data = data + at;
        at += block;
    }
    return 1;
}

static unsigned char *ReadAll(const char *path, size_t limit, size_t *size) {
    FILE *f = Psyz_FileOpen(path, "rb");
    unsigned char *data = NULL;
    size_t used = 0, cap = 0;

    if (!f)
        return NULL;
    for (;;) {
        size_t n;
        if (used == cap) {
            unsigned char *grown;
            cap = cap ? cap * 2 : 64 * 1024;
            if (cap > limit + 1)
                cap = limit + 1;
            if (used == cap)
                break; /* larger than any valid file: let the decoder refuse it */
            grown = realloc(data, cap);
            if (!grown) {
                free(data);
                fclose(f);
                return NULL;
            }
            data = grown;
        }
        n = fread(data + used, 1, cap - used, f);
        used += n;
        if (n == 0)
            break;
    }
    fclose(f);
    *size = used;
    return data;
}

int ReplayFileRead(const char *path, uint32_t layout, ReplayFileImage *image, char *err, size_t err_size) {
    size_t size = 0;

    memset(image, 0, sizeof(*image));
    image->buffer = ReadAll(path, REPLAY_HEADER_SIZE + REPLAY_PAYLOAD_MAX, &size);
    if (!image->buffer) {
        Fail(err, err_size, "The replay file cannot be read.");
        return 0;
    }
    if (!ReplayFileDecode(image->buffer, size, layout, image, err, err_size)) {
        ReplayFileFree(image);
        return 0;
    }
    return 1;
}

int ReplayFileReadInfo(const char *path, uint32_t layout, ReplayFileInfo *info, char *err, size_t err_size) {
    unsigned char header[REPLAY_HEADER_SIZE];
    FILE *f = Psyz_FileOpen(path, "rb");
    size_t n;

    memset(info, 0, sizeof(*info));
    if (!f) {
        Fail(err, err_size, "The replay file cannot be read.");
        return 0;
    }
    n = fread(header, 1, sizeof(header), f);
    fclose(f);
    return ReplayFileDecodeInfo(header, n, layout, info, err, err_size);
}

void ReplayFileFree(ReplayFileImage *image) {
    free(image->buffer);
    memset(image, 0, sizeof(*image));
}

const ReplayBlock *ReplayFileFind(const ReplayFileImage *image, const char *name) {
    for (int i = 0; i < image->count; i++)
        if (!strcmp(image->blocks[i].name, name))
            return &image->blocks[i];
    return NULL;
}

int ReplayFileWrite(const char *path, const ReplayFileInfo *info, const ReplayBlock *blocks, int count,
                    char *err, size_t err_size) {
    char temporary[1100];
    size_t size = 0;
    unsigned char *data = ReplayFileEncode(info, blocks, count, &size);
    FILE *f;
    int failed;

    if (!data) {
        Fail(err, err_size, "The replay could not be encoded.");
        return 0;
    }
    snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    f = Psyz_FileOpen(temporary, "wb");
    if (!f) {
        free(data);
        Fail(err, err_size, "The replay file cannot be created.");
        return 0;
    }
    failed = fwrite(data, 1, size, f) != size;
    if (fclose(f))
        failed = 1;
    free(data);
    if (failed || !SDL_RenamePath(temporary, path)) {
        SDL_RemovePath(temporary);
        Fail(err, err_size, "The replay file could not be written.");
        return 0;
    }
    return 1;
}

int ReplayDirectory(const char *prefs_path, char *out, size_t size) {
    const char *override = SDL_getenv("RR4_REPLAY_DIR");
    SDL_PathInfo info;

    if (override && *override) {
        snprintf(out, size, "%s", override);
    } else if (prefs_path && *prefs_path) {
        const char *slash = NULL;
        for (const char *c = prefs_path; *c; c++)
            if (*c == '/' || *c == '\\')
                slash = c;
        if (slash)
            snprintf(out, size, "%.*sreplays", (int)(slash - prefs_path + 1), prefs_path);
        else
            snprintf(out, size, "replays");
    } else {
        snprintf(out, size, "replays");
    }
    if (SDL_GetPathInfo(out, &info) && info.type == SDL_PATHTYPE_DIRECTORY)
        return 1;
    return SDL_CreateDirectory(out);
}

const char *ReplayModeName(int mode) {
    switch (mode) {
    case 0: return "Time Attack";
    case 1: return "Grand Prix";
    case 2:
    case 3: return "VS Battle";
    case 4: return "Link Battle";
    case 6: return "Extra Trial";
    default: return "Race";
    }
}

static const char *ModeTag(int mode) {
    switch (mode) {
    case 0: return "TA";
    case 1: return "GP";
    case 2:
    case 3: return "VS";
    case 4: return "Link";
    case 6: return "Extra";
    default: return "Race";
    }
}

static void FormatTime(int32_t ms, char *out, size_t size, int for_name) {
    if (ms < 0) {
        snprintf(out, size, "%s", for_name ? "no-time" : "");
        return;
    }
    snprintf(out, size, for_name ? "%d-%02d-%03d" : "%d:%02d.%03d", ms / 60000, ms / 1000 % 60, ms % 1000);
}

static void FormatDate(int64_t at, char *out, size_t size, int compact) {
    SDL_DateTime dt;

    if (!SDL_TimeToDateTime((SDL_Time)at * SDL_NS_PER_SECOND, &dt, true))
        SDL_zero(dt);
    if (compact)
        snprintf(out, size, "%04d%02d%02d-%02d%02d%02d", dt.year, dt.month, dt.day, dt.hour, dt.minute,
                 dt.second);
    else
        snprintf(out, size, "%04d-%02d-%02d %02d:%02d", dt.year, dt.month, dt.day, dt.hour, dt.minute);
}

void ReplayFileName(const ReplayFileInfo *info, char *out, size_t size) {
    char time_text[32], date[32];

    FormatTime(info->race_time, time_text, sizeof(time_text), 1);
    FormatDate(info->saved_at, date, sizeof(date), 1);
    snprintf(out, size, "%s_course%d%s_car%d-%d_%s_%s" REPLAY_FILE_EXTENSION, ModeTag(info->mode),
             info->course + 1, info->reverse ? "R" : "", (info->car >> 8) & 0xFF, info->car & 0xFF,
             time_text, date);
}

static int EndsWith(const char *s, const char *suffix) {
    size_t n = strlen(s), m = strlen(suffix);
    return n >= m && !strcmp(s + n - m, suffix);
}

static int NewestFirst(const void *a, const void *b) {
    const ReplayListEntry *x = a, *y = b;
    if (x->info.saved_at != y->info.saved_at)
        return x->info.saved_at < y->info.saved_at ? 1 : -1;
    return strcmp(y->path, x->path);
}

typedef struct ListContext {
    const char *dir;
    uint32_t layout;
    ReplayListEntry *out;
    int max, count;
} ListContext;

static SDL_EnumerationResult SDLCALL ListOne(void *userdata, const char *dirname, const char *fname) {
    ListContext *c = userdata;
    ReplayListEntry *e;
    char time_text[32], date[32], err[160];

    (void)dirname;
    if (!EndsWith(fname, REPLAY_FILE_EXTENSION))
        return SDL_ENUM_CONTINUE;
    ReplayListEntry candidate;
    e = &candidate;
    memset(e, 0, sizeof(*e));
    if ((size_t)snprintf(e->path, sizeof(e->path), "%s/%s", c->dir, fname) >= sizeof(e->path))
        return SDL_ENUM_CONTINUE;
    e->ok = ReplayFileReadInfo(e->path, c->layout, &e->info, err, sizeof(err));
    if (e->ok || e->info.version) {
        FormatTime(e->info.race_time, time_text, sizeof(time_text), 0);
        FormatDate(e->info.saved_at, date, sizeof(date), 0);
    }
    if (e->ok)
        snprintf(e->label, sizeof(e->label), "%s%s%s", time_text, time_text[0] ? "  " : "", date);
    else
        snprintf(e->label, sizeof(e->label), "%.60s (unreadable)", fname);
    if (c->count < c->max) {
        c->out[c->count++] = candidate;
    } else {
        int oldest = 0;
        for (int i = 1; i < c->count; i++)
            if (NewestFirst(&c->out[i], &c->out[oldest]) > 0)
                oldest = i;
        if (NewestFirst(&candidate, &c->out[oldest]) < 0)
            c->out[oldest] = candidate;
    }
    return SDL_ENUM_CONTINUE;
}

int ReplayFileList(const char *dir, uint32_t layout, ReplayListEntry *out, int max) {
    ListContext c = {dir, layout, out, max, 0};

    if (!dir || !*dir || max <= 0)
        return 0;
    if (!SDL_EnumerateDirectory(dir, ListOne, &c))
        return 0;
    qsort(out, (size_t)c.count, sizeof(*out), NewestFirst);
    return c.count;
}
