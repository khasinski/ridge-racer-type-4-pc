#include "rr/track_lateral.h"
#include "rr/steering.h"

void PlanComCarCornerTurn(RaceVehicle *a0);
int FindTrackNodeBehind(int a0, int a1);

void PlanComCarCornerTurn(RaceVehicle *a0) {
    int s0;
    int s1;
    RaceVehicleControl *s2 = &a0->control;
    int s3;
    int s4;
    int s5 = 0;
    int s6;
    int s7;
    int fp = 0;
    int a0_v;
    int v1;

    s3 = a0->input * 6;
    if (s3 < 0x400) {
        s3 = 0x400;
    }
    s2->dynamics94.steering.reserved14 = s3;
    s3 = 0;
    s1 = a0->trackNodeF0;
    s7 = GetTrackNodeHeading(s1, s2->laneTimer38);
    s6 = (short)-a0->heading1D4 + 0x400;
    s6 &= 0xFFF;
    s0 = s3;
    if (s6 & 0x800) {
        s6 |= ~0xFFF;
    }
    s4 = s1;
    s1 = s4 - 1;
    if (s1 < 0) {
        s1 += TrackNodeCount;
    }
    while (s3 < s2->dynamics94.steering.reserved14) {
        v1 = GetTrackNodeHeading(s1, s2->laneTimer38);
        a0_v = v1 - s7;
        a0_v &= 0xFFF;
        s7 = v1;
        if (a0_v & 0x800) {
            a0_v |= ~0xFFF;
        }
        v1 = s7 - s6;
        v1 &= 0xFFF;
        if (v1 & 0x800) {
            v1 |= ~0xFFF;
        }
        switch (s5) {
        case 1:
            if (a0_v < 0 || v1 < 0) {
                s3 = s2->dynamics94.steering.reserved14;
                fp = 1;
            }
            break;
        case -1:
            if (a0_v > 0 || v1 > 0) {
                s3 = s2->dynamics94.steering.reserved14;
                fp = 1;
            }
            break;
        case 0:
            if (a0_v > 0) {
                s5 = 1;
            }
            if (a0_v < 0) {
                s5 = -1;
            }
            break;
        }
        if (!fp) {
            s4 = s1;
            s3 += TrackNodes[s4].road.segmentLength;
            s1 = s4 - 1;
            if (s1 < 0) {
                s1 += TrackNodeCount;
            }
        }
    }
    s1 = s4 - a0->trackNodeF0;
    s1 %= TrackNodeCount;
    if (s1 > TrackNodeCount / 2) {
        s1 -= TrackNodeCount;
    }
    if (s1 < 0) {
        s1 = -s1;
    }
    if (s1 >= 5) {
        s0 = GetTrackNodeHeading(s4, s2->laneTimer38);
        s2->dynamics94.steering.target18.node = s4;
        v1 = (short)-a0->heading1D4 + 0x400;
        v1 &= 0xFFF;
        if (v1 & 0x800) {
            v1 |= ~0xFFF;
        }
        s0 = s0 - v1;
        s0 &= 0xFFF;
        if (s0 & 0x800) {
            s0 |= ~0xFFF;
        }
        s2->dynamics94.steering.remaining08 = s0;
        s2->dynamics94.steering.duration04 = s0 / 32;
        if (s2->dynamics94.steering.duration04 < 0) {
            s2->dynamics94.steering.duration04 = -s2->dynamics94.steering.duration04;
        }
        if (s2->dynamics94.steering.duration04 < 0x20) {
            s2->dynamics94.steering.duration04 = 0x20;
        }
        if (s2->dynamics94.steering.duration04 > 0x40) {
            s2->dynamics94.steering.duration04 = 0x40;
        }
        s0 = a0->rot54;
        s0 &= 0xFFF;
        if (s0 & 0x800) {
            s0 |= ~0xFFF;
        }
        s2->dynamics94.steering.angle0C = s0 << 12;
        BuildRateCurve(&s2->dynamics94.steering.primary34, s2->dynamics94.steering.remaining08, s2->dynamics94.steering.duration04, -s2->steering8C);
        s2->dynamics94.steering.mode00 = 1;
        s2->dynamics94.steering.phase28 = 0;
        s2->dynamics94.steering.timer02 = 0;
        s2->dynamics94.steering.remaining08 <<= 12;
        *(s32 *)&s2->dynamics94.steering.finish20 = -s2->dynamics94.steering.remaining08 / 24;
    }
}

int FindTrackNodeBehind(int a0, int a1) {
    if (a1 > 0) {
        int n = TrackNodeCount;
        TrackSample3C *p = TrackNodes;
        do {
            a0--;
            if (a0 < 0) a0 += n;
            a1 -= p[a0].road.segmentLength;
        } while (a1 > 0);
    }
    return a0;
}

