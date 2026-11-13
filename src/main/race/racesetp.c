#include "rr/release.h"
#include "rr/video_timing.h"
#include "rr/race_setup.h"
#include "game/car/carparam.h"

void SetupRace(void)
{
    s32 lapTable;
    s32 minusOne;
    s32 offset1;

    RaceVehicle *vehicle;
    s32 *rank;
    s32 *selectedCount;
    s32 *result;
    s32 third;
    s32 vehicleIndex;
    u32 course;
    s32 vehicleFlags;
    s32 *vector;
    if (GameModeType == 4)
    {
        ovl667_open_sio_events();
        ovl667_init_sio_session(0x7E900);
    }
    DrawSync(0);
    ApplyOptionsPadButtonMaps();
    InitRaceCars(GameModeType);
    if (((GameModeType < 2) || (GameModeType == 6)) || ((GameModeType == 4) && (LinkPlayerCount == 2)))
    {
        TrackEventSfxEnabled = 1;
    }
    else
    {
        TrackEventSfxEnabled = 0;
    }
    if (GameModeType != 5)
    {
        for (vehicleIndex = 0; vehicleIndex < 2; vehicleIndex++)
        {
            if ((s16)PlayerCars[vehicleIndex].flags1E4 != 0)
            {
                PadRumbleStart(vehicleIndex);
                PlayerCars[vehicleIndex].flags1E4 ^= 0x100;
            }
        }
    }
    InitCourseViewState();
    SetupRenderer(5);
    RaceSetupReadyFlag = 0;
    SplitIndex = 2;
    SplitTimes[1] = 0;
    SplitTimes[0] = 0;
    SplitTimes[2] = -1;
    SplitDistances[2] = TrackLength;
    third = TrackLength / 3;
    SplitDistances[0] = third;
    SplitDistances[1] = (third * 2);
    vector = CourseBestLapTimes[ReverseCourse][CourseIndex];
    BestSplitTimes[0] = vector[0];
    BestSplitTimes[1] = vector[1];
    RaceState = SavedRaceState;
    BestSplitTimes[2] = vector[2];
    if (GameModeType == 0)
    {
        BestLapTime = CourseBestLap[ReverseCourse][CourseIndex];
    }
    else if (GameModeType == 1)
    {
        BestLapTime = CourseBestLapRecords[CourseIndex];
    }
    LapVoicePending = -1;

    if ((GameModeType < 2) || (GameModeType == 6))
    {
        lapTable = 0;
        if (GameModeType == 1)
        {
            lapTable = 1;
        }
        NumLaps = CourseLapCounts[lapTable][CourseIndex];
    }
    vehicleIndex = 0;
    minusOne = -1;

    rank = LapTimes;
    result = LapFrameCounts;
    do
    {
        *result = 0;
        *rank = minusOne;
        rank++;
        vehicleIndex += 1;
        result++;
    }
    while (vehicleIndex < 0xA);
    TotalRaceTime = -1;
    course = (u16) CourseIndex;
    if (((course - 2U) < 3U) || (((s16) course) == 7))
    {
        ForceTailLightTrails = 1;
    }
    else
    {
        ForceTailLightTrails = 0;
    }
    ResetLensFlareState();
    if ((GameModeType == 0) || (GameModeType == 5))
    {
        RearViewMirrorEnabled = 0;
        ChaseViewEnabled = 0;
        DrawSubOtFlag = 0;
    }
    else
    {
        InitRearViewMirrorState();
        RearViewMirrorEnabled = 1;
        ChaseViewEnabled = 0;
        DrawSubOtFlag = 1;
    }
    BuildCameraMatrices();
    InitRaceHudPackets(GameModeType);
    InitHudNumberTargets(GameModeType);
    ScreenFrame = 0;
    ModeFrame = 0;
    FadeCountdown = 0;
    RacePaused = 0;
    PauseCursor = 0;
    RaceStartWipeY = RR_PAL_JP(0x80, 0x78);
    RaceTimeRemaining = RR_PAL_NTSC(0x3A98, 0x4650); /* 10 minutes */
    if ((GameModeType != 1) && (GameModeType != 6))
    {
        RaceTimeRemaining = RR_PAL_NTSC(0xAFC8, 0xD2F0); /* 30 minutes */
    }
    VSyncThreshold = 0x180;
    ClearTrackObjectStates(0);
    PlayerDisplayRpm[0] = 0;
    PlayerDisplayRpm[1] = 0;
    PlayerThrottleLoad[0] = 0;
    PlayerThrottleLoad[1] = 0;
    RaceVoiceFlags = 0;
    StopAllSoundAndReset();
    SelectCourseBgmTrack();
    SetModelTexturePage(0xA);

}

/* Initial values of the globals this module owns. */
s32 ChaseViewEnabled;
s16 GameModeType;
s32 TrackEventSfxEnabled;
s32 RaceSetupReadyFlag;
s32 SplitDistances[3];
