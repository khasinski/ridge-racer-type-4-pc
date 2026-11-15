#ifndef RR_GAME_DATA_CDFILE1_H
#define RR_GAME_DATA_CDFILE1_H

#include "types.h"
#include <stddef.h>

s32 GetCourseFileIndex(s32 arg0);
int GetCarFileIndex(int typeId, int teamId, int stageId, int variant);
int GetR4FileIndex(int category, int index);
s32 ParseR4Directory(const u32 *directory, size_t words, u32 fileBytes, s32 base);
void LoadR4Directory(void);
void InitCdFileDirectory(void);
void DrawLoadingTriangle(void);


#endif /* RR_GAME_DATA_CDFILE1_H */
