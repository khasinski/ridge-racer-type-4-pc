#include "rr/race.h"
#include "game/sound/sndvab.h"
#include "game/system/pad.h"
#include "game/car/carstate.h"
#include "rr/trig.h"
#include "game/course/trkintp.h"

extern int ReplayState;

void SetCarImpactState(int state, RaceVehicle *vehicle) {
    int pulseStrength = -1;
    int magnitude;
    int absoluteSteer;    int negatedSteer;
    int speed;
    vehicle->state1A4 = state;
    switch (state) {
    case 1:
        pulseStrength = 0;
        vehicle->stateTimer1A5 = 0x1E;
        vehicle->steerAngle1A2 = vehicle->steerInput1BA * 8;
        break;
    case 2:
        magnitude = AngleDiffAbs((short)vehicle->trackAngle1EC, vehicle->rot54);
        goto applyImpactAngle;
    case 8:
        magnitude = 0x400;
    applyImpactAngle:
        if (magnitude >= 0x401) magnitude = 0x800 - magnitude;
        speed = vehicle->input;
        if (speed < 0x140) {
            vehicle->steerAngle1A2 = 0;
        } else {
            vehicle->steerAngle1A2 = ((speed - 0x140) * magnitude) / 4096;
        }
        vehicle->stateTimer1A5 = 0x1E;
        negatedSteer = -(unsigned short)vehicle->steerAngle1A2;
        vehicle->steerAngle1A2 = negatedSteer;
        negatedSteer = (short)negatedSteer;
        absoluteSteer = negatedSteer;
        if (negatedSteer < 0) absoluteSteer = -absoluteSteer;
        magnitude = absoluteSteer;
        pulseStrength = 0;
        if (magnitude >= 0x64) {
            pulseStrength = 2;
            if (magnitude < 0x12C) pulseStrength = 1;
        }
        break;
    }
    if (vehicle == &PlayerCars[0] || vehicle == &PlayerCars[1]) {
        if (pulseStrength != -1) {
            PadStartPulse(vehicle->control.playerFlags24 & 1, 2, pulseStrength);
        }
    }
}

void UpdateCarShake(RaceVehicle *vehicle) {
    int amplitude, oscillation, value, wave;

    if (vehicle->state1A4) {
        vehicle->stateTimer1A5 = vehicle->stateTimer1A5 - 1;
        if (vehicle->stateTimer1A5 == 0) {
            vehicle->state1A4 = 0;
            vehicle->bumpPitch1B4 = 0;
            vehicle->shakeRollOffset1B6 = 0;
            vehicle->shakePitch1B0 = 0;
            vehicle->shakeRoll1B2 = 0;
        }
        value = vehicle->stateTimer1A5;
        amplitude = (value * vehicle->steerAngle1A2) / 128;
        wave = rsin((value * 0x3000) / 30);
        oscillation = (wave * amplitude) / 2048;
        switch (vehicle->state1A4) {
        case 1:
            if (vehicle == &PlayerCars[0] || vehicle == &PlayerCars[1]) {
                if (ReplayState != 2 && vehicle->stateTimer1A5 == 0x1D) {
                    value = vehicle->steerInput1BA * 4;
                    if (value > 0x64) value = 0x64;
                    PlaySfxVolPan(0x28, (short)value, 0x40, -1);
                }
            }
        case 5:
            vehicle->shakePitch1B0 = oscillation + amplitude;
            vehicle->shakeRoll1B2 = (oscillation + amplitude / 2) / 2;
            vehicle->renderAngleX1AC += vehicle->shakePitch1B0 / 4;
            vehicle->renderAngleZ1AE += vehicle->shakeRoll1B2;
            break;
        case 2:
        case 8:
            if (vehicle->steeringLock1B8 == 0) vehicle->shakeRollOffset1B6 = oscillation;
            vehicle->renderAngleZ1AE += vehicle->shakeRollOffset1B6;
            break;
        case 3:
            wave = oscillation + amplitude;
            vehicle->bumpPitch1B4 = wave;
            vehicle->renderAngleX1AC += wave;
            break;
        }
    } else if (vehicle->contactTimer1A7) {
        if (vehicle == &PlayerCars[0] || vehicle == &PlayerCars[1]) {
            PadStartPulse(vehicle->control.playerFlags24 & 1, 1, 1);
        }
        amplitude = vehicle->input >> 2;
        value = vehicle->contactTimer1A7;
        amplitude = (value * amplitude) / 128;
        wave = rsin((value * 0x3000) / 30);
        oscillation = (wave * amplitude) / 2048;
        if (vehicle->steeringLock1B8 == 0) vehicle->shakeRollOffset1B6 = oscillation;
        vehicle->renderAngleZ1AE += vehicle->shakeRollOffset1B6;
        vehicle->contactTimer1A7 = vehicle->contactTimer1A7 - 1;
    } else {
        vehicle->shakeRollOffset1B6 = 0;
        if (vehicle == &PlayerCars[0] || vehicle == &PlayerCars[1]) {
            PadStartPulse(vehicle->control.playerFlags24 & 1, 0, 1);
        }
    }
}
