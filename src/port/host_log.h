#ifndef RR4_PORT_HOST_LOG_H
#define RR4_PORT_HOST_LOG_H

/* The port's messages. Rr4Log writes to stderr, as before, and, once
 * Rr4LogOpen has opened it, also to the log file players attach to bug
 * reports (ridge-racer-type-4.log beside the preferences, truncated at each
 * launch). PsyZ's INFOF / WARNF / ERRORF reach SDL's log, which then goes to
 * the file as well. Headless runs open no file: stderr stays exactly what it
 * was. */

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 1, 2)))
#endif
void Rr4Log(const char *format, ...);

/* Truncates path and sends the log there too. Returns 1 when it opened. */
int Rr4LogOpen(const char *path);
/* The open log file's path, or "". */
const char *Rr4LogPath(void);
/* Stops writing to the file (the tests; the game keeps it to the end). */
void Rr4LogClose(void);

#endif
