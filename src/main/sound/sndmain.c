#include "scratchpad.h"
#include "rr/audio_state_types.h"
#include "rr/race_types.h"
#include "game/sound/sndlis2.h"
#include "game/sound/sndevt.h"
#include "game/sound/sndmain.h"
#include "rr/menu_overlay_runtime.h"
#include "rr/cpu_time.h"
#include "rr/vehicle_table.h"

typedef struct {
    s32 unk00[2];
    s32 pos08[3];
    s32 unk14;
    AudioRot10 rot18;
} ListenerScratch;

/* SoundListenerUpdate's pass on the field between two ticks at field rate
 * (SoundUpdateField): the listener and the cars' distances, pans and
 * Doppler follow the cars every field; the choice of the nearest cars and
 * the start delay stay per tick. */
static int ListenerFieldPass;

void SoundListenerUpdate(void)
{
    ListenerScratch *g = (ListenerScratch *)SCRATCHPAD;
    AudioState *b = &SoundState;
    if (b->active18B8 != 0) {
        /* its cars' search and the opponents' slots by the cars; the replay
         * camera's listener (mode 1) its spatial audio more */
        RR_CPU_CYCLES(RR_CYCLES_SOUND_LISTENER - 2 * RR_CYCLES_SOUND_COM_SLOT - RR_CYCLES_SOUND_COM_SLOTS_END +
                      RR_CYCLES_SOUND_LISTENER_CAR * (unsigned)(NumCars > 0 ? NumCars : 0) +
                      (b->mode18C0 == 1 && b->submode18C2 == 0 ? RR_CYCLES_SOUND_LISTENER_CAMERA : 0));
        switch (b->mode18C0) {
        case 0:
            if (b->submode18C2 == 0) {
                b->unk18E4 = PlayerCars[0].first + PlayerCars[0].second;
                *(AudioRot10 *)&b->pos18C4[0] = *(AudioRot10 *)&PlayerCars[0].position20;
                b->rot18D4 = *(AudioRot10 *)&PlayerCars[0].rot50;
                if (!ListenerFieldPass)
                    SoundFindNearestCars(0);
            } else {
                goto neg;
            }
            break;
        case 1:
            if (b->submode18C2 == 0) {
                b->pos18C4[0] = g->pos08[0] / 4;
                b->pos18C4[1] = g->pos08[1] / 4;
                b->pos18C4[2] = g->pos08[2] / 4;
                b->rot18D4 = ((ListenerScratch *)SCRATCHPAD)->rot18;
                if (!ListenerFieldPass)
                    SoundFindNearestCars(1);
                UpdateCarSpatialAudio(0);
            } else {
                b->pos18C4[0] = g->pos08[0] / 4;
                b->pos18C4[1] = g->pos08[1] / 4;
                b->pos18C4[2] = g->pos08[2] / 4;
                b->rot18D4 = ((ListenerScratch *)SCRATCHPAD)->rot18;
                UpdateCarSpatialAudio(0);
                UpdateCarSpatialAudio(1);
neg:
                b->rec18EC[0].carIndex04 = -1;
                b->rec18EC[1].carIndex04 = -1;
            }
            break;
        }
        SoundUpdateComSlots();
        RR_CPU_CYCLES(RR_CYCLES_SOUND_COM_SLOTS_END);
        if (SoundState.unk18E8 > 0 && !ListenerFieldPass) SoundState.unk18E8--;
    }
}

void SoundListenerUpdateField(void)
{
    ListenerFieldPass = 1;
    SoundListenerUpdate();
    ListenerFieldPass = 0;
}
