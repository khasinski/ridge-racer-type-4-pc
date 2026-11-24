#include "game/car/carctrl.h"
#include "rr/steering.h"

void UpdateComCarLaneChange(RaceVehicle *vehicle) {
    s32 stepOutput;
    s32 workingStep;
    s16 mode;
    s16 nextTimer;
    s32 timer; s32 currentNode; s32 trackCount;
    s32 phase;
    s32 duration;
    s32 delta;
    u32 targetNode;
    u32 correction;
    s32 smoothedMotion;
    s32 headingDelta;
    s32 angleDifference;
    s32 angle;
    s32 quarterDuration;
    s32 wrappedHeading;
    s32 motion;
    s32 adjustedAngle;
    s32 lateralStep;
    s32 nodeDelta;
    s32 normalizedMotion;
    s32 lateral;
    s32 gain;
    s32 shortRange;
    s32 nodeOffset;
    s32 turnStep;
    u32 wrappedOffset;
    RaceVehicleControl *control;

    control = &vehicle->control;
    if (control->steeringEnabled124 == 0) {
        vehicle->rot54 = vehicle->heading1D4;
        return;
    }
    mode = control->dynamics94.steering.mode00;
    switch (mode) {
    case 0:
        vehicle->rot54 = vehicle->heading1D4;
        break;
    case 1:
        EvalRateCurve(&vehicle->control.dynamics94.steering.primary34, control->dynamics94.steering.timer02, &stepOutput);
        workingStep = stepOutput;
        control->dynamics94.steering.angle0C = ((u32)control->dynamics94.steering.angle0C - workingStep);
        control->dynamics94.steering.delta10 = workingStep;
        control->dynamics94.steering.remaining08 = ((u32)control->dynamics94.steering.remaining08 - workingStep);
        vehicle->rot54 = control->dynamics94.steering.angle0C / 4096;
        if (control->dynamics94.steering.phase28 == 0) {
            delta = control->dynamics94.steering.delta10;
            lateralStep = delta / 8;
             vehicle->lateralMotion178 = lateralStep;
            duration = control->dynamics94.steering.duration04; timer=control->dynamics94.steering.timer02;
            quarterDuration = duration / 4;
            if (timer >= quarterDuration) {
                normalizedMotion = lateralStep;
                if (normalizedMotion < 0) {
                    normalizedMotion += 0xFF;
                }
                BuildRateCurve(&vehicle->control.dynamics94.steering.secondary4C, (normalizedMotion >> 8) * 2, duration - quarterDuration, 0);
                control->dynamics94.steering.lateral24 = ((vehicle->lateralMotion178 / 256) * 0x1000U);
                control->dynamics94.steering.phase28 = mode;
            }
        }
        phase = control->dynamics94.steering.phase28;
        if (phase > 0) {
            EvalRateCurve(&control->dynamics94.steering.secondary4C, phase - 1, &stepOutput);
            lateral = ((u32)control->dynamics94.steering.lateral24 - stepOutput);
            control->dynamics94.steering.lateral24 = lateral;
            vehicle->lateralMotion178 = lateral / 16;
            control->dynamics94.steering.phase28 = (control->dynamics94.steering.phase28 + 1U);
        }
        nextTimer = (u16) control->dynamics94.steering.timer02 + 1;
        control->dynamics94.steering.timer02 = nextTimer;
        if (nextTimer >= control->dynamics94.steering.duration04) {
            control->dynamics94.steering.mode00 = 2;
        }
        break;
    case 2:
        control->dynamics94.steering.lateral24 = (control->dynamics94.steering.lateral24 / 16);
        currentNode = vehicle->trackNodeF0;
        targetNode = control->dynamics94.steering.target18.node;
        nodeOffset = (currentNode - targetNode);
        wrappedOffset = targetNode - currentNode;
        if (nodeOffset >= 0) {
            wrappedOffset = (u32) nodeOffset >> 0x1F;
        } else {
            nodeOffset = (wrappedOffset >> 0x1F);
        }
        control->dynamics94.steering.midpoint1C = ((u32)currentNode - ((s32)(wrappedOffset + nodeOffset) >> 1));
        control->dynamics94.steering.mode00 = 3;
        /* fallthrough */
    case 3:
        if (control->dynamics94.steering.midpoint1C < vehicle->trackNodeF0) {
            vehicle->lateralMotion178 = control->dynamics94.steering.lateral24;
        } else {
            correction = (s32)(((u32)control->dynamics94.steering.lateral24 - vehicle->lateralMotion178) * 3U) / 4;
            control->dynamics94.steering.lateral24 = correction;
            smoothedMotion = (correction + vehicle->lateralMotion178);
            control->dynamics94.steering.lateral24 = smoothedMotion;
            vehicle->lateralMotion178 = smoothedMotion;
        }
        trackCount = TrackNodeCount; nodeDelta = ((u32)control->dynamics94.steering.target18.node - vehicle->trackNodeF0);  nodeDelta %= trackCount;
        if ((trackCount / 2) < nodeDelta) {
            nodeDelta -= trackCount;
        }
        if (nodeDelta >= 0) {
            headingDelta = ((u32)vehicle->rot54 - vehicle->heading1D4);
            wrappedHeading = headingDelta & 0xFFF;
            stepOutput = wrappedHeading;
            if (headingDelta & 0x800) {
                stepOutput = wrappedHeading | 0xF000; /* Original 32-bit OR: no sign extension. */
            }
            if (stepOutput == 0) {
                control->dynamics94.steering.mode00 = 0;
                break;
            }
            turnStep = stepOutput / 4;
            stepOutput = turnStep;
            if (turnStep == 0) {
                control->dynamics94.steering.mode00 = 0;
                break;
            }
            vehicle->rot54 = ((u32)vehicle->rot54 - turnStep);
        }
        break;
    }
    if (vehicle->steeringLock1B8 == 0) {
        angleDifference = AngleDiffAbs(vehicle->rot54, vehicle->heading1D4);
        if (angleDifference < 0x201) {
            gain = angleDifference / 8 + 0x40;
        } else {
            gain = 0x7F;
        }
        control->dynamics94.steering.gain30 = gain;
        /* The existing gain reload still needs for the current match. */
        control->dynamics94.steering.gain30 = ((s32)(((volatile RaceVehicleControl *)control)->dynamics94.steering.gain30 * 96U) / 128);
if (GameModeType == 6 && SelectedMakerIndex == 0) {
            shortRange = angleDifference < 0x201;
            control->dynamics94.steering.gain30 = 0;
        } else {
            shortRange = angleDifference < 0x201;
        }
        if (shortRange) {
            control->dynamics94.steering.range2C = (angleDifference * 3U + 0x1800U);
        } else {
            control->dynamics94.steering.range2C = 0x1E00;
        }
    } else {
        control->dynamics94.steering.gain30 = 0;
        control->dynamics94.steering.range2C = 0;
    }
    motion = vehicle->lateralMotion178;
    if (motion < -0xF000) {
        vehicle->lateralMotion178 = -0xF000;
    } else if (motion > 0xF000) {
        vehicle->lateralMotion178 = 0xF000;
    }
}
