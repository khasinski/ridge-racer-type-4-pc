#ifndef RR_IMAGE_ROWS_H
#define RR_IMAGE_ROWS_H

#include "rr/layout.h"

/* Animated image rows allocated by AllocImageRows. f1 is the row state
 * (preview menus drive row 0 through 0..7) and f2 its frame tick. */
typedef struct {
    signed char f0, f1;
    unsigned char f2, f3;
    union {
        unsigned short s[6];
        struct {
            short x, y;
            unsigned short u, v, cx, cy;
        } coords;
    } geom;
    unsigned char b0, b1;
    char reserved[2];
} ImageRow20;
RR_ASSERT_SIZE(ImageRow20, 20);

extern ImageRow20 *ImageRows;

#endif
