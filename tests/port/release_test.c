/* The release profiles (include/rr/release.h): identification by boot file,
 * the R4.BIN group bases, and what selecting a release sets. */
#include "rr/release.h"

#include <stdio.h>
#include <string.h>

int GetR4FileIndex(int group, int index);

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

int main(void) {
    const RrReleaseProfile *pal = RrReleaseForBootFile("SCES_017.06");
    const RrReleaseProfile *jp = RrReleaseForBootFile("slps_018.00");

    CHECK(pal && pal->release == RR_RELEASE_PAL);
    CHECK(jp && jp->release == RR_RELEASE_NTSC_J);
    CHECK(!RrReleaseForBootFile("SCUS_944.03"));
    CHECK(!RrReleaseForBootFile(""));
    CHECK(!RrReleaseForBootFile(NULL));

    /* The PAL release is the default and keeps its timing choice. */
    CHECK(GameRelease == RR_RELEASE_PAL && !RR_RELEASE_JP);
    CHECK(GetR4FileIndex(1, 64) == 0x292 + 64 && GetR4FileIndex(2, 0) == 0x2D9);
    CHECK(GetR4FileIndex(0, 0) == 0x3A0 && GetR4FileIndex(5, 0) == -1);
    CHECK(RrReleaseSelect(RR_RELEASE_PAL, 0) == 0 && !VideoTimingNtsc);
    CHECK(RrReleaseSelect(RR_RELEASE_PAL, 1) == 1 && VideoTimingNtsc);
    CHECK(RR_SCREEN_BOTTOM == 0x100 && RR_PAL_JP(1, 2) == 1);

    /* The Japanese release always runs at NTSC timing, 240 lines. */
    CHECK(RrReleaseSelect(RR_RELEASE_NTSC_J, 0) == 1 && VideoTimingNtsc && RR_RELEASE_JP);
    CHECK(GameReleaseProfile->screenHeight == 240 && RR_SCREEN_BOTTOM == 0xF0);
    CHECK(GetR4FileIndex(1, 96) == 0x292 + 96 && GetR4FileIndex(2, 0) == 0x2FC);
    CHECK(GetR4FileIndex(3, 0) == 0x397 && GetR4FileIndex(4, 0) == 0x3AA && GetR4FileIndex(0, 0) == 0x3B3);

    RrReleaseSelect(RR_RELEASE_PAL, 0);
    if (failures)
        return 1;
    printf("release: ok\n");
    return 0;
}
