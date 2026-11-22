#include "rr/field_rate.h"
#include "types.h"
#include "rr/record_banks.h"
#include "game/data/ldscn1.h"
#include "rr/gpu.h"
#include "game/menu/menu13.h"
#include "game/data/bankrec.h"


void SetupRaceMemoryArena(s32 layout,s32 mode);

void SetupDialogueTable(void *buffer) {
    u8 *a = buffer;
    int i;
    unsigned short *p;
    u16 **q;
    DialoguePageCounts = a + 0x50;
    i = 0;
    q = DialogueScriptPtrs;
    p = (unsigned short *)a;
    do { *q++ = (u16 *)(a + 0x1B8 + *p++ * 2); i++; } while (i < 0x28);
    DialogueAssetTable = (s8 *)(a + 0x78);
}

void SetCarRecordPtr(CarRecord *record, s32 index) {
    CarRecordPtrs[index] = record;
}

void SetLightTable(TrackLighting2C *table) {
    TrackLightTable = table;
}

void SetupRaceMemoryArena(s32 layout,s32 mode) {
    ReplayStorage *storage = (ReplayStorage *)SharedWorkArena;
    s32 i;

    ReplayFrame=0;
    ReplayState=mode;
    if (mode==2) {
        for (i = 0; i < NumCars; i++) {
            *CarTable[i] = *CarSnapshots[i];
            if ((s16)CarTable[i]->flags1E4 >= 17) {
                CarTable[i]->flags1E4 &= 255;
            }
        }
        CarFieldStatesRestore();
    } else {
        SavedRaceState=0;
        if (layout==1) {
            ReplayLength=2700;
            ReplayInputs[0]=storage->two.inputs[0];
            ReplayInputs[1]=storage->two.inputs[1];
            for (i = 0; i < 4; i++) {
                CarSnapshots[i] = &storage->two.snapshots[i];
            }
        } else if (layout==2) {
            ReplayLength=900;
            for (i = 0; i < 4; i++) {
                ReplayInputs[i] = storage->link.inputs[i];
                CarSnapshots[i] = &storage->link.snapshots[i];
            }
        } else {
            ReplayLength=3600;
            ReplayInputs[0]=storage->single.inputs;
            for (i = 0; i < 8; i++) {
                CarSnapshots[i] = &storage->single.snapshots[i];
            }
        }
    }
}

/* Initial values of the globals this module owns. */
RaceVehicle *CarSnapshots[8];
unsigned char *DialoguePageCounts;
ReplayFrame4 *ReplayInputs[4];
u16 *DialogueScriptPtrs[42];
