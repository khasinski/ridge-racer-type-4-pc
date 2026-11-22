#include "scratchpad.h"
#include "rr_asm.h"
#include "game/gfx/light2.h"
#include "rr/menu_overlay_runtime.h"
void SetLightColorsFromAngle(int a0) {
    int *p = (int *)(SCRATCHPAD + 0x7C);
    int s2, v0, z;
    s2 = rsin(a0 / 4 - 0x400) + 0x1000;
    v0 = rsin(a0 / 4);
    p[0] = (s2 << 16) | (s2 & 0xFFFF);
    p[1] = p[0];
    p[2] = p[0];
    p[3] = (v0 << 16) | (v0 & 0xFFFF);
    p[4] = v0;
    RR_GTE_SETLCM(p);
    z = 0;
    RR_GTE_SETBK(z, z, z);
}
