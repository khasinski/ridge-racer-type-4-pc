#include "rr/release.h"
#include "game/system/frmtask.h"
#include "game/sound/cdpos.h"
#include "rr/runtime_settings.h"
#include "rr/menu_selection.h"
#include "rr/emblem_images.h"
#include "rr/save_slots.h"
#include "game/car/carstat.h"
#include "game/data/cdfile2.h"
#include "rr/frame_environment.h"
#include "rr/load_cursor.h"
#include "rr/frame_loop.h"
#include "rr/save_progress.h"
#include "game/car/car.h"
#include "game/car/carstat.h"
#include "game/data/cardini.h"
#include "game/data/cdfile1.h"
#include "game/data/trkld8.h"
#include "game/gfx/fadespr.h"
#include "game/gfx/model.h"
#include "game/menu/menu8.h"
#include "game/race/racesamp.h"
#include "game/race/ranking.h"
#include "psyq/libgpu/gpuenv.h"
#include "psyq/libsio/siostart.h"
#include "psyq/libspu/sinit2.h"
#include "game/data/ldscn1.h"
#include "game/menu/menusave.h"
#include "game/system/frmenv.h"
#include "rr/audio_cd_controller.h"
#include "rr/library.h"
#include "rr/driver_control.h"
#include "rr/bios_devices.h"
#include "game/data/selection.h"
#include "rr/string_tail_copy.h"
#include "game/menu/mnrows.h"

extern int InitGeom();

#include "rr/cd_directory_buffer.h"
extern char StrTotalInitLog[];
extern char DefaultPlayerName[];
extern char TeamNameDigRacingTeam[];
extern char TeamAbbrevDRT[];
extern char TeamNameMicroMouseMappy[];
extern char TeamAbbrevMicroMouseMappy[];
extern char StrPacRacingClub[];
extern char TeamAbbrevPRC[];
extern char TeamNameRacingTeamSolvalou[];
extern char TeamAbbrevRTS[];
extern char TeamNameAssoluto[];
extern char TeamNameLizard[];
extern char StrTerrazi[];
extern char StrAgeSolo[];
extern RaceVehicle PlayerCars[2];
extern int MirrorRenderFlag;
extern short LinkHostFlag;
extern char SaveHeaderPlayerNameBuf[];
extern char *TeamStringPtrTable[8];
extern char *RivalTeamNamePtrTable[4];
extern int DisplayEnvOffsetX;
extern int DisplayEnvOffsetY;
extern int DialogueSceneMode;
#include "rr/menu_arena_base.h"
extern short GameModeType;
extern void SetGeomOffset(s32,s32);
extern void SetGeomScreen(s32);

void SetupDrawDispEnvs(s32 r,s32 g,s16 b,u32 flags);

void SystemInit(void)
{
    int i;
    int k;
    int m;

    DebugPrintStub(StrTotalInitLog);
    /* The Japanese release sets MODE_NTSC here (the PAL one in MainLoop). */
    if (RR_RELEASE_JP)
        SetVideoMode(0);
    CdInit();
    InitCdFileDirectory();
    SsInit();
    SoundSystemInit();
    CdAudioInit();
    ResetGraph(0);
    SetGraphDebug(0);
    SetDispMask(0);
    InitGeom();
    CardSystemInit(1);
    InitGamePads();
    InitOptionsDefaults();
    ApplyOptionsPadButtonMaps();
    AddSIO();
    _sio_control(1, 1, 0);
    SetupRenderer(5);
    VSyncThreshold = 0x80;
    DisplayEnvOffsetX = 0;
    DisplayEnvOffsetY = 0;
    DrawSubOtFlag = 0;
    NumLaps = 2;
    MirrorRenderFlag = 0;
    PsyqBzero(&SaveProgress, sizeof(SaveProgress));
    PsyqBzero(SelectionRows.bytes, 0x20);
    PsyqBzero(SelectionState.signedBytes, 0x2C);
    SelectionState.signedBytes[0x17] = -1;
    PsyqBzero(RankEntries, 8 * sizeof(*RankEntries));
    i = 7;
    do {
        RankTable[i] = &RankEntries[i];
        i--;
    } while (i >= 0);
    PsyqBzero(&PlayerCars[0], sizeof(PlayerCars[0]));
    PsyqBzero(&PlayerCars[1], sizeof(PlayerCars[1]));
    UnlockNoticeMask = 0;
    for (i = 0; i < 4; i++) {
        SaveProgress.history6C[i].selection = 5;
        SaveProgress.history6C[i].variant = 5;
        SaveProgress.history6C[i].parameter = 0;
    }
    LinkHostFlag = -1;
    CopyStringClearTail(SaveProgress.prefix.progress.prefix00.fields.name, DefaultPlayerName);
    CopyStringClearTail(SaveHeaderPlayerNameBuf, SaveProgress.prefix.progress.prefix00.fields.name);
    CopyStringClearTail(SaveHeaderPlayerNameBuf + 0xC, SaveProgress.prefix.progress.prefix00.fields.name);
    CopyStringClearTail(SaveHeaderPlayerNameBuf + 0x18, SaveProgress.prefix.progress.prefix00.fields.name);
    CopyStringClearTail(SaveHeaderPlayerNameBuf + 0x24, SaveProgress.prefix.progress.prefix00.fields.name);
    TeamStringPtrTable[0] = TeamNameDigRacingTeam;
    TeamStringPtrTable[4] = TeamAbbrevDRT;
    TeamStringPtrTable[1] = TeamNameMicroMouseMappy;
    TeamStringPtrTable[5] = TeamAbbrevMicroMouseMappy;
    TeamStringPtrTable[2] = StrPacRacingClub;
    TeamStringPtrTable[6] = TeamAbbrevPRC;
    TeamStringPtrTable[3] = TeamNameRacingTeamSolvalou;
    TeamStringPtrTable[7] = TeamAbbrevRTS;
    RivalTeamNamePtrTable[0] = TeamNameAssoluto;
    RivalTeamNamePtrTable[1] = TeamNameLizard;
    RivalTeamNamePtrTable[2] = StrTerrazi;
    RivalTeamNamePtrTable[3] = StrAgeSolo;
    LoadCursor = CdDirectoryBuffer;
    MenuArenaBase = CdDirectoryBuffer;
    InitMenuArenas();
    k = 0xAF000;
    m = -1;
    i = 2;
    DialogueSceneMode = 0;
    GameModeType = 0;
    LoadDestCursor = CdDirectoryBuffer + k;
    do {
        SaveSlotSelections[i].signedBytes[0x17] = m;
        i--;
    } while (i >= 0);
    InitDefaultTimeRecords();
    InitCourseChoices();
    InitTimTemplate(&CustomEmblemBuffers.images[0]);
    InitTimTemplate(&CustomEmblemBuffers.images[1]);
    InitTimTemplate(&SaveDataWork);
    CustomEmblemTimPtr = &CustomEmblemBuffers.images[0];
    ResetRankEntries();
    InitCarTable(1);
    ResetGraph(3);
}

void InitFrameEnvStub(s32 bank) {
}

void SetupDrawDispEnvs(s32 r,s32 g,s16 b,u32 flags) {
    u32 input=flags;
    s32 red=r;
    s32 green;
    u32 temp;
    s32 interlace;
    u32 height;
    u32 other_y;
    s32 width;
    u32 rgb24;
    u32 saved_flags;
    FrameStorage *env;
    GpuRectangleFields window;
    s32 i;
    s32 one;
    u32 loopRgb24;
    u32 background;
    s32 center_x;
    u32 center_y;
    green=g;
    center_x=160;
    temp=input>>1;
    interlace=temp&1;
    /* Lines per field: 256 in the PAL release, 240 in the Japanese one. */
    temp=interlace+1;
    height=temp*GameReleaseProfile->screenHeight;
    center_y=height>>1;
    temp=1-interlace;
    other_y=temp*GameReleaseProfile->screenHeight;
    SetGeomOffset(center_x,center_y);
    SetGeomScreen(290);
    width=320;
    rgb24=input&FRAME_RGB24;
    saved_flags=input;
    if(rgb24)width=480;
    env=DrawBuffers;
    SetDefDrawEnv(&env->buffer.environment00.draw00,0,0,width,height);
    SetDefDrawEnv(&DrawBuffers[1].buffer.environment00.draw00,0,other_y,width,height);
    SetDefDispEnv(&env->buffer.environment00.display5C,0,other_y,width,height);
    SetDefDispEnv(&DrawBuffers[1].buffer.environment00.display5C,0,0,width,height);
    i=0;
    background=saved_flags&FRAME_CLEAR_BACKGROUND;
    one=1;
    loopRgb24=rgb24;
    window.y=0;
    window.x=0;
    window.h=0;
    window.w=0;
loop:
    {
        env->buffer.environment00.draw00.r=red;env->buffer.environment00.draw00.g=green;env->buffer.environment00.draw00.b=b;
        env->buffer.environment00.draw00.background=background;
        env->buffer.environment00.draw00.dither=one;
        env->buffer.environment00.draw00.window=window;
        env->buffer.environment00.display5C.screen.x=(u16)Options.screenX34;
        env->buffer.environment00.display5C.screen.y=(u16)Options.screenY38;
        /* The Japanese release keeps SetDefDispEnv's display height,
         * non-interlaced flag and screen height. */
        if(!RR_RELEASE_JP){
            env->buffer.environment00.display5C.display.h=height;
            env->buffer.environment00.display5C.flags10.bytes.interlace=interlace;
            env->buffer.environment00.display5C.screen.h=256;
        }
        if(loopRgb24) {
            env->buffer.environment00.display5C.flags10.bytes.rgb24=one;
            env->buffer.environment00.display5C.display.w=(env->buffer.environment00.display5C.display.w*2)/3;
        }
        env++;
    }
    i++;
    if(i<2)goto loop;
    InitFrameEnvStub(0);
    InitFrameEnvStub(1);
}

/* Initial values of the globals this module owns. */
char StrTotalInitLog[12] = {116, 111, 116, 97, 108, 95, 105, 110, 105, 116, 10};
char DefaultPlayerName[12] = {78, 65, 77, 67, 79, 33, 33, 33};
char TeamNameDigRacingTeam[20] = {68, 73, 71, 32, 82, 65, 67, 73, 78, 71, 32, 84, 69, 65, 77};
char TeamAbbrevDRT[8] = {68, 46, 82, 46, 84};
char TeamNameMicroMouseMappy[32] = {82, 65, 67, 73, 78, 71, 32, 67, 76, 85, 66, 32, 77, 73, 67, 82, 79, 32, 77, 79, 85, 83, 69, 32, 77, 65, 80, 80, 89};
char TeamAbbrevMicroMouseMappy[8] = {77, 46, 77, 46, 77};
char StrPacRacingClub[20] = {80, 65, 67, 32, 82, 65, 67, 73, 78, 71, 32, 67, 76, 85, 66};
char TeamAbbrevPRC[8] = {80, 46, 82, 46, 67};
char TeamNameRacingTeamSolvalou[24] = {82, 65, 67, 73, 78, 71, 32, 84, 69, 65, 77, 32, 83, 79, 76, 86, 65, 76, 85};
char TeamAbbrevRTS[8] = {82, 46, 84, 46, 83};
char TeamNameAssoluto[12] = {65, 83, 83, 79, 76, 85, 84, 79};
char TeamNameLizard[8] = {76, 73, 90, 65, 82, 68};
char StrTerrazi[12] = {84, 69, 82, 82, 65, 90, 73};
char StrAgeSolo[12] = {65, 71, 69, 32, 83, 79, 76, 79};
char *TeamStringPtrTable[8];
char SaveHeaderPlayerNameBuf[48];
CustomEmblemImages1080 CustomEmblemBuffers;
char *RivalTeamNamePtrTable[4];
int DisplayEnvOffsetX;
int DisplayEnvOffsetY;
