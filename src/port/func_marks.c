/* RR4_FUNC_MARKS builds: the game modules and PsyZ are compiled with
 * -finstrument-functions and this prints, for the functions named in
 * RR4_MARK (a comma list of exported names), "XMARK <name> <clocks>" at
 * each entry, with the emulated clock (psyz/system.h Psyz_EmulatedClocks:
 * system clocks, the interrupt handlers' time owed included), and
 * "PVBLANK <vsync> <clocks>" at each VSync: the port's side of
 * tools/port/ds_frames.py's DuckStation marks, to compare where routines
 * start against the vblanks. PSYZ_PAD_TRACE=1 prints each blank's exact
 * time ("at <ns>"), which XMARK clocks / 33.8688 compare with. Offline
 * clock only (headless runs). */
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psyz/system.h>

#include "func_marks.h"

#define NO_INSTRUMENT __attribute__((no_instrument_function))

enum { MARK_MAX = 256, MARK_HASH = 1024 };
static struct {
    void *fn;
    const char *name;
} marks[MARK_HASH];
static int state; /* 0 not set up, 1 marking, -1 off */

NO_INSTRUMENT static unsigned mark_slot(void *fn) {
    return (unsigned)(((unsigned long long)(size_t)fn >> 2) * 0x9E3779B1u) % MARK_HASH;
}

NO_INSTRUMENT static void setup(void) {
    const char *list = getenv("RR4_MARK");
    char *copy, *tok, *save = NULL;
    int count = 0;

    state = -1;
    if (!list || !*list)
        return;
    copy = strdup(list);
    if (!copy)
        return;
    for (tok = strtok_r(copy, ",", &save); tok && count < MARK_MAX; tok = strtok_r(NULL, ",", &save)) {
        void *fn = dlsym(RTLD_DEFAULT, tok);
        unsigned slot;
        if (!fn) {
            fprintf(stderr, "RR4_MARK: no symbol %s\n", tok);
            continue;
        }
        for (slot = mark_slot(fn); marks[slot].fn && marks[slot].fn != fn; slot = (slot + 1) % MARK_HASH) {
        }
        marks[slot].fn = fn;
        marks[slot].name = tok;
        count++;
    }
    state = count ? 1 : -1;
}

NO_INSTRUMENT void __cyg_profile_func_enter(void *fn, void *site) {
    unsigned slot;
    (void)site;
    if (state <= 0) {
        if (state < 0)
            return;
        setup();
        if (state < 0)
            return;
    }
    for (slot = mark_slot(fn); marks[slot].fn; slot = (slot + 1) % MARK_HASH) {
        if (marks[slot].fn == fn) {
            fprintf(stderr, "XMARK %s %llu\n", marks[slot].name, Psyz_EmulatedClocks());
            return;
        }
    }
}

NO_INSTRUMENT void __cyg_profile_func_exit(void *fn, void *site) {
    (void)fn;
    (void)site;
}

NO_INSTRUMENT void Rr4FuncMarksVSync(long vsync) {
    if (state == 0)
        setup();
    if (state > 0)
        fprintf(stderr, "PVBLANK %ld %llu\n", vsync, Psyz_EmulatedClocks());
}
