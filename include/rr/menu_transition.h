#ifndef RR_MENU_TRANSITION_H
#define RR_MENU_TRANSITION_H

#include "game/data/menuvars.h"
#include "game/data/ldscn1.h"
#include "rr/badge_rows.h"

/* Script operations return completion; scene-entry callbacks take no arguments.
 * Script bytes are signed: negative values select the next scene. The current
 * script position is reloaded after operations, which may replace the script. */
typedef s32 (*MenuScriptOperation)(void);
typedef void (*MenuSceneEntry)(void);
extern MenuScriptOperation MenuScriptOps[];
extern MenuSceneEntry MenuSceneEntries[];
/* Native replay load also reads the preset emblem bank before car CLUTs. */
enum { MENU_SCRIPT_REPLAY = 11 };
extern s8 *MenuScripts[];

extern s8 MenuScriptStep;

s32 UpdateLinkHandshake(s32);
s32 UpdateRaceOpeningOverlay(s32);
void StartDemoFadeOutAndCloseVab(void);
s32 IntroScriptStep(void);
void SetDispMask(s32);
void UpdateMenuScript(void);

#endif
