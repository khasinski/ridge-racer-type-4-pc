/* BIOS file services (A(00h)..A(04h): open, lseek, read, write, close) under
 * the game's names. Memory card files ("buXY:") are PsyZ host files whose
 * transfers take their PS1 time on the emulated clock (Psyz_CardFile*): the
 * host I/O completes inside the call, but an FASYNC card transfer raises its
 * SwCARD end-of-I/O event only when the BIOS would have finished the last
 * sector, and a synchronous one or a file creation returns after that time.
 * The "sio:" link cable device is PsyZ libcomb's driver (Psyz_CombRead /
 * Psyz_CombWrite): its serial port reaches the other console over the
 * network when the link is set up (psyz/link.h), otherwise nothing is ever
 * received and a synchronous transfer runs into the game's wait callback. */
#include <kernel.h>
#include <romio.h>
#include <psyz/link.h>
#include <psyz/system.h>
#include "rr/bios_events.h"
#include "rr/serial_handles.h"

enum { FILE_HANDLE_COUNT = 16 };

/* BIOS open mode bit for asynchronous transfers (romio.h FASYNC). The host
 * <fcntl.h>, reached through the PsyZ headers, redefines FASYNC as O_ASYNC
 * (0x40 on macOS, 0x2000 on Linux), so the BIOS value is spelled out. */
enum { FILE_MODE_ASYNC = 0x8000, FILE_MODE_CREATE = 0x200 };

typedef enum {
    FILE_KIND_FREE,
    FILE_KIND_HOST,
    FILE_KIND_LINK,
} FileKind;

typedef struct {
    FileKind kind;
    int host;
    int card;
    long channel; /* card port << 4 | slot */
    int async;
} FileHandle;

static FileHandle FileHandles[FILE_HANDLE_COUNT];

static FileHandle *FileLookup(s32 descriptor) {
    if (descriptor < 0 || descriptor >= FILE_HANDLE_COUNT ||
        FileHandles[descriptor].kind == FILE_KIND_FREE) {
        return NULL;
    }
    return &FileHandles[descriptor];
}

static int FileIsDevice(const char *name, const char *device) {
    while (*device) {
        if (*name++ != *device++) {
            return 0;
        }
    }
    return 1;
}

static s32 FileTransferDone(FileHandle *file, long count) {
    if (count <= 0) {
        return -1; /* end of file or host error: no data was transferred */
    }
    if (file->card) {
        Psyz_CardFileTransfer(file->channel, count, file->async);
    }
    return (s32)count;
}

s32 FileOpen(const char *name, s32 mode) {
    s32 descriptor;
    for (descriptor = 0; descriptor < FILE_HANDLE_COUNT; descriptor++) {
        if (FileHandles[descriptor].kind == FILE_KIND_FREE) {
            break;
        }
    }
    if (!name || descriptor == FILE_HANDLE_COUNT) {
        return -1;
    }
    if (FileIsDevice(name, "sio:")) {
        FileHandles[descriptor] =
            (FileHandle){FILE_KIND_LINK, -1, 0, 0, (mode & FILE_MODE_ASYNC) != 0};
        return descriptor;
    }
    int host = psyz_open(name, mode);
    if (host < 0) {
        return -1;
    }
    int card = FileIsDevice(name, "bu") && name[2] >= '0' && name[2] <= '1' &&
               name[3] >= '0' && name[3] <= '3';
    FileHandles[descriptor] = (FileHandle){FILE_KIND_HOST, host, card,
                                           card ? (name[2] - '0') << 4 | (name[3] - '0') : 0,
                                           (mode & FILE_MODE_ASYNC) != 0};
    if (FileHandles[descriptor].card && (mode & FILE_MODE_CREATE)) {
        Psyz_CardFileCreate();
    }
    return descriptor;
}

s32 FileSeek(s32 descriptor, s32 offset, s32 whence) {
    FileHandle *file = FileLookup(descriptor);
    if (!file || file->kind != FILE_KIND_HOST) {
        return -1;
    }
    return (s32)psyz_lseek(file->host, (unsigned int)offset, whence);
}

s32 FileRead(s32 descriptor, void *buffer, s32 count) {
    FileHandle *file = FileLookup(descriptor);
    if (!file || !buffer || count < 0) {
        return -1;
    }
    if (file->kind == FILE_KIND_LINK) {
        return (s32)Psyz_CombRead(buffer, count, file->async);
    }
    return FileTransferDone(file, psyz_read(file->host, buffer, count));
}

s32 FileWrite(s32 descriptor, const void *buffer, s32 count) {
    FileHandle *file = FileLookup(descriptor);
    if (!file || !buffer || count < 0) {
        return -1;
    }
    if (file->kind == FILE_KIND_LINK) {
        return (s32)Psyz_CombWrite(buffer, count, file->async);
    }
    return FileTransferDone(
        file, psyz_write(file->host, (char *)buffer, count));
}

s32 FileClose(s32 descriptor) {
    FileHandle *file = FileLookup(descriptor);
    s32 result = descriptor;
    if (!file) {
        return -1;
    }
    if (file->kind == FILE_KIND_HOST && psyz_close(file->host) != 0) {
        result = -1;
    }
    file->kind = FILE_KIND_FREE;
    return result;
}
