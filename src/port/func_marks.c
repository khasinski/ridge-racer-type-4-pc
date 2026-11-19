/* RR4_FUNC_MARKS builds: the game modules and PsyZ are compiled with
 * -finstrument-functions and this prints, for the functions named in
 * RR4_MARK (a comma list of exported names), "XMARK <name> <clocks>" at
 * each entry (and "XMARK <name>.ret <clocks>" at its return for a name
 * given as name:ret), with the emulated clock (psyz/system.h Psyz_EmulatedClocks:
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
#include "rr/gpu_scratch_types.h"
#include <stdint.h>

unsigned long long Psyz_GteOpCount(unsigned op);
/* RR4_MARK_WORK=1: after each mark "XWORK <packet cursor> <counts>", the
 * GTE commands executed so far of these opcodes (Psyz_GteOpCount: RTPS,
 * NCLIP, DPCS, INTPL, MVMVA, NCDS, NCDT, NCCS, CC, NCS, NCT, SQR, DPCT,
 * AVSZ3, AVSZ4, RTPT, GPF, GPL, NCCT): the work the race frame's CPU time
 * model counts (rr/cpu_work.h), for fitting it. */
NO_INSTRUMENT static void work(void) {
    static int on = -1;
    if (on < 0) on = getenv("RR4_MARK_WORK") != NULL;
    if (!on) return;
    static const int ops[] = {0x01, 0x06, 0x10, 0x11, 0x12, 0x13, 0x16, 0x1B, 0x1C, 0x1E,
                              0x20, 0x28, 0x2A, 0x2D, 0x2E, 0x30, 0x3D, 0x3E, 0x3F};
    fprintf(stderr, "XWORK %lu", (unsigned long)(uintptr_t)RR_GPU_SCRATCH->cursor);
    for (unsigned i = 0; i < sizeof ops / sizeof ops[0]; i++)
        fprintf(stderr, " %llu", Psyz_GteOpCount(ops[i]));
    fprintf(stderr, "\n");
}

enum { MARK_MAX = 256, MARK_HASH = 1024 };
static struct {
    void *fn;
    const char *name;
    int ret;
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
        char *flag = strchr(tok, ':');
        void *fn;
        unsigned slot;
        if (flag)
            *flag++ = '\0';
        fn = dlsym(RTLD_DEFAULT, tok);
        if (!fn) {
            fprintf(stderr, "RR4_MARK: no symbol %s\n", tok);
            continue;
        }
        for (slot = mark_slot(fn); marks[slot].fn && marks[slot].fn != fn; slot = (slot + 1) % MARK_HASH) {
        }
        marks[slot].fn = fn;
        marks[slot].name = tok;
        marks[slot].ret = flag && strcmp(flag, "ret") == 0;
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
            work();
            return;
        }
    }
}

NO_INSTRUMENT void __cyg_profile_func_exit(void *fn, void *site) {
    unsigned slot;
    (void)site;
    if (state <= 0)
        return;
    for (slot = mark_slot(fn); marks[slot].fn; slot = (slot + 1) % MARK_HASH) {
        if (marks[slot].fn == fn) {
            if (marks[slot].ret) {
                fprintf(stderr, "XMARK %s.ret %llu\n", marks[slot].name, Psyz_EmulatedClocks());
                work();
            }
            return;
        }
    }
}

NO_INSTRUMENT void Rr4FuncMarksVSync(long vsync) {
    if (state == 0)
        setup();
    if (state > 0)
        fprintf(stderr, "PVBLANK %ld %llu\n", vsync, Psyz_EmulatedClocks());
}
