#include "rr/bios_devices.h"
#include "game/menu/menutra21.h"
#include "game/data/menuvars.h"
#include "rr/save_slots.h"
#include "rr/save_progress.h"
#include "rr/menu_selection.h"
/* RODATA 0x80010514 0x24 */
#include "rr/menu_transition.h"
#include "game/menu/menutran.h"
#include "game/menu/menu8.h"
#include "game/data/ldscn1.h"
#include "game/data/globals.h"
#include "game/data/selection.h"

typedef struct { int x; } RR_PACKED U4;

extern unsigned char GpStageQualifyRanks[];
extern short ComDifficultyLevel;
extern short SelectedTeamIndex;
extern short SelectedMakerIndex;

void SetNextModeByGameType(void);

void SetNextModeByGameType(void){
 switch(MainMenuChoice){
 case 0:ActiveSceneId=0x105;break;
 case 1:default:ActiveSceneId=0x131;break;
 case 2:ActiveSceneId=0x137;break;
 case 3:ActiveSceneId=0x127;break;
 case 6:ActiveSceneId=0x201;break;
 case 4:ActiveSceneId=0x149;break;
 case 5:CardMenuOperation=0;ActiveSceneId=0x147;break;
 case 7:ActiveSceneId=0x14E;break;
 case 8:ActiveSceneId=0x13B;break;
 }
}

void UpdateGrandPrixStageProgress(void) {
    int x, c0, i0, j;
    int a0;
    int v1;
    int a2;
    char *s0, *p;
    a2 = (unsigned char)SelectionState.signedBytes[0x17];
    i0 = (signed char)a2;
    if (i0 != -1) {
        if (SelectionState.signedBytes[0x19] <= GpStageQualifyRanks[i0 / 2]) {
            if (SelectionState.signedBytes[i0] != -1) {
                SelectionState.signedBytes[0x17] = a2 + 1;
            }
        }
    }
    a0 = SelectionState.fields.phase17;
    if (a0 == -1) {
        PsyqBzero(RankEntries, 8 * sizeof(RankEntries[0]));
        j = 0;
        do {
            RankEntries[j].gridRank06 = j;
            RankEntries[j].carIndex04 = j;
            j++;
        } while (j < 8);
        v1 = 0;
        PsyqBzero(SelectionRows.bytes, 0x20);
        PsyqBzero(SelectionState.signedBytes, 0x2C);
        a0 = 0;
        v1 = 0;
        a2 = -1;
        *(U4 *)&SelectionState.signedBytes[0x1C] = *(U4 *)&SelectionRows.bytes[0];
        ComDifficultyLevel = 0;
        do {
            p = (char *)SelectionState.signedBytes + v1;
            v1++;
            p[0] = a2;
            p[8] = 3;
        } while (v1 < 8);
        SelectionState.fields.reserved18 = SaveProgress.unlocked60[1];
    }
    x = (unsigned char)SelectionState.signedBytes[0x17];
    x = (x << 24) >> 25;
    SelectionState.signedBytes[0x15] = x;
    if ((x & 0xFF) >= 3U) {
        SelectionState.signedBytes[0x15] = 2;
    }
    *(U4 *)&SelectionRows.bytes[0] = *(U4 *)&SelectionState.signedBytes[0x1C];
    SelectedCourseChoice = a0;
    c0 = SelectionRows.bytes[0];
    SelectedTeamIndex = c0;
    SelectedMakerIndex = SelectionRows.bytes[1];
    SelectedMakerCarIndex = SelectionRows.bytes[2];
    CourseIndex = DecodeCourseIndex(a0);
    ComDifficultyLevel = 0;
    SaveProgress.prefix.progress.prefix00.fields.configuration0C |= 1 << SelectedCourseChoice;
    if (SelectionRows.bytes[2] >= 5) {
        ComDifficultyLevel = (SelectionRows.bytes[2] - 5) % 7;
        return;
    }
    if (SelectionRows.bytes[2] != 0) {
        ComDifficultyLevel = SelectionRows.bytes[2] - 1;
    }
}

int CountProgressFlags(void) {
    int a, b, i;
    int n = 0;
    for (a = 0; a < 4; a++) {
        for (b = 0; b < 4; b++) {
            for (i = 0; i < 20; i++) {
                if ((SaveProgress.prefix.progress.progress10[a][b] >> i) & 1) {
                    n++;
                }
            }
        }
    }
    return n;
}


/* Initial values of the globals this module owns. */
unsigned char GpStageQualifyRanks[4] = {3, 2, 1, 1};
short ComDifficultyLevel;
u16 ActiveSceneId;
RankEntry8 RankEntries[8];
