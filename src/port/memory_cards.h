#ifndef RR4_PORT_MEMORY_CARDS_H
#define RR4_PORT_MEMORY_CARDS_H

#include <stddef.h>

/* Where the memory cards live. PsyZ keeps each card as a host directory
 * (bu00 for slot 1, bu10 for slot 2). Interactive runs keep them in one
 * fixed directory, the one holding the preferences file, whatever the
 * working directory; headless runs (and a run without a preferences
 * location) keep PsyZ's default, the working directory. */

/* Maps the cards into directory (UTF-8, with or without a trailing
 * separator) and creates them there. Cards that exist only in the working
 * directory, where earlier versions kept them, are copied over first (the
 * originals stay) and the copy is logged. Returns 1 on success, 0 if the
 * directories cannot be created (the cards then stay in the working
 * directory). */
int Rr4CardsUseDirectory(const char *directory);

/* The card directory of slot "bu00" / "bu10" in the current mapping, with
 * a trailing separator ("bu00/" by default). */
void Rr4CardsPath(const char *slot, char *out, size_t size);

/* Back to the working directory (the tests). */
void Rr4CardsUseWorkingDirectory(void);

#endif
