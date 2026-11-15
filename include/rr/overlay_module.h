#ifndef RR_OVERLAY_MODULE_H
#define RR_OVERLAY_MODULE_H
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "rr/overlay_entrypoints.h"

/* Statically linked mode overlays (R4.BIN entries 659..668, duplicates
 * 669..672). The PS1 loaded one image into the 0x80114878 window; a fresh
 * load restored its initialized data and zeroed its BSS. Here every module
 * is linked, its entry points sit in the mode tables (modeinit.c), and the
 * loader binds the module instead of reading its code: OverlayModuleBind
 * runs the module's reset, which returns all of its writable state to the
 * values of a fresh load. Asset entries are still read from the disc. */

/* One writable object of a module. Each translation unit that defines
 * module state lists its objects in an ovlNNN_<unit>_state[] table. */
typedef struct {
    void *data;
    size_t size;
} OverlayStateRegion;
#define RR_OVERLAY_STATE(object) {(void *)&(object), sizeof(object)}

typedef struct {
    const OverlayStateRegion *regions;
    size_t count;
} OverlayStateTable;

/* Image of a module's state as linked (= a fresh load). Capture at startup,
 * before resident code can write overlay state, and restore on every bind. */
typedef struct {
    const OverlayStateTable *const *tables;
    size_t tableCount;
    u8 *pristine;
} OverlayStateImage;

static inline void OverlayStateCapture(OverlayStateImage *image) {
    size_t total = 0, offset = 0, t, i;
    if (image->pristine != NULL)
        return;
    for (t = 0; t < image->tableCount; t++)
        for (i = 0; i < image->tables[t]->count; i++)
            total += image->tables[t]->regions[i].size;
    image->pristine = malloc(total ? total : 1);
    if (image->pristine == NULL)
        abort();
    for (t = 0; t < image->tableCount; t++) {
        for (i = 0; i < image->tables[t]->count; i++) {
            const OverlayStateRegion *region = &image->tables[t]->regions[i];
            memcpy(image->pristine + offset, region->data, region->size);
            offset += region->size;
        }
    }
}

static inline void OverlayStateRestore(OverlayStateImage *image) {
    size_t offset = 0, t, i;
    OverlayStateCapture(image);
    for (t = 0; t < image->tableCount; t++) {
        for (i = 0; i < image->tables[t]->count; i++) {
            const OverlayStateRegion *region = &image->tables[t]->regions[i];
            memcpy(region->data, image->pristine + offset, region->size);
            offset += region->size;
        }
    }
}

/* Idempotent capture: never overwrite the linked initial state on restart. */
void OverlayModulesInit(void);
void ovl659_capture_module(void);
void ovl660_capture_module(void);
void ovl661_capture_module(void);
void ovl662_capture_module(void);
void ovl663_capture_module(void);
void ovl665_capture_module(void);
void ovl666_capture_module(void);
void ovl667_capture_module(void);
void ovl668_capture_module(void);

/* Per-module reset: restore the state a fresh load of the image gave. */
void ovl659_reset_module(void); /* grand_prix */
void ovl660_reset_module(void); /* time_attack */
void ovl661_reset_module(void); /* versus */
void ovl662_reset_module(void); /* garage */
void ovl663_reset_module(void); /* records */
void ovl665_reset_module(void); /* options */
void ovl666_reset_module(void); /* extra_trial */
void ovl667_reset_module(void); /* serial_link */
void ovl668_reset_module(void); /* movie_playback */

/* Registry entry: R4.BIN entry, its duplicate entry (0 if none), reset. */
typedef struct {
    const char *name;
    u16 entry, duplicate;
    void (*reset)(void);
    void (*capture)(void);
} OverlayModule;

/* If R4.BIN entry `file` is an executable mode overlay, reset that linked
 * module and return 1: the caller skips the read and allocates nothing
 * for it. Return 0 for any other entry, which the caller reads as before. */
s32 OverlayModuleBind(s32 file);

#endif
