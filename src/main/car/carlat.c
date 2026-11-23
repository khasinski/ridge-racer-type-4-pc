#include "rr/race.h"
#include "rr/track_lateral.h"
#include "rr/vehicle_drive.h"

s32 CalcSteerLimit(RaceVehicle *vehicle) {
    s32 raw = vehicle->input;
    s32 rounded = raw;
    s32 fraction;
    s16 value;
    s32 v0;
    s32 v1;
    if (raw < 0) rounded = raw + 7;
    fraction = rounded >> 3;
    if (raw) {
        s32 span = vehicle->control.span11A;
        if (span < fraction) {
            value = 16;
        } else {
            v0 = vehicle->control.maximum118;
            v1 = (v0 - 16) << 12;
            v1 /= span;
            {
                s32 product = v1 * fraction;
                u32 maximum = (u16)vehicle->control.maximum118;
                s32 rounded_product = product >> 12;
                if (product < 0) rounded_product = (product + 4095) >> 12;
                value = maximum - rounded_product;
            }
        }
        if (vehicle->control.boostTimer44 > 0) {
            v1 = (vehicle->control.maximum118 - value) * vehicle->control.boostTimer44;
            v0 = __builtin_abs(vehicle->control.divisor11C);
            v1 /= v0;
            value += v1;
        }
    } else {
        value = 0;
    }
    return value;
}

s32 ScanCurveAhead(RaceVehicle *a0, s32 a1, s32 *a2, s32 *a3, s32 *a4) {
    int v0, v1, s0, s1, s2, s3, s4, s5, r0;
    int r1;
    int t;
    v0 = a0->input * 0xC;
    if (v0 < 0) v0 += 0xF;
    v1 = v0 >> 4;
    if (v1 < 0x200) v1 = 0x200;
    s0 = a0->trackNodeF0;
    s1 = -a0->heading1D4 + 0x400;
    s1 &= 0xFFF;
    s2 = v1;
    if (s1 & 0x800) s1 |= ~0xFFF;
    s4 = 0;
    s3 = 0;
    s5 = 0;
    if (s2 > 0) {
        do {
            r0 = GetTrackNodeHeading(s0, a1);
            r0 &= 0xFFF;
            v1 = s1 - r0;
            if (r0 & 0x800) {
                r0 |= ~0xFFF;
                v1 = s1 - r0;
            }
            v1 = v1 & 0xFFF;
            if (v1 & 0x800) v1 |= ~0xFFF;
            if (v1 > 0) s4 += v1; else s3 += v1;
            v1 = __builtin_abs(v1);
            if (s5 < v1) s5 = v1;
            s0 -= 1;
            s1 = r0;
            if (s0 < 0) s0 += TrackNodeCount;
            s2 -= TrackNodes[s0].road.segmentLength;
        } while (s2 > 0);
    }
    *a2 = s5 << 12;
    *a3 = s3;
    *a4 = s4;
    t = s4 - s3;
    r0 = t;
    r1 = r0 << 12;
    return r1 / 12;
}
