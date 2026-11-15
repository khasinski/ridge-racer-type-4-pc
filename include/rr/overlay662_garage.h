#ifndef RR_OVERLAY662_GARAGE_H
#define RR_OVERLAY662_GARAGE_H
#include "rr/layout.h"
#include "rr/emblem_tim_types.h"
typedef struct GarageOptionRecord GarageOptionRecord;
/* One pointer owner has flat initialization and3/5 navigation group views. */
typedef struct {GarageOptionRecord *first[3],*second[5];} GarageOptionPointerGroups;
typedef union {GarageOptionRecord *flat[8];GarageOptionPointerGroups groups;} GarageOptionPointers;
typedef struct {s32 first[3],second[5];} GarageOptionIndexGroups;
typedef union {s32 flat[8];GarageOptionIndexGroups groups;} GarageOptionIndices;
/* Complete TIM byte image; byte alignment retains the original unaligned copy path. */
typedef EmblemTim840 GarageTimBytes;
typedef struct {
    s32 fade00,word04;
    s16 reset08[10];
    u8 config1C,config1D,config1E,config1F,config20,active21,config22,layer23;
    u32 *buffers24[2];
    GarageTimBytes *tim2C[2];
    u32 *scratch34; /* Complete512-word preview image. */
    u32 *image38[2],*image40[2],*palette48[2];
    u32 *paletteBackup50[2]; /* Other palette of each undo pair; eight words. */
    u32 *paletteScratch58[2];
    s32 current60,saved64[5];
    GarageOptionPointers options78;
    GarageOptionIndices optionIndices98;
    s32 selectorsB8[4];
    s16 halfC8,halfCA;
    u32 phaseCC;
    u8 shadeD0,flagsD1,stateD2,dirtyD3;
} GarageSelectionPrefix;
extern GarageSelectionPrefix ovl662_editor_workspace;
/* Fifteen editor buffer pointers precede the scalar selection fields;
 * eight option pointers precede the final scalar controls. */
#define RR_GARAGE_BUFFER_OFFSET RR_ALIGN_UP(0x24, _Alignof(void *))
RR_ASSERT_SIZE(GarageSelectionPrefix, RR_ALIGN_UP(RR_GARAGE_BUFFER_OFFSET + 23 * sizeof(void *) + 0x54, _Alignof(GarageSelectionPrefix)));
RR_ASSERT_OFFSET(GarageSelectionPrefix, phaseCC, RR_GARAGE_BUFFER_OFFSET + 23 * sizeof(void *) + 76);
RR_ASSERT_OFFSET(GarageSelectionPrefix, shadeD0, RR_GARAGE_BUFFER_OFFSET + 23 * sizeof(void *) + 80);
void ovl662_update_cursor_pulse(void);
void ovl662_setup_editor_buffers(s32 mode);
RR_ASSERT_OFFSET(GarageSelectionPrefix,reset08,0x08);
RR_ASSERT_OFFSET(GarageSelectionPrefix,config1C,0x1c);
RR_ASSERT_OFFSET(GarageSelectionPrefix, flagsD1, RR_GARAGE_BUFFER_OFFSET + 23 * sizeof(void *) + 81);
/* Negative modes initialize option pointers and scalar selections. */
void ovl662_init_option_groups(s32 mode);
RR_ASSERT_SIZE(GarageOptionPointers, 8 * sizeof(GarageOptionRecord *));
RR_ASSERT_SIZE(GarageOptionIndices,0x20);
RR_ASSERT_OFFSET(GarageOptionPointerGroups, second, 3 * sizeof(GarageOptionRecord *));
RR_ASSERT_OFFSET(GarageOptionIndexGroups,second,0x0C);
RR_ASSERT_OFFSET(GarageSelectionPrefix, options78, RR_GARAGE_BUFFER_OFFSET + 15 * sizeof(void *) + 24);
RR_ASSERT_OFFSET(GarageSelectionPrefix, optionIndices98, RR_GARAGE_BUFFER_OFFSET + 23 * sizeof(void *) + 24);
/* active21 is a two-buffer selector. Copy forward, then retain the selection. */
void ovl662_backup_layer_and_palette(void);
/* Return1 when all 512 words of the selected layer and side are zero. */
s32 ovl662_is_active_layer_empty(void);
RR_ASSERT_OFFSET(GarageSelectionPrefix,layer23,0x23);
void ovl662_toggle_preview_side(void);
RR_ASSERT_OFFSET(GarageSelectionPrefix, buffers24, RR_GARAGE_BUFFER_OFFSET + 0 * sizeof(void *));
RR_ASSERT_OFFSET(GarageSelectionPrefix, tim2C, RR_GARAGE_BUFFER_OFFSET + 2 * sizeof(void *));
RR_ASSERT_OFFSET(GarageSelectionPrefix, scratch34, RR_GARAGE_BUFFER_OFFSET + 4 * sizeof(void *));
RR_ASSERT_OFFSET(GarageSelectionPrefix, image38, RR_GARAGE_BUFFER_OFFSET + 5 * sizeof(void *));
RR_ASSERT_OFFSET(GarageSelectionPrefix, image40, RR_GARAGE_BUFFER_OFFSET + 7 * sizeof(void *));
RR_ASSERT_OFFSET(GarageSelectionPrefix, palette48, RR_GARAGE_BUFFER_OFFSET + 9 * sizeof(void *));
RR_ASSERT_OFFSET(GarageSelectionPrefix, dirtyD3, RR_GARAGE_BUFFER_OFFSET + 23 * sizeof(void *) + 83);
RR_ASSERT_OFFSET(GarageSelectionPrefix, paletteBackup50, RR_GARAGE_BUFFER_OFFSET + 11 * sizeof(void *));
RR_ASSERT_OFFSET(GarageSelectionPrefix, paletteScratch58, RR_GARAGE_BUFFER_OFFSET + 13 * sizeof(void *));
RR_ASSERT_OFFSET(GarageSelectionPrefix, saved64, RR_GARAGE_BUFFER_OFFSET + 15 * sizeof(void *) + 4);
RR_ASSERT_OFFSET(GarageSelectionPrefix, selectorsB8, RR_GARAGE_BUFFER_OFFSET + 23 * sizeof(void *) + 56);
/* Shared 64-byte working owner: scalar animation counters and halfword clips. */
typedef struct {s16 x,y;} GaragePoint;
RR_ASSERT_SIZE(GaragePoint,4);
typedef struct {
    u32 prefix00[5];
    GaragePoint cursor14;
    GaragePoint cursor18;
    u32 word1C;
    GaragePoint first20,second24;
    u32 word28;
    s32 distance2C;
    u32 tail30[4];
} GaragePointWorkingView;
typedef union {
    s16 halves[32];u32 words[16];
    GaragePointWorkingView points;
} GarageWorkingStorage;
RR_ASSERT_SIZE(GarageWorkingStorage,64);
RR_ASSERT_SIZE(GaragePointWorkingView,64);
RR_ASSERT_OFFSET(GaragePointWorkingView,cursor14,0x14);
RR_ASSERT_OFFSET(GaragePointWorkingView,cursor18,0x18);
RR_ASSERT_OFFSET(GaragePointWorkingView,first20,0x20);
RR_ASSERT_OFFSET(GaragePointWorkingView,second24,0x24);
RR_ASSERT_OFFSET(GaragePointWorkingView,distance2C,0x2C);
extern GarageWorkingStorage ovl662_editor_cursor_state;
s32 ovl662_map_controller_buttons(s32 mode);
typedef u16 GarageRectangle[4];
/* Nonzero controller movement replaces both coordinates; neutral preserves output. */
void ovl662_read_mouse_cursor_delta(GaragePoint *output);
/* Signed rectangle center; halves of negative odd extents round downward. */
GaragePoint ovl662_get_rect_center(s32 index);
extern GarageRectangle *ovl662_button_rect_lookup[];

/* Overlay-local selection state; known scalar fields are menu indices.
 * Clip working storage uses signed halfwords; rectangle payloads preserve
 * raw halfword bits and are interpreted as signed coordinates by hit testing. */
void ovl662_restore_saved_selection(void);
/* Returns0 only for a point in [x,x+w) by [y,y+h); arguments use signed low16. */
s32 ovl662_point_outside_rect(s32 x,s32 y,s32 width,s32 height);
s32 ovl662_point_outside_button(s32 index);
/* Capture the selected pointer once, then copy the four halfwords in order.
 * Payload storage may overlap the clip destination. */
void ovl662_set_button_clip_rect(s32 index);
#endif
