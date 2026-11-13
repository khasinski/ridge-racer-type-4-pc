/* The syntax of the RR4_* test hooks (src/port/diagnostics_parse.c): every
 * accepted form the scripts use, and malformed entries skipped. */

#include <psyz.h>
#include <psyz/input.h>

#include <stdio.h>
#include <string.h>

#include "diagnostics_parse.h"

static int failures;

#define CHECK(cond)                                                                  \
    do {                                                                             \
        if (!(cond)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                              \
        }                                                                            \
    } while (0)

static void TestNumbers(void) {
    long v;
    const char *end;
    int mode;

    CHECK(Rr4DiagParseLong("1300", NULL, 10, &v) && v == 1300);
    CHECK(Rr4DiagParseLong("-5", NULL, 0, &v) && v == -5);
    CHECK(Rr4DiagParseLong("0x26b0", NULL, 0, &v) && v == 0x26B0);
    CHECK(!Rr4DiagParseLong("12abc", NULL, 10, &v));
    CHECK(!Rr4DiagParseLong("", NULL, 10, &v));
    CHECK(!Rr4DiagParseLong(" 4", NULL, 10, &v));
    CHECK(!Rr4DiagParseLong("99999999999999999999999", NULL, 10, &v));
    CHECK(Rr4DiagParseLong("400+4", &end, 10, &v) && v == 400 && !strcmp(end, "+4"));

    CHECK(Rr4DiagParseAnchor("900", NULL, &mode, &v) && mode == -1 && v == 900);
    CHECK(Rr4DiagParseAnchor("m1/47:600", NULL, &mode, &v) && mode == (1 << 8 | 47) && v == 600);
    CHECK(Rr4DiagParseAnchor("m2", NULL, &mode, &v) && mode == 2 << 8 && v == 0);
    CHECK(Rr4DiagParseAnchor("m1/12", NULL, &mode, &v) && mode == (1 << 8 | 12) && v == 0);
    CHECK(!Rr4DiagParseAnchor("m", NULL, &mode, &v));
    CHECK(!Rr4DiagParseAnchor("m1/:3", NULL, &mode, &v));
    CHECK(!Rr4DiagParseAnchor("m1/12:", NULL, &mode, &v));
    CHECK(!Rr4DiagParseAnchor("4x", NULL, &mode, &v));
}

static void TestPadScript(void) {
    Rr4PadStep s[16];
    int ports[2] = {0, 0}, n;

    n = Rr4DiagParsePadScript("start@400+4,start@900+4", s, 16, ports);
    CHECK(n == 2 && s[0].mask == 0x0800 && s[0].first == 400 && s[0].count == 4 && s[0].mode == -1);
    CHECK(s[1].first == 900 && ports[0] && !ports[1]);

    ports[0] = ports[1] = 0;
    n = Rr4DiagParsePadScript("cross@900,2:cross@900+6,1:up@1200,circle@m1/12:60+4%154", s, 16, ports);
    CHECK(n == 4);
    CHECK(s[0].mask == 0x0040 && s[0].count == 4 && s[0].port == 0); /* default 4 frames */
    CHECK(s[1].port == 1 && s[1].count == 6 && ports[1]);
    CHECK(s[2].port == 0 && s[2].mask == 0x1000);
    CHECK(s[3].mode == (1 << 8 | 12) && s[3].first == 60 && s[3].count == 4 && s[3].period == 154);

    n = Rr4DiagParsePadScript("steer=0.5@m1/53:100+200,i=1@m1/53:0+400,lx=-1@5", s, 16, ports);
    CHECK(n == 3 && s[0].axis == RR4_PAD_AXIS_STEER && s[0].value == 0.5f && s[0].mask == 0);
    CHECK(s[0].count == 200 && s[1].axis == RR4_PAD_AXIS_I && s[1].value == 1.0f);
    CHECK(s[2].axis == RR4_PAD_AXIS_LX && s[2].value == -1.0f && s[2].first == 5);

    /* Bad steps are skipped, the rest is kept. */
    n = Rr4DiagParsePadScript("jump@10,start,start@x,start@10+,steer=@5,steer=fast@5,select@20", s, 16, ports);
    CHECK(n == 1 && s[0].mask == 0x0100 && s[0].first == 20);
    CHECK(Rr4DiagParsePadScript(NULL, s, 16, ports) == 0);
    CHECK(Rr4DiagParsePadScript("start@1,start@2,start@3", s, 2, ports) == 2);
    CHECK(Rr4DiagButtonMask("square", 6) == 0x0080 && Rr4DiagButtonMask("squar", 5) == 0);
}

static void TestRanges(void) {
    Rr4VsyncRange r[8];
    int n = Rr4DiagParseRanges("800,1000-1100/50,1200-1210,5/0", 1, r, 8, "RR4_CAPTURE_AT");

    CHECK(n == 4);
    CHECK(r[0].first == 800 && r[0].last == 800 && r[0].step == 1);
    CHECK(r[1].first == 1000 && r[1].last == 1100 && r[1].step == 50);
    CHECK(r[2].last == 1210 && r[2].step == 1);
    CHECK(r[3].first == 5 && r[3].step == 1); /* a step below 1 is 1 */
    n = Rr4DiagParseRanges("10,abc,20-,30", 1, r, 8, "RR4_CAPTURE_AT");
    CHECK(n == 2 && r[0].first == 10 && r[1].first == 30);
    n = Rr4DiagParseRanges("400,900-910", 0, r, 8, "RR4_GPU_DUMP_AT"); /* plain numbers only */
    CHECK(n == 1 && r[0].first == 400);
}

static void TestWatchAndPoke(void) {
    Rr4WatchSpec w[8];
    Rr4PokeSpec k[8];
    int n;

    n = Rr4DiagParseWatch("TitleSceneStage:2,FadeCountdown:2,SoundState+9904,PlayerCars+0x188:1,Bad:3,X+zz", w, 8);
    CHECK(n == 4);
    CHECK(!strcmp(w[0].name, "TitleSceneStage") && w[0].size == 2 && !strcmp(w[0].label, "TitleSceneStage"));
    CHECK(!strcmp(w[2].name, "SoundState") && w[2].offset == 9904 && w[2].size == 4 &&
          !strcmp(w[2].label, "SoundState+9904"));
    CHECK(!strcmp(w[3].name, "PlayerCars") && w[3].offset == 0x188 && w[3].size == 1 &&
          !strcmp(w[3].label, "PlayerCars+0x188"));

    n = Rr4DiagParsePoke("PlayerCars+388+=170330@3400,rand=12345@m1/47:600,"
                         "PlayerCars+388+=TrackLength@m1/47:10,PlayerCars+388=PlayerCars+1204@7,"
                         "Lap:2=-1@9,nothing,A=1,A=1@x,A:3=1@1",
                         k, 8);
    CHECK(n == 5);
    CHECK(!strcmp(k[0].name, "PlayerCars") && k[0].offset == 388 && k[0].add && k[0].value == 170330 &&
          k[0].vsync == 3400 && k[0].mode == -1 && !k[0].from[0] && k[0].size == 4);
    CHECK(!strcmp(k[1].name, "rand") && !k[1].add && k[1].value == 12345 && k[1].mode == (1 << 8 | 47) &&
          k[1].vsync == 600);
    CHECK(k[2].add && !strcmp(k[2].from, "TrackLength") && k[2].from_offset == 0 && k[2].vsync == 10);
    CHECK(!k[3].add && !strcmp(k[3].from, "PlayerCars") && k[3].from_offset == 1204 && k[3].offset == 388);
    CHECK(!strcmp(k[4].name, "Lap") && k[4].size == 2 && k[4].value == -1);
}

static void TestGraphicsAndPads(void) {
    int kinds[2] = {-1, -1};

    CHECK(Rr4DiagParseGraphics("geometry,nodither,aa,filter,interpolation,wide") == 0x3F);
    CHECK(Rr4DiagParseGraphics("wide") == RR4_GRAPHICS_WIDE);
    /* Exact names: "aa" is not found inside other words. */
    CHECK(Rr4DiagParseGraphics("aaa,widescreen,nofilter") == 0);
    CHECK(Rr4DiagParseGraphics("aa, filter") == (RR4_GRAPHICS_AA | RR4_GRAPHICS_FILTER));
    CHECK(Rr4DiagParseGraphics("") == 0);

    CHECK(Rr4DiagParsePadTypes("negcon", kinds) == 1 && kinds[0] == PSYZ_CTRL_NEGCON);
    CHECK(Rr4DiagParsePadTypes("digital,dualshock,jogcon", kinds) == 2 && kinds[0] == PSYZ_CTRL_DIGITAL_PAD &&
          kinds[1] == PSYZ_CTRL_ANALOG_PAD);
    CHECK(Rr4DiagParsePadTypes("wheel", kinds) == 1 && kinds[0] == PSYZ_CTRL_DIGITAL_PAD);
    CHECK(Rr4DiagParsePadTypes(NULL, kinds) == 0);
}

int main(void) {
    TestNumbers();
    TestPadScript();
    TestRanges();
    TestWatchAndPoke();
    TestGraphicsAndPads();
    if (failures) {
        fprintf(stderr, "diagnostics_test: %d failures\n", failures);
        return 1;
    }
    printf("diagnostics_test: ok\n");
    return 0;
}
