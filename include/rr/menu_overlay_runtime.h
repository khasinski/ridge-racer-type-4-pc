#ifndef RR_MENU_OVERLAY_RUNTIME_H
#define RR_MENU_OVERLAY_RUNTIME_H
#include "game/data/menuvars.h"
#include "rr/save_slots.h"
#include "game/system/pad.h"
#include "game/sound/sndque.h"
#include "game/gfx/fontdraw.h"
#include "game/race/racegrid.h"

#include "game/data/cdfile2.h"
#include "game/car/carwheel.h"
#include "rr/badge_rows.h"
#include "rr/load_cursor.h"
#include "rr/course_choice_types.h"
#include "rr/frame_storage_types.h"
#include "rr/gpu.h"
#include "rr/matrix_workspace.h"
#include "rr/menu_arena_types.h"
#include "rr/menu_selection.h"
#include "rr/menu_selection_word.h"
#include "rr/menu_state_types.h"
#include "rr/menu_text_rows.h"
#include "rr/preview_rgb.h"
#include "rr/projection_state.h"
#include "rr/race_checkpoint_types.h"
#include "rr/race_setup_types.h"
#include "rr/race_types.h"
#include "rr/save_progress.h"
#include "rr/selection_types.h"
#include "rr/tables.h"
#include "rr/time_record_types.h"
#include "rr/ui.h"
#include "rr/ui_bar.h"

#include "rr/current_selection_types.h"

/* Shared resident storage: one type per symbol in these overlays. */
#include "rr/cd_directory_buffer.h"
#include "rr/ui_bar.h"
extern s32 MenuStepDirection;
extern CourseChoice4 CourseChoiceTable[4][9];
extern u32 ScreenFrame;
#include "rr/player_cars.h"
extern s32 RaceTimeRemaining;
extern s16 MenuItemCount;
extern s16 MenuCursor;
#include "rr/menu_frame_state.h"
extern s16 ReverseCourse;
extern TimeRecord16 RaceRecords[2][8][5];
extern FrameStorage DrawBuffers[2];
extern s32 RaceQuitTargetMode;
#include "game/menu/menu8.h"
#include "rr/image_rows.h"
#include "game/car/carparam.h"

extern s32 TrackLength;
extern s8 PauseCursor;
extern s16 SelectedMakerIndex;

extern s32 TotalRaceTime;
extern s8 RacePaused;
extern s32 LapTimes[10];
#include "rr/menu_arena_base.h"

#include "rr/menu_arena_state.h"
extern UiRow24 *UiRows;

#include "game/ui/uidraw.h"
#include "game/data/selection.h"
extern s32 RaceState;
extern TimeRecord16 LapRecords[2][8][5];
extern MenuTextRow60 *MenuTextRows;
#include "rr/vehicle_table.h"
extern s32 RaceStartWipeY;
extern s16 LapVoicePending;

#include "rr/race_frame_scratch.h"
#include "rr/gpu_scratch_types.h"
#include "scratchpad.h"
#include "game/data/ldscn5.h"
#include "game/gfx/sprpkt.h"
#include "game/menu/menusave.h"
#include "game/menu/mntext.h"
#include "game/race/raceai2.h"
#include "game/race/racecar.h"
#include "game/race/raceset.h"
#include "game/sound/sndctrl.h"
#include "game/ui/hudctrl.h"
typedef RaceFrameScratch Ovl659Scratch;
/* Scratch views over the host scratchpad; the packet cursor and ordering
 * table live in GpuScratch (RR_GPU_SCRATCH). */
#ifndef RR_659_SCRATCH
#define RR_659_SCRATCH ((Ovl659Scratch *)SCRATCHPAD)
#endif
#ifndef RR_666_SCRATCH
#define RR_666_SCRATCH RR_GPU_SCRATCH
#endif
#ifndef RR_MENU_PREVIEW_SCRATCH_WORDS
#define RR_MENU_PREVIEW_SCRATCH_WORDS ((s32 *)SCRATCHPAD)
#endif

/* Resident game and GPU services. */
struct w3g2p;

void SetModelTexturePage(s32);
void ResetCarSlotBuffers(void);
void SetupSkyLightMatrices(char *position, u16 *matrix);
void SetSineLightColors(s32 scale);
void EmitTiledRect(GpuRectangleFields *rectangle, s32 intensity, GpuDmaTag *head);
s32 DebugPrintStub(const char *format, ...);
void SetupDrawDispEnvs(s32 r, s32 g, s16 b, u32 flags);
void *AddDrawAreaPrim(GpuDmaTag *, void *, s32, s32, s32, s32);
void *AddDrawModePrim(GpuDmaTag *ordering, GpuDrawModePacket *packet, u16 mode);
void *AddTilePrim(GpuDmaTag *, void *, s32, s32, s32, s32, s32, s32, s32);
void *AddSpritePrim(GpuDmaTag *ordering, void *cursor, s32 x, s32 y, const UiUvRect *region, u16 clut,
                    s32 flags);
void AddScreenFadeTiles(GpuDmaTag *ordering, s32 intensity, u32 mode);
void SubmitRaceHudPackets(s32);
void DrawViewportBorderFrames(s32);
void DrawHudRotatingQuad(s32, s32);
void DrawHudNumber3(s32, s32);
void DrawHudDigitMod10(s32, s32);
void UpdateHudRankDisplay(s32, s32);
void UpdateFinalLapHudSprite(s32);
void DrawLapCounter(s32, s32);
void DrawSteeringIndicator(GpuDmaTag *ordering, s32 layout);
void DrawRaceStartSignalSprite(s32);
void UpdateCarStartLineSteering(RaceVehicle *);
void SaveCarSnapshots(s32);
void UpdateScreenFade(s32);
void ClearScreenOverlayState(void);
void SetMenuFrameOptions(s32, s32, s32);
void DrawMenuStepArrows(s32, s32, s32, s32, s32, s32, s32);
void StepMenuValue(s32, s32, char *, s32, s32);
void FormatDecimalDigits(char *, s32, s32);
void FormatRaceTime(char *, s32);
void DrawLapTimeList(s32);
void DrawTimeRemaining(s32);
void DrawTimeRemainingForPlayer(s32, s32);
void DrawCars(s32);
void SetCarBounds(RaceVehicle *);
void ResetCarState(RaceVehicle *);
void UpdateCarWorldTransform(RaceVehicle *);
s32 DecodeCourseIndex(s32 selection);
void *EmitSpinningWheelIcon(GpuDmaTag *, void *, s32, s32, s32);
void DrawCourseInfoPanel(s32, s32);
void DrawCourseMapPreview(s32, s32);
void UpdateRaceIntroCamera(RaceVehicle *, s32);
void SetupMenuCarCamera(s32);

void PlayStartCountdownVoice(s32);
void UpdatePauseMenu(s32);
void ApplyPauseMenuChoice(s32);
void StopRaceAudioAndRumble(void);
void FinishRace(void);
void DrawResultsStandingsOverlay(s32);
void DrawTrackSceneryForPlayer(s32);
void UnpackChoiceGlyph(s8 *output, s32 glyph);
s32 PackChoiceGlyph(s8 *);
void InitPreviewImageRows(void);
void DrawSelectionIconGrid(s32);
void StepRaceFrame(s32);
void CheckFinalLapEvent(char *);
void ResetScreenCounters(void);
void InitMenuArenas(void);
s32 LoadCarRangeStep(s32, s32);
s32 LoadResultsSceneStep(void);
s32 LoadGlyphRowsStep(void);
s32 LoadTrackNodesStep(void);
/* Original callers use both the legacy no-argument entry and an explicit pause flag. */
void UpdateLapTiming(RaceVehicle *);
void InitBadgeRow(s32 index, s32 first, s16 x, s32 y, u16 width, u16 mode, u16 flags);
void UpdateBadgeRowAnim(s32);
void InitRect16Row(s32 index, s16 a, s16 b, s16 c, s16 d, s16 e, s16 f);
void InitGlyphRow(int index, int control, int glyph, int presetIndex);
s32 UpdateGlyphRow(s32 selected);
void InitUiRow(s32 index, s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 count, s32 g, s32 h,
                   ...);
int DrawUiRow(s32);
s32 UpdateImageRow(s32);
void SetMenuTextRow(s32 row, s32 position, s32 group, s32 mode, s32 label);
void UpdateMenuTextRow(s32);
void DrawResultsRankIcons(void);
void SetMenuTextRowStyle(s32 row, s32 first, s32 second);
void DrawLensFlareAndSun(RaceVehicle *);
void PlaySfx(s32);
void SoundSetMasterVolume(s16, s16);
void SlotsStartForRace(void);
void SlotSetLevelProgram(s32, s32, s32);
void CdAudioInit(void);
void CdAudioSelectTrack(int bank, int track);
void CdAudioBeginPlay(s32);
void CdAudioFade(s32, s32);
void SeqFadeVolume(s32, s32);
void SeqStartTrack(s32);
void SeqMuteVolume(void);
void BuildCameraMatrices(void);
void SetupPlayerView(s32);
void SetupViewport(s32);
void UpdateAndDrawRearViewMirror(s32);
void UpdateTrackObjects(void);
void DrawTrackObjects(s32);
s32 rsin(s32);
s32 rcos(s32);
#include "psyq/libgte/rotmat.h"
void SetDispMask(s32);

#endif
