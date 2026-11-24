#include "types.h"
#include "scratchpad.h"
#include "game/car/car.h"
#include "game/car/carsp.h"
#include "rr_asm.h"
#include "game/car/carstate.h"
#include "game/car/carwheel.h"
#include "game/car/carmove1.h"
#include "rr/menu_overlay_runtime.h"
#include "rr/overlay668_camera.h"


extern short TrackLengthKilometers;
extern short TrackLengthSectorCount;

void FindAxleTrackNode(void)
{
    CarTrackWork *g = (CarTrackWork *)SCRATCH_CAR_WORK;
    s32 a, n, t, x;
    TrackSample3C *p, *q;
    s32 r1;
    s32 r2;
    s32 r3;
    g->w38 = 0;
    for (;;) {
        g->w34 = (g->wheelState150.trackNodes[g->wheelIndex08] + 1) % TrackNodeCount;
        a = g->w34;
        p = &TrackNodes[g->wheelState150.trackNodes[g->wheelIndex08]];
        g->originX78 = p->road.x;
        g->originZ80 = p->road.z;
        q = &TrackNodes[a];
        g->nextLeft74.halves.x = g->nextRight70.halves.x = (u16)q->road.x - (u16)g->originX78;
        g->nextLeft74.halves.z = g->nextRight70.halves.z = (u16)q->road.z - (u16)g->originZ80;
        g->queryPoint64.halves.x = (g->positions110[g->wheelIndex08].x + g->positions110[g->wheelIndex08 + 1].x) / 2 - (u16)g->originX78;
        g->queryPoint64.halves.z = (g->positions110[g->wheelIndex08].z + g->positions110[g->wheelIndex08 + 1].z) / 2 - (u16)g->originZ80;
        g->nextRight70.halves.x += (u16)q->rightOffsetX00;
        g->nextRight70.halves.z += (u16)q->rightOffsetZ02;
        g->nextLeft74.halves.x += (u16)q->leftOffsetX04;
        g->nextLeft74.halves.z += (u16)q->leftOffsetZ06;
        g->rightEdge68.halves.x = (u16)p->rightOffsetX00;
        g->rightEdge68.halves.z = (u16)p->rightOffsetZ02;
        g->leftEdge6C.halves.x = (u16)p->leftOffsetX04;
        g->leftEdge6C.halves.z = (u16)p->leftOffsetZ06;
        r3 = g->queryPoint64.word; r1 = g->rightEdge68.word; r2 = g->leftEdge6C.word;
        RR_GTE_NCLIP3(r1, r2, r3, &g->w3C);
        r1 = g->nextLeft74.word; r2 = g->nextRight70.word; r3 = g->queryPoint64.word;
        RR_GTE_NCLIP3(r1, r2, r3, &g->w40);
        if (g->w3C >= 0 && g->w40 >= 0)
            return;
        n = g->w38 + 1;
        g->w38 = n;
        x = g->wheelState150.trackNodes[g->wheelIndex08] + TrackNodeCount;
        g->wheelState150.trackNodes[g->wheelIndex08] = ((n & 1) ? x + n : x - n) % TrackNodeCount;
    }
}

void FindWheelTrackNodes(RaceVehicle *arg0) {
    CarTrackWork *g = (CarTrackWork *)SCRATCH_CAR_WORK;
    s32 t0;
    s32 *q;

    arg0->contactFlags1F0.word = 0;
    arg0->position20.vector.x = arg0->worldPosition10.words[0] >> 2;
    arg0->position20.vector.z = arg0->worldPosition10.words[2] >> 2;
    g->positions110[0] = arg0->wheelPositions88[0];
    g->positions110[1] = arg0->wheelPositions88[1];
    g->positions110[2] = arg0->wheelPositions88[2];
    g->positions110[3] = arg0->wheelPositions88[3];
    g->sC0 = 0;
    g->sBA = 0;
    g->sC8 = 0;
    g->sC6 = 0;
    g->sC4 = 0;
    g->sE6 = 0;
    g->sE4 = 0;
    g->sE2 = 0;
    g->sDE = 0;
    g->sDC = 0;
    g->sDA = 0;
    g->translation100.vector = arg0->position20.vector;
    q = (s32 *)(SCRATCHPAD + 0x7C);
    {
        s32 a0 = arg0->trackNodeF0;
        s32 v0 = a0 - 2;
        s32 v1 = (v0 + TrackNodeCount) % TrackNodeCount;
        g->w14 = a0;
        g->w30 = arg0->width10C;
        g->wheelIndex08 = 0;
        g->w20 = v1;
    }
    do {
        g->positions110[g->wheelIndex08].x = g->translation100.vector.x + (g->positions110[g->wheelIndex08].x >> 2);
        g->positions110[g->wheelIndex08].z = g->translation100.vector.z + (g->positions110[g->wheelIndex08].z >> 2);
        g->positions110[g->wheelIndex08 + 1].x = g->translation100.vector.x + (g->positions110[g->wheelIndex08 + 1].x >> 2);
        g->positions110[g->wheelIndex08 + 1].z = g->translation100.vector.z + (g->positions110[g->wheelIndex08 + 1].z >> 2);
        g->wheelState150.trackNodes[g->wheelIndex08] = arg0->wheelNodesF8[g->wheelIndex08];
        FindAxleTrackNode();
        g->road180[g->wheelIndex08] = TrackNodes[g->wheelState150.trackNodes[g->wheelIndex08]].road;
        g->road180[g->wheelIndex08 + 1] = TrackNodes[(g->wheelState150.trackNodes[g->wheelIndex08] + 1) % TrackNodeCount].road;
        arg0->wheelNodesF8[g->wheelIndex08 + 1] = arg0->wheelNodesF8[g->wheelIndex08] = g->wheelState150.trackNodes[g->wheelIndex08];
        g->wheelIndex08 += 2;
    } while (g->wheelIndex08 < 3);
    g->wheelIndex08 = 0;
    do {
        s32 a0 = g->wheelIndex08;
        g->wC = a0;
        if (g->wheelState150.trackNodes[0] != g->wheelState150.trackNodes[2] || g->wheelIndex08 < 2) {
            g->s174 = g->road180[a0].rightAngle;
            g->s176 = g->road180[g->wC].leftAngle;
            g->s24 = rsin(g->s174);
            g->s26 = rcos(g->s174);
            g->s28 = rsin(g->s176);
            g->s2A = rcos(g->s176);
            if (g->s174 != g->s176) {
                g->w38 = 1;
                g->s174 = g->s162 = g->road180[g->wC].segmentLength;
                g->s160 = g->road180[g->wC].heading + 0x400;
            } else {
                g->w38 = 0;
                g->s174 = g->s162 = (g->road180[g->wC].segmentLength * g->s26) >> 12;
                g->s160 = g->road180[g->wC].angle + 0x400;
                g->s176 = (g->s24 * g->road180[g->wC].segmentLength) >> 12;
            }
            g->sB8 = g->sC2 = rcos(g->s160);
            {
                s16 s = rsin(g->s160);
                g->sBE = -s;
                g->sBC = s;
            }
            RR_GTE_SETROT(&g->sB8);
            g->s16C = g->road180[g->wC + 1].height - g->road180[g->wC].height;
            g->s168 = g->road180[g->wC + 1].pitch - g->road180[g->wC].pitch;
            g->s164 = g->road180[g->wC + 1].rightWidth - g->road180[g->wC].rightWidth;
            g->s166 = g->road180[g->wC + 1].leftWidth - g->road180[g->wC].leftWidth;
        }
        for (t0 = 0; t0 < 2; t0++) {
            g->sF8 = g->positions110[g->wheelIndex08 + t0].x - g->road180[g->wC].x;
            g->sFC = g->positions110[g->wheelIndex08 + t0].z - g->road180[g->wC].z;
            RR_GTE_MVMVA_ROT_V0_NOCV(&g->sF8);
            g->w34 = 0;
            g->s16E = 0;
            g->w3C = g->s174;
            g->w40 = g->s176;
            RR_GTE_STORE_IR(&g->w14);;            if (g->w38 == 1) {
                g->w40 = (g->w18 * g->s24) / g->s26;
                g->w14 -= g->w40;
                g->w3C -= g->w40;
                g->w3C += (g->w18 * g->s28) / g->s2A;
                g->w14 = (g->w14 * g->s162) / g->w3C;
            } else {
                g->w18 -= (g->w40 * g->w14) / g->s162;
            }
            g->w10 = (g->w14 << 10) / g->s162;
            g->s158 = ((g->s168 * g->w10) >> 10) + g->road180[g->wC].pitch;
            g->s15A = ((g->s164 * g->w10) >> 10) + g->road180[g->wC].rightWidth;
            g->s15C = ((g->s166 * g->w10) >> 10) + g->road180[g->wC].leftWidth;
            if (g->w18 > g->s15A) {
                g->s16A = g->w18 - g->s15A;
                g->w34 = 1;
                g->w18 = g->s15A;
            } else if (g->w18 < -g->s15C) {
                g->s16A = -(g->w18 + g->s15C);
                g->w34 = 2;
                g->w18 = -g->s15C;
            }
            if (g->w34 > 0) {
                g->w34--;
                g->s164 = g->road180[g->wC + 1].unknown34[g->w34] - g->road180[g->wC].unknown34[g->w34];
                g->s164 = ((g->s164 * g->w10) >> 10) + g->road180[g->wC].unknown34[g->w34];
                g->s170 = g->road180[g->wC + 1].unknown38[g->w34] - g->road180[g->wC].unknown38[g->w34];
                g->s170 = ((g->s170 * g->w10) >> 10) + g->road180[g->wC].unknown38[g->w34];
                if (g->s16A < g->s164) {
                    g->s172 = g->s16A;
                    g->s16A = 0;
                } else {
                    g->s16A -= (g->s172 = g->s164);
                    g->s16E = g->road180[g->wC + 1].unknown30[g->w34] - g->road180[g->wC].unknown30[g->w34];
                    g->s16E = ((g->s16E * g->w10) >> 10) + g->road180[g->wC].unknown30[g->w34];
                    {
                        s32 v = (g->w34 != 0) ? -(((g->s16E * g->s16A) << 11) >> 16) : (((g->s16E * g->s16A) << 11) >> 16);
                        g->s16E = v;
                    }
                    arg0->contactFlags1F0.bytes[g->wheelIndex08 + t0] = g->road180[g->wC].sides[g->w34].contactFlags;
                }
                {
                    s32 v = (g->w34 != 0) ? -(((g->s170 * g->s172) << 11) >> 16) : (((g->s170 * g->s172) << 11) >> 16);
                    g->s16E += (g->s170 = v);
                }
            }
            g->wheelHeights250[*q][g->wheelIndex08 + t0] = g->s16E + ((((g->s16C * g->w10) >> 10) + g->road180[g->wC].height) + ((g->s158 * g->w18) >> 5));
        }
        g->wheelIndex08 += 2;
    } while (g->wheelIndex08 < 4);
}

void UpdateWheelBumps(RaceVehicle *vehicle)
{
    int *q7c = (int *)(SCRATCHPAD + 0x7C);
    CarTrackWork *work = (CarTrackWork *)SCRATCH_CAR_WORK;
    int t, t2, t3, v;
    work->wheelIndex08 = 0;
    do {
        t = work->wheelIndex08;
        work->positions110[t].y = work->wheelHeights250[*q7c][t];
        if (vehicle->prevWheelNodes100[work->wheelIndex08] != vehicle->wheelNodesF8[work->wheelIndex08]) {
            if (vehicle->wheelBumpState1BC[work->wheelIndex08] == 0) {
                if (TriggerWheelBump(vehicle, work->wheelIndex08) != 0)
                    vehicle->wheelBumpBase1CC[work->wheelIndex08] = work->positions110[work->wheelIndex08].y;
            }
        }
        t2 = work->wheelIndex08;
        {
            int c = vehicle->wheelBumpState1BC[t2];
            if (c > 0) {
                int lo = vehicle->wheelBumpVel1C4[t2] * c;
                vehicle->wheelPositions88[t2].y = vehicle->wheelBumpBase1CC[t2] + lo + ((c * c << 11) / 900);
                t3 = work->wheelIndex08;
                v = work->positions110[t3].y;
                if (vehicle->wheelPositions88[t3].y >= v && vehicle->wheelBumpState1BC[t3] >= 2) {
                    vehicle->wheelPositions88[t3].y = v;
                    if (work->wheelIndex08 == 0) {
                        vehicle->steerInput1BA = vehicle->wheelBumpState1BC[0];
                        SetCarImpactState(1, vehicle);
                    }
                    vehicle->wheelBumpState1BC[work->wheelIndex08] = 0;
                    vehicle->steeringLock1B8 ^= 1 << work->wheelIndex08;
                } else {
                    vehicle->wheelBumpState1BC[work->wheelIndex08]++;
                }
                work->wheelState150.heightOffsets[work->wheelIndex08] = vehicle->wheelPositions88[work->wheelIndex08].y - work->positions110[work->wheelIndex08].y;
            } else {
                vehicle->wheelPositions88[t2].y = work->positions110[t2].y;
                work->wheelState150.heightOffsets[work->wheelIndex08] = 0;
            }
        }
        vehicle->prevWheelNodes100[work->wheelIndex08] = vehicle->wheelNodesF8[work->wheelIndex08];
    } while (++work->wheelIndex08 < 4);
    vehicle->wheelRotX1A8 += Atan2Table(vehicle->halfSpan10E, (work->wheelState150.heightOffsets[0] + work->wheelState150.heightOffsets[1] - (work->wheelState150.heightOffsets[2] + work->wheelState150.heightOffsets[3])) >> 3);
    vehicle->wheelRotZ1AA += Atan2Table(vehicle->width10C, (work->wheelState150.heightOffsets[0] + work->wheelState150.heightOffsets[2] - (work->wheelState150.heightOffsets[1] + work->wheelState150.heightOffsets[3])) >> 3);
    {
        int m = (vehicle->wheelPositions88[2].y + vehicle->wheelPositions88[3].y) >> 1;
        vehicle->worldPosition10.words[1] = m;
        vehicle->worldPosition10.words[1] = ((((vehicle->wheelPositions88[0].y + vehicle->wheelPositions88[1].y) - (vehicle->wheelPositions88[2].y + vehicle->wheelPositions88[3].y)) >> 1) * 0x14) / vehicle->halfSpan10E + m;
    }
    {
        int m = (work->positions110[2].y + work->positions110[3].y) >> 1;
        int v, d;
        vehicle->bodyPosition.y = m;
        d = ((((work->positions110[0].y + work->positions110[1].y) - (work->positions110[2].y + work->positions110[3].y)) >> 1) * 0x14) / vehicle->halfSpan10E;
        v = vehicle->sampleCount16C & 0xF;
        vehicle->bodyPosition.y = d + m;
        vehicle->sampleCount16C = v;
        work->w34 = v;
        work->w38 = (v + 6) & 0xF;
        work->w3C = (work->w34 + 0xC) & 0xF;
        work->w40 = (work->w34 + 2) & 0xF;
    }
    vehicle->xHistory124[work->w34] = Atan2Table(vehicle->halfSpan10E, (vehicle->wheelPositions88[0].y + vehicle->wheelPositions88[1].y - (vehicle->wheelPositions88[2].y + vehicle->wheelPositions88[3].y)) >> 3);
    vehicle->zHistory144[work->w34] = Atan2Table(vehicle->width10C, (vehicle->wheelPositions88[0].y + vehicle->wheelPositions88[2].y - (vehicle->wheelPositions88[1].y + vehicle->wheelPositions88[3].y)) >> 3);
    vehicle->pitchAverage164 += vehicle->xHistory124[work->w34];
    vehicle->rollAverage166 += vehicle->zHistory144[work->w34];
    vehicle->pitchAverage164 = (u16)vehicle->pitchAverage164 - vehicle->xHistory124[work->w38];
    vehicle->rollAverage166 = (u16)vehicle->rollAverage166 - vehicle->zHistory144[work->w38];
    vehicle->rot50 = (short)(vehicle->pitchAverage164 / 10);
    vehicle->rot58 = (short)(vehicle->rollAverage166 / 10);
    vehicle->pitchAverage168 += vehicle->xHistory124[work->w3C];
    vehicle->rollAverage16A += vehicle->zHistory144[work->w3C];
    vehicle->pitchAverage168 -= vehicle->xHistory124[work->w40];
    vehicle->rollAverage16A -= vehicle->zHistory144[work->w40];
    vehicle->ang80 = vehicle->pitchAverage168 / 10 + (vehicle->renderAngleX1AC >> 1);
    vehicle->ang82 = (u16)vehicle->rot54;
    vehicle->ang84 = vehicle->renderAngleZ1AE + vehicle->rollAverage16A / 10;
    vehicle->ir70[0] += vehicle->worldPosition10.words[0];
    vehicle->ir70[1] += vehicle->worldPosition10.words[1];
    vehicle->ir70[2] += vehicle->worldPosition10.words[2];
    vehicle->worldPosition10.words[0] += vehicle->ir40[0];
    vehicle->worldPosition10.words[1] += vehicle->ir40[1];
    vehicle->worldPosition10.words[2] += vehicle->ir40[2];
    vehicle->ir70[0] += vehicle->ir40[0];
    vehicle->ir70[1] += vehicle->ir40[1];
    {
        int w78 = vehicle->ir70[2];
        int w48 = vehicle->ir40[2];
        int w44 = vehicle->ir40[1];
        int w10 = vehicle->worldPosition10.words[0];
        int w18 = vehicle->worldPosition10.words[2];
        int wcc;
        int v2;
        wcc = vehicle->bodyPosition.y;
        w78 += w48;
        wcc += w44;
        vehicle->ir70[2] = w78;
        vehicle->bodyPosition.y = wcc;
        v2 = w10;
        v2 >>= 2;
        vehicle->position20.words[0] = v2;
        vehicle->sampleCount16C += 1;
        v2 = w18;
        v2 >>= 2;
        vehicle->bodyPosition.x = w10;
        vehicle->bodyPosition.z = w18;
        vehicle->position20.words[2] = v2;
        vehicle->position20.words[1] = vehicle->worldPosition10.words[1] >> 2;
    }
}

/* Track node file: header word, 32x32 node grid at +4, node count at +0x804,
 * TrackSample3C nodes at +0x808. Read in place; nothing is rewritten. */
void SetTrackNodeData(void *data)
{
    u8 *a = (u8 *)data;
    int i;
    TrackNodeGrid = (short *)(a + 4);
    TrackNodes = (TrackSample3C *)(a + 0x808);
    TrackLength = 0;
    TrackNodeCount = ((s32 *)a)[0x201];
    for (i = 0; i < TrackNodeCount; i++) {
        TrackLength += TrackNodes[i].road.segmentLength;
    }
    TrackLengthKilometers = TrackLength / 1024;
    TrackLengthSectorCount = (TrackLength >> 8) + 1;
}


/* Initial values of the globals this module owns. */
s32 TrackNodeCount;
short TrackLengthKilometers;
short TrackLengthSectorCount;
