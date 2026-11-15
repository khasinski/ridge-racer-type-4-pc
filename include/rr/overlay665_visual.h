#ifndef RR_OVERLAY665_VISUAL_H
#define RR_OVERLAY665_VISUAL_H
#include "rr/layout.h"
#include "rr/trig.h"
#include "rr/line_emit.h"
#include "rr/projection_state.h"
#include "rr/menu_transition.h"
#include "rr/menu_setup.h"
#include "rr/frame_environment.h"
#include "rr/bios_devices.h"
#include "rr/overlay665_matrix.h"
#include "rr/overlay665_visual_types.h"
extern u8 ovl665_twist_needle_color[4];
extern s32 DialogueSceneMode;
void ovl665_init_scene(void);
/* Signed32 angles are normalized by the actual trig helpers. Each result lies
 * in[-4096,4096], so multiplication by40 and negation are signed32-safe.
 * Output coordinates retain their low16 bits, including wrapped centers. */
void ovl665_draw_twist_gauge_needle(s32 angle);
RR_ASSERT_OFFSET(Overlay665VisualState,centerX38,0x38);
RR_ASSERT_OFFSET(Overlay665VisualState,centerY3A,0x3A);
RR_ASSERT_OFFSET(Overlay665VisualState,lighting4C,0x4C);
RR_ASSERT_OFFSET(Overlay665VisualState,view74,0x74);
RR_ASSERT_SIZE(PreviewView44,0x2C);
RR_ASSERT_OFFSET(PreviewView44,rotation18,0x18);
#endif
