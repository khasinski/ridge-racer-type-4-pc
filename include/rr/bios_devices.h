#ifndef RR_BIOS_DEVICES_H
#define RR_BIOS_DEVICES_H

#include "rr/layout.h"

/* Raw card-sector BIOS entries: 128 bytes per sector. Status is indexed by
 * slot (port >> 4); bit0 signals a completed transfer, including error statuses. */
/* Encoded card channel is an integer selector, not an address. */
long _card_info(long channel);
long _card_load(long channel);
long _card_write(long port, long sector, const u8 *buffer);
void _new_card(void);
long _card_read(long port, long sector, u8 *buffer);
long _card_status(long slot);
void PsyqBzero(void *buffer, s32 size);
/* Adds XOR of bytes 0..126 at byte127, then makes up to eight write/read attempts.
 * Verification compares the readback XOR to the current caller checksum byte. */
s32 _card_sector_write(s32 port, s32 sector, u8 *buffer);

typedef s32 (*BiosDeviceHandler)(s32 *, s32, s32);

/* Device-table entry used by the handler replacement routine. */
typedef struct BiosDevice {
    const char *name;
    u8 pad04[0x30];
    BiosDeviceHandler handler;
    u8 pad38[0x18];
} BiosDevice;

extern BiosDeviceHandler CardFirstFileHandler;
extern char D_801143C0[];
s32 PsyqStrcmp(const char *first, const char *second);
/* Install the one-shot BIOS first-file workaround, then enumerate into directory.
 * Returns the BIOS result, or NULL when the pathname's device is not registered. */
void *firstfile(char *path, void *directory);
void *firstfile2(char *path, void *directory);
s32 _firstfile_hook(s32 *context, s32 second, s32 third);

/* These are the PAL ABI offsets; native owners retain both full pointers. */
#if defined(__mips__)
RR_ASSERT_SIZE(BiosDevice, 0x50);
RR_ASSERT_OFFSET(BiosDevice, handler, 0x34);
#endif

#endif
