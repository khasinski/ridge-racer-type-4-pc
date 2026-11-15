#include "game/system/pad.h"
#include "game/data/menuvars.h"
#include "rr/audio_state_types.h"
#include "game/gfx/scene1.h"
#include "rr/menu_selection.h"
#include "types.h"
#include "game/gfx/fadesel.h"
#include "game/math/shuffle.h"
#include "game/sound/sndctrl.h"
#include "game/gfx/fadein.h"
#include "game/gfx/fadespr.h"
#include "game/ui/hudmap2.h"
#include "game/demo/attract.h"
#include "game/menu/title.h"
#include "rr/audio_loop.h"
#include "rr/bios_devices.h"
#include "rr/color_conversion.h"
#include "rr/race_setup.h"
#include "rr/driver_control.h"
#include "game/data/selection.h"
#include "psyq/libc/rand.h"
#include "game/menu/mnrows.h"
#include "rr/scene_camera_state.h"

extern int MenuTransitionFadeRates[];
extern unsigned short LoadingBarAnimCounter;
extern unsigned short MenuStageEntryModeIds[];

extern int MaskBitSetting;

void StartAttractDemo(int a0);
void AttractDemoRaceEnter(void);

void StartAttractDemo(int a0) {
    int sp[4];
    int i;
    int y;
    int u;
    int *tb;
    int *q;
    int s0;
    int s2;
    int v;
int *w;
    struct { unsigned char a, b, c, d; } *b;
    unsigned short m;
    unsigned short *e;
    int n;
    unsigned short h;
    AttractDemoState80 *d0;
    int x6;
    int *pa, *pb;
    int vq;
    int x5;

    if (SlotOrder.pending00 != 0) {
        ShuffleSlotOrder();
        SlotOrder.pending00 = 0;
    }
    PsyqBzero(&RaceContext.attract4BC, sizeof(RaceContext.attract4BC));
    RaceContext.attract4BC.mode0C = a0;
    i = 3;
    q = sp + 3;
    do {
        *q = i;
        i--;
        q--;
    } while (i >= 0);
    i = 0;
    do {
        s0 = PsyqRand() % 4;
v = s0 + PsyqRand() % 2;
        vq = v % 4;
        s2 = sp[s0];
        y = sp[vq];
        sp[s0] = y;
        sp[vq] = s2;
        i++;
    } while (i < 0x10);
    i = 0;
    x6 = PsyqRand();
    s2 = x6 % 18;
    tb = MenuTransitionFadeRates;
    w = AttractCarTeams;
    b = (void *)SelectionRows.bytes;
    do {
        b[i].a = sp[i];
        b[i].b = tb[sp[i]];
        *w = tb[sp[i]];
        u = s2 + PsyqRand() % 4;
        v = u - 2;
        if (v < 0) v = 0;
        if (v > 0x12) v = 0x12;
        b[i].c = v;
        w++;
        i++;
    } while (i < 4);
    CourseIndex = -1;
    if (a0 == 1) {
        m = AttractPromptBlinkMask & LoadingBarAnimCounter;
        if (m != 0) {
            i = 0;
            e = MenuStageEntryModeIds;
            do {
                if (m == e[0])
                    CourseIndex = e[1];
                i++;
                e += 2;
            } while (i < 8);
        }
    }
    if (CourseIndex < 0) {
        n = SlotOrder.cursor24;
        h = (unsigned short)SlotOrder.order04[n];
        n = n + 1;
        SlotOrder.cursor24 = n;
        CourseIndex = h;
        if (n >= 8)
            ShuffleSlotOrder();
    }
    InitRaceOpeningOverlay(0, 0);
    GameModeType = 5;
    MenuTransitionOverlayMode = 0;
    MenuScriptId = 0xA;
    ModeGroup = 1;
    ModeIndex = 0x23;
    SubmodeState = 0;
    if (a0 == 0) {
        StartDemoFadeIn(-1);
        SoundState.track28A4 = CourseBgmChoiceTable2[CourseIndex] + 1;
    } else {
        InitDemoFadeContext(0);
        SelectDemoFadePreset(2);
        SoundState.track28A4 = 1;
    }
    InitSceneOverlayState();
    d0 = &RaceContext.attract4BC;
    MaskBitSetting = 1;
    d0->overlayHsv24[0] = 0xD2;
    d0->overlayHsv24[1] = 0;
    d0->overlayHsv24[2] = 0x8FC;
    HsvToRgb(d0->overlayHsv24, d0->overlayRgb2C);
    SetSceneOverlayColor(0, d0->overlayRgb2C[0], d0->overlayRgb2C[1], d0->overlayRgb2C[2]);
    d0->viewMode78 = 0;
    if (a0 == 0) {
        x5 = RaceContext.overlayToggle3DC;
    } else {
        RaceContext.overlayToggle3DC = 0;
        x5 = 0;
    }
    SetSceneOverlayEnabled(x5);
    d0 = &RaceContext.attract4BC;
    d0->camera30[1] = -0x20;
    d0->camera30[0] = 0;
    d0->camera30[2] = 0;
    d0->camera30[4] = 0;
    d0->camera30[5] = 0;
    d0->camera30[6] = 0;
    d0->camera30[10] = 0x122;
    SoundReverbOffAll();
    ClearTrackObjectStates(0);
    ActiveSceneId = 0x209;
}

void AttractDemoRaceEnter(void) {
    SetDispMask(0);
    ResetScreenCounters();
    MenuScriptId = 8;
    ModeGroup = 1;
    ModeIndex = 0x23;
    SubmodeState = 0;
    InitDemoFadeContext(0);
    SelectDemoFadePreset(1);
    DrawSubOtFlag = 0;
}

/* Initial values of the globals this module owns. */
int MenuTransitionFadeRates[4] = {1, 3, 2};
unsigned short LoadingBarAnimCounter = 0xA00F;
unsigned short MenuStageEntryModeIds[16] = {0x8004, 0, 0x8008, 4, 0x8001, 2, 0x8002, 1, 0x2004, 6, 0x2008, 3, 0x2001, 5, 0x2002, 7};
int AttractCarTeams[4];
