#ifndef RR_GAME_DATA_BANKREC_H
#define RR_GAME_DATA_BANKREC_H

#include "rr/car_record_types.h"
#include "rr/track_lighting.h"

extern CarRecord *CarRecordPtrs[];

void SetupDialogueTable(void *buffer);
void SetCarRecordPtr(CarRecord *record, s32 index);
/* At NTSC timing on the PAL disc, the NTSC release's values of the car
 * records the PAL release retuned; fileId is the car's R4.BIN entry. Returns
 * whether the record now has the NTSC values. */
s32 CarRecordUseNtscValues(CarRecord *record, s32 fileId);
void SetLightTable(TrackLighting2C *table);
void SetupRaceMemoryArena(s32 layout,s32 mode);


#endif /* RR_GAME_DATA_BANKREC_H */
