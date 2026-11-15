#ifndef RR_GAME_DATA_TRKLD1_H
#define RR_GAME_DATA_TRKLD1_H

#include "types.h"

extern s32 RaceSoundTrackLoadState;
extern s32 RaceLoadStepIndex;
extern int RaceLoadState;
extern int RaceLoadSubState;
extern char *RaceLoadCursor;
int LoadCarSpecStep(int a0, int a1, int a2);
int LoadMenuCarStep(int a0, int a1);
int LoadCourseAssetsStep(int a0, int a1);
int LoadVabPairStep(char *a0);
void RaceLoadReset(void);
s32 LoadMenuSoundVabStep(void);
s32 LoadMenuSoundSeqStep(void);
int LoadRaceSoundStep(void);
void SetRaceLoadCursor(char *buffer);
char *GetRaceLoadCursor(void);
int LoadSlotBankStep(int a0, int a1);
int LoadComSlotBankStep(int a0);
int LoadSoundBanksStep(int a0, int a1, int a2);
int LoadSfxVabStep(int a0, int a1);


#endif /* RR_GAME_DATA_TRKLD1_H */
