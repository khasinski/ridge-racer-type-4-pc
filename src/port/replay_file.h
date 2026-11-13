#ifndef RR4_PORT_REPLAY_FILE_H
#define RR4_PORT_REPLAY_FILE_H

#include <stddef.h>
#include <stdint.h>

/* Saved race replays (.rr4replay), kept in a replays/ folder beside the
 * preferences file. A file is a fixed little-endian header followed by a
 * payload of named blocks:
 *
 *   header (REPLAY_HEADER_SIZE bytes)
 *     "RR4REPLY" magic, format version, header size, payload size, CRC-32
 *     of the payload, the layout signature of the build that wrote it (the
 *     host sizes of the game structures the blocks copy), the video timing
 *     and the summary the replay list shows (mode, course, car, time, date),
 *     then the CRC-32 of the header itself.
 *   payload: blocks, each
 *     u8 name length, the name, u32 size, the bytes.
 *
 * The blocks are the game's own state (replay buffer, car snapshots, race
 * setup) in the host layout; src/port/replay_session.c decides what they
 * are. A file whose version or layout signature differs from this build's
 * is refused with a message instead of being interpreted. No game data is
 * needed here: tests/port/replay_file_test.c exercises it. */

enum {
    REPLAY_FILE_VERSION = 1,
    REPLAY_HEADER_SIZE = 128,
    REPLAY_NAME_MAX = 48,     /* block name, without the terminator */
    REPLAY_BLOCKS_MAX = 64,
    REPLAY_PAYLOAD_MAX = 4 << 20,
    REPLAY_TITLE_MAX = 48,
};

#define REPLAY_FILE_EXTENSION ".rr4replay"

/* The summary in the header, enough to list a file without reading it all. */
typedef struct ReplayFileInfo {
    uint32_t version;   /* filled in by the reader */
    uint32_t layout;    /* host layout signature of the writer */
    uint32_t ntsc;      /* video timing the race ran at: 0 PAL, 1 NTSC */
    int32_t mode;       /* GameModeType */
    int32_t course;     /* CourseIndex */
    int32_t reverse;    /* ReverseCourse */
    int32_t car;        /* the first player's car (selection << 8 | variant) */
    int32_t cars;       /* NumCars */
    int32_t race_time;  /* TotalRaceTime (ms), -1 when the race had none */
    uint32_t frames;    /* replay frames recorded */
    int64_t saved_at;   /* seconds since the Unix epoch */
    char title[REPLAY_TITLE_MAX]; /* "Time Attack, Course 1, Car 3-2" */
} ReplayFileInfo;

typedef struct ReplayBlock {
    char name[REPLAY_NAME_MAX + 1];
    uint32_t size;
    const void *data;
} ReplayBlock;

/* A file read whole: the blocks point into buffer. */
typedef struct ReplayFileImage {
    ReplayFileInfo info;
    ReplayBlock blocks[REPLAY_BLOCKS_MAX];
    int count;
    unsigned char *buffer;
} ReplayFileImage;

uint32_t ReplayCrc32(const void *data, size_t size, uint32_t crc);

/* Encodes a file into memory (malloc'd, *size bytes). Returns NULL when a
 * block is invalid (empty or long name, too many, too large). */
unsigned char *ReplayFileEncode(const ReplayFileInfo *info, const ReplayBlock *blocks, int count,
                                size_t *size);
/* Decodes a file image; on success the blocks point into data, which must
 * stay alive. layout is the signature the reader accepts. Returns 1, or 0
 * with a message in err. */
int ReplayFileDecode(const unsigned char *data, size_t size, uint32_t layout, ReplayFileImage *image,
                     char *err, size_t err_size);
/* Decodes only the header (for listing). */
int ReplayFileDecodeInfo(const unsigned char *data, size_t size, uint32_t layout, ReplayFileInfo *info,
                         char *err, size_t err_size);

/* Writes atomically (temporary file and rename). Returns 1, or 0 and err. */
int ReplayFileWrite(const char *path, const ReplayFileInfo *info, const ReplayBlock *blocks, int count,
                    char *err, size_t err_size);
/* Reads and checks a whole file. Free the image with ReplayFileFree. */
int ReplayFileRead(const char *path, uint32_t layout, ReplayFileImage *image, char *err, size_t err_size);
int ReplayFileReadInfo(const char *path, uint32_t layout, ReplayFileInfo *info, char *err, size_t err_size);
void ReplayFileFree(ReplayFileImage *image);
const ReplayBlock *ReplayFileFind(const ReplayFileImage *image, const char *name);

/* The replays folder: RR4_REPLAY_DIR when set, else replays/ beside
 * prefs_path (the preferences file), else replays/ in the working
 * directory. Created when missing. Returns 0 if it cannot be created. */
int ReplayDirectory(const char *prefs_path, char *out, size_t size);

/* "TA_course1_car3-2_1-23-456_20261113-203000.rr4replay" for info. */
void ReplayFileName(const ReplayFileInfo *info, char *out, size_t size);
/* "Time Attack" ... for GameModeType values. */
const char *ReplayModeName(int mode);

/* Saved replays in dir, newest first. */
typedef struct ReplayListEntry {
    char path[1024];
    char label[96];       /* what the menu shows */
    ReplayFileInfo info;
    int ok;               /* the header could be read and matches this build */
} ReplayListEntry;
int ReplayFileList(const char *dir, uint32_t layout, ReplayListEntry *out, int max);

#endif
