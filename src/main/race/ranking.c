#include "rr/emblem_images.h"
#include "rr/cpu_time.h"
#include "types.h"
#include "rr/load_cursor.h"
#include "game/data/ldscn1.h"
#include "game/gfx/poly1.h"
#include "game/menu/menutra21.h"
#include "game/demo/splash.h"
#include "rr/bios_devices.h"
#include "rr/audio_cd_controller.h"
#include "rr/driver_control.h"
#include "game/gfx/color.h"
#include "game/race/raceset.h"
#include "game/race/ranking.h"
#include "game/system/modeinit.h"
#include "game/data/globals.h"
#include "rr/time_record_types.h"
#include "rr/course_choice_types.h"
#include "rr/course_choices.h"
#include "game/sound/sndctrl.h"
#include "rr/relative_file.h"
#include "rr/track_sample_types.h"
extern u32 FrameCounter, ScreenFrame;

typedef struct {
    char name[10];
    unsigned short car;
} DefaultRecordEntry0C;

extern int DefaultCourseTimes[2][8];
extern DefaultRecordEntry0C DefaultRecordEntries[5];
extern TimeRecord16 RaceRecords[2][8][5];
extern TimeRecord16 LapRecords[2][8][5];
extern CourseChoice4 CourseChoiceTable[4][9];
extern unsigned short DefaultCourseChoiceGlyphs[];
#include "rr/cd_directory_buffer.h"
extern char StrTitle0Log[];
extern s32 VSyncThreshold;
extern s32 ModeFrame;
extern s16 ModeIndex;
extern s16 ModeGroup;
extern s8 LoadState;
extern s16 SubmodeState;
extern s16 CardMenuOperation;

void InitDefaultTimeRecords(void) {
    RR_CPU_CYCLES(RR_CYCLES_INIT_DEFAULT_TIME_RECORDS);

    int i, j, k;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 8; j++) {
            int c = 0x2FD;
            CourseBestLapRecords[j] = DefaultCourseTimes[0][j];
            CourseBestTotalTimes[j] = (DefaultCourseTimes[0][j] - 0x2FD) * CourseLapCounts[1][j] + (CourseLapCounts[1][j] * 1000 + c);
            CourseBestLap[i][j] = DefaultCourseTimes[1][j];
            CourseBestTotal[i][j] = (DefaultCourseTimes[1][j] - 0x2FD) * CourseLapCounts[0][j] + (CourseLapCounts[0][j] * 1000 + c);
            for (k = 0; k < 3; k++) {
                CourseBestLapTimes[i][j][k] = DefaultCourseTimes[1][j];
            }
        }
    }

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 8; j++) {
            for (k = 0; k < 5; k++) {
                *(int *)&LapRecords[i][j][k].name[0] = *(int *)&DefaultRecordEntries[k].name[0];
                *(int *)&LapRecords[i][j][k].name[4] = *(int *)&DefaultRecordEntries[k].name[4];
                *(int *)&RaceRecords[i][j][k].name[0] = *(int *)&DefaultRecordEntries[k].name[0];
                *(int *)&RaceRecords[i][j][k].name[4] = *(int *)&DefaultRecordEntries[k].name[4];
                LapRecords[i][j][k].time = CourseBestLap[i][j] + k * 3000;
                {
                    int t = k * 3000;
                    RaceRecords[i][j][k].time = CourseBestTotal[i][j] + t * CourseLapCounts[0][j];
                }
                LapRecords[i][j][k].car = DefaultRecordEntries[k].car;
                RaceRecords[i][j][k].car = DefaultRecordEntries[k].car;
            }
        }
    }
}

void InitCourseChoices(void)
{
    int i, j;
    PsyqBzero(CourseChoiceTable, 0x90);
    /* The PS1 then clears 0x90 bytes from +0x24 (0x800ABE2C..0x800ABEBB):
     * the rest of the table and the nine words the linker placed after it,
     * which therefore start from zero once the frame environment is set up. */
    PsyqBzero((char *)CourseChoiceTable + 0x24, sizeof(CourseChoiceTable) - 0x24);
    RR_CPU_CYCLES(0x24 * RR_CYCLES_BIOS_FILL_BYTE); // the PS1's bzero past the table
    BackColorR = 0;               /* 0x800ABE98 */
    RelFileOffsetPtr7 = NULL;
    TrackCollisionGridPtr = NULL;
    TrackNodes = NULL;
    RelFileSection05 = NULL;
    FrameCounter = 0;             /* 0x800ABEAC */
    ScreenFrame = 0;
    TrackSectionBoundsPtr = NULL;
    RelFileOffsetPtr12 = NULL;    /* 0x800ABEB8 */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 9; j++) {
            if (j == 8) {
                CourseChoiceTable[i][j].glyph = -1;
            } else {
                CourseChoiceTable[i][j].glyph = DefaultCourseChoiceGlyphs[j];
                CourseChoiceTable[i][j].first = (j / 2) + (j < 4);
            }
        }
    }
}

void BackupSaveData(void) {
    int i;
    for (i = 0; i < 0x840; i++) {
        CustomEmblemBuffers.bytes[i] = SaveDataWork.file.bytes[i];
    }
}

void RestoreSaveData(void) {
    int i;
    for (i = 0; i < 0x840; i++) {
        SaveDataWork.file.bytes[i] = CustomEmblemBuffers.bytes[i];
    }
}

void MenuSceneNoop(void) {
}

void SplashModeEnter(void) {
    int reserved[6];
    SetDispMask(0);
    DebugPrintStub(StrTitle0Log);
    LoadCursor = CdDirectoryBuffer;
    InitSplashDispEnv();
    CardMenuOperation = 0;
    LoadSaveFromCardStep(0);
    VSyncThreshold = 0x80;
    ModeFrame = 0;
    SubmodeState = 0;
    LoadState = 0;
    AttractCycleCounter = 0;
    PendingModeId = 0x203;
    ModeGroup = 1;
    ModeIndex = 2;
    SubmodeState = 0;
}

void SplashModeUpdate(void) {
    s32 temp_v0;

    temp_v0 = LoadSplashAssetsStep();
    if (temp_v0 == 1) {
        ModeFrame += 1;
        if (SplashLogoUpdate(LoadSaveFromCardStep(1) == 1) == temp_v0) {
            SetCdMixMode(Options.mono32);
            SubmodeState = 0;
            ModeGroup = (s16) ((u16) PendingModeId >> 8);
            ModeIndex = PendingModeId & 0xFF;
        }
    }
}


void DrawSmallFontString(s32 x, s32 y, const char *text, s32 color){ int a[6];; }

/* Initial values of the globals this module owns. */
char StrTitle0Log[36] = {116, 105, 116, 108, 101, 48, 10};
int DefaultCourseTimes[2][8] = {{85765, 88765, 86765, 78765, 112765, 88765, 41765, 40765}, {84765, 92765, 90765, 94765, 111765, 102765, 49765, 60765}};
DefaultRecordEntry0C DefaultRecordEntries[5] = {{.name = {69, 78, 75, 73, 32, 32, 32, 32}, .car = 0x1E0}, {.name = {83, 72, 73, 78, 74, 73, 32, 32}, .car = 0x190}, {.name = {82, 79, 66, 69, 82, 84, 32, 32}, .car = 0x28}, {.name = {83, 79, 80, 72, 73, 69, 32, 32}, .car = 0x118}, {.name = {82, 69, 73, 75, 79, 32, 32, 32}, .car = 0xC7}};
unsigned short DefaultCourseChoiceGlyphs[8] = {0x27, 0x4F, 0x77, 0x9F, 0xC7, 0xEF, 0x117, 0x13F};
u8 CourseLapCounts[2][8] = {{3, 3, 3, 3, 3, 3, 3, 6}, {2, 3, 3, 3, 2, 3, 3, 6}};
EmblemTimImage840 SaveDataWork;
/* 0x800ABE08: four groups of nine choices (InitCourseChoices fills all
 * four). Garage's GaragePackedChoiceGlyph (0x800ABE2C) is [1][0].glyph. */
CourseChoice4 CourseChoiceTable[4][9];
