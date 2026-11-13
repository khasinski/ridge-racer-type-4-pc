#include "rr/release.h"
#include "rr/cpu_time.h"
#include "game/ui/badge1.h"
#include "types.h"
#include "rr/badge_rows.h"
#include "psyq/libgpu/gpudisp.h"
#include "rr/frame_loop.h"
#include "psyq/libgpu/primatt5.h"
#include "rr/animated_glyph.h"
#include "rr/hud_number.h"

#include "rr/sprite_emit.h"




void *AddDrawAreaPrim(GpuDmaTag *ordering, void *cursor, s32 x, s32 y, s32 width, s32 height) {
    GpuDrawModePacket *packet = cursor;
    GpuRectangleFields rectangle;
    int bank;
    rectangle.x = x;
    if (DrawBuffers[0].environment.display5C.display.y == DrawBuffers[1].environment.display5C.display.y) {
        rectangle.y = y;
    } else {
        bank = DrawBufIndex;
        rectangle.y = (s32)((u32)y + (u32)bank * (u32)RR_SCREEN_BOTTOM);
    }
    rectangle.w = width;
    rectangle.h = height;
    SetDrawArea(packet, &rectangle);
    RR_GPU_LINK(ordering, packet);
    packet++;
    return packet;
}

void *AddDrawModePrim(GpuDmaTag *ordering, GpuDrawModePacket *packet, u16 page) {
    SetDrawMode(packet, 0, 1, page, 0);
    RR_GPU_LINK(ordering, packet);
    packet++;
    return packet;
}

void *AddTilePrim(GpuDmaTag *ordering, void *cursor, s32 x, s32 y, s32 width,
                  s32 height, s32 red, s32 green, s32 blue) {
    RR_CPU_CYCLES(RR_CYCLES_ADD_TILE_PRIM);
    GpuTilePacket *tile = cursor;
    SetTile(tile);
    tile->x = x;
    tile->y = y;
    tile->w = width;
    tile->h = height;
    tile->r = red;
    tile->g = green;
    tile->b = blue;
    RR_GPU_LINK(ordering, tile);
    return tile + 1;
}

void *AddTileGrid2x2(GpuDmaTag *ordering, void *cursor, int x, int y, int width,
                     int height, int red, int green, int blue) {
    RR_CPU_CYCLES(RR_CYCLES_ADD_TILE_GRID_2X2);
    int i = 0;
    int halfWidth = width / 2;
    int halfHeight = height / 2;
    do {
        u32 tileX = x;
        u32 tileY = y;
        tileX += halfWidth * (i & 1);
        if (i >= 2)
            tileY += halfHeight;
        cursor = AddTilePrim(ordering, cursor, tileX, tileY, halfWidth,
                             halfHeight, red, green, blue);
        i++;
    } while (i < 4);
    return cursor;
}
