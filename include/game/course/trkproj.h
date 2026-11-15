#ifndef RR_GAME_COURSE_TRKPROJ_H
#define RR_GAME_COURSE_TRKPROJ_H

#include "types.h"
#include "rr/race_types.h"

void UpdateComCarOvertake(RaceVehicle *p);
void ComputeCarTrackSurfaceProj(RaceVehicle *vehicle);
void UpdateCarTrackProjection(RaceVehicle *vehicle);
s32 FindNearestCarAhead(RaceVehicle *vehicle);
s32 ScoreOvertakeOpportunity(RaceVehicle *arg0, s32 arg1);
void UpdateComCarTargetPoint(RaceVehicle *vehicle);


#endif /* RR_GAME_COURSE_TRKPROJ_H */
