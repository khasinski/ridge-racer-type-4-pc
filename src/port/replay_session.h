#ifndef RR4_PORT_REPLAY_SESSION_H
#define RR4_PORT_REPLAY_SESSION_H

#include <stddef.h>
#include <stdint.h>

/* Race replays saved as files (src/port/replay_file.c holds the format).
 *
 * The game records the last lap of a race (SaveCarSnapshots copies every
 * car, ReadCarInput the inputs, into the replay region of SharedWorkArena)
 * and shows it right after the race (ReplayModeEnter / ReplayModeUpdate);
 * the results screen then reuses that memory. The PAL release keeps no
 * replay beyond that, on the memory card or elsewhere, and has no ghost
 * car. Here, at ReplayModeEnter, a host copy of the recorded replay and of
 * the state it is played from (race setup, car snapshots, the rand() state
 * and the race globals the replay reads) is kept: Save writes it to a file,
 * Load stages a file through the game's own path. It sets the race setup,
 * runs the full race load script (the one the link battle uses: the mode
 * overlay, all cars, the course, SetupRace) with the replay mode as its
 * scene, installs the saved replay and car snapshots at ReplayModeEnter and,
 * when the replay fades out, restores the player's own setup and returns to
 * the title screen instead of the race results. Link battle replays (serial
 * cable session) are not saved.
 *
 * ReplaySessionInstall sets the game's ReplayHost hooks. Test hooks for
 * headless runs:
 *   RR4_REPLAY_SAVE=file  save each replay the game records to file when it
 *                         starts playing
 *   RR4_REPLAY_LOAD=file  load file at the title screen
 *   RR4_REPLAY_DIR=dir    the replays folder instead of the one beside the
 *                         preferences
 *   RR4_REPLAY_CAPTURE=base[,step]  write the frame each replay frame drew
 *                         as base.<ReplayFrame>.ppm (every step-th frame):
 *                         keyed by replay frame, so a recorded and a loaded
 *                         replay compare frame by frame whatever fields the
 *                         frames took
 *   RR4_REPLAY_EXIT=1     exit when a replay has played */

/* prefs_path: the preferences file (for the replays folder), or "". */
void ReplaySessionInstall(const char *prefs_path);

/* The replays folder (created on demand), or NULL. */
const char *ReplaySessionDirectory(void);
/* Layout signature of this build's replay blocks. */
uint32_t ReplaySessionLayout(void);

/* Whether a replay can be saved now, and the reason when not. */
int ReplaySessionCanSave(char *why, size_t size);
/* Saves the last recorded (or loaded) replay into the replays folder.
 * Returns 1 with the file name in status, or 0 and the reason. */
int ReplaySessionSave(char *status, size_t size);
/* Whether a load can start now (the title screen, or a saved replay
 * playing; a race's own replay leads to its results first). */
int ReplaySessionCanLoad(char *why, size_t size);
/* Reads and checks path and queues it: it starts at the next game frame.
 * Returns 1, or 0 with the reason in status. */
int ReplaySessionLoad(const char *path, char *status, size_t size);

#endif
