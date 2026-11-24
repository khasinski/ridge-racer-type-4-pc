#include "rr/relative_file.h"
#include "types.h"
#include "rr/steering.h"
#include "rr/track_resources.h"
#include "game/course/trkres2.h"
#include "game/gfx/dispenv.h"


void BuildRateCurve(Curve24 *curve, s32 displacement, s32 duration, s32 initialRate);
s32 EvalRateCurve(const Curve24 *curve, s32 time, s32 *output);

/* Build a two-phase rate curve in twelve-bit fixed-point units. */
void BuildRateCurve(Curve24 *curve, s32 displacement, s32 duration, s32 initialRate)
{
    s32 half = duration / 2;
    s32 scaledDisplacement = displacement * 4096;
    s32 middleRate;
    s32 scaledInitialRate = initialRate * 4096;
    curve->initialRate = scaledInitialRate;
    middleRate = scaledDisplacement / half - (initialRate * 2048);
    curve->displacement = scaledDisplacement;
    curve->duration = duration;
    curve->middleRate = middleRate;
    curve->firstAcceleration = (middleRate - curve->initialRate) / half;
    curve->secondAcceleration = -middleRate / half;
}

s32 EvalRateCurve(const Curve24 *curve, s32 time, s32 *output)
{
    s32 half;
    s32 rate;
    if (time >= curve->duration) {
        *output = -(curve->secondAcceleration / 2);
        return -1;
    }
    half = curve->duration / 2;
    if (time < half) {
        rate = curve->initialRate + curve->firstAcceleration / 2 + curve->firstAcceleration * time;
    } else {
        time -= half;
        rate = curve->middleRate + curve->secondAcceleration / 2 + curve->secondAcceleration * time;
    }
    *output = rate;
    return 0;
}

size_t AlignUp4(size_t a) {
    size_t v = a;
    size_t r = v & 3;
    if (r != 0) {
        v += 4 - r;
    }
    return v;
}

/* Host owner of the resolved track-event directory. */
static TrackEventDirectory TrackEventDirectoryHost;

void RelocateTrackLookupTables(const TrackEventDirectoryWire *wire)
{
    TrackEventDirectory *tables = &TrackEventDirectoryHost;
    tables->pairCount0 = wire->pairCount0;
    tables->pairCount1 = wire->pairCount1;
    tables->count0 = wire->count0;
    tables->count1 = wire->count1;
    tables->count2 = wire->count2;
    tables->count3 = wire->count3;
    tables->pairCount2 = wire->pairCount2;
    tables->reserved0E = wire->reserved0E;
    tables->pairs0.entries = (TrackEventPair *)RrWireAddress(wire, wire->pairs0);
    tables->pairs1.entries = (TrackEventPair *)RrWireAddress(wire, wire->pairs1);
    tables->table0.entries = (TrackEventEntry *)RrWireAddress(wire, wire->table0);
    tables->table1.entries = (TrackEventEntry *)RrWireAddress(wire, wire->table1);
    tables->table2.entries = (TrackEventEntry *)RrWireAddress(wire, wire->table2);
    tables->table3.entries = (TrackEventEntry *)RrWireAddress(wire, wire->table3);
    tables->pairs2.entries = (TrackEventPair *)RrWireAddress(wire, wire->pairs2);
    RaceContext.lookupTables = tables;
}

void SetTrackLaneOffsets(TrackLaneOffset8 *offsets)
{
    RaceContext.laneOffsets54C = offsets;
}

void SetTrackStartNode(Grid53C *startGrid)
{
    RaceContext.startGrid53C = startGrid;
}

void SetTrackWorkspace(u8 *workspace)
{
    SceneRaceContext *context = &RaceContext;
    context->workspace554 = workspace;
    workspace += 0xA00;
    context->lightPresets558 = (TrackLightPreset8 *)workspace;
    workspace += 0x20;
    context->workspace55C = workspace;
    workspace += 0x200;
    context->cameraNodes550 = (CamNode44 *)workspace;
}

void LoadTrackResource(TrackResourceFile *file)
{
    RelocateTrackLookupTables((const TrackEventDirectoryWire *)((u8 *)file + file->lookupOffset04));
    SetTrackLaneOffsets((TrackLaneOffset8 *)((u8 *)file + file->laneOffset08));
    SetTrackStartNode((Grid53C *)((u8 *)file + file->startNodeOffset0C));
    SetTrackWorkspace((u8 *)file + file->workspaceOffset10);
}

void SetCarSpecTable(RaceSpecEntry30 *arg0) {
    RaceContext.carSpecs540 = arg0;
}
