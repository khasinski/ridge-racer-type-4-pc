/* PocketStation command 0x5A over the memory card port (libmcx). The host
 * cards are PsyZ bu directories, which are plain memory cards: a card in a
 * direct slot rejects the PocketStation command (McxErrInvalid), an empty
 * slot (Psyz_CardSlotState) or any other channel has no card
 * (McxErrNoCard). libmcx's vblank handler sends the command at the next
 * vblank, whichever port the BIOS card driver serves, and times it out at
 * the vblank after (Psyz_CardRawBegin). */
#include <string.h>
#include <libmcx.h>
#include <psyz/system.h>
#include "psyq/libcard/cardc162.h"
#include "psyq/libcard/cardsts.h"
#include "game/data/cardini.h"

enum { PDA_CMD_GET_INFO = 0x5A };

static int PdaPending;
static int PdaResult;

s32 _cardCmd5ABegin(s32 request, u8 *buffer) {
    if (PdaPending) {
        return 0;
    }
    if (buffer) {
        memset(buffer, 0, RR_CARD_RAW_REPLY_BYTES);
    }
    PdaResult = ((request & 15) == 0 && (request >> 4) < 2 &&
                 Psyz_CardSlotState(request >> 4) != PSYZ_CARD_NONE)
                    ? McxErrInvalid
                    : McxErrNoCard;
    if (!Psyz_CardRawBegin(request)) {
        return 0;
    }
    PdaPending = 1;
    return 1;
}

int MemCardSync(int mode, int *command, int *result) {
    if (!PdaPending) {
        return McxSyncNone;
    }
    if (Psyz_CardRawSync(mode == 0) == 0) {
        return McxSyncRun;
    }
    PdaPending = 0;
    if (command) {
        *command = PDA_CMD_GET_INFO;
    }
    if (result) {
        *result = PdaResult;
    }
    return McxSyncFin;
}
