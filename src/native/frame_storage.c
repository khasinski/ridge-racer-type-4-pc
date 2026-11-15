/* Native protection for the original frame packet arenas. No packet format
 * or ordering-table ownership changes: the OS rejects a write into either
 * guard before it can reach another member/bank. */
#include "rr/frame_storage_types.h"
#include "rr/frame_loop.h"
#include <stdint.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

static int ProtectGuard(void *address, size_t bytes) {
#ifdef _WIN32
    SYSTEM_INFO info;
    DWORD oldProtection;
    GetSystemInfo(&info);
    size_t page = info.dwPageSize;
#else
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0)
        return 0;
    size_t page = (size_t)pageSize;
#endif
    /* Linkers may cap a global's requested alignment. Validate the actual
     * address against host pages before changing any memory protection. */
    if ((uintptr_t)address % page || bytes % page)
        return 0;
#ifdef _WIN32
    return VirtualProtect(address, bytes, PAGE_NOACCESS, &oldProtection) != 0;
#else
    return mprotect(address, bytes, PROT_NONE) == 0;
#endif
}

s32 InitFramePacketStorage(void) {
    for (size_t i = 0; i < 2; ++i) {
        FrameBuffer *frame = &DrawBuffers[i].buffer;
        if (!ProtectGuard(frame->packetGuardBefore, sizeof frame->packetGuardBefore) ||
            !ProtectGuard(frame->packetGuardAfter, sizeof frame->packetGuardAfter))
            return 0;
    }
    return 1;
}
