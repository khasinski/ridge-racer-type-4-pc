#ifndef RR4_PORT_SETTINGS_REPLAYS_H
#define RR4_PORT_SETTINGS_REPLAYS_H

#include <stddef.h>
#include <stdint.h>

#include "settings_model.h"

/* The Escape menu's Replays page on the host side: lists the replays folder
 * into the model and carries out its Save / Load requests through the
 * replay session (src/port/replay_session.c registers the operations; the
 * settings tests run without them and the page says so). */
typedef struct SettingsReplayOps {
    int (*can_save)(char *why, size_t size);
    int (*save)(char *status, size_t size);
    int (*can_load)(char *why, size_t size);
    int (*load)(const char *path, char *status, size_t size);
    const char *(*directory)(void);
    uint32_t (*layout)(void);
} SettingsReplayOps;

void SettingsReplaysSetOps(const SettingsReplayOps *ops);
/* Fills the page when the panel opens. */
void SettingsReplaysFill(SettingsModel *model);
/* Carries out model->replay_request; a load that starts closes the panel. */
void SettingsReplaysRun(SettingsModel *model);

#endif
