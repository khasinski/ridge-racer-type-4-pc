#ifndef RR_DRAW_DISTANCE_H
#define RR_DRAW_DISTANCE_H

/* Desktop preference, off by default. The original course renderer expands
 * its visibility lists and doubles its ordering-table depth range when on.
 * Shared with the host settings without changing the memory-card options. */
extern int ExtendedDrawDistance;

/* Called before the cars and course of each view, also after a live change. */
void ApplyCourseDrawDistance(void);

#endif
