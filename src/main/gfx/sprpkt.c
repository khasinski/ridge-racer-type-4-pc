#include "rr/release.h"
#include "rr/widescreen.h"
#include "game/sound/sndque.h"
#include "rr/frame_storage_types.h"
#include "rr/gpu_scratch_types.h"
#include "rr/gpu.h"
#include "game/ui/uipart1.h"
#include "game/race/raceset.h"
#include "rr/sprite_emit.h"
#include "rr/fade_sprite.h"
#include "rr/audio_loop.h"
#include "rr/race_setup.h"
#include "game/gfx/sprpkt.h"
extern s32 RaceState;
extern s32 SplitTimes[];
void DrawRaceOverlaySprites(s32 a0)
{
    s32 s1, s2, s3, s4, s5, t;
    s32 c;
    SpritePacket *p;
    if (GameModeType == 0 && RaceState != 0 && RaceState < 4) {
        t = SplitTimes[SplitIndex];
        if (t < 0) {
            s3 = -1;
            s4 = -1;
            s2 = 0;
        } else {
            s4 = t;
            if (SplitDeltaTimer > 0) {
                if (a0 == 0) SplitDeltaTimer--;
                s3 = LatchedBestSplitTime;
                s2 = LatchedSplitIndex;
                s4 = LatchedSplitTime;
                s1 = SplitTimes[s2] - s3;
                s5 = 0x84;
                if (s1 < 0) {
                    s5 = 0x78;
                    s1 = -s1;
                    c = 0x15;
                } else {
                    c = 0x16;
                }
                if (SplitDeltaTimer == 0x3B && s2 < 2 && a0 == 0) SoundQueueEnqueue(c, 0);
                RR_GPU_SCRATCH->cursor = (u8 *)GameQueueSprite(&CurrentDrawBuf->buffer.ordering70.tags[0], (SpritePacket *)RR_GPU_SCRATCH->cursor, 0x70, RR_PAL_JP(0x60, 0x58), 0xC, 0xC, s5, 0xA8, 0x7EF8);
                DrawRaceTime(0x80, RR_PAL_JP(0x60, 0x58), s1, 0x7EF8, 1);
            } else {
                s3 = BestSplitTimes[SplitIndex];
                s2 = SplitIndex;
            }
        }
        DrawRaceTime(WIDE_HUD_LEFT(0x14), 0x1C, s3, 0x7EF8, 0);
        DrawRaceTime(WIDE_HUD_LEFT(0x14), 0x26, s4, 0x7EF8, 0);
        p = (SpritePacket *)RR_GPU_SCRATCH->cursor;
        p->tag.length = 4;
        p->code = 0x64;
        p->x = (s16)WIDE_HUD_LEFT(0xA);
        p->y = 0x26;
        p->w = 0xC;
        p->h = 0xC;
        p->u = s2 * 12 + 12;
        p->v = 0xA8;
        p->clut = 0x7EF8;
        p->code |= 1;
        RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[0], p);
        p++;
        RR_GPU_SCRATCH->cursor = (u8 *)p;
    }
}

/* Initial values of the globals this module owns. */
s32 SplitDeltaTimer;
