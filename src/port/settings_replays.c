#include "settings_replays.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "replay_file.h"

static const SettingsReplayOps *s_ops;
static char s_paths[SETTINGS_REPLAYS_MAX][1024];

void SettingsReplaysSetOps(const SettingsReplayOps *ops) { s_ops = ops; }

static const char *BaseName(const char *path) {
    const char *name = path;
    for (const char *c = path; *c; c++)
        if (*c == '/' || *c == '\\')
            name = c + 1;
    return name;
}

void SettingsReplaysFill(SettingsModel *m) {
    static ReplayListEntry list[SETTINGS_REPLAYS_MAX];
    const char *dir;
    int count;

    m->replays_available = s_ops != NULL;
    m->replay_count = 0;
    m->replay_request = 0;
    if (!s_ops)
        return;
    m->replay_can_save = s_ops->can_save(m->replay_save_help, sizeof(m->replay_save_help));
    dir = s_ops->directory();
    count = dir ? ReplayFileList(dir, s_ops->layout(), list, SETTINGS_REPLAYS_MAX) : 0;
    for (int i = 0; i < count; i++) {
        SettingsReplayRow *row = &m->replays[i];
        const ReplayFileInfo *info = &list[i].info;

        snprintf(s_paths[i], sizeof(s_paths[i]), "%s", list[i].path);
        row->ok = list[i].ok;
        if (row->ok) {
            snprintf(row->label, sizeof(row->label), "%s", info->title[0] ? info->title : ReplayModeName(info->mode));
            snprintf(row->value, sizeof(row->value), "%s", list[i].label);
            snprintf(row->help, sizeof(row->help), "Enter plays %.150s", BaseName(list[i].path));
        } else {
            snprintf(row->label, sizeof(row->label), "%.60s", BaseName(list[i].path));
            snprintf(row->value, sizeof(row->value), "Unreadable");
            snprintf(row->help, sizeof(row->help), "Not a replay this version can play (another version, or damaged).");
        }
    }
    m->replay_count = count;
    m->dirty = 1;
}

void SettingsReplaysRun(SettingsModel *m) {
    int request = m->replay_request;
    int row = m->row;

    if (!request)
        return;
    m->replay_request = 0;
    m->dirty = 1;
    if (!s_ops)
        return;
    if (request == SETTINGS_REPLAY_SAVE) {
        s_ops->save(m->status, sizeof(m->status));
        SettingsReplaysFill(m);
        m->row = row;
        return;
    }
    if (request == SETTINGS_REPLAY_LOAD && m->replay_index >= 0 && m->replay_index < m->replay_count) {
        if (!m->replays[m->replay_index].ok) {
            snprintf(m->status, sizeof(m->status), "%s", m->replays[m->replay_index].help);
            return;
        }
        if (!s_ops->can_load(m->status, sizeof(m->status)))
            return;
        if (s_ops->load(s_paths[m->replay_index], m->status, sizeof(m->status)))
            SettingsModelClose(m); /* the game loads it when it resumes */
    }
}
