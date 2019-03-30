#include "particle.h"
#include "camera.h"
using ng::operator""_s;
ParticleData particle_data[cast(int) Particle_Type::NumParticleTypes] = {};
ParticleData *get_particle_data(Particle_Type type) {
    ng_assert(cast(int) type < cast(int) Particle_Type::NumParticleTypes,
              "type = %0", cast(int) type);
    return &particle_data[int(type)];
}
void particle_system_destroy(Particle_System *parcticle_system) {
    parcticle_system->particles.release();
}
Particle_System *particle_system = null;
Particle_System::Particle_System() {
    particle_data[cast(int) Particle_Type::NONE] = {};
    particle_data[cast(int) Particle_Type::BLEED] = ParticleData{
        texture_atlas_create("data/sprites/particles/bleed.png"_s, 2_px, 2_px),
        0.5_s,
        {0, -50},
        {0, 400}};
    particle_data[cast(int) Particle_Type::SMOKE_8] = ParticleData{
        texture_atlas_create("data/sprites/particles/smoke8_TEMP.png"_s, 8_px, 8_px),
        1.0_s,
        {0, 0},
        {0, -50}};
    particle_data[cast(int) Particle_Type::SMOKE_16] = ParticleData{
        texture_atlas_create("data/sprites/particles/smoke16.png"_s, 16_px, 16_px),
        2.0_s,
        {0, 0},
        {0, -50}};
    particle_data[cast(int) Particle_Type::RAIN] = ParticleData{
        texture_atlas_create("data/sprites/particles/rain.png"_s, 1_px, 4_px),
        2.0_s,
        {-15, 150},
        {-25, 250}};
    particle_data[cast(int) Particle_Type::SPARK] = ParticleData{
        texture_atlas_create("data/sprites/particles/spark.png"_s, 16_px, 16_px),
        0.133_s,
        {0, 0},
        {0, 0}};
}
void Particle_System::Emit(Particle_Type type, Vector2 origin, Vector2 velocity,
                           f32 lifetime) {
    if (type != Particle_Type::NONE) {
        Particle p = {};
        p.type = type;
        p.particle_timer.elapsed = 0;
        p.particle_timer.expiration = lifetime;
        p.initial_velocity = velocity;
        p.origin = origin;
        particles.push(p);
    }
}
void Particle_System::update(f32 dt) {
    for (int i = 0; i < particles.count; i += 1) {
        auto it = &particles[i];
        it->particle_timer.elapsed += dt;
        if (it->particle_timer.expired()) {
            particles.remove(i);
            i -= 1;
        }
    }
}
static void particle_render(Particle *part) {
    auto data = get_particle_data(part->type);
    // d = vi*t + 1/2a*t^2
    auto p_i = part->origin;
    auto t = part->particle_timer.elapsed;
    auto t_f = part->particle_timer.expiration;
    auto v_i = part->initial_velocity;
    auto v_f = data->final_velocity;
    auto a = (v_f - v_i) / t_f;
    auto d = p_i + v_i * t + a * .5f * (t * t);
    {
        auto tex = &data->texture;
        d -= {tex->w / 2.f, tex->h / 2.f};
        auto screen_pos = camera->game_to_local(d);
        auto frame = cast(int)((t / t_f) * tex->num_frames);
        if (frame >= tex->num_frames) {
            frame = tex->num_frames - 1;
        }
        tex->MoveTo(frame);
        Render(renderer, tex,
               /*game_to_px*/ (screen_pos.x),
               /*game_to_px*/ (screen_pos.y), Flip_Mode::NONE);
    }
}
void Particle_System::render_background_particles() {
    ng_for(particles) {
        if (it.type == Particle_Type::NONE) {
            particle_render(&it);
        }
    }
}
void Particle_System::render_midground_particles() {
    ng_for(particles) {
        if (it.type == Particle_Type::RAIN) {
            particle_render(&it);
        }
    }
}
void Particle_System::render_frontground_particles() {
    ng_for(particles) {
        if (it.type == Particle_Type::SMOKE_16 ||
            it.type == Particle_Type::SMOKE_8 ||
            it.type == Particle_Type::BLEED ||
            it.type == Particle_Type::SPARK) {
            particle_render(&it);
        }
    }
}
void Particle_System::ClearAll() { particles.count = 0; }
