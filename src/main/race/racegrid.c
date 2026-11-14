#include "rr/release.h"
#include "rr/video_timing.h"
#include "rr/projection_state.h"
#include "game/menu/menu8.h"
#include "game/sound/sndvab.h"
#include "rr/save_slots.h"
#include "rr/geometry.h"
#include "game/sound/sndque.h"
#include "game/ui/badgeprim.h"
#include "game/ui/badge1.h"
#include "rr/gpu_scratch_types.h"
#include "game/car/carbound.h"
#include "psyq/libgte/gtemat5.h"
#include "rr/frame_storage_types.h"
#include "psyq/libgte/gtemat4.h"
#include "psyq/libgte/gtemat3.h"
#include "types.h"
#include "game/car/car.h"
#include "scratchpad.h"
#include "game/system/pad.h"
#include "game/ui/hudmap.h"
#include "game/race/raceai2.h"
#include "game/race/racegrid.h"
#include "rr/race.h"
#include "rr/menu_selection.h"
#include "rr/matrix_workspace.h"
#include "rr/audio_loop.h"
#include "rr/driver_control.h"
#include "rr/bios_events.h"
#include "rr/serial_handles.h"
#include "game/gfx/gfxotag.h"
#include "game/math/rotmat.h"
#include "game/system/pad3.h"

typedef struct { int a, b, c, d; } w3g2V;

typedef struct {
    short x, y, z;
} Vec3s6;

extern PolyF3Packet *EmitPolyF3();
extern Vec3s6 StartSignalLightOffsetsA[];
extern Vec3s6 StartSignalLightOffsetsB[];
#include "game/course/trkintp.h"
extern int ViewUsesCarTrackNode;
extern s32 RaceState;
extern int RaceVoiceFlags;
extern short FadeCountdown;
extern unsigned char PauseMenuMaxChoices[];
extern signed char PauseCursor;
extern int PauseAbortNextModes[];
extern int TrackLength;
extern short ModeIndex;
extern int TotalRaceTime;
extern short ModeGroup;
extern short SubmodeState;
#include "game/gfx/spremit.h"
/* Aggregate coordinate arguments still need one recovered public contract. */
extern unsigned char PauseMenuChoiceMap[][4];
extern char SioDeviceName[];
extern int LinkWriteHandle;
extern int LinkReadHandle;
extern s16 LinkState;
extern int RaceStartSignalState;
extern int RaceTimeRemaining;
extern short LinkHostFlag;

void SetupMenuCarCamera(int a);
void PlayStartCountdownVoice(int a0);
void InitPlayerRumble(void);
void UpdatePauseMenu(int a0);
void ApplyPauseMenuChoice(int arg0);
int LinkTimeoutCallback(int a0, unsigned int a1);
void BeginLinkSync(void);
int ExitOnLinkLost(void);
int UpdateLinkHandshake(int a0);
int PollLinkStatus(void);

void SetupMenuCarCamera(int a)
{
  ProjectionView32 *g = &((ProjectionScratchPrefix *)SCRATCHPAD)->pose08;
  int i;
  g->position00.words[0] = StartSignalLightOffsetsB[a].x;
  g->position00.words[1] = StartSignalLightOffsetsB[a].y;
  g->position00.words[2] = StartSignalLightOffsetsB[a].z;
  g->angles10.words[0] = StartSignalLightOffsetsA[a].x;
  g->angles10.words[1] = StartSignalLightOffsetsA[a].y;
  g->angles10.words[2] = StartSignalLightOffsetsA[a].z;
  BuildCameraMatrices();
  ScaleMatrixRow1(2, (MatrixWords32 *)SCRATCH_ROT_MATRIX);
  if (PlayerControllers[ActivePlayerIndex & 1].input00.fields.buttons & 4)
  {
    PlayerCars[ActivePlayerIndex & 1].lateralMotion178 += 0xC0;
  }
  else
    if (PlayerControllers[ActivePlayerIndex & 1].input00.fields.buttons & 8)
  {
    PlayerCars[ActivePlayerIndex & 1].lateralMotion178 -= 0xC0;
  }
  i = PlayerCars[ActivePlayerIndex & 1].lateralMotion178;
  if (i > 0xF00)
  {
    PlayerCars[ActivePlayerIndex & 1].lateralMotion178 = 0xF00;
  }
  else
    if (i < (-0xF00))
  {
    PlayerCars[ActivePlayerIndex & 1].lateralMotion178 = -0xF00;
  }
}

void UpdateRaceChaseCamera(RaceVehicle *p) {
    struct {
        MatrixWords32 a, b;
        short v[3], reserved46;
        int w[3];
    } s;
    ProjectionView32 *g = &((ProjectionScratchPrefix *)SCRATCHPAD)->pose08;
    int r;
    short t;
    unsigned short a1, a2;
    ViewUsesCarTrackNode = 1;
    if ((u16)p->flags1E4 & 0x100) {
        BuildRotMatrixY(&s.a, 0x800 - p->rot54);
        BuildRotMatrixX(&s.b, p->rot50);
        MulMatrix(&s.a, &s.b);
        BuildRotMatrixZ(&s.b, p->rot58);
        MulMatrix2(&s.a, &s.b);
        s.v[0] = 0;
        s.v[1] = -0x64;
        if (p->height108 < 0x28) s.v[1] = (unsigned short)p->height108 - 0x8C;
        s.v[2] = 0;
        ApplyMatrix(&s.b, (const Vec3s8 *)(s.v), (Vec3i *)(s.w));
        *(w3g2V *)g->position00.words = *(w3g2V *)&p->worldPosition10.words[0];
        g->position00.words[0] -= p->ir40[0];
        g->position00.words[2] -= p->ir40[2];
        *(w3g2V *)g->angles10.words = *(w3g2V *)&p->rot50;
        g->angles10.words[0] += p->renderAngleX1AC * 2;
        a1 = (u16)p->shakeRollOffset1B6;
        a2 = (u16)p->renderAngleZ1AE;
        g->position00.words[0] += s.w[0];
        g->position00.words[1] += s.w[1];
        g->position00.words[2] += s.w[2];
        g->angles10.words[2] += ((short)a1 / 2) + ((short)a2 / 2);
        return;
    }
    s.v[0] = (s16)AngleDiffSigned(g->angles10.words[0], p->rot50) / 8;
    r = (s16)AngleDiffSigned(g->angles10.words[1], p->rot54);
    t = r / 6;
    g->angles10.words[0] += s.v[0];
    s.v[1] = t;
    g->angles10.words[1] += t;
    BuildRotMatrixY(&s.a, 0x800 - g->angles10.words[1]);
    BuildRotMatrixX(&s.b, g->angles10.words[0]);
    MulMatrix(&s.a, &s.b);
    BuildRotMatrixZ(&s.b, g->angles10.words[2]);
    MulMatrix2(&s.a, &s.b);
    s.v[0] = 0;
    s.v[1] = -0xFA;
    if (p->height108 < 0x28) s.v[1] = (unsigned short)p->height108 - 0x122;
    s.v[2] = 0x2D0;
    ApplyMatrix(&s.b, (const Vec3s8 *)(s.v), (Vec3i *)(s.w));
    *(w3g2V *)g->position00.words = *(w3g2V *)&p->worldPosition10.words[0];
    g->angles10.words[2] = p->rot58 - p->wheelRotZ1AA;
    g->position00.words[0] += s.w[0];
    g->position00.words[1] += s.w[1];
    g->position00.words[2] += s.w[2];
}

void DrawStartCountdown(GpuDmaTag *ordering, s32 frame) {
    UiUvRect glyph;
    s32 y;
    s32 index;
    s32 xOffset;
    s32 phase;
    s32 top;
    s32 tick;
    s32 palette;
    s32 intensity;
    void *cursor;

    /* Seven phases of a second from frame 75 (NTSC release: of 30 frames
     * from frame 90). */
    const s32 rate = RR_PAL_NTSC(25, 30);
    const s32 start = RR_PAL_NTSC(0x4B, 0x5A);

    if (RaceState != 0 && (s32)(frame - (u32)start) < rate * 7) {
        tick = frame % rate;
        phase = (frame - start) / rate;
        if ((&CurrentDrawBuf->buffer.ordering70.tags[0]) == ordering) {
            top = 0;
            if (phase == 6) {
                top = -(tick * 128) / rate;
            }
        } else {
            /* Split screen: the lower half (Japanese release: 240 lines). */
            RR_GPU_SCRATCH->cursor = AddDrawAreaPrim(ordering, RR_GPU_SCRATCH->cursor, 0, 0, 0x140, RR_SCREEN_BOTTOM);
            top = RR_PAL_JP(0x40, 0x38);
        }
        cursor = AddDrawModePrim(ordering, (GpuDrawModePacket *)RR_GPU_SCRATCH->cursor, 5);
        if (phase == 1) {
            intensity = ((tick * 64) / rate) + 0x40;
        } else if (phase >= 4) {
            intensity = 0xA4;
            if (phase >= 5) {
                intensity = 0x80;
            }
        } else {
            intensity = 0x80;
            if (phase <= 0) {
                intensity = 1;
            }
        }
        if (GameModeType == 1) {
            if (phase >= 4) {
                palette = ((frame >> 2) & 1) + 2;
                glyph = PauseMenuLabelUvs[7];
                cursor = AddSpritePrim(ordering, cursor, PauseRowOffsetX[3] + 0x60, top + 0x39, &glyph, PauseRowClutIds[palette], 0);
            } else if (phase > 0) {
                xOffset = 0;
                for (index = 0; index < 0x10;) {
                    s32 b, c;

                    if (index > tick - RR_PAL_NTSC(7, 12)) {
                        break;
                    }
                    b = index & 1;
                    c = b << 1;
                    cursor = AddSpritePrim(ordering, cursor, xOffset + 0x78, top + 0x39, &PauseMenuLabelUvs[2 + b], PauseRowClutIds[b * 2], 0);
                    index++;
                    xOffset += 1 << (c + 1);
                }
                palette = 0;
                glyph = PauseMenuLabelUvs[5];
                glyph = PauseMenuLabelUvs[6];
                glyph.u += (phase - 1) * 12;
                cursor = AddSpritePrim(ordering, cursor, PauseRowOffsetX[2] + 0x60, top + 0x39, &glyph, PauseRowClutIds[0], 0);
            }
        } else {
            if (phase == 0) {
                xOffset = 0;
                for (index = 0; index < 0x10;) {
                    s32 b, c;

                    if (index > tick - 6) {
                        break;
                    }
                    b = index & 1;
                    c = b << 1;
                    cursor = AddSpritePrim(ordering, cursor, xOffset + 0x78, top + 0x39, &PauseMenuLabelUvs[2 + b], PauseRowClutIds[b * 2], 0);
                    index++;
                    xOffset += 1 << (c + 1);
                }
            } else if (phase >= 4) {
                palette = ((frame >> 2) & 1) + 2;
                glyph = PauseMenuLabelUvs[5];
                glyph.u += 0x60;
                for (index = 0; index < 3; index++) {
                    if (index == 2) {
                        glyph = PauseMenuLabelUvs[7];
                        index = 3;
                    }
                    cursor = AddSpritePrim(ordering, cursor, PauseRowOffsetX[index] + 0x60, top + 0x39, &glyph, PauseRowClutIds[palette], 0);
                }
            } else {
                palette = 0;
                glyph = PauseMenuLabelUvs[5];
                for (index = 0; index < 3; index++) {
                    if (index == 0) {
                        glyph.u += ((tick * 9) / rate) * 12;
                    } else if (index == 1) {
                        glyph.u = PauseMenuLabelUvs[5].u + phase * 0x18;
                    } else {
                        glyph = PauseMenuLabelUvs[6];
                        glyph.u += (phase - 1) * 12;
                    }
                    cursor = AddSpritePrim(ordering, cursor, PauseRowOffsetX[index] + 0x60, top + 0x39, &glyph, PauseRowClutIds[palette], 0);
                }
            }
        }
        if (phase > 0) {
            y = top + 0x39;
            xOffset = 0;
            for (index = 0; index < 10; index++) {
                cursor = AddSpritePrim(ordering, cursor, xOffset + 0x78, y, &PauseMenuLabelUvs[4], PauseRowClutIds[(index - 3U) < 4U ? 3 : palette], 0);
                xOffset += 8;
            }
        }
        cursor = AddSpritePrim(ordering, cursor, 0x68, top + 0x1A, &PauseMenuLabelUvs[1], PauseTitleClutIds[phase >= 4], (intensity << 1) | 1);
        cursor = AddSpritePrim(ordering, cursor, 0x60, top, &PauseMenuLabelUvs[0], 0x797C, 0);
        RR_GPU_SCRATCH->cursor = AddDrawModePrim(ordering, cursor, 0x3F);
        if (ordering == &CurrentDrawBuf->buffer.secondaryOrderingB70.tags[0]) {
            s32 y;
            if (phase == 6) {
                intensity = ((rate - tick) * 0x18) / rate;
            } else {
                intensity = 0x18;
            }
            y = (0x18 - intensity) / 2 + 0x34;
            RR_GPU_SCRATCH->cursor = AddDrawAreaPrim(ordering, RR_GPU_SCRATCH->cursor, 0x70, top + y, 0x60, intensity);
        }
    }
}

void PlayStartCountdownVoice(int a0) {
    /* Countdown frames of each voice cue: PAL 25 a second, NTSC 30. */
    if (RaceState != 0 && a0 < RR_PAL_NTSC(0x115, 0x14C)) {
        if (a0 == RR_PAL_NTSC(0xC3, 0xE6)) {
            FadeCountdown = 0x1E;
        }
        if (a0 == RR_PAL_NTSC(0x65, 0x79)) {
            if (!(RaceVoiceFlags & 1)) {
                SoundQueueEnqueue(0, 0);
                RaceVoiceFlags |= 1;
                return;
            }
        }
        if (a0 == RR_PAL_NTSC(0x7E, 0x97)) {
            if (!(RaceVoiceFlags & 2)) {
                SoundQueueEnqueue(1, 0);
                RaceVoiceFlags |= 2;
                return;
            }
        }
        if (a0 == RR_PAL_NTSC(0x97, 0xB5)) {
            SoundQueueEnqueue(2, 0);
            return;
        }
        if (a0 == RR_PAL_NTSC(0xB0, 0xD3)) {
            if (!(RaceVoiceFlags & 4)) {
                SoundQueueEnqueue(3, 0);
                RaceVoiceFlags |= 4;
                return;
            }
        }
        if (a0 == RR_PAL_NTSC(0xE2, 0x10F)) {
            if (!(RaceVoiceFlags & 8)) {
                SoundQueueEnqueue(0x17, 0);
                RaceVoiceFlags |= 8;
                return;
            }
        }
        if (a0 == RR_PAL_NTSC(0x114, 0x14B)) {
            if (!(RaceVoiceFlags & 0x10)) {
                SoundQueueEnqueue(0x18, 0);
                RaceVoiceFlags |= 0x10;
            }
        }
    }
}

void InitPlayerRumble(void) {
    int i;
    PadClearPulse(0);
    PadClearPulse(1);
    for (i = 0; i < 2; i++) {
        if ((s16)PlayerCars[i].flags1E4 != 0) {
            PlayerControllers[i].rumbleActive50 = 1;
            PadSetRumbleParams(i, 0, 0xF);
        }
    }
}

void UpdatePauseMenu(int a0) {
    if (Pad0Buttons & 0x1000) {
        if (PauseCursor > 0) {
            PauseCursor = PauseCursor - 1;
            PlaySfx(0);
        }
    }
    if (Pad0Buttons & 0x4000) {
        if (PauseCursor < PauseMenuMaxChoices[a0]) {
            PauseCursor = PauseCursor + 1;
            PlaySfx(0);
        }
    }
    InitPlayerRumble();
}

void ApplyPauseMenuChoice(int arg0) {
    int *t;
    if (PauseCursor == 0) {
        FadeCountdown = 0x1E;
        return;
    }
    SelectionState.bytes[0x19] = 0;
    TotalRaceTime = -1;
    if (arg0 == 1) {
        SelectionState.bytes[0x19] = 9;
        RaceState = 6;
        RankEntries[0].score00 = -TrackLength;
        if (PauseCursor == 2) {
            SubmodeState = 0;
            ModeGroup = PauseAbortNextModes[3] >> 8;
            ModeIndex = *(unsigned char *)&PauseAbortNextModes[3];
            return;
        }
        FinishRace(SelectionState.bytes, PauseCursor);
        return;
    }
    RaceState = 6;
    t = &PauseAbortNextModes[(PauseCursor - 1) + arg0 * 2];
    ModeGroup = *t >> 8;
    {
        int v = *(unsigned char *)t;
        SubmodeState = 0;
        ModeIndex = v;
    }
}

void DrawPauseMenu(GpuDmaTag *ordering, int selection) {
    Vec2s corners[4];
    unsigned char rgb[4];
    void *cursor;
    int tile, vertex;
    unsigned char (*triangles)[4];
    unsigned char *indices;

    cursor = GameQueueSprite(ordering, (SpritePacket *)RR_GPU_SCRATCH->cursor, 0x90, 0x5C, 0x1C, 8, 0x60, 0xA0, 0x7FF8);
    cursor = GameQueueSprite(ordering, cursor, 0x90, 0x6A, 0x20, 8, 0xC4, 0xA0, 0x7FF8);
    if (GameModeType == 1) {
        cursor = GameQueueSprite(ordering, cursor, 0x90, 0x74, 0x1C, 8, 0x7C, 0xA0, 0x7FF8);
    } else {
        cursor = GameQueueSprite(ordering, cursor, 0x90, 0x74, 0x24, 8, 0x9C, 0xA0, 0x7FF8);
        cursor = GameQueueSprite(ordering, cursor, 0x90, 0x7E, 0x1C, 8, 0x7C, 0xA0, 0x7FF8);
    }
    cursor = GameQueueSprite(ordering, cursor, 0x88, selection * 10 + 0x6A, 8, 8, 0xE4, 0xA0, 0x7FF8);

    for (tile = 0; tile < 9; tile++) {
        for (vertex = 0; vertex < 4; vertex++) {
            int columnX = ((vertex & 1) << 5) + 0x70;
            corners[vertex].x = ((tile % 3) << 5) + columnX;
            {
                int rowY = ((vertex >> 1) << 4) + 0x56;
                corners[vertex].y = ((tile / 3) << 4) + rowY;
            }
        }
        rgb[0] = 0xFF;
        rgb[2] = 0x20;
        rgb[1] = 0x20;
        vertex = tile / 3;
        vertex = vertex == 1;
        triangles = PauseMenuChoiceMap;
        indices = triangles[vertex];
        cursor = EmitPolyF3(ordering, cursor, corners[indices[0]], corners[indices[1]], corners[indices[2]], rgb, 1);
        rgb[0] = 0xFF;
        rgb[2] = 0xFF;
        rgb[1] = 0xFF;
        cursor = EmitPolyF3(ordering, cursor, corners[indices[1]], corners[indices[2]], corners[indices[3]], rgb, 1);
    }
    RR_GPU_SCRATCH->cursor = AddDrawModePrim(ordering, cursor, 5);

    for (vertex = 0; vertex < 4; vertex++) {
        if (vertex == 1 || vertex == 2) {
            corners[vertex].x = 0xD0;
        } else {
            corners[vertex].x = 0x70;
        }
        if (vertex >= 2) {
            corners[vertex].y = 0x86;
        } else {
            corners[vertex].y = 0x56;
        }
    }

    for (tile = 0; tile < 3; tile++) {
        vertex = 0x40;
        AddPartialLine(ordering, corners[tile].x, corners[tile].y, corners[tile + 1].x, corners[tile + 1].y, 0x100, 0, vertex, vertex, vertex, -1);
    }
}

void ResetLinkSioControl(void) {
    _sio_control(1, 1, 1);
}

int IsLinkPeerPresent(void) {
    return (((unsigned)_sio_control(0, 0, 0) >> 7) & 3) - 1 < 2;
}

int LinkTimeoutCallback(int a0, unsigned int a1) {
    return a1 <= 0x1869E;
}

void OpenLinkSerial(void) {
    _sio_control(1, 3, 0x7E900);
    _sio_control(1, 4, 1);
    LinkWriteHandle = FileOpen(SioDeviceName, 2);
    LinkReadHandle = FileOpen(SioDeviceName, 1);
    _sio_control(4, 0, (uintptr_t)LinkTimeoutCallback);
    _sio_control(2, 0, 0);
}

void CloseLinkSerial(void) {
    _sio_control(2, 3, 0);
    _sio_control(2, 0, 0);
    FileClose(LinkWriteHandle);
    FileClose(LinkReadHandle);
    _sio_control(4, 0, 0);
}

void BeginLinkSync(void) {
    if (LinkState == 2) {
        LinkState = 3;
        _sio_control(1, 1, 2);
    }
}

int ExitOnLinkLost(void) {
    if (LinkState == -1) {
        ModeGroup = 1;
        ModeIndex = 0x2D;
        SubmodeState = 0;
        _sio_control(1, 1, 0);
        return 1;
    }
    return 0;
}

int UpdateLinkHandshake(int a0)
{
  int r = 0;
  int st;
  int mm;
  signed char *t;
  int q3;
  ControllerState5C (*e)[2];
  unsigned int k = ((unsigned int)_sio_control(0, 0, 0) >> 7) & 3;
  st = (short) (LinkState + 1);
  switch (st)
  {
    case 1:
      if (_sio_control(3, 1, 0) != 0)
    {
      _sio_control(1, 1, 2);
      if (a0 == 0)
      {
        LinkHostFlag = 0;
        LinkEntryState[1][0] = 1;
        RaceStartSignalState = 3;
        LinkState = 10;
      }
      else
      {
        goto s2;
      }
      break;
    }
      _sio_control(1, 1, 2);
      if (a0 == 0)
    {
      LinkHostFlag = 1;
      LinkEntryState[0][0] = 1;
      LinkState = 1;
    }
    else
      if (k != 1)
    {
      LinkState = 1;
    }
    else
    {
      goto s2;
    }
      break;

    case 11:
      if (RaceStartSignalState > 0)
    {
      _sio_control(1, 1, 2);
      RaceStartSignalState -= 1;
      if (k == 1)
      {
        LinkState = 2;
        RaceStartSignalState = 0;
      }
      break;
    }
      s2:
    LinkState = 2;

      break;

    case 2:
      if ((k == 0) || ((k == 3) && (a0 == 0)))
    {
      goto m1;
    }
    else
      if (k == 2)
    {
      LinkState = k;
      RaceStartSignalState = 4;
    }
      break;

    case 3:
      _sio_control(1, 1, 1);
      if (RaceStartSignalState > 0)
    {
      if (a0 == 0)
      {
        RaceStartSignalState -= 1;
        break;
      }
    }
    else
      if (a0 == 0)
    {
      t = &LinkEntryState[1 - LinkHostFlag][1];
      if ((*t) == 0)
      {
        e = &PlayerControllers;
        if ((*e)[1].action28 != 0)
        {
          PlaySfx(1);
          LinkEntryState[1 - LinkHostFlag][1] = 1;
        }
        else
          if ((*e)[1].action2C != 0)
        {
          *t = -1;
        }
        if (CourseSelectLockedFlag != 0)
        {
          LinkEntryState[0][1 - (q3 = LinkHostFlag)] = -1;
          _sio_control(1, 1, 0);
        }
      }
    }
      if ((k == 0) || ((k == 3) && (a0 == 0)))
    {
      goto m1;
    }
    else
      if (a0 == 0)
    {
      int v = RaceTimeRemaining - 1;
      RaceTimeRemaining = v;
      if (v <= 0)
      {
        LinkState = 4;
        _sio_control(1, 1, 2);
      }
      else
        if (LinkEntryState[1 - LinkHostFlag][1] != 0)
      {
        LinkState = 3;
        _sio_control(1, 1, 2);
      }
    }
      break;

    case 4:
      if (a0 == 0)
    {
      if (RaceTimeRemaining > 0)
      {
        RaceTimeRemaining -= 1;
      }
    }
      if ((k == 0) || ((k == 3) && (a0 == 0)))
    {
      goto m1;
    }
      if (k == 1)
    {
      break;
    }

    case 5:
    if (a0 != 2)
    {
      LinkState = 0;
    }

      r = 1;
      break;

    case 0:
      r = -1;
      break;
      m1:
    *((short *) (&LinkState)) = -1;

      break;

  }

  return r;
}

int PollLinkStatus(void) {
    int r = 0;
    int t = ((unsigned int)_sio_control(0, 0, 0) >> 7) & 3;
    if (t == 0 || t == 3) {
        LinkState = -1;
        r = -1;
    } else if (t == 1) {
        LinkState = t;
    } else if (t == 2) {
        if (LinkState == 3) r = 1;
    }
    return r;
}

/* Initial values of the globals this module owns. */
char SioDeviceName[56] = {115, 105, 111, 58};
Vec3s6 StartSignalLightOffsetsA[2] = {{.x = 512}, {.x = 120}};
Vec3s6 StartSignalLightOffsetsB[2] = {{.y = -256}, {.x = -16, .y = -248, .z = -96}};
UiUvRect PauseMenuLabelUvs[8] = {{.u = 0x80, .w = 0x80, .h = 0x4C}, {.w = 0x70, .h = 0x10}, {.u = 0x44, .v = 0x20, .w = 0xE, .h = 0x10}, {.u = 0x50, .v = 0x20, .w = 8, .h = 0x10}, {.u = 0x5C, .v = 0x20, .w = 8, .h = 0x10}, {.v = 0x10, .w = 0xC, .h = 0x10}, {.u = 0x20, .v = 0x20, .w = 0xC, .h = 0x10}, {.v = 0x20, .w = 0x20, .h = 0x10}};
u16 PauseTitleClutIds[2] = {0x797E, 0x797D};
u16 PauseRowClutIds[4] = {0x79BD, 0x79BD, 0x79BC, 0x79BE};
u8 PauseRowOffsetX[4] = {0x58, 0x1C, 0x3A, 0x30};
unsigned char PauseMenuMaxChoices[8] = {2, 1, 2, 2, 2, 2, 2};
int PauseAbortNextModes[14] = {308, 279, 279, 259, 293, 279, 293, 279, 281, 279, 337, 279, 337, 279};
unsigned char PauseMenuChoiceMap[3][4] = {{2, 0, 3, 1}, {0, 1, 2, 3}, {0xFF, 0xFF, 0xFF, 0xFF}};
int RaceStartSignalState;
int RaceVoiceFlags;
int RaceTimeRemaining;
short LinkHostFlag;
signed char PauseCursor;
s16 LinkState;
