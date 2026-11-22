/* BIOS rand/srand (A(2Fh)/A(30h)): PsyZ implements the BIOS generator as
 * Psyz_Rand/Psyz_Srand. */
#include "psyq/libc/rand.h"

int Psyz_Rand(void);
void Psyz_Srand(unsigned int seed);

s32 PsyqRand(void) {
    return Psyz_Rand();
}

void PsyqSrand(u32 seed) { Psyz_Srand(seed); }
