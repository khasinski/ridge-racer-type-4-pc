#include "types.h"
#include "rr/track_sample_types.h"
#include "rr/race_types.h"
#include "game/car/carphys2.h"
#include "game/course/trknode.h"
#include "game/course/trkintp.h"
#include "rr/menu_overlay_runtime.h"
#include "rr/scene_camera_state.h"

s32 GetSmoothedTrackHeading(int a, int b)
{
    int s2;
    int s0;
    int x;
    s2 = GetTrackHeadingAt(a, b);
    x = a - 2;
    if (x < 0) {
        int d = TrackNodeCount - 2;
        x = d + a;
    }
    s0 = LerpAngle12(s2, GetTrackHeadingAt(x, b), 0x200);
    return LerpAngle12(s0, LerpAngle12(s2, GetTrackHeadingAt((a + 2) % TrackNodeCount, b), 0x200), 0x200);
}

int AngleDiffAbs(int a0, int a1) {
    int d;
    a0 &= 0xFFF;
    a1 &= 0xFFF;
    if (a0 < a1) d = a1 - a0; else d = a0 - a1;
    if (d > 0x800) d = 0x1000 - d;
    return d;
}

s32 AngleDiffSigned(s32 arg0, s32 arg1) {
    s32 temp_a0;
    s32 temp_v0;
    s32 var_a0;
    s32 var_v0;
    s32 var_v1;

    temp_a0 = arg0 & 0xFFF;
    temp_v0 = arg1 & 0xFFF;
    var_v1 = temp_a0 < temp_v0;
    if (var_v1 != 0) {
        var_a0 = temp_v0 - temp_a0;
    } else {
        var_a0 = temp_a0 - temp_v0;
    }
    if (var_a0 >= 0x801) {
        var_a0 = 0x1000 - var_a0;
        var_v1 = var_v1 == 0;
    }
    var_v0 = var_a0;
    if (var_v1 == 0) {
        var_v0 = -var_v0;
    }
    return var_v0;
}

void ComputeStartDistance(RaceVehicle *vehicle, int direction)
{
    int node;
    int target;
    int distance;
    int wrapped;
    int previous;
    *(volatile s32 *)&vehicle->first = 0;
    node = RaceContext.startGrid53C->f0;
    target = vehicle->trackNodeF0;
    distance = 0;
    int steps = 0;
    if (ReverseCourse == 0) {
        if (direction == 0) {
            while (1) {
                wrapped = node % TrackNodeCount;
                distance -= TrackNodes[wrapped].road.segmentLength;
                steps++;
                if (target == wrapped) break;
                node++;
            }
        } else {
            while (1) {
                node--;
                previous = node;
                if (node < 0) previous = TrackNodeCount + node;
                node = previous;
                if (target == node) break;
                distance += TrackNodes[node].road.segmentLength;
                steps++;
            }
        }
    } else {
        if (direction == 0) {
            do {
                node--;
                previous = node;
                if (node < 0) previous = TrackNodeCount + node;
                node = previous;
                distance -= TrackNodes[node].road.segmentLength;
                steps++;
            } while (target != node);
        } else {
            while (1) {
                if (target == node % TrackNodeCount) break;
                node++;
                distance += TrackNodes[node % TrackNodeCount].road.segmentLength;
                steps++;
            }
        }
    }
    vehicle->first = distance;
}
