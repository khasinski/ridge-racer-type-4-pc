#include "rr/vehicle_rpm.h"
#include "rr/cpu_md_leaf.h"
#include "rr_asm.h"
#include "rr/video_timing.h"
void UpdateEngineRpm(RaceVehicle *vehicle) {
    RaceVehicle *body=(RaceVehicle *)vehicle;
    RaceVehicleControl *control=&body->control;
    s32 a0,a1,a3;
    s32 t0,t2;
    u16 t1;
    s32 v0,v1;
    a0=control->gear68;
    t2=0;
    if(a0==0) {
        t0=t2;
        t1=8500;
        a1=control->shiftThresholds10C[0];
        a3=8500;
    } else {
        t0=control->shiftThresholds10C[a0-1];
        v1=(control->gearCountFC-1U);t1=2500;
        if(a0<v1) {v0=control->shiftThresholds10C[a0];a3=8500;
            a1=v0-t0;}
        else {
            t1=4240;v0=control->maxSpeedF8;a3=10240;
            if(v0<0)v0+=255;
            a1=(v0>>8)-t0;t2=1;
        }
    }
    v1=control->holdFlag48;a1&=65535;
    if(v1==0) {
        v0=body->input;v1=v0-t0;
        if(a1==0)goto minimum;
        v0=t1&65535;
        RR_MD_MULT_S(v0,v1);RR_MD_LO(v1);
        RR_NOP_DEP_ON(v1);RR_NOP_DEP_ON(v1);
        RR_MD_DIV_S(v1,v1,a1);
        v0=a3-v0;a1=v1+v0;
        if(t2!=0) {
            if((u16)control->engine54.halves.accelerating!=0) {
                {
                    s32 high;
                    v1=0x51EB851F;a0=control->engine54.halves.rpm;
                    a1-=a0;v0=(a1*24U);RR_MD_MULT_S(v0,v1);
                    v0>>=31;RR_MD_HI(high);v1=(high>>3)-v0;
                    /* NTSC release: the RPM closes 1/30 of the gap a frame. */
                    if(VideoTimingNtsc)v1=(a1*29)/30;
                }
                a1 = v1 + a0;
                v0 = __builtin_abs(control->steering8C);
                a1 -= v0 << 6;
            }
        }
    } else {
        v0=control->shiftState64;
        if(v0>0) {
            v0=v1<12;
            if(!v0) {
                v1=1;v0=control->engine54.halves.rpm;a1=v0+300;
                v0=a1<10241;control->engine54.halves.accelerating=v1;
                if(v0)goto floor;
                a1=10240;
            } else {
                v0=control->engine54.halves.rpm;a1=v0-500;
                v0=a1<6000;
                if(v0) {
minimum:
                    a1=6000;
                }
            }
        } else {
            a0=control->engine54.halves.rpm;v1=control->shiftDelay66;v0=1;
            control->engine54.halves.accelerating=v0;a1=a0+v1;
        }
    }
    v0=a1<10241;
    if(!v0)a1=10240;
floor:
    if(a1<0)a1=0;
    a0=control->maxSpeedF8;v0=a0;
    if(a0<0)v0=a0+255;
    v1=v0>>8;
    if(v1>=2191)v1=2190;
    if(a1>=4001) {
        a1-=4000;RR_MD_MULT_S(a1,v1);RR_MD_LO(v1);
        v0=(s32)0xEF66A241U;RR_MD_MULT_S(v1,v0);RR_MD_HI(v0);
        v0=(v0+v1);
        v0>>=11;
        v1>>=31;
        a1=v0-v1;
        a1+=4000;
    }
    if(GameModeType==6 && SelectedMakerIndex==3) {
        v1=a0;if(v1<0)v1+=255;
        a3=8500;
        v0=body->input;
        RR_MD_MULT_S(v0,a3);
        RR_MD_LO(a3);
        a1=(u32)v1>>8;v0=a1&65535;RR_MD_DIV_S(a1,a3,v0);
    }
    if(control->latch60==0) {
        v1=control->engine54.halves.rpm;
        v0=v1<a1;
        if(v0){v0=1;control->latch60=v0;return;}
        v0=v1-20;control->engine54.halves.rpm=(u16)v0;return;
    }
    control->engine54.halves.rpm=(u16)a1;
}
