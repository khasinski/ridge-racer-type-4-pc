#include "rr/race_types.h"
#include "game/car/cartimer.h"

void DecayCarOffset(RaceVehicle *vehicle) {
    RaceVehicle *car = vehicle;
    if (car->shakeActive19C != 0) {
        if (car->shakeTimer19E > 0) {
            car->shakeTimer19E = car->shakeTimer19E - 1;
            if (car->shakeTimer19E <= 0) {
                car->shakeActive19C = 0;
                car->shakeTimer19E = 0;
                if (car->flags1E4 & 0x10) car->control.pathDeltaX30 = 0;
            }
            {
                s32 offsetX;
                s32 offsetY, scaledX;
                offsetX = car->shakeX198;
                offsetY = car->shakeY19A;
                car->posScaled00[0] = (s32)((u32)car->posScaled00[0] - (u32)offsetX);
                car->posScaled00[2] = (s32)((u32)car->posScaled00[2] - (u32)offsetY);
                scaledX = offsetX * 7;
                car->shakeX198 = scaledX >> 3;
                car->shakeY19A = (offsetY * 7) >> 3;
            }
        }
    }
}
