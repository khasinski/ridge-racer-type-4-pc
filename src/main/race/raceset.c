#include "rr/release.h"
#include <string.h>
#include "rr/save_slots.h"
#include "rr/menu_selection.h"
#include "game/race/ranking.h"
#include "game/sound/sndque.h"
#include "rr/vehicle_table.h"
#include "rr/race_types.h"
#include "rr/menu_choice.h"
#include "game/ui/uipart1.h"
#include "game/race/raceset.h"
#include "rr/overlay661_init.h"
#include "rr/audio_loop.h"
#include "rr/menu_transition.h"
#include "game/menu/menu8.h"
#include "game/race/racesave.h"
#include "game/sound/sndctrl.h"
#include "game/race/racesetp.h"
#include "rr/video_timing.h"

extern u8 GpStageRivalRankLimits[];
extern s16 FadeCountdown;
extern s16 ReverseCourse;
extern s32 TrackLength;
extern s32 LapFrameCounts[];
extern s32 TotalRaceTime;
extern s32 BestLapTime;
extern s32 LapTimes[];
extern s32 RaceState;
extern s32 BestSplitTimes[3];
extern s32 SplitTimes[3];
extern s32 SplitIndex;
extern s16 LapVoicePending;
extern s32 SplitDistances[3];
extern unsigned short GameTypeSceneIds[];

/* After the finish the split check keeps running with SplitIndex 3 and up
 * (the lap counter no longer resets it). The PS1 then reads the words that
 * follow each 3-word array: SplitDistances[3] is an unused zero word
 * (0x8010C14C), [4] the first word of FractionText; SplitTimes[3] is
 * SplitIndex, [4] an unused zero word; BestSplitTimes[3] is LatchedSplitIndex
 * with a zero upper half, [4] LatchedSplitTime. Read those instead of past the
 * host arrays. Farther words are not used: the check stops where FractionText
 * (or a past-the-end distance) exceeds the finished distance, and nothing
 * reads the latched split values before the next race resets them. */
extern char FractionText[];
static s32 SplitDistanceWord(s32 index) {
    s32 word;
    if ((u32)index < 3) return SplitDistances[index];
    if (index == 3) return 0;
    if (index == 4) {
        memcpy(&word, FractionText, sizeof(word));
        return word;
    }
    return 0x7FFFFFFF;
}

static s32 SplitTimeWord(s32 index) {
    if ((u32)index < 3) return SplitTimes[index];
    return index == 3 ? SplitIndex : 0;
}

static s32 BestSplitTimeWord(s32 index) {
    if ((u32)index < 3) return BestSplitTimes[index];
    if (index == 3) return (u16)LatchedSplitIndex;
    return index == 4 ? LatchedSplitTime : 0;
}

void UpdateLapTiming(RaceVehicle *arg0);

void UpdateLapTiming(RaceVehicle *arg0) {
    RaceVehicleControl *s0;
    s16 v1;
    s32 msg;
    s32 target;

    s0 = &arg0->control;
    v1 = s0->dynamics94.steering.count22;
    if (v1 > 0) {
        if (v1 <= NumLaps) {
            LapFrameCounts[v1 - 1] += 1;
            if (LapFrameCounts[s0->dynamics94.steering.count22 - 1] > 0xFFFF) {
                LapFrameCounts[s0->dynamics94.steering.count22 - 1] = 0x10000;
            }
            LapTimes[s0->dynamics94.steering.count22 - 1] = FramesToMilliseconds(
                LapFrameCounts[s0->dynamics94.steering.count22 - 1],
                /* The milliseconds within the frame: 40 a frame (NTSC
                 * release: rand & 31 of its 33). */
                VideoTimingNtsc ? PsyqRand() & 31 : PsyqRand() % 40
            );
            if (LapTimes[s0->dynamics94.steering.count22 - 1] > 0x927BE) {
                LapTimes[s0->dynamics94.steering.count22 - 1] = 0x927BF;
            }
            if (GameModeType == 0) {
                SplitTimes[SplitIndex] = LapTimes[s0->dynamics94.steering.count22 - 1];
            }
        }
    }
    if (s0->dynamics94.steering.count22 * TrackLength <= arg0->second + arg0->first) {
        if (s0->dynamics94.steering.count22 <= NumLaps) {
            s0->dynamics94.steering.count22 = (u16)s0->dynamics94.steering.count22 + 1;
            if (s0->dynamics94.steering.count22 == 1) {
                SplitDeltaTimer = 0;
            } else {
                SplitDeltaTimer = 0x3C;
            }
            LatchedSplitTime = SplitTimes[SplitIndex];
            LatchedBestSplitTime = BestSplitTimes[SplitIndex];
            /* The Japanese release latches the index after resetting it. */
            LatchedSplitIndex = RR_PAL_JP(SplitIndex, 0);
            SplitIndex = 0;
            LapVoicePending = 0;
            if (BestLapTime > LapTimes[s0->dynamics94.steering.count22 - 2]) {
                if (s0->dynamics94.steering.count22 != 1) {
                    SoundQueueEnqueue(9, 0);
                    BestLapTime = LapTimes[s0->dynamics94.steering.count22 - 2];
                    s0->dynamics94.steering.finish20 = (u16)s0->dynamics94.steering.count22 - 2;
                    if (GameModeType == 0) {
                        BestSplitTimes[0] = SplitTimes[0];
                        BestSplitTimes[1] = SplitTimes[1];
                        BestSplitTimes[2] = SplitTimes[2];
                    }
                }
            }
            if ((u16)(CourseIndex - 6) < 2U) {
                target = NumLaps - 1;
            } else {
                target = NumLaps;
            }
            if (s0->dynamics94.steering.count22 == target) {
                SaveCarSnapshots(0);
            } else if (s0->dynamics94.steering.count22 > NumLaps) {
                if (RaceState < 4) {
                    LapVoicePending = -1;
                    RaceState = 4;
                    FadeCountdown = RR_PAL_NTSC(0x4B, 0x5A);
                    SoundSetMasterVolume(0, RR_PAL_NTSC(0x47, 0x56));
                    if (GameModeType != 1) {
                        msg = 0x13;
                    } else if (arg0->half1EE <= GpStageRivalRankLimits[SelectionState.fields.phase17 / 2]) {
                        msg = 0x13;
                    } else if (SelectionState.fields.retries08[SelectionState.fields.phase17] > 0) {
                        msg = 0x14;
                    } else {
                        msg = -1;
                    }
                    if (msg >= 0) {
                        SoundQueueEnqueue(msg, 1);
                    }
                    if (arg0->half1EE < 8 && GameModeType < 2) {
                        msg = 0;
                        TotalRaceTime = 0;
                        if (NumLaps > 0) {
                            do {
                                TotalRaceTime += LapTimes[msg];
                                msg++;
                            } while (msg < NumLaps);
                        }
                        if (TotalRaceTime > 0x927BE) {
                            TotalRaceTime = 0x927BF;
                        }
                        if (GameModeType == 0) {
                            if (CourseBestTotal[ReverseCourse][CourseIndex] > TotalRaceTime) {
                                CourseBestTotal[ReverseCourse][CourseIndex] = TotalRaceTime;
                            }
                            if (CourseBestLap[ReverseCourse][CourseIndex] > BestLapTime) {
                                CourseBestLap[ReverseCourse][CourseIndex] = BestLapTime;
                            }
                            CourseBestLapTimes[ReverseCourse][CourseIndex][0] = BestSplitTimes[0];
                            CourseBestLapTimes[ReverseCourse][CourseIndex][1] = BestSplitTimes[1];
                            CourseBestLapTimes[ReverseCourse][CourseIndex][2] = BestSplitTimes[2];
                        } else if (GameModeType == 1) {
                            if (CourseBestTotalTimes[CourseIndex] > TotalRaceTime) {
                                CourseBestTotalTimes[CourseIndex] = TotalRaceTime;
                            }
                            if (CourseBestLapRecords[CourseIndex] > BestLapTime) {
                                CourseBestLapRecords[CourseIndex] = BestLapTime;
                            }
                        }
                    }
                }
            }
        } else {
            goto check_split;
        }
    } else {
check_split:
        if (SplitDistanceWord(SplitIndex) + TrackLength * (s0->dynamics94.steering.count22 - 1) <= arg0->second + arg0->first) {
            if (GameModeType == 0) {
                SplitDeltaTimer = 0x3C;
                /* The Japanese release latches the next split's index. */
                LatchedSplitIndex = SplitIndex + RR_PAL_JP(0, 1);
                LatchedSplitTime = SplitTimeWord(SplitIndex);
                LatchedBestSplitTime = BestSplitTimeWord(SplitIndex);
                SplitIndex += 1;
            }
        }
    }
    if (LapVoicePending == 0) {
        msg = 9 - (NumLaps - s0->dynamics94.steering.count22);
        if (msg == 9) {
            msg = 0xF;
        }
        if (msg >= 6) {
            SoundQueueEnqueue(msg, 0);
        }
        LapVoicePending = -1;
    }
}

void UpdateRaceRanking(void)
{
  int i;
  int j;
  if (RaceState < 4)
  {
    for (i = 0; i < NumCars; i++)
    {
      RankEntry8 *q = RankTable[i];
      RaceVehicle **o = &CarTable[q->carIndex04];
      q->score00 = (*o)->first + (*o)->second;
      (*o)->half1EE = i;
    }
    for (i = 0; i < NumCars - 1; i++)
    {
      for (j = NumCars - 1; i < j; j--)
      {
        RankEntry8 *a2 = RankTable[j - 1];
        RankEntry8 *a1 = RankTable[j];
        int a0 = a2->score00;
        int v1 = a1->score00;
        /* The PAL release ranks the player car (index 0) first on a tie. */
        if (a0 < v1 || (!RR_RELEASE_JP && a0 == v1 && a1->carIndex04 == 0))
        {
          RankTable[j] = a2;
          RankTable[j - 1] = a1;
        }
      }
    }
  }
}

void SetupRaceAndSceneId(void) {
    SetupRace();
    ActiveSceneId = GameTypeSceneIds[GameModeType];
}

/* Initial values of the globals this module owns. */
u8 GpStageRivalRankLimits[4] = {2, 1};
unsigned short GameTypeSceneIds[8] = {0x135, 0x12F, 0x126, 0x126, 0x11A, 0x12F, 0x152};
int CourseBestTotalTimes[8];
s32 CourseBestLap[2][8];
s32 CourseBestLapTimes[2][8][3];
s32 NumLaps;
s32 CourseBestLapRecords[8];
s32 LapFrameCounts[10];
s32 TotalRaceTime;
s32 BestLapTime;
s32 LapTimes[10];
int CourseBestTotal[2][8];
s32 BestSplitTimes[3];
s16 LatchedSplitIndex;
s32 LatchedSplitTime;
s32 LatchedBestSplitTime;
s32 SplitTimes[3];
s32 SplitIndex;
s16 LapVoicePending;
