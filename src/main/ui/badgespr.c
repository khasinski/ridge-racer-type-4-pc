#include "game/ui/badgespr.h"
#include "rr/cpu_work.h"
#include "rr/badge_rows.h"
#include "psyq/libgpu/primattr.h"
#include "psyq/libgpu/settrans.h"
#include "rr/animated_glyph.h"
#include "rr/menu_label_render.h"


void *AddSpriteWithDrawMode(GpuDmaTag *ordering, void *cursor,
    int x, int y, int width, int height, int u, int v,
    unsigned short page, unsigned short clut, s32 transparency, s32 brightness)
{
    u16 semi = (u16)transparency;
    u16 intensity = (u16)brightness;
    SpritePacket *packet;
    if (!RrWorkFrame)
        RR_CPU_CYCLES(RR_CYCLES_ADD_SPRITE_WITH_DRAW_MODE);
    packet = AddDrawModePrim(ordering, cursor, 5);
    SetSprt(packet);
    packet->x = x;
    packet->y = y;
    packet->w = width;
    packet->h = height;
    packet->u = u;
    packet->v = v;
    SetSemiTrans(packet, semi);
    if (intensity != 0x80) {
        SetShadeTex(packet, 0);
        packet->r = intensity;
        packet->g = intensity;
        packet->b = intensity;
    } else {
        SetShadeTex(packet, 1);
    }
    packet->clut = clut;
    RR_GPU_LINK(ordering, packet);
    packet++;
    return AddDrawModePrim(ordering, (GpuDrawModePacket *)packet, page);
}
