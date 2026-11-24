#include "rr/scene_camera_state.h"
#include "types.h"
#include "game/course/control.h"
#include "rr/track_projection.h"
#include "rr/geometry.h"
#include "rr/trig.h"
#include "rr/race_front_search.h"
#include "rr/track_interpolation.h"
#include "rr/race.h"
#include "rr/track_lateral.h"
#include "game/data/globals.h"
#include "game/course/trkproj.h"
#include "rr/audio_loop.h"
#include "rr/vehicle_drive.h"

/* Keep the original table-relative cursors, while using the canonical native
 * record stride and array offset (the pointer-bearing row grows on a host). */
#define RR_STANDING_PREFIX_SIZE \
    (sizeof(RaceStandingsTable)-sizeof(((RaceStandingsTable*)0)->entries18))
#define RR_STANDING_ROW(base) \
    ((RaceStandingEntry0C*)((base)+RR_STANDING_PREFIX_SIZE))
#define SIN(x) (((x) & 0x800) ? (((x) & 0x400) ? -SinTable[0x400 - ((x) & 0x3FF)] : -SinTable[(x) & 0x3FF]) : (((x) & 0x400) ? SinTable[0x400 - ((x) & 0x3FF)] : SinTable[(x) & 0x3FF]))

extern s8 CarShadowOffsetTableA[][16];
extern s8 CarShadowOffsetTableB[][16];

void ProjectToTrackSegment(TrackOffsetArgument offsetX, TrackOffsetArgument offsetZ, s32 node, s16 *fraction, s16 *lateral, s32 *remaining);
s32 ScoreOvertakeOpportunity(RaceVehicle *arg0, s32 arg1);
void InterpolateLanePointXZ(const TrackSample3C *first, const TrackSample3C *second, s32 lateral, s32 fraction, s32 *output);
void InterpolateLanePointXYZ(const TrackSample3C *first, const TrackSample3C *second, s32 lateral, s32 fraction, s32 *output);

void ComputeCarTrackSurfaceProj(RaceVehicle *vehicle)
{
    struct {
        TrackOffsetArgument a;
        TrackOffsetArgument b;
        Vector first;
        Vector second;
        s16 fraction;
        int remaining;
    } line;
    int distance = 0x400 - vehicle->trackFraction170;
    int rounded = distance;
    int firstNode;
    int secondNode;
    int fraction;
    TrackLaneOffset8 *offset;
    TrackLaneOffset8 *sample;
    RaceVehicleControl *control = &vehicle->control;
    if (rounded < 0) {
        rounded += 0x3FF;
    }
    firstNode = vehicle->trackNodeF0 - (rounded >> 10);
    if (firstNode < 0) {
        firstNode += TrackNodeCount;
    }
    secondNode = firstNode - 1;
    if (secondNode < 0) {
        secondNode += TrackNodeCount;
    }
    offset = RaceContext.laneOffsets54C + firstNode;
    sample = offset;
    line.first.x = sample->x;
    line.first.z = sample->z;
    sample = RaceContext.laneOffsets54C + secondNode;
    line.second.x = sample->x;
    line.second.z = sample->z;
    fraction = distance & 0x3FF;
    {
        int dx = (line.second.x - line.first.x) * fraction;
        int dz;
        if (dx < 0) {
            dx += 0x3FF;
        }
        dz = (line.second.z - line.first.z) * fraction;
        line.a.value = line.first.x + (dx >> 10);
        if (dz < 0) {
            dz += 0x3FF;
        }
        line.b.value = line.first.z + (dz >> 10);
    }
    ProjectToTrackSegment(line.a, line.b, vehicle->trackNodeF0, &line.fraction, &vehicle->control.laneCurrent3E, &line.remaining);
    if (control->laneTimer38 == 0) {
        control->laneTarget3C = control->laneCurrent3E;
    }
}

#define SIN(x) (((x) & 0x800) ? (((x) & 0x400) ? -SinTable[0x400 - ((x) & 0x3FF)] : -SinTable[(x) & 0x3FF]) : (((x) & 0x400) ? SinTable[0x400 - ((x) & 0x3FF)] : SinTable[(x) & 0x3FF]))

void ProjectToTrackSegment(TrackOffsetArgument offsetX, TrackOffsetArgument offsetZ, s32 node, s16 *fraction, s16 *lateral, s32 *remaining) {
    s32 nextNode;
    TrackSample3C *sample;
    TrackSample3C *nextSample;
    s32 angle;
    s32 cosine;
    s32 along;
    s32 across;
    s32 position;
    s32 width;
    s32 nextWidth;

    nextNode = (node + 1) % TrackNodeCount;
    sample = &TrackNodes[node];
    nextSample = &TrackNodes[nextNode];

    angle = sample->road.heading + 0x800;
    node = sample->road.heading + 0x400;

    across = SIN(angle);
    cosine = SIN(node);

    along = (across * offsetX.value + cosine * offsetZ.value) / 4096;
    across = (-cosine * offsetX.value + across * offsetZ.value) / 4096;

    if (along < 0) {
        along = 0;
    }

    node = nextSample->road.heading - sample->road.heading;
    node = sample->road.segmentLength - ((across * SIN(node)) / 4096);
    if (along > node) {
        along = node;
    }

    position = (along << 10) / node;
    *fraction = position;
    *remaining = node - ((node * (s16)position) >> 10);

    if (across < 0) {
        width = sample->road.leftWidth + sample->road.leftExtension;
        nextWidth = nextSample->road.leftWidth + nextSample->road.leftExtension;
        node = width - nextWidth;
        if (node == 0) {
            *lateral = (across << 10) / width;
        } else {
            node = ((node * *fraction) / 1024) + nextWidth;
            *lateral = (across << 10) / node;
        }
    } else {
        width = sample->road.rightWidth + sample->road.rightExtension;
        nextWidth = nextSample->road.rightWidth + nextSample->road.rightExtension;
        node = width - nextWidth;
        if (node == 0) {
            *lateral = (across << 10) / width;
        } else {
            node = ((node * *fraction) / 1024) + nextWidth;
            *lateral = (across << 10) / node;
        }
    }
}

void UpdateCarTrackProjection(RaceVehicle *vehicle)
{
    struct { TrackOffsetArgument x, z; } offset;
    offset.x.value = vehicle->position20.words[0] - TrackNodes[vehicle->trackNodeF0].road.x;
    offset.z.value = vehicle->position20.words[2] - TrackNodes[vehicle->trackNodeF0].road.z;
    ProjectToTrackSegment(offset.x, offset.z, vehicle->trackNodeF0,
        &vehicle->trackFraction170, &vehicle->lateralOffset172, &vehicle->second);
}

s32 FindNearestCarAhead(RaceVehicle *vehicle) {
    s32 best=-1,index=0,selected,limit,count,fresh_count,id;
    s32 first_count,second_count,distance,half,negative_half;
    s32 circumference;
    RaceStandingsTable *table;
    u8 *cursor;
    u8 *reference;
    SceneRaceContext *context;
    u32 flags;
    s32 input;
    u8 *lookup_cursor;
    context=&RaceContext;
    flags=vehicle->flags1E4;
    circumference=context->playerCount14;
    input=vehicle->input;
    table=context->standings;
    flags=(flags&15)+best;
    input++;
    limit=input*16;
    count=NumCars;
    selected=(flags+circumference);
    if(count>0) {
        first_count=count;
        lookup_cursor=(u8*)table;
lookup:
        if(selected==RR_STANDING_ROW(lookup_cursor)->unk00)selected=index;
        else {
            index++;
            lookup_cursor+=sizeof(RaceStandingEntry0C);
            if(index<first_count)goto lookup;
        }
    }
    index=0;
    fresh_count=NumCars;
    if(fresh_count>0) {
        reference=(u8*)table+selected*sizeof(RaceStandingEntry0C);
        second_count=fresh_count;
        circumference=TrackLength;
        cursor=(u8*)table;
        half=circumference/2;
        negative_half=(s32)(0U-circumference)/2;
        do {
            if(index!=selected) {
                id=RR_STANDING_ROW(cursor)->unk00;
                distance=table->entries18[id].totalDist04;
                distance=((u32)distance-RR_STANDING_ROW(reference)->totalDist04);
                distance%=circumference;
                if(distance>=half)distance=((u32)distance-circumference);
                if(distance<negative_half)distance=((u32)distance+circumference);
                if(distance>0&&distance<limit) {
                    best=id;
                    limit=distance;
                    table->gap14=distance;
                }
            }
            index++;
            cursor+=sizeof(RaceStandingEntry0C);
        }while(index<second_count);
    }
    return best;
}

s32 ScoreOvertakeOpportunity(RaceVehicle *arg0, s32 arg1) {
    s32 sp18;
    s32 sp1C;
    s32 s0;
    RaceVehicleControl *s1;
    RaceStandingsTable *t;
    RaceVehicle *other;
    s32 i;
    s32 v1;
    s32 a2;
    s32 a1;
    s16 temp_v0;
    s32 v1_2;

    s0 = 0;
    t = RaceContext.standings;
    s1 = &arg0->control;
    other = t->entries18[arg1].vehicle08;

    i = -other->heading1D4 + 0x400;
    i -= TrackNodes[other->trackNodeF0].road.heading;
    v1 = -((SIN(i) * other->input) / 4096);
    a1 = t->gap14;
    a2 = s1->lateralSpeed58 - v1;
    a1 /= 100;
    if (a1 >= 16) {
        a1 = 15;
    }

    for (i = 0; i < 2; i++) {
        s0 += CarShadowOffsetTableA[i][a1];
    }

    a2 = (a2 * 160) / 1168;
    if (a2 > 0) {
        if (a2 >= 80) {
            a2 = 79;
        }
        a2 /= 5;
        if (GameModeType != 5) {
            s0 += CarShadowOffsetTableA[2][a2];
        } else {
            s0 += CarShadowOffsetTableB[2][a2];
        }
    } else {
        s0 -= 8;
    }

    s1->curveDistance70 = ScanCurveAhead(arg0, 0, &s1->curveSample80, &s1->relDist74, &s1->relDist78);
    s1->previewDistance7C = ScanCurveAhead(arg0, 10, &s1->previewSample84.node, &sp18, &sp1C);
    temp_v0 = CalcSteerLimit(arg0);
    s1->limit6E = temp_v0;
    v1_2 = temp_v0 << 12;

    if (s0 > 0) {
        if (v1_2 < s1->previewDistance7C) {
            s0 = 0;
        }
    }
    return s0;
}

void UpdateComCarOvertake(RaceVehicle *p)
{
  RaceVehicleControl *q;
  RaceStandingsTable *s5;
  RaceVehicle *s1;
  int x8;
  int u1;
  int u2;
  int t8;
  int ab;
  int w1;
  int cnt;
  int flg;
  int n;
  int ab2;
  int th2;
  int c0;
  int e1;
  int e3;
  int s2;
  int s4;
  int s6;
  int a0;
  int v0;
  int v1;
  s6 = 0;
  s4 = -1;
  s5 = RaceContext.standings;
  q = &p->control;
  s2 = FindNearestCarAhead(p);
  if (s2 >= 0)
  {
    s1 = s5->entries18[s2].vehicle08;
    if ((ScoreOvertakeOpportunity(p, s2) > 0) && (q->boostTimer44 == 0))
    {
      if (q->laneActive6A == 0)
      {
        e1 = (u16)q->laneTarget3C;
        q->laneTimer38 = 10;
        q->laneActive6A = 1;
        q->laneMode88 = 0;
        q->laneStart3A = e1;
        ab = s5->gap14;
        if (ab < 0)
        {
          ab = -ab;
        }
        e3 = (short)((u16)p->halfSpan10E);
        e3 = e3 + (e3 / 2);
        if (ab < e3)
        {
          if (s1->lateralOffset172 < p->lateralOffset172)
          {
            q->laneTarget3C = (s1->lateralOffset172 + 0x400) / 2;
          }
          else
          {
            q->laneTarget3C = (s1->lateralOffset172 - 0x400) / 2;
          }
        }
        else
        {
          if (s1->lateralOffset172 > 0)
          {
            q->laneTarget3C = s1->lateralOffset172 - 0x200;
          }
          else
          {
            q->laneTarget3C = s1->lateralOffset172 + 0x200;
          }
        }
        if (q->laneTarget3C < (-0x220))
        {
          q->laneTarget3C = -0x220;
        }
        if (q->laneTarget3C > 0x220)
        {
          q->laneTarget3C = 0x220;
        }
        q->laneDelta6C = ((u16)q->laneTarget3C) - ((u16)q->laneStart3A);
      }
    }
    else
    {
      s2 = -1;
    }
  }
  if (RaceContext.playerCount14 > 0)
  {
    x8 = s5->playerDistance[0];
    if ((x8 < 0) && (q->metric12A < x8))
    {
      s4 = 0;
    }
    if (RaceContext.playerCount14 == 2)
    {
      if ((s5->playerDistance[0] < s5->playerDistance[1]) && (q->metric12A < s5->playerDistance[1]))
      {
        s4 = 1;
      }
    }
    if (s4 >= 0)
    {
      if ((q->laneActive6A == 0) && (q->laneMode88 == 0))
      {
        s1 = &PlayerCars[s4];
        w1 = q->relDist74 + q->relDist78;
        if (w1 < 0)
        {
          if (s1->lateralOffset172 < p->lateralOffset172)
          {
            cnt = 0;
            u1 = (u16)q->laneTarget3C;
            q->laneTimer38 = 10;
            q->laneActive6A = 1;
            q->laneMode88 = 2;
            q->laneStart3A = u1;
          }
        }
        else
        {
          t8 = s1->lateralOffset172;
          if (p->lateralOffset172 < t8)
          {
            u2 = (u16)q->laneTarget3C;
            q->laneTimer38 = 10;
            q->laneActive6A = 1;
            q->laneMode88 = 1;
            q->laneStart3A = u2;
          }
        }
        s2 = 0;
      }
    }
    else
    {
      q->laneMode88 = 0;
    }
  }
  else
  {
    q->laneMode88 = 0;
  }
  if (s2 < 0)
  {
    cnt = 0;
    n = NumCars;
    flg = cnt;
    if (n > 0)
    {
      do
      {
        s1 = s5->entries18[cnt].vehicle08;
        if (s1 != p)
        {
          ab2 = s5->entries18[cnt].totalDist04 - (p->first + p->second);
          if (ab2 < 0)
          {
            ab2 = -ab2;
          }
          th2 = (short)((u16)p->halfSpan10E);
          th2 = th2 + (th2 / 2);
          if (ab2 < th2)
          {
            c0 = q->laneCurrent3E;
            if (c0 < q->laneStart3A)
            {
              if (s1->lateralOffset172 > (c0 - 0x100))
              {
                s6 = 1;
              }
            }
            else
            {
              if (s1->lateralOffset172 < (c0 + 0x100))
              {
                s6 = 1;
              }
            }
          }
        }
        cnt++;
      }
      while (cnt < NumCars);
    }
    switch (q->laneMode88)
    {
      case 1:
        if (q->laneStart3A < q->laneCurrent3E)
      {
        flg = 1;
      }
        break;

      case 2:
        if (q->laneCurrent3E < q->laneStart3A)
      {
        flg = 1;
      }
        break;

    }

    if (((s6 == 0) && (q->laneTimer38 != 0)) && (flg == 0))
    {
      v0 = q->laneCooldown40;
      if (v0 > 0)
      {
        q->laneCooldown40 = v0 - 1;
      }
      else
      {
        q->laneTimer38 = 0;
        q->laneActive6A = 1;
        ComputeCarTrackSurfaceProj(p);
        v1 = (u16)q->laneTarget3C;
        a0 = (u16)q->laneStart3A;
        q->laneMode88 = 3;
        q->laneDelta6C = v1 - a0;
      }
    }
  }
}

static inline s32 extend_track_edge(s32 edge, s32 width, s32 extension)
{
    return edge * (width + extension) / width;
}

/* Interpolate the lateral road position in the horizontal plane. */
void InterpolateLanePointXZ(const TrackSample3C *first, const TrackSample3C *second,
    s32 lateral, s32 fraction, s32 *output)
{
    Vector firstPoint;
    Vector secondPoint;
    s32 offset;
    const TrackSample3C *sample;

    offset = __builtin_abs(lateral);
    sample = first;
    if (lateral < 0) {
        firstPoint.x = extend_track_edge(sample->leftOffsetX04, sample->road.leftWidth, sample->road.leftExtension);
        firstPoint.x = (firstPoint.x * offset) / 1024 + sample->road.x;
        firstPoint.z = extend_track_edge(sample->leftOffsetZ06, sample->road.leftWidth, sample->road.leftExtension);
        firstPoint.z = (firstPoint.z * offset) / 1024 + sample->road.z;
        secondPoint.x = extend_track_edge(second->leftOffsetX04, second->road.leftWidth, second->road.leftExtension);
        secondPoint.x = (secondPoint.x * offset) / 1024 + second->road.x;
        secondPoint.z = extend_track_edge(second->leftOffsetZ06, second->road.leftWidth, second->road.leftExtension);
        secondPoint.z = (secondPoint.z * offset) / 1024 + second->road.z;
    } else {
        firstPoint.x = extend_track_edge(sample->rightOffsetX00, sample->road.rightWidth, sample->road.rightExtension);
        firstPoint.x = (firstPoint.x * offset) / 1024 + sample->road.x;
        firstPoint.z = extend_track_edge(sample->rightOffsetZ02, sample->road.rightWidth, sample->road.rightExtension);
        firstPoint.z = (firstPoint.z * offset) / 1024 + sample->road.z;
        secondPoint.x = extend_track_edge(second->rightOffsetX00, second->road.rightWidth, second->road.rightExtension);
        secondPoint.x = (secondPoint.x * offset) / 1024 + second->road.x;
        secondPoint.z = extend_track_edge(second->rightOffsetZ02, second->road.rightWidth, second->road.rightExtension);
        secondPoint.z = (secondPoint.z * offset) / 1024 + second->road.z;
    }

    output[0] = (secondPoint.x - firstPoint.x) * fraction / 1024 + firstPoint.x;
    output[2] = (secondPoint.z - firstPoint.z) * fraction / 1024 + firstPoint.z;
}

/* The camera path also needs the height of the centre line. */
void InterpolateLanePointXYZ(const TrackSample3C *first, const TrackSample3C *second,
    s32 lateral, s32 fraction, s32 *output)
{
    Vector firstPoint;
    Vector secondPoint;
    s32 offset;
    const TrackSample3C *sample;

    offset = __builtin_abs(lateral);
    sample = first;
    if (lateral < 0) {
        firstPoint.x = extend_track_edge(sample->leftOffsetX04, sample->road.leftWidth, sample->road.leftExtension);
        firstPoint.x = (firstPoint.x * offset) / 1024 + sample->road.x;
        firstPoint.y = sample->road.height;
        firstPoint.z = extend_track_edge(sample->leftOffsetZ06, sample->road.leftWidth, sample->road.leftExtension);
        firstPoint.z = (firstPoint.z * offset) / 1024 + sample->road.z;
        secondPoint.x = extend_track_edge(second->leftOffsetX04, second->road.leftWidth, second->road.leftExtension);
        secondPoint.x = (secondPoint.x * offset) / 1024 + second->road.x;
        secondPoint.y = second->road.height;
        secondPoint.z = extend_track_edge(second->leftOffsetZ06, second->road.leftWidth, second->road.leftExtension);
        secondPoint.z = (secondPoint.z * offset) / 1024 + second->road.z;
    } else {
        firstPoint.x = extend_track_edge(sample->rightOffsetX00, sample->road.rightWidth, sample->road.rightExtension);
        firstPoint.x = (firstPoint.x * offset) / 1024 + sample->road.x;
        firstPoint.y = sample->road.height;
        firstPoint.z = extend_track_edge(sample->rightOffsetZ02, sample->road.rightWidth, sample->road.rightExtension);
        firstPoint.z = (firstPoint.z * offset) / 1024 + sample->road.z;
        secondPoint.x = extend_track_edge(second->rightOffsetX00, second->road.rightWidth, second->road.rightExtension);
        secondPoint.x = (secondPoint.x * offset) / 1024 + second->road.x;
        secondPoint.y = second->road.height;
        secondPoint.z = extend_track_edge(second->rightOffsetZ02, second->road.rightWidth, second->road.rightExtension);
        secondPoint.z = (secondPoint.z * offset) / 1024 + second->road.z;
    }

    output[0] = (secondPoint.x - firstPoint.x) * fraction / 1024 + firstPoint.x;
    output[1] = (secondPoint.y - firstPoint.y) * fraction / 1024 + firstPoint.y;
    output[2] = (secondPoint.z - firstPoint.z) * fraction / 1024 + firstPoint.z;
}

void UpdateComCarTargetPoint(RaceVehicle *vehicle) {
    Vector firstPoint;
    Vector secondPoint;
    Vector target;
    RaceVehicleControl *control;
    s32 fraction;
    s32 firstNode;
    s32 secondNode;
    TrackSample3C *firstSample;
    TrackSample3C *secondSample;
    TrackLaneOffset8 *firstOffset;
    TrackLaneOffset8 *secondOffset;
    s32 delta;

    control = &vehicle->control;
    fraction = 0x400 - vehicle->trackFraction170;
    firstNode = vehicle->trackNodeF0 - fraction / 1024;
    if (firstNode < 0) {
        firstNode += TrackNodeCount;
    }
    secondNode = firstNode - 1;
    if (secondNode < 0) {
        secondNode += TrackNodeCount;
    }
    firstSample = &TrackNodes[firstNode];
    secondSample = &TrackNodes[secondNode];
    fraction &= 0x3FF;
    if (control->laneActive6A == 0) {
        switch (control->laneTimer38) {
        case 0:
            firstOffset = &RaceContext.laneOffsets54C[firstNode];
            firstPoint.x = firstSample->road.x + firstOffset->x;
            firstPoint.z = firstSample->road.z + firstOffset->z;
            secondOffset = &RaceContext.laneOffsets54C[secondNode];
            secondPoint.x = secondSample->road.x + secondOffset->x;
            secondPoint.z = secondSample->road.z + secondOffset->z;
            target.x = (secondPoint.x - firstPoint.x) * fraction / 1024 + firstPoint.x;
            target.z = (secondPoint.z - firstPoint.z) * fraction / 1024 + firstPoint.z;
            break;
        case 10:
            InterpolateLanePointXZ(firstSample, secondSample, control->laneStart3A, fraction, (s32 *)&target);
            break;
        }
    } else {
        delta = control->laneDelta6C >= 0 ? control->laneDelta6C : -control->laneDelta6C;
        if (delta < 0x10) {
            control->laneActive6A = 0;
            control->laneStart3A = control->laneTarget3C;
            if (control->laneMode88 == 3) {
                control->laneMode88 = 0;
            }
        } else {
            if (control->laneDelta6C > 0) {
                control->laneDelta6C -= 14;
                if (control->laneDelta6C < 0) {
                    control->laneDelta6C = 0;
                }
            } else {
                control->laneDelta6C += 14;
                if (control->laneDelta6C > 0) {
                    control->laneDelta6C = 0;
                }
            }
            control->laneStart3A = control->laneTarget3C - control->laneDelta6C;
            if (control->laneStart3A < -800) {
                control->laneStart3A = -800;
            }
            if (control->laneStart3A > 800) {
                control->laneStart3A = 800;
            }
            control->laneDelta6C = control->laneTarget3C - control->laneStart3A;
        }
        InterpolateLanePointXZ(firstSample, secondSample, control->laneStart3A, fraction, (s32 *)&target);
    }
    vehicle->control.pathDeltaX30 = target.x - vehicle->position20.words[0];
    vehicle->control.pathDeltaZ34 = target.z - vehicle->position20.words[2];
}

/* Initial values of the globals this module owns. */
s8 CarShadowOffsetTableA[6][16] = {{0, 0, 0, 0, 0, -1, -2, -3, -4, -5, -6, -7, -7, -7, -7, -7}, {8, 8, 7, 6, 5, 3, 1}, {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8}, {-8, -8, -8, -8, -8, -8, -8, -8}, {1, 2, 3, 4, 5, 6, 7, 8, 8, 7, 6, 5, 4, 3, 2, 1}};
s8 CarShadowOffsetTableB[8][16] = {{0, 0, 0, 0, 0, -1, -2, -3, -4, -5, -6, -6, -6, -6, -6, -6}, {8, 8, 7, 6, 5, 3, 1}, {8, 7, 4, 4, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8}, {-8, -8, -8, -8, -8, -8, -8, -8}, {1, 2, 3, 4, 5, 6, 7, 8, 8, 7, 6, 5, 4, 3, 2, 1}};
s16 SinTable[1032] = {0, 6, 13, 19, 25, 31, 38, 44, 50, 57, 63, 69, 75, 82, 88, 94, 101, 107, 113, 119, 126, 132, 138, 144, 151, 157, 163, 170, 176, 182, 188, 195, 201, 207, 214, 220, 226, 232, 239, 245, 251, 257, 264, 270, 276, 283, 289, 295, 301, 308, 314, 320, 326, 333, 339, 345, 351, 358, 364, 370, 376, 383, 389, 395, 401, 408, 414, 420, 426, 433, 439, 445, 451, 458, 464, 470, 476, 483, 489, 495, 501, 508, 514, 520, 526, 533, 539, 545, 551, 557, 564, 570, 576, 582, 589, 595, 601, 607, 613, 620, 626, 632, 638, 644, 651, 657, 663, 669, 675, 682, 688, 694, 700, 706, 713, 719, 725, 731, 737, 744, 750, 756, 762, 768, 774, 781, 787, 793, 799, 805, 811, 818, 824, 830, 836, 842, 848, 854, 861, 867, 873, 879, 885, 891, 897, 904, 910, 916, 922, 928, 934, 940, 946, 953, 959, 965, 971, 977, 983, 989, 995, 1001, 1007, 1014, 1020, 1026, 1032, 1038, 1044, 1050, 1056, 1062, 1068, 1074, 1080, 1086, 1092, 1099, 1105, 1111, 1117, 1123, 1129, 1135, 1141, 1147, 1153, 1159, 1165, 1171, 1177, 1183, 1189, 1195, 1201, 1207, 1213, 1219, 1225, 1231, 1237, 1243, 1249, 1255, 1261, 1267, 1273, 1279, 1285, 1291, 1297, 1303, 1309, 1315, 1321, 1327, 1332, 1338, 1344, 1350, 1356, 1362, 1368, 1374, 1380, 1386, 1392, 1398, 1404, 1409, 1415, 1421, 1427, 1433, 1439, 1445, 1451, 1457, 1462, 1468, 1474, 1480, 1486, 1492, 1498, 1503, 1509, 1515, 1521, 1527, 1533, 1538, 1544, 1550, 1556, 1562, 1567, 1573, 1579, 1585, 1591, 1596, 1602, 1608, 1614, 1620, 1625, 1631, 1637, 1643, 1648, 1654, 1660, 1666, 1671, 1677, 1683, 1689, 1694, 1700, 1706, 1711, 1717, 1723, 1729, 1734, 1740, 1746, 1751, 1757, 1763, 1768, 1774, 1780, 1785, 1791, 1797, 1802, 1808, 1813, 1819, 1825, 1830, 1836, 1842, 1847, 1853, 1858, 1864, 1870, 1875, 1881, 1886, 1892, 1898, 1903, 1909, 1914, 1920, 1925, 1931, 1936, 1942, 1947, 1953, 1958, 1964, 1970, 1975, 1981, 1986, 1992, 1997, 2002, 2008, 2013, 2019, 2024, 2030, 2035, 2041, 2046, 2052, 2057, 2062, 2068, 2073, 2079, 2084, 2090, 2095, 2100, 2106, 2111, 2117, 2122, 2127, 2133, 2138, 2143, 2149, 2154, 2159, 2165, 2170, 2175, 2181, 2186, 2191, 2197, 2202, 2207, 2213, 2218, 2223, 2228, 2234, 2239, 2244, 2249, 2255, 2260, 2265, 2270, 2276, 2281, 2286, 2291, 2296, 2302, 2307, 2312, 2317, 2322, 2328, 2333, 2338, 2343, 2348, 2353, 2359, 2364, 2369, 2374, 2379, 2384, 2389, 2394, 2399, 2405, 2410, 2415, 2420, 2425, 2430, 2435, 2440, 2445, 2450, 2455, 2460, 2465, 2470, 2475, 2480, 2485, 2490, 2495, 2500, 2505, 2510, 2515, 2520, 2525, 2530, 2535, 2540, 2545, 2550, 2555, 2559, 2564, 2569, 2574, 2579, 2584, 2589, 2594, 2598, 2603, 2608, 2613, 2618, 2623, 2628, 2632, 2637, 2642, 2647, 2652, 2656, 2661, 2666, 2671, 2675, 2680, 2685, 2690, 2694, 2699, 2704, 2709, 2713, 2718, 2723, 2727, 2732, 2737, 2741, 2746, 2751, 2755, 2760, 2765, 2769, 2774, 2779, 2783, 2788, 2792, 2797, 2802, 2806, 2811, 2815, 2820, 2824, 2829, 2833, 2838, 2843, 2847, 2852, 2856, 2861, 2865, 2870, 2874, 2878, 2883, 2887, 2892, 2896, 2901, 2905, 2910, 2914, 2918, 2923, 2927, 2932, 2936, 2940, 2945, 2949, 2953, 2958, 2962, 2967, 2971, 2975, 2979, 2984, 2988, 2992, 2997, 3001, 3005, 3009, 3014, 3018, 3022, 3026, 3031, 3035, 3039, 3043, 3048, 3052, 3056, 3060, 3064, 3068, 3073, 3077, 3081, 3085, 3089, 3093, 3097, 3102, 3106, 3110, 3114, 3118, 3122, 3126, 3130, 3134, 3138, 3142, 3146, 3150, 3154, 3158, 3162, 3166, 3170, 3174, 3178, 3182, 3186, 3190, 3194, 3198, 3202, 3206, 3210, 3214, 3217, 3221, 3225, 3229, 3233, 3237, 3241, 3244, 3248, 3252, 3256, 3260, 3264, 3267, 3271, 3275, 3279, 3282, 3286, 3290, 3294, 3297, 3301, 3305, 3309, 3312, 3316, 3320, 3323, 3327, 3331, 3334, 3338, 3342, 3345, 3349, 3352, 3356, 3360, 3363, 3367, 3370, 3374, 3378, 3381, 3385, 3388, 3392, 3395, 3399, 3402, 3406, 3409, 3413, 3416, 3420, 3423, 3426, 3430, 3433, 3437, 3440, 3444, 3447, 3450, 3454, 3457, 3461, 3464, 3467, 3471, 3474, 3477, 3481, 3484, 3487, 3490, 3494, 3497, 3500, 3504, 3507, 3510, 3513, 3516, 3520, 3523, 3526, 3529, 3532, 3536, 3539, 3542, 3545, 3548, 3551, 3555, 3558, 3561, 3564, 3567, 3570, 3573, 3576, 3579, 3582, 3585, 3588, 3591, 3594, 3597, 3600, 3603, 3606, 3609, 3612, 3615, 3618, 3621, 3624, 3627, 3630, 3633, 3636, 3639, 3642, 3644, 3647, 3650, 3653, 3656, 3659, 3661, 3664, 3667, 3670, 3673, 3675, 3678, 3681, 3684, 3686, 3689, 3692, 3695, 3697, 3700, 3703, 3705, 3708, 3711, 3713, 3716, 3719, 3721, 3724, 3727, 3729, 3732, 3734, 3737, 3739, 3742, 3745, 3747, 3750, 3752, 3755, 3757, 3760, 3762, 3765, 3767, 3770, 3772, 3775, 3777, 3779, 3782, 3784, 3787, 3789, 3791, 3794, 3796, 3798, 3801, 3803, 3805, 3808, 3810, 3812, 3815, 3817, 3819, 3822, 3824, 3826, 3828, 3831, 3833, 3835, 3837, 3839, 3842, 3844, 3846, 3848, 3850, 3852, 3854, 3857, 3859, 3861, 3863, 3865, 3867, 3869, 3871, 3873, 3875, 3877, 3879, 3881, 3883, 3885, 3887, 3889, 3891, 3893, 3895, 3897, 3899, 3901, 3903, 3905, 3907, 3909, 3910, 3912, 3914, 3916, 3918, 3920, 3921, 3923, 3925, 3927, 3929, 3930, 3932, 3934, 3936, 3937, 3939, 3941, 3943, 3944, 3946, 3948, 3949, 3951, 3953, 3954, 3956, 3958, 3959, 3961, 3962, 3964, 3965, 3967, 3969, 3970, 3972, 3973, 3975, 3976, 3978, 3979, 3981, 3982, 3984, 3985, 3987, 3988, 3989, 3991, 3992, 3994, 3995, 3996, 3998, 3999, 4001, 4002, 4003, 4005, 4006, 4007, 4008, 4010, 4011, 4012, 4014, 4015, 4016, 4017, 4019, 4020, 4021, 4022, 4023, 4024, 4026, 4027, 4028, 4029, 4030, 4031, 4032, 4034, 4035, 4036, 4037, 4038, 4039, 4040, 4041, 4042, 4043, 4044, 4045, 4046, 4047, 4048, 4049, 4050, 4051, 4052, 4053, 4053, 4054, 4055, 4056, 4057, 4058, 4059, 4060, 4060, 4061, 4062, 4063, 4064, 4064, 4065, 4066, 4067, 4067, 4068, 4069, 4070, 4070, 4071, 4072, 4072, 4073, 4074, 4074, 4075, 4076, 4076, 4077, 4077, 4078, 4079, 4079, 4080, 4080, 4081, 4081, 4082, 4082, 4083, 4083, 4084, 4084, 4085, 4085, 4086, 4086, 4087, 4087, 4088, 4088, 4088, 4089, 4089, 4089, 4090, 4090, 4090, 4091, 4091, 4091, 4092, 4092, 4092, 4092, 4093, 4093, 4093, 4093, 4094, 4094, 4094, 4094, 4094, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096};
