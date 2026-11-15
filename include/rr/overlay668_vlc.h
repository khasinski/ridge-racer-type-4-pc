#ifndef RR_OVERLAY668_VLC_H
#define RR_OVERLAY668_VLC_H
#include "types.h"

/* LIBPRESS DecDCTvlc as linked into the movie_playback image (0x80116958):
 * decodes a version 2 or 3 STR bitstream frame into MDEC run/level codes. */
typedef struct {
    u16 shift, size;
} Ovl668VlcDcCode;
extern const Ovl668VlcDcCode ovl668_vlc_dc_chroma[256];
extern const Ovl668VlcDcCode ovl668_vlc_dc_luma[256];
extern const u32 ovl668_vlc_ac_short[8192][2];
extern const u32 ovl668_vlc_ac_long[512];

/* Decoder state kept between partial calls (originally nine words at
 * 0x80116E64) and the end of the current MDEC command (0x80116E88). */
typedef struct {
    const u8 *input, *input_end;
    u16 *output;
    size_t bits_left;
    u32 window, bits, qscale, block;
    s32 dcCr, dcCb, dcY;
} Ovl668VlcState;
extern Ovl668VlcState ovl668_vlc_state;
extern u16 *ovl668_vlc_output_end;
/* Output limit in halfwords (DecDCTvlcSize); 0xFFFFFF = unlimited. */
extern u32 ovl668_vlc_max_halfwords;

/* bs != NULL starts a frame, NULL resumes the previous one. Returns 0 when
 * the frame is complete (the command is padded with 0xFE00), 1 when the
 * soft output limit stopped it, or -1 on invalid/truncated input or an
 * insufficient destination. Resuming uses the saved extents; pass zero sizes.
 * No write exceeds either the declared command size or output_bytes. */
s32 ovl668_mdec_decode_vlc(const void *bs, size_t input_bytes,
                         void *buf, size_t output_bytes);
#endif
