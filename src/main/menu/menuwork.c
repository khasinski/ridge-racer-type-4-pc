#include "rr/cpu_time.h"
#include "game/system/pad.h"
#include "game/data/menuvars.h"
#include "game/ui/uipart1.h"
#include "game/sound/sndvab.h"
#include "rr/save_slots.h"
#include "game/car/carbound.h"
#include "game/car/carwheel.h"
#include "rr/menu_arena_state.h"
#include "game/data/cdfile2.h"
#include "game/camera/campath.h"
#include "rr/load_cursor.h"
#include "types.h"
#include "rr/race_types.h"
#include "game/system/modeslot.h"
#include "scratchpad.h"
#include "game/system/frmtask.h"
#include "game/gfx/meshquad.h"
#include "game/menu/menuwork.h"
#include "rr/menu_background.h"
#include "game/data/globals.h"
#include "game/data/selection.h"
#include "game/data/screenframe.h"
#include "game/ui/uitext.h"
#include "rr/ui.h"
#include "rr/badge_row_types.h"
#include "rr/badge_rows.h"
#include "rr/menu_state_types.h"
#include "game/race/racesamp.h"
#include "game/ui/uiwin.h"
#include "game/sound/cdstart.h"
#include "game/math/rotmat.h"
#include "game/camera/viewcam.h"
#include "game/menu/menu8.h"
#include "game/race/racegrid.h"
#include "game/menu/mnrows.h"
#include "game/sound/fxplay.h"
#include "game/data/ldscn1.h"
#include "game/ui/uidraw.h"

extern unsigned char NameCharset[];

extern unsigned char ProgressBitMasks[];

extern RaceVehicle PlayerCars[2];
extern s32 VSyncThreshold;
extern unsigned char GpStageShowcaseModes[];

void GetNameChar(unsigned char *a0, int a1);
void StepNameChar(unsigned char *a0);
void CarPreviewLoadModeEnter(void);
void CarPreviewLoadModeUpdate(void);

void GetNameChar(unsigned char *a0, int a1) {
    *a0 = NameCharset[a1];
}

void StepNameChar(unsigned char *a0) {
    int i;
    for (i = 0; i < 0x29; i++) {
        if (*a0 == NameCharset[i]) break;
    }
    StepMenuValue(2, 4, (char *)&i, 0x29, 0);
    *a0 = NameCharset[i];
}

void CarPreviewLoadModeEnter(void) {
    ModeFrame = 0;
    ResetFrmTaskState();
    UpdateScreenFade(0x1000C);
    ModeGroup = 0;
    ModeIndex = 6;
    SubmodeState = 0;
}

void CarPreviewLoadModeUpdate(void) {
}

void SelectTeamCarGroup(void) {
    int v;
    if (SelectionState.bytes[0x1E] == 0x12) {
        int i = SelectionState.bytes[0x1C] * 2 + (SelectionState.bytes[0x1D] >> 1);
        v = ProgressBitMasks[i];
    } else {
        v = 1;
    }
    if (v < 0) {
        v = 0;
    } else if (v >= 9) {
        v = 8;
    }
    SelectionState.bytes[0x12] = v;
    if (v == 1) {
        SelectionState.bytes[0x13] = SelectionState.bytes[0x18];
    }
}

void UpdateUnlockedCarMask(void) {
    int i = SelectionState.bytes[0x1C] * 2 + (SelectionState.bytes[0x1D] >> 1);
    int v = ProgressBitMasks[i];
    if (v < 0) {
        v = 0;
    } else if (v >= 9) {
        v = 8;
    }
    SelectionState.fields.reserved10[3] = SaveProgress.unlocked60[v];
}

void InitCarPreviewScene(void) {
    SetupRenderer(5);
    ResetScreenCounters();
    SetupMenuCarCamera(1);
    ResetCarState(PlayerCars);
    PlayerCars[0].position20.words[2] = 0x134;
    PlayerCars[0].position20.words[1] = 0xA;
    PlayerCars[0].rot54 = 0xA00;
    InitPreviewImageRows();
    InitUiRow(0, 0x18, 0x58, 0x44, 0x20, 0, 0, 1, 0, 0x53);
    InitUiRow(1, 0x74, 0x58, 0x1C, 0x20, 5, 0, 1, 0, 0x37);
    InitUiRow(2, 0x74, 0x80, 0x1C, 0x20, 5, 0, 1, 0, 0x38);
    ScreenFrame = 0;
    ModeFrame = 0;
    ActiveSceneId = 0x122;
    VSyncThreshold = 0x80;
    InitSelectionRows();
}

void CarPreviewModeUpdate(void) {
    ModeFrame = (ModeFrame + 1U);
    SetupViewport(6);
    if (SubmodeState == 0) {
        if (StepCarShowcaseScene(GpStageShowcaseModes[((MenuSettings2C *)SelectionState.bytes)->phase17 / 2]) != 0) {
            FadeCountdown = 0x40;
            CdAudioFade(0, 0x3C);
            SubmodeState = 1;
            InitCarPreviewScene();
            ImageRows->f1 = 0;
        }
    } else {
        BuildCameraMatrices();
        ScaleMatrixRow1(2, (MatrixWords32 *)SCRATCH_ROT_MATRIX);
        UpdateCarWorldTransform(PlayerCars);
        { PreviewRgb3 black = {0, 0, 0};
          DrawPreviewCar(PlayerCars, black, 0x100, 1); }
        UpdateImageRow(0);
        if (ImageRows->f1 == 2) {
            ModeGroup = 1;
            ModeIndex = 0x22;
            SubmodeState = 0;
            SeqStartTrack(0);
        }
    }
    UpdateScreenFade(0x10028);
    SetMenuFrameOptions(0, 0, 0);
}

void GarageCarPreviewModeUpdate(void)
{
    RR_CPU_CYCLES(RR_CYCLES_GARAGE_CAR_PREVIEW);
    int rowStatus, ready, optionCount, rowState, row;
    void *loadedArena;
    UiRow24 *rows;
    unsigned char *selection;
    PreviewColor color;
    int brightness;

    ModeFrame = (ModeFrame + 1U);
    UpdateScreenFade(0x10022);
    ready = LoadCarRangeStep(1, 3);
    if (ready == 1) {
        loadedArena = LoadCursor;
        MenuArenas.first = loadedArena;
        switch (SubmodeState) {
        case 0:
            rowStatus = (u8)UiRows->byte14;
            if (rowStatus == 2) {
                SetMenuFrameOptions(1, 1, 0);
                UiRows->cells[0] = rowStatus;
                if (MenuReturnSubmodeIndex != 0) {
                    PlaySfx(1);
                    UiRows->cells[0] = ready;
                    UiRows[1].byte14 = 0;
                    UiRows[2].byte14 = 0;
                    SubmodeState = ready;
                }
            }
            break;
        case 1:
            rows = UiRows;
            rowState = (u8)rows[1].byte14;
            optionCount = 2;
            if (rowState == optionCount) {
                selection = SelectionRows.bytes;
                rows[selection[3] + 1].cells[0] = 0;
                StepMenuValue(0, 1, (char *)&selection[3], optionCount, 0);
                UiRows[selection[3] + 1].cells[0] = optionCount;
                if (MenuReturnSubmodeIndex != 0) {
                    PlaySfx(1);
                    UiRows[selection[3] + 1].cells[0] = ready;
                    for (row = 0; row < 3; row++)
                        UiRows[row].byte14 = 3;
                }
            } else if (rowState == 5) {
                SubmodeState = optionCount;
            }
            break;
        case 2:
            ModeFrame = 0;
            ImageRows->f1 = 3;
            BadgeRows->state10 = 4;
            ResetScreenCounters();
            {
                MenuSettings2C *settings = (MenuSettings2C *)SelectionState.bytes;
                settings->copies[0] = SelectionRows.choice;
                if (settings->phase17 <= 0)
                    settings->mode1A = 2;
            }
            InitMotionController();
            ModeGroup = ready;
            ModeIndex = 0xB;
            SubmodeState = 0;
            break;
        default:
            break;
        }
    }
    SetupMenuCarCamera(1);
    SetupViewport(6);
    if (ImageRows->f1 > 0) {
        RaceVehicle *vehicle = PlayerCars;
        color.channels.r = 0x80;
        color.channels.g = 0x80;
        color.channels.b = 0x80;
        vehicle->rot54 = ((vehicle->rot54 + 8U) & 0xFFFU);
        if (ImageRows->f1 == 2)
            brightness = 0x100;
        else
            brightness = (ImageRows->f2 << 8) / 30;
        BadgeRows->half02 = 1;
        DrawUiRow(0);
        DrawUiRow(1);
        DrawUiRow(2);
        DrawSelectionIconGrid(0);
        UpdateCarWorldTransform(PlayerCars);
        DrawPreviewCar(PlayerCars, color.argument, brightness, 1);
    }
    UpdateBadgeRowAnim(0);
    UpdateImageRow(0);
}

/* Initial values of the globals this module owns. */
unsigned char NameCharset[44] = {0x20, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x21, 0x3F, 0x26, 0x40};
unsigned char ProgressBitMasks[8] = {7, 5, 0, 2, 3, 8, 4, 6};
unsigned char GpStageShowcaseModes[4] = {0, 0, 1, 2};
