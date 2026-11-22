/* BIOS rand/srand (A(2Fh)/A(30h)): PsyZ implements the BIOS generator as
 * Psyz_Rand/Psyz_Srand. */
#include "psyq/libc/rand.h"
#include "rr/cpu_work.h"

int Psyz_Rand(void);
void Psyz_Srand(unsigned int seed);

/* the work being counted charged first (rr/cpu_work.h) */
s32 PsyqRand(void) {
    RrWorkFlush();
    return Psyz_Rand();
}

void PsyqSrand(u32 seed) { Psyz_Srand(seed); }
