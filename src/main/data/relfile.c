#include "rr/relative_file.h"

/* The original relocated the three record offsets, the sixteen section
 * offsets and count*8 segment-view slots in place. The host keeps the image
 * as loaded and publishes resolved addresses in host globals; segment views
 * are resolved on use through RelFileSegmentView. */
void RelocateRelativeFile(RelativeHeader *input) {
    RelativeHeader *base = input;

    TrackRelFileHeaderPtr = base;
    RelFileSegmentCount = base->count;
    TrackRelSecondTablePtr = RrWireAddress(base, base->first[1]);
    RelFileViewRecords = (CourseViewRecord *)RrWireAddress(base, base->first[2]);
    RelFileSection00 = RrWireAddress(base, base->offsets[0]);
    RelFileSection01 = RrWireAddress(base, base->offsets[1]);
    TrackRelSegment2Ptr = RrWireAddress(base, base->offsets[2]);
    RelFileOffsetPtr3 = RrWireAddress(base, base->offsets[3]);
    TrackRelSegment4Ptr = RrWireAddress(base, base->offsets[4]);
    RelFileSection05 = RrWireAddress(base, base->offsets[5]);
    RelFileOffsetPtr6 = RrWireAddress(base, base->offsets[6]);
    RelFileOffsetPtr7 = RrWireAddress(base, base->offsets[7]);
    TrackRelSegment8Ptr = RrWireAddress(base, base->offsets[8]);
    RelFileOffsetPtr9 = RrWireAddress(base, base->offsets[9]);
    TrackCollisionGridPtr = RrWireAddress(base, base->offsets[10]);
    RelFileSection11 = RrWireAddress(base, base->offsets[11]);
    RelFileOffsetPtr12 = RrWireAddress(base, base->offsets[12]);
    RelFileOffsetPtr13 = RrWireAddress(base, base->offsets[13]);
    TrackSectionBoundsPtr = RrWireAddress(base, base->offsets[14]);
    RelFileSection15 = RrWireAddress(base, base->offsets[15]);
    RelFileSegmentViewSlots = (u32 *)(base + 1);
}

/* Initial values of the globals this module owns. */
u8 *RelFileOffsetPtr7;
u8 *TrackCollisionGridPtr;
u8 *RelFileSection05;
u8 *TrackSectionBoundsPtr;
u8 *RelFileOffsetPtr12;
u8 *RelFileSection15;
u8 *RelFileSection00;
RelativeHeader *TrackRelFileHeaderPtr;
u32 *RelFileSegmentViewSlots;
u8 *TrackRelSecondTablePtr;
u8 *RelFileOffsetPtr6;
u8 *RelFileOffsetPtr9;
u8 *TrackRelSegment4Ptr;
u8 *RelFileOffsetPtr13;
u8 *RelFileSection11;
u8 *RelFileSection01;
u8 *TrackRelSegment8Ptr;
