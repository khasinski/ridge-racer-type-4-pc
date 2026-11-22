#include "rr/release.h"
#include "scratchpad.h"
#include "rr/gpu_scratch_types.h"
#include "rr_asm.h"
#include "rr/sprite_preset.h"
#include "game/effect/spfade.h"
static const SpritePreset PresetSpriteDescriptorsJp[55];


void RampValueToLimit(unsigned short *a0, short *a1, int a2) {
    int v = *a1;
    unsigned short u = *(unsigned short *)a1;
    if (v > 0) {
        *a0 += u;
        if ((short)*a0 >= a2) {
            *a0 = a2;
            *a1 = 0;
        }
    } else if (v < 0) {
        *a0 += u;
        if ((short)*a0 <= 0) {
            *a0 = 0;
            *a1 = 0;
        }
    }
}

void RampValuePairTo4096(unsigned short *a0, unsigned short *a1, short *a2) {
    int v = *a2;
    unsigned short u = *(unsigned short *)a2;
    if (v > 0) {
        *a1 += u;
        if ((short)*a1 >= 0x1000) {
            *a1 = 0x1000;
            *a2 = 0;
        }
    } else if (v < 0) {
        *a0 += u;
        if ((short)*a0 <= 0) {
            *a0 = 0;
            *a2 = 0;
        }
    }
}

void RampValueTo4096(unsigned short *a0, short *a1) {
    int v = *a1;
    unsigned short u = *(unsigned short *)a1;
    if (v > 0) {
        *a0 += u;
        if ((short)*a0 >= 0x1000) {
            *a0 = 0x1000;
            *a1 = 0;
        }
    } else if (v < 0) {
        *a0 += u;
        if ((short)*a0 <= 0) {
            *a0 = 0;
            *a1 = 0;
        }
    }
}

/* Clamped grey sprite color word with the given GPU code. */
static u32 FadeColorWord(s32 value, u32 code) {
    value = value < 0 ? 0 : value > 255 ? 255 : value;
    return (code << 24) | ((u32)value << 16) | ((u32)value << 8) | (u32)value;
}

/* Link one sprite/tile command at ordering entry 1 of the current table. */
static void LinkFadeCommand(GpuPrimitiveHeader *packet, u32 length) {
    RR_GPU_ADD_PRIM(&RR_GPU_SCRATCH->order[1], packet, length);
}

/* Six 32x108 beam sprites whose spread is scale interpolated by the GTE
 * (INTPL of 0 towards 16 by IR0 = scale). */
void EmitTitleBeamFades(s32 intensity, s32 scale) {
    static const struct { s32 base, factor; u32 uv; s32 high, highFactor; } beams[6] = {
        {0x70, -3, 0x1BA40000U, 0x88, 1},
        {0x90, -1, 0x1BA40000U, 0x88, 1},
        {0xB0, 1, 0x1BA40000U, 0x88, 1},
        {0x70, -1, 0x1BA40020U, 0x88, -1},
        {0x90, 1, 0x1BA40020U, 0x88, -1},
        {0xB0, 3, 0x1BA40020U, 0x88, -1},
    };
    GpuSpriteCommand *packet;
    u32 color;
    s32 spread;
    s32 i;

    if (intensity != 0) {
        color = FadeColorWord(intensity, 0x66);
        RR_GTE_MTC2_IR0(scale);
        RR_GTE_MTC2_IR1(0);
        RR_GTE_CTC2_FCR(0x10);
        RR_GTE_INTPL();
        RR_GTE_MFC2(spread, 9);
        packet = (GpuSpriteCommand *)RR_GPU_SCRATCH->cursor;
        for (i = 0; i < 6; i++) {
            u32 y = (u32)(beams[i].high + beams[i].highFactor * spread);
            u32 x = (u32)(beams[i].base + beams[i].factor * spread);
            packet->words.color.word = color;
            packet->words.position = (y << 16) | x;
            packet->words.uv = beams[i].uv;
            packet->words.size = 0x006C0020U;
            LinkFadeCommand(&packet->header, 4);
            packet++;
        }
        RR_GPU_SCRATCH->cursor = (u8 *)packet;
    }
}

void EmitPresetSprite(s32 intensity,s32 index) {
    GpuSpriteCommand *packet;
    const SpritePreset *preset;
    if(intensity!=0) {
        packet=(GpuSpriteCommand *)RR_GPU_SCRATCH->cursor;
        packet->words.color.word=FadeColorWord(intensity,0x66);
        preset=&RR_PAL_JP(PresetSpriteDescriptors, PresetSpriteDescriptorsJp)[index];
        packet->words.position=preset->position;
        packet->words.size=preset->size;
        packet->words.uv=preset->uv;
        LinkFadeCommand(&packet->header,4);
        RR_GPU_SCRATCH->cursor=(u8 *)(packet+1);
    }
}

/* Semi-transparent tile at pos, 2 pixels high, width scaled by the GTE. */
void EmitFadeTile(s32 intensity, s32 scale, u32 pos, s32 width) {
    u32 interp;
    GpuTileCommand *packet;

    if (intensity != 0 && scale != 0) {
        RR_GTE_MTC2_IR0(scale);
        RR_GTE_MTC2_IR1(width);
        RR_GTE_CMD_2NOP(0x4B98003D);
        RR_GTE_MFC2(interp, 9);
        packet = (GpuTileCommand *)RR_GPU_SCRATCH->cursor;
        packet->words.color.word = FadeColorWord(intensity, 0x62);
        packet->words.position = pos;
        packet->words.size = interp | 0x20000U;
        LinkFadeCommand(&packet->header, 3);
        RR_GPU_SCRATCH->cursor = (u8 *)(packet + 1);
    }
}

void EmitTitleMaskSprites(s32 mask, s32 animation)
{
    GpuSpriteCommand *packet = (GpuSpriteCommand *)RR_GPU_SCRATCH->cursor;
    u32 uv = (((s32)(0U - (u32)animation) >> 3) & 3U) | 0x1BA4F0D0U;
    s16 *vertex = SpinningWheelBaseVerts;
    s32 remaining = RR_TITLE_MASK_SPRITE_COUNT;
    do {
        if (mask & 1) {
            packet->words.color.word = 0x65000000U;
            packet->words.position = *vertex | 0xA80000;
            packet->words.size = 0x100004;
            packet->words.uv = uv;
            LinkFadeCommand(&packet->header, 4);
            packet++;
        }
        mask >>= 1;
        remaining--;
        vertex++;
    } while (remaining > 0);
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
}

/* Initial values of the globals this module owns. */
SpritePreset PresetSpriteDescriptors[55] = {{.position = 0xFC006Du, .size = 0x120062u, .uv = 0x1BA40050u}, {.position = 0x480018u, .size = 0x1C0018u, .uv = 0x1BA41440u}, {.position = 0x480030u, .size = 0x1C002Cu, .uv = 0x1BA41474u}, {.position = 0x770018u, .size = 0x1C0074u, .uv = 0x1BA48000u}, {.position = 0x9E0018u, .size = 0x1C0074u, .uv = 0x1BA49C00u}, {.position = 0x480018u, .size = 0x1C0048u, .uv = 0x1BA41458u}, {.position = 0x770018u, .size = 0x1C0074u, .uv = 0x1BA4B800u}, {.position = 0x9E0018u, .size = 0x1C0074u, .uv = 0x1BA4D400u}, {.position = 0x480018u, .size = 0x1C0038u, .uv = 0x1BA43040u}, {.position = 0x770018u, .size = 0x1C0074u, .uv = 0x1BA48074u}, {.position = 0x9E0018u, .size = 0x1C0074u, .uv = 0x1BA49C74u}, {.position = 0xC50018u, .size = 0x1C0074u, .uv = 0x1BA4B874u}, {.position = 0xEC0018u, .size = 0x1C0074u, .uv = 0x1BA4D474u}, {.position = 0x500066u, .size = 0x120010u, .uv = 0x1B640040u}, {.position = 0x500076u, .size = 0x120062u, .uv = 0x1BA40050u}, {.position = 0xE8001Eu, .size = 0x100024u, .uv = 0x1BA4F000u}, {.position = 0xE80070u, .size = 0x100028u, .uv = 0x1BA4F024u}, {.position = 0xE800E5u, .size = 0x10001Cu, .uv = 0x1BA4F04Cu}, {.position = 0x980022u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xB8001Fu, .size = 0x100023u, .uv = 0x1BA4704Cu}, {.position = 0x980076u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xB80072u, .size = 0x100025u, .uv = 0x1BA47024u}, {.position = 0x9800E6u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xB800E3u, .size = 0x100022u, .uv = 0x1BA47000u}, {.position = 0xA0007Eu, .size = 0x1C0018u, .uv = 0x1BA41440u}, {.position = 0xA00096u, .size = 0x1C002Cu, .uv = 0x1BA41474u}, {.position = 0xC80092u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xE8008Fu, .size = 0x100023u, .uv = 0x1BA4704Cu}, {.position = 0xA0007Eu, .size = 0x1C0048u, .uv = 0x1BA41458u}, {.position = 0xC80092u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xE8008Eu, .size = 0x100025u, .uv = 0x1BA47024u}, {.position = 0xA00086u, .size = 0x1C0038u, .uv = 0x1BA43040u}, {.position = 0xC80092u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xE8008Fu, .size = 0x100022u, .uv = 0x1BA47000u}, {.position = 0xA20050u, .size = 0x1C0064u, .uv = 0x1BA44C40u}, {.position = 0xA200B8u, .size = 0x1C0038u, .uv = 0x1BA43078u}, {.position = 0x108006Du, .size = 0x120062u, .uv = 0x1BA40050u}, {.position = 0xC80020u, .size = 0x20006Cu, .uv = 0x3FE70000u}, {.position = 0xE80020u, .size = 0x1000A4u, .uv = 0x3FE72000u}, {.position = 0x980020u, .size = 0xC00A4u, .uv = 0x3FE73800u}, {.position = 0xA80020u, .size = 0xC00CCu, .uv = 0x3FE74800u}, {.position = 0xB80020u, .size = 0xC00CCu, .uv = 0x3FE75800u}, {.position = 0xC40020u, .size = 0x1000B4u, .uv = 0x3FE76400u}, {.position = 0xD40020u, .size = 0xC0094u, .uv = 0x3FE77400u}, {.position = 0xE40020u, .size = 0xC009Cu, .uv = 0x3FE78400u}, {.position = 0xF40020u, .size = 0xC0074u, .uv = 0x3FE79400u}, {.position = 0x1100020u, .size = 0x10007Cu, .uv = 0x3FE7B000u}, {.position = 0xC80020u, .size = 0x200048u, .uv = 0x2F280000u}, {.position = 0xE80020u, .size = 0x100088u, .uv = 0x2F282000u}, {.position = 0x980020u, .size = 0xC00C0u, .uv = 0x2F283800u}, {.position = 0xA80020u, .size = 0xC00E0u, .uv = 0x2F284800u}, {.position = 0xB80020u, .size = 0xC00B0u, .uv = 0x2F285800u}, {.position = 0xC40020u, .size = 0x1000F0u, .uv = 0x2F286400u}, {.position = 0xD40020u, .size = 0xC0090u, .uv = 0x2F287400u}, {.position = 0xF40020u, .size = 0x100038u, .uv = 0x2F289000u}};
/* The Japanese release's values (SLPS-01800). */
static const SpritePreset PresetSpriteDescriptorsJp[55] = {{.position = 0xFC006Du, .size = 0x120062u, .uv = 0x1BA40050u}, {.position = 0x480018u, .size = 0x1C0018u, .uv = 0x1BA41440u}, {.position = 0x480030u, .size = 0x1C002Cu, .uv = 0x1BA41474u}, {.position = 0x770018u, .size = 0x1C0074u, .uv = 0x1BA48000u}, {.position = 0x9E0018u, .size = 0x1C0074u, .uv = 0x1BA49C00u}, {.position = 0x480018u, .size = 0x1C0048u, .uv = 0x1BA41458u}, {.position = 0x770018u, .size = 0x1C0074u, .uv = 0x1BA4B800u}, {.position = 0x9E0018u, .size = 0x1C0074u, .uv = 0x1BA4D400u}, {.position = 0x480018u, .size = 0x1C0038u, .uv = 0x1BA43040u}, {.position = 0x770018u, .size = 0x1C0074u, .uv = 0x1BA48074u}, {.position = 0x9E0018u, .size = 0x1C0074u, .uv = 0x1BA49C74u}, {.position = 0xC50018u, .size = 0x1C0074u, .uv = 0x1BA4B874u}, {.position = 0xEC0018u, .size = 0x1C0074u, .uv = 0x1BA4D474u}, {.position = 0x500066u, .size = 0x120010u, .uv = 0x1B640040u}, {.position = 0x500076u, .size = 0x120062u, .uv = 0x1BA40050u}, {.position = 0xE8001Eu, .size = 0x100024u, .uv = 0x1BA4F000u}, {.position = 0xE80070u, .size = 0x100028u, .uv = 0x1BA4F024u}, {.position = 0xE800E5u, .size = 0x10001Cu, .uv = 0x1BA4F04Cu}, {.position = 0x980022u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xB80024u, .size = 0x100018u, .uv = 0x1BA4F0B8u}, {.position = 0x980076u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xB80078u, .size = 0x100018u, .uv = 0x1BA4F0A0u}, {.position = 0x9800E6u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xB800E8u, .size = 0x100018u, .uv = 0x1BA4F088u}, {.position = 0xA0007Eu, .size = 0x1C0018u, .uv = 0x1BA41440u}, {.position = 0xA00096u, .size = 0x1C002Cu, .uv = 0x1BA41474u}, {.position = 0xC80092u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xE80094u, .size = 0x100018u, .uv = 0x1BA4F0B8u}, {.position = 0xA0007Eu, .size = 0x1C0048u, .uv = 0x1BA41458u}, {.position = 0xC80092u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xE80094u, .size = 0x100018u, .uv = 0x1BA4F0A0u}, {.position = 0xA00086u, .size = 0x1C0038u, .uv = 0x1BA43040u}, {.position = 0xC80092u, .size = 0x10001Cu, .uv = 0x1BA4F06Cu}, {.position = 0xE80094u, .size = 0x100018u, .uv = 0x1BA4F088u}, {.position = 0xA20050u, .size = 0x1C0064u, .uv = 0x1BA44C40u}, {.position = 0xA200B8u, .size = 0x1C0038u, .uv = 0x1BA43078u}, {.position = 0x108006Du, .size = 0x120062u, .uv = 0x1BA40050u}, {.position = 0x600020u, .size = 0x20006Cu, .uv = 0x3FE70000u}, {.position = 0x800020u, .size = 0x1000A4u, .uv = 0x3FE72000u}, {.position = 0x980020u, .size = 0xC00A4u, .uv = 0x3FE73800u}, {.position = 0xA80020u, .size = 0xC00CCu, .uv = 0x3FE74800u}, {.position = 0xB80020u, .size = 0xC00CCu, .uv = 0x3FE75800u}, {.position = 0xC40020u, .size = 0x1000B4u, .uv = 0x3FE76400u}, {.position = 0xD40020u, .size = 0xC0094u, .uv = 0x3FE77400u}, {.position = 0xE40020u, .size = 0xC009Cu, .uv = 0x3FE78400u}, {.position = 0xF40020u, .size = 0xC0074u, .uv = 0x3FE79400u}, {.position = 0x1100020u, .size = 0x10007Cu, .uv = 0x3FE7B000u}, {.position = 0x600020u, .size = 0x200048u, .uv = 0x2F280000u}, {.position = 0x800020u, .size = 0x100088u, .uv = 0x2F282000u}, {.position = 0x980020u, .size = 0xC00C0u, .uv = 0x2F283800u}, {.position = 0xA80020u, .size = 0xC00E0u, .uv = 0x2F284800u}, {.position = 0xB80020u, .size = 0xC00B0u, .uv = 0x2F285800u}, {.position = 0xC40020u, .size = 0x1000F0u, .uv = 0x2F286400u}, {.position = 0xD40020u, .size = 0xC0090u, .uv = 0x2F287400u}, {.position = 0xF40020u, .size = 0x100038u, .uv = 0x2F289000u}};
s16 SpinningWheelBaseVerts[RR_TITLE_MASK_SPRITE_COUNT] = {46, 80, 88, 96, 130, 164, 172, 180, 214, 242, 270};
