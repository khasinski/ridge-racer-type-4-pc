#ifndef RR_GAME_DATA_CDFILE2_H
#define RR_GAME_DATA_CDFILE2_H

#include "rr/car_render_types.h"
#include "game/data/resinit.h"
#include "rr/model_table.h"

extern CarSlotBufferC8 CarSlotBuffers[16];

/* Captures the raw depth shift and current render flags in scratchpad; resets
 * the two RGB/command templates and the viewport to (0,0,320,256). */
void SetupRenderer(s32 shift);
void AllocCarSlotBuffers(const CarModelHeader *model, s32 carIndex);
s32 CdFileStep(void);
s32 CdFileReadStart(s32 a0, void *a1);
s32 CdFileReadRange(s32 a0, s32 a1, s32 a2, void *a3);
s32 CdFileBytesLoaded(void);
/* RroModelTable *RelocateOffsetTable(const void *wire): see rr/model_table.h. */
/* Patch decoded mesh command pointers; native tables retain full pointers. */
void PatchOffsetTableTexPages(RroModelTable *table, s32 textureSlot);
/* bankImage is the serialized RroModelBankWire (unchanged by registration). */
void PatchModelBankTexPages(const void *bankImage, s32 textureSlot);
int RequestSceneLoad(int a0);
int SceneLoadStep(void);
int RequestCarLoad(int typeId, int teamId, int stageId, int variant, int destSlot);
s32 CarLoadStep(void);
int RequestVramImageLoad(void);
int VramImageLoadStep(void);
struct RaceVehicle;
/* Interpolates the course lane lighting at the vehicle and loads the GTE
 * light/colour matrices and back colour; returns the low light index. */
int ComputeCarSurfaceLighting(const struct RaceVehicle *vehicle, int lightIndex);
s32 CdFileStateCheckShell(void);
s32 CdFileStateGetTN(void);
s32 CdFileStateReloadDirectory(void);
s32 CdFileStateSetMode(void);
s32 CdFileStateWaitSetMode(void);
s32 CdFileStateWaitShellClose(void);
s32 CdFileStateSetloc(void);
s32 CdFileStateWaitSetloc(void);
s32 CdFileStateStartRead(void);
s32 CdFileStateReadSync(void);
s32 CdFileFinish(void);
s32 CdFileStateRetry(void);
int ApplyCarWheelTexturePatch(int a0, int a1, int a2, int a3, int a4);


#endif /* RR_GAME_DATA_CDFILE2_H */
