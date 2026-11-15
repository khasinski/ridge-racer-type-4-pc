#ifndef RR_GAME_DATA_CARDINI_H
#define RR_GAME_DATA_CARDINI_H

#include "types.h"

enum {
    RR_CARD_PORT_COUNT = 2,
    RR_CARD_RAW_REPLY_BYTES = 17,
    RR_CARD_RAW_SCRATCH_BYTES = 0x30,
    RR_CARD_PORT_START_INFO = 0,
    RR_CARD_PORT_WAIT_INFO = 1,
    RR_CARD_PORT_START_READ = 2,
    RR_CARD_PORT_WAIT_READ = 3,
    RR_CARD_PORT_START_LOAD = 4,
    RR_CARD_PORT_WAIT_LOAD = 5,
    RR_CARD_PORT_PUBLISH_RESULT = 6
};
extern s32 CardPortState[RR_CARD_PORT_COUNT];
extern s32 CardPortTimer[RR_CARD_PORT_COUNT];
extern s32 CardPortLastResult[RR_CARD_PORT_COUNT];
extern s32 CardPortResult[RR_CARD_PORT_COUNT];
extern s32 CardPortLoaded[RR_CARD_PORT_COUNT];
extern u8 CardRawSectorWorkBuf[RR_CARD_RAW_SCRATCH_BYTES];

void CardSystemInit(void);
s32 GetHwCardEvent(void);
int CountCardFiles(int a, int b);
int CardFreeBlocks(int a0);
int CardHasFile(int a0, unsigned char *a1);
int CardReadFile(int port, int slot, const char *name, void *buffer, int blocks);
int CardReadFileAt(int port, int slot, const char *name, void *buffer, int bytes, int offset);
int CardWriteFile(int port, int slot, const char *name, const void *buffer, int blocks, int create);
int CardWriteFileAt(int port, int slot, const char *name, const void *buffer, int bytes, int offset);
/* Physical port0/1 and a numeric slot, encoded as port*16+slot for the BIOS. */
int CardUpdate(int port, int slot);
/* Formats port*16+slot; returns the _card_format result (1 on success). */
s32 CardFormatSlot(s32 port, s32 slot);

#endif /* RR_GAME_DATA_CARDINI_H */
