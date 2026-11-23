#include "game/car/carphys1.h"
#include "rr/race_types.h"
#include "rr/menu_overlay_runtime.h"
#include "game/car/carphys2.h"
#include "game/course/trkintp.h"
void UpdateCarHeading(RaceVehicle *a0) {
    VehicleHandlingPrefix *s1;
    RaceVehicleControl *s2;
    int v1, a1, d;
    int v;
    int t;
    s2 = &a0->control;
    s1 = s2->handling1C;
    v1 = a0->physicsFlags1E0 * rcos(a0->heading1D4 - a0->rot54);
    if (v1 < 0) v1 += 0xFFF;
    a0->input = (unsigned short)a0->input + (v1 >> 12);
    if (s1->steeringMode0A == 0) {
        if (s2->laneTarget3C > 0) {
            a0->heading1D4 = (unsigned short)a0->heading1D4 + ((AngleDiffSigned(a0->heading1D4, GetTrackTargetHeading(a0)) * s2->laneTarget3C) >> 2) / 60;
        }
    }
    a1 = AngleDiffSigned(a0->heading1D4, a0->rot54);
    if (s1->steeringMode0A > 0) {
        if (a0->steerRoll1DE < 0) {
            if (a0->input < 0x3C0) {
                v = a0->steerPitch1DC;
                t = s1->headingDamping1C;
                if (v < 0) v = -v;
                v >>= 4;
            } else {
                v = a0->steerPitch1DC;
                t = s1->headingDamping1C;
                if (v < 0) v = -v;
                v >>= 3;
            }
        } else {
            v = a0->steerPitch1DC;
            t = s1->headingDamping1C;
            if (v < 0) v = -v;
            v >>= 2;
        }
        t++;
        d = v / t + 4;
    } else {
        d = s2->laneTarget3C + 4;
        t = s1->headingDamping1C;
        if (t > 0) {
            v = a0->steerPitch1DC;
            if (v < 0) v = -v;
            d += v / t;
        }
    }
    if (a0->steeringLock1B8 == 0) {
        if (a1 < d && -d < a1) {
            a0->heading1D4 = (unsigned short)a0->heading1D4 + (a1 >> 1);
        } else {
            a0->heading1D4 = (unsigned short)a0->heading1D4 + a1 / d;
        }
    }
    a0->heading1D4 = (unsigned short)a0->heading1D4 & 0xFFF;
}
