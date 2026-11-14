#include "psyq/libc/exit.h"
#include "types.h"
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

void LoadR4Directory(void)
{
    CdlFILE file;
    s32 status;
    CdlFILE *found;
    s32 sectors;
    u32 count;
    u32 i;
    s32 base, sector, next;
    u32 *directory;
    CdFileEntry *entry;
    DebugPrintStub(NowLoadingFmt,R4BinPath,CdDirectoryBuffer);
    found=CdSearchFile(&file,(char *)R4BinPath);
    if (!found) {
        DebugPrintStub(StrFileNotFoundFmt,R4BinPath);
        PsyqExit(1);
    }
    sectors=2;
    do {
        CdControl(2,file.pos,0);
        CdRead(sectors,CdDirectoryBuffer,0x80);
        do { status=CdReadSync(1,0); } while(status>0);
    } while(status);
    DebugPrintStub(StrReadSectorFmt,sectors);
    base=CdPosToInt(file.pos);
    directory=(u32 *)CdDirectoryBuffer;
    i=0;
    count=*directory++;
    sector=base+*directory++;
    entry=CdFileTable;
    if(count>RR_R4_ENTRIES_MAX) {
        count=RR_R4_ENTRIES_MAX;
    }
    while(i<count) {
        CdIntToPos(sector,entry->location.c);
        next=base+*directory++;
        entry->sectors=next-sector;
        sector=next;
        i++;
        entry++;
    }
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
