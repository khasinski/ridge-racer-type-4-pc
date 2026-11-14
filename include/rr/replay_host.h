#ifndef RR_REPLAY_HOST_H
#define RR_REPLAY_HOST_H

/* Replay files of the desktop build (src/port/replay_session.c). The host
 * installs these hooks; without them (NULL, the default) the game runs as
 * the original. The hooks only observe the replay the game recorded, or
 * stage a saved one through the game's own load script and replay mode:
 *   frame   MainLoop, before the mode dispatch: a load the player asked for
 *           starts here, between game frames;
 *   enter   start of ReplayModeEnter, before the snapshots are restored:
 *           keeps a copy of the recorded replay, or installs the loaded one;
 *   finish  the replay faded out (where ReplayModeUpdate calls FinishRace):
 *           nonzero when the host chose the next mode (a loaded replay goes
 *           back to the title instead of the race results). */
typedef struct ReplayHostHooks {
    void (*frame)(void);
    void (*enter)(void);
    int (*finish)(void);
} ReplayHostHooks;

extern const ReplayHostHooks *ReplayHost;

#endif
