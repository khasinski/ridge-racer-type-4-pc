#include "rr/release.h"
#include "types.h"
#include "rr/menu_selection.h"
#include "scratchpad.h"
#include "rr/vehicle_table.h"
#include "rr/lookup.h"
#include "rr/sequence_queue.h"
#include "game/car/carai.h"
#include "game/car/carstate.h"
#include "game/course/trkseek2.h"
#include "game/ui/uipart1.h"
#include "game/car/carsetup.h"
#include "game/race/racesave.h"
#include "game/sound/sndbgm3.h"
#include "game/sound/sndvab.h"
#include "psyq/libgte/gtemath3.h"
#include "game/race/racemai2.h"
#include "rr/steering.h"
#include "rr/trig.h"
#include "rr/track_sample_types.h"
#include "rr/audio_loop.h"
#include "game/math/rotmat.h"


extern unsigned char CourseStartSteeringBias[];
extern RaceVehicle PlayerCars[2];
extern s32 ReplayState;
extern s32 TrackLength;
extern int RaceVoiceFlags;
extern short ReverseCourse;

void ResolveCarCollisions(RaceVehicle *arg0);
void UpdateRumbleStrip(RaceVehicle *p, int b);
void CheckFinalLapEvent(RaceVehicle *vehicle);

void StepRaceFrame(unsigned int gameMode)
{
  int i;
  int j;
  int impactAngle;
  int speedScale;
  int courseGroup;
  int updateComProjections;
  int impactCosine;
  RaceVehicle *car;
  int *carIndex;
  carIndex = (int *) (SCRATCHPAD + 0x7C);
  updateComProjections = 0;
  courseGroup = (s16)CourseIndex >> 1;
  switch (gameMode)
  {
    case 0:
    default:
      ReadCarInput(PlayerCars, 0);
      UpdatePlayerCar(PlayerCars, 0);
      break;

    case 1:
    case 6:
      ReadCarInput(PlayerCars, 0);
      UpdatePlayerCar(PlayerCars, 0);
      updateComProjections = 1;
      UpdateComCars();
      break;

    case 2:
      ReadCarInput(PlayerCars, 0);
      ReadCarInput(PlayerCars + 1, 1);
      UpdatePlayerCar(PlayerCars, 0);
      UpdatePlayerCar(PlayerCars + 1, 1);
      break;

    case 3:
      ReadCarInput(PlayerCars, 0);
      ReadCarInput(PlayerCars + 1, 1);
      UpdatePlayerCar(PlayerCars, 0);
      UpdatePlayerCar(PlayerCars + 1, 1);
      updateComProjections = 1;
      UpdateComCars();
      break;

    case 4:
      for (i = 0; i < NumCars; i++)
      {
        ReadCarInput(CarTable[i], i);
      }
      for (i = 0; i < NumCars; i++)
      {
        if (CarTable[i]->half1E8 != (-1))
        {
          UpdatePlayerCar(CarTable[i], i);
        }
      }
      break;

    case 5:
      updateComProjections = 1;
      UpdateComCars();
      break;
  }

  for (i = 0; i < 7; i++)
  {
    for (*carIndex = 0; *carIndex < NumCars; (*carIndex)++)
    {
      if (CarTable[*carIndex]->half1E8 != (-1))
      {
        CarUpdateStages[i](CarTable[*carIndex]);
      }
    }
  }
  for (i = 0; i < NumCars; i++)
  {
    if (CarTable[i]->half1E8 != (-1))
    {
      if (CarTable[i]->impactFlags1A0 & 7)
      {
        SetCarImpactState(2, CarTable[i]);
        StartCollisionShake(CarTable[i], (short) CarTable[i]->impactFlags1A0);
      }
      if (((short) CarTable[i]->impactFlags1A0) != 0)
      {
        impactAngle = ((AngleDiffAbs(0x800 - (s16)CarTable[i]->trackAngle1EC, CarTable[i]->heading1D4) - 0x400) * 3) / 5;
        car = CarTable[i];
        if (car->input >= 0xA1)
        {
          if (car->flags1E4 & 0x10)
          {
            if (car->input >= 0x2F9)
            {
              car->control.pathDeltaX30 = 0x80;
              car = CarTable[i];
            }
            speedScale = (car->input * 0x3A) / (car->control.dynamics94.player.topGearSpeed04 + 1);
            car->control.lateralSpeed58 -= 0x1388;
            impactCosine = rcos(impactAngle);
            car = CarTable[i];
            car->control.dynamics94.player.driveProduct0C = (car->control.dynamics94.player.driveProduct0C * (0x64 - ((speedScale * impactCosine) >> 12))) / 100;
          }
          else
          {
            speedScale = (car->input * 0x1E) / ((car->control.maxSpeedF8 >> 8) + 1);
          }
          impactCosine = rcos(impactAngle);
          car = CarTable[i];
          car->input = (car->input * (0x64 - ((speedScale * impactCosine) / 4096))) / 100;
        }
      }
      if (CarTable[i]->input >= 0x321)
      {
        if (CarTable[i]->steeringLock1B8 == 0)
        {
          for (j = 0; j < 4; j++)
          {
            if (CarTable[i]->contactFlags1F0.bytes[j] != 0)
            {
              CarTable[i]->input -= CourseStartSteeringBias[courseGroup];
              if (CarTable[i]->contactFlags1F0.bytes[j] == 2)
              {
                CarTable[i]->input += CourseStartSteeringBias[courseGroup];
              }
              if (courseGroup == 2 || CarTable[i]->contactTimer1A7 == 0)
              {
                CarTable[i]->contactTimer1A7 = 0xA;
              }
            }
          }
        }
      }
    }
  }
  for (i = 7; i < 0xB; i++)
  {
    for (*carIndex = 0; *carIndex < NumCars; (*carIndex)++)
    {
      if (CarTable[*carIndex]->half1E8 != (-1))
      {
        CarUpdateStages[i](CarTable[*carIndex]);
      }
    }
  }
  /* The Japanese release projects the opponents in every mode but the
   * link battle (GameModeType 4); the PAL one only after UpdateComCars. */
  if (RR_RELEASE_JP ? GameModeType != 4 : updateComProjections == 1)
  {
    UpdateComCarTrackProjections();
  }
}

unsigned PointInQuad(int corner0, int corner1, int corner2, int corner3, int point) {
    return NormalClip(corner0, corner1, point) >= 0 &&
           NormalClip(corner1, corner3, point) >= 0 &&
           NormalClip(corner3, corner2, point) >= 0 &&
           NormalClip(corner2, corner0, point) >= 0;
}

u32 PointInQuadPtr(const CollisionPoint *corners, s32 point) {
    return NormalClip(corners[2].word, corners[3].word, point) >= 0 &&
           NormalClip(corners[3].word, corners[1].word, point) >= 0 &&
           NormalClip(corners[1].word, corners[0].word, point) >= 0 &&
           NormalClip(corners[0].word, corners[2].word, point) >= 0;
}

void ComputeQuadMidpoints(s32 carIndex) {
    ScratchpadCollision *work = (ScratchpadCollision *)SCRATCH_CAR_WORK;

    work->pts40[carIndex][COLLISION_EDGE01].pt.x =
        (work->pts40[carIndex][COLLISION_CORNER0].pt.x +
         work->pts40[carIndex][COLLISION_CORNER1].pt.x) >> 1;
    work->pts40[carIndex][COLLISION_EDGE01].pt.y =
        (work->pts40[carIndex][COLLISION_CORNER0].pt.y +
         work->pts40[carIndex][COLLISION_CORNER1].pt.y) >> 1;
    work->pts40[carIndex][COLLISION_EDGE02].pt.x =
        (work->pts40[carIndex][COLLISION_CORNER0].pt.x +
         work->pts40[carIndex][COLLISION_CORNER2].pt.x) >> 1;
    work->pts40[carIndex][COLLISION_EDGE02].pt.y =
        (work->pts40[carIndex][COLLISION_CORNER0].pt.y +
         work->pts40[carIndex][COLLISION_CORNER2].pt.y) >> 1;
    work->pts40[carIndex][COLLISION_EDGE13].pt.x =
        (work->pts40[carIndex][COLLISION_CORNER1].pt.x +
         work->pts40[carIndex][COLLISION_CORNER3].pt.x) >> 1;
    work->pts40[carIndex][COLLISION_EDGE13].pt.y =
        (work->pts40[carIndex][COLLISION_CORNER1].pt.y +
         work->pts40[carIndex][COLLISION_CORNER3].pt.y) >> 1;
    work->pts40[carIndex][COLLISION_EDGE23].pt.x =
        (work->pts40[carIndex][COLLISION_CORNER2].pt.x +
         work->pts40[carIndex][COLLISION_CORNER3].pt.x) >> 1;
    work->pts40[carIndex][COLLISION_EDGE23].pt.y =
        (work->pts40[carIndex][COLLISION_CORNER2].pt.y +
         work->pts40[carIndex][COLLISION_CORNER3].pt.y) >> 1;
    work->pts40[carIndex][COLLISION_CENTER].pt.x =
        (work->pts40[carIndex][COLLISION_CORNER0].pt.x +
         work->pts40[carIndex][COLLISION_CORNER1].pt.x +
         work->pts40[carIndex][COLLISION_CORNER2].pt.x +
         work->pts40[carIndex][COLLISION_CORNER3].pt.x) >> 2;
    work->pts40[carIndex][COLLISION_CENTER].pt.y =
        (work->pts40[carIndex][COLLISION_CORNER0].pt.y +
         work->pts40[carIndex][COLLISION_CORNER1].pt.y +
         work->pts40[carIndex][COLLISION_CORNER2].pt.y +
         work->pts40[carIndex][COLLISION_CORNER3].pt.y) >> 2;
}

void ResolveCarCollisions(RaceVehicle *arg0) {
    ScratchpadCollision *sp = (ScratchpadCollision *)SCRATCH_CAR_WORK;
    RaceVehicle *cars[2];
    s32 rx[2];
    s32 ry[2];
    s32 dx[2];
    s32 dy[2];
    s32 unused[4];
    RaceVehicle *other;
    s32 i;
    s32 j;
    s32 j_xor_1;
    s32 k;
    s32 hit;
    s32 sinVal;
    s32 cosVal;
    s32 ang;
    s32 side;
    s32 arg0_18C;
    s32 arg0_F4;
    s32 other_F4;
    s32 arg0_24;
    s32 offX;
    s32 offZ;

    if (GameModeType == 0) {
        return;
    }

    sp->pts40[0][1].pt.x = arg0->wheelPositions88[0].x;
    sp->pts40[0][1].pt.y = arg0->wheelPositions88[0].z;
    sp->pts40[0][2].pt.x = arg0->wheelPositions88[1].x;
    sp->pts40[0][2].pt.y = arg0->wheelPositions88[1].z;
    sp->pts40[0][3].pt.x = arg0->wheelPositions88[2].x;
    sp->pts40[0][3].pt.y = arg0->wheelPositions88[2].z;
    sp->pts40[0][4].pt.x = arg0->wheelPositions88[3].x;
    sp->pts40[0][4].pt.y = arg0->wheelPositions88[3].z;
    i = 0;
    ComputeQuadMidpoints(0);

    arg0_18C = arg0->word18C;
    arg0_F4 = arg0->lateralVelF4;
    arg0_24 = arg0->position20.words[1];

    for (; i < NumCars; i++) {
        other = CarTable[i];
        if (other != arg0 && other->half1E8 != -1) {
            s32 diffF4;
            s32 diff18C;
            s32 other_24;

            diff18C = ((other->word18C + TrackLength) - arg0_18C) % TrackLength;
            other_F4 = other->lateralVelF4;
            diffF4 = other_F4 - arg0_F4;
            other_24 = other->position20.words[1];

            if (diffF4 < 0) {
                diffF4 = -diffF4;
            }
            offZ = other_24 - arg0_24;
            if (offZ < 0) {
                offZ = -offZ;
            }

            if ((other->steeringLock1B8 == arg0->steeringLock1B8 || offZ < 0x1A) &&
                diffF4 < 0x64 &&
                (diff18C < 0xC8 || TrackLength - 0xC8 < diff18C) &&
                offZ < 0x3C) {
                {
                    s32 tx;
                    s32 tz;
                    tx = other->worldPosition10.words[0];
                    tx -= arg0->worldPosition10.words[0];
                    sp->dx30 = tx;
                    tz = other->worldPosition10.words[2];
                    tz -= arg0->worldPosition10.words[2];
                    sp->dy34 = tz;
                    sp->pts40[1][1].pt.x = other->wheelPositions88[0].x + sp->dx30;
                    sp->pts40[1][1].pt.y = other->wheelPositions88[0].z + sp->dy34;
                    sp->pts40[1][2].pt.x = other->wheelPositions88[1].x + sp->dx30;
                    sp->pts40[1][2].pt.y = other->wheelPositions88[1].z + sp->dy34;
                    sp->pts40[1][3].pt.x = other->wheelPositions88[2].x + sp->dx30;
                    sp->pts40[1][3].pt.y = other->wheelPositions88[2].z + sp->dy34;
                    sp->pts40[1][4].pt.x = other->wheelPositions88[3].x + sp->dx30;
                    sp->pts40[1][4].pt.y = other->wheelPositions88[3].z + sp->dy34;
                    offX = tx;
                    offZ = tz;
                    ComputeQuadMidpoints(1);
                }

                for (j = 0; j < 2; j++) {
                    j_xor_1 = j ^ 1;
                    for (k = 0; k < 9; k++) {
                        hit = PointInQuadPtr(&sp->pts40[j][1], *(s32 *)(sp->pts40[j_xor_1] + k));
                        if (hit > 0) {
                            s32 rz0;

                            if (j == 0) {
                                cars[0] = arg0;
                                cars[1] = other;
                                sinVal = rsin(-cars[0]->rot54);
                                cosVal = rcos(-cars[0]->rot54);
                                ang = (sp->dx30 * cosVal + sp->dy34 * sinVal) / 4096;
                                rz0 = (-sp->dx30 * sinVal + sp->dy34 * cosVal) / 4096;
                                offZ = 0;
                                offX = 0;
                            } else {
                                cars[0] = other;
                                cars[1] = arg0;
                                sinVal = rsin(-cars[0]->rot54);
                                cosVal = rcos(-cars[0]->rot54);
                                ang = -((sp->dx30 + sp->dy34) * cosVal) / 4096;
                                rz0 = -((sp->dy34 - sp->dx30) * sinVal) / 4096;
                            }

                            ang = 0x400 - Atan2Table(ang, rz0);
                            side = 0;
                            if (ang >= -cars[0]->footprint110.fields.heading12 && ang <= cars[0]->footprint110.fields.heading12) {
                                if (ang <= -cars[0]->footprint110.fields.heading10 || ang >= cars[0]->footprint110.fields.heading10) {
                                    side = 2;
                                }
                            } else {
                                side = 1;
                            }

                            if (j_xor_1 != 0) {
                                ang = -Atan2Table(sp->dx30, sp->dy34) + 0xC00;
                            } else {
                                ang = -Atan2Table(-sp->dx30, -sp->dy34) + 0xC00;
                            }
                            sp->dx30 = rsin(ang) << 4;
                            sp->dy34 = rcos(ang) << 4;

                            dx[1] = (sp->pts40[j_xor_1][k].pt.x - (sp->dx30 >> 6)) - offX;
                            dy[1] = (sp->pts40[j_xor_1][k].pt.y - (sp->dy34 >> 6)) - offZ;
                            dx[0] = sp->pts40[j_xor_1][k].pt.x - offX;
                            dy[0] = sp->pts40[j_xor_1][k].pt.y - offZ;

                            rx[0] = (dx[0] * cosVal + dy[0] * sinVal) >> 12;
                            ry[0] = (-dx[0] * sinVal + dy[0] * cosVal) >> 12;
                            rx[1] = (dx[1] * cosVal + dy[1] * sinVal) >> 12;
                            ry[1] = (-dx[1] * sinVal + dy[1] * cosVal) >> 12;

                            ang = Atan2Table(rx[1] - rx[0], ry[1] - ry[0]) & 0xFFF;
                            sp->angles14[0] = Atan2Table(cars[0]->footprint110.fields.corners[1].x - rx[0], cars[0]->footprint110.fields.corners[1].y - ry[0]) & 0xFFF;
                            sp->angles14[1] = Atan2Table(cars[0]->footprint110.fields.corners[0].x - rx[0], cars[0]->footprint110.fields.corners[0].y - ry[0]) & 0xFFF;
                            sp->angles14[2] = Atan2Table(cars[0]->footprint110.fields.corners[2].x - rx[0], cars[0]->footprint110.fields.corners[2].y - ry[0]) & 0xFFF;
                            sp->angles14[3] = Atan2Table(cars[0]->footprint110.fields.corners[3].x - rx[0], cars[0]->footprint110.fields.corners[3].y - ry[0]) & 0xFFF;

                            for (dy[0] = 0; dy[0] < 3; dy[0]++) {
                                if (ang >= sp->angles14[dy[0]] && ang <= sp->angles14[dy[0] + 1]) {
                                    break;
                                }
                                if (dy[0] == 2) {
                                    dy[0] = 3;
                                }
                            }

                            dy[1] = dy[0];
                            if (dy[0] & 1) {
                                s32 v1;
                                s32 r0;
                                if (dy[0] == 1) {
                                    dy[0] = 0;
                                }
                                v1 = cars[0]->footprint110.fields.corners[dy[0]].x;
                                r0 = rx[0];
                                dx[0] = __builtin_abs(rx[1] - r0);
                                dx[1] = __builtin_abs(v1 - r0);
                            } else {
                                s32 v1;
                                s32 r0;
                                /* dy[0] is 4 when no edge matched: pair 4 is heading10/heading12, as on the PS1 */
                                v1 = cars[0]->footprint110.pairs[dy[0]].y;
                                r0 = ry[0];
                                dx[0] = __builtin_abs(ry[1] - r0);
                                dx[1] = __builtin_abs(v1 - r0);
                            }

                            if (dx[0] != 0 && dx[0] >= dx[1]) {
                                sp->dx30 = (sp->dx30 / dx[0]) * dx[1];
                                sp->dy34 = (sp->dy34 / dx[0]) * dx[1];
                            } else {
                                sp->dx30 >>= 6;
                                sp->dy34 >>= 6;
                            }

                            ang = rcos(AngleDiffAbs(cars[0]->heading1D4, cars[1]->heading1D4));
                            cars[1]->posScaled00[0] -= sp->dx30;
                            cars[1]->posScaled00[2] -= sp->dy34;
                            cars[1]->worldPosition10.words[0] -= sp->dx30 >> 6;
                            cars[1]->worldPosition10.words[2] -= sp->dy34 >> 6;
                            cars[1]->impactFlags1A0 |= hit << 3;

                            if (side < 2) {
                                SetVehicleShake(cars[0], -sp->dx30, -sp->dy34);
                                SetVehicleShake(cars[1], sp->dx30, sp->dy34);
                                if (cars[side]->flags1E4 & 0x10) {
                                    if (cars[side]->input >= 0xF1) {
                                        arg0->control.pathDeltaX30 = 0x40;
                                    }
                                    cars[side]->control.lateralSpeed58 -= 0x1388;
                                    cars[side]->control.dynamics94.player.driveProduct0C = (cars[side]->control.dynamics94.player.driveProduct0C * 92) / 100;
                                    cars[side ^ 1]->input = (cars[side ^ 1]->input + (cars[side]->input * ang) / 4096) / 2;
                                    if (cars[side ^ 1]->input < 0) {
                                        cars[side ^ 1]->input = 0;
                                    }
                                    if (cars[side]->input >= 0xF1) {
                                        cars[side]->input = (cars[side ^ 1]->input * 95) / 100;
                                    }
                                } else {
                                    if (cars[side]->input > 0) {
                                        cars[side ^ 1]->input = (cars[side ^ 1]->input + (cars[side]->input * ang) / 4096) / 2;
                                        if (cars[side ^ 1]->input < 0) {
                                            cars[side ^ 1]->input = 0;
                                        }
                                        if (cars[side]->input >= 0xF1 || (s16)cars[side ^ 1]->flags1E4 >= 0x10) {
                                            cars[side]->input = (-cars[side ^ 1]->input * 99) / 100;
                                        }
                                    }
                                }
                            } else {
                                SetVehicleShake(cars[0], -sp->dx30 >> 1, -sp->dy34 >> 1);
                                SetVehicleShake(cars[1], sp->dx30 >> 1, sp->dy34 >> 1);
                            }

                            side &= 1;
                            if (k == 0) {
                                s32 tx;
                                s32 tz;
                                tx = other->worldPosition10.words[0];
                                tx -= arg0->worldPosition10.words[0];
                                sp->dx30 = tx;
                                tz = other->worldPosition10.words[2];
                                tz -= arg0->worldPosition10.words[2];
                                sp->dy34 = tz;
                                sp->pts40[1][1].pt.x = other->wheelPositions88[0].x + sp->dx30;
                                sp->pts40[1][1].pt.y = other->wheelPositions88[0].z + sp->dy34;
                                sp->pts40[1][2].pt.x = other->wheelPositions88[1].x + sp->dx30;
                                sp->pts40[1][2].pt.y = other->wheelPositions88[1].z + sp->dy34;
                                sp->pts40[1][3].pt.x = other->wheelPositions88[2].x + sp->dx30;
                                sp->pts40[1][3].pt.y = other->wheelPositions88[2].z + sp->dy34;
                                sp->pts40[1][4].pt.x = other->wheelPositions88[3].x + sp->dx30;
                                sp->pts40[1][4].pt.y = other->wheelPositions88[3].z + sp->dy34;
                                offX = tx;
                                offZ = tz;
                                ComputeQuadMidpoints(1);
                            } else {
                                if (cars[side]->stateTimer1A5 == 0) {
                                    SetCarImpactState(8, cars[side]);
                                }
                                /* The Japanese release knocks the other car only
                                 * in the two-player modes (GameModeType 2..4). */
                                if ((!RR_RELEASE_JP || (u16)(GameModeType - 2) < 3) &&
                                    cars[side ^ 1]->stateTimer1A5 == 0) {
                                    SetCarImpactState(8, cars[side ^ 1]);
                                }
                                if ((arg0 == &PlayerCars[0] || arg0 == &PlayerCars[1]) && arg0->stateTimer1A5 == 0x1E) {
                                    s16 pan = (other_F4 - arg0_F4) / 2;
                                    s16 vol;
                                    if (arg0->half1EA != 0) {
                                        pan = -pan;
                                    }
                                    vol = cars[side]->prevInput1DA - cars[side]->input;
                                    pan += 0x40;
                                    if (vol >= 0x200) {
                                        vol = 0x1FF;
                                    }
                                    if (ReplayState != 2 && arg0->input >= 0) {
                                        PlaySfxVolPan(0x21, (vol >> 3) + 0x3C, pan, -1);
                                    }
                                }
                                break;
                            }
                        }
                    }
                    if (hit != 0) {
                        break;
                    }
                }
            }
        }
    }
}

s32 GetCarTrackLightIndex(RaceVehicle *vehicle)
{
    return LookupTrackEvent(3, vehicle->trackNodeF0);
}

void PlayTrackEventSfx(RaceVehicle *vehicle)
{
    SetReverbDepthBoth(LookupTrackEvent(2, vehicle->trackNodeF0));
}

void UpdateRumbleStrip(RaceVehicle *p, int b) {
    int s3;
    int s2;
    int t;
    int a1;
    int s0;
    int ang;
    a1 = LookupTrackEvent(4, p->trackNodeF0);
    s3 = 0;
    s2 = s3;
    if (a1 >= 0) {
        if (a1 == 0) a1 = 3;
        s0 = p->lateralOffset172;
        if (s0 < 0) {
            s0 += 0x100;
            if (s0 > 0) s0 = 0;
        } else {
            s0 -= 0x100;
            if (s0 < 0) s0 = 0;
        }
        if (s0 != 0) {
            s0 = s0 * p->input / 7665;
            if (s0 >= 0x40) {
                s0 = 0x3F;
            } else if (s0 < -0x3F) {
                s0 = -0x3F;
            }
            t = p->rot54 - 0x800;
            t += TrackNodes[p->trackNodeF0].road.heading;
            ang = t & 0xFFF;
            if (s0 < 0 && (a1 & 1) > 0) {
                s2 = -(s0 + s0 * rcos(ang) / 4096);
                s3 = -(s0 + -s0 * rcos(ang) / 4096);
            } else if (s0 > 0) {
                if ((a1 & 2) > 0) {
                    s3 = s0 + s0 * rcos(ang) / 4096;
                    s2 = s0 + -s0 * rcos(ang) / 4096;
                }
            }
        }
    }
    SetLoopVoiceVolumes(b, s2, s3);
}

void CheckFinalLapEvent(RaceVehicle *vehicle)
{
    int distanceLimit;
    int courseLength;
    int flags, node;
    if (!(RaceVoiceFlags & 0x100)) {
        if (ReverseCourse != 1) {
            courseLength = TrackLength;
            distanceLimit = (NumLaps - 1) * courseLength;
            if ((vehicle->second + vehicle->first) >= distanceLimit) {
                node = vehicle->trackNodeF0;
                if (LookupTrackEvent(6, node) == 1) {
                    flags = RaceVoiceFlags & 0x100;
                    RaceVoiceFlags = flags;
                    if (flags == 0) {
                        SoundQueueEnqueue(0x12, 0);
                        RaceVoiceFlags |= 0x100;
                    }
                }
            }
        }
    }
}

/* Initial values of the globals this module owns. */
#include "game/car/aipath.h"
#include "game/car/aipath2.h"
#include "game/car/cartimer.h"
#include "game/car/carmove1.h"
#include "game/car/carparam.h"
#include "game/car/carsteer.h"
CarUpdateStage CarUpdateStages[11] = {(void (*)(struct RaceVehicle *))ClampAiCarLateralMotion, (void (*)(struct RaceVehicle *))UpdateCarSteering, (void (*)(struct RaceVehicle *))DecayCarOffset, (void (*)(struct RaceVehicle *))IntegrateCarMotion, (void (*)(struct RaceVehicle *))UpdateCarTrackNode, (void (*)(struct RaceVehicle *))ComputeWheelOffsets, (void (*)(struct RaceVehicle *))ConstrainCarToTrack, (void (*)(struct RaceVehicle *))ResolveCarCollisions, (void (*)(struct RaceVehicle *))UpdateCarShake, (void (*)(struct RaceVehicle *))FindWheelTrackNodes, (void (*)(struct RaceVehicle *))UpdateWheelBumps};
unsigned char CourseStartSteeringBias[4] = {4, 4, 0xC, 4};
RaceVehicle *CarTable[8];
