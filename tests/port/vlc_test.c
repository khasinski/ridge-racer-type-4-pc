#include "rr/overlay668_vlc.h"
#include "rr/overlay668_frame_acquire.h"
#include "rr/scene_camera_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

/* Independent STR encoder for known DC, escape and EOB tokens. Halfwords
 * are little endian on disc, with bits consumed from their most significant
 * end. No retail fixture or decoder table is needed to form the expectation. */
typedef struct { u8 bytes[256]; unsigned bits; } Encoded;
static void le16(u8 *p, unsigned value) { p[0] = (u8)value; p[1] = (u8)(value >> 8); }
static void bits(Encoded *e, unsigned value, unsigned count) {
    for (unsigned i = count; i; --i) {
        unsigned n = e->bits++, bit = 15 - n % 16;
        e->bytes[8 + n / 16 * 2 + bit / 8] |= ((value >> (i - 1)) & 1) << (bit % 8);
    }
}
static size_t encode(Encoded *e, unsigned version, u16 *expected) {
    memset(e, 0, sizeof(*e));
    le16(e->bytes, 9); le16(e->bytes + 2, 0x3800);
    le16(e->bytes + 4, 3); le16(e->bytes + 6, version);
    expected[0] = 9; expected[1] = 0x3800;
    const unsigned dc[] = {0, 1, 1023, 512, 317, 25};
    for (unsigned i = 0; i < 6; ++i) {
        if (version == 2) bits(e, dc[i], 10);
        else if (i < 2) bits(e, 0, 2); /* chroma DC difference 0 */
        else bits(e, 4, 3);            /* luma DC difference 0 */
        bits(e, 1, 6);                 /* escape */
        bits(e, 0x0402 + i, 16);       /* a run/level pair */
        bits(e, 2, 2);                 /* EOB */
        expected[2 + i * 3] = 3 * 1024 + (version == 2 ? dc[i] : 0);
        expected[3 + i * 3] = 0x0402 + i;
        expected[4 + i * 3] = 0xFE00;
    }
    bits(e, version == 2 ? 0x1FF : 0x3FF, 10);
    return 8 + ((e->bits + 15) / 16 + 2) * 2; /* refill lookahead */
}
static unsigned random_word(unsigned *seed) {
    return *seed = *seed * 1664525u + 1013904223u;
}
static int invalid_frame_is_released(void) {
    /* Exercise the actual stream ring and playback consumer. None of these
     * rejected frames may become the selected MDEC command buffer. */
    u32 ring[RR_STR_RING_SECTORS * 512];
    u32 commands[2][RR_STR_COMMAND_BYTES / 4];
    StrPlaybackState state = {0};
    void *previous = RaceContext.streamStateB8;
    state.width00 = 320; state.height04 = 176;
    state.commands10[0] = (StrCommandHeader *)commands[0];
    state.commands10[1] = (StrCommandHeader *)commands[1];
    RaceContext.streamStateB8 = &state;
    for (unsigned variant = 0; variant < 10; ++variant) {
        memset(ring, 0, sizeof(ring));
        memset(commands, 0xA5, sizeof(commands));
        StSetRing((LibQueueRecord32 *)ring, RR_STR_RING_SECTORS);
        LibQueueRecord32 *header = (LibQueueRecord32 *)ring;
        header->state = 2; header->sectors06 = 1;
        header->width10 = 320; header->height12 = 176; header->frameBytes0C = 12;
        if (variant == 0) header->width10 = 0;
        if (variant == 1) header->width10 = 336;
        if (variant == 2) header->width10 = 319;
        if (variant == 3) header->height12 = 0;
        if (variant == 4) header->height12 = 256;
        if (variant == 5) header->height12 = 175;
        if (variant == 6) header->frameBytes0C = 11;
        if (variant == 7) header->frameBytes0C = 2017;
        u8 *payload = (u8 *)(header + RR_STR_RING_SECTORS);
        le16(payload + 2, 0x3800);
        le16(payload + 6, variant == 8 ? 4 : 3);
        /* Variant 9 declares no output words but contains a DC coefficient. */
        CHECK(ovl668_decode_next_frame(&state.display24) == -2);
        CHECK(header->state == 0); /* claimed slot released on every error */
        CHECK(state.display24.word00 == 0);
        for (unsigned i = 0; i < RR_STR_COMMAND_BYTES / 4; ++i)
            CHECK(commands[0][i] == 0xA5A5A5A5); /* active commands untouched */
    }
    StSetRing(NULL, 0);
    RaceContext.streamStateB8 = previous;
    return 0;
}

int main(void) {
    CHECK(ovl668_mdec_decode_vlc(NULL, 0, NULL, 0) == -1);
    for (unsigned version = 2; version <= 3; ++version) {
        Encoded e; u16 expected[20], output[22];
        size_t len = encode(&e, version, expected);
        for (unsigned partial = 0; partial < 10; ++partial) {
            memset(output, 0xA5, sizeof(output));
            ovl668_vlc_max_halfwords = partial ? partial - 1 : 0xFFFFFF;
            int result = ovl668_mdec_decode_vlc(e.bytes, len, output + 1, sizeof(expected));
            unsigned calls = 0;
            while (result == 1 && calls++ < 20)
                result = ovl668_mdec_decode_vlc(NULL, 0, NULL, 0);
            CHECK(result == 0);
            CHECK(!memcmp(output + 1, expected, sizeof(expected)));
            CHECK(output[0] == 0xA5A5 && output[21] == 0xA5A5);
            CHECK(ovl668_mdec_decode_vlc(NULL, 0, NULL, 0) == -1);
        }
        ovl668_vlc_max_halfwords = 0xFFFFFF;
        size_t exact = 8 + ((e.bits + 15) / 16) * 2;
        /* Version 3 reaches EOF with a speculative halfword refill beyond
         * this exact extent. It must complete without reading that halfword. */
        CHECK(ovl668_mdec_decode_vlc(e.bytes, exact, output + 1, sizeof(expected)) == 0);
        CHECK(!memcmp(output + 1, expected, sizeof(expected)));
        for (size_t n = 0; n < len; ++n) {
            u8 *short_input = malloc(n ? n : 1);
            CHECK(short_input != NULL);
            memcpy(short_input, e.bytes, n);
            memset(output, 0xA5, sizeof(output));
            int result = ovl668_mdec_decode_vlc(short_input, n, output + 1, sizeof(expected));
            CHECK(result == 0 || result == -1);
            if (result == 0) CHECK(!memcmp(output + 1, expected, sizeof(expected)));
            if (n < exact) CHECK(result == -1);
            CHECK(output[0] == 0xA5A5 && output[21] == 0xA5A5);
            free(short_input);
        }
        for (size_t n = 0; n < sizeof(expected); ++n) {
            memset(output, 0xA5, sizeof(output));
            CHECK(ovl668_mdec_decode_vlc(e.bytes, len, output + 1, n) == -1);
            for (unsigned i = 0; i < 22; ++i) CHECK(output[i] == 0xA5A5);
        }
        /* The declared command ends before the supplied codes do. */
        le16(e.bytes, 1);
        memset(output, 0xA5, sizeof(output));
        CHECK(ovl668_mdec_decode_vlc(e.bytes, len, output + 1, sizeof(expected)) == -1);
        for (unsigned i = 5; i < 22; ++i) CHECK(output[i] == 0xA5A5);
        CHECK(ovl668_mdec_decode_vlc(NULL, 0, NULL, 0) == -1);
        le16(e.bytes, 0xFFFF);
        CHECK(ovl668_mdec_decode_vlc(e.bytes, len, output + 1, sizeof(expected)) == -1);
    }
    unsigned seed = 0x752B09F1, completed = 0, refused = 0;
    for (unsigned trial = 0; trial < 20000; ++trial) {
        size_t input_bytes = random_word(&seed) % 257;
        size_t output_bytes = random_word(&seed) % 513;
        u8 *input = malloc(input_bytes ? input_bytes : 1);
        u8 *output = malloc(output_bytes ? output_bytes : 1);
        CHECK(input && output);
        for (size_t i = 0; i < input_bytes; ++i) input[i] = random_word(&seed) >> 24;
        memset(output, 0xA5, output_bytes);
        unsigned words = random_word(&seed) % 65;
        if (input_bytes >= 8) {
            le16(input, words); le16(input + 2, 0x3800);
            le16(input + 4, trial % 64); le16(input + 6, 2 + (trial & 1));
        }
        ovl668_vlc_max_halfwords = trial % 8 ? trial % 8 : 0xFFFFFF;
        int result = ovl668_mdec_decode_vlc(input, input_bytes, output, output_bytes);
        unsigned calls = 0;
        while (result == 1 && calls++ < output_bytes + 1)
            result = ovl668_mdec_decode_vlc(NULL, 0, NULL, 0);
        CHECK(result == 0 || result == -1);
        completed += result == 0; refused += result == -1;
        for (size_t i = 4 + words * 4; i < output_bytes; ++i) CHECK(output[i] == 0xA5);
        CHECK(ovl668_mdec_decode_vlc(NULL, 0, NULL, 0) == -1);
        free(input); free(output);
    }
    CHECK(invalid_frame_is_released() == 0);
    printf("VLC: versions 2/3, partial resumes, truncation, output bounds; fuzz completed=%u refused=%u\n", completed, refused);
    return 0;
}
