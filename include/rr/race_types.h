#ifndef RR_RACE_TYPES_H
#define RR_RACE_TYPES_H
#include "rr/layout.h"
#include "rr/geometry.h"
#include "rr/car_record_types.h"

/* Six-word interpolation record, shared by the constructor and evaluator. */
typedef struct {
 s32 displacement, duration;
 s32 initialRate, middleRate;
 s32 firstAcceleration, secondAcceleration;
} Curve24;
/* Lane-change target. The renderer also reads the signed high half of this
 * word when selecting the alternate drag response. Both views are used. */
typedef union {
 s32 node;
 struct {s16 low, high;} halves;
} SteeringTarget;

/* Lane-change / steering transition state at vehicle offset 0x288. */
typedef struct {
 s16 mode00,timer02;
 s32 duration04,remaining08,angle0C,delta10,reserved14;
 SteeringTarget target18;
 s32 midpoint1C;
 s16 finish20,count22;
 s32 lateral24,phase28,range2C,gain30;
 Curve24 primary34,secondary4C;
} SteeringState;
RR_ASSERT_OFFSET(SteeringState,count22,0x22);
enum {RR_RACE_CONTROL_OFFSET=0x1F4};

/* Four planar corners followed by the two collision-heading limits. The
 * wheel projector prefetches all five consecutive pairs, including the final
 * limit pair after its fourth projection. Keep that read inside its owner. */
typedef struct {
    Vec2s corners[4];
    s16 heading10, heading12;
} VehicleFootprintFields20;
typedef union {
    VehicleFootprintFields20 fields;
    Vec2s pairs[5];
} VehicleFootprint20;
RR_ASSERT_SIZE(VehicleFootprintFields20, 20);
RR_ASSERT_SIZE(VehicleFootprint20, 20);
RR_ASSERT_OFFSET(VehicleFootprintFields20, heading10, 16);
RR_ASSERT_OFFSET(VehicleFootprintFields20, heading12, 18);

/* Four-word position copies include the reserved word as well as x/y/z. */
typedef union {Vector vector; s32 words[3];} VehiclePosition16;

typedef union {
    s32 word;
    struct {s16 accelerating; u16 rpm;} halves;
} VehicleEngineWord;

typedef union {s32 word; u8 bytes[4];} VehicleContactFlags;

/* Player physics and computer steering reuse this region of the control owner.
 * The player threshold is tested when entering a slide; it is not an AI mode. */
typedef struct {
    s32 slideThreshold00, topGearSpeed04;
    s32 steeringHeading08, driveProduct0C;
    s32 motionState10;
    s16 acceleratorEdge14, brakeEdge16, throttle18, brake1A;
    s32 digitalThrottle1C;
} PlayerDynamicsPrefix;
typedef union {
    SteeringState steering;
    PlayerDynamicsPrefix player;
} VehicleDynamics64;
RR_ASSERT_SIZE(VehicleDynamics64,0x64);
RR_ASSERT_OFFSET(VehicleDynamics64,steering,0);
RR_ASSERT_OFFSET(VehicleDynamics64,player,0);
RR_ASSERT_OFFSET(PlayerDynamicsPrefix,motionState10,0x10);

/* The actual control owner begins at vehicle+0x1F4. Both selected car-record
 * and handling members are real pointers, retaining their native width. */
typedef struct {
    s32 unk00[3];
    s32 steerInput0C;
    s32 unk10[2];
    CarRecord *record18;
    VehicleHandlingPrefix *handling1C;
    u8 reserved20[4];
    s32 playerFlags24;
    s16 reserved28, steeringThreshold2A;
    s32 unk2C;
    s16 pathDeltaX30, reserved32, pathDeltaZ34, reserved36;
    s16 laneTimer38, laneStart3A, laneTarget3C, laneCurrent3E;
    s32 laneCooldown40;
    s16 boostTimer44, dragFlag46, holdFlag48, attenuation4A;
    s32 speed4C, accel50;
    VehicleEngineWord engine54;
    s32 lateralSpeed58;
    s16 slideTimer5C, slideDivisor5E;
    s16 latch60, shiftDir62, shiftState64, shiftDelay66;
    s16 gear68, laneActive6A, laneDelta6C, limit6E;
    s32 curveDistance70, relDist74, relDist78, previewDistance7C;
    s32 curveSample80;
    SteeringTarget previewSample84;
    s32 laneMode88;
    s16 steering8C, reserved8E, filtered90, error92;
    VehicleDynamics64 dynamics94;
    s32 maxSpeedF8, gearCountFC;
    s16 gearRatios100[6];
    u16 shiftThresholds10C[6];
    s16 maximum118, span11A, divisor11C, factor11E;
    s32 altSpeed120;
    s16 steeringEnabled124;
    s16 unk126, unk128;
    s16 metric12A;
} RaceVehicleControl;
RR_ASSERT_SIZE(RaceVehicleControl,
    RR_ALIGN_UP(0x12C + 2 * (sizeof(void *) - 4), _Alignof(RaceVehicleControl)));

/* 0x30-byte performance interpolation row (CarSpecTable / context table). */
typedef struct {
    s32 baseSpeed00, gearCount04;
    unsigned short ratios08[6];
    unsigned short thresholds14[6];
    s16 maximum20, span22, divisor24, factor26;
    s32 altSpeed28;
    s16 steering2C, pad2E;
} RaceSpecEntry30;

/* Fields shared by distance comparison, vehicle physics, AI and response calculations. */
typedef struct RaceVehicle {
    s32 posScaled00[3], pad0C;
    VectorWords16 worldPosition10;
    VehiclePosition16 position20;
    s32 vel30, pad34, vel38, pad3C;
    s32 ir40[3], pad4C;
    s32 rot50, rot54, rot58, pad5C;
    s32 ir60[3], pad6C;
    s32 ir70[4];
    s16 ang80, ang82, ang84;
    u8 reserved86[2];
    Vector wheelPositions88[4];
    Vector bodyPosition;
    u8 reservedD8[4];
    s16 wheelSuspensionDC[4], wheelOffsetE4[4];
    s32 prevLateralVelEC;
    s32 trackNodeF0, lateralVelF4;
    s16 wheelNodesF8[4];
    s16 prevWheelNodes100[4];
    s32 height108;
    s16 width10C, halfSpan10E;
    /* Collision corners use planar (x,z), stored as ordinary 2D pairs. */
    VehicleFootprint20 footprint110;
    s16 xHistory124[16], zHistory144[16];
    s16 pitchAverage164, rollAverage166, pitchAverage168, rollAverage16A;
    s32 sampleCount16C;
    s16 trackFraction170;
    s16 lateralOffset172;
    u8 pad174[4];
    s32 lateralMotion178;
    s32 wheelSteering17C;
    u8 pad180[4];
    s32 first, second;
    s32 word18C, word190;
    s16 courseDistance194;
    u8 reserved196[2];
    s16 shakeX198, shakeY19A, shakeActive19C, shakeTimer19E;
    u16 impactFlags1A0;
    s16 steerAngle1A2;
    u8 state1A4, stateTimer1A5, reserved1A6, contactTimer1A7;
    s16 wheelRotX1A8, wheelRotZ1AA, renderAngleX1AC, renderAngleZ1AE;
    s16 shakePitch1B0, shakeRoll1B2, bumpPitch1B4, shakeRollOffset1B6;
    s16 steeringLock1B8;
    s16 steerInput1BA;
    s16 wheelBumpState1BC[4];
    s16 wheelBumpVel1C4[4];
    s16 wheelBumpBase1CC[4];
    s16 heading1D4, prevHeading1D6, input, prevInput1DA;
    s16 steerPitch1DC, steerRoll1DE;
    s32 physicsFlags1E0;
    unsigned short flags1E4;
    s16 slotIndex1E6;
    s16 half1E8;
    s16 half1EA;
    unsigned short trackAngle1EC;
    s16 half1EE;
    VehicleContactFlags contactFlags1F0;
    RaceVehicleControl control;
} RaceVehicle;
RR_ASSERT_OFFSET(RaceVehicle, control, RR_ALIGN_UP(0x1F4, _Alignof(RaceVehicleControl)));
RR_ASSERT_SIZE(RaceVehicle,
    RR_ALIGN_UP(0x1F4, _Alignof(RaceVehicleControl)) + sizeof(RaceVehicleControl));
RR_ASSERT_OFFSET(RaceVehicle, worldPosition10, 0x10);
RR_ASSERT_OFFSET(RaceVehicle, position20, 0x20);
RR_ASSERT_OFFSET(RaceVehicle, footprint110, 0x110);

typedef struct {
    s32 unk00, totalDist04;
    RaceVehicle *vehicle08;
} RaceStandingEntry0C;

/* Per-frame race work: player speeds, distances from the current car, then
 * sorted standings. CalcDistanceToPlayer updates this same scratchpad owner. */
typedef struct RaceStandingsTable {
    s32 playerSpeed[2];
    s32 playerDistance[2];
    s32 closestPlayer;
    s32 gap14;
    RaceStandingEntry0C entries18[8];
} RaceStandingsTable;

#endif
