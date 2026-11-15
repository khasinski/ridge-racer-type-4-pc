#ifndef RR_GPU_H
#define RR_GPU_H
#include <stdint.h>

#include "rr/layout.h"

/* Configured GPU data and status/control register addresses. */
extern volatile u32 *GPU_DATA, *GPU_STATUS;
s32 GPU_CheckVersion(s32 mode);

/* GPU DMA channel and queue timeout state. Queue tail is interrupt-visible. */
extern volatile u32 *DMA2_MADR, *DMA2_BCR, *DMA2_CHCR;
extern s32 GpuResetSavedIntrMask, GpuTimeoutDeadline, GpuAlarmCount;
extern volatile s32 _qin, _qout;

/* Drawing limits and packed E3/E4 drawing-area coordinate commands. */
extern s16 D_800ABAC4, D_800ABAC6;
u32 get_cs(s32 x, s32 y);
u32 get_ce(s32 x, s32 y);

/* Ordering-table DMA on channel 6. Address/count arithmetic wraps at 32 bits on PAL.
 * The supplied address is recorded after adding signed32(count*4-4). */
extern u8 * volatile *DMA6_MADR;
extern volatile u32 *DMA6_BCR, *DMA6_CHCR, *DPCR;
s32 _otc(u8 *buffer, s32 count);
void set_alarm(void);
s32 get_alarm(void);

/* RECT has both signed-coordinate and packed-word views on little-endian PAL. */
typedef struct { s16 x,y,w,h; } GpuRectangleFields;
typedef union GpuRectangle {
    GpuRectangleFields rect;
    u32 words[2];
} GpuRectangle;

typedef struct {
    u32 position, destination, size;
} GpuMovePayload12;

/* The DMA packet header occupies the eight bytes preceding this payload. */
extern GpuMovePayload12 GpuMovePayload;
void checkRECT(const char *name, GpuRectangle *rectangle);
s32 MoveImage2(GpuRectangle *rectangle, s32 x, s32 y);
/* Uploads ceil(width*height/2) packed pixel words, using a CPU tail and DMA
 * blocks of16 words. Dimensions clamp against the signed configured limits
 * (inclusive), storing their low16 bits. Returns-1 for a nonpositive word
 * count or timeout, otherwise0. The source must cover the complete transfer. */
s32 _dws(GpuRectangle *rectangle, const u32 *pixels);
/* Reverse transfer with the same dimension and buffer-capacity contract.
 * Polls command readiness, requests image data, then polls receive readiness
 * before reading the CPU tail and starting DMA. Returns -1 on either timeout
 * or a nonpositive word count, otherwise 0. */
s32 _drs(GpuRectangle *rectangle, u32 *pixels);
/* In-place dimensions clamp against signed limit-1. Aligned X/width use the
 * fast GPU fill; other rectangles use a linked packet that restores draw state.
 * The color's high bit contributes bit10 of the drawing-mode command. */
s32 _clr(GpuRectangle *rectangle, u32 color);
/* Query a GPU state selector and return its low24 bits. */
s32 _param(s32 selector);
RR_ASSERT_SIZE(GpuRectangle, 8);
RR_ASSERT_OFFSET(GpuRectangleFields, w, 4);
RR_ASSERT_OFFSET(GpuRectangleFields, h, 6);
RR_ASSERT_SIZE(GpuMovePayload12, 12);
RR_ASSERT_OFFSET(GpuMovePayload12, destination, 4);
RR_ASSERT_OFFSET(GpuMovePayload12, size, 8);

/* Host GPU packet tag: the PsyZ P_TAG/OT_TYPE header (libgpu.h, __psyz).
 * address links the next packet (a host pointer, or RR_GPU_END_LINK), length
 * counts the payload words that follow the header. Ordering-table entries
 * are bare tags. PS1 code that packed "len<<24 | addr&0xFFFFFF" into one
 * word uses these fields; payload offsets start after the 16-byte header. */
typedef struct GpuDmaTag {
    uintptr_t address;
    uintptr_t length;
} GpuDmaTag;
/* Allocation stride includes the native DMA header and tail alignment.
 * Packet lengths still count only the payload's 32-bit words. */
#define RR_GPU_PACKET_SIZE(payload_bytes) \
    RR_ALIGN_UP(sizeof(GpuDmaTag) + (payload_bytes), _Alignof(GpuDmaTag))
#define RR_GPU_END_LINK ((uintptr_t)0xFFFFFF)
/* PsyZ reads a packet whose first payload word is not a drawing primitive
 * (code 0x20..0x7F) as u_long words unless one payload word is this GP0 NOP
 * (PSYZ_MERGED_PRIM_NOP). Packets that place E1..E6 commands before a
 * primitive carry it right after those commands and count it in length. */
#define RR_GPU_MERGED_PRIM_NOP 0x005A4D50U
#define RR_GPU_TAG(p) ((GpuDmaTag *)(p))
/* Insert packet p (n payload words) at the head of ordering entry ot. */
#define RR_GPU_ADD_PRIM(ot, p, n) do { GpuDmaTag *rr_ot_ = (GpuDmaTag *)(ot); GpuDmaTag *rr_p_ = (GpuDmaTag *)(p); \
    rr_p_->length = (n); rr_p_->address = rr_ot_->address; rr_ot_->address = (uintptr_t)rr_p_; } while (0)
/* Link packet p to follow at the head of ot keeping p's length. */
#define RR_GPU_LINK(ot, p) do { GpuDmaTag *rr_ot_ = (GpuDmaTag *)(ot); GpuDmaTag *rr_p_ = (GpuDmaTag *)(p); \
    rr_p_->address = rr_ot_->address; rr_ot_->address = (uintptr_t)rr_p_; } while (0)

/* GPU sprite packets with a packet tag.
 * CLUT signedness differs between original units; preserve both views. */

typedef struct SpritePacket {
    GpuDmaTag tag;
    u8 r, g, b, code;
    s16 x, y;
    u8 u, v;
    u16 clut;
    s16 w, h;
} SpritePacket;

typedef struct SpritePacketSignedClut {
    GpuDmaTag tag;
    u8 r, g, b, code;
    s16 x, y;
    u8 u, v;
    s16 clut, w, h;
} SpritePacketSignedClut;

RR_ASSERT_SIZE(SpritePacket, RR_GPU_PACKET_SIZE(16));
RR_ASSERT_OFFSET(SpritePacket, r, sizeof(GpuDmaTag));
RR_ASSERT_OFFSET(SpritePacket, x, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(SpritePacket, clut, sizeof(GpuDmaTag) + 10);
RR_ASSERT_OFFSET(SpritePacket, w, sizeof(GpuDmaTag) + 12);
RR_ASSERT_SIZE(SpritePacketSignedClut, RR_GPU_PACKET_SIZE(16));
RR_ASSERT_OFFSET(SpritePacketSignedClut, r, sizeof(GpuDmaTag));
RR_ASSERT_OFFSET(SpritePacketSignedClut, x, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(SpritePacketSignedClut, clut, sizeof(GpuDmaTag) + 10);
RR_ASSERT_OFFSET(SpritePacketSignedClut, w, sizeof(GpuDmaTag) + 12);

/* RGB bytes and command byte shared by CPU templates and GTE output words. */
typedef struct {u8 r,g,b,code;} GpuColorCommand;
typedef union {u32 word;GpuColorCommand bytes;} GpuColorWord;
/* Shared eight-byte prefix of GPU drawing primitives. Setters need only this
 * real header; coordinates and texture data belong to their packet owners. */
typedef struct {
    GpuDmaTag tag;
    GpuColorWord color;
} GpuPrimitiveHeader;
RR_ASSERT_SIZE(GpuPrimitiveHeader, RR_GPU_PACKET_SIZE(4));
RR_ASSERT_OFFSET(GpuPrimitiveHeader, color, sizeof(GpuDmaTag));

RR_ASSERT_SIZE(GpuColorCommand, 4);
RR_ASSERT_OFFSET(GpuColorCommand, code, 3);
RR_ASSERT_SIZE(GpuColorWord, 4);

typedef struct {
    GpuColorWord color0; s32 xy0; u32 uv0;
    GpuColorWord color1; s32 xy1; u32 uv1;
    GpuColorWord color2; s32 xy2; u32 uv2;
    GpuColorWord color3; s32 xy3; u32 uv3;
} GpuGT4Body;
/* Environment-mapped quad output. A masked packet is E6 mask + GT4 (13
 * words); a direct packet is the bare GT4 (12 words). The PS1 overlapped the
 * direct header with the masked E6 word; host tags are 16 bytes, so the
 * emitter stages the masked layout and compacts it into the direct one. */
typedef struct {
    GpuDmaTag tag;
    GpuGT4Body body;
} GpuEnvMapDirect;
typedef struct {
    GpuDmaTag tag;
    u32 setMask;
    u32 nop; /* RR_GPU_MERGED_PRIM_NOP: 14 packed payload words */
    GpuGT4Body body;
} GpuEnvMapMasked;
/* The reflected coordinates are staged even for direct rendering, so a
 * caller provides the whole emission footprint for each quad. */
typedef struct {
    GpuEnvMapMasked primary, reflected;
    struct {GpuDmaTag tag; uintptr_t command;} restore; /* DR word: native width */
} GpuEnvMapEmission;

/* Flat textured triangle: command, three XY/UV pairs, CLUT and texture page. */
typedef struct {
    GpuDmaTag tag;
    u8 r, g, b, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    s16 x2, y2;
    u8 u2, v2;
    u16 reserved1E;
} GpuPolyFT3Packet;
RR_ASSERT_SIZE(GpuPolyFT3Packet, RR_GPU_PACKET_SIZE(28));
RR_ASSERT_OFFSET(GpuPolyFT3Packet, x0, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(GpuPolyFT3Packet, u0, sizeof(GpuDmaTag) + 8);
RR_ASSERT_OFFSET(GpuPolyFT3Packet, clut, sizeof(GpuDmaTag) + 10);
RR_ASSERT_OFFSET(GpuPolyFT3Packet, tpage, sizeof(GpuDmaTag) + 18);
RR_ASSERT_OFFSET(GpuPolyFT3Packet, x2, sizeof(GpuDmaTag) + 20);
RR_ASSERT_OFFSET(GpuPolyFT3Packet, reserved1E, sizeof(GpuDmaTag) + 26);

/* DR_* command packets (E1..E6 words): PsyZ reads their payload as native
 * u_long words (DR_MODE/DR_TPAGE/DR_AREA layout), unlike shape primitives,
 * whose payload stays packed 32-bit. Each command occupies a uintptr_t. */
typedef struct {GpuDmaTag tag;uintptr_t command[2];} GpuDrawModePacket;
typedef struct {GpuDmaTag tag;uintptr_t command;} GpuCommandPacket;
typedef struct {
    GpuDmaTag tag;
    s32 color0, xy0, color1, xy1, color2, xy2;
} GpuPolyG3;
/* One DMA chain entry: E6 mask command, untextured Gouraud quad, E6 restore.
 * Color words include the primitive command byte produced by the GTE. The
 * merged-primitive NOP after the E6 word keeps the payload packed (11 words). */
typedef struct {
    GpuDmaTag tag;
    u32 setMask04;
    u32 nop;
    s32 color08, xy0C, color10, xy14, color18, xy1C, color20, xy24;
    u32 restoreMask28;
} GpuMaskedG4Packet;
RR_ASSERT_SIZE(GpuMaskedG4Packet, RR_GPU_PACKET_SIZE(44));
RR_ASSERT_OFFSET(GpuMaskedG4Packet, color08, sizeof(GpuDmaTag) + 8);
RR_ASSERT_OFFSET(GpuMaskedG4Packet, color20, sizeof(GpuDmaTag) + 32);
RR_ASSERT_OFFSET(GpuMaskedG4Packet, restoreMask28, sizeof(GpuDmaTag) + 40);
RR_ASSERT_SIZE(GpuCommandPacket, sizeof(GpuDmaTag) + sizeof(uintptr_t));
RR_ASSERT_SIZE(GpuPolyG3, RR_GPU_PACKET_SIZE(24));
RR_ASSERT_OFFSET(GpuPolyG3, xy0, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(GpuPolyG3, xy2, sizeof(GpuDmaTag) + 20);
RR_ASSERT_SIZE(GpuDrawModePacket, sizeof(GpuDmaTag) + 2 * sizeof(uintptr_t));
RR_ASSERT_OFFSET(GpuDrawModePacket, command, sizeof(GpuDmaTag));
typedef struct {
    GpuDmaTag tag;
    u8 r,g,b,code;
    s16 x,y,w,h;
} GpuTilePacket;
RR_ASSERT_SIZE(GpuDmaTag, 2 * sizeof(uintptr_t));
RR_ASSERT_SIZE(GpuTilePacket, RR_GPU_PACKET_SIZE(12));
RR_ASSERT_OFFSET(GpuTilePacket, r, sizeof(GpuDmaTag));
RR_ASSERT_OFFSET(GpuTilePacket, x, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(GpuTilePacket, w, sizeof(GpuDmaTag) + 8);

/* Whole sprite/tile owners expose the same packet through packed GPU words
 * and the coordinate/color fields used by CPU-side primitive builders. */
typedef struct {
    GpuDmaTag tag;
    GpuColorWord color;
    u32 position, uv, size;
} GpuSpriteCommandWords;
typedef union {SpritePacket fields;GpuSpriteCommandWords words;GpuPrimitiveHeader header;} GpuSpriteCommand;
typedef struct {
    GpuDmaTag tag;
    GpuColorWord color;
    u32 position, size;
} GpuTileCommandWords;
typedef union {GpuTilePacket fields;GpuTileCommandWords words;GpuPrimitiveHeader header;} GpuTileCommand;
RR_ASSERT_SIZE(GpuSpriteCommand, RR_GPU_PACKET_SIZE(16));
RR_ASSERT_SIZE(GpuSpriteCommandWords, RR_GPU_PACKET_SIZE(16));
RR_ASSERT_OFFSET(GpuSpriteCommand,words,0);
RR_ASSERT_OFFSET(GpuSpriteCommandWords, position, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(GpuSpriteCommandWords, uv, sizeof(GpuDmaTag) + 8);
RR_ASSERT_OFFSET(GpuSpriteCommandWords, size, sizeof(GpuDmaTag) + 12);
RR_ASSERT_SIZE(GpuTileCommand, RR_GPU_PACKET_SIZE(12));
RR_ASSERT_SIZE(GpuTileCommandWords, RR_GPU_PACKET_SIZE(12));
RR_ASSERT_OFFSET(GpuTileCommand,words,0);
RR_ASSERT_OFFSET(GpuTileCommand,header,0);
RR_ASSERT_OFFSET(GpuSpriteCommand,header,0);
RR_ASSERT_OFFSET(GpuTileCommandWords, position, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(GpuTileCommandWords, size, sizeof(GpuDmaTag) + 8);

/* Flat triangle: three packed signed16 X/Y pairs in wire words. */
typedef struct {
    GpuDmaTag tag;
    u8 r, g, b, code;
    s32 xy0, xy1, xy2;
} PolyF3Packet;
RR_ASSERT_SIZE(PolyF3Packet, RR_GPU_PACKET_SIZE(16));
RR_ASSERT_OFFSET(PolyF3Packet, xy0, sizeof(GpuDmaTag) + 4);

/* Textured quad and four-vertex polyline DMA packets. */
typedef struct {
    GpuDmaTag tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad1;
    s16 x3, y3;
    u8 u3, v3;
    u16 pad2;
} GpuPolyFT4Packet;
/* Packed word writes and byte command/tag updates address the same packet. */
typedef union {
    GpuPolyFT4Packet fields;
    struct {
        GpuDmaTag tag;
        s32 color04;
        s32 xy0, uv0, xy1, uv1, xy2, uv2, xy3, uv3;
    } words;
} GpuPolyFT4;
typedef struct {
    GpuDmaTag tag;
    u8 r, g, b, code;
    s16 x0, y0, x1, y1, x2, y2, x3, y3;
    u32 pad;
} GpuLineF4Packet;
RR_ASSERT_SIZE(GpuPolyFT4Packet, RR_GPU_PACKET_SIZE(36));
RR_ASSERT_SIZE(GpuPolyFT4, RR_GPU_PACKET_SIZE(36));
RR_ASSERT_OFFSET(GpuPolyFT4Packet, x0, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(GpuPolyFT4Packet, u0, sizeof(GpuDmaTag) + 8);
RR_ASSERT_OFFSET(GpuPolyFT4Packet, tpage, sizeof(GpuDmaTag) + 18);
RR_ASSERT_OFFSET(GpuPolyFT4Packet, x3, sizeof(GpuDmaTag) + 28);
RR_ASSERT_SIZE(GpuLineF4Packet, RR_GPU_PACKET_SIZE(24));
RR_ASSERT_OFFSET(GpuLineF4Packet, x0, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(GpuLineF4Packet, pad, sizeof(GpuDmaTag) + 20);

/* Quad primitive GPU packets (POLY_F4, POLY_G4, POLY_GT4) and mesh face records. */
typedef struct {
    GpuDmaTag tag;
    s32 colorCode04;
    s32 xy0, xy1, xy2, xy3;
} PolyF4Packet;

typedef struct {
    GpuDmaTag tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 r1, g1, b1, pad0F;
    s16 x1, y1;
    u8 r2, g2, b2, pad17;
    s16 x2, y2;
    u8 r3, g3, b3, pad1F;
    s16 x3, y3;
} PolyG4Packet;

typedef struct {
    GpuDmaTag tag;
    s32 color04, xy0, color0C, xy1, color14, xy2, color1C, xy3;
} PolyG4WordPacket;

/* The byte fields, packed color/XY writes and DMA header share one G4 packet. */
typedef union {
    PolyG4Packet fields;
    PolyG4WordPacket words;
    GpuDmaTag tag;
} GpuPolyG4;
RR_ASSERT_SIZE(GpuPolyG4, RR_GPU_PACKET_SIZE(32));
RR_ASSERT_OFFSET(PolyG4WordPacket, xy3, sizeof(GpuDmaTag) + 28);

typedef struct {
    GpuDmaTag tag;
    s32 color04, xy0, uvClut0C, color10, xy1, uvTpage18, color1C, xy2;
    u16 uv2;
    u8 pad26[2];
    s32 color28, xy3;
    u16 uv3;
    u8 pad32[2];
} PolyGT4WordPacket;

typedef struct {
    u16 v0, v1, v2, v3;
    s32 color08;
    u8 pad0C, otBias0D, pad0E[2];
} MeshQuadF4;

typedef struct {
    u16 v0, v1, v2, v3;
    s32 color08, color0C, color10, color14;
    u8 pad18, otBias19, pad1A[2];
} MeshQuadG4;

typedef struct {
    u16 v0, v1, v2, v3;
    s32 color08, color0C, color10, color14, uvClut18, uvTpage1C;
    u16 uv2, uv3;
    u8 pad24, otBias25, pad26[2];
} MeshQuadGT4;

RR_ASSERT_SIZE(PolyF4Packet, RR_GPU_PACKET_SIZE(20));
RR_ASSERT_SIZE(PolyG4Packet, RR_GPU_PACKET_SIZE(32));
RR_ASSERT_OFFSET(PolyG4Packet, x0, sizeof(GpuDmaTag) + 4);
RR_ASSERT_OFFSET(PolyG4Packet, x1, sizeof(GpuDmaTag) + 12);
RR_ASSERT_OFFSET(PolyG4Packet, x2, sizeof(GpuDmaTag) + 20);
RR_ASSERT_OFFSET(PolyG4Packet, x3, sizeof(GpuDmaTag) + 28);
RR_ASSERT_SIZE(PolyG4WordPacket, RR_GPU_PACKET_SIZE(32));
RR_ASSERT_SIZE(PolyGT4WordPacket, RR_GPU_PACKET_SIZE(48));
RR_ASSERT_OFFSET(PolyGT4WordPacket, uv2, sizeof(GpuDmaTag) + 32);
RR_ASSERT_OFFSET(PolyGT4WordPacket, uv3, sizeof(GpuDmaTag) + 44);
RR_ASSERT_SIZE(MeshQuadF4, 0x10);
RR_ASSERT_SIZE(MeshQuadG4, 0x1C);
RR_ASSERT_SIZE(MeshQuadGT4, 0x28);

#endif
