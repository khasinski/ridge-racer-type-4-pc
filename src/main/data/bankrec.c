#include "rr/field_rate.h"
#include "types.h"
#include "rr/record_banks.h"
#include "game/data/ldscn1.h"
#include "rr/gpu.h"
#include "game/menu/menu13.h"
#include "game/data/bankrec.h"
#include "rr/release.h"


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

/* The 34 car records the PAL release retuned (docs/REGIONS.md), by R4.BIN
 * entry: gear record 4's speed, the two target frame counts and the top
 * speed, as the PAL disc has them and as the NTSC release (SLPS-01800) has
 * them. The PAL release raised the cars' speed to make up in part for its
 * 25 frames a second; at NTSC timing (60 fields, 30 race frames a second)
 * the NTSC values make these cars drive as in the NTSC original. Read from
 * both discs with tools/port/region_diff.py's R4.BIN reader. */
static const struct {
    s16 fileId;
    s16 pal[4], ntsc[4]; /* gears04[4].speed02, targetFrames1E8[0..1], topSpeed1F0 */
} CarRecordRetunes[] = {
    { 18, {269, 4475, 5750, 175}, {254, 4635, 6060, 166}}, /* PROMESSA */
    { 38, {273, 4475, 5750, 173}, {258, 4635, 6060, 164}}, /* BONFIRE */
    { 58, {267, 4475, 5750, 175}, {252, 4635, 6060, 166}}, /* AMBITIOUS */
    { 78, {261, 4475, 5750, 175}, {246, 4635, 6060, 164}}, /* PROPHETIE */
    { 98, {268, 4600, 6000, 171}, {252, 4800, 6255, 162}}, /* PROMESSA */
    {118, {266, 4600, 6000, 171}, {250, 4800, 6255, 162}}, /* BONFIRE */
    {138, {258, 4600, 6000, 171}, {242, 4800, 6255, 159}}, /* AMBITIOUS */
    {158, {258, 4600, 6000, 171}, {242, 4800, 6255, 159}}, /* PROPHETIE */
    {178, {278, 4425, 5713, 180}, {263, 4515, 5859, 171}}, /* PROMESSA */
    {198, {278, 4425, 5713, 178}, {262, 4515, 5859, 169}}, /* BONFIRE */
    {218, {273, 4425, 5713, 180}, {258, 4515, 5859, 171}}, /* AMBITIOUS */
    {238, {268, 4425, 5713, 180}, {253, 4515, 5859, 169}}, /* PROPHETIE */
    {258, {292, 4225, 5538, 189}, {276, 4320, 5625, 180}}, /* PROMESSA */
    {278, {293, 4225, 5538, 189}, {278, 4320, 5625, 178}}, /* BONFIRE */
    {298, {287, 4225, 5538, 189}, {273, 4320, 5625, 180}}, /* AMBITIOUS */
    {318, {286, 4225, 5538, 189}, {271, 4320, 5625, 178}}, /* PROPHETIE */
    {338, {269, 4475, 5750, 175}, {254, 4635, 6060, 166}}, /* PROMESSA */
    {357, {278, 4425, 5713, 180}, {263, 4515, 5859, 171}}, /* PROMESSA */
    {358, {273, 4475, 5750, 173}, {258, 4635, 6060, 164}}, /* BONFIRE */
    {378, {267, 4475, 5750, 175}, {252, 4635, 6060, 166}}, /* AMBITIOUS */
    {398, {261, 4475, 5750, 175}, {246, 4635, 6060, 164}}, /* PROPHETIE */
    {418, {268, 4600, 6000, 171}, {252, 4800, 6255, 162}}, /* PROMESSA */
    {437, {268, 4425, 5713, 180}, {253, 4515, 5859, 169}}, /* PROPHETIE */
    {438, {266, 4600, 6000, 171}, {250, 4800, 6255, 162}}, /* BONFIRE */
    {458, {258, 4600, 6000, 171}, {242, 4800, 6255, 159}}, /* AMBITIOUS */
    {478, {258, 4600, 6000, 171}, {242, 4800, 6255, 159}}, /* PROPHETIE */
    {498, {278, 4425, 5713, 180}, {263, 4515, 5859, 171}}, /* PROMESSA */
    {518, {278, 4425, 5713, 178}, {262, 4515, 5859, 169}}, /* BONFIRE */
    {538, {273, 4425, 5713, 180}, {258, 4515, 5859, 171}}, /* AMBITIOUS */
    {558, {268, 4425, 5713, 180}, {253, 4515, 5859, 169}}, /* PROPHETIE */
    {578, {292, 4225, 5538, 189}, {276, 4320, 5625, 180}}, /* PROMESSA */
    {598, {293, 4225, 5538, 189}, {278, 4320, 5625, 178}}, /* BONFIRE */
    {618, {287, 4225, 5538, 189}, {273, 4320, 5625, 180}}, /* AMBITIOUS */
    {638, {286, 4225, 5538, 189}, {271, 4320, 5625, 178}}, /* PROPHETIE */
};

static int CarRecordHas(const CarRecord *record, const s16 *v) {
    return record->gears04[4].speed02 == v[0] && record->targetFrames1E8[0] == v[1] &&
           record->targetFrames1E8[1] == v[2] && record->topSpeed1F0 == v[3];
}

s32 CarRecordUseNtscValues(CarRecord *record, s32 fileId) {
    u32 i;

    if (RR_RELEASE_JP || !VideoTimingNtsc || record == NULL)
        return 0;
    for (i = 0; i < sizeof(CarRecordRetunes) / sizeof(CarRecordRetunes[0]); i++) {
        const s16 *ntsc = CarRecordRetunes[i].ntsc;
        if (CarRecordRetunes[i].fileId != fileId)
            continue;
        /* Only the record as the PAL disc has it (a freshly read entry). */
        if (!CarRecordHas(record, CarRecordRetunes[i].pal))
            return CarRecordHas(record, ntsc);
        record->gears04[4].speed02 = ntsc[0];
        record->targetFrames1E8[0] = ntsc[1];
        record->targetFrames1E8[1] = ntsc[2];
        record->topSpeed1F0 = ntsc[3];
        return 1;
    }
    return 0;
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
