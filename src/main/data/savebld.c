#include "game/data/savedat.h"
#include "rr/emblem_images.h"
#include "game/race/ranking.h"
#include "rr/save_slots.h"
#include "game/data/savebld.h"
#include "game/data/savetype.h"
#include "game/data/globals.h"
extern CourseChoice4 CourseChoiceTable[4][9];
extern TimeRecord16 RaceRecords[2][8][5];
extern int EmblemEditFrames;
extern unsigned short GpSaveSlotCursor;
extern TimeRecord16 LapRecords[2][8][5];

void BuildSaveData(int mode) {
    GameSaveData1AA8 *gameSave;
    EmblemSaveData1380 *emblemSave;
    int i;
    int j;
    int k;
    u32 checksum;

    switch (mode) {
    case 0:
    case 2:
    case 4:
    default:
        for (i = 0xA0; i < 0x800; i++) {
            ((CardSaveBlock2000 *)SaveBuffer)->words[i] = 0;
        }
        gameSave = (GameSaveData1AA8 *)SaveBuffer;
        gameSave->fields.progress280 = SaveProgress;
        gameSave->fields.options2FC = Options;
        for (i = 0; i < 9; i++) {
            gameSave->fields.courseChoices33C[i] = CourseChoiceTable[1][i];
        }
        for (i = 0; i < 3; i++) {
            SaveSlotSnapshots[i].signedHalves[RR_SAVE_SLOT_CHECKSUM_HALFWORD] = 0;
            checksum = 0;
            for (j = 0; (u32)j < RR_SAVE_SLOT_HALFWORDS; j++) {
                checksum += SaveSlotSnapshots[i].signedHalves[j];
            }
            SaveSlotSnapshots[i].signedHalves[RR_SAVE_SLOT_CHECKSUM_HALFWORD] = checksum;
            gameSave->fields.slots360[i] = SaveSlotSnapshots[i];
        }
        if (mode == 3) {
            SelectionState.signedHalves[RR_SAVE_SLOT_CHECKSUM_HALFWORD] = 0;
            checksum = 0;
            for (j = 0; (u32)j < RR_SAVE_SLOT_HALFWORDS; j++) {
                checksum += SelectionState.signedHalves[j];
            }
            SelectionState.signedHalves[RR_SAVE_SLOT_CHECKSUM_HALFWORD] = checksum;
            gameSave->fields.slots360[(short)((short)GpSaveSlotCursor % 3)] = SelectionState;
        }
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 8; j++) {
                for (k = 0; k < 3; k++) {
                    gameSave->fields.bestLapTimes3E4[i][j][k] = CourseBestLapTimes[i][j][k];
                }
                for (k = 0; k < 5; k++) {
                    gameSave->fields.lapRecords4A4[i][j][k] = LapRecords[i][j][k];
                    gameSave->fields.raceRecords9A4[i][j][k] = RaceRecords[i][j][k];
                }
            }
        }
        for (i = 0; i < 0x210; i++) {
            gameSave->fields.emblemWordsEA4[i] = CustomEmblemBuffers.words[i];
        }
        checksum = 0;
        for (i = 0; (u32)i < 0x6A9U; i++) {
            checksum += gameSave->words[i];
        }
        gameSave->fields.checksum1AA4 = checksum;
        SaveDataToRelease();
        break;
    case 6:
        gameSave = (GameSaveData1AA8 *)SaveBuffer;
        for (i = 0; i < 0x210; i++) {
            gameSave->fields.emblemWordsEA4[i] = CustomEmblemBuffers.words[i];
        }
        /* fallthrough */
    case 17:
        emblemSave = (EmblemSaveData1380 *)SaveBuffer;
        emblemSave->editFrames280 = EmblemEditFrames;
        for (j = 0; j < 2; j++) {
            for (i = 0; i < 0x840; i++) {
                if (j == 0) {
                    emblemSave->decals300[0].bytes[i] = GarageActiveDecalTimA->bytes[i];
                } else {
                    emblemSave->decals300[j].bytes[i] = GarageActiveDecalTimB->bytes[i];
                }
            }
        }
        break;
    }
}

/* Initial values of the globals this module owns. */
EmblemTim840 *GarageActiveDecalTimA, *GarageActiveDecalTimB;
EmblemTim840 *GarageActiveDecalTimA;
SavedMenuSettings SaveSlotSnapshots[RR_SAVE_SLOT_COUNT];
