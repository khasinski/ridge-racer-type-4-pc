#include "rr/cpu_time.h"
#include "rr/overlay_module.h"
#include "game/gfx/model.h"
#include "scratchpad.h"
#include "psyq/libgte/gtemat4.h"
#include "game/camera/viewcam.h"
#include "rr/overlay665_render.h"
#include "rr/overlay665_visual.h"
#include "rr/overlay665_strokes.h"

#ifndef RR_PREVIEW_RENDER_SCRATCH
#define RR_PREVIEW_RENDER_SCRATCH ((PreviewRenderScratch *)SCRATCHPAD)
#endif

void ovl665_draw_controller_model(void) {
    PreviewRenderScratch *scratch = RR_PREVIEW_RENDER_SCRATCH;
    PreviewMatrixWorkspace sp10;
    PreviewMatrixWorkspace sp30;
    s32 scale[3];
    s32 temp_s6;
    s32 var_a1;
    s32 var_a1_2;
    s32 var_a1_3;
    s32 var_a1_4;
    s32 var_a1_5;
    s32 var_a1_6;
    PreviewLighting40 *temp_s0;
    PreviewLighting40 *temp_s0_2;
    PreviewLighting40 *temp_s0_3;
    PreviewLighting40 *temp_s0_4;
    PreviewView44 *temp_s1;
    PreviewView44 *temp_s1_2;
    PreviewView44 *temp_s4;
    PreviewView44 *temp_s5;
    PreviewView44 *temp_s5_2;

    temp_s6 = scratch->scene.depthShift60;
    scratch->scene.depthShift60 = 3;
    scratch->scene.view.projection.pose08.position00.words[2] = 0;
    scratch->scene.view.projection.pose08.position00.words[1] = 0;
    scratch->scene.view.projection.pose08.position00.words[0] = 0;
    scratch->scene.view.projection.pose08.angles10.xyz.z = 0;
    scratch->scene.view.projection.pose08.angles10.xyz.y = 0;
    scratch->scene.view.projection.pose08.angles10.xyz.x = 0;
    BuildCameraMatrices();
    SetupViewport(6);
    SetGeomOffset((s16)ovl665_options_state.visual.centerX38, (s16)ovl665_options_state.visual.centerY3A);
    ScaleMatrixRow1(2, &scratch->scene.view.matrix28);
    switch (ovl665_options_state.visual.kind18) {
    case 0:
            temp_s1 = &ovl665_options_state.visual.view74;
            temp_s0 = &ovl665_options_state.visual.lighting4C;
            scale[2] = ovl665_options_state.visual.scale3C;
            scale[1] = ovl665_options_state.visual.scale3C;
            scale[0] = ovl665_options_state.visual.scale3C;
            ovl665_options_state.visual.view74.rotation10 = (u16) ovl665_options_state.visual.rotation44;
            ovl665_options_state.visual.view74.position00.words[2] = ovl665_options_state.visual.position48;
            SetupObjectLightMatrices((char *)temp_s1, (char *)temp_s0, &sp10.matrix, &sp30.matrix);
            ScaleMatrix(sp10.words, scale);
            SetGeomScreen(ovl665_options_state.visual.depth40);
            ovl665_setup_preview_light_matrix(&sp10, &temp_s0->matrix);
            SelectModelBank(8);
            SetGteObjectTransform(&scratch->tail7C.projection, temp_s1->position00.halves, (s32 *)sp10.words);
            var_a1 = 1;
            scratch->scene.draw74.draw.counter = 0;
            SubmitModelTwoTri(scratch, var_a1, 0, 0);
            break;
    case 1:
    temp_s1_2 = &ovl665_options_state.visual.view74;
    temp_s0_4 = &ovl665_options_state.visual.lighting4C;
    scale[2] = ovl665_options_state.visual.scale3C;
    scale[1] = ovl665_options_state.visual.scale3C;
    scale[0] = ovl665_options_state.visual.scale3C;
    ovl665_options_state.visual.view74.rotation10 = (u16) ovl665_options_state.visual.rotation44;
    ovl665_options_state.visual.view74.position00.words[2] = ovl665_options_state.visual.position48;
    SetupObjectLightMatrices((char *)temp_s1_2, (char *)temp_s0_4, &sp10.matrix, &sp30.matrix);
    ScaleMatrix(sp10.words, scale);
    SetGeomScreen(ovl665_options_state.visual.depth40);
    ovl665_setup_preview_light_matrix(&sp10, &temp_s0_4->matrix);
    SelectModelBank(8);
    SetGteObjectTransform(&scratch->tail7C.projection, temp_s1_2->position00.halves, (s32 *)sp10.words);
    scratch->scene.draw74.draw.counter = 0;
    var_a1 = ModelBankFormat < 1;
    SubmitModelTwoTri(scratch, var_a1, 0, 0);
    break;

    case 2:
temp_s4 = &ovl665_options_state.visual.view74;
        temp_s0_3 = &ovl665_options_state.visual.lighting4C;
        scale[2] = ovl665_options_state.visual.scale3C;
        scale[1] = ovl665_options_state.visual.scale3C;
        scale[0] = ovl665_options_state.visual.scale3C;
        ovl665_options_state.visual.view74.rotation10 = (u16) ovl665_options_state.visual.rotation44;
        ovl665_options_state.visual.view74.position00.words[2] = ovl665_options_state.visual.position48;
        SetupObjectLightMatrices((char *)temp_s4, (char *)temp_s0_3, &sp10.matrix, &sp30.matrix);
        ScaleMatrix(sp10.words, scale);
        SetGeomScreen(ovl665_options_state.visual.depth40);
        SelectModelBank(8);
        BuildRotMatrixX(&sp30.matrix, ovl665_options_state.visual.view74.rotation18);
        MulMatrix2(&sp10.matrix, &sp30.matrix);
        ovl665_setup_preview_light_matrix(&sp30, &temp_s0_3->matrix);
        SetGteObjectTransform(
            &RR_PREVIEW_RENDER_SCRATCH->tail7C.projection,
            temp_s4->position00.halves,
            (s32 *)sp30.words);
        var_a1_4 = 1;
        scratch->scene.draw74.draw.counter = 0;
        if (ModelBankFormat >= 3) {
            var_a1_4 = 2;
        }
        SubmitModelTwoTri(RR_PREVIEW_RENDER_SCRATCH, var_a1_4, 0, 0);
        if (ovl665_options_state.visual.layout0C >= 2) {
            if (ovl665_options_state.visual.detail10 >= 2) {
                SetGteObjectTransform(
                    &RR_PREVIEW_RENDER_SCRATCH->tail7C.projection,
                    temp_s4->position00.halves,
                    (s32 *)sp30.words);
                var_a1_5 = 1;
                scratch->scene.draw74.draw.counter = 0;
                if (ModelBankFormat >= 7) {
                    var_a1_5 = 6;
                }
                SubmitModelTwoTri(RR_PREVIEW_RENDER_SCRATCH, var_a1_5, 0, 0);
            }
        }
        BuildRotMatrixX(&sp30.matrix, ovl665_options_state.visual.view74.rotation20);
        MulMatrix2(&sp10.matrix, &sp30.matrix);
        ovl665_setup_preview_light_matrix(&sp30, &ovl665_options_state.visual.lighting4C.matrix);
        temp_s5_2 = &ovl665_options_state.visual.view74;
        SetGteObjectTransform(
            &RR_PREVIEW_RENDER_SCRATCH->tail7C.projection,
            temp_s5_2->position00.halves,
            (s32 *)sp30.words);
        var_a1_6 = 1;
        RR_PREVIEW_RENDER_SCRATCH->scene.draw74.draw.counter = 0;
        if (ModelBankFormat >= 4) {
            var_a1_6 = 3;
        }
        SubmitModelTwoTri(RR_PREVIEW_RENDER_SCRATCH, var_a1_6, 0, 0);
        if (ovl665_options_state.visual.layout0C >= 2) {
            if (ovl665_options_state.visual.detail10 >= 2) {
                SetGteObjectTransform(
                    &RR_PREVIEW_RENDER_SCRATCH->tail7C.projection,
                    temp_s5_2->position00.halves,
                    (s32 *)sp30.words);
                var_a1 = 1;
                RR_PREVIEW_RENDER_SCRATCH->scene.draw74.draw.counter = 0;
                if (ModelBankFormat >= 8) {
                    var_a1 = 7;
                }
                SubmitModelTwoTri(RR_PREVIEW_RENDER_SCRATCH, var_a1, 0, 0);
            }
        }
        break;
    case 3:
temp_s5 = &ovl665_options_state.visual.view74;
            temp_s0_2 = &ovl665_options_state.visual.lighting4C;
            scale[2] = ovl665_options_state.visual.scale3C;
            scale[1] = ovl665_options_state.visual.scale3C;
            scale[0] = ovl665_options_state.visual.scale3C;
            ovl665_options_state.visual.view74.rotation10 = (u16) ovl665_options_state.visual.rotation44;
            ovl665_options_state.visual.view74.position00.words[2] = ovl665_options_state.visual.position48;
            SetupObjectLightMatrices((char *)temp_s5, (char *)temp_s0_2, &sp10.matrix, &sp30.matrix);
            ScaleMatrix(sp10.words, scale);
            SetGeomScreen(ovl665_options_state.visual.depth40);
            ovl665_setup_preview_light_matrix(&sp10, &temp_s0_2->matrix);
            SelectModelBank(8);
            SetGteObjectTransform(&scratch->tail7C.projection, temp_s5->position00.halves, (s32 *)sp10.words);
            var_a1_2 = 1;
            scratch->scene.draw74.draw.counter = 0;
            if (ModelBankFormat >= 5) {
                var_a1_2 = 4;
            }
            SubmitModelTwoTri(scratch, var_a1_2, 0, 0);
            BuildRotMatrixZ(&sp30.matrix, ovl665_options_state.visual.view74.rotation1C + 0x400);
            MulMatrix2(&sp10.matrix, &sp30.matrix);
            ovl665_setup_preview_light_matrix(&sp30, &temp_s0_2->matrix);
            SetGteObjectTransform(&scratch->tail7C.projection, temp_s5->position00.halves, (s32 *)sp30.words);
            var_a1_3 = 1;
            scratch->scene.draw74.draw.counter = 0;
            if (ModelBankFormat >= 6) {
                var_a1_3 = 5;
            }
            SubmitModelTwoTri(scratch, var_a1_3, 0, 0);
            if (ovl665_options_state.visual.layout0C >= 2) {
                if (ovl665_options_state.visual.detail10 >= 2) {
                    if (ovl665_options_state.visual.detail10 < 4) {
                        SetGteObjectTransform(
                            &scratch->tail7C.projection,
                            temp_s5->position00.halves,
                            (s32 *)sp30.words);
                        var_a1 = 1;
                        scratch->scene.draw74.draw.counter = 0;
                        if (ModelBankFormat >= 9) {
                            var_a1 = 8;
                        }
                        SubmitModelTwoTri(scratch, var_a1, 0, 0);
                    }
                }
            }
            break;
    default:scratch->scene.depthShift60=temp_s6;return;
    }
    scratch->scene.depthShift60 = temp_s6;
}

void ovl665_draw_twist_gauge_needle(s32 arg0)
{
    u16 pts[4];

    pts[0] = (rsin(arg0) / 128) + ovl665_options_state.visual.centerX38;
    pts[1] = (-rcos(arg0) / 64) + ovl665_options_state.visual.centerY3A;
    pts[2] = ((rsin(arg0) * 40) / 8192) + ovl665_options_state.visual.centerX38;
    pts[3] = ((-rcos(arg0) * 40) / 4096) + ovl665_options_state.visual.centerY3A;
    AddLineF2Prim((const s16 *)pts, ovl665_twist_needle_color, 0, 0);
}

void ovl665_draw_callout_strokes(const StrokeStep4 *const *paths,s32 factor) {
    RR_CPU_CYCLES(RR_CYCLES_CALLOUT_STROKES);
    u16 xy[4];
    StrokeColor4 color=ovl665_callout_stroke_color;
    const StrokeStep4 *step;
    u32 sum;
    s32 limit,n,i;

    if (factor) {
        for (i=0;i<5;i++) {
            xy[0]=ovl665_callout_endpoint_coords[i][0];
            xy[1]=ovl665_callout_endpoint_coords[i][1];
            step=paths[i];
            sum=0;
            while (step->direction!=-1) {
                sum=(sum+step->length);
                step++;
            }
            limit=(s32)(sum*factor)/16;
            step=paths[i];
            sum=0;
            while (step->direction!=-1) {
                if (limit>=(s32)(sum+step->length)) {
                    n=step->length;
                    sum=(sum+step->length);
                } else {
                    n=(limit-sum);
                    sum=(sum+n);
                }
                switch (step->direction) {
                case 0:
                    xy[2]=xy[0];
                    xy[3]=xy[1]-n;
                    AddLineF2Prim((const s16 *)xy,color.bytes,0,0);
                    break;
                case 1:
                    xy[2]=xy[0];
                    xy[3]=xy[1]+(u16)n;
                    AddLineF2Prim((const s16 *)xy,color.bytes,0,0);
                    break;
                case 2:
                    xy[2]=xy[0]-n;
                    xy[3]=xy[1];
                    AddLineF2Prim((const s16 *)xy,color.bytes,0,0);
                    xy[1]++;
                    xy[3]++;
                    AddLineF2Prim((const s16 *)xy,color.bytes,0,0);
                    break;
                case 3:
                    xy[2]=xy[0]+(u16)n;
                    xy[3]=xy[1];
                    AddLineF2Prim((const s16 *)xy,color.bytes,0,0);
                    xy[1]++;
                    xy[3]++;
                    AddLineF2Prim((const s16 *)xy,color.bytes,0,0);
                    break;
                }
                xy[0]=xy[2];
                xy[1]=xy[3];
                step++;
            }
        }
    }
}

/* Initial values of the overlay globals this module owns. */
StrokeColor4 ovl665_callout_stroke_color = {.bytes = {0xFF, 0xFF, 0xFF}};
u16 ovl665_callout_endpoint_coords[5][2] = {{0x58, 0xC0}, {0xA0, 0xC0}, {0xE8, 0xC0}, {0x7C, 0x168}, {0xC4, 0x168}};

/* Writable state of this unit; ovl665_reset_module restores it. */
static const OverlayStateRegion ovl665_controller_draw_regions[] = {
    RR_OVERLAY_STATE(ovl665_callout_endpoint_coords),
    RR_OVERLAY_STATE(ovl665_callout_stroke_color)};
const OverlayStateTable ovl665_controller_draw_state = {ovl665_controller_draw_regions, sizeof(ovl665_controller_draw_regions) / sizeof(ovl665_controller_draw_regions[0])};
