#include "game/ui/badgeprim.h"
#include "rr/sprite_emit.h"
#include "types.h"
#include "psyq/libgpu/primattr.h"
#include "psyq/libgpu/settrans.h"
#include "rr/animated_glyph.h"

void *AddSpritePrim(GpuDmaTag *ordering, void *packet, s32 x, s32 y, const UiUvRect *region, u16 clut, s32 flags) {
    s32 brightness = flags >> 1;
    SetSprt(packet);
    if (brightness == 0) {
        SetShadeTex(packet, 1);
    } else {
        SpritePacket *sprite = packet;
        sprite->r = brightness;
        sprite->g = brightness;
        sprite->b = brightness;
    }
    SetSemiTrans(packet, flags & 1);
    {
        SpritePacket *sprite = packet;
        sprite->x = x;
        sprite->y = y;
        sprite->w = region->w;
        sprite->h = region->h;
        sprite->u = region->u;
        sprite->v = region->v;
        sprite->clut = clut;
        packet = sprite + 1;
        RR_GPU_LINK(ordering, sprite);
    }
    return packet;
}
