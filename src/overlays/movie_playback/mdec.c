#include "rr/overlay_module.h"
#include "rr/overlay668_decode.h"
#include "rr/overlay668_control.h"
#include "rr/overlay668_dma.h"
#include "rr/diagnostics.h"
#include "rr/overlay668_wait.h"
#include "types.h"
#include "rr/overlay668_vlc.h"
#include <stdint.h>

void ovl668_mdec_reset(s32 mode){if(mode==0)ResetCallback();ovl668_mdec_reset_hw(mode);}

u32 *ovl668_mdec_get_env(u32 *output){
 u32 *cursor=output,*source=ovl668_mdec_quant_cmd.luma;
 s32 n;
 for(n=15;n!=-1;n--)*cursor++=*source++;
 cursor=output+16;source=ovl668_mdec_quant_cmd.chroma;
 for(n=15;n!=-1;n--)*cursor++=*source++;
 cursor=output+32;source=ovl668_mdec_scale_cmd.matrix;
 for(n=31;n!=-1;n--)*cursor++=*source++;
 return output;
}

u32 *ovl668_mdec_put_env(u32 *input){
 u32 *source=input,*destination=ovl668_mdec_quant_cmd.luma;
 s32 n;
 for(n=15;n!=-1;n--)*destination++=*source++;
 destination=ovl668_mdec_quant_cmd.chroma;source=input+16;
 for(n=15;n!=-1;n--)*destination++=*source++;
 ovl668_mdec_start_in_dma(&ovl668_mdec_quant_cmd,32);
 ovl668_mdec_start_in_dma(&ovl668_mdec_scale_cmd,32);
 return input;
}

u32 ovl668_mdec_buf_size(const StrCommandHeader *header){return header->halves[0];}

void ovl668_mdec_in(StrCommandHeader *header,u32 flags){
 if(flags&1)header->word&=~0x08000000U;else header->word|=0x08000000U;
 if(flags&2)header->word|=0x02000000U;else header->word&=~0x02000000U;
 ovl668_mdec_start_in_dma(header,header->halves[0]);
}

void ovl668_mdec_out(void *output,u32 words){ovl668_mdec_start_out_dma(output,words);}

s32 ovl668_mdec_in_sync(s32 mode){
    s32 result;
    if(mode==0)result=ovl668_mdec_wait_in_dma();
    else result=(ovl668_mdec_read_status()>>29)&1;
    return result;
}

s32 ovl668_mdec_out_sync(s32 mode){
    s32 result;
    if(mode==0)result=ovl668_mdec_wait_out_dma();
    else result=(ovl668_mdec_read_status()>>24)&1;
    return result;
}

LibInterruptCallback ovl668_mdec_in_callback(LibInterruptCallback callback){return DMACallback(0,callback);}

LibInterruptCallback ovl668_mdec_out_callback(LibInterruptCallback callback){return DMACallback(1,callback);}

extern const char ovl668_mdec_reset_error_fmt[];

/* Mode 0 resets the decoder and reloads the overlay's quantization and
 * scale commands; mode 1 aborts the current transfer only. */
void ovl668_mdec_reset_hw(s32 mode) {
    switch (mode) {
    case 0:
        MDEC_reset(1);
        ovl668_mdec_start_in_dma(&ovl668_mdec_quant_cmd, 32);
        ovl668_mdec_start_in_dma(&ovl668_mdec_scale_cmd, 32);
        break;
    case 1:
        MDEC_reset(1);
        break;
    default:
        DebugPrintStub(ovl668_mdec_reset_error_fmt, mode);
        break;
    }
}

void ovl668_mdec_start_in_dma(const void *command,u32 words)
{
    ovl668_mdec_wait_in_dma();
    MDEC_in((void *)command, (int)words);
}

void ovl668_mdec_start_out_dma(void *output,u32 words)
{
    ovl668_mdec_wait_out_dma();
    MDEC_out(output, (int)words);
}

s32 ovl668_mdec_wait_in_dma(void)
{
    if (MDEC_in_sync() != 0) {
        ovl668_mdec_handle_timeout(ovl668_mdec_in_timeout_msg);
        return -1;
    }
    return 0;
}

s32 ovl668_mdec_wait_out_dma(void)
{
    if (MDEC_out_sync() != 0) {
        ovl668_mdec_handle_timeout(ovl668_mdec_out_timeout_msg);
        return -1;
    }
    return 0;
}

u32 ovl668_mdec_read_status(void) { return (u32)MDEC_status(); }

extern const char ovl668_mdec_timeout_fmt[];
/* Report the failed transfer and abort the decoder, as the original did. */
s32 ovl668_mdec_handle_timeout(const char *message) {
    DebugPrintStub(ovl668_mdec_timeout_fmt, message);
    MDEC_reset(1);
    return 0;
}

/* ---------------------------------------------------------------- DecDCTvlc
 * LIBPRESS DecDCTvlc of this image (originally hand-written MIPS at
 * 0x80116958). v0 is a 32-bit bit window over the little-endian halfword
 * stream, v1 the bits consumed from its low halfword; MDEC codes are
 * written as halfwords after the copied header word. */
Ovl668VlcState ovl668_vlc_state;
u16 *ovl668_vlc_output_end;
u32 ovl668_vlc_max_halfwords = 0xFFFFFF;

static u32 ovl668_vlc_read16(const u8 *p) { return (u32)p[0] | (u32)p[1] << 8; }
static u32 ovl668_vlc_sll(u32 value, u32 count) { return value << (count & 31); }
static u32 ovl668_vlc_srl(u32 value, u32 count) { return value >> (count & 31); }

/* The original decoder eagerly reads a halfword ahead even when the end
 * marker is already buffered. Supply zero lookahead at the physical end;
 * bits_left separately forbids consuming any bit outside the supplied data. */
static u32 ovl668_vlc_refill(const u8 **input, const u8 *end) {
    if ((size_t)(end - *input) < 2) return 0;
    u32 value = ovl668_vlc_read16(*input);
    *input += 2;
    return value;
}

/* Consume `count` bits and refill a halfword once 16 have been used. */
#define RR_VLC_ADVANCE(count) do { \
        u32 rr_n_ = (count); \
        if (bits_left < rr_n_) goto invalid; \
        bits_left -= rr_n_; \
        window = ovl668_vlc_sll(window, rr_n_); \
        bits += rr_n_; \
        if (bits & 0x10) { \
            bits &= 0xF; \
            window |= ovl668_vlc_sll(ovl668_vlc_refill(&input, input_end), bits); \
        } else { \
            bits &= 0xF; \
        } \
    } while (0)

s32 ovl668_mdec_decode_vlc(const void *bs, size_t input_bytes,
                         void *buf, size_t output_bytes) {
    const u8 *input, *input_end;
    u16 *output;
    u32 window, bits, qscale, block, code, extra;
    s32 dcCr, dcCb, dcY, dc;
    uintptr_t limit;
    size_t bits_left;

    if (bs == NULL) {
        if (!ovl668_vlc_state.input) return -1;
        input = ovl668_vlc_state.input;
        input_end = ovl668_vlc_state.input_end;
        bits_left = ovl668_vlc_state.bits_left;
        output = ovl668_vlc_state.output;
        window = ovl668_vlc_state.window;
        bits = ovl668_vlc_state.bits;
        qscale = ovl668_vlc_state.qscale;
        block = ovl668_vlc_state.block;
        dcCr = ovl668_vlc_state.dcCr;
        dcCb = ovl668_vlc_state.dcCb;
        dcY = ovl668_vlc_state.dcY;
        limit = (uintptr_t)output + (uintptr_t)ovl668_vlc_max_halfwords * 2;
        goto ac_code;
    }
    ovl668_vlc_state.input = NULL;
    if (!buf || input_bytes < 12 || input_bytes > PTRDIFF_MAX ||
        input_bytes > SIZE_MAX / 8 || (input_bytes & 1) || output_bytes < 4 ||
        (uintptr_t)buf % _Alignof(u16)) return -1;
    input_end = (const u8 *)bs + input_bytes;
    bits_left = (input_bytes - 8) * 8;
    block = 0;
    dcCr = dcCb = dcY = 0;
    output = buf;
    limit = (uintptr_t)output + (uintptr_t)ovl668_vlc_max_halfwords * 2;
    input = bs;
    {
        /* Header: MDEC command word, quantization scale, version, first bits. */
        u32 header = ovl668_vlc_read16(input) | ovl668_vlc_read16(input + 2) << 16;
        u32 version = ovl668_vlc_read16(input + 6);
        size_t command_bytes = 4 + (size_t)(header & 0xFFFF) * 4;
        if (command_bytes > output_bytes || (header >> 29) != 1 ||
            (version != 2 && version != 3) || ovl668_vlc_read16(input + 4) > 63)
            goto invalid;
        qscale = ovl668_vlc_read16(input + 4) << 10;
        if (version == 3) {
            block = 1; /* version 3: Huffman-coded DC, block counter Cr, Cb, Y0..Y3 */
        }
        window = ovl668_vlc_read16(input + 8) << 16 | ovl668_vlc_read16(input + 10);
        bits = 0;
        input += 12;
        output[0] = (u16)header;
        output[1] = (u16)(header >> 16);
        ovl668_vlc_output_end = output + (header & 0xFFFF) * 2 + 2;
        output += 1;
    }
block_start:
    if (bits_left < 10) goto invalid;
    code = window >> 22;
    output += 1;
    if (block == 0) {
        if (code == 0x1FF)
            goto pad;
        RR_VLC_ADVANCE(10);
        if (output >= ovl668_vlc_output_end) goto invalid;
        *output = (u16)(qscale | code);
    } else {
        const Ovl668VlcDcCode *entry;
        u32 shift, size;
        s32 diff = 0;
        if (code == 0x3FF)
            goto pad;
        entry = &(block >= 3 ? ovl668_vlc_dc_luma : ovl668_vlc_dc_chroma)[window >> 24];
        shift = entry->shift;
        size = entry->size;
        if (!shift || bits_left < shift + size) goto invalid;
        bits_left -= shift + size;
        window = ovl668_vlc_sll(window, shift);
        if (size != 0) {
            u32 rest = 32 - size;
            diff = (s32)ovl668_vlc_srl(window, rest);
            if ((s32)window >= 0) {
                diff -= (s32)ovl668_vlc_srl(0xFFFFFFFFu, rest);
            }
            window = ovl668_vlc_sll(window, size);
            bits += size;
        }
        bits += shift;
        if (bits & 0x10) {
            bits &= 0xF;
            window |= ovl668_vlc_sll(ovl668_vlc_refill(&input, input_end), bits);
        } else {
            bits &= 0xF;
        }
        if (block > 2) {
            dc = (dcY += diff);
        } else if (block == 2) {
            dc = (dcCb += diff);
        } else {
            dc = (dcCr += diff);
        }
        block++;
        if (output >= ovl668_vlc_output_end) goto invalid;
        *output = (u16)((((u32)dc << 2) & 0x3FF) | qscale);
        if (block == 7) {
            block -= 6;
        }
    }
    if ((intptr_t)((uintptr_t)output - limit) >= 0) {
        output += 1;
        goto save;
    }
    output += 1;
ac_code:
    {
        const u32 *entry = ovl668_vlc_ac_short[window >> 19];
        code = entry[0];
        if (code != 0) {
            extra = entry[1];
        } else {
            RR_VLC_ADVANCE(8);
            code = ovl668_vlc_ac_long[window >> 23];
            extra = 0;
        }
        if (!(code & 0xFF)) goto invalid;
        RR_VLC_ADVANCE(code & 0xFF);
        code >>= 16;
        if (code == 0x7C1F)
            goto escape;
        if (output >= ovl668_vlc_output_end) goto invalid;
        *output = (u16)code;
        if (code == 0xFE00)
            goto block_start;
        output += 1;
        if (extra == 0)
            goto ac_code;
        code = extra & 0xFFFF;
        if (code == 0x7C1F)
            goto escape;
        if (output >= ovl668_vlc_output_end) goto invalid;
        *output = (u16)code;
        if (code == 0xFE00)
            goto block_start;
        output += 1;
        code = extra >> 16;
        if (code == 0)
            goto ac_code;
        if (code == 0x7C1F)
            goto escape;
        if (output >= ovl668_vlc_output_end) goto invalid;
        *output = (u16)code;
        if (code == 0xFE00)
            goto block_start;
        output += 1;
        goto ac_code;
    }
escape:
    /* Escape: the next 16 bits are the run/level code itself. */
    if (bits_left < 16) goto invalid;
    bits_left -= 16;
    if (output >= ovl668_vlc_output_end) goto invalid;
    *output = (u16)(window >> 16);
    output += 1;
    window = window << 16 | ovl668_vlc_sll(ovl668_vlc_refill(&input, input_end), bits);
    goto ac_code;
pad:
    while ((intptr_t)((uintptr_t)output - (uintptr_t)ovl668_vlc_output_end) < 0) {
        *output = 0xFE00;
        output += 1;
    }
    ovl668_vlc_state.input = NULL;
    return 0;
invalid:
    ovl668_vlc_state.input = NULL;
    return -1;
save:
    ovl668_vlc_state.input = input;
    ovl668_vlc_state.input_end = input_end;
    ovl668_vlc_state.bits_left = bits_left;
    ovl668_vlc_state.output = output;
    ovl668_vlc_state.window = window;
    ovl668_vlc_state.bits = bits;
    ovl668_vlc_state.qscale = qscale;
    ovl668_vlc_state.block = block;
    ovl668_vlc_state.dcCr = dcCr;
    ovl668_vlc_state.dcCb = dcCb;
    ovl668_vlc_state.dcY = dcY;
    return 1;
}
#undef RR_VLC_ADVANCE

/* Initial values of the overlay globals this module owns. */
const char ovl668_mdec_reset_error_fmt[28] = {77, 68, 69, 67, 95, 114, 101, 115, 116, 58, 98, 97, 100, 32, 111, 112, 116, 105, 111, 110, 40, 37, 100, 41, 10};
char ovl668_mdec_in_timeout_msg[16] = {77, 68, 69, 67, 95, 105, 110, 95, 115, 121, 110, 99};
char ovl668_mdec_out_timeout_msg[16] = {77, 68, 69, 67, 95, 111, 117, 116, 95, 115, 121, 110, 99};
const char ovl668_mdec_timeout_fmt[20] = {37, 115, 32, 116, 105, 109, 101, 111, 117, 116, 58, 10};
StrQuantizationCommand ovl668_mdec_quant_cmd = {.header = {.word = 0x40000001u}, .luma = {0x13101002u, 0x16161310u, 0x16161616u, 0x1B1A181Au, 0x1A1A1B1Bu, 0x1B1B1A1Au, 0x1D1D1D1Bu, 0x1D222222u, 0x1B1B1D1Du, 0x20201D1Du, 0x26252222u, 0x22232325u, 0x28262623u, 0x30302828u, 0x38382E2Eu, 0x5345453Au}, .chroma = {0x13101002u, 0x16161310u, 0x16161616u, 0x1B1A181Au, 0x1A1A1B1Bu, 0x1B1B1A1Au, 0x1D1D1D1Bu, 0x1D222222u, 0x1B1B1D1Du, 0x20201D1Du, 0x26252222u, 0x22232325u, 0x28262623u, 0x30302828u, 0x38382E2Eu, 0x5345453Au}};
StrMatrixCommand ovl668_mdec_scale_cmd = {.header = {.word = 0x60000000u}, .matrix = {0x5A825A82u, 0x5A825A82u, 0x5A825A82u, 0x5A825A82u, 0x6A6D7D8Au, 0x18F8471Cu, 0xB8E3E707u, 0x82759592u, 0x30FB7641u, 0x89BECF04u, 0xCF0489BEu, 0x764130FBu, 0xE7076A6Du, 0xB8E38275u, 0x7D8A471Cu, 0x959218F8u, 0xA57D5A82u, 0x5A82A57Du, 0xA57D5A82u, 0x5A82A57Du, 0x8275471Cu, 0x6A6D18F8u, 0xE7079592u, 0xB8E37D8Au, 0x89BE30FBu, 0xCF047641u, 0x7641CF04u, 0x30FB89BEu, 0xB8E318F8u, 0x82756A6Du, 0x95927D8Au, 0xE707471Cu}};

/* Writable state of this unit; ovl668_reset_module restores it. */
static const OverlayStateRegion ovl668_mdec_regions[] = {
    RR_OVERLAY_STATE(ovl668_mdec_in_timeout_msg),
    RR_OVERLAY_STATE(ovl668_mdec_out_timeout_msg),
    RR_OVERLAY_STATE(ovl668_mdec_quant_cmd),
    RR_OVERLAY_STATE(ovl668_mdec_scale_cmd),
    RR_OVERLAY_STATE(ovl668_vlc_state),
    RR_OVERLAY_STATE(ovl668_vlc_output_end),
    RR_OVERLAY_STATE(ovl668_vlc_max_halfwords)};
const OverlayStateTable ovl668_mdec_state = {ovl668_mdec_regions, sizeof(ovl668_mdec_regions) / sizeof(ovl668_mdec_regions[0])};
