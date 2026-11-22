#include "types.h"
#include "rr_asm.h"
#include "rr/track_sample_types.h"
#include "game/course/trkclip.h"

typedef struct {
    u16 h0, h2, h4, h6, h8, ha, hc, he, h10, h12, pad14, pad16;
    s32 r18;
    s32 r1c;
} StackFrame;

void GetTrackSegmentAndYaw(int node, int yaw, short *output)
{
    int segment = node / 5;
    int relativeYaw = 0x800 - TrackNodes[node].road.heading;
    relativeYaw = ((u32)yaw - relativeYaw) & 0xFFF;
    output[1] = relativeYaw;
    output[0] = segment;
}

s32 FindTrackNode(s32 *pos, s32 startIdx, s32 maxIter) {
    StackFrame sp;
    s32 idx;
    s32 cnt;
    s32 px, pz;
    s32 next;
    s32 w78, w80;
    TrackSample3C *p, *q;
    s32 r1;
    s32 r2;
    s32 r3;

    cnt = 0;
    idx = startIdx;
    px = pos[0];
    pz = pos[2];
    for (;;) {
        next = (idx + 1) % TrackNodeCount;
        p = &TrackNodes[idx];
        q = &TrackNodes[next];
        w78 = p->road.x;
        w80 = p->road.z;
        sp.h4 = sp.h8 = (u16)q->road.x - (u16)w78;
        sp.h6 = sp.ha = (u16)q->road.z - (u16)w80;
        sp.h0 = (u16)px - (u16)w78;
        sp.h2 = (u16)pz - (u16)w80;
        sp.h4 += (u16)q->rightOffsetX00;
        sp.h6 += (u16)q->rightOffsetZ02;
        sp.h8 += (u16)q->leftOffsetX04;
        sp.ha += (u16)q->leftOffsetZ06;
        sp.hc = (u16)p->rightOffsetX00;
        sp.he = (u16)p->rightOffsetZ02;
        sp.h10 = (u16)p->leftOffsetX04;
        sp.h12 = (u16)p->leftOffsetZ06;
        r3 = *(s32 *)&sp.h0;
        r1 = *(s32 *)&sp.hc;
        r2 = *(s32 *)&sp.h10;
        RR_GTE_NCLIP3(r1, r2, r3, &sp.r18);
        r1 = *(s32 *)&sp.h8;
        r2 = *(s32 *)&sp.h4;
        r3 = *(s32 *)&sp.h0;
        RR_GTE_NCLIP3(r1, r2, r3, &sp.r1c);
        if (sp.r18 >= 0 && sp.r1c >= 0) {
            break;
        }
        cnt++;
        if (!(cnt & 1)) {
            idx -= cnt;
        } else {
            idx += cnt;
        }
        idx = (TrackNodeCount + idx) % TrackNodeCount;
        if (cnt > maxIter) {
            idx = startIdx;
            break;
        }
    }
    return idx;
}

