#include "rr/video_timing.h"
#include "game/data/resinit.h"
#include "psyq/libgte/rotmat.h"
#include "game/data/bankrec.h"
#include "types.h"
#include "rr/race_types.h"
#include "rr_asm.h"
#include "game/car/aipath.h"
#include "game/car/carparam.h"
#include "game/course/trktext.h"
#include "game/race/racemai2.h"
#include "game/sound/sndbgm3.h"
#include "game/car/carsetup.h"
#include "rr/bios_devices.h"
#include "rr/controller_hud.h"
#include "game/car/aipath2.h"
#include "game/car/carsteer.h"
#include "game/race/racesamp.h"
#include "game/ui/hudmap.h"

#include "game/data/selection.h"
extern int RaceState;
extern int TrackEventSfxEnabled;
extern int ReplayState;
extern int ChaseViewEnabled;
extern RaceVehicle PlayerCars[];

void UpdatePlayerCar(RaceVehicle *a0, int a1);
void StepCarPhysics(RaceVehicle *arg0);
void UpdateCarStartLineSteering(RaceVehicle *arg0);

void ComputeWheelOffsets(RaceVehicle *a0)
{
    Vec3sFields8 v;
    MatrixWords32 m;
    int i, n;
    v.x = 0;
    v.y = (short)a0->rot54;
    v.z = 0;
    RotMatrix(&v, &m);
    RR_GTE_SETROT(m.w);
    v.x = a0->footprint110.pairs[0].x;
    v.y = (short)a0->wheelPositions88[0].y;
    i = 0;
    v.z = a0->footprint110.pairs[0].y;
    do {
        RR_GTE_MVMVA_ROT_V0_NOCV(&v);
        n = i + 1;
        v.x = a0->footprint110.pairs[n].x;
        v.y = (short)a0->wheelPositions88[i].y;
        v.z = a0->footprint110.pairs[n].y;
        RR_GTE_STORE_IR_MEM(&a0->wheelPositions88[i]);
        i = n;
    } while (i < 4);
}

void InitPlayerCar(RaceVehicle *vehicle, s32 recordIndex, s32 gridIndex)
{
    CarRecord *record;
    RaceVehicleControl *control;
    CarGearRecord3C *gear;
    s16 *speed;
    s32 i, threshold;

    control = &vehicle->control;
    record = CarRecordPtrs[recordIndex];
    PsyqBzero(vehicle, (s32)(sizeof(*vehicle)));
    control->previewSample84.halves.low = SelectionRows.entries[recordIndex].option & 1;
    vehicle->slotIndex1E6 = recordIndex * 2;
    control->playerFlags24 = recordIndex;
    vehicle->flags1E4 = (u16)control->playerFlags24 | 0x110;
    control->dynamics94.player.motionState10 = 4;
    threshold = record->handling1C8.slideThreshold16;
    RaceState = 2;
    control->dynamics94.player.slideThreshold00 = threshold;
    if (threshold < 10000) {
        control->dynamics94.player.slideThreshold00 = 10000;
    }
    gear = record->gears04;
    control->handling1C = &record->handling1C8;
    control->pathDeltaZ34 = 1;
    control->previewSample84.halves.high = 1;
    control->record18 = record;
    control->holdFlag48 = 4;
    control->steeringThreshold2A = RR_PAL_NTSC(100, 120); /* 4 seconds */
    i = 0;
    if (record->lastGear00 >= 0) {
        do {
            speed = &gear->speed02;
            gear->scaledRate00 = (((s32)((u32)*speed * 0x36C000U) / 1600) * 256) / 100000;
            gear->accumulator32 = 0;
            if (i == record->lastGear00) {
                control->dynamics94.player.topGearSpeed04 = (*speed * 0x490) / 160;
            }
            i++;
            gear++;
        } while (record->lastGear00 >= i);
    }
    PlaceCarOnGrid(vehicle, gridIndex);
}

void UpdatePlayerCar(RaceVehicle *a0, int a1)
{
  if (RaceState > 0)
  {
    a0->half1EA = IsCarFacingReverse(a0);
    UpdateCarGear(a0, a1);
    UpdatePlayerSteering(a0, a1);
    UpdateCarEngine(a0);
    if ((a0 == &PlayerCars[0]) || (a0 == &PlayerCars[1]))
    {
      if (TrackEventSfxEnabled == 1)
      {
        PlayTrackEventSfx(a0);
      }
      if (ReplayState != 2)
      {
        UpdateRumbleStrip(a0, a1 & 1);
      }
    }
  }
  else
    if (RaceState == 0)
  {
    SetLoopVoiceVolumes(a1 & 1, 0, 0);
  }
  if (LinkHostFlag < 0)
  {
    if (GetMappedPadButtons(a1 & 1) != 0)
    {
      if (RaceState - 2U < 2U)
      {
        unsigned short t = a0->flags1E4 ^ 0x100;
        ChaseViewEnabled ^= 1;
        a0->flags1E4 = t;
        ResetCarSlotBuffers();
      }
    }
  }
}

void StepCarPhysics(RaceVehicle *arg0) {
    UpdateCarSteering(arg0);
    IntegrateCarMotion(arg0);
    UpdateCarTrackNode(arg0);
    ComputeWheelOffsets(arg0);
    ConstrainCarToTrack(arg0);
}

void UpdateCarStartLineSteering(RaceVehicle *arg0) {
    s32 temp_a2;
    RaceVehicleControl *control;

    control = &arg0->control;
    control->relDist74 = 0;
    temp_a2 = (control->laneMode88 - 0x1964U);
    if (control->previewSample84.halves.high >= 2) {
        control->dynamics94.steering.angle0C = 0;
    }
    if (temp_a2 < 0) {
        control->relDist74 = (temp_a2 >> 2);
        control->dynamics94.steering.angle0C = ((s32)(control->dynamics94.steering.angle0C * 7U) / 10);
        return;
    }
    control->relDist74 = temp_a2;
    control->laneMode88 = ((u32)control->laneMode88 - temp_a2);
    control->dynamics94.steering.angle0C = ((u32)control->dynamics94.steering.angle0C - temp_a2);
}
