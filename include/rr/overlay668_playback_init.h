#ifndef RR_OVERLAY668_PLAYBACK_INIT_H
#define RR_OVERLAY668_PLAYBACK_INIT_H
#include "game/data/menuvars.h"
#include "rr/overlay668_stream.h"
#include "rr/load_cursor.h"
#include "rr/frame_environment.h"
#include "rr/driver_control.h"
#include "rr/menu_text_rows.h"
/* CD file descriptor: position, byte count and16-byte filename storage. */
typedef struct {
    CdLocation location;
    u32 byteSize;
    char name[16];
} StrCdFileInfo;
#include "rr/menu_arena_base.h"

void ovl668_resolve_stream_file(s32 resource,StrCdFileInfo *file,u32 *sectors);
/* After the display setup service, MenuArenaBase covers an aligned complete
 * playback owner. Publish its base and end, then clear the captured owner.
 * Services may replace roots and scalar state; the resource id is read after
 * both CD calls and must satisfy the resource service's directory contract.
 * The local output descriptors are not read by this initializer. */
void ovl668_init_stream_playback(void);
RR_ASSERT_SIZE(StrCdFileInfo,24);
RR_ASSERT_OFFSET(StrCdFileInfo,byteSize,4);
RR_ASSERT_OFFSET(StrCdFileInfo,name,8);
#endif
