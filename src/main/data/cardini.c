#include "rr/work_arena.h"
#include "game/menu/menutra21.h"
#include "types.h"
#include "rr/bios_devices.h"
#include "psyq/libcard/cardsts.h"
#include "psyq/libcard/init.h"
#include "psyq/libc/printf.h"
#include "psyq/libcard/cardc162.h"
#include "psyq/libgpu/gpuctl.h"
#include "game/data/cardini.h"
#include "rr/bios_events.h"
#include "rr/serial_handles.h"
#include "rr/overlay667_packet.h"
#include "rr/card_format.h"
#include "game/data/globals.h"
#include "rr/spu_tick_control.h"

extern void _bu_init(void);
extern int HwCardEvIoe;
extern int HwCardEvError;
extern int HwCardEvTimeout;
extern int HwCardEvNew;
extern int SwCardEvIoe;
extern int SwCardEvError;
extern int SwCardEvTimeout;
extern int SwCardEvNew;
extern char StrCardDirPatternFmt[];
/* BIOS directory entry (struct DIRENTRY): firstfile/nextfile fill one per
 * card file. `next` is a pointer, so the host entry is wider than the PS1's
 * 0x28 bytes; the table is indexed by entry, never by a PS1 byte stride. */
typedef struct CardDirEntry {
    char name[20];
    s32 attr;
    s32 size;
    struct CardDirEntry *next;
    char system[8];
} CardDirEntry;
extern CardDirEntry CardDirEntries[15];
extern void *nextfile(void *directory);
extern char CardFilePathFmt[];
extern int CardFileHandle;

void CardSystemShutdown(void);
int PollHwCardEvents(int arg);
void ClearHwCardEvents(void);
int WaitSwCardEvent(void);
void ClearSwCardEvents(void);
int CountCardFiles(int a, int b);
int CardHasFile(int a0, unsigned char *a1);
s32 CardFormatSlot(s32 arg0, s32 arg1);

void CardSystemInit(void) {
    InitCARD(1);
    StartCARD();
    _bu_init();
    EnterCriticalSection();
    HwCardEvIoe = OpenEvent(0xF4000001, 4, 0x2000, 0);
    HwCardEvError = OpenEvent(0xF4000001, 0x8000, 0x2000, 0);
    HwCardEvTimeout = OpenEvent(0xF4000001, 0x100, 0x2000, 0);
    HwCardEvNew = OpenEvent(0xF4000001, 0x2000, 0x2000, 0);
    SwCardEvIoe = OpenEvent(0xF0000011, 4, 0x2000, 0);
    SwCardEvError = OpenEvent(0xF0000011, 0x8000, 0x2000, 0);
    SwCardEvTimeout = OpenEvent(0xF0000011, 0x100, 0x2000, 0);
    SwCardEvNew = OpenEvent(0xF0000011, 0x2000, 0x2000, 0);
    ExitCriticalSection();
    EnableEvent(HwCardEvIoe);
    EnableEvent(HwCardEvError);
    EnableEvent(HwCardEvTimeout);
    EnableEvent(HwCardEvNew);
    EnableEvent(SwCardEvIoe);
    EnableEvent(SwCardEvError);
    EnableEvent(SwCardEvTimeout);
    EnableEvent(SwCardEvNew);
    CardPortPollStatus[1] = 0x5A;
    CardPortPollStatus[0] = 0x5A;
    SaveBuffer = SharedWorkArena;
}

void CardSystemShutdown(void) {
    EnterCriticalSection();
    CloseEvent(HwCardEvIoe);
    CloseEvent(HwCardEvError);
    CloseEvent(HwCardEvTimeout);
    CloseEvent(HwCardEvNew);
    CloseEvent(SwCardEvIoe);
    CloseEvent(SwCardEvError);
    CloseEvent(SwCardEvTimeout);
    CloseEvent(SwCardEvNew);
    ExitCriticalSection();
    StopCARD();
}

int PollHwCardEvents(int arg) {
    int r;
    int t;
    r = TestEvent(HwCardEvIoe) == 1;
    if (TestEvent(HwCardEvError) == 1) r = 2;
    if (TestEvent(HwCardEvTimeout) == 1) r = 3;
    if (TestEvent(HwCardEvNew) == 1) r = 4;
    t = CardPortTimer[arg]++;
    if (t >= 0x78) r = 2;
    return r;
}

s32 GetHwCardEvent(void) {
    if (TestEvent(HwCardEvIoe) == 1) {
        return 1;
    }
    if (TestEvent(HwCardEvError) == 1) {
        return 2;
    }
    if (TestEvent(HwCardEvTimeout) != 1) {
        return (TestEvent(HwCardEvNew) == 1) * 4;
    }
    return 3;
}

void ClearHwCardEvents(void) {
    TestEvent(HwCardEvIoe);
    TestEvent(HwCardEvError);
    TestEvent(HwCardEvTimeout);
    TestEvent(HwCardEvNew);
}

/* The game spins here until the BIOS card driver raises the request's
 * software event (after _card_clear); the host's card requests have raised
 * it by the time they return. */

int WaitSwCardEvent(void) {
    for (;;) {
        if (TestEvent(SwCardEvIoe) == 1) return 1;
        if (TestEvent(SwCardEvError) == 1) return 2;
        if (TestEvent(SwCardEvTimeout) == 1) return 3;
        if (TestEvent(SwCardEvNew) == 1) return 4;
    }
}

void ClearSwCardEvents(void) {
    TestEvent(SwCardEvIoe);
    TestEvent(SwCardEvError);
    TestEvent(SwCardEvTimeout);
    TestEvent(SwCardEvNew);
}

int CountCardFiles(int a, int b) {
    char sp[32];
    CardDirEntry *s0;
    int n = 0;
    PsyqSprintf((s8 *)sp, (const s8 *)StrCardDirPatternFmt, a, b);
    s0 = CardDirEntries;
    if (firstfile(sp, s0) == s0) {
        /* A card holds at most 15 files. */
        do { n++; s0++; } while (n < 15 && nextfile(s0) == s0);
    }
    return n;
}

int CardFreeBlocks(int a0) {
    int a2 = 0, a1 = 0;
    CardDirEntry *v1;
    int v;
    if (a0 > 0) {
        v1 = CardDirEntries;
        do {
            a1 += v1->size;
            v1++;
        } while (++a2 < a0);
    }
    v = a1;
    if (v < 0) v += 0x1FFF;
    a1 = v >> 13;
    return 15 - a1;
}

int CardHasFile(int a0, unsigned char *a1) {
    int t1 = 0, t2 = 0;
    CardDirEntry *t0;
    int a3, v1;
    unsigned char a2;
    CardDirEntry *b = CardDirEntries;
    if (a0 > 0) {
        t0 = b;
        do {
            a3 = 0;
            v1 = a3;
            do {
                a2 = a1[v1];
                if (a2 == 0) goto l64;
                if (a2 != ((const unsigned char *)t0)[v1]) goto l74; /* name, then attr */
                v1++;
            } while (v1 < 0x15);
l64:
            t1++;
            if (a3 == 0) {
                t2 = 1;
                break;
            }
            goto next_entry;
l74:
            a3++;
            goto l64;
next_entry:
            t0++;
        } while (t1 < a0);
    }
    return t2;
}

int CardReadFile(int port, int slot, const char *name, void *buffer, int blocks) {
    char path[32];
    int descriptor;
    int length;
    int result;
    PsyqSprintf((s8 *)path, (const s8 *)CardFilePathFmt, port, slot, name);
    length = blocks << 13;
    descriptor = FileOpen(path, 0x8001);
    if (descriptor != -1) {
        CardFileHandle = descriptor;
        ClearHwCardEvents();
        while (FileRead(descriptor, buffer, length) == 0) {
        }
        result = 0;
        FileClose(descriptor);
    } else {
        result = -1;
    }
    return result;
}

int CardReadFileAt(int port, int slot, const char *name, void *buffer, int bytes, int offset) {
    char path[32];
    int descriptor;
    int result = -1;
    int length;
    PsyqSprintf((s8 *)path, (const s8 *)CardFilePathFmt, port, slot, name);
    length = bytes;
    descriptor = FileOpen(path, 0x8001);
    if (descriptor != result) {
        if (FileSeek(descriptor, offset, 0) != result) {
            CardFileHandle = descriptor;
            ClearHwCardEvents();
            while (FileRead(descriptor, buffer, length) == 0) ;
            result = 0;
        }
        FileClose(descriptor);
        return result;
    }
    return result;
}

int CardWriteFile(int port, int slot, const char *name, const void *buffer, int blocks, int create) {
    char path[32];
    int descriptor;
    int length;
    int result;
    PsyqSprintf((s8 *)path, (const s8 *)CardFilePathFmt, port, slot, name);
    if (create == 1) {
        descriptor = FileOpen(path, (blocks << 16) | 0x200);
        if (descriptor == -1) {
            return -1;
        }
        FileClose(descriptor);
    }
    length = blocks << 13;
    descriptor = FileOpen(path, 0x8002);
    if (descriptor != -1) {
        CardFileHandle = descriptor;
        ClearHwCardEvents();
        while (FileWrite(descriptor, buffer, length) == 0) {
        }
        result = 0;
        FileClose(descriptor);
    } else {
        result = -1;
    }
    return result;
}

int CardWriteFileAt(int port, int slot, const char *name, const void *buffer, int bytes, int offset) {
    char path[32];
    int descriptor;
    int result = -1;
    int length;
    PsyqSprintf((s8 *)path, (const s8 *)CardFilePathFmt, port, slot, name);
    length = bytes;
    descriptor = FileOpen(path, 0x8002);
    if (descriptor != result) {
        if (FileSeek(descriptor, offset, 0) != result) {
            CardFileHandle = descriptor;
            ClearHwCardEvents();
            while (FileWrite(descriptor, buffer, length) == 0) ;
            result = 0;
        }
        FileClose(descriptor);
        return result;
    }
    return result;
}

int CardUpdate(int port, int slot) {
    int channel;
    int status;
    int sync[2];
    int previousResult;
    channel = (port << 4) + slot;
    switch (CardPortState[port]) {
    case RR_CARD_PORT_START_INFO:
        _card_info(channel);
        CardPortState[port] = RR_CARD_PORT_WAIT_INFO;
        CardPortTimer[port] = 0;
        CardPortLastResult[port] = 0;
        break;
    case RR_CARD_PORT_WAIT_INFO:
        status = PollHwCardEvents(port);
        if (status == 0) break;
        switch (status) {
        case 1:
            CardPortResult[port] = status;
            CardPortState[port] = RR_CARD_PORT_START_READ;
            CardPortTimer[port] = 0;
            break;
        case 3:
            CardPortResult[port] = -1;
            CardPortState[port] = RR_CARD_PORT_PUBLISH_RESULT;
            CardPortLoaded[port] = 0;
            break;
        case 4:
            CardPortResult[port] = 2;
            CardPortState[port] = RR_CARD_PORT_START_READ;
            CardPortTimer[port] = 0;
            break;
        case 2:
        default:
            CardPortResult[port] = -4;
            CardPortState[port] = RR_CARD_PORT_PUBLISH_RESULT;
            CardPortLoaded[port] = 0;
            break;
        }
        break;
    case RR_CARD_PORT_START_READ:
        if (_cardCmd5ABegin(channel, port * RR_CARD_RAW_REPLY_BYTES + CardRawSectorWorkBuf)) {
            CardPortState[port] = RR_CARD_PORT_WAIT_READ;
            CardPortTimer[port] = 0;
        } else {
            if (CardPortTimer[port]++ >= 0x78) {
                CardPortResult[port] = -4;
                CardPortState[port] = RR_CARD_PORT_PUBLISH_RESULT;
                CardPortLoaded[port] = 0;
            }
        }
        break;
    case RR_CARD_PORT_WAIT_READ:
        status = MemCardSync(1, &sync[0], &sync[1]);
        if (status != 0) {
            if (status > 0) {
                status = sync[1];
                switch (status) {
                case 0:
                    CardPortResult[port] = 3;
                    if (CardPortLoaded[port] == 1) {
                        CardPortState[port] = RR_CARD_PORT_PUBLISH_RESULT;
                    } else {
                        CardPortState[port] = RR_CARD_PORT_START_LOAD;
                    }
                    break;
                case 2:
                    if (CardPortResult[port] == 1) {
                        if (CardPortLoaded[port] == 1) {
                            CardPortState[port] = RR_CARD_PORT_PUBLISH_RESULT;
                        } else {
                            CardPortState[port] = RR_CARD_PORT_START_LOAD;
                        }
                    } else {
                        CardPortResult[port] = 2;
                        ClearSwCardEvents();
                        _card_clear(channel);
                        WaitSwCardEvent();
                        CardPortState[port] = RR_CARD_PORT_START_LOAD;
                        CardPortLoaded[port] = 0;
                    }
                    break;
                case 1:
                    CardPortResult[port] = -1;
                    CardPortState[port] = RR_CARD_PORT_PUBLISH_RESULT;
                    CardPortLoaded[port] = 0;
                    break;
                case 3:
                    CardPortResult[port] = 4;
                    ClearSwCardEvents();
                    _card_clear(channel);
                    WaitSwCardEvent();
                    CardPortState[port] = RR_CARD_PORT_START_LOAD;
                    CardPortLoaded[port] = 0;
                    break;
                }
            } else {
                CardPortResult[port] = -4;
                CardPortState[port] = RR_CARD_PORT_PUBLISH_RESULT;
                CardPortLoaded[port] = 0;
            }
        } else {
            if (CardPortTimer[port]++ >= 0x78) {
                CardPortResult[port] = -4;
                CardPortState[port] = RR_CARD_PORT_PUBLISH_RESULT;
                CardPortLoaded[port] = 0;
            }
        }
        break;
    case RR_CARD_PORT_START_LOAD:
        ClearHwCardEvents();
        _card_load(channel);
        CardPortState[port] = RR_CARD_PORT_WAIT_LOAD;
        CardPortTimer[port] = 0;
        break;
    case RR_CARD_PORT_WAIT_LOAD:
        status = PollHwCardEvents(port);
        if (status == 0) break;
        CardPortState[port] = RR_CARD_PORT_PUBLISH_RESULT;
        switch (status) {
        case 1:
            CardPortLoaded[port] = status;
            break;
        case 3:
            CardPortResult[port] = -1;
            CardPortLoaded[port] = 0;
            break;
        case 4:
            previousResult = CardPortResult[port];
            if (((u32)previousResult - 1U) < 2U) {
                CardPortResult[port] = -2;
            } else if (((u32)previousResult - 3U) < 2U) {
                CardPortResult[port] = -3;
            }
            CardPortLoaded[port] = 0;
            break;
        case 2:
        default:
            CardPortResult[port] = -4;
            CardPortLoaded[port] = 0;
            break;
        }
        break;
    case RR_CARD_PORT_PUBLISH_RESULT:
        CardPortLastResult[port] = CardPortResult[port];
        CardPortState[port] = RR_CARD_PORT_START_INFO;
        break;
    }
    return CardPortLastResult[port];
}

/* Returns the _card_format result (1 on success); callers test it. */
s32 CardFormatSlot(s32 arg0, s32 arg1) {
    return _card_format((arg0 * 0x10) + arg1);
}

/* Initial values of the globals this module owns. */
char StrCardDirPatternFmt[12] = {98, 117, 37, 49, 100, 37, 49, 100, 58, 42};
char CardFilePathFmt[384] = {98, 117, 37, 49, 100, 37, 49, 100, 58, 37, 115};
s32 CardPortLoaded[RR_CARD_PORT_COUNT];
s32 CardPortState[RR_CARD_PORT_COUNT];
s32 CardPortTimer[RR_CARD_PORT_COUNT];
s32 CardPortLastResult[RR_CARD_PORT_COUNT];
CardDirEntry CardDirEntries[15];
u8 CardRawSectorWorkBuf[RR_CARD_RAW_SCRATCH_BYTES];
int HwCardEvIoe;
int HwCardEvError;
int HwCardEvTimeout;
int HwCardEvNew;
int SwCardEvIoe;
int SwCardEvError;
int SwCardEvTimeout;
int SwCardEvNew;
int CardFileHandle;
s32 CardPortResult[RR_CARD_PORT_COUNT];
