#include <string.h>
#include "rr/release.h"
#include "rr/emblem_images.h"
#include "game/race/ranking.h"
#include "rr/save_slots.h"
#include "game/gfx/color.h"
#include "psyq/libc/printf.h"
#include "game/data/savedat.h"
#include "game/data/savetype.h"
#include "game/data/globals.h"
#include "rr/menu_selection.h"
#include "rr/overlay662_frame_step.h"
#include "rr/overlay668_playback_step.h"
#include "game/race/raceset.h"
#include "rr/course_choices.h"
#include "rr/audio_loop.h"
#include "rr/overlay667_course_initializer.h"
#include "rr/string_tail_copy.h"

extern TimeRecord16 RaceRecords[2][8][5];
extern short GpSaveSlotCursor;
extern TimeRecord16 LapRecords[2][8][5];
extern CourseChoice4 LinkCarChoiceTable[2][2][9];
/* VS mode's player names and car choices are the versus overlay's objects
 * (PS1 0x80118420 and 0x80118458): a car set loaded from the card in the VS
 * preview goes straight into them. */
extern char ovl661_player_names[24];
extern CourseChoice4 ovl661_car_choices[2][2][9];
extern int GarageExtraDefaultTimes[];
extern char StrFmtPercentS[];
extern GpuRectangle SavedEmblemClutRect;
extern char StrCardMagicCrd0[];
extern GpuRectangle SavedEmblemPixelRect;


enum { SAVE_OPTIONS_END = 0x33C, SAVE_PAL_WORDS = 0x6A9, SAVE_JP_WORDS = 0x6A8 };

void SaveDataToRelease(void) {
    u32 *words = ((GameSaveData1AA8 *)SaveBuffer)->words;
    u8 *bytes = (u8 *)words;
    u32 checksum = 0;
    int i;

    if (!RR_RELEASE_JP)
        return;
    memmove(bytes + SAVE_OPTIONS_END - 4, bytes + SAVE_OPTIONS_END, SAVE_PAL_WORDS * 4 - SAVE_OPTIONS_END);
    for (i = 0; i < SAVE_JP_WORDS; i++)
        checksum += words[i];
    words[SAVE_JP_WORDS] = checksum;
    words[SAVE_PAL_WORDS] = 0;
}

/* A Japanese save is moved to the PAL layout with the current language
 * word inserted and a PAL checksum that holds only if its own did. */
void SaveDataFromRelease(void) {
    GameSaveData1AA8 *gameSave = (GameSaveData1AA8 *)SaveBuffer;
    u32 *words = gameSave->words;
    u8 *bytes = (u8 *)words;
    u32 checksum = 0;
    s32 valid;
    u32 language;
    int i;

    if (!RR_RELEASE_JP)
        return;
    for (i = 0; i < SAVE_JP_WORDS; i++)
        checksum += words[i];
    valid = checksum == words[SAVE_JP_WORDS];
    memmove(bytes + SAVE_OPTIONS_END, bytes + SAVE_OPTIONS_END - 4, SAVE_JP_WORDS * 4 - (SAVE_OPTIONS_END - 4));
    language = (u32)Options.storyBranch3C;
    memcpy(bytes + SAVE_OPTIONS_END - 4, &language, 4);
    checksum = 0;
    for (i = 0; i < SAVE_PAL_WORDS; i++)
        checksum += words[i];
    gameSave->fields.checksum1AA4 = valid ? checksum : checksum + 1;
}

int ApplySaveData(int mode) {
    int result = 0;
    GameSaveData1AA8 *gameSave;
    EmblemSaveData1380 *emblemSave;
    u32 checksum;
    int i;
    int j;
    int k;
    int headerFrames;
    int emblemFrames;

    switch (mode) {
    case 0:
    case 1:
    case 0x30:
    default:
        SaveDataFromRelease();
        gameSave = (GameSaveData1AA8 *)SaveBuffer;
        checksum = 0;
        for (i = 0; (u32)i < 0x6A9U; i++) {
            checksum += gameSave->words[i];
        }
        if (gameSave->fields.checksum1AA4 == checksum) {
            SaveProgress = gameSave->fields.progress280;
            Options = gameSave->fields.options2FC;
            for (i = 0; i < 3; i++) {
                if (mode == 0x30) {
                    SaveSlotSelections[i] = gameSave->fields.slots360[i];
                } else {
                    SaveSlotSnapshots[i] = gameSave->fields.slots360[i];
                }
            }
            for (i = 0; i < 9; i++) {
                CourseChoiceTable[1][i] = gameSave->fields.courseChoices33C[i];
            }
            for (i = 0; i < 2; i++) {
                for (j = 0; j < 8; j++) {
                    for (k = 0; k < 3; k++) {
                        CourseBestLapTimes[i][j][k] = gameSave->fields.bestLapTimes3E4[i][j][k];
                    }
                    for (k = 0; k < 5; k++) {
                        LapRecords[i][j][k] = gameSave->fields.lapRecords4A4[i][j][k];
                        RaceRecords[i][j][k] = gameSave->fields.raceRecords9A4[i][j][k];
                    }
                    CourseBestLap[i][j] = LapRecords[i][j][0].time;
                    CourseBestTotal[i][j] = RaceRecords[i][j][0].time;
                }
            }
            for (i = 0; i < 0x210; i++) {
                CustomEmblemBuffers.words[i] = gameSave->fields.emblemWordsEA4[i];
            }
            ApplySoundVolumes();
            result = 1;
        }
        break;
    case 2:
        SaveDataFromRelease();
        gameSave = (GameSaveData1AA8 *)SaveBuffer;
        checksum = 0;
        for (i = 0; (u32)i < 0x6A9U; i++) {
            checksum += gameSave->words[i];
        }
        if (gameSave->fields.checksum1AA4 == checksum) {
            for (i = 0; i < 9; i++) {
                if (GameModeType == 2 || GameModeType == 3) {
                    ovl661_car_choices[ActivePlayerIndex & 1][1][i] = gameSave->fields.courseChoices33C[i];
                    CopyStringClearTail(ovl661_player_names + (ActivePlayerIndex & 1) * 12, gameSave->fields.progress280.prefix.progress.prefix00.fields.name);
                } else if (GameModeType == 4) {
                    LinkCarChoiceTable[ActivePlayerIndex & 1][1][i] = gameSave->fields.courseChoices33C[i];
                    CopyStringClearTail(LinkPlayerNames[ActivePlayerIndex], gameSave->fields.progress280.prefix.progress.prefix00.fields.name);
                }
            }
            for (i = 0; i < 0x210; i++) {
                *(u32 *)(CustomEmblemBuffers.bytes + (ActivePlayerIndex & 1) * sizeof(EmblemTim840) + i * sizeof(u32)) = gameSave->fields.emblemWordsEA4[i];
            }
            result = 1;
        }
        break;
    case 0x10:
        headerFrames = ((CardSaveBlock2000 *)SaveBuffer)->words[0];
        GarageExtraDefaultTimes[GpSaveSlotCursor] = headerFrames;
        result = 1;
        break;
    case 0x11:
        emblemFrames = (emblemSave = (EmblemSaveData1380 *)SaveBuffer)->editFrames280;
        EmblemEditFrames = GarageExtraDefaultTimes[GpSaveSlotCursor] = emblemFrames;
        for (j = 0; j < 2; j++) {
            for (i = 0; i < 0x840; i++) {
                if (j == 0) {
                    GarageSavedDecalTimA->bytes[i] = emblemSave->decals300[0].bytes[i];
                } else {
                    GarageSavedDecalTimB->bytes[i] = emblemSave->decals300[j].bytes[i];
                }
            }
        }
        result = 1;
        break;
    }
    return result;
}

void BuildCardHeaderAnim(CardFileHeader280 *header, const char *title,
    s32 paletteX, s32 paletteY, s32 imageX, s32 imageY) {
    int frameIndex;
    /* GPU rectangle coordinates wrap in their signed 16-bit fields. */
    s16 frameX;
    header->magic00[0] = 0x53;
    header->magic00[1] = 0x43;
    header->iconMode02 = 0x13;
    header->blockCount03 = 1;
    PsyqSprintf((s8 *)header->title04, (const s8 *)StrFmtPercentS, title);
    SavedEmblemClutRect.rect.x = paletteX;
    SavedEmblemClutRect.rect.w = 0x10;
    SavedEmblemClutRect.rect.y = paletteY;
    SavedEmblemClutRect.rect.h = 1;
    StoreImage(&SavedEmblemClutRect, header->palette60);
    DrawSync(0);
    frameX = imageX;
    frameIndex = 0;
    do {
        SavedEmblemClutRect.rect.x = frameX;
        SavedEmblemClutRect.rect.y = imageY;
        SavedEmblemClutRect.rect.w = 4;
        SavedEmblemClutRect.rect.h = 0x10;
        StoreImage(&SavedEmblemClutRect, &header->payload80.frames[frameIndex]);
        DrawSync(0);
        frameIndex++;
        frameX = frameX + 4;
    } while (frameIndex < 3);
}

void BuildCardHeaderStatic(CardFileHeader280 *header, const char *title,
    s32 paletteX, s32 paletteY, s32 imageX, s32 imageY,
    const CardHeaderBlock128 *extra) {
    int frameIndex;
    header->magic00[0] = 0x53;
    header->magic00[1] = 0x43;
    /* The Japanese release's icon is animated: three frames. */
    s32 frames = RR_PAL_JP(1, 3);
    header->iconMode02 = RR_PAL_JP(0x11, 0x13);
    header->blockCount03 = 1;
    PsyqSprintf((s8 *)header->title04, (const s8 *)StrFmtPercentS, title);
    header->flags50[0] = 1;
    header->flags50[1] = 0;
    PsyqSprintf((s8 *)header->applicationTag52, (const s8 *)StrFmtPercentS, StrCardMagicCrd0);
    SavedEmblemPixelRect.rect.x = paletteX;
    SavedEmblemPixelRect.rect.w = 0x10;
    SavedEmblemPixelRect.rect.y = paletteY;
    SavedEmblemPixelRect.rect.h = 1;
    StoreImage(&SavedEmblemPixelRect, header->palette60);
    DrawSync(0);
    frameIndex = 0;
    while (frameIndex < frames) {
        SavedEmblemPixelRect.rect.x = imageX;
        SavedEmblemPixelRect.rect.y = imageY;
        SavedEmblemPixelRect.rect.w = 4;
        SavedEmblemPixelRect.rect.h = 0x10;
        StoreImage(&SavedEmblemPixelRect, &header->payload80.frames[frameIndex]);
        DrawSync(0);
        frameIndex++;
        imageX = (u32)imageX + 4U;
    }
    if (RR_RELEASE_JP) {
        header->payload80.frames[3] = *extra;
        return;
    }
    header->payload80.single.extra[0] = *extra;
    header->payload80.single.extra[1] = *extra;
    header->payload80.single.extra[2] = *extra;
}

/* Initial values of the globals this module owns. */
char StrFmtPercentS[4] = {37, 115};
char StrCardMagicCrd0[60] = {67, 82, 68, 48};
EmblemTim840 *GarageSavedDecalTimA, *GarageSavedDecalTimB;
EmblemTim840 *GarageSavedDecalTimA;
GpuRectangle SavedEmblemClutRect;
GpuRectangle SavedEmblemPixelRect;
