#include "rr/release.h"
#include "rr/video_timing.h"
#include "rr/scene_camera_state.h"
#include "game/gfx/fadeset.h"
#include "game/sound/sndvab.h"
#include "game/gfx/fadectl.h"
#include "game/gfx/fadespr.h"


void ResetDemoFadeState(void) {
    int v, w, x;
    RaceContext.fade3E0.f434 = 0;
    RaceContext.fade3E0.f440 = 0;
    v = RaceContext.fade3E0.f408;
    if (v == 1 || (v == 0 && RaceContext.fade3E0.f3EC == 0)) {
        if (RaceContext.fade3E0.f3F4 == 0) {
            SetParticleBrightFade(1, 0, 0x100);
            RaceContext.fade3E0.f3F4 = 1;
        }
    }
    w = RaceContext.fade3E0.f408;
    if (w == 2 || (w == 0 && RaceContext.fade3E0.f3EC == 0xFF)) {
        if (RaceContext.fade3E0.f3F4) {
            SetParticleBrightFade(0, 0, 0);
            RaceContext.fade3E0.f3F4 = 0;
        }
    }
    RaceContext.fade3E0.f400 = RR_PAL_JP(0x8C, 0x40);
    RaceContext.fade3E0.f404 = 0x100;
    RaceContext.fade3E0.f3F8 = 0;
    RaceContext.fade3E0.f410 = 1;
    RaceContext.fade3E0.f412 = 0x10;
    RaceContext.fade3E0.f41A = 0x10;
    RaceContext.fade3E0.f416 = 0x10;
    RaceContext.fade3E0.f418 = 1;
    RaceContext.fade3E0.f414 = 1;
    RaceContext.fade3E0.f3E4 = RR_PAL_NTSC(0xBB8, 0xE10); /* 2 minutes */
    x = RaceContext.fade3E0.f430;
    RaceContext.fade3E0.f424 = 0;
    RaceContext.fade3E0.f41C = 1;
    if (x == 0) {
        PlaySfxBank2(RaceContext.fade3E0.f42C);
        RaceContext.fade3E0.f430 = 1;
    }
}
