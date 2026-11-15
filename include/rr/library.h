#ifndef RR_LIBRARY_H
#define RR_LIBRARY_H

#include "rr/layout.h"
#include "rr/spu.h"
#include "rr/sequence_channel.h"

typedef struct {
    u32 value;
} RR_PACKED LibPackedWord;

/* Partial layout of the 0x20-byte entries referenced by StRingHeaders. */
typedef struct {
    u16 state;
    u16 type02, sector04, sectors06;
    s32 value08;
    u32 frameBytes0C;
    u16 width10,height12;
    u8 pad14[8];
    LibPackedWord word1C;
} LibQueueRecord32;

extern LibQueueRecord32 *StRingHeaders;
void StSetRing(LibQueueRecord32 *first, s32 second);
void StSetStream(s32 a, s32 b, s32 c, s32 d, s32 e);
extern s32 StRingReadIndex, StEndFrame, StRingSize;
/* Buffer must contain the configured header table and its 0x7E0-byte payload slots. */
s32 StGetNext(u8 **data, LibQueueRecord32 **header);

RR_ASSERT_SIZE(LibPackedWord, 4);
RR_ASSERT_SIZE(LibQueueRecord32, 0x20);
RR_ASSERT_OFFSET(LibQueueRecord32, sectors06, 6);
RR_ASSERT_OFFSET(LibQueueRecord32, value08, 8);
RR_ASSERT_OFFSET(LibQueueRecord32, frameBytes0C, 0xC);
RR_ASSERT_OFFSET(LibQueueRecord32, width10, 0x10);
RR_ASSERT_OFFSET(LibQueueRecord32, height12, 0x12);
RR_ASSERT_OFFSET(LibQueueRecord32, word1C, 0x1C);

/* Prefix at D_800ABAC2. Width/height also have assembler aliases
 * D_800ABAC4 and D_800ABAC6, used by drawing-area coordinate helpers. */
typedef struct {
    u8 level,mode;
    s16 width,height;
} GpuDebugSettings;
extern GpuDebugSettings D_800ABAC2;
extern s32 (*GPU_printf)(const char *,...);
RR_ASSERT_SIZE(GpuDebugSettings, 6);
RR_ASSERT_OFFSET(GpuDebugSettings, width, 2);
RR_ASSERT_OFFSET(GpuDebugSettings, height, 4);

/* Resident GPU driver: the executable's sixteen pointer-sized cells.
 * Queue callbacks receive two native-width words, as in PsyZ addque2.
 * Payload and parameter transport retain complete native pointer widths. */
typedef int (*GpuOpFunc)(uintptr_t, uintptr_t);
typedef void (*LibDrawOrderingTable)(uintptr_t address);
union GpuRectangle;
typedef struct GpuDriver40 {
    const char *name00;
    void (*enqueue04)(GpuOpFunc, void *, uintptr_t);
    s32 (*dispatch)(GpuOpFunc, void *, s32, uintptr_t);
    s32 (*clear0C)(union GpuRectangle *, u32);
    void (*control10)(u32);
    s32 (*writeBuffer14)(int *, s32);
    LibDrawOrderingTable draw_ordering_table;
    s32 (*download1C)(union GpuRectangle *, u32 *);
    s32 (*upload20)(union GpuRectangle *, const u32 *);
    s32 (*execute24)(void);
    s32 (*readControl28)(s32);
    s32 (*clearOrdering2C)(u8 *, s32);
    s32 (*parameter30)(s32);
    s32 (*reset34)(s32);
    u32 (*status38)(void);
    s32 (*sync3C)(s32);
} GpuDriver40;
/* Compatibility spelling; this is the complete owner, not a separate prefix. */
typedef GpuDriver40 LibGraphicsDriverPrefix;
#if defined(mips)
RR_ASSERT_SIZE(GpuDriver40, 0x40);
RR_ASSERT_OFFSET(GpuDriver40, dispatch, 8);
RR_ASSERT_OFFSET(GpuDriver40, draw_ordering_table, 0x18);
RR_ASSERT_OFFSET(GpuDriver40, reset34, 0x34);
#endif

typedef void (*LibInterruptCallback)(void);
typedef LibInterruptCallback (*LibInterruptExchange)(s32, LibInterruptCallback);

/* DMA callback table has eight slots (initialization clears all eight).
 * Registration has no bounds check; channel must be in 0..7. */
extern LibInterruptCallback InterruptCallbackTable[8];
extern volatile u32 *DICR_VSync;
LibInterruptCallback setIntrDMA(s32 channel, LibInterruptCallback callback);
LibInterruptExchange startIntrDMA(void);
void trapIntrDMA(void);
void DMA_memclr(LibInterruptCallback *callbacks, s32 count);

/* Prefix of the callback driver referenced by IntrDriverTablePtr. */
typedef struct {
    LibInterruptCallback reserved;
    LibInterruptExchange exchange;
    LibInterruptExchange install;
    void (*reset)(void);
} LibInterruptDriverPrefix;

extern LibInterruptDriverPrefix *IntrDriverTablePtr;
extern void ResetCallback(void);
LibInterruptCallback DMACallback(s32 channel, LibInterruptCallback callback);
LibInterruptCallback _SpuDataCallback(LibInterruptCallback callback);
LibInterruptCallback InterruptCallback(s32 channel, LibInterruptCallback callback);

#if defined(mips)
RR_ASSERT_SIZE(LibInterruptDriverPrefix, 0x10);
RR_ASSERT_OFFSET(LibInterruptDriverPrefix, exchange, 4);
RR_ASSERT_OFFSET(LibInterruptDriverPrefix, install, 8);
RR_ASSERT_OFFSET(LibInterruptDriverPrefix, reset, 0xC);
#endif

typedef void (*LibDataCallback)(s32, u8 *);
void CdCommandCallback(void);

typedef void (*LibCompletionCallback)(void);
typedef void (*LibNotificationCallback)(s32, s32);

extern LibDataCallback CD_cbsync, CD_cbready;
extern LibNotificationCallback CD_cbread;
extern u32 CD_read_dma_mode;
s32 CdInit(void);
void def_cbsync(s32 selector, u8 *data);
void def_cbready(s32 selector, u8 *data);
void def_cbread(s32 event, s32 argument);

/* Partial state updated by the multi-step library transfer callback. */
typedef struct {
    u8 pad00[8];
    u32 position08;
    u8 pad0C[4];
    u32 step10, remaining14;
    u8 pad18[8];
    u32 completed20, finished24;
    u8 pad28[4];
    LibCompletionCallback restore2C, restore30;
    s32 notify_arg34;
} LibTransferState38;

#if defined(mips)
RR_ASSERT_SIZE(LibTransferState38, 0x38);
RR_ASSERT_OFFSET(LibTransferState38, position08, 8);
RR_ASSERT_OFFSET(LibTransferState38, step10, 0x10);
RR_ASSERT_OFFSET(LibTransferState38, remaining14, 0x14);
RR_ASSERT_OFFSET(LibTransferState38, completed20, 0x20);
RR_ASSERT_OFFSET(LibTransferState38, finished24, 0x24);
RR_ASSERT_OFFSET(LibTransferState38, restore2C, 0x2C);
RR_ASSERT_OFFSET(LibTransferState38, restore30, 0x30);
RR_ASSERT_OFFSET(LibTransferState38, notify_arg34, 0x34);
#endif

/* Named-entry table beginning at D_80110954. A zero key terminates the scan. */
typedef struct {
    s32 key;
    s32 value;
    char name[0x24];
} LibNamedEntry;

RR_ASSERT_SIZE(LibNamedEntry, 0x2C);
RR_ASSERT_OFFSET(LibNamedEntry, name, 8);

/* Prefix of the saved state at IntrDispatchState; callback data begins at 0x38. */
typedef struct {
    u16 active;
    s16 dispatching02;
    LibInterruptCallback callbacks[11];
    u16 enabled_mask;
    u16 register16;
    s32 register32;
    u8 callback_data[4];
} LibSavedState;

/* IRQ selector must be in the callback-table range 0..10. */
LibInterruptCallback setIntr(s32 selector, LibInterruptCallback callback);

#if defined(mips)
RR_ASSERT_SIZE(LibSavedState, 0x3C);
RR_ASSERT_OFFSET(LibSavedState, dispatching02, 2);
RR_ASSERT_OFFSET(LibSavedState, callbacks, 4);
RR_ASSERT_OFFSET(LibSavedState, enabled_mask, 0x30);
RR_ASSERT_OFFSET(LibSavedState, register16, 0x32);
RR_ASSERT_OFFSET(LibSavedState, register32, 0x34);
RR_ASSERT_OFFSET(LibSavedState, callback_data, 0x38);
#endif

/* Bank reset requires a low-halfword bank index in the 32-entry table (0..31). */
void _SsClose(s32 bank);
void SsSeqClose(s32 bank);
void SsSepClose(s32 bank);

/* Callback entry points use full argument registers; selectors narrow inside. */
typedef void (*LibControllerCallback)(s32 bank, s32 row, s32 value);
typedef void (*LibControllerResetCallback)(s32 bank, s32 row);
extern LibControllerCallback SsCtrlHandlerCC6, D_8010EFE4, D_8010EFE8, D_8010EFEC;
extern LibControllerCallback SsCtrlHandlerCC64, SsCtrlHandlerCC98, D_8010EFF8, D_8010EFFC;
extern LibControllerCallback D_8010F000, D_8010F004;
extern LibControllerCallback _SsMarkCallback[][16];
extern LibControllerResetCallback SsCtrlHandlerCC121;
extern void _SsContMainVol(s32 bank, s32 row, s32 value);
void _SsContPanpot(s32 bank, s32 row, s32 value);
extern void _SsContDamper(s32 bank, s32 row, s32 value);
void _SsContExternal(s32 bank, s32 row, s32 value);
void SsUtSetReverbDepth(s32 left, s32 right);
void _SsContNrpn1(s32 bank, s32 row, s32 value);
void _SsContNrpn2(s32 bank, s32 row, s32 value);
void _SsContExpression(s32 bank, s32 row, s32 value);
extern void _SsContRpn1(s32 bank, s32 row, s32 value);
extern void _SsContRpn2(s32 bank, s32 row, s32 value);
extern void _SsContResetAll(s32 bank, s32 row);
extern void _SsSetPitchBend(s32 bank, s32 row);
extern void _SsSetControlChange(s32 bank, s32 row, s32 control);
extern void _SsSetProgramChange(s32 bank, s32 row, s32 value);
extern s32 _SsReadDeltaValue(s32 bank, s32 row);
extern void _SsVmSetVol(s32 channel, s32 device, s32 slot, u32 pitch, u32 value);


s32 SsUtSetReverbType(s32 value);

#endif
