#include "psyq/libgte/rotmat.h"
#include "rr/player_cars.h"
#include "psyq/libgte/gteatan.h"
#include "types.h"
#include "rr/audio_state_types.h"
#include "game/car/car.h"
#include "rr_asm.h"
#include "game/sound/sndown.h"
#include "game/sound/sndcmd.h"
#include "rr/bios_devices.h"
#include "rr/scene_camera_state.h"
#include "rr/overlay662_distance.h"
#include "game/sound/sndevt.h"
#include "rr/trig.h"
#include "rr/field_rate.h"


typedef struct {
    s32 d[3];
    s32 unused0;
    MatrixWords32 m;
    Vec3sFields8 sv;
    s32 unused1[2];
    Vec3sFields8 rot;
    s32 fl;
} SoundGteScratch;

extern s32 TrackLength;
extern RaceVehicle PlayerCars[];

int abs(int);
void SoundListenerInit(int arg0, int arg1);

void SoundListenerInit(int arg0_in, int arg1_in) {
    short arg0 = arg0_in;
    short arg1 = arg1_in;
    int t;
    SoundState.mode18C0 = arg0;
    SoundState.submode18C2 = arg1;
    PsyqBzero(SoundState.rec18EC, sizeof(SoundState.rec18EC));
    t = arg1 & 0xFFFF;
    switch (t) {
    case 0:
        SoundState.rec18EC[0].mode00 = 1;
        SoundState.rec18EC[1].mode00 = 1;
        SoundState.rec18EC[2].mode00 = 0;
        SoundState.rec18EC[2].carIndex04 = 0;
        break;
    case 1:
        SoundState.rec18EC[2].mode00 = 0;
        SoundState.rec18EC[3].mode00 = 0;
        SoundState.rec18EC[2].carIndex04 = 0;
        SoundState.rec18EC[3].carIndex04 = t;
        break;
    }
    SoundState.unk18E8 = 4;
}

void SoundFindNearestCars(s32 arg0) {
    s32 d[4];
    s32 min_dist[2];
    s32 min_idx[2];
    s32 prev_idx[2];
    s32 slot[2];
    s32 i;
    s32 dist;
    RaceVehicle *car;
    const RaceVehicleControl *sub;

    prev_idx[0] = SoundState.rec18EC[0].carIndex04;
    prev_idx[1] = SoundState.rec18EC[1].carIndex04;

    switch (arg0) {
    case 0:
        min_idx[1] = -1;
        min_idx[0] = -1;
        min_dist[1] = TrackLength;
        min_dist[0] = TrackLength;
        for (i = 0; i < RaceContext.comCarCount10; i++) {
            car = &ComCars[i];
            dist = (car->first + car->second) - SoundState.unk18E4;
            dist %= TrackLength;
            if (dist >= TrackLength / 2) {
                dist -= TrackLength;
            }
            if (dist < -TrackLength / 2) {
                dist += TrackLength;
            }
            if (dist < 0) {
                dist = -dist;
            }
            if (dist < min_dist[0]) {
                min_dist[1] = min_dist[0];
                min_idx[1] = min_idx[0];
                min_dist[0] = dist;
                min_idx[0] = i;
            } else if (dist < min_dist[1]) {
                min_dist[1] = dist;
                min_idx[1] = i;
            }
        }
        goto update_slots;
    case 1:
        min_dist[1] = 0x7FFFFFFF;
        min_dist[0] = 0x7FFFFFFF;
        min_idx[1] = -1;
        min_idx[0] = -1;
        for (i = 0; i < RaceContext.comCarCount10; i++) {
            d[0] = (ComCars[i].position20.vector.x - SoundState.pos18C4[0]) / 16;
            d[1] = (ComCars[i].position20.vector.y - SoundState.pos18C4[1]) / 16;
            d[2] = (ComCars[i].position20.vector.z - SoundState.pos18C4[2]) / 16;
            dist = CordicSqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            if (dist < min_dist[0]) {
                min_dist[1] = min_dist[0];
                min_idx[1] = min_idx[0];
                min_dist[0] = dist;
                min_idx[0] = i;
            } else if (dist < min_dist[1]) {
                min_dist[1] = dist;
                min_idx[1] = i;
            }
        }
    update_slots:
        slot[0] = 0;
        slot[1] = 1;
        if (min_idx[0] == SoundState.rec18EC[1].carIndex04) {
            slot[0] = 1;
            slot[1] = 0;
        }
        if (min_idx[1] == SoundState.rec18EC[0].carIndex04) {
            slot[0] = 1;
            slot[1] = 0;
        }
        SoundState.rec18EC[slot[0]].carIndex04 = min_idx[0];
        SoundState.rec18EC[slot[1]].carIndex04 = min_idx[1];
        break;
    }

    for (i = 0; i < 2; i++) {
        SoundState.rec18EC[i].changed06 = 0;
        if (prev_idx[i] != SoundState.rec18EC[i].carIndex04) {
            SoundState.rec18EC[i].changed06 = 1;
        }
    }

    sub = &SoundComCar(min_idx[0])->control;
    if (sub->dynamics94.steering.mode00 != 0) {
        SetPlayerVoice(1, sub->dynamics94.steering.range2C, sub->dynamics94.steering.gain30, 1);
        SoundState.playerVoices195C[1].slot18 = slot[0] + 2;
    } else {
        sub = &SoundComCar(min_idx[1])->control;
        if (sub->dynamics94.steering.mode00 != 0) {
            SetPlayerVoice(1, sub->dynamics94.steering.range2C, sub->dynamics94.steering.gain30, 1);
            SoundState.playerVoices195C[1].slot18 = slot[1] + 2;
        } else {
            SetPlayerVoice(0, 0, 0, 1);
        }
    }
}

/* A listener record with no car holds index -1 and the original still reads
 * that "car" (ComCars[-1] / PlayerCars[-1]). On the PS1 ComCars[-1] is the
 * never-written tail of the dialogue table buffer (zero); PlayerCars[-1]
 * holds libgpu state words. Nothing is applied from such a record (the
 * slot update skips index -1), so read a zeroed vehicle. */
static const RaceVehicle SoundNoCar;

static const RaceVehicle *SoundCar(RaceVehicle *cars, s32 index) {
    return index < 0 ? &SoundNoCar : &cars[index];
}

const RaceVehicle *SoundComCar(s32 index) {
    return SoundCar(ComCars, index);
}

void SoundCalcCarParams(s32 idx) {
    SoundGteScratch f;
    AudioState *B = &SoundState;
    AudioState *B2, *B3;
    const RaceVehicle *e;
    s32 rx, ry, rz;
    s32 mode, a, c, t, ang, prev, t2;
    s32 s2;
    s32 alpha;
    B->rec18EC[idx].prevDist0C = B->rec18EC[idx].dist08;
    mode = B->rec18EC[idx].mode00;
    switch (mode) {
    case 0:
        e = SoundCar(PlayerCars, B->rec18EC[idx].carIndex04);
        f.d[0] = (e->position20.vector.x - B->pos18C4[0]) / 16;
        f.d[1] = (e->position20.vector.y - B->pos18C4[1]) / 16;
        f.d[2] = (e->position20.vector.z - B->pos18C4[2]) / 16;
        break;
    case 1:
        e = SoundComCar(B->rec18EC[idx].carIndex04);
        f.d[0] = (e->position20.vector.x - B->pos18C4[0]) / 16;
        f.d[1] = (e->position20.vector.y - B->pos18C4[1]) / 16;
        f.d[2] = (e->position20.vector.z - B->pos18C4[2]) / 16;
        break;
    }
    t = CordicSqrt(f.d[0] * f.d[0] + f.d[1] * f.d[1] + f.d[2] * f.d[2]);
    B2 = &SoundState;
    prev = B2->rec18EC[idx].prevDist0C;
    B2->rec18EC[idx].dist08 = t;
    /* The Doppler shift is the distance's change over one original tick:
     * at field rate (called every field) over the last two fields
     * (rr/field_rate.h). */
    if (RaceFieldRateLive) {
        static s32 distTwoAgo[4];
        s32 older = distTwoAgo[idx & 3];
        distTwoAgo[idx & 3] = prev;
        prev = older;
    }
    a = prev - t;
    B2->rec18EC[idx].deltaDist10 = a;
    if (a == 0x2350) B2->rec18EC[idx].deltaDist10 = 0x234F;
    rx = B2->rot18D4.r.rx00;
    f.m.w[7] = 0;
    f.m.w[6] = 0;
    f.m.w[5] = 0;
    f.rot.x = rx;
    ry = B2->rot18D4.r.ry04;
    rz = B2->rot18D4.r.rz08;
    f.rot.y = -ry;
    f.rot.z = rz;
    RotMatrix(&f.rot, &f.m);
    RR_GTE_SETROT_TR(f.m.w);
    f.sv.x = f.d[0];
    f.sv.y = f.d[1];
    f.sv.z = f.d[2];
    RR_GTE_MVMVA_ROT_TR_V0(&f.sv);
    RR_GTE_SWC2_MAC123(f.d);
    RR_GTE_STORE_FLAG(&f.fl);
    ang = ratan2(-f.d[2], f.d[0]);
    s2 = rcos(ang) / 64 + 0x40;
    if (s2 >= 0x80) s2 = 0x7F;
    c = rsin(ang) / 64 + 0x40;
    if (c >= 0x80) c = 0x7F;
    t2 = B2->rec18EC[idx].dist08;
    if (t2 > 0x2800) {
        alpha = 0;
    } else {
        alpha = 0x2800 - t2;
        alpha = (alpha * alpha) / 737280;
        if (alpha > 0x80) alpha = 0x80;
    }
    if (RaceContext.cameraNodeChanged1C) alpha = 0;
    B3 = &SoundState;
    t2 = B3->rec18EC[idx].deltaDist10;
    t2 = abs(t2);
    if (t2 > 0x800) alpha = 0;
    if (B3->unk18E8) alpha = 0;
    B3->rec18EC[idx].vol16 = alpha;
    B3->rec18EC[idx].pan14 = s2;
    B3->rec18EC[idx].depth18 = c;
}

void UpdateCarSpatialAudio(int arg0) {
    int t = arg0 + 2;
    AudioState *b;
    AudioState *s;
    SoundCalcCarParams(t);
    b = &SoundState;
    s = b;
    /* The original addresses slot arg0 as SoundState + arg0 * 0x620. */
    s->slots38[arg0].filter30 = b->rec18EC[t].deltaDist10;
    s->slots38[arg0].pan24 = b->rec18EC[t].pan14;
    s->slots38[arg0].levels20.halves.gain = b->rec18EC[t].vol16;
    s->slots38[arg0].spatial2C.word = b->rec18EC[t].depth18;
}

/* Initial values of the globals this module owns. */
RaceVehicle ComCars[11];
