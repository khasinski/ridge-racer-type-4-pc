#include "rr/cpu_time.h"
#include "game/system/pad.h"
#include "rr/player_cars.h"
#include "rr/save_slots.h"
#include "types.h"
#include "rr/load_cursor.h"
#include "scratchpad.h"
#include "rr/course_choice_types.h"
#include "rr/menu_choice.h"
#include "game/car/aipath.h"
#include "game/menu/title.h"
#include "game/gfx/sky.h"
#include "rr/cpu_work.h"

extern int ViewUsesCarTrackNode;

void DrawTrackSceneryForPlayer(int a0);
static void DrawTrackSceneryForPlayerWork(int a0);
void UnpackChoiceGlyph(signed char *a0, int a1);
s32 PackChoiceGlyph(void *arg0);
s32 CountChoiceGlyphs(const CourseChoice4 *a0);

void StepResultsMenuChoice(void) {
    s32 state;
    u32 selected=SelectionRows.choice.bytes[1];
    SelectedMakerIndex=selected;
    UiRows[0].cells[selected]=0;
    if ((MenuSelectedCarSlot & 0x2000) && SelectedMakerIndex < 3) {
        SelectedMakerIndex = SelectedMakerIndex + 1;
        MenuStepDirection = 2;
    } else if ((MenuSelectedCarSlot & 0x8000) && SelectedMakerIndex > 0) {
        SelectedMakerIndex = SelectedMakerIndex - 1;
        MenuStepDirection = 1;
    } else {
        MenuStepDirection = 0;
        goto skip_step;
    }
    /* The sound helper can change selection and backing pointers. */
    PlaySfx(0);
    if(SelectedMakerIndex<0)SelectedMakerIndex=0;
    else if(SelectedMakerIndex>=4)SelectedMakerIndex=3;
    Dialogue->b4=(u8)SelectedMakerIndex+34;
    SelectionRows.choice.bytes[1]=SelectedMakerIndex;
    GlyphRows[SelectedMakerIndex].control02=-6;
    GlyphRows[SelectedMakerIndex].frameIndex00=0;
    UiRows->byte16=(u8)SelectedMakerIndex+4;
skip_step:;
    {
        u8 *ui=UiRows->cells;
        s32 active;

        active=SelectedMakerIndex;
        ui[active]=2;
    }
    if(MenuReturnSubmodeIndex) {
        state=BadgeRows->state10;
        if(state==3) {
            PlaySfx(1);
            {
                MenuSettings2C *settings=&SelectionState.fields;
                BadgeRow1C *control=BadgeRows;
                UiRow24 *ui=UiRows;

                /* Four copies of the same byte-aligned source record. */
                settings->copies[0]=SelectionRows.choice;
                settings->copies[1]=SelectionRows.choice;
                settings->copies[2]=SelectionRows.choice;
                settings->copies[3]=SelectionRows.choice;
                control->state10=4;
                ui->byte14=state;
                {
                    u32 byte=SelectionRows.choice.bytes[1];
                    s32 mode;

                    mode=2;
                    settings->mode1A=mode;
                    settings->copies[0].bytes[1]=byte;
                }
            }
            UpdateUnlockedCarMask();
            ResetScreenCounters();
            {
                s32 zero=0;
                void *resource;
                s32 one;
                s32 last;

                /* Both preceding helpers may replace the resource buffer. */
                resource=MenuArenaBase;

                LoadCursor=resource;
                one=1;

                last=zero;
                SetMenuFrameOptions(zero,one,last);
            }
        }
    }
}

static void DrawTrackSceneryForPlayerWork(int a0) {
    int v;
    if (ViewUsesCarTrackNode == 1) {
        if (a0 < 2) {
            v = PlayerCars[0].trackNodeF0; /* TitleDemoTrackNode 0x800ABFE8 */
        } else {
            v = PlayerCars[1].trackNodeF0; /* MenuSavedTrackNode 0x800AC308 */
        }
    } else {
        int *p = (int *)SCRATCHPAD;
        v = FindTrackNodeAtXZ(p[2], p[4]);
    }
    DrawTrackScenery(v);
}

/* With its PS1 time (rr/cpu_work.h). */
void DrawTrackSceneryForPlayer(int a0) {
    RR_WORK_DRAW(DrawTrackSceneryForPlayerWork(a0), &RrCostDrawTrackScenery);
}

void UnpackChoiceGlyph(signed char *a0, int a1) {
    RR_CPU_CYCLES(RR_CYCLES_UNPACK_CHOICE_GLYPH);
    int t = a1 / 160;
    a0[0] = t;
    a0[1] = a1 / 40 - t * 4;
    a0[2] = a1 % 40;
}

s32 PackChoiceGlyph(void *arg0) {
    return (((u8 *)arg0)[0] * 0xA0) + (((u8 *)arg0)[1] * 0x28) + ((u8 *)arg0)[2];
}

s32 CountChoiceGlyphs(const CourseChoice4 *a0) {
    RR_CPU_CYCLES(RR_CYCLES_COUNT_CHOICE_GLYPHS);
    int i;
    for (i = 0; i < 9; i++) {
        if (a0[i].glyph == -1) break;
        if (i == 8 && a0[8].glyph != 0x167) break;
    }
    return i;
}

/* Initial values of the globals this module owns. */
