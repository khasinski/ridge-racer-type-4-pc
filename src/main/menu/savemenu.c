#include "rr/release.h"
#include "rr/save_slots.h"
#include "rr/save_progress.h"
/* RODATA 0x80011570 0x18 */
#include "game/data/cardini.h"
#include "game/data/savedat.h"
#include "game/data/savebld.h"
#include "game/system/modeinit.h"
#include "game/menu/savemenu.h"
#include "rr/bios_devices.h"
#include "rr/save_buffer.h"
#include "game/data/savetype.h"

#include "rr/cd_directory_buffer.h"
extern char CardHeaderTitles[][0x40];
extern char CardTitleStrings[][0x40];
extern CardHeaderBlock128 CardExtraDataTitleBuf;
extern CardHeaderBlock128 CardSlotHeaderTitles[3];
static const char CardHeaderTitlesJp[2][0x3C];
static const char CardTitleStringsJp[3][0x3C];
static const CardHeaderBlock128 CardExtraDataTitleBufJp;
static const CardHeaderBlock128 CardSlotHeaderTitlesJp[3];

/* The card file's title and its block after the icon frames. */
#define CARD_MAIN_TITLE(i) (RR_RELEASE_JP ? CardHeaderTitlesJp[i] : CardHeaderTitles[i])
#define CARD_SLOT_TITLE(i) (RR_RELEASE_JP ? CardTitleStringsJp[i] : CardTitleStrings[i])
#define CARD_MAIN_EXTRA (RR_RELEASE_JP ? &CardExtraDataTitleBufJp : &CardExtraDataTitleBuf)
#define CARD_SLOT_EXTRA(i) (RR_RELEASE_JP ? &CardSlotHeaderTitlesJp[i] : &CardSlotHeaderTitles[i])
extern short CardTransferMode;
extern short GpSaveSlotCursor;
extern char CardTargetFileName[];
extern short CardMenuOperation;
extern int GarageExtraDefaultTimes[];

int CardSaveStep(short initialize, int port) {
    int index;
    int status;
    EmblemSaveData1380 *emblemSave;

    if (initialize > 0) {
        CardSaveStepState.step = 0;
    }
    switch (CardSaveStepState.step) {
    case 0:
        CardSaveStepState.retryCount = 0;
        PsyqBzero(SaveBuffer, sizeof(GameSaveData1AA8));
        index = SaveProgress.prefix.unlocks50.extraModeUnlocked50 != 0;
        if (initialize == 1) {
            CardSaveStepState.isMainData = 1;
            if (CardTransferMode < 0x10) {
                BuildCardHeaderStatic((CardFileHeader280 *)SaveBuffer, CARD_MAIN_TITLE(index), 0x250, 0x1FC, 0x254, 0x1D8, CARD_MAIN_EXTRA);
                BuildSaveData(CardTransferMode);
            } else if (CardTransferMode & 0x10) {
                BuildCardHeaderStatic((CardFileHeader280 *)SaveBuffer, CARD_SLOT_TITLE(GpSaveSlotCursor), 0x250, 0x1FC, 0x254, 0x1D8, CARD_SLOT_EXTRA(GpSaveSlotCursor));
                BuildSaveData(CardTransferMode);
            } else if (CardTransferMode == 0x20) {
                *(CardFileImage4000 *)SaveBuffer = *(const CardFileImage4000 *)CdDirectoryBuffer;
            }
        } else if (initialize >= 2) {
            CardSaveStepState.isMainData = 0;
            if (CardTransferMode < 0x10) {
                BuildCardHeaderStatic((CardFileHeader280 *)SaveBuffer, CARD_MAIN_TITLE(index), 0x250, 0x1FC, 0x254, 0x1D8, CARD_MAIN_EXTRA);
                BuildSaveData(CardTransferMode);
            } else if (CardTransferMode & 0x10) {
                BuildCardHeaderStatic((CardFileHeader280 *)SaveBuffer, CARD_SLOT_TITLE(GpSaveSlotCursor), 0x250, 0x1FC, 0x254, 0x1D8, CARD_SLOT_EXTRA(GpSaveSlotCursor));
                BuildSaveData(CardTransferMode);
            } else if (CardTransferMode == 0x20) {
                *(CardFileImage4000 *)SaveBuffer = *(const CardFileImage4000 *)CdDirectoryBuffer;
            }
        }
        CardSaveStepState.step = 1;
        break;
    case 1:
        CardSaveStepState.step = 2;
        break;
    case 2:
        for (index = 0x1D; index >= 0; index--) {
        }
        if (CardTransferMode == 0x20) {
            status = CardWriteFile((short)port, 0, CardTargetFileName, SaveBuffer, 2, CardSaveStepState.isMainData);
        } else {
            status = CardWriteFile((short)port, 0, CardTargetFileName, SaveBuffer, 1, CardSaveStepState.isMainData);
        }
        if (status == -1) {
            if (CardSaveStepState.retryCount < 3) {
                CardSaveStepState.retryCount = (unsigned short)CardSaveStepState.retryCount + 1;
                break;
            }
            CardSaveStepState.step = 5;
        } else {
            CardSaveStepState.step = 3;
        }
        break;
    case 3:
        index = GetHwCardEvent();
        if (index != 0) {
            if (index == 1) {
                CardSaveStepState.step = 4;
                switch (CardMenuOperation) {
                case 3:
                    SaveSlotSnapshots[GpSaveSlotCursor] = SelectionState;
                    for (index = 0; index < 3; index++) {
                        SaveSlotSelections[index] = SaveSlotSnapshots[index];
                    }
                    break;
                case 6:
                case 7:
                    emblemSave = (EmblemSaveData1380 *)SaveBuffer;
                    GarageExtraDefaultTimes[GpSaveSlotCursor] = emblemSave->editFrames280;
                    break;
                }
            } else {
                if (CardSaveStepState.retryCount >= 3) {
                    CardSaveStepState.step = 5;
                } else {
                    CardSaveStepState.step = 1;
                    CardSaveStepState.retryCount = (unsigned short)CardSaveStepState.retryCount + 1;
                }
            }
        }
        break;
    case 4:
    case 5:
        break;
    }
    return CardSaveStepState.step;
}

/* Initial values of the globals this module owns. */
char CardHeaderTitles[2][0x40] = {{-126, 113, -126, 83, -127, 64, -126, 102, -126, -127, -126, -115, -126, -123, -127, 64, -126, 99, -126, -127, -126, -108, -126, -127}, {-126, 113, -126, 83, -127, 64, -126, 102, -126, -127, -126, -115, -126, -123, -127, 64, -126, 99, -126, -127, -126, -108, -126, -127, -127, 105, -126, 115, -126, -110, -126, -103, -127, 64, -126, 111, -126, -127, -126, -125, -127, 124, -126, 108, -126, -127, -126, -114, -127, 64, -126, 98, -126, -127, -126, -110, -127, 73, -127, 106}};
char CardTitleStrings[3][0x40] = {{-126, 113, -126, 83, -127, 64, -126, 99, -126, -123, -126, -125, -126, -127, -126, -116, -127, 64, -126, 99, -126, -127, -126, -108, -126, -127, -127, 64, -126, 96}, {-126, 113, -126, 83, -127, 64, -126, 99, -126, -123, -126, -125, -126, -127, -126, -116, -127, 64, -126, 99, -126, -127, -126, -108, -126, -127, -127, 64, -126, 97}, {-126, 113, -126, 83, -127, 64, -126, 99, -126, -123, -126, -125, -126, -127, -126, -116, -127, 64, -126, 99, -126, -127, -126, -108, -126, -127, -127, 64, -126, 98}};
CardSaveStepState4 CardSaveStepState;
CardHeaderBlock128 CardExtraDataTitleBuf = {.bytes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFE, 0xF, 0xFE, 0x1F, 0xFE, 0x1F, 0xFF, 0x1F, 0x3E, 0xBC, 0xF, 0x1F, 0x3E, 0xB8, 7, 0x1F, 0x3E, 0xB8, 7, 0x1F, 0xFE, 0xBF, 0xFF, 0x7F, 0xFE, 0xBF, 0xFF, 0x7F, 0xBE, 0x1F, 0, 0x1F, 0x3E, 0x3F, 0, 0x1F, 0x3E, 0x7E, 0, 0x1F, 0, 0, 0, 0, 0, 0, 0, 0, 0x80, 0xBB, 0xEF, 0, 0x80, 0xA8, 0x2A, 0, 0x80, 0xBA, 0x6A, 0, 0x80, 0xAA, 0x2A, 0, 0x80, 0xAB, 0xEA, 0, 0, 0, 0, 0, 0, 0x73, 0x77, 0, 0, 0x55, 0x52, 0, 0, 0x75, 0x72, 0, 0, 0x55, 0x52, 0, 0, 0x53, 0x52}};
CardHeaderBlock128 CardSlotHeaderTitles[3] = {{.bytes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFE, 0xF, 0xFE, 0x1F, 0xFE, 0x1F, 0xFF, 0x1F, 0x3E, 0xBC, 0xF, 0x1F, 0x3E, 0xB8, 7, 0x1F, 0x3E, 0xB8, 7, 0x1F, 0xFE, 0xBF, 0xFF, 0x7F, 0xFE, 0xBF, 0xFF, 0x7F, 0xBE, 0x1F, 0, 0x1F, 0x3E, 0x3F, 0, 0x1F, 0x3E, 0x7E, 0, 0x1F, 0, 0, 0, 0, 0, 0, 0, 0, 0x98, 0xBB, 0xB, 0, 0xA8, 0x88, 0xA, 0, 0xA8, 0x89, 0xB, 0, 0xA8, 0x88, 0xA, 0, 0x98, 0xBB, 0x3A, 0, 0, 0, 0, 0, 0x98, 0xBB, 0x83, 0xE, 0xA8, 0x92, 0x42, 0xA, 0xA8, 0x93, 0x23, 0xE, 0xA8, 0x92, 0x12, 0xA, 0x98, 0x92, 0xA, 0xA}}, {.bytes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFE, 0xF, 0xFE, 0x1F, 0xFE, 0x1F, 0xFF, 0x1F, 0x3E, 0xBC, 0xF, 0x1F, 0x3E, 0xB8, 7, 0x1F, 0x3E, 0xB8, 7, 0x1F, 0xFE, 0xBF, 0xFF, 0x7F, 0xFE, 0xBF, 0xFF, 0x7F, 0xBE, 0x1F, 0, 0x1F, 0x3E, 0x3F, 0, 0x1F, 0x3E, 0x7E, 0, 0x1F, 0, 0, 0, 0, 0, 0, 0, 0, 0x98, 0xBB, 0xB, 0, 0xA8, 0x88, 0xA, 0, 0xA8, 0x89, 0xB, 0, 0xA8, 0x88, 0xA, 0, 0x98, 0xBB, 0x3A, 0, 0, 0, 0, 0, 0x98, 0xBB, 0x83, 6, 0xA8, 0x92, 0x42, 0xA, 0xA8, 0x93, 0x23, 6, 0xA8, 0x92, 0x12, 0xA, 0x98, 0x92, 0xA, 6}}, {.bytes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFE, 0xF, 0xFE, 0x1F, 0xFE, 0x1F, 0xFF, 0x1F, 0x3E, 0xBC, 0xF, 0x1F, 0x3E, 0xB8, 7, 0x1F, 0x3E, 0xB8, 7, 0x1F, 0xFE, 0xBF, 0xFF, 0x7F, 0xFE, 0xBF, 0xFF, 0x7F, 0xBE, 0x1F, 0, 0x1F, 0x3E, 0x3F, 0, 0x1F, 0x3E, 0x7E, 0, 0x1F, 0, 0, 0, 0, 0, 0, 0, 0, 0x98, 0xBB, 0xB, 0, 0xA8, 0x88, 0xA, 0, 0xA8, 0x89, 0xB, 0, 0xA8, 0x88, 0xA, 0, 0x98, 0xBB, 0x3A, 0, 0, 0, 0, 0, 0x98, 0xBB, 0x83, 0xE, 0xA8, 0x92, 0x42, 2, 0xA8, 0x93, 0x23, 2, 0xA8, 0x92, 0x12, 2, 0x98, 0x92, 0xA, 0xE}}};
/* The Japanese release's card titles (Shift JIS) and icon blocks (SLPS-01800). */
static const char CardHeaderTitlesJp[2][0x3C] = {{-126, 113, -126, 83, -126, -52, -125, 81, -127, 91, -125, -128, -125, 102, -127, 91, -125, 94, -126, -59, -126, -73, -127, 66}, {-126, 113, -126, 83, -126, -52, -125, 81, -127, 91, -125, -128, -125, 102, -127, 91, -125, 94, -126, -59, -126, -73, -127, 66, -127, 105, -125, 75, -125, -116, -127, 91, -125, 87, -126, -55, -125, 112, -125, 98, -125, 78, -125, 125, -125, -109, -125, 74, -127, 91, -126, -86, -127, 73, -127, 106}};
static const char CardTitleStringsJp[3][0x3C] = {{-126, 113, -126, 83, -127, 64, -125, 73, -125, -118, -125, 87, -125, 105, -125, -117, -125, 88, -125, 101, -125, 98, -125, 74, -127, 91, -126, 96, -126, -52, -125, 102, -127, 91, -125, 94, -126, -59, -126, -73, -127, 66}, {-126, 113, -126, 83, -127, 64, -125, 73, -125, -118, -125, 87, -125, 105, -125, -117, -125, 88, -125, 101, -125, 98, -125, 74, -127, 91, -126, 97, -126, -52, -125, 102, -127, 91, -125, 94, -126, -59, -126, -73, -127, 66}, {-126, 113, -126, 83, -127, 64, -125, 73, -125, -118, -125, 87, -125, 105, -125, -117, -125, 88, -125, 101, -125, 98, -125, 74, -127, 91, -126, 98, -126, -52, -125, 102, -127, 91, -125, 94, -126, -59, -126, -73, -127, 66}};
static const CardHeaderBlock128 CardExtraDataTitleBufJp = {.bytes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 254, 15, 254, 31, 254, 31, 255, 31, 62, 188, 15, 31, 62, 184, 7, 31, 62, 184, 7, 31, 254, 191, 255, 127, 254, 191, 255, 127, 190, 31, 0, 31, 62, 63, 0, 31, 62, 126, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 128, 187, 239, 0, 128, 168, 42, 0, 128, 186, 106, 0, 128, 170, 42, 0, 128, 171, 234, 0, 0, 0, 0, 0, 0, 115, 119, 0, 0, 85, 82, 0, 0, 117, 114, 0, 0, 85, 82, 0, 0, 83, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
static const CardHeaderBlock128 CardSlotHeaderTitlesJp[3] = {{.bytes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 254, 15, 254, 31, 254, 31, 255, 31, 62, 188, 15, 31, 62, 184, 7, 31, 62, 184, 7, 31, 254, 191, 255, 127, 254, 191, 255, 127, 190, 31, 0, 31, 62, 63, 0, 31, 62, 126, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 184, 235, 234, 14, 8, 41, 42, 10, 56, 41, 102, 6, 32, 41, 42, 10, 56, 233, 234, 10, 0, 0, 0, 0, 152, 187, 131, 14, 168, 146, 66, 10, 168, 147, 35, 14, 168, 146, 18, 10, 152, 146, 10, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}, {.bytes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 254, 15, 254, 31, 254, 31, 255, 31, 62, 188, 15, 31, 62, 184, 7, 31, 62, 184, 7, 31, 254, 191, 255, 127, 254, 191, 255, 127, 190, 31, 0, 31, 62, 63, 0, 31, 62, 126, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 184, 235, 234, 14, 8, 41, 42, 10, 56, 41, 102, 6, 32, 41, 42, 10, 56, 233, 234, 10, 0, 0, 0, 0, 152, 187, 131, 6, 168, 146, 66, 10, 168, 147, 35, 6, 168, 146, 18, 10, 152, 146, 10, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}, {.bytes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 254, 15, 254, 31, 254, 31, 255, 31, 62, 188, 15, 31, 62, 184, 7, 31, 62, 184, 7, 31, 254, 191, 255, 127, 254, 191, 255, 127, 190, 31, 0, 31, 62, 63, 0, 31, 62, 126, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 184, 235, 234, 14, 8, 41, 42, 10, 56, 41, 102, 6, 32, 41, 42, 10, 56, 233, 234, 10, 0, 0, 0, 0, 152, 187, 131, 14, 168, 146, 66, 2, 168, 147, 35, 2, 168, 146, 18, 2, 152, 146, 10, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}};
u8 *SaveBuffer;
short CardTransferMode;
