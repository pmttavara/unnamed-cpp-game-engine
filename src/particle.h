#pragma once
#include "renderer.h"
enum struct Particle_Type : u8 {
    NONE,

    BLEED,
    SMOKE_8,
    SMOKE_16,
    RAIN,
    SPARK,

    NumParticleTypes
};
struct Particle {
    Particle_Type type = Particle_Type::NONE;
    Timer particle_timer = {};
    Vector2 initial_velocity = {};
    Vector2 origin = {};
};
struct ParticleData {
    TextureAtlas texture;
    f32 default_lifetime;
    Vector2 default_initial_velocity;
    Vector2 final_velocity;
};
ParticleData *get_particle_data(Particle_Type type);
/* singleton */ struct Particle_System {
    ng::array<Particle> particles = {video_allocator};
    Particle_System();
    void Emit(Particle_Type type, Vector2 origin) {
        auto data = get_particle_data(type);
        Emit(type, origin, data->default_initial_velocity,
             data->default_lifetime);
    }
    void Emit(Particle_Type type, Vector2 origin, Vector2 velocity) {
        auto data = get_particle_data(type);
        Emit(type, origin, velocity, data->default_lifetime);
    }
    void Emit(Particle_Type type, Vector2 origin, Vector2 velocity,
              f32 lifetime);
    void update(f32 dt);
    void render_background_particles();
    void render_midground_particles();
    void render_frontground_particles();
    void ClearAll();
};
void particle_system_destroy(Particle_System *parcticle_system);
extern Particle_System *particle_system;