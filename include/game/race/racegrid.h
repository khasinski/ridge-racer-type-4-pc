#ifndef RR_GAME_RACE_RACEGRID_H
#define RR_GAME_RACE_RACEGRID_H

#include "types.h"
#include "rr/race_types.h"
#include "rr/ui_bar.h"
#include "rr/gpu.h"

extern UiUvRect PauseMenuLabelUvs[8];
extern u16 PauseTitleClutIds[2], PauseRowClutIds[4];
extern u8 PauseRowOffsetX[4];

/* Serial-link entry flags, [console][car on that console]; the flat index is
 * the car slot (0..3). Row 1 is named LinkRaceSyncFlags (0x800ABEBE) in the
 * PS1 map; the link exchange copies the host's row 0 and the client's row 1
 * to the peer. Values: 1 entered, 0 none, -1 declined. */
extern s8 LinkEntryState[2][2];
/* Finishing rank of each link car in the last race (0x8010C0C8); the course
 * menu stores -1 in entry 0 when leaving for a new session. */
extern s16 LinkPrevCarRanks[4];

void DrawStartCountdown(GpuDmaTag *ordering, s32 frame);
void DrawPauseMenu(GpuDmaTag *ordering, s32 selection);
void ResetLinkSioControl(void);
int IsLinkPeerPresent(void);
void OpenLinkSerial(void);
void CloseLinkSerial(void);
void BeginLinkSync(void);
void SetupMenuCarCamera(int a);
void PlayStartCountdownVoice(int a0);
void InitPlayerRumble(void);
void UpdatePauseMenu(int a0);
void ApplyPauseMenuChoice(int arg0);
int LinkTimeoutCallback(int a0, unsigned int a1);
int ExitOnLinkLost(void);
int UpdateLinkHandshake(int a0);
int PollLinkStatus(void);



void UpdateRaceChaseCamera(RaceVehicle *vehicle);

extern s16 LinkHostFlag;

#endif /* RR_GAME_RACE_RACEGRID_H */
