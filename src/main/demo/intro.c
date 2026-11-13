#include "game/ui/badge1.h"
#include "rr/release.h"
/* RODATA 0x80011270 0x2c */
#include "rr/video_timing.h"
#include "rr/cpu_time.h"
#include "types.h"
#include "rr/frame_storage_types.h"
#include "rr/frame_loop.h"
#include "rr/load_cursor.h"
#include "scratchpad.h"
#include "rr/line_emit.h"
#include "game/effect/effect.h"
#include "game/effect/fxspr.h"
#include "game/gfx/fadesel.h"
#include "game/gfx/fadeset.h"
#include "game/gfx/fadespr.h"
#include "game/sound/sndvab.h"
#include "rr/driver_control.h"
#include "rr/bios_devices.h"
#include "rr/course_choices.h"
#include "rr/fade_sprite.h"
#include "rr/trig.h"
#include "rr/badge_rows.h"
#include "game/data/trkld1.h"
#include "rr/scene_camera_state.h"

struct Entry {
    int frame;
    int cmd;
};

extern struct Entry IntroTimeline[];
extern const struct Entry IntroTimelineNtsc[];
#define INTRO_TIMELINE RR_PAL_NTSC(IntroTimeline, IntroTimelineNtsc)
#include "rr/cd_directory_buffer.h"
extern s16 SubmodeState;

int IntroScriptStep(void);
void IntroInit(void);

int IntroScriptStep(void) {
    RR_CPU_CYCLES(RR_CYCLES_INTRO_SCRIPT_STEP);

    short sp28[4];
    unsigned char sp30[4];
    int i;
    int v;
    int cmd;
    int ret = 0;

    cmd = -1;
    for (i = 0; INTRO_TIMELINE[i].frame >= 0; i++) {
        if (INTRO_TIMELINE[i].frame == RaceContext.fade3E0.f3E4) {
            cmd = INTRO_TIMELINE[i].cmd;
            break;
        }
    }

    switch (cmd) {
    case 0:
        RaceContext.fade3E0.f3F4 = 1;
        break;
    case 1:
        SetParticleBrightFade(0x20, 0x100, 0);
        break;
    case 2:
        RaceContext.fade3E0.f3F8 = 1;
        break;
    case 3:
        RaceContext.fade3E0.f3F8 = 2;
        RaceContext.fade3E0.f3FC = 0;
        break;
    case 4:
        RaceContext.fade3E0.f408 = 1;
        break;
    case 5:
        RaceContext.fade3E0.f408 = 2;
        break;
    case 6:
        RaceContext.fade3E0.f410 = 1;
        break;
    case 7:
        RaceContext.fade3E0.f418 = 1;
        break;
    case 8:
        RaceContext.fade3E0.f414 = 1;
        RaceContext.fade3E0.f41C = 1;
        RaceContext.fade3E0.f424 = 0;
        break;
    case 9:
        RaceContext.fade3E0.f434 = 1;
        RaceContext.fade3E0.f43C = 0;
        RaceContext.fade3E0.f438 = 0;
        break;
    case 10:
        RaceContext.fade3E0.f434 = 2;
        RaceContext.fade3E0.f43C = 0;
        break;
    }

    if (RaceContext.fade3E0.f428 == 1) {
        RaceContext.fade3E0.scale3E8 -= 0x10;
        if (RaceContext.fade3E0.scale3E8 <= 0x80) {
            RaceContext.fade3E0.scale3E8 = 0x80;
            RaceContext.fade3E0.f428 = 2;
        }
    }

    switch (RaceContext.fade3E0.f434) {
    case 1:
        RaceContext.fade3E0.f43C++;
        RaceContext.fade3E0.f440 = (RaceContext.fade3E0.f43C * 0xFF) / 16;
        if (RaceContext.fade3E0.f43C >= 0x10) {
            RaceContext.fade3E0.f434 = 0;
            RaceContext.fade3E0.f440 = 0xFF;
        }
        break;
    case 2:
        RaceContext.fade3E0.f43C++;
        RaceContext.fade3E0.f440 = ((0x10 - RaceContext.fade3E0.f43C) * 0xFF) / 16;
        if (RaceContext.fade3E0.f43C >= 0x10) {
            RaceContext.fade3E0.f434 = 0;
            RaceContext.fade3E0.f440 = 0;
        }
        break;
    }

    if (RaceContext.fade3E0.f440 > 0) {
        DrawIntroRippleSprites();
    }

    if (RaceContext.fade3E0.f3F4) {
        UpdateParticles();
    }

    switch (RaceContext.fade3E0.f3F8) {
    case 1:
        RaceContext.fade3E0.f404 += 8;
        if (RaceContext.fade3E0.f404 >= 0x100) {
            RaceContext.fade3E0.f404 = 0x100;
            RaceContext.fade3E0.f3F8 = 0;
        }
        break;
    case 2:
        RaceContext.fade3E0.f3FC++;
        if (RaceContext.fade3E0.f3FC >= 0x10) {
            RaceContext.fade3E0.f3F8 = 0;
            RaceContext.fade3E0.f3FC = 0x10;
            if (RaceContext.fade3E0.f430 == 0) {
                PlaySfxBank2(RR_RELEASE_JP ? RaceContext.fade3E0.f42C : 1);
                RaceContext.fade3E0.f430 = 1;
            }
        }
        if (RR_RELEASE_JP)
            RaceContext.fade3E0.f400 = 0x68 - (RaceContext.fade3E0.f3FC * 0x28) / 16;
        else
            RaceContext.fade3E0.f400 = 0xCE - (RaceContext.fade3E0.f3FC * 0x42) / 16;
        break;
    case 3:
        RaceContext.fade3E0.f404 -= 8;
        if (RaceContext.fade3E0.f404 <= 0) {
            RaceContext.fade3E0.f404 = 0;
            RaceContext.fade3E0.f3F8 = 0;
        }
        break;
    }

    EmitFadeSprite(0, 0x2C, RaceContext.fade3E0.f400, RaceContext.fade3E0.f404);
    /* The Japanese title logo has a second part 200 pixels to the right. */
    if (RR_RELEASE_JP)
        EmitFadeSprite(4, 0x2C, RaceContext.fade3E0.f400 + 200, RaceContext.fade3E0.f404);

    if (RaceContext.fade3E0.f410) {
        int c;
        RaceContext.fade3E0.f412++;
        if (RaceContext.fade3E0.f412 > 0x10) {
            RaceContext.fade3E0.f412 = 0x10;
        }
        c = 0xFF - RaceContext.fade3E0.f3EC;
        c *= RaceContext.fade3E0.f404;
        if (c < 0) {
            c += 0xFF;
        }
        c >>= 8;
        c *= RaceContext.fade3E0.scale3E8;
        if (c < 0) {
            c += 0xFF;
        }
        c >>= 8;
        sp30[0] = sp30[1] = sp30[2] = c;
        sp28[0] = 0x10;
        sp28[2] = RaceContext.fade3E0.f412 * 18 + 0x10;
        for (i = 0; i < 2; i++) {
            sp28[1] = sp28[3] = i * 40 + RR_PAL_JP(0x172, 0x190);
            AddLineF2Prim(sp28, sp30, 0, 10);
            sp28[1] = sp28[3] = i * 40 + RR_PAL_JP(0x173, 0x191);
            AddLineF2Prim(sp28, sp30, 0, 10);
        }
    }

    if (RaceContext.fade3E0.f418) {
        int v2;
        RaceContext.fade3E0.f41A++;
        if (RaceContext.fade3E0.f41A > 0x10) {
            RaceContext.fade3E0.f41A = 0x10;
            v2 = 0x100;
        } else {
            v2 = RaceContext.fade3E0.f41A * 16;
        }
        v2 = (v2 * RaceContext.fade3E0.f404) / 256;
        if (RR_RELEASE_JP)
            EmitFadeSprite(3, 0x4C, 0x19A, v2);
        else
            EmitFadeSprite(3, 0x42, 0x17C, v2);
    }

    if (RaceContext.fade3E0.f414) {
        RaceContext.fade3E0.f416++;
        if (RaceContext.fade3E0.f416 > 0x10) {
            RaceContext.fade3E0.f416 = 0x10;
            v = 0x100;
        } else {
            v = RaceContext.fade3E0.f416 * 16;
        }
        v = (v * RaceContext.fade3E0.f404) / 256;
        EmitFadeSprite(2, 0x68, RR_PAL_JP(0x13C, 0x162), v);
        if (RaceContext.fade3E0.f428 == 0) {
            v = (rsin((FrameCounter << 7) & 0xF80) * v) / 4096;
        }
        EmitFadeSprite(1, 0x68, RR_PAL_JP(0x13C, 0x162), v);
    }

    switch (RaceContext.fade3E0.f408) {
    case 1:
        RaceContext.fade3E0.f3EC += 8;
        if (RaceContext.fade3E0.f3EC >= 0xFF) {
            RaceContext.fade3E0.f3EC = 0xFF;
            RaceContext.fade3E0.f408 = 0;
            RaceContext.fade3E0.f3F4 = 0;
        }
        break;
    case 2:
        RaceContext.fade3E0.f3EC -= 8;
        if (RaceContext.fade3E0.f3EC <= 0) {
            RaceContext.fade3E0.f3EC = 0;
            RaceContext.fade3E0.f408 = 0;
        }
        break;
    }

    UpdateFadeTint(RaceContext.fade3E0.f3EC);
    if (RR_RELEASE_JP) {
        /* One tile over the drawing area's height in the tint, then draw
         * mode 5; the clear colour of the draw environments stays. */
        GpuTilePacket *tile = (GpuTilePacket *)RR_GPU_SCRATCH->cursor;
        tile->tag.length = 3;
        tile->code = 0x60;
        tile->x = 0;
        tile->y = 0;
        tile->w = 0x140;
        tile->h = CurrentDrawBuf->buffer.environment00.draw00.clip.h;
        tile->r = RaceContext.fade3E0.r3F0;
        tile->g = RaceContext.fade3E0.g3F1;
        tile->b = RaceContext.fade3E0.b3F2;
        RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[703], tile);
        RR_GPU_SCRATCH->cursor = (u8 *)(tile + 1);
        RR_GPU_SCRATCH->cursor = AddDrawModePrim(&CurrentDrawBuf->buffer.ordering70.tags[703],
                                                 (GpuDrawModePacket *)RR_GPU_SCRATCH->cursor, 5);
    } else {
    RR_GPU_SCRATCH->cursor = AddTileGrid2x2(&CurrentDrawBuf->buffer.ordering70.tags[703], RR_GPU_SCRATCH->cursor, 0, 0, 0x140, 0x200,
                                       RaceContext.fade3E0.r3F0, RaceContext.fade3E0.g3F1, RaceContext.fade3E0.b3F2);
    for (i = 0; i < 2; i++) {
        DrawBuffers[i].buffer.environment00.draw00.r = RaceContext.fade3E0.r3F0;
        DrawBuffers[i].buffer.environment00.draw00.g = RaceContext.fade3E0.g3F1;
        DrawBuffers[i].buffer.environment00.draw00.b = RaceContext.fade3E0.b3F2;
    }
    }

    RaceContext.fade3E0.f3E4++;
    if (RaceContext.fade3E0.f3E4 > RR_PAL_NTSC(3000, 3600)) {
        RaceContext.fade3E0.f3E4 = RR_PAL_NTSC(3000, 3600);
    }

    if (RaceContext.fade3E0.f41C == 1) {
        RaceContext.fade3E0.f424++;
        if (RaceContext.fade3E0.f424 >= RR_PAL_NTSC(1000, 1200)) {
            if (RaceContext.fade3E0.f3E0 == 0) {
                RaceContext.fade3E0.f408 = 2;
                RaceContext.fade3E0.f3E0 = 1;
                SetParticleBrightFade(0x20, 0, 0x100);
                RaceContext.fade3E0.f3F4 = 1;
            } else {
                RaceContext.fade3E0.f408 = 1;
                RaceContext.fade3E0.f3E0 = 0;
                SetParticleBrightFade(0x20, 0x100, 0);
            }
            RaceContext.fade3E0.f424 = 0;
        }
        ret = 1;
    } else if (RaceContext.fade3E0.f41C == 3) {
        RaceContext.fade3E0.f420++;
        if (RaceContext.fade3E0.f420 > 20) {
            RaceContext.fade3E0.f420 = 20;
            ret = 2;
        } else {
            ret = 1;
        }
    }

    return ret;
}

void IntroInit(void) {
    SetDispMask(0);
    PsyqBzero(&RaceContext.work444.intro, sizeof(RaceContext.work444.intro));
    InitDemoFadeContext(0);
    SelectDemoFadePreset(0);
    LoadCursor = CdDirectoryBuffer;
    RaceLoadReset();
    ModeGroup = 2;
    ModeIndex = 6;
    SubmodeState = 0;
}

/* Initial values of the globals this module owns. */
struct Entry IntroTimeline[11] = {{.cmd = 9}, {.frame = 50}, {.frame = 149, .cmd = 10}, {.frame = 150, .cmd = 2}, {.frame = 250, .cmd = 1}, {.frame = 251, .cmd = 4}, {.frame = 300, .cmd = 3}, {.frame = 330, .cmd = 6}, {.frame = 350, .cmd = 7}, {.frame = 380, .cmd = 8}, {.frame = -1}};
/* The NTSC release's frames (rr/video_timing.h). */
const struct Entry IntroTimelineNtsc[11] = {{.cmd = 9}, {.frame = 60}, {.frame = 179, .cmd = 10}, {.frame = 180, .cmd = 2}, {.frame = 300, .cmd = 1}, {.frame = 301, .cmd = 4}, {.frame = 360, .cmd = 3}, {.frame = 390, .cmd = 6}, {.frame = 420, .cmd = 7}, {.frame = 450, .cmd = 8}, {.frame = -1}};
