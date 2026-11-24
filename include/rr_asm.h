#ifndef RR_ASM_H
#define RR_ASM_H
#include <stdint.h>
#include <string.h>
#include "types.h"
/* PsyZ GTE register file and command endpoints (psyz/gte.h); declared here
 * so game modules keep their own PsyQ libgte prototypes. */
unsigned int Psyz_GteDataRead(unsigned reg);
int Psyz_GteSetExtendedProjection(int enabled);
void Psyz_GteDataWrite(unsigned reg, unsigned int value);
unsigned int Psyz_GteCtrlRead(unsigned reg);
void Psyz_GteCtrlWrite(unsigned reg, unsigned int value);
void Psyz_GteCommand(unsigned int cmd);
/* Screen XY words carry PsyZ's precise projection of their vertex (subpixel
 * x/y and depth, drawn by the geometry correction option) by the address
 * they are stored at. Every store of SXY0..SXYP to memory, every load of a
 * stored word back into them and every copy of a projected word to another
 * place (a cache, a packet) goes through these helpers; a plain store leaves
 * the vertex snapped to whole pixels. Without geometry correction they are
 * ordinary loads, stores and copies. */
void Psyz_GteStoreSxyTo(unsigned reg, void *dst);
void Psyz_GteLoadSxyFrom(unsigned reg, const void *src);
void Psyz_GteCopyPgxpSxy(const void *src, const void *dst, unsigned int sxy);
/* Precise vertices belong to the game frame that projected them: the main
 * loop starts a new generation before it builds the next ordering table.
 * Library stores: libgte RotTransPers / RotAverage outputs carry them too. */
void Psyz_PgxpNextFrame(void);
void Psyz_GteSetPgxpLibraryStores(int enabled);
/* With geometry correction, NCLIP (back-face and sliver culling) takes the
 * facing of the precise vertices in SXY0..SXY2, and a vertex projected right
 * after INTPL interpolated it keeps the exact interpolated position. */
void Psyz_GteSetPgxpNclip(int enabled);
void Psyz_GteSetPgxpIntpl(int enabled);
/* Depth buffer class of the vertices projected or copied from now on
 * (rr/depth_buffer.h): PSYZ_PGXP_DEPTH_NONE for a sky or backdrop around the
 * camera, which stays behind the world by its ordering-table order; WORLD
 * again after it. */
#ifndef PSYZ_PGXP_DEPTH_WORLD
#define PSYZ_PGXP_DEPTH_WORLD 0
#define PSYZ_PGXP_DEPTH_NONE 1
#endif
void Psyz_GteSetPgxpDepthClass(int depth_class);
/* The ordering bias of the face whose vertices are projected or copied from
 * now on (active != 0): it decides between faces at the same depth. */
void Psyz_GteSetPgxpDepthLayer(int active, int ordering_bias);
/* The SZ of a screen word projected in an earlier frame, stored at dst. */
void Psyz_GteStorePgxpDepth(const void *dst, unsigned int sxy, int sz);
/* swc2 / lwc2 of any data register. */
static inline void RrGteSwc2(unsigned reg, void *dst)
{
    if (reg - 12u <= 3u) {
        Psyz_GteStoreSxyTo(reg, dst);
    } else {
        u32 value = Psyz_GteDataRead(reg);
        memcpy(dst, &value, 4);
    }
}
static inline void RrGteLwc2(unsigned reg, const void *src)
{
    if (reg - 12u <= 3u) {
        Psyz_GteLoadSxyFrom(reg, src);
    } else {
        u32 value;
        memcpy(&value, src, 4);
        Psyz_GteDataWrite(reg, value);
    }
}
/* *dst = *src for a 32-bit screen XY word. */
static inline void RrSxyCopy(void *dst, const void *src)
{
    u32 value;
    memcpy(&value, src, 4);
    memcpy(dst, &value, 4);
    Psyz_GteCopyPgxpSxy(src, dst, value);
}
/* lvalue forms: RR_SXY_COPY(p->xy0, cache[i]); RR_GTE_STORE_SXY(12, &sxy[0]). */
#define RR_SXY_COPY(dst, src) RrSxyCopy(&(dst), &(src))
#define RR_GTE_STORE_SXY(reg, dst) Psyz_GteStoreSxyTo((reg), (dst))
#define RR_GTE_LOAD_SXY(reg, src) Psyz_GteLoadSxyFrom((reg), (src))
/* GTE (COP2) transfers and commands of the original hand-written sequences,
 * expressed in portable C over the PsyZ GTE. Register numbers and operation
 * order are those of the original; MIPS temporaries are block locals.
 * RR_NOP_DEP*: former load-delay gaps, no host effect. */
#define RR_NOP_DEP() do {  } while (0)
/* RR_NOP_DEP_ON(value): keep that pipeline gap after the load producing value. */
#define RR_NOP_DEP_ON(value) do {  } while (0)
/* GTE (COP2) helpers. All use $t4-$t6 ($12-$14) as scratch exactly like the original hand-written PsyQ-style macros. */
/* RR_GTE_SETLCM(p): load the 0x14-byte light-colour matrix block at p into GTE control regs $16-$20 (LCM). */
#define RR_GTE_SETLCM(p) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (4)); Psyz_GteCtrlWrite(16, (u32)rr_r12); Psyz_GteCtrlWrite(17, (u32)rr_r13); rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (8)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (12)); rr_r14 = *(u32 *)((u8 *)(uintptr_t)((p)) + (16)); Psyz_GteCtrlWrite(18, (u32)rr_r12); Psyz_GteCtrlWrite(19, (u32)rr_r13); Psyz_GteCtrlWrite(20, (u32)rr_r14); } while (0)
/* RR_GTE_SETLLM(p): light matrix block (control regs $8-$12). */
#define RR_GTE_SETLLM(p) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (4)); Psyz_GteCtrlWrite(8, (u32)rr_r12); Psyz_GteCtrlWrite(9, (u32)rr_r13); rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (8)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (12)); rr_r14 = *(u32 *)((u8 *)(uintptr_t)((p)) + (16)); Psyz_GteCtrlWrite(10, (u32)rr_r12); Psyz_GteCtrlWrite(11, (u32)rr_r13); Psyz_GteCtrlWrite(12, (u32)rr_r14); } while (0)
/* RR_GTE_SETROT(p): rotation matrix block (control regs $0-$4). */
#define RR_GTE_SETROT(p) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (4)); Psyz_GteCtrlWrite(0, (u32)rr_r12); Psyz_GteCtrlWrite(1, (u32)rr_r13); rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (8)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (12)); rr_r14 = *(u32 *)((u8 *)(uintptr_t)((p)) + (16)); Psyz_GteCtrlWrite(2, (u32)rr_r12); Psyz_GteCtrlWrite(3, (u32)rr_r13); Psyz_GteCtrlWrite(4, (u32)rr_r14); } while (0)
/* RR_GTE_SETTR(p): translation vector (control regs $5-$7) from 3 words at p. */
#define RR_GTE_SETTR(p) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (4)); rr_r14 = *(u32 *)((u8 *)(uintptr_t)((p)) + (8)); Psyz_GteCtrlWrite(5, (u32)rr_r12); Psyz_GteCtrlWrite(6, (u32)rr_r13); Psyz_GteCtrlWrite(7, (u32)rr_r14); } while (0)
/* RR_GTE_SETFC(r,g,b): far colour (control regs $21-$23) from three register values. */
#define RR_GTE_SETFC(r, g, b) do { Psyz_GteCtrlWrite(21, (u32)(r)); Psyz_GteCtrlWrite(22, (u32)(g)); Psyz_GteCtrlWrite(23, (u32)(b)); } while (0)
/* RR_GTE_SETBK(r,g,b): background colour (control regs $13-$15). */
#define RR_GTE_SETBK(r, g, b) do { Psyz_GteCtrlWrite(13, (u32)(r)); Psyz_GteCtrlWrite(14, (u32)(g)); Psyz_GteCtrlWrite(15, (u32)(b)); } while (0)

/* Raw GTE command words and single-matrix-half loader (central, ex w3t0_gte.h). */
#define RR_GTE_SETMAT(p, c0) do { Psyz_GteCtrlWrite((c0), ((const u32 *)(p))[0]); Psyz_GteCtrlWrite((c0) + 1, ((const u32 *)(p))[1]); } while (0)
#define RR_GTE_OP(w) Psyz_GteCommand((u32)(w) & 0x1FFFFFFu)
#define GRTPS ".word 0x4A180001"
#define GRTPT ".word 0x4A280030"
#define GNCLIP ".word 0x4B400006"
#define GAVSZ3 ".word 0x4B58002D"
#define GAVSZ4 ".word 0x4B68002E"
#define GDPCS ".word 0x4A780010"
#define GDPCT ".word 0x4AF8002A"
#define GINTPL ".word 0x4A980011"
#define GGPF ".word 0x4B98003D"
#define GCC ".word 0x4B38041C"
#define GSQR ".word 0x4AA80428"
#define GNCCS ".word 0x4B08041B"
#define GNCCT ".word 0x4B18043F"
/* Load packed SXY0, SXY2, SXY1 in PsyQ order and run NCLIP. MAC0 remains in
 * the GTE so a caller may read it after its depth test. */
#define RR_GTE_NCLIP_SXY3(v12, v13, v14) do { Psyz_GteDataWrite(12, (u32)(v12)); Psyz_GteDataWrite(14, (u32)(v14)); Psyz_GteDataWrite(13, (u32)(v13)); Psyz_GteCommand(0x1400006); } while (0)
/* RR_GTE_NCLIP3(v12, v13, v14, out): load three packed screen-XY words into SXY0..SXY2 ($12-$14, written $12,$14,$13 like PsyQ gte_ldsxy3), run NCLIP, store MAC0 ($24) to *out. */
#define RR_GTE_NCLIP3(v12, v13, v14, out) do {      do { Psyz_GteDataWrite(12, (u32)(v12)); Psyz_GteDataWrite(14, (u32)(v14)); Psyz_GteDataWrite(13, (u32)(v13)); Psyz_GteCommand(0x1400006); } while (0);      do { *(u32 *)((u8 *)(uintptr_t)((out)) + (0)) = Psyz_GteDataRead(24); } while (0); } while (0)
/* Single-instruction / fused GTE helpers (TransformRroVertices). */
#define RR_GTE_MTC2_IR0(x) do { Psyz_GteDataWrite(8, (u32)(x)); } while (0)
/* RGB/code input word consumed by the GTE lighting and depth-cue commands. */
#define RR_GTE_MTC2_RGB(x) do { Psyz_GteDataWrite(6, (u32)(x)); } while (0)
#define RR_GTE_LWC2_RGB(p) do { Psyz_GteDataWrite(6, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); } while (0)
#define RR_GTE_SWC2_RGB2(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(22); } while (0)
/* RR_GTE_DPCS_FC4(q): far colour = 3 bytes at q, each << 4 (ctc2 $21-$23), then DPCS. */
#define RR_GTE_DPCS_FC4(q) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = *(u8 *)((u8 *)(uintptr_t)((q)) + (0)); rr_r13 = *(u8 *)((u8 *)(uintptr_t)((q)) + (1)); rr_r14 = *(u8 *)((u8 *)(uintptr_t)((q)) + (2)); rr_r12 = (u32)rr_r12 << 4; rr_r13 = (u32)rr_r13 << 4; rr_r14 = (u32)rr_r14 << 4; Psyz_GteCtrlWrite(21, (u32)rr_r12); Psyz_GteCtrlWrite(22, (u32)rr_r13); Psyz_GteCtrlWrite(23, (u32)rr_r14); Psyz_GteCommand(0x0780010); } while (0)
/* Set far-color control registers from the low three bytes, scaled by 16.
 * Unlike RR_GTE_DPCS_FC4, this leaves the depth-cue command to the caller. */
#define RR_GTE_SETFC_BYTES3(p) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = *(u8 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r13 = *(u8 *)((u8 *)(uintptr_t)((p)) + (1)); rr_r14 = *(u8 *)((u8 *)(uintptr_t)((p)) + (2)); rr_r12 = (u32)rr_r12 << 4; rr_r13 = (u32)rr_r13 << 4; rr_r14 = (u32)rr_r14 << 4; Psyz_GteCtrlWrite(21, (u32)rr_r12); Psyz_GteCtrlWrite(22, (u32)rr_r13); Psyz_GteCtrlWrite(23, (u32)rr_r14); } while (0)
/* Spark endpoints share one staging vector: load V1, replace the vector,
 * then load V2 and project the three GTE input slots. */
#define RR_GTE14_LWC2_V1(p) do { Psyz_GteDataWrite(2, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); Psyz_GteDataWrite(3, *(u32 *)((u8 *)(uintptr_t)((p)) + (4))); } while (0)
#define RR_GTE_RTPT_LDV2(p) do { Psyz_GteDataWrite(4, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); Psyz_GteDataWrite(5, *(u32 *)((u8 *)(uintptr_t)((p)) + (4))); Psyz_GteCommand(0x0280030); } while (0)
#define RR_GTE14_SWC2_SZ3(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(19); } while (0)
#define RR_GTE14_SWC2_SXY1(p) do { RrGteSwc2(13, (u8 *)(uintptr_t)(p) + (0)); } while (0)
/* RR_GTE_MVMVA_LDV0(p): load vector V0 (VXY0/VZ0) from 3 shorts at p and run MVMVA sf=1 mx=rot v=V0 cv=none lm=0; RR_GTE_STORE_IR(p): store IR1..IR3 ($9-$11) to p. */
#define RR_GTE_MVMVA_LDV0(p) do { do { Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)((p)) + (4))); } while (0); do {  } while (0); do { Psyz_GteCommand(0x0080012); } while (0); } while (0)
#define RR_GTE_STORE_IR(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(9); *(u32 *)((u8 *)(uintptr_t)((p)) + (4)) = Psyz_GteDataRead(10); *(u32 *)((u8 *)(uintptr_t)((p)) + (8)) = Psyz_GteDataRead(11); } while (0)
/* DrawTrackObjectLitModel: MVMVA sf=1 mx=LLM v=V0 cv=BK lm=0 on the vector at p (lwc2 $0/$1); RR_GTE_SWC2_IR3(p): swc2 $11; RR_GTE_TR_FROM_IR(): TR = IR1..IR3. */
#define RR_GTE_MVMVA_LLM_BK_LDV0(p) do { do { Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)((p)) + (4))); } while (0); do {  } while (0); do { Psyz_GteCommand(0x04A2012); } while (0); } while (0)
#define RR_GTE_SWC2_IR3(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(11); } while (0)
#define RR_GTE_TR_FROM_IR() do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = Psyz_GteDataRead(9); rr_r13 = Psyz_GteDataRead(10); rr_r14 = Psyz_GteDataRead(11); Psyz_GteCtrlWrite(5, (u32)rr_r12); Psyz_GteCtrlWrite(6, (u32)rr_r13); Psyz_GteCtrlWrite(7, (u32)rr_r14); } while (0)
/* Read the signed IR3 data register; the caller supplies its CPU delay. */
#define RR_GTE_READ_IR3(out) do { (out) = Psyz_GteDataRead(11); } while (0)
/* Glow-fan projection: read signed IR3 with its CPU transfer delay, then use
 * an identity rotation and packed XY inputs whose Z registers are zero. */
#define RR_GTE_READ_IR3_NOP(out) do { (out) = Psyz_GteDataRead(11); } while (0)
#define RR_GTE_SETROT_IDENTITY_CLEAR_Z() do { Psyz_GteCtrlWrite(0, (u32)(0x1000)); Psyz_GteCtrlWrite(1, (u32)0u); Psyz_GteCtrlWrite(2, (u32)(0x1000)); Psyz_GteCtrlWrite(3, (u32)0u); Psyz_GteCtrlWrite(4, (u32)(0x1000)); Psyz_GteDataWrite(1, (u32)0u); Psyz_GteDataWrite(3, (u32)0u); Psyz_GteDataWrite(5, (u32)0u); } while (0)
#define RR_GTE_RTPT_PACKED_XY(p) do { Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); Psyz_GteDataWrite(2, *(u32 *)((u8 *)(uintptr_t)((p)) + (4))); Psyz_GteDataWrite(4, *(u32 *)((u8 *)(uintptr_t)((p)) + (8))); Psyz_GteCommand(0x0280030); } while (0)
/* StepIntroScene: RR_GTE_LWC2_IR(p): load IR1..IR3 ($9-$11) from 3 words at p; RR_GTE_INTPL(): two nops then INTPL. */
#define RR_GTE_LWC2_IR(p) do { Psyz_GteDataWrite(9, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); Psyz_GteDataWrite(10, *(u32 *)((u8 *)(uintptr_t)((p)) + (4))); Psyz_GteDataWrite(11, *(u32 *)((u8 *)(uintptr_t)((p)) + (8))); } while (0)
#define RR_GTE_INTPL() do { Psyz_GteCommand(0x0980011); } while (0)
/* StepIntroScene: RR_GTE_SETFC_P(p): far colour from 3 words at p via $t2-$t4; RR_COPY16_T(dst, src, off): copy 16 bytes src+off -> dst via $t2-$t4 (3 loads, 3 stores, 4th word last). */
#define RR_GTE_SETFC_P(p) do { u32 rr_r10 = 0, rr_r11 = 0, rr_r12 = 0; (void)rr_r10; (void)rr_r11; (void)rr_r12; rr_r10 = *(u32 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r11 = *(u32 *)((u8 *)(uintptr_t)((p)) + (4)); rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (8)); Psyz_GteCtrlWrite(21, (u32)rr_r10); Psyz_GteCtrlWrite(22, (u32)rr_r11); Psyz_GteCtrlWrite(23, (u32)rr_r12); } while (0)
#define RR_COPY16_T(dst, src, off) do { u32 rr_r10 = 0, rr_r11 = 0, rr_r12 = 0; (void)rr_r10; (void)rr_r11; (void)rr_r12; rr_r10 = *(u32 *)((u8 *)(uintptr_t)((src)) + (off)); rr_r11 = *(u32 *)((u8 *)(uintptr_t)((src)) + ((off)+4)); rr_r12 = *(u32 *)((u8 *)(uintptr_t)((src)) + ((off)+8)); *(u32 *)((u8 *)(uintptr_t)((dst)) + (0)) = (u32)rr_r10; *(u32 *)((u8 *)(uintptr_t)((dst)) + (4)) = (u32)rr_r11; *(u32 *)((u8 *)(uintptr_t)((dst)) + (8)) = (u32)rr_r12; rr_r10 = *(u32 *)((u8 *)(uintptr_t)((src)) + ((off)+12)); *(u32 *)((u8 *)(uintptr_t)((dst)) + (12)) = (u32)rr_r10; } while (0)
/* DrawTrackObjectBillboard: RR_GTE_SETBK_P(p): background colour (ctc2 $13-$15) from 3 words at p via $t4-$t6; RR_GTE_RTPT_LDV(a,b,c): load V0..V2 from three vectors (lwc2 0..5), two nops, RTPT; RR_GTE_RTPS_LDV(p): load V0, two nops, RTPS; RR_GTE_SWC2_SXYZ(p): store SXY0..SXY2 ($12-$14) to p; RR_GTE_SWC2_SXY2(p): store SXY2 ($14). */
#define RR_GTE_SETBK_P(p) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (4)); rr_r14 = *(u32 *)((u8 *)(uintptr_t)((p)) + (8)); Psyz_GteCtrlWrite(13, (u32)rr_r12); Psyz_GteCtrlWrite(14, (u32)rr_r13); Psyz_GteCtrlWrite(15, (u32)rr_r14); } while (0)
#define RR_GTE_RTPT_LDV(a, b, c) do { do { Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)((a)) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)((a)) + (4))); Psyz_GteDataWrite(2, *(u32 *)((u8 *)(uintptr_t)((b)) + (0))); Psyz_GteDataWrite(3, *(u32 *)((u8 *)(uintptr_t)((b)) + (4))); Psyz_GteDataWrite(4, *(u32 *)((u8 *)(uintptr_t)((c)) + (0))); Psyz_GteDataWrite(5, *(u32 *)((u8 *)(uintptr_t)((c)) + (4))); } while (0); do {  } while (0); do { Psyz_GteCommand(0x0280030); } while (0); } while (0)
#define RR_GTE_RTPS_LDV(p) { do { Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)((p)) + (4))); } while (0); do {  } while (0); do { Psyz_GteCommand(0x0180001); } while (0); }
#define RR_GTE_SWC2_SXYZ(p) do { RrGteSwc2(12, (u8 *)(uintptr_t)(p) + (0)); RrGteSwc2(13, (u8 *)(uintptr_t)(p) + (4)); RrGteSwc2(14, (u8 *)(uintptr_t)(p) + (8)); } while (0)
#define RR_GTE_SWC2_SXY2(p) do { RrGteSwc2(14, (u8 *)(uintptr_t)(p) + (0)); } while (0)
/* Store three projected XY words in a POLY_G3 packet, then read those same
 * GTE registers for the CPU viewport test. */
#define RR_GTE_STORE_READ_SXY3(p, x0, x1, x2) do { RrGteSwc2(12, (u8 *)(uintptr_t)(p) + (8)); RrGteSwc2(13, (u8 *)(uintptr_t)(p) + (16)); RrGteSwc2(14, (u8 *)(uintptr_t)(p) + (24)); (x0) = Psyz_GteDataRead(12); (x1) = Psyz_GteDataRead(13); (x2) = Psyz_GteDataRead(14); } while (0)
/* DrawTrackScenery: RR_GTE_CLRTR(): TR = 0; RR_GTE_MTC2_IR1(x): mtc2 $9; RR_GTE_CTC2_FCR(x): ctc2 $21; RR_GTE_SWC2_IR1(p): swc2 $9. */
#define RR_GTE_CLRTR() do { Psyz_GteCtrlWrite(5, (u32)0u); Psyz_GteCtrlWrite(6, (u32)0u); Psyz_GteCtrlWrite(7, (u32)0u); } while (0)
#define RR_GTE_MTC2_IR1(x) do { Psyz_GteDataWrite(9, (u32)(x)); } while (0)
#define RR_GTE_CTC2_FCR(x) do { Psyz_GteCtrlWrite(21, (u32)(x)); } while (0)
#define RR_GTE_SWC2_IR1(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(9); } while (0)
/* EmitRroLitGouraudQuads GTE helpers (one macro per distinct instruction/sequence). */
#define RR_GTE14_LWC2_V012(p) do { Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)((p)) + (4))); Psyz_GteDataWrite(2, *(u32 *)((u8 *)(uintptr_t)((p)) + (8))); Psyz_GteDataWrite(3, *(u32 *)((u8 *)(uintptr_t)((p)) + (12))); Psyz_GteDataWrite(4, *(u32 *)((u8 *)(uintptr_t)((p)) + (16))); Psyz_GteDataWrite(5, *(u32 *)((u8 *)(uintptr_t)((p)) + (20))); } while (0)
#define RR_GTE14_LWC2_V0(p) do { Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)((p)) + (4))); } while (0)
#define RR_GTE14_LWC2_V0_V1_V2(a, b, c) do { Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)((a)) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)((a)) + (4))); Psyz_GteDataWrite(2, *(u32 *)((u8 *)(uintptr_t)((b)) + (0))); Psyz_GteDataWrite(3, *(u32 *)((u8 *)(uintptr_t)((b)) + (4))); Psyz_GteDataWrite(4, *(u32 *)((u8 *)(uintptr_t)((c)) + (0))); Psyz_GteDataWrite(5, *(u32 *)((u8 *)(uintptr_t)((c)) + (4))); } while (0)
#define RR_GTE14_LWC2_RGB(p) do { Psyz_GteDataWrite(6, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); } while (0)
#define RR_GTE14_NOP2() do {  } while (0)
#define RR_GTE14_RTPT() do { Psyz_GteCommand(0x0280030); } while (0)
/* SDK RTPT command with the two CPU load-delay slots in the same block. */
#define RR_GTE_RTPT_NOP2() do { Psyz_GteCommand(0x0280030); } while (0)
#define RR_GTE14_RTPS() do { Psyz_GteCommand(0x0180001); } while (0)
#define RR_GTE14_NCLIP() do { Psyz_GteCommand(0x1400006); } while (0)
#define RR_GTE14_AVSZ4() do { Psyz_GteCommand(0x168002E); } while (0)
#define RR_GTE14_SWC2_MAC0(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(24); } while (0)
#define RR_GTE14_SWC2_OTZ(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(7); } while (0)
#define RR_GTE14_SWC2_SXYZ(p) do { RrGteSwc2(12, (u8 *)(uintptr_t)(p) + (0)); RrGteSwc2(13, (u8 *)(uintptr_t)(p) + (4)); RrGteSwc2(14, (u8 *)(uintptr_t)(p) + (8)); } while (0)
#define RR_GTE14_SWC2_SXY2(p) do { RrGteSwc2(14, (u8 *)(uintptr_t)(p) + (0)); } while (0)
#define RR_GTE14_LIGHT0() do { Psyz_GteCommand(0x04A6412); Psyz_GteCommand(0x0A80428); Psyz_GteCommand(0x0A80428); Psyz_GteCommand(0x138041C); } while (0)
#define RR_GTE14_LIGHT1() do { Psyz_GteCommand(0x04AE412); Psyz_GteCommand(0x0A80428); Psyz_GteCommand(0x0A80428); Psyz_GteCommand(0x138041C); } while (0)
#define RR_GTE14_LIGHT2() do { Psyz_GteCommand(0x04B6412); Psyz_GteCommand(0x0A80428); Psyz_GteCommand(0x0A80428); Psyz_GteCommand(0x138041C); } while (0)
#define RR_GTE14_SWC2_RGB012(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (4)) = Psyz_GteDataRead(20); *(u32 *)((u8 *)(uintptr_t)((p)) + (12)) = Psyz_GteDataRead(21); *(u32 *)((u8 *)(uintptr_t)((p)) + (20)) = Psyz_GteDataRead(22); } while (0)
#define RR_GTE14_SWC2_RGB2(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(22); } while (0)
/* FindWheelTrackNodes: MVMVA rot-matrix x V0, no translation, shift 12 (sf=1,mx=0,v=0,cv=3,lm=0); preceded by the V0 load from p. */
#define RR_GTE_MVMVA_ROT_V0_NOCV(p) { do { Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)((p)) + (4))); } while (0); do {  } while (0); do { Psyz_GteCommand(0x0486012); } while (0); }

/* SQR: square signed IR1..IR3, with fixed-point shift 12 or no shift.
 * MAC1..MAC3 retain the arithmetic results; IR saturation and FLAG are hardware side effects. */
#define RR_GTE_SQR12() RR_GTE_OP(0x4AA80428)
#define RR_GTE_SQR0() RR_GTE_OP(0x4AA00428)
#define RR_GTE_SWC2_MAC123(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(25); *(u32 *)((u8 *)(uintptr_t)((p)) + (4)) = Psyz_GteDataRead(26); *(u32 *)((u8 *)(uintptr_t)((p)) + (8)) = Psyz_GteDataRead(27); } while (0)

/* Memory-clobbering GTE command barriers used by polygon packet builders (DrawMeshQuadGT4..DrawMeshQuadF4). */
#define RR_GTE_RTPT_MEM() do { Psyz_GteCommand(0x0280030); } while (0)
#define RR_GTE_RTPS_MEM() do { Psyz_GteCommand(0x0180001); } while (0)
#define RR_GTE_NCLIP_MEM() do { Psyz_GteCommand(0x1400006); } while (0)
#define RR_GTE_AVSZ4_MEM() do { Psyz_GteCommand(0x168002E); } while (0)

/* SetupSkyLightMatrices: column-wise IR1..IR3 load/MVMVA and short store helpers. */
#define RR_GTE_MV3_COL(src) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = *(u16 *)((u8 *)(uintptr_t)((src)) + (0)); rr_r13 = *(u16 *)((u8 *)(uintptr_t)((src)) + (6)); rr_r14 = *(u16 *)((u8 *)(uintptr_t)((src)) + (12)); Psyz_GteDataWrite(9, (u32)rr_r12); Psyz_GteDataWrite(10, (u32)rr_r13); Psyz_GteDataWrite(11, (u32)rr_r14); Psyz_GteCommand(0x049E012); } while (0)
#define RR_GTE_ST3_COL(dst) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = Psyz_GteDataRead(9); rr_r13 = Psyz_GteDataRead(10); rr_r14 = Psyz_GteDataRead(11); *(u16 *)((u8 *)(uintptr_t)((dst)) + (0)) = (u16)rr_r12; *(u16 *)((u8 *)(uintptr_t)((dst)) + (6)) = (u16)rr_r13; *(u16 *)((u8 *)(uintptr_t)((dst)) + (12)) = (u16)rr_r14; } while (0)

/* UpdateComCarSpeed: store IR1..IR3 ($9-$11) as contiguous 16-bit components. */
#define RR_GTE_STORE_IR_S16(p) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = Psyz_GteDataRead(9); rr_r13 = Psyz_GteDataRead(10); rr_r14 = Psyz_GteDataRead(11); *(u16 *)((u8 *)(uintptr_t)((p)) + (0)) = (u16)rr_r12; *(u16 *)((u8 *)(uintptr_t)((p)) + (2)) = (u16)rr_r13; *(u16 *)((u8 *)(uintptr_t)((p)) + (4)) = (u16)rr_r14; } while (0)

/* SoundCalcCarParams / SetTrackObjectTransform: combined rotation + translation matrix loaders and FLAG reader. */
#define RR_GTE_SETROT_TR(p) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (4)); Psyz_GteCtrlWrite(0, (u32)rr_r12); Psyz_GteCtrlWrite(1, (u32)rr_r13); rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (8)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (12)); rr_r14 = *(u32 *)((u8 *)(uintptr_t)((p)) + (16)); Psyz_GteCtrlWrite(2, (u32)rr_r12); Psyz_GteCtrlWrite(3, (u32)rr_r13); Psyz_GteCtrlWrite(4, (u32)rr_r14); rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (20)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (24)); Psyz_GteCtrlWrite(5, (u32)rr_r12); rr_r14 = *(u32 *)((u8 *)(uintptr_t)((p)) + (28)); Psyz_GteCtrlWrite(6, (u32)rr_r13); Psyz_GteCtrlWrite(7, (u32)rr_r14); } while (0)
#define RR_GTE_SETTR_AT20(p) do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (20)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((p)) + (24)); Psyz_GteCtrlWrite(5, (u32)rr_r12); rr_r14 = *(u32 *)((u8 *)(uintptr_t)((p)) + (28)); Psyz_GteCtrlWrite(6, (u32)rr_r13); Psyz_GteCtrlWrite(7, (u32)rr_r14); } while (0)
#define RR_GTE_MVMVA_ROT_TR_V0(p) do { Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)((p)) + (4))); } while (0); do { Psyz_GteCommand(0x0480012); } while (0)
#define RR_GTE_STORE_FLAG(p) do { u32 rr_r12 = 0; (void)rr_r12; rr_r12 = Psyz_GteCtrlRead(31); *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = (u32)rr_r12; } while (0)
#define RR_GTE_STORE_IR0_THEN_FLAG(p) do { u32 rr_r12 = 0; (void)rr_r12; *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(8); rr_r12 = Psyz_GteCtrlRead(31); *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = (u32)rr_r12; } while (0)
#define RR_GTE_SWC2_IR0(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(8); } while (0)
#define RR_GTE_STORE_SZ3_SRA2(p) do { u32 rr_r12 = 0; (void)rr_r12; rr_r12 = Psyz_GteDataRead(19); rr_r12 = (u32)((s32)rr_r12 >> 2); *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = (u32)rr_r12; } while (0)

/* PsyQ libgte hand-coded matrix multiply helpers (MulMatrix0..ApplyMatrixSV). */
#define RR_GTE_MASK_HI16(x) do { (x) = (u32)(x) & (u32)(0xFFFF0000); } while (0)
#define RR_GTE_MVMVA_ROT_V0_1NOP() do { Psyz_GteCommand(0x0486012); } while (0)
#define RR_GTE_SW_AT(val, base, idx) do { *(u32 *)((u8 *)(uintptr_t)((base)) + (idx*4)) = (u32)(val); } while (0)
#define RR_GTE_SH_AT(val, base, idx) do { *(u16 *)((u8 *)(uintptr_t)((base)) + (idx*2)) = (u16)(val); } while (0)

/* Generic single-register and single-offset GTE transfers and commands. */
#define RR_GTE_CTC2(val, reg) do { Psyz_GteCtrlWrite(reg, (u32)(val)); } while (0)
#define RR_GTE_CFC2(out, reg) do { (out) = Psyz_GteCtrlRead(reg); } while (0)
#define RR_GTE_MTC2(val, reg) do { Psyz_GteDataWrite(reg, (u32)(val)); } while (0)
#define RR_GTE_MFC2(out, reg) do { (out) = Psyz_GteDataRead(reg); } while (0)
#define RR_GTE_LWC2_AT(reg, off, base) do { RrGteLwc2(reg, (u8 *)(uintptr_t)(base) + (off)); } while (0)
/* RR_GTE_LWC2_SP read stack-passed PsyQ arguments; the host uses PsyZ libgte instead. */
#define RR_GTE_SWC2_AT(reg, off, base) do { RrGteSwc2(reg, (u8 *)(uintptr_t)(base) + (off)); } while (0)
#define RR_GTE_SWC2_AT_MEM(reg, off, base) do { RrGteSwc2(reg, (u8 *)(uintptr_t)(base) + (off)); } while (0)
#define RR_GTE_CMD_1NOP(op) Psyz_GteCommand((u32)(op) & 0x1FFFFFFu)
#define RR_GTE_CMD_2NOP(op) Psyz_GteCommand((u32)(op) & 0x1FFFFFFu)
#define RR_GTE_LWC2_PAIR_AT(r0, off0, r1, off1, base) do { RrGteLwc2(r0, (u8 *)(uintptr_t)(base) + (off0)); RrGteLwc2(r1, (u8 *)(uintptr_t)(base) + (off1)); } while (0)
#define RR_GTE_SWC2_PAIR_AT(r0, off0, r1, off1, base) do { RrGteSwc2(r0, (u8 *)(uintptr_t)(base) + (off0)); RrGteSwc2(r1, (u8 *)(uintptr_t)(base) + (off1)); } while (0)
#define RR_GTE_SWC2_TRIPLE_AT(r0, off0, r1, off1, r2, off2, base) do { RrGteSwc2(r0, (u8 *)(uintptr_t)(base) + (off0)); RrGteSwc2(r1, (u8 *)(uintptr_t)(base) + (off1)); RrGteSwc2(r2, (u8 *)(uintptr_t)(base) + (off2)); } while (0)
#define RR_GTE_LWC2_V012_3P(v0, v1, v2) do { Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)((v0)) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)((v0)) + (4))); Psyz_GteDataWrite(2, *(u32 *)((u8 *)(uintptr_t)((v1)) + (0))); Psyz_GteDataWrite(3, *(u32 *)((u8 *)(uintptr_t)((v1)) + (4))); Psyz_GteDataWrite(4, *(u32 *)((u8 *)(uintptr_t)((v2)) + (0))); Psyz_GteDataWrite(5, *(u32 *)((u8 *)(uintptr_t)((v2)) + (4))); } while (0)

/* gtepoly1.c GTE helpers. */
#define GTE_STORE_SXY_SZ(p) do { RrGteSwc2(12, (u8 *)(uintptr_t)(p) + (0)); *(u32 *)((u8 *)(uintptr_t)((p)) + (4)) = Psyz_GteDataRead(17); RrGteSwc2(13, (u8 *)(uintptr_t)(p) + (8)); *(u32 *)((u8 *)(uintptr_t)((p)) + (12)) = Psyz_GteDataRead(18); RrGteSwc2(14, (u8 *)(uintptr_t)(p) + (16)); *(u32 *)((u8 *)(uintptr_t)((p)) + (20)) = Psyz_GteDataRead(19); } while (0)
#define GTE_SETROT_AND_SAVE(src, dst) do { u32 rr_r12 = 0, rr_r13 = 0; (void)rr_r12; (void)rr_r13; rr_r12 = *(u32 *)((u8 *)(uintptr_t)((src)) + (0)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((src)) + (4)); Psyz_GteCtrlWrite(0, (u32)rr_r12); Psyz_GteCtrlWrite(1, (u32)rr_r13); *(u32 *)((u8 *)(uintptr_t)((dst)) + (0)) = (u32)rr_r12; *(u32 *)((u8 *)(uintptr_t)((dst)) + (4)) = (u32)rr_r13; rr_r12 = *(u32 *)((u8 *)(uintptr_t)((src)) + (8)); rr_r13 = *(u32 *)((u8 *)(uintptr_t)((src)) + (12)); Psyz_GteCtrlWrite(2, (u32)rr_r12); Psyz_GteCtrlWrite(3, (u32)rr_r13); *(u32 *)((u8 *)(uintptr_t)((dst)) + (8)) = (u32)rr_r12; rr_r12 = *(u32 *)((u8 *)(uintptr_t)((src)) + (16)); *(u32 *)((u8 *)(uintptr_t)((dst)) + (12)) = (u32)rr_r13; Psyz_GteCtrlWrite(4, (u32)rr_r12); *(u32 *)((u8 *)(uintptr_t)((dst)) + (16)) = (u32)rr_r12; } while (0)
#define GTE_CTC2_27(val) do { Psyz_GteCtrlWrite(27, (u32)(val)); } while (0)
#define GTE_CTC2_28(val) do { Psyz_GteCtrlWrite(28, (u32)(val)); } while (0)
#define GTE_LWC2_IR0(p) do { Psyz_GteDataWrite(8, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); } while (0)
#define GTE_LWC2_IR0_MEM(p) do { Psyz_GteDataWrite(8, *(u32 *)((u8 *)(uintptr_t)((p)) + (0))); } while (0)
#define GTE_DPCS_COL(col, dc) do { Psyz_GteDataWrite(8, (u32)(dc)); Psyz_GteDataWrite(6, (u32)(col)); Psyz_GteCommand(0x0780010); (col) = Psyz_GteDataRead(22); } while (0)
#define GTE_MTC2_RGB_DPCS(c) do { Psyz_GteDataWrite(6, (u32)(c)); Psyz_GteCommand(0x0780010); } while (0)
#define RR_GTE_MFC2_IR1_IR2(x, y) do { (x) = Psyz_GteDataRead(9); (y) = Psyz_GteDataRead(10); } while (0)
#define RR_GTE_SWC2_RGB012_ENV(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (4)) = Psyz_GteDataRead(20); *(u32 *)((u8 *)(uintptr_t)((p)) + (16)) = Psyz_GteDataRead(21); *(u32 *)((u8 *)(uintptr_t)((p)) + (28)) = Psyz_GteDataRead(22); } while (0)
#define RR_GTE_LDSXY3(sxy0, sxy1, sxy2) do { Psyz_GteDataWrite(12, (u32)(sxy0)); Psyz_GteDataWrite(14, (u32)(sxy2)); Psyz_GteDataWrite(13, (u32)(sxy1)); } while (0)
#define RR_GTE_MFC2_SXY2_NOP(out) do { (out) = Psyz_GteDataRead(14); } while (0)

/* ring.c GTE helper: RTPT projects the inner endpoint twice; the last two SXY stores share a slot. */
#define RING_STORE_SXY_PAIR(outer, inner) do { RrGteSwc2(12, (u8 *)(uintptr_t)(outer) + (0)); RrGteSwc2(13, (u8 *)(uintptr_t)(inner) + (0)); RrGteSwc2(14, (u8 *)(uintptr_t)(inner) + (0)); } while (0)

/* sky.c GTE helpers. */
#define RR_GTE_LOAD_SKY_TRI_VERTS(a1) do { u32 rr_r13 = 0, rr_r14 = 0, rr_r15 = 0; (void)rr_r13; (void)rr_r14; (void)rr_r15; rr_r13 = *(u32 *)((u8 *)(uintptr_t)((a1)) + (0x18)); rr_r14 = *(u32 *)((u8 *)(uintptr_t)((a1)) + (0x1C)); rr_r15 = *(u32 *)((u8 *)(uintptr_t)((a1)) + (0x20)); Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)(rr_r13) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)(rr_r13) + (4))); Psyz_GteDataWrite(2, *(u32 *)((u8 *)(uintptr_t)(rr_r14) + (0))); Psyz_GteDataWrite(3, *(u32 *)((u8 *)(uintptr_t)(rr_r14) + (4))); Psyz_GteDataWrite(4, *(u32 *)((u8 *)(uintptr_t)(rr_r15) + (0))); Psyz_GteDataWrite(5, *(u32 *)((u8 *)(uintptr_t)(rr_r15) + (4))); } while (0)
#define RR_GTE_DPCT_SKY_TRI(a1, v0, v1, a3, pkt) do { Psyz_GteDataWrite(8, *(u32 *)((u8 *)(uintptr_t)((a1)) + (0))); Psyz_GteDataWrite(6, (u32)(v0)); Psyz_GteDataWrite(20, (u32)(v0)); Psyz_GteDataWrite(21, (u32)(v1)); Psyz_GteDataWrite(22, (u32)(a3)); Psyz_GteCommand(0x0F8002A); *(u32 *)((u8 *)(uintptr_t)((pkt)) + (4)) = Psyz_GteDataRead(20); *(u32 *)((u8 *)(uintptr_t)((pkt)) + (12)) = Psyz_GteDataRead(21); *(u32 *)((u8 *)(uintptr_t)((pkt)) + (20)) = Psyz_GteDataRead(22); } while (0)
#define RR_GTE_RTPT_SKY_QUAD_VERTS(a1) do { u32 rr_r15 = 0, rr_r24 = 0, rr_r25 = 0; (void)rr_r15; (void)rr_r24; (void)rr_r25; rr_r15 = *(u32 *)((u8 *)(uintptr_t)((a1)) + (0x18)); rr_r24 = *(u32 *)((u8 *)(uintptr_t)((a1)) + (0x1C)); rr_r25 = *(u32 *)((u8 *)(uintptr_t)((a1)) + (0x20)); Psyz_GteDataWrite(0, *(u32 *)((u8 *)(uintptr_t)(rr_r15) + (0))); Psyz_GteDataWrite(1, *(u32 *)((u8 *)(uintptr_t)(rr_r15) + (4))); Psyz_GteDataWrite(2, *(u32 *)((u8 *)(uintptr_t)(rr_r24) + (0))); Psyz_GteDataWrite(3, *(u32 *)((u8 *)(uintptr_t)(rr_r24) + (4))); Psyz_GteDataWrite(4, *(u32 *)((u8 *)(uintptr_t)(rr_r25) + (0))); Psyz_GteDataWrite(5, *(u32 *)((u8 *)(uintptr_t)(rr_r25) + (4))); Psyz_GteCommand(0x0280030); } while (0)
#define RR_GTE_DPCT_DPCS_SKY_QUAD(a1, v1, t0, a3, t1, pkt) do { Psyz_GteDataWrite(8, *(u32 *)((u8 *)(uintptr_t)((a1)) + (0))); Psyz_GteDataWrite(6, (u32)(v1)); Psyz_GteDataWrite(20, (u32)(v1)); Psyz_GteDataWrite(21, (u32)(t0)); Psyz_GteDataWrite(22, (u32)(a3)); Psyz_GteCommand(0x0F8002A); *(u32 *)((u8 *)(uintptr_t)((pkt)) + (4)) = Psyz_GteDataRead(20); *(u32 *)((u8 *)(uintptr_t)((pkt)) + (12)) = Psyz_GteDataRead(21); *(u32 *)((u8 *)(uintptr_t)((pkt)) + (20)) = Psyz_GteDataRead(22); Psyz_GteDataWrite(6, (u32)(t1)); Psyz_GteCommand(0x0780010); } while (0)
#define RR_GTE_DPCS_SKY_FT4(a1, t1) do { Psyz_GteDataWrite(8, *(u32 *)((u8 *)(uintptr_t)((a1)) + (0))); Psyz_GteDataWrite(6, (u32)(t1)); Psyz_GteCommand(0x0780010); } while (0)
#define RR_GTE_DPCT_SKY_GT3(a1, v0, v1, a3, pkt) do { Psyz_GteDataWrite(8, *(u32 *)((u8 *)(uintptr_t)((a1)) + (0))); Psyz_GteDataWrite(6, (u32)(v0)); Psyz_GteDataWrite(20, (u32)(v0)); Psyz_GteDataWrite(21, (u32)(v1)); Psyz_GteDataWrite(22, (u32)(a3)); Psyz_GteCommand(0x0F8002A); *(u32 *)((u8 *)(uintptr_t)((pkt)) + (4)) = Psyz_GteDataRead(20); *(u32 *)((u8 *)(uintptr_t)((pkt)) + (16)) = Psyz_GteDataRead(21); *(u32 *)((u8 *)(uintptr_t)((pkt)) + (28)) = Psyz_GteDataRead(22); } while (0)
#define RR_GTE_STORE_IR_MEM(p) do { *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(9); *(u32 *)((u8 *)(uintptr_t)((p)) + (4)) = Psyz_GteDataRead(10); *(u32 *)((u8 *)(uintptr_t)((p)) + (8)) = Psyz_GteDataRead(11); } while (0)

/* cdfile2.c GTE helpers. */
#define RR_GTE_INTPL_TRACK_TABLE(v0, v1, a1, a2, a3, t2) do { Psyz_GteDataWrite(8, (u32)(a3)); Psyz_GteCtrlWrite(21, (u32)(v0)); Psyz_GteCtrlWrite(22, (u32)(a1)); Psyz_GteDataWrite(9, (u32)(a2)); Psyz_GteDataWrite(10, (u32)(v1)); Psyz_GteCommand(0x0980011); Psyz_GteDataWrite(8, (u32)(t2)); (v1) = Psyz_GteDataRead(10); Psyz_GteCtrlWrite(21, (u32)(v1)); Psyz_GteCommand(0x0980011); (v1) = Psyz_GteDataRead(9); Psyz_GteDataWrite(8, (u32)(v1)); } while (0)
#define RR_GTE_MTC2_IR123_INTPL(a0, t1, t2) do { Psyz_GteDataWrite(9, (u32)(a0)); Psyz_GteDataWrite(10, (u32)(t1)); Psyz_GteDataWrite(11, (u32)(t2)); Psyz_GteCommand(0x0980011); } while (0)
#define RR_GTE_BK_FROM_IR() do { u32 rr_r12 = 0, rr_r13 = 0, rr_r14 = 0; (void)rr_r12; (void)rr_r13; (void)rr_r14; rr_r12 = Psyz_GteDataRead(9); rr_r13 = Psyz_GteDataRead(10); rr_r14 = Psyz_GteDataRead(11); Psyz_GteCtrlWrite(13, (u32)rr_r12); Psyz_GteCtrlWrite(14, (u32)rr_r13); Psyz_GteCtrlWrite(15, (u32)rr_r14); } while (0)
#define RR_GTE_SWC2_SXY_3P(p0, p1, p2) do { RrGteSwc2(12, (u8 *)(uintptr_t)(p0) + (0)); RrGteSwc2(13, (u8 *)(uintptr_t)(p1) + (0)); RrGteSwc2(14, (u8 *)(uintptr_t)(p2) + (0)); } while (0)

/* poly1.c GTE helper: save far colour (control regs $21-$23) to three words. */
#define RR_GTE_SAVE_FC(p0, p1, p2) do { u32 rr_r8 = 0, rr_r9 = 0, rr_r10 = 0; (void)rr_r8; (void)rr_r9; (void)rr_r10; rr_r8 = Psyz_GteCtrlRead(21); rr_r9 = Psyz_GteCtrlRead(22); rr_r10 = Psyz_GteCtrlRead(23); *(u32 *)((u8 *)(uintptr_t)((p0)) + (0)) = (u32)rr_r8; *(u32 *)((u8 *)(uintptr_t)((p1)) + (0)) = (u32)rr_r9; *(u32 *)((u8 *)(uintptr_t)((p2)) + (0)) = (u32)rr_r10; } while (0)

/* tilerect.c GTE INTPL helper. */
#define RR_GTE_TILERECT_INTPL(ir2, p) do { u32 rr_r8 = 0, rr_r9 = 0, rr_r10 = 0, rr_r11 = 0; (void)rr_r8; (void)rr_r9; (void)rr_r10; (void)rr_r11; rr_r8 = *(u16 *)((u8 *)(uintptr_t)((p)) + (0x4A)); Psyz_GteDataWrite(8, (u32)rr_r8); rr_r9 = (u32)0u + (u32)(0x720); Psyz_GteDataWrite(9, (u32)rr_r9); rr_r10 = (u32)0u + (u32)(0u); Psyz_GteCtrlWrite(21, (u32)rr_r10); rr_r11 = (u32)0u + (u32)(0x90); Psyz_GteDataWrite(10, (u32)rr_r11); rr_r8 = (u32)0u + (u32)(0xFF); Psyz_GteCtrlWrite(22, (u32)rr_r8); Psyz_GteCommand(0x0980011); (p) = (u32)(p) + (u32)(0x30); *(u32 *)((u8 *)(uintptr_t)((p)) + (0)) = Psyz_GteDataRead(9); (ir2) = Psyz_GteDataRead(10); } while (0)

/* objdraw.c GTE helpers. */
#define RR_GTE_COPY_ROT_TR_TO_LLM_BK() do { u32 rr_r12 = 0, rr_r13 = 0; (void)rr_r12; (void)rr_r13; rr_r12 = Psyz_GteCtrlRead(0); rr_r13 = Psyz_GteCtrlRead(1); Psyz_GteCtrlWrite(8, (u32)rr_r12); Psyz_GteCtrlWrite(9, (u32)rr_r13); rr_r12 = Psyz_GteCtrlRead(2); rr_r13 = Psyz_GteCtrlRead(3); Psyz_GteCtrlWrite(10, (u32)rr_r12); Psyz_GteCtrlWrite(11, (u32)rr_r13); rr_r12 = Psyz_GteCtrlRead(4); rr_r13 = Psyz_GteCtrlRead(5); Psyz_GteCtrlWrite(12, (u32)rr_r12); Psyz_GteCtrlWrite(13, (u32)rr_r13); rr_r12 = Psyz_GteCtrlRead(6); rr_r13 = Psyz_GteCtrlRead(7); Psyz_GteCtrlWrite(14, (u32)rr_r12); Psyz_GteCtrlWrite(15, (u32)rr_r13); } while (0)
#define RR_GTE_SET_IDENTITY_ROT_ZERO_TR(one) do { Psyz_GteCtrlWrite(0, (u32)(one)); Psyz_GteCtrlWrite(1, (u32)0u); Psyz_GteCtrlWrite(2, (u32)(one)); Psyz_GteCtrlWrite(3, (u32)0u); Psyz_GteCtrlWrite(4, (u32)(one)); Psyz_GteCtrlWrite(5, (u32)0u); Psyz_GteCtrlWrite(6, (u32)0u); Psyz_GteCtrlWrite(7, (u32)0u); } while (0)

/* RR_GTE_SWC2_RGB012_A3_SP16_20 read stack-passed PsyQ arguments; the host uses PsyZ libgte instead. */
/* RR_GTE_SWC2_RGB012_SP read stack-passed PsyQ arguments; the host uses PsyZ libgte instead. */
/* Outer product: temporarily load the diagonal of the rotation matrix from a0, IR from a1, run op, store MAC1..3 to a2. */
#define RR_GTE_OUTER_PRODUCT(a0, a1, a2, op) do { \
    u32 rr_d0 = Psyz_GteCtrlRead(0), rr_d2 = Psyz_GteCtrlRead(2), rr_d4 = Psyz_GteCtrlRead(4); \
    Psyz_GteCtrlWrite(0, ((const u32 *)(a0))[0]); Psyz_GteCtrlWrite(2, ((const u32 *)(a0))[1]); Psyz_GteCtrlWrite(4, ((const u32 *)(a0))[2]); \
    Psyz_GteDataWrite(11, ((const u32 *)(a1))[2]); Psyz_GteDataWrite(9, ((const u32 *)(a1))[0]); Psyz_GteDataWrite(10, ((const u32 *)(a1))[1]); \
    Psyz_GteCommand((u32)(op) & 0x1FFFFFFu); \
    ((u32 *)(a2))[0] = Psyz_GteDataRead(25); ((u32 *)(a2))[1] = Psyz_GteDataRead(26); ((u32 *)(a2))[2] = Psyz_GteDataRead(27); \
    Psyz_GteCtrlWrite(0, rr_d0); Psyz_GteCtrlWrite(2, rr_d2); Psyz_GteCtrlWrite(4, rr_d4); } while (0)
#define RR_GTE_SETROT_5W(p) do { u32 rr_r8 = 0, rr_r9 = 0, rr_r10 = 0, rr_r11 = 0, rr_r12 = 0; (void)rr_r8; (void)rr_r9; (void)rr_r10; (void)rr_r11; (void)rr_r12; rr_r8 = *(u32 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r9 = *(u32 *)((u8 *)(uintptr_t)((p)) + (4)); rr_r10 = *(u32 *)((u8 *)(uintptr_t)((p)) + (8)); rr_r11 = *(u32 *)((u8 *)(uintptr_t)((p)) + (12)); rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (16)); Psyz_GteCtrlWrite(0, (u32)rr_r8); Psyz_GteCtrlWrite(1, (u32)rr_r9); Psyz_GteCtrlWrite(2, (u32)rr_r10); Psyz_GteCtrlWrite(3, (u32)rr_r11); Psyz_GteCtrlWrite(4, (u32)rr_r12); } while (0)
#define RR_GTE_SETLIGHT_5W(p) do { u32 rr_r8 = 0, rr_r9 = 0, rr_r10 = 0, rr_r11 = 0, rr_r12 = 0; (void)rr_r8; (void)rr_r9; (void)rr_r10; (void)rr_r11; (void)rr_r12; rr_r8 = *(u32 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r9 = *(u32 *)((u8 *)(uintptr_t)((p)) + (4)); rr_r10 = *(u32 *)((u8 *)(uintptr_t)((p)) + (8)); rr_r11 = *(u32 *)((u8 *)(uintptr_t)((p)) + (12)); rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (16)); Psyz_GteCtrlWrite(8, (u32)rr_r8); Psyz_GteCtrlWrite(9, (u32)rr_r9); Psyz_GteCtrlWrite(10, (u32)rr_r10); Psyz_GteCtrlWrite(11, (u32)rr_r11); Psyz_GteCtrlWrite(12, (u32)rr_r12); } while (0)
#define RR_GTE_LDV0_2W(p) do { u32 rr_r8 = 0, rr_r9 = 0; (void)rr_r8; (void)rr_r9; rr_r8 = *(u32 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r9 = *(u32 *)((u8 *)(uintptr_t)((p)) + (4)); Psyz_GteDataWrite(0, (u32)rr_r8); Psyz_GteDataWrite(1, (u32)rr_r9); } while (0)
#define RR_GTE_STORE_IR_S16_T012(p) do { u32 rr_r8 = 0, rr_r9 = 0, rr_r10 = 0; (void)rr_r8; (void)rr_r9; (void)rr_r10; rr_r8 = Psyz_GteDataRead(9); rr_r9 = Psyz_GteDataRead(10); rr_r10 = Psyz_GteDataRead(11); *(u16 *)((u8 *)(uintptr_t)((p)) + (0)) = (u16)rr_r8; *(u16 *)((u8 *)(uintptr_t)((p)) + (2)) = (u16)rr_r9; *(u16 *)((u8 *)(uintptr_t)((p)) + (4)) = (u16)rr_r10; } while (0)
#define RR_GTE_SETCOL_5W(p) do { u32 rr_r8 = 0, rr_r9 = 0, rr_r10 = 0, rr_r11 = 0, rr_r12 = 0; (void)rr_r8; (void)rr_r9; (void)rr_r10; (void)rr_r11; (void)rr_r12; rr_r8 = *(u32 *)((u8 *)(uintptr_t)((p)) + (0)); rr_r9 = *(u32 *)((u8 *)(uintptr_t)((p)) + (4)); rr_r10 = *(u32 *)((u8 *)(uintptr_t)((p)) + (8)); rr_r11 = *(u32 *)((u8 *)(uintptr_t)((p)) + (12)); rr_r12 = *(u32 *)((u8 *)(uintptr_t)((p)) + (16)); Psyz_GteCtrlWrite(16, (u32)rr_r8); Psyz_GteCtrlWrite(17, (u32)rr_r9); Psyz_GteCtrlWrite(18, (u32)rr_r10); Psyz_GteCtrlWrite(19, (u32)rr_r11); Psyz_GteCtrlWrite(20, (u32)rr_r12); } while (0)
#define RR_GTE_SETTR_3W(p) do { u32 rr_r8 = 0, rr_r9 = 0, rr_r10 = 0; (void)rr_r8; (void)rr_r9; (void)rr_r10; rr_r8 = *(u32 *)((u8 *)(uintptr_t)((p)) + (20)); rr_r9 = *(u32 *)((u8 *)(uintptr_t)((p)) + (24)); rr_r10 = *(u32 *)((u8 *)(uintptr_t)((p)) + (28)); Psyz_GteCtrlWrite(5, (u32)rr_r8); Psyz_GteCtrlWrite(6, (u32)rr_r9); Psyz_GteCtrlWrite(7, (u32)rr_r10); } while (0)
/* RR_GTE_ROT_AVG3_STORE read stack-passed PsyQ arguments; the host uses PsyZ libgte instead. */
/* RR_GTE_ROT_AVG4_STORE read stack-passed PsyQ arguments; the host uses PsyZ libgte instead. */

#endif
