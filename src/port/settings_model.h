#ifndef RR4_PORT_SETTINGS_MODEL_H
#define RR4_PORT_SETTINGS_MODEL_H

#include "preferences.h"

/* The Escape settings panel as data: its pages and rows, the selection, the
 * binding being captured and what the host has to do next. It edits a
 * Rr4Prefs copy and knows neither SDL events nor the GPU, so every rule can
 * be exercised by feeding it actions (tests/port/settings_test.c). The look
 * and behaviour follow the Parasite Eve and Rage Racer ports. */

enum {
    SETTINGS_PAGE_GRAPHICS = 0,
    SETTINGS_PAGE_CONTROLS,
    SETTINGS_PAGE_AUDIO,
    SETTINGS_PAGE_DISC,
    SETTINGS_PAGE_LINK,
    SETTINGS_PAGE_COUNT,
    /* Rows after a page's content: restore page defaults, resume, quit. */
    SETTINGS_ACTION_ROWS = 3,
    /* Content rows shown at once; longer pages scroll. */
    SETTINGS_VISIBLE_ROWS = 9,
};

/* Graphics enhancements the renderer may lack (SettingsModel.unavailable),
 * with the values of PsyZ's PSYZ_VIDEO_FEATURE_*. */
enum {
    SETTINGS_FEATURE_UPSCALE = 1 << 0,
    SETTINGS_FEATURE_ANTIALIASING = 1 << 1,
    SETTINGS_FEATURE_FILTERING = 1 << 2,
    SETTINGS_FEATURE_GEOMETRY = 1 << 3,
    SETTINGS_FEATURE_WIDESCREEN = 1 << 4,
    SETTINGS_FEATURE_INTERPOLATION = 1 << 5,
};

typedef enum SettingsAction {
    SETTINGS_NONE = 0,
    SETTINGS_UP,
    SETTINGS_DOWN,
    SETTINGS_LEFT,
    SETTINGS_RIGHT,
    SETTINGS_PAGE_PREV,
    SETTINGS_PAGE_NEXT,
    SETTINGS_ACTIVATE, /* Enter / Space / gamepad South */
    SETTINGS_BACK,     /* Escape / gamepad East: cancel capture or resume */
    SETTINGS_CLEAR,    /* Backspace / Delete: restore the selected binding */
} SettingsAction;

enum {
    SETTINGS_DEVICE_KEYBOARD = 0,
    SETTINGS_DEVICE_CONTROLLER = 1,
};

typedef struct SettingsModel {
    Rr4Prefs prefs;         /* what the game runs with while open */
    int open;
    /* SETTINGS_FEATURE_* the renderer cannot draw (set by the host after
     * SettingsModelOpen): their rows are greyed out, keep their value and
     * show unavailable_note as help. */
    unsigned unavailable;
    const char *unavailable_note;
    int page;
    int row;                /* selected row of the page */
    int scroll;             /* first visible content row */
    int device;             /* Controls page: which bindings are listed */
    int capturing;          /* waiting for a key or button for the row */
    int dirty;              /* needs repainting */
    int changed;            /* prefs changed since the host last applied */
    int edited;             /* prefs changed since the panel opened */
    int quit;               /* Quit game was chosen */
    int choose_disc;        /* the host should show the image dialog */
    const char *mounted;    /* image in the drive (host owned), or NULL */
    /* Identifies a chosen image (Rr4DiscCheckImage): 0 refuses it with the
     * message written to message. NULL (as SettingsModelOpen leaves it)
     * accepts any CUE file. */
    int (*check_disc)(const char *path, char *message, size_t size);
    int editing;            /* Link cable page: typing into the row */
    char edit[256];         /* the text being typed */
    char link_brief[48];    /* the connection, as the host last reported it */
    char link_status[192];
    char status[192];       /* replaces the row help until the next move */
} SettingsModel;

void SettingsModelOpen(SettingsModel *model, const Rr4Prefs *prefs, const char *mounted);
void SettingsModelClose(SettingsModel *model);

const char *SettingsPageName(int page);
int SettingsContentRows(const SettingsModel *model);
int SettingsRowCount(const SettingsModel *model);
int SettingsVisibleContentRows(const SettingsModel *model);

void SettingsModelDo(SettingsModel *model, SettingsAction action);
void SettingsModelSwitchPage(SettingsModel *model, int page);
void SettingsModelSelect(SettingsModel *model, int row);
/* While capturing: a key (SDL scancode) or gamepad binding (SDL button, or
 * SDL_GAMEPAD_BUTTON_COUNT / +1 for a trigger) for the selected row. Inputs
 * for the other device are ignored. Returns 1 if consumed. */
int SettingsModelCaptureKey(SettingsModel *model, int scancode);
int SettingsModelCaptureButton(SettingsModel *model, int binding);
/* The image dialog finished: path, or NULL when cancelled. A CUE file that
 * check_disc accepts becomes the image at the next start. */
void SettingsModelDiscChosen(SettingsModel *model, const char *path);

/* While editing a text row (Link cable page): typed UTF-8 text; only the
 * characters of an address or port are taken. Returns 1 if consumed.
 * SETTINGS_ACTIVATE keeps the text, SETTINGS_BACK drops it and
 * SETTINGS_CLEAR deletes the last character. */
int SettingsModelTextInput(SettingsModel *model, const char *text);
/* The host's view of the link connection for the Link cable page: a short
 * value ("Connected, 0.4 ms") and the whole line for the help. */
void SettingsModelSetLinkStatus(SettingsModel *model, const char *brief, const char *status);

/* Returns, and forgets, whether prefs changed since the last call. */
int SettingsModelTakeChanged(SettingsModel *model);

/* What a row shows. kind: 0 action, 1 choice (left/right), 2 binding,
 * 3 information. label/value point at static or model storage. */
typedef struct SettingsRowView {
    int kind;
    const char *label;
    const char *value;
    const char *help;
    int danger;
    int capturing;
    int disabled; /* the renderer lacks it: shown greyed, not changed */
} SettingsRowView;
void SettingsModelRow(const SettingsModel *model, int row, SettingsRowView *view);
/* Help for the selected row (or the status message). */
const char *SettingsModelHelp(const SettingsModel *model);

/* Pointer at (x, y) in design units from the panel's top left. click: 0 hover,
 * 1 press. A press outside the panel resumes. */
void SettingsModelPointer(SettingsModel *model, float x, float y, int click);
/* Mouse wheel: steps the scroll of a long page. */
void SettingsModelWheel(SettingsModel *model, int step);

/* ---- geometry, design units scaled to the window (as Rage Racer / PE) ---- */

enum {
    SETTINGS_PANEL_W = 600,
    SETTINGS_PAD = 32,
    SETTINGS_HEADER_H = 128,
    SETTINGS_ROW_H = 46,
    SETTINGS_ACTION_GAP = 14,
    SETTINGS_FOOTER_H = 74,
    SETTINGS_VALUE_W = 250,
    SETTINGS_TAB_TOP = 62,
    SETTINGS_TAB_H = 34,
    SETTINGS_REFERENCE_H = 820,
};

typedef struct SettingsLayout {
    float scale;
    int x, y, w, h; /* panel rectangle in window pixels */
} SettingsLayout;

typedef struct SettingsRect {
    float x, y, w, h;
} SettingsRect;

int SettingsDesignHeight(const SettingsModel *model);
void SettingsComputeLayout(const SettingsModel *model, int window_w, int window_h,
                           SettingsLayout *layout);
/* Top of the visible slot of a row, or a negative value when scrolled away. */
float SettingsRowTop(const SettingsModel *model, int row);
SettingsRect SettingsTabRect(int page);
float SettingsValueCentre(void);

#endif
