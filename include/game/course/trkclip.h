#ifndef RR_GAME_COURSE_TRKCLIP_H
#define RR_GAME_COURSE_TRKCLIP_H

#include "types.h"

void GetTrackSegmentAndYaw(int node, int yaw, short *output);
s32 FindTrackNode(s32 *pos, s32 startIdx, s32 maxIter);

#endif /* RR_GAME_COURSE_TRKCLIP_H */
