#ifndef RR_OVERLAY668_FRAME_SUBMIT_H
#define RR_OVERLAY668_FRAME_SUBMIT_H
#include "rr/overlay668_frame_acquire.h"
#include "psyq/libcd/dsstrm3.h"
#include "rr/overlay668_vlc.h"
/* Decode into the inactive command buffer, publishing its selector only on
 * success. Return StFreeRing status, -1 for exhausted polling, or -2 after
 * releasing a malformed frame. The playback loop stops on the latter. */
s32 ovl668_decode_next_frame(StrDisplayConfig *config);
#endif
