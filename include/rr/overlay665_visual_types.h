#ifndef RR_OVERLAY665_VISUAL_TYPES_H
#define RR_OVERLAY665_VISUAL_TYPES_H
#include "rr/layout.h"
#include "rr/overlay665_matrix.h"
typedef union {s32 words[3];s16 halves[6];} PreviewPosition12;
#include "rr/menu_selection_word.h"
typedef struct {
    PreviewPosition12 position00;
    s32 reserved0C;
    s16 rotation10,rotation12,rotation14;
    u8 pad16[2];
    s16 rotation18;u8 pad1A[2];
    s16 rotation1C;u8 pad1E[2];
    s16 rotation20;u8 tail22[10];
} PreviewView44;
/* Exact160-byte owner cleared by14918. Only observed scalar fields are named;
 * each RGB row and signed direction row retains its fourth padding component. */
typedef struct {
    s32 scene00,reserved04,selection08;
    s32 layout0C,detail10;
    u8 pad14[4];
    s32 kind18;
    u8 pad1C[16];
    s32 reset2C;
    u8 pad30[4];
    MenuSelectionWord32 exit34;
    u16 centerX38,centerY3A;
    s32 scale3C,depth40;
    u16 rotation44;u8 pad46[2];s32 position48;
    PreviewLighting40 lighting4C;
    PreviewView44 view74;
} Overlay665VisualState;
RR_ASSERT_SIZE(Overlay665VisualState,0xA0);
typedef struct {
    s16 x00, y02;
    s32 words04[4];
} Ovl665View14;

typedef struct {
    s32 scene00, reserved04, selection08;
    s32 step0C, detail10, next14, kind18;
    s32 angle1C, phase20, fade24;
    u16 buttons28;
    s16 course2A;
    s32 ready2C;
    u8 pad30[8];
    Ovl665View14 view38;
    u8 pad4C[0x3A];
    u16 rotation86;
    u8 pad88[4];
    s16 offset8C, offset8E, offset90;
    u8 pad92[2];
    s16 offset94, offset96, offset98;
    u8 pad9A[6];
} Ovl665CarState;

/* One owner shared by the menu and controller views. Neither view contains
 * host pointers; the original offsets apply on every supported ABI. */
typedef union {
    Overlay665VisualState visual;
    Ovl665CarState controller;
} Overlay665State;
extern Overlay665State ovl665_options_state;
_Static_assert(sizeof(Overlay665VisualState) == 0xA0, "options visual layout");
_Static_assert(sizeof(Ovl665CarState) == 0xA0, "options controller layout");
_Static_assert(offsetof(Ovl665CarState, view38) == 0x38, "controller camera");
_Static_assert(offsetof(Ovl665CarState, rotation86) == 0x86, "controller rotation");
_Static_assert(offsetof(Overlay665VisualState, view74) == 0x74, "visual camera");
#endif
