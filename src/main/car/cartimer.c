#include "rr/race_types.h"
#include "rr/field_rate.h"
#include "game/car/cartimer.h"

/* The car stages every field (at field rate): the timer counts its ticks
 * by every second field (CAR_FIELD_REM_SHAKE_HALF; the setters clear it),
 * the push keeps sqrt(7/8) a field and moves the car by 1/(1+sqrt(7/8)) of
 * itself, as far by each tick as the original's. */
void DecayCarOffset(RaceVehicle *vehicle) {
    RaceVehicle *car = vehicle;
    if (car->shakeActive19C != 0) {
        if (car->shakeTimer19E > 0) {
            CarFieldState *fs = RaceFieldRateLive ? CarFieldStateOf(car) : NULL;
            if (!fs || (fs->rem[CAR_FIELD_REM_SHAKE_HALF] ^= 1) == 0) {
                car->shakeTimer19E = car->shakeTimer19E - 1;
            }
            if (car->shakeTimer19E <= 0) {
                car->shakeActive19C = 0;
                car->shakeTimer19E = 0;
                if (car->flags1E4 & 0x10) car->control.pathDeltaX30 = 0;
            }
            if (fs) {
                s32 offsetX = car->shakeX198;
                s32 offsetY = car->shakeY19A;
                car->posScaled00[0] = (s32)((u32)car->posScaled00[0] -
                    (u32)RaceFieldFilterAdd(offsetX, 7, 8, 0, &fs->rem[CAR_FIELD_REM_SHAKE_PUSH_X]));
                car->posScaled00[2] = (s32)((u32)car->posScaled00[2] -
                    (u32)RaceFieldFilterAdd(offsetY, 7, 8, 0, &fs->rem[CAR_FIELD_REM_SHAKE_PUSH_Z]));
                car->shakeX198 = RaceFieldKeep(offsetX, 7, 8, &fs->rem[CAR_FIELD_REM_SHAKE_X]);
                car->shakeY19A = RaceFieldKeep(offsetY, 7, 8, &fs->rem[CAR_FIELD_REM_SHAKE_Z]);
            } else {
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
