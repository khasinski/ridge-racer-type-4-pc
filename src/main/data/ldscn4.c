#include "game/data/cdfile2.h"
#include "game/data/ldscn1.h"
#include "rr/load_cursor.h"
#include "game/data/trkld6.h"
#include "game/gfx/fadespr.h"
#include "game/data/ldscn4.h"
#include "game/data/cdfile1.h"
#include "rr/course_choices.h"
#include "game/course/trkintp2.h"
#include "game/data/trkld1.h"
#include "psyq/libc/rand.h"
#include "rr/cd_directory_buffer.h"
#include "rr/menu_arena_base.h"
#include "rr/overlay_module.h"

int LoadAttractTimStep(void) {
    int r = 0;
    int t;
    switch (LoadState) {
    case 0:
        LoadState = 1;
        LoadIndex = 0;
    case 1:
        if (CdFileReadStart(GetR4FileIndex(1, 0x10), LoadCursor) != 0) {
            LoadState = 2;
        }
        break;
    case 2:
        if (CdFileBytesLoaded() != 0) {
            LoadTimList(LoadCursor);
            LoadState = 3;
            LoadCursor = CdDirectoryBuffer;
        }
        break;
    case 3:
        /* movie_playback is linked, but entry 668 is still read (disc time,
         * arena space); loading resets the module. */
        if (CdFileReadStart(GetR4FileIndex(1, 10), LoadCursor) != 0) {
            LoadState = 4;
        }
        break;
    case 4:
        t = CdFileBytesLoaded();
        if (t != 0) {
            OverlayModuleBind(GetR4FileIndex(1, 10));
            LoadCursor = (u8 *)LoadCursor + (t);
            LoadState = 5;
            MenuArenaBase = LoadCursor;
            RaceLoadReset();
        }
        break;
    case 5:
        if (LoadAttractRaceStep() == 1) {
            InitDemoFadeContext(PsyqRand() & 1);
            LoadState = 6;
    case 6:
            r = 1;
        }
        break;
    }
    return r;
}
