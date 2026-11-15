#define _POSIX_C_SOURCE 200809L
#include "rr/frame_storage_types.h"
#include "rr/frame_loop.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <signal.h>
#include <unistd.h>
#endif
#include <SDL3/SDL.h>

static uintptr_t expectedFault;
#ifdef _WIN32
static LONG WINAPI GuardFault(EXCEPTION_POINTERS *info) {
    if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionInformation[1] == expectedFault)
        ExitProcess(99);
    return EXCEPTION_CONTINUE_SEARCH;
}
#else
static void GuardFault(int signal, siginfo_t *info, void *context) {
    (void)signal;
    (void)context;
    _exit((uintptr_t)info->si_addr == expectedFault ? 99 : 98);
}
#endif

int main(int argc, char **argv) {
    if (!InitFramePacketStorage() || !InitFramePacketStorage())
        return 1;
    if (argc == 3) {
        unsigned bank = (unsigned)atoi(argv[1]);
        FrameBuffer *frame = &DrawBuffers[bank & 1].buffer;
        /* Last byte before the arena, first byte after it. */
        volatile u8 *target = argv[2][0] == 'b'
            ? &frame->packetGuardBefore[RR_FRAME_GUARD_BYTES - 1]
            : &frame->packetGuardAfter[0];
        expectedFault = (uintptr_t)target;
#ifdef _WIN32
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        SetUnhandledExceptionFilter(GuardFault);
#else
        struct sigaction action;
        memset(&action, 0, sizeof action);
        action.sa_sigaction = GuardFault;
        action.sa_flags = SA_SIGINFO;
        sigemptyset(&action.sa_mask);
        if (sigaction(SIGSEGV, &action, NULL) || sigaction(SIGBUS, &action, NULL))
            return 2;
#endif
        *target = 0xE2;
        return 3; /* The write must fault, before any neighbouring state changes. */
    }
    for (unsigned bank = 0; bank < 2; ++bank) {
        FrameBuffer *frame = &DrawBuffers[bank].buffer;
        frame->packets1670[0] = 0x12;
        frame->packets1670[RR_FRAME_PACKET_BYTES - 1] = 0x34;
        memset(frame->hudSprites, 0x5A, sizeof frame->hudSprites);
        for (unsigned side = 0; side < 2; ++side) {
            const char *args[] = {argv[0], bank ? "1" : "0", side ? "after" : "before", NULL};
            SDL_Process *child = SDL_CreateProcess(args, false);
            int code = 0;
            if (!child || !SDL_WaitProcess(child, true, &code)) {
                fprintf(stderr, "guard probe process: %s\n", SDL_GetError());
                return 4;
            }
            SDL_DestroyProcess(child);
            if (code != 99) {
                fprintf(stderr, "bank %u side %u: guard probe returned %d\n", bank, side, code);
                return 5;
            }
        }
        if (frame->packets1670[0] != 0x12 ||
            frame->packets1670[RR_FRAME_PACKET_BYTES - 1] != 0x34)
            return 6;
    }
    return 0;
}
