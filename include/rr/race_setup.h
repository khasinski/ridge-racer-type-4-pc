#ifndef RR_RACE_SETUP_H
#define RR_RACE_SETUP_H
#include "game/data/menuvars.h"
#include "game/ui/hudmap2.h"
#include "rr/menu_selection.h"
#include "game/car/carstat.h"
#include "game/data/cdfile2.h"
#include "rr/race_types.h"
#include "rr/standings.h"
#include "game/race/ranking.h"

void SetModelTexturePage(s32);
void InitRaceHudPackets(s32);
void InitHudNumberTargets(s32);
void ResetLensFlareState(void);
void PadRumbleStart(s32);
void StopAllSoundAndReset(void);
void BuildCameraMatrices(void);
void InitRearViewMirrorState(void);
void ClearTrackObjectStates(s32);
int DrawSync(int mode);
s32 ovl667_open_sio_events(void);
void ovl667_init_sio_session(u32);
extern u32 ScreenFrame;
extern s32 RaceVoiceFlags;
extern s16 ForceTailLightTrails;
extern s32 ChaseViewEnabled;
extern s32 RaceTimeRemaining;
extern s32 VSyncThreshold;
extern s32 DrawSubOtFlag;
extern s16 RearViewMirrorEnabled;
extern s16 ReverseCourse;
extern s32 SavedRaceState;
extern s32 TrackLength;
extern s8 PauseCursor;
extern s8 RacePaused;
extern s32 BestLapTime;
extern s32 RaceState;
extern s32 TrackEventSfxEnabled;
extern s16 LinkPlayerCount;
extern s32 RaceStartWipeY;
extern s32 SplitIndex;
extern s16 LapVoicePending;
extern s32 RaceSetupReadyFlag;
#include "rr/race_setup_types.h"
#include "game/camera/viewini.h"
#include "game/race/raceinit.h"
#include "game/car/carparam.h"
#include "rr/player_cars.h"
extern s32 BestSplitTimes[3],SplitTimes[3],SplitDistances[3];
extern s32 LapFrameCounts[10];

/* Course/group must remain0..7/0..1 whenever tables are selected. All signed16
 * mode values are supported. Helpers may change later-read globals, tables,
 * current/future vehicle flags and output state; both whole vehicles stay live.
 * The 8011 hooks are the overlay entry points used when entering mode4. */
void SetupRace(void);
RR_ASSERT_SIZE(RaceSetupPair4,4);
RR_ASSERT_OFFSET(RaceSetupPair4,second,2);
#endif
