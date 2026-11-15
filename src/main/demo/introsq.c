#include "game/sound/cdpos.h"
#include "game/data/menuvars.h"
#include "scratchpad.h"
#include "rr/textured_panel.h"
#include "game/data/trkld8.h"
#include "rr/driver_control.h"
#include "game/sound/cdplay.h"
#include "game/gfx/fadeend.h"
#include "game/demo/intro.h"
#include "rr/scene_camera_state.h"

void IntroModeUpdate(void);

void IntroModeUpdate(void) {
    int s1 = IntroScriptStep();
    switch (RaceContext.work444.intro.step444) {
    case 0:
        SetDispMask(1);
        RaceContext.work444.intro.step444++;
    case 1:
        if (LoadIntroSceneStep() != 0) {
            RaceContext.work444.intro.step444++;
            StartDemoFadeOutAndCloseVab();
            CdAudioInit();
            CdAudioSelectTrack(0, 0x25);
        }
        break;
    case 2:
        if (s1 == RaceContext.work444.intro.step444) {
            SetDispMask(0);
            ModeGroup = s1;
            ModeIndex = 7;
            RaceContext.work444.intro.step444 = 0;
            SubmodeState = 0;
        }
        break;
    }
}

void EmitTexturedPanel(s32 index) {
    PolyFT4Packet *p;
    GpuScratchPointers *scratch;

    scratch=RR_GPU_SCRATCH;
    p=(PolyFT4Packet*)scratch->cursor;
    p->tag.length=9;
    p->code=0x2D;
    p->x0=32;
    p->y0=112;
    p->x1=288;
    p->y1=112;
    p->x2=32;
    p->y2=368;
    p->x3=288;
    p->y3=368;
    p->u0=0;
    p->v0=(index%2)*128U;
    p->u1=255;
    p->v1=(index%2)*128U;
    p->u2=0;
    p->v2=(index%2)*128U+127;
    p->u3=255;
    p->v3=(index%2)*128U+127;
    p->tpage=(((s32)((index/2)*64U+320U)&1023)>>6)|16;
    p->clut=0x1BD6;
    RR_GPU_LINK(&scratch->order[1], p);
    scratch->cursor=(u8*)(p+1);
}
