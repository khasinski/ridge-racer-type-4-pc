#ifndef RR_OVERLAY_RACE_STATE_H
#define RR_OVERLAY_RACE_STATE_H
#include "game/data/menuvars.h"
#include "rr/race.h"

extern s32 RaceState,TrackLength;
extern s16 ReverseCourse;
void SoundSetMasterVolume(s16 first,s16 second);
void ovl660_check_reverse_run(RaceVehicle *player);
RR_ASSERT_OFFSET(RaceVehicle,half1EA,0x1EA);
#endif
