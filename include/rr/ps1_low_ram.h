#ifndef RR_PS1_LOW_RAM_H
#define RR_PS1_LOW_RAM_H

#include "rr/layout.h"
#include "rr/car_record_types.h"
#include "game/car/bounds.h"
#include "rr/presentation_types.h"

/* The first bytes of PS1 RAM: the kernel's jump vectors and table pointers,
 * constant once the BIOS has booted. Original code reaches them through
 * pointers that are still null: the car-record and car-bounds slots of a
 * player car whose model is not loaded yet (CarRecordPtrs, CarBoundsTable),
 * the dialogue page store through a null Dialogue (UpdateDialogue) and the
 * presentation controller before InitMotionController binds it
 * (MotionControllerView, rr/scene_camera_state.h). On
 * the console those reads and stores share one memory, so a store made
 * through one null pointer is seen by the reads through another; the host
 * keeps one image of the bytes for all of them. The car views have the PS1
 * layout; the presentation controller's fields after its script pointer
 * (+0x64) sit a few bytes later on 64-bit hosts than on the console. */
enum { PS1_LOW_RAM_BYTES = 0x400 };

typedef union {
    u8 bytes[PS1_LOW_RAM_BYTES];
    CarRecord carRecord;
    CarBounds carBounds;
    PresentationController presentation;
} Ps1LowRam;

extern Ps1LowRam Ps1LowRamImage;

/* PS1 offset of MenuTextState.b7 (after its 32-bit page pointer). */
enum { PS1_DIALOGUE_B7_OFFSET = 7 };

RR_ASSERT_SIZE(CarRecord, 0x218);
_Static_assert(sizeof(CarRecord) <= PS1_LOW_RAM_BYTES, "car record view");

#endif
