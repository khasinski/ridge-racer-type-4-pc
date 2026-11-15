#ifndef RR_GOURAUD_QUAD_H
#define RR_GOURAUD_QUAD_H
#include "rr/gpu.h"
/* One complete POLY_G4 owner: semantic packet fields and nine DMA words.
 * The word view permits tag updates and valid ordering-word aliases without
 * reinterpreting a three-byte link array as a separate unsigned32 object. */
typedef union {PolyG4Packet packet;u32 words[9];} PolyG4Storage;
RR_ASSERT_SIZE(PolyG4Storage, RR_GPU_PACKET_SIZE(32));
#endif
