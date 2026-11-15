#ifndef RR_FRAME_STORAGE_TYPES_H
#define RR_FRAME_STORAGE_TYPES_H
#include <stddef.h>
#include "rr/frame_environment_types.h"
/* Two RAM arenas, each holding the drawing environments, ordering tables and
 * packet storage for one bank. Offsets used by the outer loop:
 * display environment 0x5C, ordering tables 0x70 and 0xB70 (704 entries),
 * draw packets 0xB6C and optional 0x166C, initial packet cursor 0x1670. */
#define RR_FRAME_PACKET_BYTES 0x400000u
#define RR_FRAME_GUARD_BYTES 0x10000u
/* Mach-O common symbols accept at most 32 KiB alignment; macOS pages are
 * 4/16 KiB. Windows pages are 4 KiB (64 KiB is allocation granularity).
 * ELF hosts may use 64 KiB pages. Runtime initialization verifies alignment. */
#if defined(__APPLE__)
#define RR_FRAME_GUARD_ALIGNMENT 0x4000u
#elif defined(_WIN32)
#define RR_FRAME_GUARD_ALIGNMENT 0x1000u
#else
#define RR_FRAME_GUARD_ALIGNMENT RR_FRAME_GUARD_BYTES
#endif

typedef struct {
    FrameEnvironment environment00;
    FrameOrderingTable ordering70, secondaryOrderingB70;
    /* Enough space for native tags and enhanced visibility. Guard regions
     * occupy whole 4/16/64 KiB host pages and are made inaccessible before
     * rendering begins, stopping overflow before HUD/other-bank corruption. */
    _Alignas(RR_FRAME_GUARD_ALIGNMENT) u8 packetGuardBefore[RR_FRAME_GUARD_BYTES];
    u8 packets1670[RR_FRAME_PACKET_BYTES];
    u8 packetGuardAfter[RR_FRAME_GUARD_BYTES];
    /* Persistent pre-linked race HUD sprites. PS1: the last 12 0x14-byte
     * packets of the arena (frame offset 0x22688, MenuCoursePreviewArena+0x18
     * for bank 0); built by InitRaceHudPackets, linked by SubmitRaceHudPackets. */
    SpritePacket hudSprites[12];
} FrameBuffer;

typedef union {
    FrameBuffer buffer;
    FrameEnvironment environment;
    FrameOrderingPrefix ordering;
    u32 alignment;
} FrameStorage;

/* Current bank owner. Callers select its environments, ordering words or
 * packet storage; narrower views do not declare separate global objects. */
extern FrameStorage *CurrentDrawBuf;
/* Idempotent; returns zero if the host could not protect every guard. */
s32 InitFramePacketStorage(void);
_Static_assert(offsetof(FrameBuffer, packets1670) ==
                   offsetof(FrameBuffer, packetGuardBefore) + RR_FRAME_GUARD_BYTES,
               "packet underflow guard must be adjacent");
_Static_assert(offsetof(FrameBuffer, packetGuardAfter) ==
                   offsetof(FrameBuffer, packets1670) + RR_FRAME_PACKET_BYTES,
               "packet overflow guard must be adjacent");

RR_ASSERT_SIZE(FrameBuffer, RR_ALIGN_UP(RR_FIELD_OFFSET(FrameBuffer, hudSprites) + 12 * sizeof(SpritePacket), RR_FRAME_GUARD_ALIGNMENT));
RR_ASSERT_OFFSET(FrameBuffer, ordering70, RR_ALIGN_UP(sizeof(FrameEnvironment), _Alignof(FrameOrderingTable)));
RR_ASSERT_OFFSET(FrameBuffer, secondaryOrderingB70, RR_FIELD_OFFSET(FrameBuffer, ordering70) + sizeof(FrameOrderingTable));
RR_ASSERT_OFFSET(FrameBuffer, packets1670, RR_ALIGN_UP(sizeof(FrameEnvironment) + 2 * sizeof(FrameOrderingTable), RR_FRAME_GUARD_ALIGNMENT) + RR_FRAME_GUARD_BYTES);
RR_ASSERT_SIZE(FrameStorage, sizeof(FrameBuffer));
/* Both views must name the same ordering table on every host. */
_Static_assert(offsetof(FrameStorage, ordering.ordering70) ==
                   offsetof(FrameStorage, buffer.ordering70),
               "FrameStorage ordering views disagree");
#endif
