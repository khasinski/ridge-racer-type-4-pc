#include "rr/scene_camera_state.h"
#include "game/gfx/model.h"
#include "game/camera/viewcam.h"
#include "scratchpad.h"
#include "rr/camera_motion.h"
#include "rr/cpu_md_leaf.h"
#include "game/camera/cammove.h"
#include "rr/overlay665_render.h"

#define W_ADD(a,b) ((u32)(a)+(b))
#define W_SUB(a,b) ((u32)(a)-(b))
/* Constant products below expand to shifts/adds; variable MD operations use
 * the typed HI/LO interface with an independent native arithmetic reference. */
#define W_MUL(a,b) ((u32)(a)*(b))

void UpdateCameraMotion(MotionClock20 *input) {
    MotionClock20 *arg0=input;
    SceneRaceContext *root=&RaceContext;
    PresentationController *object=MotionControllerView();
    s32 temp_lo;
    s32 temp_lo_2;
    s32 temp_v0_4;
    s32 temp_v0_5;
    s32 temp_v0_6;
    s32 temp_v1;
    s32 temp_v1_2;
    s32 temp_v1_5;
    s32 value, temp_v1_3;
    s32 first, second;
    CameraMotion70 *temp_a3;
    CameraKey16 *temp_v1_4;

    temp_a3 = &object->motion74;
    temp_v1 = temp_a3->mode04;
    switch (temp_v1) {
    case 0:
        temp_v1_2 = object->motion74.phase00;
        switch (temp_v1_2) {
        case 0:
            if (arg0->active == 0) {
copy_start:
                temp_a3->current40[0] = temp_a3->start10[0];
                temp_a3->current40[1] = temp_a3->start10[1];
                temp_a3->current40[2] = temp_a3->start10[2];
                return;
            }
            {
                s32 third;
                value= arg0->tempo;temp_v1_3=object->scale4C;
                value=W_SUB(W_MUL(value,2),300);RR_MD_MULT_S(value,temp_v1_3);
                value = temp_a3->keys0C[temp_a3->key08].x;
                temp_a3->target20[0] = value;
                value = W_SUB(value, temp_a3->start10[0]);
                first = W_MUL(W_ADD(W_MUL(value, 2), value), 8);
                RR_MD_LO_ACCUM_SHL2(temp_lo, first, value);
                RR_MD_DIV_S(first, first, temp_lo);
                value = temp_a3->keys0C[temp_a3->key08].y;
                temp_a3->target20[1] = value;
                value = W_SUB(value, temp_a3->start10[1]);
                second = W_MUL(value, 100);
                RR_MD_DIV_S(second, second, temp_lo);
                value = temp_a3->keys0C[temp_a3->key08].z;
                temp_a3->target20[2]=value;
                temp_v1_3=value;
                value=temp_a3->start10[2];
                temp_v1_3=W_SUB(temp_v1_3,value);
                third=W_MUL(temp_v1_3,100);
                RR_MD_DIV_S(third,third,temp_lo);
                temp_a3->delta30[0]=first;
                temp_a3->delta30[1]=second;
                temp_a3->delta30[2]=third;
            }
            {
                PresentationController *settings=MotionControllerView();
                value=settings->scale4C;temp_v1_3= arg0->tempo;
                value=W_MUL(value,2);RR_MD_MULT_S(temp_v1_3,value);RR_MD_LO(temp_v1_3);
                value=0x51EB851FU;RR_MD_MULT_U(temp_v1_3,value);

                temp_v1_3=(u16)settings->scale4C;
                temp_a3->counter50=0;
                temp_v1_3+=1;

                RR_MD_HI(value);value=(u32)value>>5;
                value=W_SUB(value,temp_v1_3);temp_a3->horizon52=(s16)value;
            }
            object->motion74.phase00 = W_ADD(object->motion74.phase00,1);
        case 1:
            temp_a3->current40[0] = W_ADD(temp_a3->current40[0], temp_a3->delta30[0]);
            temp_a3->current40[1] = W_ADD(temp_a3->current40[1], temp_a3->delta30[1]);
            temp_a3->current40[2] = W_ADD(temp_a3->current40[2], temp_a3->delta30[2]);
            if (temp_a3->delta30[0] > 0) {
                if (temp_a3->current40[0] > temp_a3->target20[0]) {
                    temp_a3->current40[0] = temp_a3->target20[0];
                }
            } else {
                if (temp_a3->current40[0] < temp_a3->target20[0]) {
                    temp_a3->current40[0] = temp_a3->target20[0];
                }
            }
            if (temp_a3->delta30[1] > 0) {
                if (temp_a3->current40[1] > temp_a3->target20[1]) {
                    temp_a3->current40[1] = temp_a3->target20[1];
                }
            } else {
                if (temp_a3->current40[1] < temp_a3->target20[1]) {
                    temp_a3->current40[1] = temp_a3->target20[1];
                }
            }
            if (temp_a3->delta30[2] > 0) {
                if (temp_a3->current40[2] > temp_a3->target20[2]) {
                    temp_a3->current40[2] = temp_a3->target20[2];
                }
            } else {
                if (temp_a3->current40[2] < temp_a3->target20[2]) {
                    temp_a3->current40[2] = temp_a3->target20[2];
                }
            }
            {
                s32 multiplier=0x51EB0000;
                s32 first,second,third;
                s32 highFirst,highSecond,highThird;
                first=temp_a3->current40[0];RR_MD_MULT_S_FINISH16(first,multiplier,0x851F);RR_MD_HI(highFirst);
                second=temp_a3->current40[1];RR_MD_MULT_S(second,multiplier);
                temp_v1_3=temp_a3->counter50;RR_MD_HI(highSecond);
                third=temp_a3->current40[2];temp_v1_3++;
                RR_MD_MULT_S(third,multiplier);
                first>>=31;temp_a3->counter50=(u16)temp_v1_3;
                temp_v1_3 = (temp_v1_3 * 0x10000U);second>>=31;
                value=highFirst>>5;value=W_SUB(value,first);temp_a3->angles64[0]=(s16)value;
                value=highSecond>>5;
                value=W_SUB(value,second);
                third>>=31;
                temp_a3->angles64[1]=(s16)value;
                RR_MD_HI(highThird);
                value=highThird>>5;
                value=W_SUB(value,third);
                temp_a3->angles64[2]=(s16)value;
            }
            if ((temp_v1_3>>16) >= temp_a3->horizon52) {
                temp_v0_4 = W_ADD(temp_a3->key08,1);
                temp_a3->key08 = temp_v0_4;
                if (temp_v0_4 >= 3) {
                    temp_v1_4 = temp_a3->keys0C;
                    temp_v0_5 = ((volatile CameraMotion70 *)temp_a3)->keys0C->x;
                    temp_a3->current40[0] = temp_v0_5;
                    temp_a3->start10[0] = temp_v0_5;
                    temp_v0_6 = temp_v1_4->y;
                    temp_a3->current40[1] = temp_v0_6;
                    temp_a3->start10[1] = temp_v0_6;
                    temp_v1_5 = temp_v1_4->z;
                    temp_a3->key08 = 1;
                    temp_a3->current40[2] = temp_v1_5;
                    temp_a3->start10[2] = temp_v1_5;
                } else {
                    temp_a3->start10[0] = temp_a3->target20[0];
                    temp_a3->start10[1] = temp_a3->target20[1];
                    temp_a3->start10[2] = temp_a3->target20[2];
                }
                temp_a3->counter50 = 0U;
                temp_a3->phase00 = 0;
                return;
            }
            return;
        }
        break;
    case 1:
        switch (object->motion74.phase00) {
        case 0:
            if (arg0->active == 0) {
                goto copy_start;
            }
            {
                s32 third;
                value= arg0->tempo;temp_v1_3=object->scale4C;
                value=W_SUB(W_MUL(value,2),300);RR_MD_MULT_S(value,temp_v1_3);
                value = temp_a3->keys0C[temp_a3->key08].x;
                temp_a3->target20[0] = value;
                value = W_SUB(value, temp_a3->start10[0]);
                first = W_MUL(W_ADD(W_MUL(value, 2), value), 8);
                RR_MD_LO_ACCUM_SHL2(temp_lo_2, first, value);
                RR_MD_DIV_S(first, first, temp_lo_2);
                value = temp_a3->keys0C[temp_a3->key08].y;
                temp_a3->target20[1] = value;
                value = W_SUB(value, temp_a3->start10[1]);
                second = W_MUL(value, 100);
                RR_MD_DIV_S(second, second, temp_lo_2);
                value = temp_a3->keys0C[temp_a3->key08].z;
                temp_a3->target20[2]=value;
                temp_v1_3=value;
                value=temp_a3->start10[2];
                temp_v1_3=W_SUB(temp_v1_3,value);
                third=W_MUL(temp_v1_3,100);
                RR_MD_DIV_S(third,third,temp_lo_2);
                temp_a3->delta30[0]=first;
                temp_a3->delta30[1]=second;
                temp_a3->delta30[2]=third;
            }
            object->motion74.phase00 = W_ADD(object->motion74.phase00,1);
        case 1:
            {
                s32 delta;
                s32 first,second,third;
                s32 highFirst,highSecond,highThird;
                s32 multiplier=0x51EB851F;
                value=temp_a3->current40[0];
                delta=temp_a3->delta30[0];
                value=W_ADD(value,delta);
                temp_a3->current40[0]=value;
                first=value;
                RR_MD_MULT_S(first,multiplier);
                value=temp_a3->current40[1];
                delta=temp_a3->delta30[1];
                value=W_ADD(value,delta);
                RR_MD_HI(highFirst);
                temp_a3->current40[1]=value;
                second=value;
                RR_MD_MULT_S(second,multiplier);
                value=temp_a3->current40[2];
                delta=temp_a3->delta30[2];
                value=W_ADD(value,delta);
                RR_MD_HI(highSecond);
                temp_a3->current40[2]=value;
                third=value;
                RR_MD_MULT_S(third,multiplier);
                first>>=31;
                value=highFirst>>5;
                value=W_SUB(value,first);
                second>>=31;
                temp_a3->angles64[0]=(s16)value;
                value=highSecond>>5;
                value=W_SUB(value,second);
                third>>=31;
                temp_a3->angles64[1]=(s16)value;
                RR_MD_HI(highThird);
                value=highThird>>5;
                value=W_SUB(value,third);
                temp_a3->angles64[2]=(s16)value;
            }
            break;
        }
        break;
    }
}

typedef struct {
    u8 reserved00[0x78];
    s32 w78;
} CamMoveScratch;

void DrawPresentationMotionModel(void)
{
    MatrixWords32 modelMatrix;
    MatrixWords32 cameraMatrix;
    Vec3i scale;
    s32 value;

    SetupObjectLightMatrices((char *)&MotionControllerView()->motion74.modelPosition54,
        (char *)&MotionControllerView()->lighting20, &modelMatrix, &cameraMatrix);
    value = MotionControllerView()->motion74.modelScale6C;
    scale.z = value;
    scale.y = value;
    scale.x = value;
    ScaleMatrix((u32 *)modelMatrix.w, &scale.x);
    SelectModelBank(7);
    SetGteObjectTransform(
        (ObjectTransform38 *)(SCRATCHPAD + 0x7C),
        (s16 *)&MotionControllerView()->motion74.modelPosition54,
        modelMatrix.w);
    ((CamMoveScratch *)SCRATCHPAD)->w78 = 0;
    SubmitModelTwoTri((PreviewRenderScratch *)SCRATCHPAD, ModelBankFormat < 1, 0, 0x100);
}
