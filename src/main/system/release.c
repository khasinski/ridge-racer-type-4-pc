#include "rr/release.h"

#include <ctype.h>

/* rr/release.h. The values are measured on the two discs
 * (tools/port/region_diff.py, docs/REGIONS.md). */
static const RrReleaseProfile ReleaseProfiles[RR_RELEASE_COUNT] = {
    [RR_RELEASE_PAL] = {
        .release = RR_RELEASE_PAL,
        .bootFile = "SCES_017.06",
        .productCode = "SCES-01706",
        .name = "European PAL release",
        .ntsc = 0,
        .screenHeight = 256,
        .r4GroupBase = {0x3A0, 0x292, 0x2D9, 0x384, 0x397},
        .r4Entries = 950,
    },
    [RR_RELEASE_NTSC_J] = {
        .release = RR_RELEASE_NTSC_J,
        .bootFile = "SLPS_018.00",
        .productCode = "SLPS-01800",
        .name = "Japanese release",
        .ntsc = 1,
        .screenHeight = 240,
        .r4GroupBase = {0x3B3, 0x292, 0x2FC, 0x397, 0x3AA},
        .r4Entries = 969,
    },
};

RrRelease GameRelease = RR_RELEASE_PAL;
const RrReleaseProfile *GameReleaseProfile = &ReleaseProfiles[RR_RELEASE_PAL];

const RrReleaseProfile *RrReleaseProfileOf(RrRelease release) {
    if ((unsigned)release >= RR_RELEASE_COUNT)
        return NULL;
    return &ReleaseProfiles[release];
}

static int SameFileName(const char *a, const char *b) {
    for (; *a && *b; a++, b++)
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b))
            return 0;
    return *a == *b;
}

const RrReleaseProfile *RrReleaseForBootFile(const char *bootFile) {
    if (!bootFile || !*bootFile)
        return NULL;
    for (int i = 0; i < RR_RELEASE_COUNT; i++)
        if (SameFileName(ReleaseProfiles[i].bootFile, bootFile))
            return &ReleaseProfiles[i];
    return NULL;
}

s32 RrReleaseSelect(RrRelease release, s32 ntscTiming) {
    const RrReleaseProfile *profile = RrReleaseProfileOf(release);

    if (!profile)
        profile = &ReleaseProfiles[RR_RELEASE_PAL];
    GameRelease = profile->release;
    GameReleaseProfile = profile;
    VideoTimingNtsc = profile->ntsc || ntscTiming;
    return VideoTimingNtsc;
}
