/* BIOS bzero (A(28h)) and memset (A(2Bh)): PsyZ's ROM routines, which take
 * their PS1 time (a slow byte loop in the BIOS ROM). */
#include <string.h>
#include <psyz/system.h>
#include "rr/memory.h"

void PsyqBzero(void *buffer, s32 size);

void PsyqBzero(void *buffer, s32 size) {
    Psyz_BiosBzero(buffer, size);
}

void PsyqBzeroPs1(void *buffer, u32 hostBytes, s32 ps1Bytes);

void PsyqBzeroPs1(void *buffer, u32 hostBytes, s32 ps1Bytes) {
    memset(buffer, 0, hostBytes);
    Psyz_BiosBzeroTime(ps1Bytes);
}

void *PsyqMemset(void *destination, s32 value, u32 length) {
    return Psyz_BiosMemset(destination, value, (int)length);
}
