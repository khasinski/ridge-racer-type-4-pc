#include "game/course/trkseek2.h"
#include "rr/player_cars.h"
#include "rr/track_projection.h"


void UpdateComCarTrackProjections(void)
{
    int s2 = 0;
    RaceVehicle *s1 = ComCars;
    RaceVehicleControl *s0;
    int v;
    unsigned short u;
    do {
        s1 = &ComCars[s2];
        if ((short)s1->flags1E4 > 0) {
            s0 = &s1->control;
            s0->attenuation4A = (unsigned short)s0->attenuation4A - 1;
            if (s0->attenuation4A < 0) {
                s0->attenuation4A = 0;
            }
            if ((short)s1->impactFlags1A0 != 0) {
                s0->laneStart3A = (unsigned short)s0->laneTarget3C;
                UpdateCarTrackProjection(s1);
                s0->laneTimer38 = 10;
                u = s1->lateralOffset172;
                s0->laneActive6A = 1;
                s0->laneTarget3C = u;
                v = s1->input; if (v < 0) v = 0x20; else v = 0x10;
                s0->attenuation4A = v;
                v = s1->input;
                if (v < 0) v = -v;
                s0->speed4C = v << 8;
            } else {
                s0->attenuation4A = 0;
            }
        }
        s2++;
    } while (s2 < 0xB);
}

