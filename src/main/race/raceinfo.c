#include "rr/field_rate.h"
#include "rr/video_timing.h"
#include "game/data/menuvars.h"
#include "rr/runtime_settings.h"
#include "rr/vehicle_table.h"
#include "rr/menu_selection.h"
#include "rr/projection_state.h"
#include "game/menu/menu13.h"
#include "psyq/libgte/gtemat5.h"
#include "rr/frame_storage_types.h"
#include "psyq/libgte/gtemat4.h"
#include "psyq/libgte/gtemat3.h"
#include "scratchpad.h"
#include "rr/race.h"
#include "game/ui/window1.h"
#include "game/ui/hudmap2.h"
#include "game/ui/uipart1.h"
#include "game/data/ldscn1.h"
#include "game/race/raceinfo.h"
#include "game/race/ranking.h"
#include "rr/track_sample_types.h"
#include "rr/overlay662_distance.h"
#include "rr/matrix_workspace.h"
#include "game/race/raceset.h"
#include "game/math/rotmat.h"
#include "game/ui/badge1.h"
#include "rr/badge_rows.h"
#include "rr/menu_label_render.h"
#include "game/data/cdfile2.h"
#include "rr/trig.h"


extern signed char CourseInfoRowOrder[];
extern UiUvRect CourseInfoLabelUvs[9];
extern short CourseLengthMetersTable[];
extern short ComDifficultyLevel;
extern int TrackNodeCount;
extern short ReverseCourse;
extern short RaceLapSplitFlags[];

void DrawCourseInfoPanel(int arg0, int arg1);
void DrawCourseMapPreview(int unusedSelection,int unusedMode);
void UpdateRaceIntroCamera(RaceVehicle *a0, int a1);

void DrawCourseInfoPanel(int arg0, int arg1) {
    char sp28[8];
    int i;
    int s4;
    void *p;
    char *b;

    p = RR_GPU_SCRATCH->cursor;
    for (i = 0; i < 4; i++) {
        int v = CourseInfoRowOrder[i + (arg1 << 2)];
        int y;
        if (v == -1) {
            break;
        }
        y = 0x128 + i * 0x10;
        p = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], p, 0x16, y, &CourseInfoLabelUvs[v], 0x7FA4, 1);
        if (i == 1) {
            p = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], p, 0x4A, y, &CourseInfoLabelUvs[8], 0x7FA4, 1);
        }
    }

    s4 = arg0 >= 8;
    arg0 &= 7;
    p = AddSpritePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], p, 0x3A, 0x104, &CourseInfoLabelUvs[6 + s4], s4 + 0x75E4, 0);
    p = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], p, 0x59);
    RR_GPU_SCRATCH->cursor = AddTilePrim(&CurrentDrawBuf->buffer.ordering70.tags[0], p, 0x10, 0xE0, 2, 0x88, 0x4C, 0x4C, 0x4C);

    if (GameModeType < 2) {
        NumLaps = CourseLapCounts[GameModeType == 1 ? 1 : 0][CourseIndex];
    }

    i = 7;
    b = sp28 + 7;
    do {
        *b = 0;
        i--;
        b--;
    } while (i >= 0);

    for (i = 0; i < 2; i++) {
        if (i == 0) {
            FormatDecimalDigits(sp28, NumLaps, 0);
        } else {
            FormatDecimalDigits(sp28, CourseLengthMetersTable[arg0], 3);
        }
        DrawDigitString16(&CurrentDrawBuf->buffer.ordering70.tags[0], 0x2E + i * 8, 0x128 + i * 0x10, sp28);
    }

    if (arg1 == 0) {
        for (i = 0; i < 2; i++) {
            int val;
            if (GameModeType == 1) {
                if (i == 0) {
                    val = CourseBestLapRecords[CourseIndex];
                } else {
                    val = CourseBestTotalTimes[CourseIndex];
                }
            } else {
                if (i == 0) {
                    val = CourseBestLap[s4][CourseIndex];
                } else {
                    val = CourseBestTotal[s4][CourseIndex];
                }
            }
            DrawDigitString16(&CurrentDrawBuf->buffer.ordering70.tags[0], 0x44 + i * 8, 0x148 + i * 0x10, FormatMillisecondsTime(val));
        }
    } else if (arg1 == 1) {
        i = 7;
        b = sp28 + 7;
        do {
            *b = 0;
            i--;
            b--;
        } while (i >= 0);

        for (i = 0; i < 2; i++) {
            if (i == 0) {
                FormatDecimalDigits(sp28, NumCars - 2, 0);
            } else {
                FormatDecimalDigits(sp28, ComDifficultyLevel + 1, 0);
            }
            DrawDigitString16(&CurrentDrawBuf->buffer.ordering70.tags[0], 0x3A - i * 0xC, 0x148 + i * 0x10, sp28);
        }
    }

    RR_GPU_SCRATCH->cursor = EmitSpinningWheelIcon(&CurrentDrawBuf->buffer.ordering70.tags[0], RR_GPU_SCRATCH->cursor, 0x16, 0xE4, arg0);
}

void DrawCourseMapPreview(int unusedSelection,int unusedMode) {
    struct { int x, y, z; } v;
    int i, r;
    TrackSample3C *t;
    if (ReverseCourse != 0) {
        i = (TrackPreviewTick >> 2) % TrackNodeCount;
    } else {
        int n = TrackNodeCount;
        r = (TrackPreviewTick >> 2) % n;
        r++;
        i = n - r;
    }
    t = &TrackNodes[i];
    v.x = t->road.x;
    v.z = t->road.z;
    TrackPreviewTick += 1;
    DrawMapBitmapIso(&CurrentDrawBuf->buffer.ordering70.tags[0], (u8 *)TrackNodesEnd, &v.x);
}

void UpdateRaceIntroCamera(RaceVehicle *a0, int a1)
{
  int sp10[3];
  MatrixWords32 m20;
  MatrixWords32 m40;
  ProjectionView32 *g = &((ProjectionScratchPrefix *)SCRATCHPAD)->pose08;
  int s0;
  int v0;
  int v1;
  int i;
  int k;
  short *s5;
  /* A 75-frame orbit (NTSC release: 90 frames). */
  const int frames = RR_PAL_NTSC(75, 90);
  /* Called every field at field rate: the orbit by the field (its time in
   * fields over 2 * frames), the rumble's save and restore by the tick
   * (rr/field_rate.h). */
  const int tick = RaceSlowTick();
  const int ft = RaceFieldTimeAfter(a1);
  const int fframes = frames * RaceFieldsPerTick();
  if (a1 >= frames)
  {
    return;
  }
  if (a1 < 2)
  {
    ResetCarSlotBuffers();
    RaceLapSplitFlags[0] = 0;
    RaceLapSplitFlags[2] = -0x100;
    RaceLapSplitFlags[1] = -0x100;
    if ((a1 == 1) && (a0 == PlayerCars) && tick)
    {
      /* Save each wheel's rumble setting and force mode 2 for the intro. */
      k = 2;
      for (i = 0; i < 2; i++)
      {
        RaceContext.introSavedRumbleA8[i] = Options.wheel[i].rumble02;
        Options.wheel[i].rumble02 = k;
      }
    }
  }
  else
  {
    s0 = (ft << 11) / fframes;
    v0 = rsin(s0);
    s5 = RaceLapSplitFlags;
    if (v0 < 0)
    {
      v0 += 7;
    }
    s5[0] = v0 >> 3;
    v0 = -(rcos(s0) << 9);
    if (v0 < 0)
    {
      v0 += 0xFFF;
    }
    s5[2] = v0 >> 12;
    s5[1] = ((ft << 8) / fframes) - 0x200;
  }
  BuildRotMatrixY(&m20, 0x800 - a0->rot54);
  BuildRotMatrixX(&m40, a0->rot50);
  MulMatrix(&m20, &m40);
  BuildRotMatrixZ(&m40, a0->rot58);
  MulMatrix2(&m20, &m40);
  ApplyMatrix(&m40, (const Vec3s8 *)(RaceLapSplitFlags), (Vec3i *)(sp10));
  g->position00.words[0] = a0->worldPosition10.words[0] + sp10[0];
  g->position00.words[1] = a0->worldPosition10.words[1] + sp10[1];
  g->position00.words[2] = a0->worldPosition10.words[2] + sp10[2];
  v1 = rsin(i = a0->rot54) * 0x60;
  if (v1 < 0)
  {
    v1 += 0xFFF;
  }
  sp10[0] = (a0->worldPosition10.words[0] + (v1 >> 12)) - g->position00.words[0];
  sp10[1] = a0->worldPosition10.words[1] - g->position00.words[1];
  v1 = rcos(a0->rot54) * 0x60;
  if (v1 < 0)
  {
    v1 += 0xFFF;
  }
  v1 >>= 12;
  sp10[2] = (a0->worldPosition10.words[2] + v1) - g->position00.words[2];
  v0 = Atan2Table(sp10[0], sp10[2]);
  g->angles10.words[1] = 0x400 - v0;
  v0 = CordicSqrt((sp10[0] * sp10[0]) + (sp10[2] * sp10[2]));
  v0 = Atan2Table(sp10[1], v0 >> 6);
  g->angles10.words[0] = 0x400 - v0;
  g->angles10.words[2] = 0;
  if (a1 < frames - 2)
  {
    return;
  }
  a0->flags1E4 |= 0x100;
  if ((a1 == frames - 2) && (a0 == PlayerCars) && tick)
  {
    for (i = 0; i < 2; i++)
    {
      Options.wheel[i].rumble02 = (s8)RaceContext.introSavedRumbleA8[i];
    }
  }
}

/* Initial values of the globals this module owns. */
/* 0x8009F654: nine UV rects. Rows 6..8 are the PS1 map's
 * CourseInfoDirectionUvs (0x8009F66C); the label loop draws row 8 too. */
UiUvRect CourseInfoLabelUvs[9] = {{.v = 0x68, .w = 0x18, .h = 0x10}, {.u = 0x18, .v = 0x68, .w = 0x20, .h = 0x10}, {.u = 0x3C, .v = 0x68, .w = 0x30, .h = 0x10}, {.u = 0x6C, .v = 0x68, .w = 0x38, .h = 0x10}, {.u = 0xA4, .v = 0x68, .w = 0x24, .h = 0x10}, {.u = 0xC8, .v = 0x68, .w = 0x18, .h = 0x10}, {.v = 0xD8, .w = 0x10, .h = 0x20}, {.u = 0x10, .v = 0xD8, .w = 0x10, .h = 0x20}, {.u = 0x64, .v = 0x58, .w = 8, .h = 0x10}};
short CourseLengthMetersTable[8] = {5220, 6686, 5665, 5564, 3012, 5868, 6456, 3958};
signed char CourseInfoRowOrder[12] = {0, 1, 2, 3, 0, 1, 4, 5, 0, 1, -1, -1};
int TrackPreviewTick;
short RaceLapSplitFlags[4];
