#ifndef RR_REPLAY_TYPES_H
#define RR_REPLAY_TYPES_H
#include "rr/race_types.h"

/* Four encoded input bytes; byte-aligned transfers also support packed streams. */
typedef struct {u32 word;} RR_PACKED ReplayFrame4;

/* One car's encoded input for the current frame; replays record the same four
 * bytes as a ReplayFrame4. CarInputs (0x800ADAD8) holds one per player slot.
 *   +0 steer   signed steering, -127..127
 *   +1 throttle, +2 brake  analogue pedal levels 0..255
 *   +3 buttons digital control bits 0-4; bits 5-7 input mode (0x20 analogue) */
typedef union {
    struct {
        s8 steer;
        u8 throttle;
        u8 brake;
        u8 buttons;
    };
    ReplayFrame4 frame;
} CarInput4;
extern CarInput4 CarInputs[4];

/* Serial-link input exchange for the two cars on each console: pending is the
 * local pair queued for transmission, local the pair applied after the link
 * delay, remote the pair received from the peer. Initialisation clears the
 * pairs as two words. PS1: LinkRemoteCarInputs 0x800ACBE0, LinkLocalCarInputs
 * 0x800ACBF0, LinkPendingCarInputs 0x800F3AF0, eight bytes each. */
typedef union {
    CarInput4 cars[2];
    u32 words[2];
} LinkCarInputPair;
extern LinkCarInputPair LinkRemoteCarInputs, LinkLocalCarInputs,
    LinkPendingCarInputs;
typedef struct {
    ReplayFrame4 inputs[3601];
    RaceVehicle snapshots[8];
} ReplaySingleStorage;
typedef struct {
    ReplayFrame4 inputs[2][2701];
    RaceVehicle snapshots[4];
} ReplayTwoStorage;
typedef struct {
    ReplayFrame4 inputs[4][901];
    RaceVehicle snapshots[4];
} ReplayLinkStorage;
typedef union {
    ReplaySingleStorage single;
    ReplayTwoStorage two;
    ReplayLinkStorage link;
} ReplayStorage;
extern ReplayFrame4 *ReplayInputs[4];
/* A replay recorded at field rate (rr/field_rate.h): ReplayInputs holds
 * each original tick's first field (the tick), ReplayFieldInputs its
 * second, and ReplayFrame counts ticks. Host-only; the two players of a
 * single or split-screen race (the link stays at the original rate). */
extern ReplayFrame4 ReplayFieldInputs[2][3601];
/* The rate the replay was recorded at: 1 per field, 0 the original. */
extern s32 ReplayFieldRate;

RR_ASSERT_SIZE(ReplayFrame4,4);
RR_ASSERT_SIZE(CarInput4,4);
RR_ASSERT_SIZE(LinkCarInputPair,8);
#if defined(mips)
RR_ASSERT_OFFSET(ReplaySingleStorage,snapshots,0x3844);
RR_ASSERT_OFFSET(ReplayTwoStorage,snapshots,0x5468);
RR_ASSERT_OFFSET(ReplayLinkStorage,snapshots,0x3850);
RR_ASSERT_SIZE(ReplayStorage,0x60E8);
#endif
#endif
