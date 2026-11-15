#ifndef RR_SCENE_CAMERA_TYPES_H
#define RR_SCENE_CAMERA_TYPES_H

#include "rr/geometry.h"
#include "rr/lighting_types.h"
#include "rr/track_lookup_types.h"
#include "rr/model_table.h"
#include "rr/presentation_types.h"
#include "rr/race_types.h"
#include "rr/track_sample_types.h"
#include "rr/cd.h"
#include "game/car/grid.h"
#include "game/demo/attract.h"

struct CamNode44;
struct TrackLightPreset8;

/* A word-component position followed by the SDK short-angle vector. */
typedef struct {
    VectorWords16 position00;
    Vec3sFields8 rotation10;
} SceneCameraPose18;

typedef struct {
    SceneCameraPose18 pose00;
    s32 absolute18, distance1C, projection20;
} SceneCameraView24;

typedef struct {
    SceneCameraView24 view00;
    s32 time24, restart28;
} SceneCameraKeyframe2C;

typedef SceneCameraKeyframe2C SceneCameraTrack24[24];

/* Shared storage initialized by InitCameraState and consumed by both the
 * showcase controller and the reflection renderer. Unknown words stay
 * explicit; they are not separate, incompatible views of the same camera. */
typedef struct {
    SceneCameraPose18 pose00;
    SceneCameraPose18 parts18[4];
    PreviewLighting40 lighting78;
    s32 extentA0, mirrorA4;
    s32 reservedA8;
    s32 offsetAC, offsetB0;
    s32 reservedB4[4];
    s32 offsetC4, offsetC8;
    s32 reservedCC[13];
    s32 distance100;
    s32 frame104, phase108, segment10C;
    VectorWords16 position110;
    Vec3s8 rotation120;
    s32 reserved128, value12C, value130, reserved134;
    SceneCameraTrack24 *tracks138;
    s32 mode13C, mode140;
    s32 reserved144[7];
} SceneCameraState160;

/* Written by race-audio setup, then consumed by the staged bank loader and
 * slot starter. All four words are configuration, not independent globals. */
typedef struct {
    s32 sfxVabId;
    s32 carVariants[2];
    s32 comBank;
} RaceSoundConfig;
RR_ASSERT_SIZE(RaceSoundConfig, 0x10);
RR_ASSERT_OFFSET(RaceSoundConfig, carVariants, 4);
RR_ASSERT_OFFSET(RaceSoundConfig, comBank, 0xC);

/* Demo/attract fade controller words (PS1 RaceContext+0x3E0..0x443). */
typedef struct {
    s32 f3E0, f3E4, scale3E8, f3EC;
    u8 r3F0, g3F1, b3F2, f3F3;
    s32 f3F4, f3F8, f3FC, f400, f404, f408, f40C;
    s16 f410, f412, f414, f416, f418, f41A;
    s32 f41C, f420, f424, f428, f42C, f430, f434, f438, f43C, f440;
} DemoFadeState;

/* Opening lens-flare/intro sequence words (PS1 RaceContext+0x88..0xA7).
 * The PS1 symbol SceneRenderScratchPad named this block; RaceIntroCameraState
 * was the done9C word. */
typedef struct {
    s32 frame88, fadeFrame8C, fadeActive90, mode94;
    s32 endFrame98, done9C, volumeA0, triggerA4;
} SceneOpeningState20;

/* Title intro work word (PS1 IntroSceneWorkState, RaceContext+0x444). */
typedef struct {
    s32 step444;
    s32 reserved448[(0x4BC - 0x448) / 4];
} SceneIntroWork78;

/* Epilogue credits camera (movie_playback overlay, RaceContext+0x444). */
typedef struct {
    s32 counter444, step448, state44C;
    s32 position450[3], reserved45C;
    s16 pitch460, yaw462, roll464, reserved466;
    s32 target468[4], delta478[3];
    u8 reserved484[4];
    s16 offset488[4], angle490, timer492, timer494, reserved496;
    s32 node498;
    s16 fraction49C, lateral49E;
    s32 velocity4A0;
    s32 counter4A4, mode4A8, phase4AC, reserved4B0, counter4B4;
    s32 reserved4B8;
} EpilogueCameraWork78;

/* The one RaceContext object (PS1 0x800AC650..0x800ACBCB). Every PS1 symbol
 * that addressed into this range is a named member here; the PS1 offset is
 * kept as the member suffix. Pointer members are host pointers, so offsets
 * after the first pointer differ on the host. */
typedef struct SceneRaceContext {
    s16 startGridOrder00[8];      /* car id (1-based) per start-grid slot */
    s32 comCarCount10;            /* PS1 ActiveSoundEventCount */
    s32 playerCount14;            /* PS1 NumPlayers */
    s32 bgmViewerReturn18;        /* set when leaving the BGM viewer demo */
    s32 cameraNodeChanged1C;      /* PS1 CameraCutFlag */
    VectorWords16 cameraFollowPosition20;
    Vec3s8 cameraFollowOffset30;
    s32 cameraDistanceSpan38, cameraNearDistance3C;
    s32 cameraPreviousDistance40; /* PS1 CarTrackProgressDelta */
    s32 cameraLimitReached44;     /* PS1 CarTrackDirectionSign */
    s32 cameraNodeFrames48;       /* PS1 HorizonRollTargetAngle */
    s32 cameraNodeIndex4C;
    /* Showcase sub-ordering tables (0x15F entries each), [object][draw buffer]. */
    struct GpuDmaTag *subOrdering50[2][2];
    /* Secondary viewport camera (PS1 SecondaryViewportCam). */
    SceneCameraView24 view60;
    s32 reserved84;
    SceneOpeningState20 opening88;
    s32 introSavedRumbleA8[2];
    /* Movie (STR) playback; the resource index is chosen before the mode
     * switch (epilogue credits) and cleared by InitDemoFadeContext. */
    s32 streamResourceB0;
    s32 streamPhaseB4;            /* PS1 MoviePlaybackDispatchMode */
    void *streamStateB8;          /* PS1 ActiveStrStreamPtr: StrPlaybackState */
    CdLocation streamStartBC, streamCurrentC0;
    s32 streamFrameC4;
    u32 streamFrameLimitC8, streamFrameBudgetCC;
    s32 streamStopD0;
    s32 overlayEnabledD4;         /* PS1 SceneOverlayEnabled */
    s32 overlayStyleD8, overlayModeDC, overlayBlendE0;
    u8 overlayColorE4[4];
    struct RaceStandingsTable *standings; /* PS1 RaceStandings (+0xE8) */
    SceneCameraState160 cameraEC;
    SceneCameraState160 *camera24C;       /* PS1 ActiveSparkEmitterCar */
    /* Presentation controller. The PS1 kept a pointer to it at +0x3D8
     * (MotionController), set by InitMotionController; it never selected
     * anything else, and stores made before that init went through a null
     * pointer into low RAM. The host addresses the member directly. */
    PresentationController presentation250;
    u32 controllerSlot3D8;
    s32 overlayToggle3DC;         /* PS1 TitleSceneOverlayEnabled */
    /* PS1 FadeSpriteRaceContext; scale3E8 = FadeSpriteIntensityScale,
     * f3EC = FadeSpritePhaseAngle, f428 = DemoFadeSkipEnabled. */
    DemoFadeState fade3E0;
    /* Mode-specific work block: the title intro clears all 0x78 bytes. */
    union {
        SceneIntroWork78 intro;
        EpilogueCameraWork78 epilogue;
    } work444;
    AttractDemoState80 attract4BC; /* PS1 AttractDemoStateBuffer */
    Grid53C *startGrid53C;        /* PS1 TrackStartNode */
    RaceSpecEntry30 *carSpecs540; /* PS1 CarSpecTable */
    RaceSpecEntry30 *draftSpec544; /* PS1 ActiveDraftCarSpecPtr */
    RaceSpecEntry30 *draftSpec548;
    TrackLaneOffset8 *laneOffsets54C; /* PS1 TrackLaneOffsets */
    struct CamNode44 *cameraNodes550; /* PS1 TrackCameraNodes */
    u8 *workspace554;             /* PS1 CameraTrackTriggerTable */
    struct TrackLightPreset8 *lightPresets558; /* PS1 TrackLightPresets */
    u8 *workspace55C;
    /* Decoded garage-floor mesh table (host table built by RelocateOffsetTable). */
    const RroModelTable *model560;
    /* Decoded mesh table of the track audio/results resource (RelocateOffsetTable);
     * PS1 TrackSoundPayloadPtr, also the presentation ring model. */
    const RroModelTable *soundPayload564;
    TrackEventDirectory *lookupTables; /* PS1 TrackEventTables */
    /* PS1 PendingTrackSfxVabId (+0x56C) .. ComSoundBankIndex (+0x578). */
    RaceSoundConfig sound;
} SceneRaceContext;

RR_ASSERT_SIZE(DemoFadeState, 0x64);
RR_ASSERT_OFFSET(DemoFadeState, f410, 0x30);
RR_ASSERT_SIZE(SceneCameraPose18, 0x18);
RR_ASSERT_OFFSET(SceneCameraPose18, rotation10, 0x10);
RR_ASSERT_SIZE(SceneCameraView24, 0x24);
RR_ASSERT_OFFSET(SceneCameraView24, projection20, 0x20);
RR_ASSERT_SIZE(SceneCameraKeyframe2C, 0x2C);
RR_ASSERT_OFFSET(SceneCameraKeyframe2C, time24, 0x24);
RR_ASSERT_SIZE(SceneCameraTrack24, 0x420);
RR_ASSERT_OFFSET(SceneCameraState160, parts18, 0x18);
RR_ASSERT_OFFSET(SceneCameraState160, lighting78, 0x78);
RR_ASSERT_OFFSET(SceneCameraState160, extentA0, 0xA0);
RR_ASSERT_OFFSET(SceneCameraState160, offsetAC, 0xAC);
RR_ASSERT_OFFSET(SceneCameraState160, distance100, 0x100);
RR_ASSERT_OFFSET(SceneCameraState160, position110, 0x110);
RR_ASSERT_OFFSET(SceneCameraState160, rotation120, 0x120);
RR_ASSERT_OFFSET(SceneCameraState160, tracks138, 0x138);
RR_ASSERT_OFFSET(SceneRaceContext, playerCount14, 0x14);
RR_ASSERT_OFFSET(SceneRaceContext, standings, RR_ALIGN_UP(RR_FIELD_OFFSET(SceneRaceContext, overlayEnabledD4) + 0x14, _Alignof(void *)));
RR_ASSERT_OFFSET(SceneRaceContext, cameraNodeChanged1C, 0x1C);
RR_ASSERT_OFFSET(SceneRaceContext, cameraFollowPosition20, 0x20);
RR_ASSERT_OFFSET(SceneRaceContext, cameraFollowOffset30, 0x30);
RR_ASSERT_OFFSET(SceneRaceContext, cameraDistanceSpan38, 0x38);
RR_ASSERT_OFFSET(SceneRaceContext, cameraNodeIndex4C, 0x4C);
RR_ASSERT_OFFSET(SceneRaceContext, view60, 0x50 + 4 * sizeof(struct GpuDmaTag *));
RR_ASSERT_OFFSET(SceneRaceContext, comCarCount10, 0x10);
RR_ASSERT_OFFSET(SceneRaceContext, opening88, RR_FIELD_OFFSET(SceneRaceContext, view60) + sizeof(SceneCameraView24) + 4);
RR_ASSERT_OFFSET(SceneRaceContext, streamResourceB0, RR_FIELD_OFFSET(SceneRaceContext, opening88) + sizeof(SceneOpeningState20) + 8);
RR_ASSERT_OFFSET(SceneRaceContext, overlayEnabledD4, RR_FIELD_OFFSET(SceneRaceContext, streamResourceB0) + 0x20 + sizeof(void *));
RR_ASSERT_SIZE(SceneOpeningState20, 0x20);
RR_ASSERT_SIZE(SceneIntroWork78, 0x78);
RR_ASSERT_SIZE(EpilogueCameraWork78, 0x78);
RR_ASSERT_OFFSET(EpilogueCameraWork78, position450, 0x450 - 0x444);
RR_ASSERT_OFFSET(EpilogueCameraWork78, target468, 0x468 - 0x444);
RR_ASSERT_OFFSET(EpilogueCameraWork78, node498, 0x498 - 0x444);
RR_ASSERT_OFFSET(EpilogueCameraWork78, counter4B4, 0x4B4 - 0x444);
#ifdef __mips__
RR_ASSERT_SIZE(SceneCameraState160, 0x160);
RR_ASSERT_OFFSET(SceneCameraState160, mode13C, 0x13C);
RR_ASSERT_OFFSET(SceneRaceContext, cameraEC, 0xEC);
RR_ASSERT_SIZE(SceneRaceContext, 0x57C);
RR_ASSERT_OFFSET(SceneRaceContext, camera24C, 0x24C);
RR_ASSERT_OFFSET(SceneRaceContext, presentation250, 0x250);
RR_ASSERT_OFFSET(SceneRaceContext, controllerSlot3D8, 0x3D8);
RR_ASSERT_OFFSET(SceneRaceContext, overlayToggle3DC, 0x3DC);
RR_ASSERT_OFFSET(SceneRaceContext, fade3E0, 0x3E0);
RR_ASSERT_OFFSET(SceneRaceContext, work444, 0x444);
RR_ASSERT_OFFSET(SceneRaceContext, attract4BC, 0x4BC);
RR_ASSERT_OFFSET(SceneRaceContext, startGrid53C, 0x53C);
RR_ASSERT_OFFSET(SceneRaceContext, carSpecs540, 0x540);
RR_ASSERT_OFFSET(SceneRaceContext, draftSpec544, 0x544);
RR_ASSERT_OFFSET(SceneRaceContext, laneOffsets54C, 0x54C);
RR_ASSERT_OFFSET(SceneRaceContext, cameraNodes550, 0x550);
RR_ASSERT_OFFSET(SceneRaceContext, workspace554, 0x554);
RR_ASSERT_OFFSET(SceneRaceContext, lightPresets558, 0x558);
RR_ASSERT_OFFSET(SceneRaceContext, workspace55C, 0x55C);
RR_ASSERT_OFFSET(SceneRaceContext, model560, 0x560);
RR_ASSERT_OFFSET(SceneRaceContext, soundPayload564, 0x564);
RR_ASSERT_OFFSET(SceneRaceContext, lookupTables, 0x568);
RR_ASSERT_OFFSET(SceneRaceContext, sound, 0x56C);
#endif

#endif
