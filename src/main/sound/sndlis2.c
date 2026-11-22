#include "types.h"
#include "rr/player_cars.h"
#include "rr/audio_state_types.h"
#include "rr/race_types.h"
#include "game/sound/sndevt.h"
#include "game/sound/sndlis2.h"
#include "rr/audio_bank_setup.h"
#include "game/sound/sndslot.h"


void SoundUpdateComSlots(void) {
    int i;
    for (i = 0; i < 2; i++) {
        AudioListenerRecord1C *c = &SoundState.rec18EC[i];
        const RaceVehicle *r = SoundComCar(c->carIndex04);
        SoundCalcCarParams(i);
        if (c->carIndex04 != -1) {
            SoundState.slots38[i + 2].filter30 = c->deltaDist10;
            SoundState.slots38[i + 2].pan24 = c->pan14;
            SoundState.slots38[i + 2].levels20.halves.gain = c->vol16;
            SoundState.slots38[i + 2].spatial2C.word = c->depth18;
            SlotSetLevelProgram(r->control.engine54.halves.rpm, (u16)r->control.engine54.halves.accelerating, i + 2);
        }
    }
}
