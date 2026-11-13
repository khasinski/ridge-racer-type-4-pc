#include "rr/cpu_time.h"
#include "rr/runtime_settings.h"
#include "rr/scene_camera_state.h"
#include "game/ui/badgeprim.h"
#include "rr/gpu_scratch_types.h"
#include "rr/gpu.h"
#include "rr/frame_storage_types.h"
#include "rr/fade_sprite.h"
#include "game/effect/ptinit.h"
#include "game/sound/cdmain.h"
#include "psyq/libsnd/ssmvol.h"
#include "game/camera/campath.h"
#include "game/gfx/fadespr.h"
#include "rr/audio_state_types.h"
#include "rr/bios_devices.h"
#include "rr/library.h"
#include "psyq/libsnd/vmcfg.h"

struct OptionsDefaultTail16 { int a[4]; };

extern struct OptionsDefaultTail16 OptionsDefaultTailData;
extern unsigned char HudPauseMenuTitleY[];

void EmitFadeSprite(s32 index, s32 x, s32 y, s32 intensity);

void InitOptionsDefaults(void) {
    int i;

    i = 0;
    do {
        Options.pulse[i].buttonLayout00 = 0;
        Options.pulse[i].disabled02 = 0;
        Options.pad[i].deadzone08 = 1;
        Options.pad[i].range0A = 1;
        Options.pad[i].buttonLayout0C = 0;
        Options.wheel[i].deadzone00 = 1;
        Options.wheel[i].range01 = 0;
        Options.wheel[i].rumble02 = 2;
        Options.wheel[i].buttonLayout03 = 0;
        i++;
    } while (i < 2);
    Options.bgmVolume30 = 0x10;
    Options.volume31 = 0x10;
    Options.mono32 = 0;
    Options.screenX34 = 0;
    Options.screenY38 = 0x10;
}

void InitDemoFadeContext(int a0) {
    struct OptionsDefaultTail16 v;
    int i;
    s16 t;
    DemoFadeState *p = &RaceContext.fade3E0;
    v = OptionsDefaultTailData;
    PsyqBzero((char *)p, sizeof(*p));
    p->f400 = 0xCE;
    p->f42C = a0;
    p->scale3E8 = 0x100;
    RaceContext.streamResourceB0 = 0;
    InitParticles();
    i = 0;
    if (SoundState.seqHandle2864 >= 0) {
        SsSeqStop(*(short *)&SoundState.seqHandle2864);
        SsSeqClose(*(short *)&SoundState.seqHandle2864);
        SoundState.seqHandle2864 = -1;
    }
    for (i = 0; i < 4; i++) {
        t = SoundState.banks24.ids[v.a[i]];
        if (t >= 0) {
            SsVabClose(t);
            SoundState.banks24.ids[v.a[i]] = -1;
        }
    }
    ResetSeqFadeState();
    SoundState.seqTrack2868 = -1;
}

void UpdateFadeTint(int a0) {
    RR_CPU_CYCLES(RR_CYCLES_UPDATE_FADE_TINT);

    int v1, a, v2;
    RaceContext.fade3E0.r3F0 = (HudPauseMenuTitleY[0] * a0) / 255;
    RaceContext.fade3E0.g3F1 = (HudPauseMenuTitleY[1] * a0) / 255;
    v1 = RaceContext.fade3E0.r3F0 * RaceContext.fade3E0.scale3E8;
    RaceContext.fade3E0.b3F2 = (HudPauseMenuTitleY[2] * a0) / 255;
    if (v1 < 0) v1 += 0xFF;
    a = RaceContext.fade3E0.g3F1 * RaceContext.fade3E0.scale3E8;
    RaceContext.fade3E0.r3F0 = v1 >> 8;
    if (a < 0) a += 0xFF;
    v2 = RaceContext.fade3E0.b3F2 * RaceContext.fade3E0.scale3E8;
    RaceContext.fade3E0.g3F1 = a >> 8;
    if (v2 < 0) v2 += 0xFF;
    RaceContext.fade3E0.b3F2 = v2 >> 8;
}

void EmitFadeSprite(s32 index, s32 x, s32 y, s32 intensity) {
    RR_CPU_CYCLES(RR_CYCLES_EMIT_FADE_SPRITE);

    s32 phase;
    s32 selector;
    s32 scale;
    GpuCommandPacket *p;

    selector = (index == 4);
    phase = RaceContext.fade3E0.f3EC;
    if (phase >= 128) {
        selector += 2;
        scale = phase - 127;
    } else {
        scale = 128U - (u32)phase;
    }
    scale = (s32)((u32)scale * intensity) / 256;
    scale = (s32)((u32)scale * RaceContext.fade3E0.scale3E8) / 256;
    if (scale > 0) {
        GpuDmaTag *ordering = &CurrentDrawBuf->buffer.ordering70.tags[10];
        u32 mode;
        p = (GpuCommandPacket *)RR_GPU_SCRATCH->cursor;
        p = (GpuCommandPacket *)AddSpritePrim(
            ordering,
            (SpritePacket *)p,
            x, y, &HudCourseMapScaleTable[index], 0x7F38,
            (scale * 2U) | 1U);
        mode = FadeSpriteDrawModes[selector];
        RR_GPU_SCRATCH->cursor = (u8 *)p;
        p->command = (mode & 0x9FF) | 0xE1000200U;
        RR_GPU_ADD_PRIM(ordering, p, 1);
        RR_GPU_SCRATCH->cursor = (u8 *)(p + 1);
    }
}

/* Initial values of the globals this module owns. */
u32 FadeSpriteDrawModes[4] = {0x2Bu, 0x2Cu, 0x4Bu, 0x4Cu};
struct OptionsDefaultTail16 OptionsDefaultTailData = {.a = {2, 3, 1, 4}};
UiUvRect HudCourseMapScaleTable[5] = {{.w = 0xE8, .h = 0x64}, {.v = 0xC8, .w = 8, .h = 0x18}, {.u = 8, .v = 0xC8, .w = 0x70, .h = 0x18}, {.v = 0xE0, .w = 0xBC, .h = 0x18}, {.w = 0xE8, .h = 0x48}};
unsigned char HudPauseMenuTitleY[4] = {0xFF, 0xBD};
