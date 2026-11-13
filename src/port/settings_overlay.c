#include "settings_overlay.h"

#include <SDL3/SDL.h>
#include <psyz.h>
#include <psyz/audio.h>
#include <psyz/overlay.h>
#include <psyz/overlay_sdl3.h>
#include <psyz/overlay_sdl3_gpu.h>
#include <psyz/video.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_check.h"
#include "host_log.h"
#include "settings_model.h"
#include "settings_raster.h"

/* SDL's precompiled sprite shaders (SPIR-V, MSL, DXIL), as Parasite Eve. */
#include "tri_texture.vert.dxil.h"
#include "tri_texture.vert.msl.h"
#include "tri_texture.vert.spv.h"
#include "texture_rgba.frag.dxil.h"
#include "texture_rgba.frag.msl.h"
#include "texture_rgba.frag.spv.h"

static Rr4Prefs *s_prefs;
static const char *s_mounted;
static SettingsModel s_model;
static SettingsSurface s_surface;
static SettingsLayout s_layout;
static int s_stick_x, s_stick_y;
static int s_pending_open;

/* RR4_SETTINGS_SCRIPT: keys pressed at given presents, for headless checks. */
#define SCRIPT_MAX 256
static struct {
    long present;
    SDL_Scancode key;
} s_script[SCRIPT_MAX];
static int s_script_len;
static long s_presents;
static PsyzOverlayFrameCB s_prev_frame;

/* ---- disc dialog ---- */

/* One native dialog at a time. Each gets a generation: its answer counts
 * only while the host still waits for that generation, so a dialog left
 * open when the panel closed cannot hand its path to a later panel. The
 * callback may run on another thread; the path is written before the
 * answered generation is published. */
static SDL_AtomicInt s_dialog_generation; /* the dialog the host waits for */
static SDL_AtomicInt s_dialog_answered;   /* generation of the last answer */
static SDL_AtomicInt s_dialog_open;       /* a dialog of any generation is up */
static char s_dialog_path[RR4_PREFS_PATH_MAX];
static int s_dialog_wait; /* generation the panel waits for, or 0 */

static void SDLCALL DiscChosen(void *userdata, const char *const *files, int filter) {
    int generation = (int)(intptr_t)userdata;
    (void)filter;
    if (generation == SDL_GetAtomicInt(&s_dialog_generation)) {
        s_dialog_path[0] = '\0';
        if (files && files[0] && strlen(files[0]) < sizeof(s_dialog_path))
            snprintf(s_dialog_path, sizeof(s_dialog_path), "%s", files[0]);
        SDL_SetAtomicInt(&s_dialog_answered, generation);
    }
    SDL_SetAtomicInt(&s_dialog_open, 0);
}

static const SDL_DialogFileFilter kCueFilter[] = {{"Ridge Racer Type 4 (Europe) CUE image", "cue"}};

/* The generation of the new dialog, or 0 while an earlier one is still up. */
static int StartDiscDialog(SDL_Window *parent) {
    int generation;

    if (!SDL_CompareAndSwapAtomicInt(&s_dialog_open, 0, 1))
        return 0;
    generation = SDL_AddAtomicInt(&s_dialog_generation, 1) + 1;
    SDL_ShowOpenFileDialog(DiscChosen, (void *)(intptr_t)generation, parent, kCueFilter, 1, NULL, false);
    return generation;
}

static int DiscDialogAnswered(int generation) { return SDL_GetAtomicInt(&s_dialog_answered) == generation; }

/* Nobody waits for the open dialog any more: its answer is dropped. */
static void ForgetDiscDialog(void) { SDL_AddAtomicInt(&s_dialog_generation, 1); }

int SettingsChooseDisc(char *out, size_t size) {
    int generation;

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        return 0;
    generation = StartDiscDialog(NULL);
    if (!generation)
        return 0;
    while (!DiscDialogAnswered(generation)) {
        SDL_PumpEvents();
        SDL_Delay(10);
    }
    if (!s_dialog_path[0] || strlen(s_dialog_path) >= size)
        return 0;
    snprintf(out, size, "%s", s_dialog_path);
    return 1;
}

static int CheckDisc(const char *path, char *message, size_t size) {
    return Rr4DiscCheckImage(path, message, size) == RR4_DISC_OK;
}

/* ---- GPU presentation (Parasite Eve's settings overlay path) ---- */

typedef struct OverlayVertex {
    float x, y;
    float r, g, b, a;
    float u, v;
} OverlayVertex;

static SDL_Window *s_window;
static SDL_GPUDevice *s_device;
static SDL_GPUGraphicsPipeline *s_pipeline;
static SDL_GPUSampler *s_sampler;
static SDL_GPUTexture *s_texture;
static SDL_GPUBuffer *s_vertices;
static int s_texture_w, s_texture_h;
static int s_window_w, s_window_h;
static PsyzOverlayInitCB_SDL3GPU s_prev_init;
static PsyzOverlayRenderCB_SDL3GPU s_prev_render;
static PsyzOverlayDestroyCB s_prev_destroy;

static void ReleaseGpu(void) {
    if (s_device) {
        if (s_texture)
            SDL_ReleaseGPUTexture(s_device, s_texture);
        if (s_vertices)
            SDL_ReleaseGPUBuffer(s_device, s_vertices);
        if (s_sampler)
            SDL_ReleaseGPUSampler(s_device, s_sampler);
        if (s_pipeline)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_pipeline);
    }
    s_texture = NULL;
    s_vertices = NULL;
    s_sampler = NULL;
    s_pipeline = NULL;
    s_texture_w = s_texture_h = 0;
    s_window_w = s_window_h = 0;
}

static void OnGpuInit(SDL_Window *window, SDL_GPUDevice *device) {
    if (s_prev_init)
        s_prev_init(window, device);
    if (s_device && s_device != device)
        ReleaseGpu();
    s_window = window;
    s_device = device;
    /* The window exists now: window-dependent preferences apply. */
    if (s_prefs)
        Psyz_SetFullscreen_SDL3(s_prefs->fullscreen != 0);
}

static void OnGpuDestroy(void) {
    if (s_prev_destroy)
        s_prev_destroy();
    ReleaseGpu();
    SettingsSurfaceRelease(&s_surface);
    s_device = NULL;
    s_window = NULL;
}

static SDL_GPUShader *CreateShader(SDL_GPUDevice *device, SDL_GPUShaderStage stage, const unsigned char *spirv,
                                   size_t spirv_size, const unsigned char *msl, size_t msl_size,
                                   const unsigned char *dxil, size_t dxil_size, Uint32 samplers, Uint32 uniforms) {
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderCreateInfo info;

    SDL_zero(info);
    info.stage = stage;
    info.num_samplers = samplers;
    info.num_uniform_buffers = uniforms;
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        info.code = spirv;
        info.code_size = spirv_size;
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        info.code = msl;
        info.code_size = msl_size;
        info.entrypoint = "main0";
        info.format = SDL_GPU_SHADERFORMAT_MSL;
    } else if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
        info.code = dxil;
        info.code_size = dxil_size;
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_DXIL;
    } else {
        return NULL;
    }
    return SDL_CreateGPUShader(device, &info);
}

static int EnsurePipeline(SDL_GPUDevice *device, SDL_GPUTextureFormat format) {
    SDL_GPUShader *vertex, *fragment;
    SDL_GPUVertexBufferDescription vbd;
    SDL_GPUVertexAttribute attribs[3];
    SDL_GPUColorTargetDescription color;
    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_GPUSamplerCreateInfo sampler;
    SDL_GPUBufferCreateInfo buffer;

    if (format == SDL_GPU_TEXTUREFORMAT_INVALID)
        return 0;
    if (s_pipeline && s_sampler && s_vertices)
        return 1;
    vertex = CreateShader(device, SDL_GPU_SHADERSTAGE_VERTEX, tri_texture_vert_spv, sizeof(tri_texture_vert_spv),
                          tri_texture_vert_msl, sizeof(tri_texture_vert_msl), tri_texture_vert_dxil,
                          sizeof(tri_texture_vert_dxil), 0, 1);
    fragment = CreateShader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, texture_rgba_frag_spv,
                            sizeof(texture_rgba_frag_spv), texture_rgba_frag_msl, sizeof(texture_rgba_frag_msl),
                            texture_rgba_frag_dxil, sizeof(texture_rgba_frag_dxil), 1, 1);
    if (!vertex || !fragment) {
        if (vertex)
            SDL_ReleaseGPUShader(device, vertex);
        if (fragment)
            SDL_ReleaseGPUShader(device, fragment);
        return 0;
    }
    SDL_zero(vbd);
    vbd.pitch = sizeof(OverlayVertex);
    vbd.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    SDL_zero(attribs);
    attribs[0].location = 0;
    attribs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attribs[0].offset = 0;
    attribs[1].location = 1;
    attribs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    attribs[1].offset = 2 * sizeof(float);
    attribs[2].location = 2;
    attribs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attribs[2].offset = 6 * sizeof(float);
    SDL_zero(color);
    color.format = format;
    color.blend_state.enable_blend = true;
    color.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    color.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    SDL_zero(info);
    info.vertex_shader = vertex;
    info.fragment_shader = fragment;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_buffer_descriptions = &vbd;
    info.vertex_input_state.num_vertex_attributes = 3;
    info.vertex_input_state.vertex_attributes = attribs;
    info.target_info.color_target_descriptions = &color;
    info.target_info.num_color_targets = 1;
    s_pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    SDL_ReleaseGPUShader(device, vertex);
    SDL_ReleaseGPUShader(device, fragment);
    SDL_zero(sampler);
    sampler.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    s_sampler = SDL_CreateGPUSampler(device, &sampler);
    SDL_zero(buffer);
    buffer.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer.size = sizeof(OverlayVertex) * 12;
    s_vertices = SDL_CreateGPUBuffer(device, &buffer);
    if (!s_pipeline || !s_sampler || !s_vertices) {
        ReleaseGpu();
        return 0;
    }
    return 1;
}

static void WriteQuad(OverlayVertex *out, float x0, float y0, float x1, float y1, float u0, float v0, float u1,
                      float v1, float alpha) {
    out[0] = (OverlayVertex){x0, y0, 1.0f, 1.0f, 1.0f, alpha, u0, v0};
    out[1] = (OverlayVertex){x1, y0, 1.0f, 1.0f, 1.0f, alpha, u1, v0};
    out[2] = (OverlayVertex){x0, y1, 1.0f, 1.0f, 1.0f, alpha, u0, v1};
    out[3] = (OverlayVertex){x0, y1, 1.0f, 1.0f, 1.0f, alpha, u0, v1};
    out[4] = (OverlayVertex){x1, y0, 1.0f, 1.0f, 1.0f, alpha, u1, v0};
    out[5] = (OverlayVertex){x1, y1, 1.0f, 1.0f, 1.0f, alpha, u1, v1};
}

static int Upload(SDL_GPUDevice *device) {
    OverlayVertex verts[12];
    float w = (float)s_window_w, h = (float)s_window_h;
    SDL_GPUTransferBufferCreateInfo transfer_info;
    SDL_GPUTransferBuffer *transfer;
    SDL_GPUCommandBuffer *command;
    SDL_GPUCopyPass *copy;
    SDL_GPUTextureTransferInfo tex_src;
    SDL_GPUTextureRegion tex_dst;
    SDL_GPUTransferBufferLocation vtx_src;
    SDL_GPUBufferRegion vtx_dst;
    Uint32 tex_bytes, vtx_bytes;
    uint8_t *mapped;
    bool submitted;

    if (s_surface.w <= 0 || s_surface.h <= 0 || s_window_w <= 0 || s_window_h <= 0 || !s_vertices)
        return 0;
    if (!s_texture || s_texture_w != s_surface.w || s_texture_h != s_surface.h) {
        SDL_GPUTextureCreateInfo info;
        if (s_texture)
            SDL_ReleaseGPUTexture(device, s_texture);
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)s_surface.w;
        info.height = (Uint32)s_surface.h;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        s_texture = SDL_CreateGPUTexture(device, &info);
        s_texture_w = s_texture ? s_surface.w : 0;
        s_texture_h = s_texture ? s_surface.h : 0;
        if (!s_texture)
            return 0;
    }
    {
        /* A dimmed backdrop from the solid texel, then the panel. */
        float solid_u = 0.5f / (float)s_texture_w;
        float solid_v = ((float)s_texture_h - 0.5f) / (float)s_texture_h;
        float x0 = (float)s_layout.x / w * 2.0f - 1.0f;
        float y0 = 1.0f - (float)s_layout.y / h * 2.0f;
        float x1 = (float)(s_layout.x + s_layout.w) / w * 2.0f - 1.0f;
        float y1 = 1.0f - (float)(s_layout.y + s_layout.h) / h * 2.0f;
        float panel_v1 = (float)(s_texture_h - 1) / (float)s_texture_h;
        WriteQuad(verts, -1.0f, 1.0f, 1.0f, -1.0f, solid_u, solid_v, solid_u, solid_v, 0.62f);
        WriteQuad(verts + 6, x0, y0, x1, y1, 0.0f, 0.0f, 1.0f, panel_v1, 1.0f);
    }
    tex_bytes = (Uint32)((size_t)s_surface.w * (size_t)s_surface.h * 4u);
    vtx_bytes = (Uint32)sizeof(verts);
    SDL_zero(transfer_info);
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = tex_bytes + vtx_bytes;
    transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!transfer)
        return 0;
    mapped = (uint8_t *)SDL_MapGPUTransferBuffer(device, transfer, false);
    if (!mapped) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return 0;
    }
    memcpy(mapped, s_surface.pixels, tex_bytes);
    memcpy(mapped + tex_bytes, verts, vtx_bytes);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    command = SDL_AcquireGPUCommandBuffer(device);
    if (!command) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return 0;
    }
    copy = SDL_BeginGPUCopyPass(command);
    if (!copy) {
        SDL_CancelGPUCommandBuffer(command);
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return 0;
    }
    SDL_zero(tex_src);
    tex_src.transfer_buffer = transfer;
    tex_src.pixels_per_row = (Uint32)s_surface.w;
    tex_src.rows_per_layer = (Uint32)s_surface.h;
    SDL_zero(tex_dst);
    tex_dst.texture = s_texture;
    tex_dst.w = (Uint32)s_surface.w;
    tex_dst.h = (Uint32)s_surface.h;
    tex_dst.d = 1;
    SDL_UploadToGPUTexture(copy, &tex_src, &tex_dst, false);
    SDL_zero(vtx_src);
    vtx_src.transfer_buffer = transfer;
    vtx_src.offset = tex_bytes;
    SDL_zero(vtx_dst);
    vtx_dst.buffer = s_vertices;
    vtx_dst.size = vtx_bytes;
    SDL_UploadToGPUBuffer(copy, &vtx_src, &vtx_dst, false);
    SDL_EndGPUCopyPass(copy);
    submitted = SDL_SubmitGPUCommandBuffer(command);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return submitted;
}

static void RefreshGpu(void) {
    int w = 0, h = 0;
    SettingsLayout layout;

    if (!s_window || !s_device)
        return;
    SDL_GetWindowSizeInPixels(s_window, &w, &h);
    if (w <= 0 || h <= 0 || !SettingsFontReady())
        return;
    SettingsComputeLayout(&s_model, w, h, &layout);
    if (w != s_window_w || h != s_window_h || layout.w != s_layout.w || layout.h != s_layout.h)
        s_model.dirty = 1;
    s_window_w = w;
    s_window_h = h;
    s_layout = layout;
    if (!s_model.dirty)
        return;
    if (!SettingsSurfaceResize(&s_surface, layout.w, layout.h))
        return;
    SettingsPaint(&s_surface, &s_model, &layout);
    if (EnsurePipeline(s_device, SDL_GetGPUSwapchainTextureFormat(s_device, s_window)) && Upload(s_device))
        s_model.dirty = 0;
}

static void OnGpuRender(SDL_GPUCommandBuffer *command, SDL_GPURenderPass *pass) {
    static const float kIdentity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    static const float kColorScale[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    SDL_GPUViewport viewport;
    SDL_Rect scissor;
    SDL_GPUTextureSamplerBinding sampler_bind;
    SDL_GPUBufferBinding vertex_bind;

    if (s_prev_render)
        s_prev_render(command, pass);
    if (!s_model.open || !s_pipeline || !s_texture || !s_vertices || s_window_w <= 0 || s_window_h <= 0)
        return;
    viewport = (SDL_GPUViewport){0.0f, 0.0f, (float)s_window_w, (float)s_window_h, 0.0f, 1.0f};
    scissor = (SDL_Rect){0, 0, s_window_w, s_window_h};
    SDL_SetGPUViewport(pass, &viewport);
    SDL_SetGPUScissor(pass, &scissor);
    SDL_BindGPUGraphicsPipeline(pass, s_pipeline);
    SDL_zero(sampler_bind);
    sampler_bind.texture = s_texture;
    sampler_bind.sampler = s_sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, &sampler_bind, 1);
    SDL_zero(vertex_bind);
    vertex_bind.buffer = s_vertices;
    SDL_BindGPUVertexBuffers(pass, 0, &vertex_bind, 1);
    SDL_PushGPUVertexUniformData(command, 0, kIdentity, sizeof(kIdentity));
    SDL_PushGPUFragmentUniformData(command, 0, kColorScale, sizeof(kColorScale));
    SDL_DrawGPUPrimitives(pass, 12, 1, 0, 0);
}

/* ---- fallback: a separate settings window when there is no GPU overlay ---- */

static void RenderFallback(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture **texture) {
    int w = 640, h = 600;
    SettingsLayout layout;

    SDL_GetWindowSizeInPixels(window, &w, &h);
    SettingsComputeLayout(&s_model, w, h, &layout);
    s_layout = layout;
    if (SettingsFontReady() && SettingsSurfaceResize(&s_surface, layout.w, layout.h)) {
        float tw = 0, th = 0;
        SettingsPaint(&s_surface, &s_model, &layout);
        if (*texture)
            SDL_GetTextureSize(*texture, &tw, &th);
        if (!*texture || (int)tw != s_surface.w || (int)th != s_surface.h) {
            if (*texture)
                SDL_DestroyTexture(*texture);
            *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                         s_surface.w, s_surface.h);
            if (*texture)
                SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
        }
        if (*texture)
            SDL_UpdateTexture(*texture, NULL, s_surface.pixels, s_surface.w * 4);
    }
    SDL_SetRenderDrawColor(renderer, 12, 14, 18, 255);
    SDL_RenderClear(renderer);
    if (*texture) {
        SDL_FRect src = {0.0f, 0.0f, (float)layout.w, (float)layout.h};
        SDL_FRect dst = {(float)layout.x, (float)layout.y, (float)layout.w, (float)layout.h};
        SDL_RenderTexture(renderer, *texture, &src, &dst);
    }
    SDL_RenderPresent(renderer);
    s_model.dirty = 0;
}

/* ---- input ---- */

static void Pointer(SDL_Window *window, float x, float y, int click) {
    float density = window ? SDL_GetWindowPixelDensity(window) : 1.0f;
    float scale = s_layout.scale > 0.0f ? s_layout.scale : 1.0f;

    if (density <= 0.0f)
        density = 1.0f;
    SettingsModelPointer(&s_model, (x * density - (float)s_layout.x) / scale,
                         (y * density - (float)s_layout.y) / scale, click);
}

static void StickAxis(int vertical, int value) {
    int *held = vertical ? &s_stick_y : &s_stick_x;
    int next = value > 16000 ? 1 : value < -16000 ? -1 : (value > -8000 && value < 8000 ? 0 : *held);

    if (next == *held)
        return;
    *held = next;
    if (next)
        SettingsModelDo(&s_model, vertical ? (next > 0 ? SETTINGS_DOWN : SETTINGS_UP)
                                           : (next > 0 ? SETTINGS_RIGHT : SETTINGS_LEFT));
}

static void ProcessEvent(const SDL_Event *e, SDL_Window *window) {
    switch (e->type) {
    case SDL_EVENT_QUIT:
        s_model.quit = 1;
        SettingsModelClose(&s_model);
        return;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        if (window && s_window && window != s_window && e->window.windowID == SDL_GetWindowID(window)) {
            SettingsModelClose(&s_model); /* the fallback window: resume */
        } else {
            s_model.quit = 1;
            SettingsModelClose(&s_model);
        }
        return;
    case SDL_EVENT_KEY_DOWN:
        if (s_model.capturing) {
            if (e->key.scancode == SDL_SCANCODE_ESCAPE)
                SettingsModelDo(&s_model, SETTINGS_BACK);
            else if (!e->key.repeat)
                SettingsModelCaptureKey(&s_model, (int)e->key.scancode);
            return;
        }
        switch (e->key.scancode) {
        case SDL_SCANCODE_ESCAPE:
            if (!e->key.repeat)
                SettingsModelDo(&s_model, SETTINGS_BACK);
            break;
        case SDL_SCANCODE_TAB:
            if (!e->key.repeat)
                SettingsModelDo(&s_model, (e->key.mod & SDL_KMOD_SHIFT) ? SETTINGS_PAGE_PREV : SETTINGS_PAGE_NEXT);
            break;
        case SDL_SCANCODE_PAGEDOWN:
            if (!e->key.repeat)
                SettingsModelDo(&s_model, SETTINGS_PAGE_NEXT);
            break;
        case SDL_SCANCODE_PAGEUP:
            if (!e->key.repeat)
                SettingsModelDo(&s_model, SETTINGS_PAGE_PREV);
            break;
        case SDL_SCANCODE_UP:
            SettingsModelDo(&s_model, SETTINGS_UP);
            break;
        case SDL_SCANCODE_DOWN:
            SettingsModelDo(&s_model, SETTINGS_DOWN);
            break;
        case SDL_SCANCODE_LEFT:
            SettingsModelDo(&s_model, SETTINGS_LEFT);
            break;
        case SDL_SCANCODE_RIGHT:
            SettingsModelDo(&s_model, SETTINGS_RIGHT);
            break;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
        case SDL_SCANCODE_SPACE:
            if (!e->key.repeat)
                SettingsModelDo(&s_model, SETTINGS_ACTIVATE);
            break;
        case SDL_SCANCODE_BACKSPACE:
        case SDL_SCANCODE_DELETE:
            if (!e->key.repeat)
                SettingsModelDo(&s_model, SETTINGS_CLEAR);
            break;
        default:
            break;
        }
        return;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        if (s_model.capturing) {
            if (s_model.device == SETTINGS_DEVICE_CONTROLLER)
                SettingsModelCaptureButton(&s_model, e->gbutton.button);
            else if (e->gbutton.button == SDL_GAMEPAD_BUTTON_EAST)
                SettingsModelDo(&s_model, SETTINGS_BACK);
            return;
        }
        switch (e->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_EAST:
        case SDL_GAMEPAD_BUTTON_START:
        case SDL_GAMEPAD_BUTTON_GUIDE:
            SettingsModelDo(&s_model, SETTINGS_BACK);
            break;
        case SDL_GAMEPAD_BUTTON_SOUTH:
            SettingsModelDo(&s_model, SETTINGS_ACTIVATE);
            break;
        case SDL_GAMEPAD_BUTTON_WEST:
            SettingsModelDo(&s_model, SETTINGS_CLEAR);
            break;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            SettingsModelDo(&s_model, SETTINGS_PAGE_PREV);
            break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            SettingsModelDo(&s_model, SETTINGS_PAGE_NEXT);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            SettingsModelDo(&s_model, SETTINGS_UP);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            SettingsModelDo(&s_model, SETTINGS_DOWN);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            SettingsModelDo(&s_model, SETTINGS_LEFT);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            SettingsModelDo(&s_model, SETTINGS_RIGHT);
            break;
        default:
            break;
        }
        return;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (s_model.capturing) {
            if (s_model.device == SETTINGS_DEVICE_CONTROLLER && e->gaxis.value > 16000) {
                if (e->gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER)
                    SettingsModelCaptureButton(&s_model, SDL_GAMEPAD_BUTTON_COUNT);
                else if (e->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
                    SettingsModelCaptureButton(&s_model, SDL_GAMEPAD_BUTTON_COUNT + 1);
            }
            return;
        }
        if (e->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX || e->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY)
            StickAxis(e->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY, e->gaxis.value);
        return;
    case SDL_EVENT_MOUSE_MOTION:
        Pointer(window, e->motion.x, e->motion.y, 0);
        return;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (e->button.button == SDL_BUTTON_LEFT)
            Pointer(window, e->button.x, e->button.y, 1);
        return;
    case SDL_EVENT_MOUSE_WHEEL: {
        int step = e->wheel.y > 0.0f ? -1 : e->wheel.y < 0.0f ? 1 : 0;
        if (e->wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
            step = -step;
        if (step)
            SettingsModelWheel(&s_model, step);
        return;
    }
    default:
        return;
    }
}

/* ---- the modal panel ---- */

static void ScriptKey(SDL_Scancode key) {
    SDL_Event e;

    SDL_zero(e);
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.scancode = key;
    e.key.down = true;
    SDL_PushEvent(&e);
    e.type = SDL_EVENT_KEY_UP;
    e.key.down = false;
    SDL_PushEvent(&e);
}

static void OnFrame(void) {
    if (s_prev_frame)
        s_prev_frame();
    s_presents++;
    for (int i = 0; i < s_script_len; i++)
        if (s_script[i].present == s_presents)
            ScriptKey(s_script[i].key);
}

static void ParseScript(const char *script) {
    static const struct {
        const char *name;
        SDL_Scancode key;
    } keys[] = {{"escape", SDL_SCANCODE_ESCAPE}, {"up", SDL_SCANCODE_UP},       {"down", SDL_SCANCODE_DOWN},
                {"left", SDL_SCANCODE_LEFT},     {"right", SDL_SCANCODE_RIGHT}, {"enter", SDL_SCANCODE_RETURN},
                {"tab", SDL_SCANCODE_TAB},       {"backspace", SDL_SCANCODE_BACKSPACE}};

    while (script && *script && s_script_len < SCRIPT_MAX) {
        const char *at = strchr(script, '@'), *next = strchr(script, ',');
        size_t len = at ? (size_t)(at - script) : 0;
        int found = 0;

        if (!at || (next && next < at))
            break;
        for (size_t k = 0; k < sizeof(keys) / sizeof(keys[0]); k++) {
            if (strlen(keys[k].name) == len && !strncmp(keys[k].name, script, len)) {
                s_script[s_script_len].present = strtol(at + 1, NULL, 10);
                s_script[s_script_len++].key = keys[k].key;
                found = 1;
            }
        }
        if (!found)
            fprintf(stderr, "RR4_SETTINGS_SCRIPT: unknown key in \"%.*s\"\n", (int)len, script);
        script = next ? next + 1 : NULL;
    }
    if (s_script_len == SCRIPT_MAX && script && *script)
        fprintf(stderr, "RR4_SETTINGS_SCRIPT: more than %d keys, ignoring \"%s\"\n", SCRIPT_MAX, script);
}

static void ApplyChanges(void) {
    if (SettingsModelTakeChanged(&s_model)) {
        *s_prefs = s_model.prefs;
        Rr4PrefsApply(s_prefs, 1);
    }
}

static void RunPanel(void) {
    SDL_Window *game = Psyz_GetWindow_SDL3();
    int use_gpu = game && s_device;
    SDL_Window *fallback_window = NULL;
    SDL_Renderer *fallback_renderer = NULL;
    SDL_Texture *fallback_texture = NULL;
    int audio_paused;

    if (s_model.open || !s_prefs)
        return;
    if (game)
        s_prefs->fullscreen = (SDL_GetWindowFlags(game) & SDL_WINDOW_FULLSCREEN) != 0; /* F4 */
    if (!use_gpu) {
        fallback_window = SDL_CreateWindow("Ridge Racer Type 4 - Settings", 640, 640, SDL_WINDOW_HIGH_PIXEL_DENSITY);
        fallback_renderer = fallback_window ? SDL_CreateRenderer(fallback_window, NULL) : NULL;
        if (!fallback_renderer) {
            if (fallback_window)
                SDL_DestroyWindow(fallback_window);
            return;
        }
    }
    Psyz_PauseHostClock_SDL3(true);
    audio_paused = Psyz_AudioIsPaused();
    Psyz_AudioPause();
    SettingsModelOpen(&s_model, s_prefs, s_mounted);
    s_model.check_disc = CheckDisc;
    s_stick_x = s_stick_y = 0;
    if (s_script_len)
        fprintf(stderr, "settings: open at present %ld vsync %d\n", s_presents, Psyz_VideoVSync(-1));
    while (s_model.open) {
        if (s_model.choose_disc && !s_dialog_wait) {
            s_dialog_wait = StartDiscDialog(use_gpu ? game : fallback_window);
            if (!s_dialog_wait) {
                s_model.choose_disc = 0;
                snprintf(s_model.status, sizeof(s_model.status),
                         "A file dialog is still open. Close it, then choose again.");
                s_model.dirty = 1;
            }
        }
        if (s_dialog_wait && DiscDialogAnswered(s_dialog_wait)) {
            s_dialog_wait = 0;
            SettingsModelDiscChosen(&s_model, s_dialog_path[0] ? s_dialog_path : NULL);
        }
        ApplyChanges();
        if (use_gpu) {
            RefreshGpu();
            Psyz_VideoVSync(0); /* presents the held frame with the panel and polls input */
        } else {
            SDL_Event event;
            RenderFallback(fallback_window, fallback_renderer, &fallback_texture);
            if (SDL_WaitEventTimeout(&event, 20))
                ProcessEvent(&event, fallback_window);
        }
    }
    /* A dialog still open answers nobody. */
    if (s_dialog_wait) {
        ForgetDiscDialog();
        s_dialog_wait = 0;
    }
    ApplyChanges();
    if (s_model.edited && Rr4PrefsPath()[0] && !Rr4PrefsSave(s_prefs)) {
        Rr4Log("ridge-racer-type-4-pc: cannot save preferences to %s\n", Rr4PrefsPath());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Ridge Racer Type 4",
                                 "Could not save preferences. Changes apply for this session.",
                                 fallback_window ? fallback_window : game);
    }
    if (fallback_texture)
        SDL_DestroyTexture(fallback_texture);
    if (fallback_renderer)
        SDL_DestroyRenderer(fallback_renderer);
    if (fallback_window)
        SDL_DestroyWindow(fallback_window);
    /* Keys held for the menu must not reach the resumed pad poll. */
    SDL_ResetKeyboard();
    Psyz_PauseHostClock_SDL3(false);
    if (!audio_paused)
        Psyz_AudioUnpause();
    if (s_script_len)
        fprintf(stderr, "settings: close at present %ld vsync %d resolution %ux audio %s quit %d\n", s_presents,
                Psyz_VideoVSync(-1), Psyz_VideoGetInternalResolution(),
                Psyz_AudioIsPaused() ? "paused" : "running", s_model.quit);
    if (s_model.quit) {
        SDL_Event quit;
        SDL_zero(quit);
        quit.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quit);
    }
}

static bool OnEvent(const SDL_Event *e) {
    if (s_model.open) {
        /* PsyZ still tracks the window and the connected pads. */
        if (e->type == SDL_EVENT_WINDOW_RESIZED || e->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
            e->type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED || e->type == SDL_EVENT_GAMEPAD_ADDED ||
            e->type == SDL_EVENT_GAMEPAD_REMOVED) {
            s_model.dirty = 1;
            return false;
        }
        ProcessEvent(e, s_window);
        return true;
    }
    if ((e->type == SDL_EVENT_KEY_DOWN || e->type == SDL_EVENT_KEY_UP) && e->key.scancode == SDL_SCANCODE_ESCAPE) {
        /* Never PsyZ's quit shortcut. */
        if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat)
            s_pending_open = 1;
    } else if (e->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && e->gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE) {
        s_pending_open = 1;
    } else if (!s_pending_open) {
        return false;
    }
    /* The panel runs on the thread that presents. */
    if (s_pending_open && SDL_IsMainThread()) {
        s_pending_open = 0;
        RunPanel();
    }
    return true;
}

void SettingsOverlayInstall(Rr4Prefs *prefs, const char *mounted) {
    static int installed;

    s_prefs = prefs;
    s_mounted = mounted;
    if (installed)
        return;
    installed = 1;
    s_prev_init = Psyz_OverlayInit_SDL3GPU(OnGpuInit);
    s_prev_render = Psyz_OverlayRender_SDL3GPU(OnGpuRender);
    s_prev_destroy = Psyz_OverlayDestroyCB(OnGpuDestroy);
    Psyz_SetOverlayEventFilter_SDL3(OnEvent);
    ParseScript(SDL_getenv("RR4_SETTINGS_SCRIPT"));
    if (s_script_len)
        s_prev_frame = Psyz_OverlayFrameCB(OnFrame);
}
