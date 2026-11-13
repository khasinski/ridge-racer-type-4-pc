#include "rr/frame_interp.h"
#include "rr/widescreen.h"
#include "rr/release.h"
#include <string.h>
#include "rr/frame_storage_types.h"
#include "rr/gpu.h"
#include "rr/gpu_scratch_types.h"
#include "game/camera/viewcam.h"
#include "types.h"
#include "scratchpad.h"
#include "rr_asm.h"
#include "game/gfx/gtepoly1.h"
#include "rr/rro_model.h"
#include "rr/rro_effects.h"
#include "game/data/cdfile2.h"
#include "rr/fade_sprite.h"
#include "rr/audio_cd_controller.h"
#include "game/data/globals.h"
#include "game/data/screenframe.h"
#include "game/car/bounds.h"
#include "rr/audio_loop.h"
#include "psyq/libc/rand.h"

/* Car trail rings: the scalar state word's halves and the vertex buffer. */
typedef struct {
    s16 unk0;
    u16 unk2;
    RroProjectedVertex *verts;
} TrailEntry4;

typedef struct {
    s16 unk0;
    s16 unk2;
    Vec3sFields8 *verts;
} TrailEntry8;

extern int MaskBitSetting;
extern char UnknownRroTypeFmt[];
extern signed char RacePaused;
extern RroEffectColorTable RroEffectColorPresets;
extern int SpotlightConeRingInner[];

/* The car effect workspace (PAL scratchpad+0x7C); see rro_effects.h. */
static RroEffectWork RroEffectWorkArea;

#define RRO_SCENE ((RroSceneScratchPrefix *)SCRATCHPAD)
#define RRO_DRAW_MODE 0xE1000220u
#define RRO_MASK_COMMAND 0xE6000000u

/* Packed index words: the low half is unsigned, the high half signed. */
static const Vec3s8 *RroLowVertex(const Vec3s8 *base, s32 word) {
    return base + (u16)word;
}

static const Vec3s8 *RroHighVertex(const Vec3s8 *base, s32 word) {
    return base + (s16)(word >> 16);
}

static void RroStageQuad(RroRenderWork *work, const RroQuadIndices *indices) {
    const Vec3s8 *vertices = RroScene.workVertices;
    work->vertices28[0] = *RroLowVertex(vertices, indices->words[0]);
    work->vertices28[1] = *RroHighVertex(vertices, indices->words[0]);
    work->vertices28[2] = *RroLowVertex(vertices, indices->words[1]);
    work->vertices28[3] = *RroHighVertex(vertices, indices->words[1]);
}

/* Some X and some Y must lie on each side of the screen boundaries. */
static int RroQuadOnScreen(const RroScreenPosition *p, s32 yLimit) {
    s32 x0 = p[0].xy.x, x1 = p[1].xy.x, x2 = p[2].xy.x, x3 = p[3].xy.x;
    s32 y0 = p[0].xy.y, y1 = p[1].xy.y, y2 = p[2].xy.y, y3 = p[3].xy.y;
    if (!(x0 >= WIDE_SCREEN_LEFT || x1 >= WIDE_SCREEN_LEFT || x2 >= WIDE_SCREEN_LEFT || x3 >= WIDE_SCREEN_LEFT)) return 0;
    if (!(x0 < WIDE_SCREEN_RIGHT || x1 < WIDE_SCREEN_RIGHT || x2 < WIDE_SCREEN_RIGHT || x3 < WIDE_SCREEN_RIGHT)) return 0;
    if (!(y0 >= 0 || y1 >= 0 || y2 >= 0 || y3 >= 0)) return 0;
    return y0 < yLimit || y1 < yLimit || y2 < yLimit || y3 < yLimit;
}

/* Projects the staged quad. Returns nonzero when it is in depth range and
 * front-facing; depth18 then holds the shifted depth and projected50 all four
 * screen positions. The next quad is staged before visibility is tested. */
static int RroProjectStagedQuad(RroRenderWork *work, const RroQuadIndices *next, s32 depthShift) {
    RR_GTE14_LWC2_V012(&work->vertices28[0]);
    RR_GTE14_RTPT();
    RR_GTE14_NCLIP();
    RR_GTE14_SWC2_MAC0(&work->firstArea1C);
    RR_GTE14_SWC2_SXYZ(&work->projected50[0].word);
    RR_GTE14_LWC2_V0(&work->vertices28[3]);
    RR_GTE14_RTPS();
    RR_GTE14_AVSZ4();
    RR_GTE14_SWC2_OTZ(&work->depth18);
    RroStageQuad(work, next);
    if (work->depth18 <= 0) return 0;
    work->depth18 >>= depthShift;
    if (work->depth18 >= 0x1C0) return 0;
    RR_GTE14_NCLIP();
    RR_GTE14_SWC2_MAC0(&work->secondArea20);
    if (!(work->firstArea1C > 0 || work->secondArea20 < 0)) return 0;
    RR_GTE14_SWC2_SXY2(&work->projected50[3].word);
    return 1;
}

const u32 *EmitRroRawTexturedQuads(const u32 *commands, RroRenderWork *work, s32 count) {
    const RroRawTexturedQuad *quad = (const RroRawTexturedQuad *)commands;
    GpuPolyFT4 *packet = (GpuPolyFT4 *)RR_GPU_SCRATCH->cursor;
    s32 depthShift = RRO_SCENE->depthShift60;

    RroStageQuad(work, &quad->vertices);
    while (count-- > 0) {
        if (RroProjectStagedQuad(work, &quad[1].vertices, depthShift)) {
            RR_SXY_COPY(packet->words.xy0, work->projected50[0].word);
            RR_SXY_COPY(packet->words.xy1, work->projected50[1].word);
            RR_SXY_COPY(packet->words.xy2, work->projected50[2].word);
            RR_SXY_COPY(packet->words.xy3, work->projected50[3].word);
            if (RroQuadOnScreen(work->projected50, RR_PAL_JP(0x200, 0x1E0))) {
                packet->words.color04 = 0x2F000000;
                packet->words.uv0 = quad->uv08;
                packet->words.uv1 = quad->uv0C;
                packet->words.uv2 = quad->uv10;
                packet->words.uv3 = quad->uv10 >> 16;
                RR_GPU_ADD_PRIM(RroScene.workOrdering + work->depth18 + quad->orderingBias15, packet, 9);
                INTERP_TAG(packet, quad, 0);
                packet++;
            }
        }
        quad++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
    return (const u32 *)quad;
}

const u32 *EmitRroLitGouraudQuads(const u32 *commands, RroRenderWork *work, s32 count) {
    const RroLitGouraudQuad *quad = (const RroLitGouraudQuad *)commands;
    GpuMaskedG4Packet *packet = (GpuMaskedG4Packet *)RR_GPU_SCRATCH->cursor;
    s32 depthShift = RRO_SCENE->depthShift60;

    RroStageQuad(work, &quad->vertices);
    while (count-- > 0) {
        if (RroProjectStagedQuad(work, &quad[1].vertices, depthShift)) {
            packet->xy0C = work->projected50[0].word;
            packet->xy14 = work->projected50[1].word;
            packet->xy1C = work->projected50[2].word;
            packet->xy24 = work->projected50[3].word;
            if (RroQuadOnScreen(work->projected50, RR_PAL_JP(0x200, 0x1E0))) {
                const Vec3s8 *normals = RroScene.workNormals;
                RR_GTE14_LWC2_V0_V1_V2(RroLowVertex(normals, quad->normals.words[0]),
                                       RroHighVertex(normals, quad->normals.words[0]),
                                       RroLowVertex(normals, quad->normals.words[1]));
                RR_GTE14_LWC2_RGB(&quad->color10);
                RR_GTE14_LIGHT0();
                work->normal48 = *RroHighVertex(normals, quad->normals.words[1]);
                RR_GTE14_LIGHT1();
                RR_GPU_ADD_PRIM(RroScene.workOrdering + work->depth18 + quad->orderingBias15, packet, 11);
                INTERP_TAG_SHAPE(&packet->color08, quad, 0, 0);
                packet->setMask04 = RRO_MASK_COMMAND | 1;
                packet->nop = RR_GPU_MERGED_PRIM_NOP;
                packet->restoreMask28 = RRO_MASK_COMMAND | MaskBitSetting;
                RR_GTE14_LIGHT2();
                RR_GTE_MFC2(packet->color08, 20);
                RR_GTE_MFC2(packet->color10, 21);
                RR_GTE_MFC2(packet->color18, 22);
                RR_GTE14_LWC2_V0(&work->normal48);
                RR_GTE14_LIGHT0();
                RR_GTE_MFC2(packet->color20, 22);
                packet++;
            }
        }
        quad++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
    return (const u32 *)quad;
}

/* Converts the GTE's IR1/IR2 normal projection into one reflection UV word. */
static u32 RroEnvMapUV(s32 envVShift) {
    s32 envU, envV;
    RR_GTE_MFC2_IR1_IR2(envU, envV);
    envU = (envU >> 6) + 0xC0;
    envV = (envV >> envVShift) + 0xC0;
    return ((u32)envV << 8) | (u32)envU;
}

/* The original tested the byte after the head packet's tag (the low byte of
 * its first payload word) before inserting the mask restore command. */
static u8 RroHeadPayloadByte(const GpuDmaTag *head) {
    if (head->address == RR_GPU_END_LINK) return 0;
    return *(const u8 *)((const GpuDmaTag *)head->address + 1);
}

const u32 *EmitRroEnvMapQuads(const u32 *commands, RroRenderWork *work, s32 count) {
    const RroEnvMapQuad *quad = (const RroEnvMapQuad *)commands;
    u8 *cursor = RR_GPU_SCRATCH->cursor;
    s32 depthShift = RRO_SCENE->depthShift60;
    u32 restoreMask = MaskBitSetting | RRO_MASK_COMMAND;
    s32 envVShift = CurrentDrawBuf->buffer.environment00.draw00.clip.h == RR_PAL_JP(0x100, 0xF0) ? 6 : 7;

    RroStageQuad(work, &quad->vertices);
    while (count-- > 0) {
        GpuEnvMapEmission *e = (GpuEnvMapEmission *)cursor;
        if (RroProjectStagedQuad(work, &quad[1].vertices, depthShift)) {
            RR_SXY_COPY(e->primary.body.xy0, work->projected50[0].word);
            RR_SXY_COPY(e->reflected.body.xy0, work->projected50[0].word);
            RR_SXY_COPY(e->primary.body.xy1, work->projected50[1].word);
            RR_SXY_COPY(e->reflected.body.xy1, work->projected50[1].word);
            RR_SXY_COPY(e->primary.body.xy2, work->projected50[2].word);
            RR_SXY_COPY(e->reflected.body.xy2, work->projected50[2].word);
            RR_SXY_COPY(e->primary.body.xy3, work->projected50[3].word);
            RR_SXY_COPY(e->reflected.body.xy3, work->projected50[3].word);
            if (RroQuadOnScreen(work->projected50, RR_PAL_JP(0x200, 0x1E0))) {
                const Vec3s8 *normals = RroScene.workNormals;
                GpuDmaTag *head;
                u32 control;

                RR_GTE14_LWC2_V0_V1_V2(RroLowVertex(normals, quad->normals.words[0]),
                                       RroHighVertex(normals, quad->normals.words[0]),
                                       RroLowVertex(normals, quad->normals.words[1]));
                RR_GTE_OP(0x4A4A6412);
                work->normal48 = *RroHighVertex(normals, quad->normals.words[1]);
                RR_GTE_SQR12();
                RR_GTE_SQR12();
                RR_GTE_OP(0x4B38041C);
                RR_GTE_OP(0x4A4AE412);
                e->primary.body.uv0 = (u32)quad->uv10 + work->flags08;
                e->primary.body.uv1 = quad->uv14;
                RR_GTE_SQR12();
                RR_GTE_SQR12();
                RR_GTE_OP(0x4B38041C);
                RR_GTE_OP(0x4A4B6412);
                e->primary.body.uv2 = quad->uv18;
                e->primary.body.uv3 = quad->uv18 >> 16;
                RR_GTE_SQR12();
                RR_GTE_SQR12();
                RR_GTE_OP(0x4B38041C);
                RR_GTE_MFC2(e->primary.body.color0.word, 20);
                RR_GTE_MFC2(e->primary.body.color1.word, 21);
                RR_GTE_MFC2(e->primary.body.color2.word, 22);

                head = RroScene.workOrdering + work->depth18 + quad->orderingBias1D;
                control = quad->control1E;
                if ((control & 3) == 0) {
                    GpuEnvMapDirect *direct = (GpuEnvMapDirect *)cursor;
                    RR_GTE14_LWC2_V0(&work->normal48);
                    RR_GTE14_LIGHT0();
                    e->primary.body.color0.bytes.code = control >> 1;
                    RR_GTE_MFC2(e->primary.body.color3.word, 22);
                    /* Compact the staged body behind a bare GT4 header. */
                    memmove(&direct->body, &e->primary.body, sizeof(direct->body));
                    RR_GPU_ADD_PRIM(head, direct, 12);
                    INTERP_TAG(direct, quad, 0);
                    cursor += sizeof(GpuEnvMapDirect);
                } else {
                    e->primary.setMask = RRO_MASK_COMMAND;
                    e->primary.nop = RR_GPU_MERGED_PRIM_NOP;
                    e->reflected.setMask = RRO_MASK_COMMAND | (control & 3);
                    e->reflected.nop = RR_GPU_MERGED_PRIM_NOP;
                    RR_GTE_MFC2(e->reflected.body.color0.word, 20);
                    RR_GTE_MFC2(e->reflected.body.color1.word, 21);
                    RR_GTE_MFC2(e->reflected.body.color2.word, 22);
                    e->reflected.body.color0.bytes.code = 0x3E;
                    RR_GTE_OP(0x4A486012);
                    if (RroHeadPayloadByte(head) != 0xE6) {
                        e->restore.command = restoreMask;
                        RR_GPU_ADD_PRIM(head, &e->restore, 1);
                    }
                    RR_GPU_ADD_PRIM(head, &e->reflected, 14);
                    RR_GPU_ADD_PRIM(head, &e->primary, 14);
                    INTERP_TAG_SHAPE(&e->primary.body, quad, 0, 0);
                    INTERP_TAG_SHAPE(&e->reflected.body, quad, 1, INTERP_ANY_TEXTURE);
                    e->reflected.body.uv0 = RroEnvMapUV(envVShift) | work->envClut0C;

                    RR_GTE14_LWC2_V0(&work->normal48);
                    RR_GTE14_LIGHT0();
                    RR_GTE_MFC2(e->primary.body.color3.word, 22);
                    RR_GTE_MFC2(e->reflected.body.color3.word, 22);
                    RR_GTE_OP(0x4A48E012);
                    {
                        u32 uv1;
                        s32 envU, envV;
                        RR_GTE_MFC2_IR1_IR2(envU, envV);
                        RR_GTE_OP(0x4A496012);
                        uv1 = ((u32)((envV >> envVShift) + 0xC0) << 8) | (u32)((envU >> 6) + 0xC0);
                        e->reflected.body.uv1 = uv1 | work->envTpage10;
                        RR_GTE_MFC2_IR1_IR2(envU, envV);
                        RR_GTE_OP(0x4A486012);
                        e->reflected.body.uv2 = ((u32)((envV >> envVShift) + 0xC0) << 8) | (u32)((envU >> 6) + 0xC0);
                    }
                    e->reflected.body.uv3 = RroEnvMapUV(envVShift);
                    cursor += sizeof(GpuEnvMapEmission);
                }
            }
        }
        quad++;
    }
    RR_GPU_SCRATCH->cursor = cursor;
    return (const u32 *)quad;
}

void SetModelTexturePage(s32 page) {
    ModelTexturePage = page;
}

int DrawRroModel(const Vector *position, MatrixWords32 *rotation, s32 object, s32 flags)
{
    RroRenderWork *work = &((RroModelScratch *)SCRATCHPAD)->work7C.render;
    RroSceneScratchPrefix *scene = RRO_SCENE;
    const u32 *commands;
    s32 command, count;
    u32 textureX, textureY;

    if (ModelTexturePage < 0) {
        return 0;
    }
    SetGteObjectTransform(&((RroModelScratch *)SCRATCHPAD)->work7C.transform,
                          (const s16 *)position, rotation->w);
    work->flags08 = flags;
    textureX = (ModelTexturePage & 0xF) << 6;
    textureY = (ModelTexturePage & 0x10) << 4;
    work->envClut0C = (((textureY + 0x7F) << 6) | ((textureX >> 4) | 3)) << 16;
    work->envTpage10 = ((textureY >> 4) | (textureX >> 6) | 0x20) << 16;
    RroScene.workVertices = RroScene.vertices;
    RroScene.workNormals = RroScene.normals;
    RroScene.workOrdering = RR_GPU_SCRATCH->order + 0x80;
    commands = RroScene.commands[object].pointer;
    while ((command = *commands++) != 0) {
        count = command >> 16;
        command &= 0xFFFF;
        switch (command) {
        case 0:
            commands += count * 4;
            break;
        case 1:
            commands += count * 6;
            break;
        case 2:
            commands = EmitRroLitGouraudQuads(commands, work, count);
            break;
        case 3:
            RR_GTE_MTC2_RGB(0x3C808080);
            commands = EmitRroEnvMapQuads(commands, work, count);
            break;
        default:
            return DebugPrintStub(UnknownRroTypeFmt, command, count);
        }
    }
    return 1;
}

void DrawSparkLines(RroSpark *sparks, RroEffectWork *work, s32 speed) {
    RroSparkPacket *packet = (RroSparkPacket *)RR_GPU_SCRATCH->cursor;
    GpuDmaTag *ordering = RR_GPU_SCRATCH->order + 0x6C;
    s32 depthShift = RRO_SCENE->depthShift60 + 2;
    s32 spawnGate, launchZ, remaining;

    RR_GTE_SETFC_BYTES3(&work->colors15C.palette.rgb160[5]);
    RR_GTE_MTC2_RGB(0x42000000);
    spawnGate = PsyqRand() & 3;
    launchZ = (s32)(0U - (0U + speed + 0xA0));
    for (remaining = 0x10; remaining > 0; remaining--, sparks++) {
        RroSparkMotion *motion = &sparks->motion;
        s32 life = (s32)(sparks->timer - 0xAAU);
        s32 x, y, z, velocityX, velocityY, velocityZ, depth;

        if (life <= 0) {
            s32 dx, dy;
            if (spawnGate != 0) {
                continue;
            }
            spawnGate = 1;
            life = 0x1000;
            dx = (PsyqRand() & 0x1FF) - 0x100;
            dy = (PsyqRand() & 0x1FF) - 0x100;
            if (dx == 0) dx = 0x100;
            if (dy == 0) dy = 0x100;
            motion->fields.x = dx;
            motion->fields.y = dy + 0x140;
            motion->fields.z = (PsyqRand() & 0x7F) - 0xC30;
            motion->fields.velocityX = dx / 2;
            motion->fields.velocityY = dy / 2;
            motion->fields.velocityZ = (s16)launchZ;
        }
        sparks->timer = life;
        RR_GTE_MTC2_IR0(life);
        RR_GTE_OP(0x4A780010);
        RR_GTE_MFC2(packet->color08, 22);

        x = motion->fields.x;
        y = motion->fields.y;
        z = motion->fields.z;
        velocityX = motion->fields.velocityX;
        velocityY = motion->fields.velocityY;
        velocityZ = motion->fields.velocityZ;
        /* The line runs from the position to one eighth of the velocity ahead. */
        work->verticesE0[0].fields.x = x >> 4;
        work->verticesE0[0].fields.y = y >> 4;
        work->verticesE0[0].fields.z = z >> 4;
        RR_GTE14_LWC2_V1(&work->verticesE0[0]);
        work->verticesE0[0].fields.x = (x + (velocityX >> 3)) >> 4;
        work->verticesE0[0].fields.y = (y + (velocityY >> 3)) >> 4;
        work->verticesE0[0].fields.z = (z + (velocityZ >> 3)) >> 4;
        RR_GTE_RTPT_LDV2(&work->verticesE0[0]);
        motion->fields.x = x + (velocityX >> 4);
        motion->fields.y = y + (velocityY >> 4);
        motion->fields.z = z + (velocityZ >> 4);
        RR_GTE14_SWC2_SZ3(&work->depth70);

        if (work->depth70 > 0) {
            depth = work->depth70 >> depthShift;
            if (depth < 0x1C0) {
                RR_GTE14_SWC2_SXY1(&packet->firstXY0C);
                RR_GTE14_SWC2_SXY2(&packet->secondXY10);
                packet->drawMode04 = RRO_DRAW_MODE;
                packet->nop = RR_GPU_MERGED_PRIM_NOP;
                RR_GPU_ADD_PRIM(ordering + depth, packet, 5);
                packet++;
            }
        }
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
}

void EmitCarShadowQuads(const RroEffectMesh *mesh, RroEffectWork *work) {
    const RroTexturedQuadPrefix *quad = (const RroTexturedQuadPrefix *)(mesh->commands + 1);
    const Vec3s8 *vertices = mesh->vertices;
    GpuPolyFT4 *packet;
    s32 depth;

    RR_GTE_RTPT_LDV(&vertices[quad->vertices.indices[0]], &vertices[quad->vertices.indices[1]],
                    &vertices[quad->vertices.indices[2]]);
    RR_GTE14_NCLIP();
    RR_GTE14_SWC2_SXYZ(&work->projection188.shadow.xy[0]);
    RR_GTE14_SWC2_MAC0(&work->firstArea74);
    RR_GTE_RTPS_LDV(&vertices[quad->vertices.indices[3]]);
    RR_GTE14_AVSZ4();
    RR_GTE14_NCLIP();
    RR_GTE14_SWC2_SXY2(&work->projection188.shadow.xy[3]);
    RR_GTE14_SWC2_MAC0(&work->secondArea78);
    RR_GTE14_SWC2_OTZ(&work->depth70);

    if (work->depth70 <= 0) {
        return;
    }
    depth = work->depth70 >> RRO_SCENE->depthShift60;
    work->depth70 = depth;
    if (depth >= 0x1C0) {
        return;
    }
    if ((work->firstArea74 ^ work->windingBC) <= 0 && (work->secondArea78 ^ work->windingBC) >= 0) {
        return;
    }

    packet = (GpuPolyFT4 *)RR_GPU_SCRATCH->cursor;
    RR_GPU_ADD_PRIM(RR_GPU_SCRATCH->order + 0x80 + depth + quad->orderingBias19, packet, 9);
    INTERP_TAG(packet, quad, 0);
    RR_SXY_COPY(packet->words.xy0, work->projection188.shadow.xy[0]);
    RR_SXY_COPY(packet->words.xy1, work->projection188.shadow.xy[1]);
    RR_SXY_COPY(packet->words.xy2, work->projection188.shadow.xy[2]);
    RR_SXY_COPY(packet->words.xy3, work->projection188.shadow.xy[3]);
    packet->words.color04 = work->colors15C.palette.rgb160[4];
    packet->fields.code = 0x2E;
    packet->words.uv0 = quad->uv0C;
    packet->words.uv1 = quad->uv10;
    packet->words.uv2 = quad->uv14;
    packet->words.uv3 = quad->uv14 >> 16;
    RR_GPU_SCRATCH->cursor = (u8 *)(packet + 1);
}

void EmitCarBodyQuads(const RroEffectMesh *mesh, RroEffectWork *work) {
    const Vec3s8 *source = mesh->vertices;
    RroProjectedVertex *output = work->projection188.pipeline;
    const RroProjectedVertex *projected = work->projection188.mesh.vertices;
    const u32 *commands = mesh->commands;
    const RroBodyQuad *quad = (const RroBodyQuad *)(commands + 1);
    s32 remaining = mesh->vertexCount;
    s32 count, shift;
    GpuDmaTag *ordering;
    RroBodyPacket *packet;

    work->verticesE0[0] = source[0];
    work->verticesE0[1] = source[1];
    work->verticesE0[2] = source[2];
    do {
        RR_GTE14_LWC2_V012(&work->verticesE0[0]);
        RR_GTE14_RTPT();
        source += 3;
        remaining -= 3;
        output += 3;
        work->verticesE0[0] = source[0];
        work->verticesE0[1] = source[1];
        work->verticesE0[2] = source[2];
        GTE_STORE_SXY_SZ(output);
    } while (remaining > 0);

    count = ((const RroBodyCommandHeader *)commands)->fields.count;
    shift = RRO_SCENE->depthShift60 + 4;
    ordering = RR_GPU_SCRATCH->order + (work->facing98.z > 0 ? 0x71 : 0x6C);
    packet = (RroBodyPacket *)RR_GPU_SCRATCH->cursor;
    while (count-- > 0) {
        s32 depth;
        RR_GTE_NCLIP_SXY3(projected[quad->vertices[0]].sxy, projected[quad->vertices[1]].sxy,
                          projected[quad->vertices[2]].sxy);
        depth = (s32)((u32)projected[quad->vertices[0]].sz + projected[quad->vertices[1]].sz +
                      projected[quad->vertices[2]].sz + projected[quad->vertices[3]].sz);
        if (depth > 0) {
            depth >>= shift;
            if (depth < 0x1C0) {
                RR_GTE14_SWC2_MAC0(&work->firstArea74);
                if (work->firstArea74 != 0) {
                    int i;
                    RR_GPU_ADD_PRIM(ordering + depth, packet, 10);
                    INTERP_TAG_SHAPE(&packet->vertices[0], quad, 0, 0);
                    for (i = 0; i < 4; i++) {
                        packet->vertices[i].color = work->colors15C.palette.rgb160[quad->colors[i]];
                        RR_SXY_COPY(packet->vertices[i].xy, projected[quad->vertices[i]].sxy);
                    }
                    packet->drawMode04 = RRO_DRAW_MODE;
                    packet->nop = RR_GPU_MERGED_PRIM_NOP;
                    packet++;
                }
            }
        }
        quad++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
}

void TransformRroVertices(CarSlotBufferC8 *slot, RroEffectWork *work)
{
    s32 intensity;
    u32 *input;
    u8 (*farColor)[4];
    s32 *output;
    s32 remaining, speed, frame;

    RR_GTE_SETTR(work->object00.fields.translation14);
    if (RacePaused != 0) {
        intensity = slot->intensity64;
    } else {
        intensity = (work->vehicle90->input - 0x3E0) * 2;
        intensity = (intensity < 0 ? 0 : intensity) > 0x1000 ? 0x1000 : (intensity < 0 ? 0 : intensity);
        intensity += PsyqRand() & 0xF;
        slot->intensity64 = intensity;
    }
    RR_GTE_MTC2_IR0(intensity);
    input = RroEffectColorPresets.input;
    farColor = RroEffectColorPresets.farColor;
    output = work->colors15C.words;
    remaining = 6;
    do {
        RR_GTE_LWC2_RGB(input);
        RR_GTE_DPCS_FC4(farColor);
        input++;
        farColor++;
        output++;
        RR_GTE_SWC2_RGB2(output);
        remaining--;
    } while (remaining > 0);
    speed = work->vehicle90->input;
    DrawSparkLines(slot->sparksC4, work, speed / 4);
    frame = ScreenFrame & 3;
    EmitCarBodyQuads(&slot->model->body64[frame], work);
    {
        s32 jitter = (PsyqRand() & 3) - 1;
        work->jitterTranslationAC.x = ((u32)work->object00.fields.translation14[0] + jitter);
    }
    {
        s32 jitter = (PsyqRand() & 3) - 1;
        work->jitterTranslationAC.y = ((u32)work->object00.fields.translation14[1] + jitter);
    }
    work->jitterTranslationAC.z = work->object00.fields.translation14[2];
    RR_GTE_SETTR(&work->jitterTranslationAC);
    EmitCarShadowQuads(&slot->model->shadow58, work);
}

void DrawGlowFan(CarSlotBufferC8 *slot, RroEffectWork *work) {
    int depth;
    int *ring;
    s32 *projected;
    int remaining;
    u32 color;
    RroGlowPacket *packet;
    GpuDmaTag *head;

    RR_GTE_SETTR(work->object00.fields.translation14);
    work->inputC8.fields.x = 0;
    work->inputC8.fields.y = -0x5C;
    work->inputC8.fields.z = 0x98;
    RR_GTE_RTPS_LDV(&work->inputC8);
    RR_GTE_SWC2_SXY2(work->projection188.fan.xy + 2);
    RR_GTE_TR_FROM_IR();
    RR_GTE_READ_IR3_NOP(depth);
    if (depth < 0) {
        return;
    }
    depth >>= RRO_SCENE->depthShift60 + 2;
    if (depth >= 0x1C0) {
        return;
    }
    RR_GTE_SETROT_IDENTITY_CLEAR_Z();
    ring = SpotlightConeRingInner;
    projected = work->projection188.fan.xy;
    remaining = 0x11;
    do {
        RR_GTE_RTPT_PACKED_XY(ring);
        ring += 3;
        projected += 3;
        RR_GTE_SWC2_SXYZ(projected);
        remaining -= 3;
    } while (remaining > 0);

    projected = work->projection188.fan.xy + 3;
    color = slot->color08;
    color = (color << 8) | (color | 0x32000000);
    packet = (RroGlowPacket *)RR_GPU_SCRATCH->cursor;
    head = RR_GPU_SCRATCH->order + 0x69 + depth;
    for (remaining = 0x10; remaining > 0; remaining--) {
        packet->drawMode04 = RRO_DRAW_MODE;
        packet->nop = RR_GPU_MERGED_PRIM_NOP;
        packet->centerColor08 = color;
        RR_SXY_COPY(packet->centerXY0C, work->projection188.fan.xy[2]);
        packet->firstColor10 = 0;
        RR_SXY_COPY(packet->firstXY14, projected[0]);
        packet->secondColor18 = 0;
        RR_SXY_COPY(packet->secondXY1C, projected[1]);
        projected++;
        RR_GPU_ADD_PRIM(head, packet, 8);
        INTERP_TAG_SHAPE(&packet->centerColor08, slot, remaining, 0);
        packet++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
}

const RroTexturedQuadPrefix *EmitProjectedQuadsFT4(const RroTexturedQuadPrefix *quad, s32 count, RroEffectWork *work) {
    const RroProjectedVertex *vertices = &work->projection188.pipeline[3];
    s32 shift = RRO_SCENE->depthShift60 + 4;
    GpuPolyFT4 *packet = (GpuPolyFT4 *)RR_GPU_SCRATCH->cursor;
    s32 winding = work->windingBC;

    while (count-- > 0) {
        s32 firstIndices = quad->vertices.words[0];
        s32 secondIndices = quad->vertices.words[1];
        s32 i1 = firstIndices >> 16;
        s32 i0 = firstIndices & 0xFFFF;
        s32 i3 = secondIndices >> 16;
        s32 i2 = secondIndices & 0xFFFF;
        s32 xy0 = vertices[i0].sxy;
        s32 xy1 = vertices[i1].sxy;
        s32 xy2 = vertices[i2].sxy;
        s32 depth;

        RR_GTE_NCLIP_SXY3(xy0, xy1, xy2);
        RR_SXY_COPY(packet->words.xy0, vertices[i0].sxy);
        RR_SXY_COPY(packet->words.xy1, vertices[i1].sxy);
        RR_SXY_COPY(packet->words.xy2, vertices[i2].sxy);
        RR_SXY_COPY(packet->words.xy3, vertices[i3].sxy);
        depth = (s32)((u32)vertices[i0].sz + vertices[i1].sz + vertices[i2].sz + vertices[i3].sz);
        RR_GTE14_SWC2_MAC0(&work->firstArea74);
        if ((work->firstArea74 ^ winding) > 0 && depth > 0) {
            depth >>= shift;
            if (depth < 0x1C0) {
                RR_GPU_ADD_PRIM(work->ordering6C + depth + quad->orderingBias19, packet, 9);
                INTERP_TAG(packet, quad, 0);
                packet->words.uv0 = quad->uv0C;
                packet->words.uv1 = quad->uv10;
                packet->words.uv2 = quad->uv14;
                packet->words.uv3 = quad->uv14 >> 16;
                packet->words.color04 = work->colors15C.palette.rgb160[0];
                packet++;
            }
        }
        quad++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
    return quad;
}

/* Frame interpolation: the trail sample (ring entry) the panels being
 * emitted reach back to. A trail's ring advances every frame, so the
 * panels of one sample keep their identity only keyed by it. */
static const void *TrailPanelSample;

void EmitProjectedLitTrisGT3(const RroTrailPanelCommand *command, s32 count, RroEffectWork *work) {
    GpuPolyG4 *packet = (GpuPolyG4 *)RR_GPU_SCRATCH->cursor;
    s32 primaryColor = work->colors15C.palette.rgb160[0] | 0x3A000000;
    s32 shift = RRO_SCENE->depthShift60 + 3;
    const RroProjectedVertex *base = work->projectedD0;
    const RroProjectedVertex *alternate = work->projectedD4;
    s32 secondaryColor = work->colors15C.palette.rgb160[1];
    s32 yLimit = RR_SCREEN_BOTTOM;
    s32 winding = work->windingBC;

    while (count-- > 0) {
        s32 sxy0 = base[command->outer[0]].sxy;
        s32 sxy1 = base[command->outer[1]].sxy;
        s32 sxy2 = base[command->outer[2]].sxy;
        s32 depth;

        RR_GTE_LDSXY3(sxy0, sxy1, sxy2);
        depth = base[command->center].sz >> shift;
        work->depth70 = depth;
        if (depth - 1U < 0x1BFU) {
            s32 x0, x1, x2, x3, x4, x5;
            s32 y0, y1, y2, y3, y4, y5;

            RR_GTE14_NCLIP();
            RR_SXY_COPY(packet[0].words.xy1, base[command->outer[0]].sxy);
            RR_SXY_COPY(packet[1].words.xy1, base[command->outer[1]].sxy);
            RR_SXY_COPY(packet[2].words.xy1, base[command->outer[2]].sxy);
            x0 = (s16)(base[command->outer[0]].sxy);
            x1 = (s16)(base[command->outer[1]].sxy);
            x2 = (s16)(base[command->outer[2]].sxy);
            x3 = (s16)(alternate[command->outer[0]].sxy);
            x4 = (s16)(alternate[command->outer[1]].sxy);
            x5 = (s16)(alternate[command->outer[2]].sxy);
            RR_GTE14_SWC2_MAC0(&work->firstArea74);
            if ((work->firstArea74 ^ winding) > 0) {
                if ((x0 >= WIDE_SCREEN_LEFT || x1 >= WIDE_SCREEN_LEFT || x2 >= WIDE_SCREEN_LEFT || x3 >= WIDE_SCREEN_LEFT || x4 >= WIDE_SCREEN_LEFT || x5 >= WIDE_SCREEN_LEFT) &&
                    (x0 < WIDE_SCREEN_RIGHT || x1 < WIDE_SCREEN_RIGHT || x2 < WIDE_SCREEN_RIGHT || x3 < WIDE_SCREEN_RIGHT || x4 < WIDE_SCREEN_RIGHT || x5 < WIDE_SCREEN_RIGHT)) {
                    y0 = (s16)(base[command->outer[0]].sxy >> 16);
                    y1 = (s16)(base[command->outer[1]].sxy >> 16);
                    y2 = (s16)(base[command->outer[2]].sxy >> 16);
                    y3 = (s16)(alternate[command->outer[0]].sxy >> 16);
                    y4 = (s16)(alternate[command->outer[1]].sxy >> 16);
                    y5 = (s16)(alternate[command->outer[2]].sxy >> 16);
                    if ((y0 >= 0 || y1 >= 0 || y2 >= 0 || y3 >= 0 || y4 >= 0 || y5 >= 0) &&
                        (y0 < yLimit || y1 < yLimit || y2 < yLimit || y3 < yLimit || y4 < yLimit || y5 < yLimit)) {
                        GpuDmaTag *head = work->ordering6C + work->depth70 + command->orderingBias09;
                        GpuCommandPacket *mode;
                        int i;

                        for (i = 0; i < 3; i++) {
                            RR_SXY_COPY(packet[i].words.xy3, alternate[command->outer[i]].sxy);
                            RR_SXY_COPY(packet[i].words.xy0, base[command->center].sxy);
                            RR_SXY_COPY(packet[i].words.xy2, alternate[command->center].sxy);
                            packet[i].words.color04 = primaryColor;
                            packet[i].words.color0C = 0;
                            packet[i].words.color14 = secondaryColor;
                            packet[i].words.color1C = 0;
                            RR_GPU_ADD_PRIM(head, &packet[i], 8);
                            INTERP_TAG(&packet[i], command, (u32)((uintptr_t)TrailPanelSample >> 2) << 2 | (u32)i);
                        }
                        /* Three G4 panels are followed by one E1 command. */
                        mode = (GpuCommandPacket *)(packet + 3);
                        mode->command = RRO_DRAW_MODE;
                        RR_GPU_ADD_PRIM(head, mode, 1);
                        packet = (GpuPolyG4 *)(mode + 1);
                    }
                }
            }
        }
        command++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
}

const RroGlowPointCommand *EmitProjectedLitQuadsGT4(const RroGlowPointCommand *input, s32 count, RroEffectWork *work) {
    const RroGlowPointCommand *points = input;
    s32 offsetX = (s32)(0U - work->facing98.x) / 64;
    s32 offsetY = (s32)(0U - work->facing98.y) / 64;
    RroGlowProjection *projection = work->projection188.glow.points;
    s32 remaining = count;
    s32 shift = RRO_SCENE->depthShift60 + 2;
    GpuPolyG3 *packet;

    while (remaining > 0) {
        s32 rawDepth, depth;
        RR_GTE_RTPS_LDV(&points->position);
        RR_GTE14_SWC2_SZ3(&work->depth70);
        rawDepth = work->depth70;
        depth = rawDepth >> shift;
        if (rawDepth <= 0 || depth >= 0x1C0) {
            projection->ordering = NULL;
        } else {
            RR_GTE_STORE_IR_MEM(&projection->translation);
            RR_GTE14_SWC2_SXY2(&projection->centerXY);
            projection->translation.x += offsetX;
            projection->translation.y += offsetY;
            projection->ordering = work->ordering6C + depth + points->fields.orderingBias07;
        }
        remaining--;
        projection++;
        points++;
    }
    work->matrix44.glow.rotation48.w[0] = 0x1000;
    work->matrix44.glow.rotation48.w[1] = 0;
    work->matrix44.glow.rotation48.w[2] = 0x1000;
    work->matrix44.glow.rotation48.w[3] = 0;
    work->matrix44.glow.rotation48.w[4] = 0x1000;
    RR_GTE_SETROT(work->matrix44.glow.rotation48.w);

    packet = (GpuPolyG3 *)RR_GPU_SCRATCH->cursor;
    projection = work->projection188.glow.points;
    while (count > 0) {
        GpuDmaTag *head = projection->ordering;
        if (head) {
            const Vec3s8 *ring = SpotlightConeRingOuter;
            s32 previous;
            s32 color = work->colors15C.palette.rgb160[0];
            GpuCommandPacket *mode;

            RR_GTE_SETTR(&projection->translation);
            RR_GTE_RTPS_LDV(ring);
            RR_GTE_STORE_SXY(14, &previous);
            for (remaining = 8; remaining > 0; remaining--) {
                ring++;
                RR_GTE_RTPS_LDV(ring);
                RR_SXY_COPY(packet->xy0, projection->centerXY);
                RR_SXY_COPY(packet->xy1, previous);
                packet->color0 = color;
                packet->color1 = 0;
                packet->color2 = 0;
                RR_GPU_ADD_PRIM(head, packet, 6);
                INTERP_TAG(packet, work, (u32)(projection - work->projection188.glow.points) << 4 | (u32)remaining);
                RR_GTE_STORE_SXY(14, &previous);
                RR_SXY_COPY(packet->xy2, previous);
                packet++;
            }
            mode = (GpuCommandPacket *)packet;
            mode->command = RRO_DRAW_MODE;
            RR_GPU_ADD_PRIM(head, mode, 1);
            packet = (GpuPolyG3 *)(mode + 1);
        }
        count--;
        projection++;
    }
    RR_GPU_SCRATCH->cursor = (u8 *)packet;
    return points;
}

/* Projects a mesh's vertices in triples into the effect pipeline buffer,
 * starting at pipeline[3] (the first result follows three reserved pairs). */
static void RroProjectEffectVertices(RroEffectWork *work, const Vec3s8 *vertices, s32 count,
                                     RroProjectedVertex *output, s32 storeInterpolation) {
    while (count > 0) {
        if (storeInterpolation) {
            RR_GTE_SWC2_IR0(&work->interpolationDC);
        }
        RR_GTE14_LWC2_V012(vertices);
        RR_GTE14_RTPT();
        vertices += 3;
        output += 3;
        GTE_STORE_SXY_SZ(output);
        count -= 3;
    }
}

/* Command streams start with a header word whose signed high half counts records. */
#define RRO_COMMAND_COUNT(p) ((s32)(p)[0] >> 16)

void DrawWheelTrailEffects(void *arg0, s32 *arg1, void *arg2, void *arg3, s32 arg4, s32 arg5, s32 arg6) {
    RroEffectWork *work = &RroEffectWorkArea;
    const ProjectionScratchPrefix *view = (const ProjectionScratchPrefix *)SCRATCHPAD;
    CarSlotBufferC8 *slot = &CarSlotBuffers[arg4 * 2];
    s32 s0;
    s32 a0;
    TrailEntry8 *a2;
    const u32 *p;
    s32 vcnt;

    work->flags8C = arg5;
    if (RRO_SCENE->renderFlags64 != 0) {
        slot++;
        work->windingBC = 0x80000000;
    } else {
        work->windingBC = 0;
    }
    work->vehicle90 = arg0;
    work->ordering6C = RR_GPU_SCRATCH->order + 0x80 + arg6;
    work->bounds94 = CarBoundsTable[arg4 / 2];

    if (arg1 == 0 || (work->flags8C & 1) == 0) {
        slot->mainCount0C = 0;
        slot->secondCount34 = 0;
        slot->historyCount68 = 0;
        slot->phase5C = 0;
    }

    if (work->bounds94->wheelRotationAxis == 3) {
        if (RacePaused == 0) {
            s0 = slot->intensity60;
            if (work->flags8C & 1) {
                a0 = (work->vehicle90->input - 0x358) / 4;
                if (s0 < a0) {
                    s0 = a0;
                } else if (a0 < s0) {
                    s0 -= 4;
                }
            } else if (s0 != 0) {
                s0 -= 4;
            }
            s0 = (s0 < 0 ? 0 : s0) > 0xF0 ? 0xF0 : (s0 < 0 ? 0 : s0);
            slot->intensity60 = s0;
            if (work->flags8C & 1) {
                s0 += PsyqRand() & 0xF;
            }
            slot->color08 = s0;
        }
        if (slot->color08 <= 0) {
            return;
        }
    } else {
        s0 = 0x80;
        if ((work->flags8C & 1) == 0) {
            s0 = 0;
            a0 = 0x80;
            slot->mainCount0C = 0;
            slot->secondCount34 = 0;
            slot->historyCount68 = 0;
            slot->phase5C = 0;
        } else {
            a0 = 0xF0;
        }
        if (RacePaused == 0) {
            s32 v1;
            s32 d = a0 - s0;
            if (work->flags8C & 2) {
                v1 = slot->intensity60 + d / 4;
                slot->intensity60 = v1 < a0 ? v1 : a0;
                v1 = slot->intensity64 + a0 / 4;
                slot->intensity64 = v1 < a0 ? v1 : a0;
            } else {
                v1 = slot->intensity60 - d / 8;
                slot->intensity60 = v1 > s0 ? v1 : s0;
                v1 = slot->intensity64 - a0 / 8;
                slot->intensity64 = v1 > 0 ? v1 : 0;
            }
        }
    }

    if (arg1 == 0) {
        return;
    }

    work->worldPosition38.x = arg1[0] - 0x8000;
    work->worldPosition38.y = arg1[1];
    work->worldPosition38.z = arg1[2] - 0x8000;
    work->inputC8.fields.x = arg1[0] - view->pose08.position00.vector.x;
    work->inputC8.fields.y = arg1[1] - view->pose08.position00.vector.y;
    work->inputC8.fields.z = arg1[2] - view->pose08.position00.vector.z;
    RR_GTE_SETROT(SCRATCH_ROT_MATRIX);
    RR_GTE_MVMVA_ROT_V0_NOCV(&work->inputC8);
    RR_GTE_SWC2_MAC123(work->object00.fields.translation14);
    GTE_SETROT_AND_SAVE(arg2, &work->object00);

    work->inputC8.fields.y = 0;
    work->inputC8.fields.x = 0;
    work->inputC8.fields.z = -0x1000;

    RR_GTE_MVMVA_ROT_V0_NOCV(&work->inputC8);
    RR_GTE_SWC2_MAC123(&work->facing98);

    if (work->bounds94->wheelRotationAxis == 3 && GameModeType == 0) {
        DrawGlowFan(slot, work);
        return;
    }

    if (slot->overlay70 != 0 && (work->flags8C & 3) == 3) {
        if (slot->phase5C == 0) {
            slot->storedPosition74.x = work->worldPosition38.x;
            slot->storedPosition74.y = work->worldPosition38.y;
            slot->storedPosition74.z = work->worldPosition38.z;
            slot->phase5C = 1;
        }
    } else if (slot->phase5C == 1) {
        slot->phase5C = 2;
    }

    if (work->bounds94->wheelRotationAxis == 2 && (GameModeType == 6 || GameModeType == 0)) {
        if (ActiveViewportIndex != 6) {
            TransformRroVertices(slot, work);
        }
    }

    RR_GTE_SETFC(0, 0, 0);
    GTE_CTC2_28(0);

    if (work->flags8C & 1) {
        s32 v1;
        /* The four bytes after this bounds record hold the trail base colour. */
        RR_GTE_LWC2_RGB(work->bounds94 + 1);
        v1 = work->facing98.z;
        if (v1 >= -0xEC8) {
            if (RacePaused != 0) {
                work->blendC0 = slot->color08;
            } else {
                s32 blend;
                work->temporaryC4 = v1;
                if (v1 < 0) {
                    work->temporaryC4 = 0;
                } else if (v1 >= 0x1000) {
                    work->temporaryC4 = 0xFFF;
                }
                blend = PsyqRand() & work->temporaryC4;
                v1 = (work->facing98.z + 0xEC8) / 2;
                if (v1 > 0x200) {
                    v1 = 0x200;
                }
                blend = (blend >> 2) + v1;
                if (blend > 0xE00) {
                    blend = 0xE00;
                }
                work->blendC0 = 0x1000 - blend;
                slot->color08 = work->blendC0;
            }
            RR_GTE_SETTR(work->object00.fields.translation14);
            work->vertices7C = slot->model->primary10.vertices;
            work->vertexCount88 = slot->model->primary10.vertexCount;
            work->commands84 = slot->model->primary10.commands;
            RroProjectEffectVertices(work, work->vertices7C, work->vertexCount88,
                                     work->projection188.pipeline, 1);
            if (RacePaused == 0) {
                s32 idx = slot->mainIndex10;
                s32 n = slot->mainCount0C;
                TrailEntry4 *entry = (TrailEntry4 *)&slot->main14[idx];
                s32 rem = 4 - idx;
                RroProjectedVertex *dst;
                const RroProjectedVertex *src;
                while (n > 0) {
                    entry->unk0 += entry->unk0 / 2;
                    if (entry->unk0 > 0x1000) {
                        entry->unk0 = 0x1000;
                    }
                    rem--;
                    entry++;
                    if (rem <= 0) {
                        entry = (TrailEntry4 *)&slot->main14[0];
                        rem = 4;
                    }
                    n--;
                }
                if (slot->mainCount0C < 4) {
                    slot->mainCount0C += 1;
                }
                slot->mainIndex10 = (slot->mainIndex10 + 3) % 4;
                entry = (TrailEntry4 *)&slot->main14[slot->mainIndex10];
                v1 = 0x1000 - work->temporaryC4;
                entry->unk0 = v1 >= 0x200 ? v1 : 0x200;
                entry->unk2 = (u16)work->interpolationDC;
                n = work->vertexCount88;
                dst = entry->verts;
                src = work->projection188.mesh.vertices;
                while (n-- > 0) {
                    *dst++ = *src++;
                }
            }
            if (slot->mainCount0C >= 2) {
                s32 *out = &work->colors15C.palette.rgb160[0];
                s32 idx = slot->mainIndex10;
                s32 s3 = slot->mainCount0C - 1;
                TrailEntry4 *entry = (TrailEntry4 *)&slot->main14[idx];
                s32 s1 = 4 - idx;
                while (s3 > 0) {
                    RR_GTE_MTC2_IR0(entry->unk0);
                    RR_GTE_OP(0x4A780010);
                    RR_GTE_SWC2_RGB2(out);
                    out++;
                    s1--;
                    entry++;
                    if (s1 <= 0) {
                        entry = (TrailEntry4 *)&slot->main14[0];
                        s1 = 4;
                    }
                    s3--;
                }
                *out = 0;
                work->projectedD4 = work->projection188.mesh.vertices;
                work->colorsD8 = &work->colors15C.palette.rgb160[2];
                idx = slot->mainIndex10;
                s1 = 4 - idx;
                s3 = slot->mainCount0C - 1;
                entry = (TrailEntry4 *)&slot->main14[idx];
                while (s3 > 0) {
                    p = work->commands84;
                    work->projectedD0 = work->projectedD4;
                    work->projectedD4 = entry->verts;
                    TrailPanelSample = entry;
                    EmitProjectedLitTrisGT3((const RroTrailPanelCommand *)(p + 1), RRO_COMMAND_COUNT(p), work);
                    work->colors15C.palette.rgb160[0] = work->colors15C.palette.rgb160[1];
                    s1--;
                    work->colors15C.palette.rgb160[1] = *work->colorsD8++;
                    entry++;
                    if (s1 <= 0) {
                        entry = (TrailEntry4 *)&slot->main14[0];
                        s1 = 4;
                    }
                    s3--;
                }
            }
            RroProjectEffectVertices(work, slot->model->secondary1C.vertices,
                                     slot->model->secondary1C.vertexCount,
                                     work->projection188.pipeline, 0);
            work->colors15C.palette.rgb160[0] = 0x2E808080;
            p = slot->model->secondary1C.commands;
            EmitProjectedQuadsFT4((const RroTexturedQuadPrefix *)(p + 1), RRO_COMMAND_COUNT(p), work);
            GTE_LWC2_IR0(&work->blendC0);
            RR_GTE_OP(0x4A780010);
            RR_GTE_SWC2_RGB2(&work->colors15C.palette.rgb160[0]);
            work->colors15C.palette.rgb160[1] = 0;
            work->colors15C.palette.rgb160[0] = (work->colors15C.palette.rgb160[0] & 0xFFFFFF) | 0x32000000;
            p = slot->model->litQuadCommands0C;
            EmitProjectedLitQuadsGT4((const RroGlowPointCommand *)(p + 1), RRO_COMMAND_COUNT(p), work);
        } else {
            slot->mainCount0C = 0;
        }
    }

    if (work->facing98.z >= 0x7D0) {
        s32 v1 = slot->phase5C;
        slot->secondCount34 = 0;
        if (v1 == 0) {
            return;
        }
        if (v1 == 1) {
            slot->phase5C = 2;
        }
    } else {
        RR_GTE_SETROT(&work->object00);
        RR_GTE_SETTR(work->object00.fields.translation14);
        GTE_CTC2_27(0x88);
        work->vertices7C = slot->model->history34.vertices;
        work->vertexCount88 = slot->model->history34.vertexCount;
        work->commands84 = slot->model->history34.commands;
        RroProjectEffectVertices(work, work->vertices7C, work->vertexCount88,
                                 work->projection188.pipeline, 1);
        if (RacePaused == 0) {
            s32 idx;
            TrailEntry4 *entry;
            s32 n;
            RroProjectedVertex *dst;
            const RroProjectedVertex *src;
            if (slot->secondCount34 < 4) {
                slot->secondCount34 += 1;
            }
            idx = (slot->secondIndex38 + 3) % 4;
            slot->secondIndex38 = idx;
            entry = (TrailEntry4 *)&slot->second3C[idx];
            entry->unk0 = (s16)slot->intensity60;
            n = work->vertexCount88;
            dst = entry->verts;
            src = work->projection188.mesh.vertices;
            while (n-- > 0) {
                *dst++ = *src++;
            }
        }
        if (slot->secondCount34 >= 2) {
            s32 col = slot->intensity60;
            s32 *out;
            s32 n, s1;
            GTE_DPCS_COL(col, work->interpolationDC);
            out = &work->colors15C.palette.rgb160[0];
            for (n = slot->secondCount34 - 1; n > 0; n--) {
                *out++ = col > 0 ? col : 0;
                col -= col / 4;
            }
            *out = 0;
            work->projectedD4 = work->projection188.mesh.vertices;
            work->colorsD8 = &work->colors15C.palette.rgb160[2];
            n = slot->secondCount34 - 1;
            s1 = slot->secondIndex38;
            while (n > 0) {
                work->projectedD0 = work->projectedD4;
                s1 = (s1 + 1) % 4;
                p = work->commands84;
                work->projectedD4 = ((TrailEntry4 *)&slot->second3C[s1])->verts;
                TrailPanelSample = &slot->second3C[s1];
                EmitProjectedLitTrisGT3((const RroTrailPanelCommand *)(p + 1), RRO_COMMAND_COUNT(p), work);
                if (work->colors15C.palette.rgb160[1] <= 0) {
                    break;
                }
                work->colors15C.palette.rgb160[0] = work->colors15C.palette.rgb160[1];
                n--;
                work->colors15C.palette.rgb160[1] = *work->colorsD8++;
            }
        }
        RroProjectEffectVertices(work, slot->model->trailing40.vertices,
                                 slot->model->trailing40.vertexCount,
                                 work->projection188.pipeline, 0);
        {
            s32 c = slot->intensity60;
            work->colors15C.palette.rgb160[0] = 0x2E000000 | (c << 16) | (c << 8) | c;
            p = slot->model->trailing40.commands;
            EmitProjectedQuadsFT4((const RroTexturedQuadPrefix *)(p + 1), RRO_COMMAND_COUNT(p), work);
        }
        RroProjectEffectVertices(work, slot->model->trailing28.vertices,
                                 slot->model->trailing28.vertexCount,
                                 work->projection188.pipeline, 0);
        {
            s32 c = slot->intensity64;
            work->colors15C.palette.rgb160[0] = 0x2E000000 | (c << 16) | (c << 8) | c;
            p = slot->model->trailing28.commands;
            EmitProjectedQuadsFT4((const RroTexturedQuadPrefix *)(p + 1), RRO_COMMAND_COUNT(p), work);
        }
    }

    GTE_CTC2_27(0x88);
    if (slot->phase5C == 0) {
        return;
    }

    work->vertices7C = slot->model->history34.vertices;
    work->vertexCount88 = slot->model->history34.vertexCount;
    work->commands84 = slot->model->history34.commands;
    work->trailVerticesA8 = ((TrailEntry8 *)&slot->history84[slot->historyIndex6C])->verts;

    if (slot->phase5C == 1) {
        const Vec3s8 *vsrc = work->vertices7C;
        Vec3s8 *vdst = work->verticesE0;
        RR_GTE_SETROT(arg3);
        work->jitterTranslationAC.x = work->worldPosition38.x - slot->storedPosition74.x;
        work->jitterTranslationAC.y = work->worldPosition38.y - slot->storedPosition74.y;
        work->jitterTranslationAC.z = work->worldPosition38.z - slot->storedPosition74.z;
        RR_GTE_SETTR(&work->jitterTranslationAC);
        work->trailVerticesA8 = &work->verticesE0[0].fields;
        for (vcnt = work->vertexCount88; vcnt > 0; vcnt--) {
            RR_GTE_MVMVA_ROT_TR_V0(vsrc);
            RR_GTE_STORE_IR_S16(vdst);
            vsrc++;
            vdst++;
        }
        if (RacePaused == 0) {
            Vec3sFields8 *dst;
            const Vec3sFields8 *src;
            slot->historyCount68 += 1;
            if (slot->historyCount68 == 8) {
                slot->phase5C = 2;
            }
            slot->historyIndex6C = (slot->historyIndex6C + 7) % 8;
            a2 = (TrailEntry8 *)&slot->history84[slot->historyIndex6C];
            dst = a2->verts;
            a2->unk0 = (s16)slot->intensity60;
            src = work->trailVerticesA8;
            for (vcnt = work->vertexCount88; vcnt > 0; vcnt--) {
                *dst++ = *src++;
            }
        }
    }

    {
        s32 nleft = slot->historyCount68;
        s32 dec8, i8, s5 = 0;
        if (nleft == 0) {
            return;
        }
        dec8 = (slot->phase5C == 2) ? 8 : 4;
        i8 = slot->historyIndex6C;
        while (nleft > 0) {
            a2 = (TrailEntry8 *)&slot->history84[i8];
            i8 = (i8 + 1) % 8;
            s5 = a2->unk0 - dec8;
            if (s5 < 0) {
                s5 = 0;
            }
            nleft--;
            a2->unk0 = s5;
        }
        if (s5 == 0) {
            slot->historyCount68 -= 1;
        }
        if (slot->historyCount68 == 0) {
            slot->phase5C = 0;
            return;
        }
        if (slot->historyCount68 >= 2) {
            RroProjectedVertex *pipeline = work->projection188.pipeline;
            RroProjectedVertex *vdst;
            const Vec3s8 *vsrc;
            s32 nvert, s0i, s1;

            RR_GTE_SETROT(SCRATCH_ROT_MATRIX);
            work->projectedD0 = &pipeline[3];
            work->inputC8.fields.x = slot->storedPosition74.x - 0x8000 - view->pose08.position00.vector.x;
            work->inputC8.fields.y = slot->storedPosition74.y - view->pose08.position00.vector.y;
            work->inputC8.fields.z = slot->storedPosition74.z - 0x8000 - view->pose08.position00.vector.z;
            RR_GTE_MVMVA_ROT_V0_NOCV(&work->inputC8);
            RR_GTE_SWC2_MAC123(&work->worldPosition38);
            RR_GTE_SETTR(&work->worldPosition38);
            vsrc = (const Vec3s8 *)work->trailVerticesA8;
            vdst = work->projectedD0 - 3;
            for (nvert = work->vertexCount88; nvert > 0; nvert -= 3) {
                RR_GTE14_LWC2_V012(vsrc);
                RR_GTE14_RTPT();
                vsrc += 3;
                vdst += 3;
                GTE_STORE_SXY_SZ(vdst);
            }
            work->projectedD4 = vdst + 3;
            s0i = slot->historyIndex6C;
            work->colors15C.palette.rgb160[0] = 0;
            for (s1 = slot->historyCount68 - 1; s1 > 0; s1--) {
                RroProjectedVertex *d0, *d4;
                s32 next;
                s0i = (s0i + 1) % 8;
                a2 = (TrailEntry8 *)&slot->history84[s0i];
                vsrc = (const Vec3s8 *)a2->verts;
                vdst = work->projectedD4 - 3;
                for (nvert = work->vertexCount88; nvert > 0; nvert -= 3) {
                    RR_GTE_SWC2_IR0(&work->interpolationDC);
                    RR_GTE14_LWC2_V012(vsrc);
                    RR_GTE14_RTPT();
                    vsrc += 3;
                    vdst += 3;
                    GTE_STORE_SXY_SZ(vdst);
                }
                if (s1 == 1) {
                    work->colors15C.palette.rgb160[1] = 0;
                } else {
                    GTE_LWC2_IR0_MEM(&work->interpolationDC);
                    GTE_MTC2_RGB_DPCS((u16)a2->unk0);
                    RR_GTE_SWC2_RGB2(&work->colors15C.palette.rgb160[1]);
                }
                p = work->commands84;
                TrailPanelSample = a2;
                EmitProjectedLitTrisGT3((const RroTrailPanelCommand *)(p + 1), RRO_COMMAND_COUNT(p), work);
                d4 = work->projectedD4;
                d0 = work->projectedD0;
                next = work->colors15C.palette.rgb160[1];
                work->projectedD0 = d4;
                work->projectedD4 = d0;
                if (next <= 0) {
                    break;
                }
                work->colors15C.palette.rgb160[0] = next;
            }
        }
    }
}

/* Initial values of the globals this module owns. */
char UnknownRroTypeFmt[24] = {117, 110, 107, 110, 111, 119, 110, 32, 82, 82, 79, 32, 116, 121, 112, 101, 32, 37, 120, 10};
s32 ModelTexturePage = 10;
RroEffectColorTable RroEffectColorPresets = {.input = {0x3A184040u, 0x3A182B40u, 0x3A101040u, 0x3A000000u, 0x2E101040u, 0x420080FFu}, .farColor = {{0x6F, 0x73, 0x80, 0x3A}, {0x4F, 0x5B, 0x80, 0x3A}, {0x20, 0x20, 0x40, 0x3A}, {0, 0, 0, 0x3A}, {0x20, 0x20, 0x40, 0x2E}, {0xC0, 0x80, 0xFF, 0x42}}};
int SpotlightConeRingInner[17] = {-11796480, -10878908, -8322945, -4456282, 180, 4456614, 8323199, 10879044, 11796480, 10944444, 8388481, 4521818, 65356, -4391078, -8257663, -10813508, -11796480};
Vec3s8 SpotlightConeRingOuter[9] = {{.fields = {.y = -102}}, {.fields = {.x = 90, .y = -72}}, {.fields = {.x = 128}}, {.fields = {.x = 90, .y = 72}}, {.fields = {.y = 102}}, {.fields = {.x = -90, .y = 72}}, {.fields = {.x = -128}}, {.fields = {.x = -90, .y = -72}}, {.fields = {.y = -102}}};
