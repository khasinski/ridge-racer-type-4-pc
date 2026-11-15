#ifndef RR_CAMERA_MOTION_H
#define RR_CAMERA_MOTION_H
#include "rr/presentation.h"
#include "game/camera/cammove.h"
/* Initial key08 selects an accessible key; the cyclic mode uses three keys.
 * Input, readonly keys and mutable camera storage are disjoint. Active setup
 * requires a nonzero wrapped signed divisor and excludes INT_MIN/-1. Every
 * word product wraps before division. The horizon uses unsigned division;
 * counter comparison narrows through signed16. Modes0/1 retain their distinct
 * stepping and key-wrap rules. The object pointer is captured before updates. */
/* Three leading words are padded before the native key pointer. */
#define RR_CAMERA_KEYS_OFFSET RR_ALIGN_UP(12, _Alignof(CameraKey16 *))
RR_ASSERT_SIZE(CameraKey16,0x10);
RR_ASSERT_SIZE(CameraMotion70, RR_ALIGN_UP(RR_CAMERA_KEYS_OFFSET + sizeof(CameraKey16 *) + 0x60, _Alignof(CameraMotion70)));
RR_ASSERT_OFFSET(CameraMotion70, keys0C, RR_CAMERA_KEYS_OFFSET);
RR_ASSERT_OFFSET(CameraMotion70, current40, RR_CAMERA_KEYS_OFFSET + sizeof(CameraKey16 *) + 48);
RR_ASSERT_OFFSET(CameraMotion70, counter50, RR_CAMERA_KEYS_OFFSET + sizeof(CameraKey16 *) + 64);
RR_ASSERT_OFFSET(CameraMotion70, horizon52, RR_CAMERA_KEYS_OFFSET + sizeof(CameraKey16 *) + 66);
RR_ASSERT_OFFSET(CameraMotion70, start10, RR_CAMERA_KEYS_OFFSET + sizeof(CameraKey16 *) + 0);
RR_ASSERT_OFFSET(CameraMotion70, angles64, RR_CAMERA_KEYS_OFFSET + sizeof(CameraKey16 *) + 84);
RR_ASSERT_OFFSET(CameraMotion70, modelPosition54, RR_CAMERA_KEYS_OFFSET + sizeof(CameraKey16 *) + 68);
RR_ASSERT_OFFSET(CameraMotion70, modelScale6C, RR_CAMERA_KEYS_OFFSET + sizeof(CameraKey16 *) + 92);
RR_ASSERT_OFFSET(PresentationController, motion74, RR_ALIGN_UP(RR_FIELD_OFFSET(PresentationController, scriptArgument70) + 4, _Alignof(CameraMotion70)));
RR_ASSERT_SIZE(CameraScriptEntry8,8);
RR_ASSERT_OFFSET(PresentationController, script64, RR_ALIGN_UP(0x64, _Alignof(CameraScriptEntry8 *)));
RR_ASSERT_OFFSET(PresentationController, scriptArgument70, RR_FIELD_OFFSET(PresentationController, script64) + sizeof(CameraScriptEntry8 *) + 8);
#endif
