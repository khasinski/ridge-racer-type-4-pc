#include "rr/release.h"
#include "rr/gpu_scratch_types.h"
#include "rr/frame_storage_types.h"
#include "rr/screen_tiles.h"
#include "psyq/libgpu/primatt5.h"
#include "psyq/libgpu/settrans.h"
#include "rr/sprite_emit.h"
#include "rr/animated_glyph.h"
#include "rr/widescreen.h"
#include <psyz/video.h>
void AddScreenFadeTiles(GpuDmaTag *ordering,s32 intensity,u32 mode) {
    GpuTilePacket *tile;
    s32 index;
    s32 column;
    GpuRectangleFields *rectangle;
    s32 margin=WideMargin();
    if(intensity<0)intensity=0;
    else if(intensity>=256)intensity=255;
    tile=(GpuTilePacket *)RR_GPU_SCRATCH->cursor;
    if(RR_RELEASE_JP){
        /* The Japanese release covers its screen with one tile; the PAL
         * one with four 160-wide tiles of half the height. */
        SetTile(tile);
        SetSemiTrans(tile,1);
        tile->x=-margin;
        tile->y=0;
        tile->w=0x140+2*margin;
        tile->h=CurrentDrawBuf->buffer.environment00.draw00.clip.h;
        tile->r=intensity;tile->g=intensity;tile->b=intensity;
        AddPrim(ordering,tile);
        tile++;
        RR_GPU_SCRATCH->cursor=(u8 *)tile;
        RR_GPU_SCRATCH->cursor=AddDrawModePrim(ordering,(GpuDrawModePacket *)tile,(u16)mode);
        return;
    }
    index=0;
    do {
        SetTile(tile);
        SetSemiTrans(tile,1);
        column=index&1;
        rectangle=&CurrentDrawBuf->buffer.environment00.draw00.clip;
        /* Widescreen: the outer columns cover the squeezed buffer's sides. */
        tile->x=column*160-(column?0:margin);
        tile->w=160+margin;
        tile->y=(rectangle->h/2)*(index/2);
        tile->r=intensity;tile->g=intensity;tile->b=intensity;
        tile->h=rectangle->h/2;
        AddPrim(ordering,tile);
        tile++;
        index++;
    }while(index<4);
    RR_GPU_SCRATCH->cursor=(u8 *)tile;
    RR_GPU_SCRATCH->cursor=AddDrawModePrim(ordering,(GpuDrawModePacket *)tile,(u16)mode);
}
