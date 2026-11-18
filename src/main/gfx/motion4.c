#include "rr/scene_camera_state.h"
#include "rr/presentation.h"
#include "game/gfx/motion4.h"

void UpdateScrollTrack(MotionClock20 *clock)
{
    ScrollTrack40 *s = &MotionControllerView()->scroll;
    s32 i;
    s32 speed;
    s32 ch;

    if (s->disabled != 0) {
        return;
    }
    switch (MotionControllerView()->scroll.state) {
    case 0:
        if (clock->active == 0) {
            return;
        }
        MotionControllerView()->scroll.state = 2;
    case 1:
        if (clock->active != 0) {
            s->hold = s->hold - 1U;
            if (s->hold >= 0) {
                break;
            }
            s->hold = 0;
        }
        if (s->hold != 0) {
            break;
        }
        s->wait = s->wait - 1U;
        if (s->wait >= 0) {
            break;
        }
        s->wait = 0;
        s->state++;
    case 2:
        speed = (clock->tempo * ((u32)MotionControllerView()->scale4C << 1)) / 100;
        ch = (s->channel[0].glyph != 0);
        if (speed > 0) {
            s->step = -(0x1900 / speed);
        } else {
            s->step = -0xA0;
        }
        s->channel[ch].glyph = s->keys[s->index].glyph;
        s->channel[ch].offset = 0;
        s->hold = (s16)s->keys[s->index].timing >> 8;
        s->wait = (((clock->tempo * MotionControllerView()->scale4C) / 100) * (u8)s->keys[s->index].timing) / 12;
        if (s->keys[++s->index].glyph < 0) {
            s->index = 0;
        }
        s->state = 1;
        break;
    }
    for (i = 0; i < 2; i++) {
        if (s->channel[i].glyph > 0) {
            s->channel[i].offset += (u32)s->step;
            if ((s16)s->channel[i].offset < -0x95F) {
                s->channel[i].glyph = 0;
            }
        }
    }
}
