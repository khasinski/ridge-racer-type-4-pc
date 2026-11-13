#include "rr/release.h"
#include "rr/overlay_module.h"
#include "rr/overlay662_draw_area.h"
#include "rr/overlay662_garage.h"
#include "rr/frame_loop.h"
#include "psyq/libgpu/gpudisp.h"
#include "rr/gpu_address.h"
#include "rr/overlay662_primitives.h"
#include "rr/overlay662_line.h"
#include "rr/overlay662_gradient.h"
#include "rr/overlay662_sprites.h"
#include "rr/animated_sprite_grid.h"

u8 *ovl662_emit_draw_area(GpuDmaTag *ordering, void *cursor, u32 x, u32 y, u32 width, u32 height) {
    GpuDrawModePacket *packet = cursor; /* DR_AREA: tag and two command words */
    u16 w = width, h = height;
    GpuRectangleFields rect;
    s16 py;
    rect.x = ovl662_editor_workspace.halfC8 + x;
    py = ovl662_editor_workspace.halfCA + y;
    if (DrawBuffers[0].environment.display5C.display.y == DrawBuffers[1].environment.display5C.display.y) {
        rect.y = py;
    } else {
        rect.y = py + ((u32)DrawBufIndex * (u32)RR_SCREEN_BOTTOM);
    }
    rect.w = w;
    rect.h = h;
    SetDrawArea(packet, &rect);
    RR_GPU_LINK(ordering, packet);
    return (u8 *)(packet + 1);
}

GpuDrawModePacket *ovl662_emit_draw_mode(GpuDmaTag *ordering,GpuDrawModePacket *packet,u32 x,u32 y)
{
    s32 page;
    page = ((s32)((y & 0x100) << 16) >> 20) | ((x & 0x3FF) >> 6) | ((s16)(y & 0x200) * 4);
    SetDrawMode(packet,0,1,page,0);
    RR_GPU_LINK(ordering, packet);
    return packet+1;
}

GpuTilePacket *ovl662_emit_hline_tile(GpuDmaTag *ordering, GpuTilePacket *packet, u32 x0, u32 y, u32 x1, u32 r, u32 g, u32 b)
{
    u16 baseX = ovl662_editor_workspace.halfC8;
    u16 baseY = ovl662_editor_workspace.halfCA;
    u16 px0;
    u16 px1 = x1;

    packet->tag.length = 3;
    packet->code = 0x40;
    px0 = baseX + x0;
    baseY = baseY + y;
    px1 = px1 + baseX;
    packet->x = px0;
    packet->y = baseY;
    packet->w = px1;
    packet->h = baseY;
    packet->r = r;
    packet->g = g;
    packet->b = b;
    RR_GPU_LINK(ordering, packet);
    packet++;

    packet->tag.length = 3;
    packet->code = 0x40;
    baseY = baseY + 1;
    packet->x = px0;
    packet->y = baseY;
    packet->w = px1;
    packet->h = baseY;
    packet->r = r;
    packet->g = g;
    packet->b = b;
    RR_GPU_LINK(ordering, packet);
    return packet + 1;
}

GpuLineF2Packet *ovl662_emit_vline_prim(GpuDmaTag *ordering, GpuLineF2Packet *packet, u32 x, u32 y0, u32 y1, u32 r, u32 g, u32 b)
{
    u16 baseX = ovl662_editor_workspace.halfC8;
    u16 rawY = ovl662_editor_workspace.halfCA;
    u32 baseY = rawY;
    packet->tag.length=3;
    packet->code=0x40;
    packet->x0=baseX+x;
    packet->x1=baseX+x;
    packet->y0=baseY+y0;
    packet->y1=y1+baseY;
    packet->r=r;packet->g=g;packet->b=b;
    RR_GPU_LINK(ordering, packet);
    return packet+1;
}

GpuLineF4Packet *ovl662_emit_wire_box(GpuDmaTag *ordering, GpuLineF4Packet *packet, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b)
{
    u16 baseX = ovl662_editor_workspace.halfC8;
    u16 baseY = ovl662_editor_workspace.halfCA;
    u16 xEnd;
    u16 yEnd;

    packet->tag.length = 6;
    packet->code = 0x4C;
    packet->pad = 0x55555555;
    packet->code &= ~2;
    baseX += x;
    baseY += y;
    xEnd = baseX + w;
    yEnd = baseY + h;
    packet->x0 = baseX;
    packet->y0 = baseY;
    packet->x1 = xEnd - 1;
    packet->y1 = baseY;
    packet->x2 = xEnd - 1;
    packet->y2 = yEnd - 1;
    packet->x3 = baseX;
    packet->y3 = yEnd - 1;
    packet->r = r;
    packet->g = g;
    packet->b = b;
    RR_GPU_LINK(ordering, packet);
    packet++;

    packet->tag.length = 6;
    packet->code = 0x4C;
    packet->pad = 0x55555555;
    packet->code &= ~2;
    xEnd -= 2;
    baseY += 1;
    yEnd -= 2;
    packet->x0 = xEnd;
    packet->y0 = baseY;
    packet->x1 = baseX;
    packet->y1 = baseY;
    packet->x2 = baseX;
    packet->y2 = yEnd;
    packet->x3 = xEnd;
    packet->y3 = yEnd;
    packet->r = r;
    packet->g = g;
    packet->b = b;
    RR_GPU_LINK(ordering, packet);
    return packet + 1;
}

GpuTilePacket *ovl662_emit_filled_tile(GpuDmaTag *ordering, GpuTilePacket *packet, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b)
{
    u16 baseX=ovl662_editor_workspace.halfC8;
    u16 baseY=ovl662_editor_workspace.halfCA;
    packet->tag.length=3;
    packet->code=0x60;
    packet->x=baseX+x;
    packet->y=baseY+y;
    packet->w=w;
    packet->h=h;
    packet->r=r;packet->g=g;packet->b=b;
    RR_GPU_LINK(ordering, packet);
    return packet+1;
}

PolyG4Storage *ovl662_emit_gradient_quad(GpuDmaTag *ordering, PolyG4Storage *packet, u32 x, u32 y, u32 w, u32 h, u32 r0, u32 g0, u32 b0, u32 r1, u32 g1, u32 b1) {
    u16 baseX = ovl662_editor_workspace.halfC8;
    u16 baseY = ovl662_editor_workspace.halfCA;
    u16 x1;
    packet->packet.tag.length = 8;
    packet->packet.code = 0x38;
    baseX += x;
    baseY += y;
    x1 = baseX + w - 1;
    packet->packet.x0 = baseX;
    packet->packet.y0 = baseY;
    packet->packet.x1 = x1;
    packet->packet.y1 = baseY;
    baseY = baseY + h - 1;
    packet->packet.x2 = baseX;
    packet->packet.y2 = baseY;
    packet->packet.x3 = x1;
    packet->packet.y3 = baseY;
    packet->packet.r0 = r0;
    packet->packet.g0 = g0;
    packet->packet.b0 = b0;
    packet->packet.r1 = r1;
    packet->packet.g1 = g1;
    packet->packet.b1 = b1;
    packet->packet.r2 = r0;
    packet->packet.g2 = g0;
    packet->packet.b2 = b0;
    packet->packet.r3 = r1;
    packet->packet.g3 = g1;
    packet->packet.b3 = b1;
    RR_GPU_LINK(ordering, &packet->packet);
    return packet + 1;
}

GpuPolyFT4Packet *ovl662_emit_textured_quad(GpuDmaTag *ordering, GpuPolyFT4Packet *packet, u32 x, u32 y, u32 w, u32 h, u32 u, u32 v, u32 uw, u32 vh, u32 clut, u32 tpage)
{
    u16 baseX = ovl662_editor_workspace.halfC8;
    u16 baseY = ovl662_editor_workspace.halfCA;
    u16 x1;
    u16 u1;
    u16 v1;

    packet->tag.length = 9;
    packet->code = 0x2D;
    u1 = u + uw - 1;
    v1 = v + vh - 1;
    baseX += x;
    baseY += y;
    x1 = baseX + w - 1;
    packet->x0 = baseX;
    packet->y0 = baseY;
    packet->x1 = x1;
    packet->y1 = baseY;
    baseY = baseY + h - 1;
    packet->x2 = baseX;
    packet->y2 = baseY;
    packet->x3 = x1;
    packet->y3 = baseY;
    packet->u0 = u;
    packet->v0 = v;
    packet->u1 = u1;
    packet->v1 = v;
    packet->u2 = u;
    packet->v2 = v1;
    packet->u3 = u1;
    packet->v3 = v1;
    packet->clut = clut;
    packet->tpage = tpage;
    RR_GPU_LINK(ordering, packet);
    return packet + 1;
}

u8 *ovl662_emit_indexed_sprite(s32 index,u8 *cursor,s32 x,s32 y)
{
    GpuDmaTag *ordering=RR_GPU_SCRATCH->order+128;
    SpritePacket *packet=(SpritePacket *)cursor;
    GarageSpriteRecord *record;
    s32 i;
    if (index<0) {
        for (i=0;i<72;i++) {
            record=&ovl662_sprite_records[i];
            ovl662_sprite_lookup[record->key0C]=record;
        }
        return 0;
    }
    record=ovl662_sprite_lookup[index];
    packet->tag.length=4;
    packet->code=0x64;
    if (record->flags10 & 1) packet->code = packet->code | 2;
    else packet->code = packet->code & ~2;
    packet->x=x;packet->y=y;
    packet->code |=1;
    packet->w=record->width04;
    packet->h=record->height06;
    packet->u=record->u08;packet->v=record->v09;
    packet->clut=record->clut0A;
    RR_GPU_LINK(&ordering[record->depth00], packet);
    return (u8 *)(packet+1);
}

void ovl662_init_editor_draw_env(void)
{
    GpuScratchPointers *scratch=RR_GPU_SCRATCH;
    GpuDmaTag *order=scratch->order;
    scratch->cursor=ovl662_emit_draw_area(order+146, scratch->cursor,12,61,296,352);
    scratch->cursor=(u8 *)ovl662_emit_draw_mode(order+133,(GpuDrawModePacket *)scratch->cursor,704,0);
    scratch->cursor=(u8 *)ovl662_emit_draw_mode(order+147,(GpuDrawModePacket *)scratch->cursor,704,0);
}

/* Initial values of the overlay globals this module owns. */
GarageSpriteRecord ovl662_sprite_records[72] = {{.depth00 = 4, .width04 = 8, .height06 = 0x10, .u08 = 0x40, .v09 = 0x48, .clut0A = 0x252C}, {.depth00 = 4, .width04 = 8, .height06 = 0x10, .u08 = 0x48, .v09 = 0x48, .clut0A = 0x252C, .key0C = 1}, {.depth00 = 11, .width04 = 0x38, .height06 = 0x10, .u08 = 0x78, .v09 = 0x20, .clut0A = 0x252C, .key0C = 2}, {.depth00 = 11, .width04 = 0x38, .height06 = 0x10, .u08 = 0x78, .v09 = 0x20, .clut0A = 0x252D, .key0C = 3}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .clut0A = 0x252C, .key0C = 4}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .clut0A = 0x252D, .key0C = 5}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x10, .clut0A = 0x252C, .key0C = 6}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x10, .clut0A = 0x252D, .key0C = 7}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x20, .clut0A = 0x252C, .key0C = 8}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x20, .clut0A = 0x252D, .key0C = 9}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x30, .clut0A = 0x252C, .key0C = 10}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x30, .clut0A = 0x252D, .key0C = 11}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x40, .clut0A = 0x252C, .key0C = 12}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x40, .clut0A = 0x252D, .key0C = 13}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x50, .clut0A = 0x252C, .key0C = 14}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x50, .clut0A = 0x252D, .key0C = 15}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x20, .u08 = 0xC0, .clut0A = 0x252C, .key0C = 16}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x20, .u08 = 0xC0, .clut0A = 0x252D, .key0C = 17}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x60, .clut0A = 0x252C, .key0C = 18}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x60, .clut0A = 0x252D, .key0C = 19}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x70, .clut0A = 0x252C, .key0C = 20}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x70, .clut0A = 0x252D, .key0C = 21}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x80, .clut0A = 0x252C, .key0C = 22}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x80, .clut0A = 0x252D, .key0C = 23}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x90, .clut0A = 0x252C, .key0C = 24}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0x90, .clut0A = 0x252D, .key0C = 25}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .v09 = 0x20, .clut0A = 0x252C, .key0C = 26}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .v09 = 0x20, .clut0A = 0x252D, .key0C = 27}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .u08 = 0x14, .v09 = 0x20, .clut0A = 0x252C, .key0C = 28}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .u08 = 0x14, .v09 = 0x20, .clut0A = 0x252D, .key0C = 29}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .u08 = 0x28, .v09 = 0x20, .clut0A = 0x252C, .key0C = 30}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .u08 = 0x28, .v09 = 0x20, .clut0A = 0x252D, .key0C = 31}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .u08 = 0x3C, .v09 = 0x20, .clut0A = 0x252C, .key0C = 32}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .u08 = 0x3C, .v09 = 0x20, .clut0A = 0x252D, .key0C = 33}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .u08 = 0x50, .v09 = 0x20, .clut0A = 0x252C, .key0C = 34}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .u08 = 0x50, .v09 = 0x20, .clut0A = 0x252D, .key0C = 35}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .u08 = 0x64, .v09 = 0x20, .clut0A = 0x252C, .key0C = 36}, {.depth00 = 11, .width04 = 0x14, .height06 = 0x28, .u08 = 0x64, .v09 = 0x20, .clut0A = 0x252D, .key0C = 37}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0xA0, .clut0A = 0x252C, .key0C = 38}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0xA0, .clut0A = 0x252D, .key0C = 39}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0xB0, .clut0A = 0x252C, .key0C = 40}, {.depth00 = 11, .width04 = 0x10, .height06 = 0x20, .u08 = 0xB0, .clut0A = 0x252D, .key0C = 41}, {.depth00 = 11, .width04 = 0x20, .height06 = 0x20, .v09 = 0x48, .clut0A = 0x252C, .key0C = 42}, {.depth00 = 11, .width04 = 0x20, .height06 = 0x20, .v09 = 0x48, .clut0A = 0x252D, .key0C = 43}, {.depth00 = 11, .width04 = 0x20, .height06 = 0x20, .u08 = 0x20, .v09 = 0x48, .clut0A = 0x252C, .key0C = 44}, {.depth00 = 11, .width04 = 0x20, .height06 = 0x20, .u08 = 0x20, .v09 = 0x48, .clut0A = 0x252D, .key0C = 45}, {.depth00 = 11, .width04 = 7, .height06 = 0x2C, .v09 = 0x68, .clut0A = 0x252E, .key0C = 46}, {.depth00 = 11, .width04 = 6, .height06 = 0x12, .u08 = 0xEC, .clut0A = 0x252C, .key0C = 47}, {.depth00 = 11, .width04 = 6, .height06 = 0x12, .u08 = 0xEC, .clut0A = 0x252D, .key0C = 48}, {.depth00 = 11, .width04 = 6, .height06 = 0x12, .u08 = 0xF4, .clut0A = 0x252C, .key0C = 49}, {.depth00 = 11, .width04 = 6, .height06 = 0x12, .u08 = 0xF4, .clut0A = 0x252D, .key0C = 50}, {.depth00 = 11, .width04 = 9, .height06 = 0xC, .u08 = 0xD4, .clut0A = 0x252C, .key0C = 51}, {.depth00 = 11, .width04 = 9, .height06 = 0xC, .u08 = 0xD4, .clut0A = 0x252D, .key0C = 52}, {.depth00 = 11, .width04 = 8, .height06 = 0xC, .u08 = 0xD4, .v09 = 0x10, .clut0A = 0x252C, .key0C = 53}, {.depth00 = 11, .width04 = 8, .height06 = 0xC, .u08 = 0xD4, .v09 = 0x10, .clut0A = 0x252D, .key0C = 54}, {.depth00 = 11, .width04 = 9, .height06 = 0xC, .u08 = 0xE0, .clut0A = 0x252C, .key0C = 55}, {.depth00 = 11, .width04 = 9, .height06 = 0xC, .u08 = 0xE0, .clut0A = 0x252D, .key0C = 56}, {.depth00 = 11, .width04 = 8, .height06 = 0xC, .u08 = 0xDC, .v09 = 0x10, .clut0A = 0x252C, .key0C = 57}, {.depth00 = 11, .width04 = 8, .height06 = 0xC, .u08 = 0xDC, .v09 = 0x10, .clut0A = 0x252D, .key0C = 58}, {.depth00 = 11, .width04 = 4, .height06 = 0xA, .u08 = 8, .v09 = 0x68, .clut0A = 0x252E, .key0C = 59}, {.depth00 = 11, .width04 = 4, .height06 = 0xA, .u08 = 0xC, .v09 = 0x68, .clut0A = 0x252E, .key0C = 60}, {.depth00 = 11, .width04 = 4, .height06 = 0xA, .u08 = 0x10, .v09 = 0x68, .clut0A = 0x252E, .key0C = 61}, {.depth00 = 11, .width04 = 4, .height06 = 0xA, .u08 = 0x14, .v09 = 0x68, .clut0A = 0x252E, .key0C = 62}, {.depth00 = 11, .width04 = 4, .height06 = 0xA, .u08 = 0x18, .v09 = 0x68, .clut0A = 0x252E, .key0C = 63}, {.depth00 = 11, .width04 = 4, .height06 = 0xA, .u08 = 0x1C, .v09 = 0x68, .clut0A = 0x252E, .key0C = 64}, {.depth00 = 11, .width04 = 4, .height06 = 0xA, .u08 = 0x20, .v09 = 0x68, .clut0A = 0x252E, .key0C = 65}, {.depth00 = 11, .width04 = 4, .height06 = 0xA, .u08 = 0x24, .v09 = 0x68, .clut0A = 0x252E, .key0C = 66}, {.depth00 = 11, .width04 = 4, .height06 = 0xA, .u08 = 0x28, .v09 = 0x68, .clut0A = 0x252E, .key0C = 67}, {.depth00 = 11, .width04 = 4, .height06 = 0xA, .u08 = 0x2C, .v09 = 0x68, .clut0A = 0x252E, .key0C = 68}, {.depth00 = 10, .width04 = 0xC, .height06 = 0x14, .u08 = 0xB0, .v09 = 0x20, .clut0A = 0x252D, .key0C = 69}, {.depth00 = 10, .width04 = 0xB, .height06 = 0x24, .u08 = 0xBC, .v09 = 0x20, .clut0A = 0x252D, .key0C = 70}, {.depth00 = 10, .width04 = 4, .height06 = 0x11, .u08 = 0xC8, .v09 = 0x20, .clut0A = 0x252D, .key0C = 71}};
GarageSpriteRecord *ovl662_sprite_lookup[72];

/* Writable state of this unit; ovl662_reset_module restores it. */
static const OverlayStateRegion ovl662_editor_primitives_regions[] = {
    RR_OVERLAY_STATE(ovl662_sprite_lookup),
    RR_OVERLAY_STATE(ovl662_sprite_records)};
const OverlayStateTable ovl662_editor_primitives_state = {ovl662_editor_primitives_regions, sizeof(ovl662_editor_primitives_regions) / sizeof(ovl662_editor_primitives_regions[0])};
