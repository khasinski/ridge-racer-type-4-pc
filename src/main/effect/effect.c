#include "rr/cpu_time.h"
#include "types.h"
#include "rr/frame_storage_types.h"
#include "scratchpad.h"
#include "rr/gpu_scratch_types.h"
#include "rr/ui.h"
#include "game/effect/ptinit.h"
#include "game/effect/effect.h"
#include "psyq/libc/rand.h"

#include "psyq/libgpu/gpudisp.h"

void UpdateParticles(void) {
    RR_CPU_CYCLES(RR_CYCLES_UPDATE_PARTICLES);

    GpuDrawModePacket *p;
    s32 i;

    if (ParticleSystem.fadeTimer0C > 0) {
        ParticleSystem.fadeTimer0C--;
        ParticleSystem.brightness18 = ParticleSystem.fadeTarget14 - ((ParticleSystem.fadeTarget14 - ParticleSystem.fadeStart10) * ParticleSystem.fadeTimer0C) / ParticleSystem.fadeDuration08;
    }
    if (ParticleSystem.spawnTimer04 <= 0) {
        SpawnParticlePair();
        ParticleSystem.spawnTimer04 = (PsyqRand() % 16) * 2 + 8;
    }
    p = (GpuDrawModePacket *)RR_GPU_SCRATCH->cursor;
    ParticleSystem.spawnTimer04--;
    SetDrawMode(p, 0, 1, 0xA0, 0);
    RR_GPU_LINK(&CurrentDrawBuf->buffer.ordering70.tags[75], p);
    p++;
    RR_GPU_SCRATCH->cursor = (u8 *)p;
    for (i = 0; i < 0x18; i++) {
        if (ParticleSystem.entries2C[i].active00 & 1) {
            UpdateParticle(i);
            DrawParticle(i);
        }
    }
}

/* Initial values of the globals this module owns. */
ParticleSystem32C ParticleSystem;
