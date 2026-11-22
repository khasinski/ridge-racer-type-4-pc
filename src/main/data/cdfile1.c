#include "psyq/libc/exit.h"
#include "types.h"
#include <limits.h>
#include <stdio.h>
#include "rr/load_cursor.h"
#include "scratchpad.h"

/* Read the two-sector R4 directory, then convert relative boundaries to entries.
 * The disc supplies count + 1 valid offsets; sector arithmetic must fit s32. */
#include "rr/cd.h"
#include "psyq/libcd/cdfile.h"
#include "psyq/libcd/dsfinish.h"
#include "psyq/libcd/s_1.h"
#include "game/data/cdfile1.h"
#include "rr/course_choices.h"
#include "rr/cd_read_request.h"
#include "rr/cd_poll.h"
#include "rr/trig.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "rr/release.h"
#include "rr/cpu_time.h"

s32 CdPosToInt(const u8 *location);
extern const char NowLoadingFmt[];
extern const char R4BinPath[];
extern const char StrFileNotFoundFmt[];
extern const char StrReadSectorFmt[];
#include "rr/cd_directory_buffer.h"
extern CdFileEntry CdFileTable[];
extern int LoadingTriangleAngle;
extern int LoadingTriangleFlip;
extern struct LoadingTriG3Packet {
    GpuDmaTag tag;
    int w4;
    short x0, y0;
    int c1;
    short x1, y1;
    int c2;
    short x2, y2;
} LoadingIndicatorTriBuf[];
extern struct LoadingTriG3Packet *ActiveLoadingTriPacket;

int GetCarFileIndex(int a0, int a1, int a2, int a3);
int GetR4FileIndex(int a0, int a1);
void DrawLoadingTriangle(void);

s32 GetCourseFileIndex(s32 arg0) {
    return arg0 * 2;
}

int GetCarFileIndex(int a0, int a1, int a2, int a3) {
    int v = ((a1 * 4 + a0) * 0x14) + a2;
    if (a3 == 0) return v + 0x12;
    return v + 0x152;
}

/* R4.BIN entry a1 of group a0. The group bases are the release's
 * (rr/release.h): PAL 0x3A0, 0x292, 0x2D9, 0x384, 0x397 for groups 0..4. */
int GetR4FileIndex(int a0, int a1) {
    if ((unsigned)a0 > 4)
        return -1;
    return a1 + GameReleaseProfile->r4GroupBase[a0];
}

/* Validate all boundaries before publishing any entries. Disc offsets are
 * sectors relative to R4.BIN, never host addresses or signed byte counts. */
s32 ParseR4Directory(const u32 *directory, size_t words, u32 fileBytes, s32 base) {
    if (!directory || words < 2 || base < 0)
        return 0;
    const u32 count = directory[0];
    const u32 fileSectors = fileBytes / 2048u + (fileBytes % 2048u != 0);
    if (count != (u32)GameReleaseProfile->r4Entries ||
        count > RR_R4_ENTRIES_MAX || words < (size_t)count + 2 ||
        directory[1] < 2)
        return 0;
    u32 previous = directory[1];
    for (u32 i = 0; i <= count; ++i) {
        const u32 next = directory[i + 1];
        if (next < previous || next > fileSectors ||
            next > (u32)(INT_MAX - base) ||
            next - previous > INT_MAX / 2048u)
            return 0;
        previous = next;
    }
    for (u32 i = 0; i < count; ++i) {
        CdIntToPos(base + (s32)directory[i + 1], CdFileTable[i].location.c);
        CdFileTable[i].sectors = (s32)(directory[i + 2] - directory[i + 1]);
    }
    return 1;
}

void LoadR4Directory(void)
{
    CdlFILE file;
    s32 status;
    CdlFILE *found;
    s32 sectors;
    s32 base;
    DebugPrintStub(NowLoadingFmt,R4BinPath,CdDirectoryBuffer);
    found=CdSearchFile(&file,(char *)R4BinPath);
    if (!found) {
        DebugPrintStub(StrFileNotFoundFmt,R4BinPath);
        PsyqExit(1);
    }
    sectors=2;
    do {
        CdControl(2,file.pos,0);
        RR_CPU_CYCLES(RR_CYCLES_R4_DIRECTORY_READ_CALL);
        CdRead(sectors,CdDirectoryBuffer,0x80);
        RR_CPU_CYCLES(RR_CYCLES_R4_DIRECTORY_SYNC_CALL);
        do { status=CdReadSync(1,0); RR_CPU_CYCLES(RR_CYCLES_CD_READ_SYNC_LOOP); } while(status>0);
    } while(status);
    RR_CPU_CYCLES(RR_CYCLES_R4_DIRECTORY_READ_END);
    DebugPrintStub(StrReadSectorFmt,sectors);
    base=CdPosToInt(file.pos);
    if (!ParseR4Directory((const u32 *)CdDirectoryBuffer, 4096 / sizeof(u32),
                          file.size, base)) {
        fprintf(stderr, "RR4: invalid R4.BIN directory\n");
        PsyqExit(1);
    }
    RR_CPU_CYCLES(RR_CYCLES_R4_DIRECTORY_ENTRY * GameReleaseProfile->r4Entries);
}

void InitCdFileDirectory(void) {
    CdSetDebug(1);
    LoadR4Directory();
    LoadCursor = CdDirectoryBuffer;
}


void DrawLoadingTriangle(void)
{
    int a, v;
    a = LoadingTriangleAngle + 0x10;
    v = LoadingTriangleFlip ^ 1;
    LoadingTriangleFlip = v;
    LoadingTriangleAngle = a;
    ActiveLoadingTriPacket = &LoadingIndicatorTriBuf[v];
    ActiveLoadingTriPacket->x0 = (rsin(a) >> 7) + 0x50;
    ActiveLoadingTriPacket->y0 = (rcos(LoadingTriangleAngle) >> 7) + RR_PAL_JP(0x80, 0x78);
    ActiveLoadingTriPacket->x1 = (rsin(LoadingTriangleAngle + 0x555) >> 7) + 0x50;
    ActiveLoadingTriPacket->y1 = (rcos(LoadingTriangleAngle + 0x555) >> 7) + RR_PAL_JP(0x80, 0x78);
    ActiveLoadingTriPacket->x2 = (rsin(LoadingTriangleAngle - 0x555) >> 7) + 0x50;
    ActiveLoadingTriPacket->y2 = (rcos(LoadingTriangleAngle - 0x555) >> 7) + RR_PAL_JP(0x80, 0x78);
    ActiveLoadingTriPacket->c1 = 0xFF00;
    ActiveLoadingTriPacket->c2 = 0xFF0000;
    ActiveLoadingTriPacket->w4 = 0x300000FF;
    RR_GPU_ADD_PRIM(RR_GPU_SCRATCH->order, ActiveLoadingTriPacket, 6);
}

/* Initial values of the globals this module owns. */
const char NowLoadingFmt[32] = {78, 111, 119, 32, 76, 111, 97, 100, 105, 110, 103, 32, 91, 37, 115, 93, 45, 62, 91, 48, 120, 37, 48, 56, 120, 93, 32, 46, 46, 46, 32};
const char R4BinPath[12] = {92, 82, 52, 46, 66, 73, 78, 59, 49};
const char StrFileNotFoundFmt[20] = {70, 105, 108, 101, 32, 110, 111, 116, 32, 102, 111, 117, 110, 100, 58, 32, 37, 115, 10};
const char StrReadSectorFmt[80] = {114, 101, 97, 100, 58, 37, 100, 115, 101, 99, 116, 111, 114, 10};
int LoadingTriangleAngle;
int LoadingTriangleFlip;
struct LoadingTriG3Packet LoadingIndicatorTriBuf[2];
struct LoadingTriG3Packet *ActiveLoadingTriPacket;
