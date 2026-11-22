#ifndef RR_STEERING_H
#define RR_STEERING_H
#include "game/data/menuvars.h"
#include "rr/race_types.h"

/* Active node comparisons require a positive track count. Curve construction
 * requires duration / 2 != 0 and fixed-point products representable in s32. */
void UpdateComCarLaneChange(RaceVehicle *vehicle);
s32 AngleDiffAbs(s32 first, s32 second);
void BuildRateCurve(Curve24 *record, s32 displacement, s32 duration, s32 initialRate);
void BuildRateCurveQ12(Curve24 *record, s32 displacement, s32 duration, s32 scaledInitialRate);
s32 EvalRateCurve(const Curve24 *record, s32 time, s32 *output);
extern s32 TrackNodeCount;
extern s16 SelectedMakerIndex;

RR_ASSERT_SIZE(Curve24, 0x18);
RR_ASSERT_OFFSET(Curve24, duration, 4);
RR_ASSERT_OFFSET(Curve24, initialRate, 8);
RR_ASSERT_OFFSET(Curve24, firstAcceleration, 0x10);
RR_ASSERT_OFFSET(Curve24, secondAcceleration, 0x14);
RR_ASSERT_SIZE(SteeringState, 0x64);
RR_ASSERT_OFFSET(SteeringState, primary34, 0x34);
RR_ASSERT_OFFSET(SteeringState, secondary4C, 0x4C);
#if defined(mips)
RR_ASSERT_SIZE(RaceVehicle, 0x320);
RR_ASSERT_OFFSET(RaceVehicle, lateralMotion178, 0x178);
RR_ASSERT_OFFSET(RaceVehicle, steeringLock1B8, 0x1B8);
RR_ASSERT_SIZE(RaceVehicleControl, 0x12C);
RR_ASSERT_OFFSET(RaceVehicleControl, dynamics94, 0x94);
RR_ASSERT_OFFSET(RaceVehicleControl, steeringEnabled124, 0x124);
#endif
#endif
