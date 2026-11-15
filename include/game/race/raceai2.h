#ifndef RR_GAME_RACE_RACEAI2_H
#define RR_GAME_RACE_RACEAI2_H

#include "types.h"

void StopRaceRumble(void);
void FinishRace(void);
int ExitOnLinkLost(void);
int UpdateLinkHandshake(int a0);
void DrawLinkEntryScreen(s32 mode);
/* Each peer alternates the two real buffers according to LinkHostFlag. */
int LinkExchange(void *first, void *second, s32 count, s32 bidirectional);
void LinkWaitModeEnter(void);
void LinkWaitModeUpdate(void);
void LinkExchangeModeUpdate(void);
void LinkErrorModeUpdate(void);
void LinkBattleEndModeUpdate(void);
void StopRaceAudioAndRumble(void);
void RaceAbortToMenuUpdate(void);
void InitPostRaceScreen(void);
void PostRaceScreenModeUpdate(void);
void EvaluateGpQualifyingResult(void);
void SortRaceRankTable(void);
void EnterRaceResultScreen(void);
void SortRankAndShowResults(void);
void SortRankAndEnterResults(void);


#endif /* RR_GAME_RACE_RACEAI2_H */
