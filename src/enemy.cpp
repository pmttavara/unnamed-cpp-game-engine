#include "enemy.h"
#include "camera.h"
#include "particle.h"
#include "player.h"
using ng::operator""_s;
int sign(float x);
Bool within_enemy_ai_range(Player *p, Enemy *e) {
    if (!p->alive()) {
        return False;
    }
    auto dist = (p->pos - e->pos).magsq();
    auto radius = e->get_data()->response_radius;
    return dist < radius * radius;
}
Enemy_Data enemy_data[cast(int) Enemy_Type::NUM_ENEMY_TYPES] = {};
Enemy_Data *Enemy::get_data() {
    ng_assert(cast(int) type < cast(int) Enemy_Type::NUM_ENEMY_TYPES,
              "type = %0", cast(int) type);
    return &enemy_data[cast(int) type];
}
void deinit_enemy_data() {
    ng_for(enemy_data) { it.sprite.animations.release(); }
}
void init_enemy_data() {
    {
        Sprite goomber_sprite = {};
        goomber_sprite.animations.push(
            animation_create("data/sprites/player/stun_air.png"_s, 16_px, 16_px,
                             5_fps, {-8, -8}));
        auto data = &enemy_data[cast(int) Enemy_Type::GOOMBER];
        data->sprite = goomber_sprite;
        data->hitbox = Rect{-8, -8, 16, 16};
        data->player_collider = Rect{-6, -6, 12, 12};
        data->gravity = tile_to_game(16_t);
        data->max_health = 1_hp;
        data->damageplayer_amount = 1_hp;
        data->response_radius = tile_to_game(0_t);
    }
    {
        Sprite rabbit_sprite = {};
        {
            rabbit_sprite.animations.push(
                animation_create("data/sprites/enemies/rabbit.png"_s, 16_px,
                                 16_px, 5_fps, {-8, -8}));
            rabbit_sprite.animations.push(
                animation_create("data/sprites/enemies/rabbit_hop.png"_s, 16_px,
                                 16_px, 10_fps, {-8, -8}));
            rabbit_sprite.animations.push(
                animation_create("data/sprites/enemies/rabbit_leap.png"_s,
                                 16_px, 16_px, 10_fps, {-8, -8}));
        }
        auto data = &enemy_data[cast(int) Enemy_Type::RUNNY_RABBIT];
        data->sprite = rabbit_sprite;
        data->hitbox = Rect{-6, -6, 12, 14};
        data->player_collider = Rect{-4, -4, 8, 8};
        data->gravity = tile_to_game(36_t);
        data->max_health = 15_hp;
        data->damageplayer_amount = 1_hp;
        data->response_radius = tile_to_game(10_t);
    }
    {
        Sprite bird_sprite = {};
        {
            bird_sprite.animations.push(
                animation_create("data/sprites/enemies/bird.png"_s, 16_px,
                                 16_px, 5_fps, {-8, -8}));
        }
        auto data = &enemy_data[cast(int) Enemy_Type::FLYING_RAT];
        data->sprite = bird_sprite;
        data->hitbox = Rect{-8, -8, 16, 14};
        data->player_collider = Rect{-4, -4, 8, 8};
        data->gravity = 0._g;
        data->max_health = 2_hp;
        data->damageplayer_amount = 1_hp;
        data->response_radius = 0._g;
    }
    {
        Sprite spikey_sprite = {};
        {
            spikey_sprite.animations.push(
                animation_create("data/sprites/enemies/spikey.png"_s, 16_px,
                                 16_px, 1_fps, {-8, -8}));
        }
        auto data = &enemy_data[cast(int) Enemy_Type::SYNCED_JUMPER];
        data->sprite = spikey_sprite;
        data->hitbox = Rect{-4, -4, 8, 8};
        data->player_collider = Rect{-4, -4, 8, 8};
        data->gravity = tile_to_game(36_t);
        data->max_health = 1_hp;
        data->damageplayer_amount = 1_hp;
        data->response_radius = tile_to_game(5_t);
    }
    {
        Sprite cannon_toast_sprite = {};
        cannon_toast_sprite.animations.push(
            animation_create("data/sprites/enemies/cannon_toast.png"_s, 24_px,
                             24_px, 5_fps, {-12, -12}));
        auto data = &enemy_data[cast(int) Enemy_Type::CANNON_TOAST];
        data->sprite = cannon_toast_sprite;
        data->hitbox = Rect{-7, -8, 14, 20};
        data->player_collider = Rect{-4, -6, 8, 14};
        data->gravity = tile_to_game(24_t);
        data->max_health = 20_hp;
        data->damageplayer_amount = 0_hp;
        data->response_radius = tile_to_game(20_t);
    }
    {
        Sprite charging_cheetah_sprite = {};
        charging_cheetah_sprite.animations.push(
            animation_create("data/sprites/enemies/cheetah.png"_s, 16_px, 16_px,
                             0_fps, {-8, -8}));
        charging_cheetah_sprite.animations.push(
            animation_create("data/sprites/enemies/cheetah_prepare.png"_s,
                             32_px, 16_px, 10_fps, {-16, -8}));
        charging_cheetah_sprite.animations.push(
            animation_create("data/sprites/enemies/cheetah_sprint.png"_s, 32_px,
                             16_px, 10_fps, {-16, -8}));
        auto data = &enemy_data[cast(int) Enemy_Type::CHARGING_CHEETAH];
        data->sprite = charging_cheetah_sprite;
        data->hitbox = Rect{-7, -7, 14, 15};
        data->player_collider = Rect{-10, -2, 24, 10};
        data->gravity = tile_to_game(24_t);
        data->max_health = 8_hp;
        data->damageplayer_amount = 4_hp;
        data->response_radius = tile_to_game(10_t);
    }
}
Enemy enemy_copy(Enemy *rhs) {
    Enemy result = *rhs;
    result.sprite = sprite_copy(&rhs->sprite);
    return result;
}
void enemy_destroy(Enemy *rhs) { sprite_destroy(&rhs->sprite); }
Enemy::Enemy(Enemy_Type type, u16 location, s16 x, s16 y) {
    this->type = type;
    auto data = get_data();
    this->health = data->max_health;
    this->max_health = data->max_health;
    this->sprite = data->sprite;
    this->pos = {tile_to_game(x), tile_to_game(y)};
    this->location = location;

    switch (type) {
    default: {
    } break;
    case (Enemy_Type::RUNNY_RABBIT): {
        runny_rabbit = {};
    } break;
    }
}
static void enemy_face_player(Enemy *enemy, Vector2 *player_pos) {
    if (player_pos) {
        enemy->facing_left = player_pos->x - enemy->pos.x < 0.f;
    }
};
static void cheetah_go_idle(Enemy *enemy, Vector2 *player_pos) {
    enemy->sprite.change_to_animation(0);
    enemy_face_player(enemy, player_pos);
    enemy->ai_ticker.elapsed = 0;
};
void Enemy::update(f32 dt, Room *room) {
    auto data = get_data();
    Bool in_range = within_enemy_ai_range(player, this);
    Bool motoring = False;
    ai_ticker.elapsed += dt;
    { // tick A.I.
        vel.y += data->gravity * dt;
        auto orig_vel = vel;
        auto p_pos = &player->pos;
        switch (type) {
        default: {
        } break;
        case (Enemy_Type::GOOMBER): {
            auto dir = facing_left ? -1.f : +1.f;
            // @Todo: this 3_t was 4_t, but the goomber was skipping 1 tile gaps
            // Maybe this should return to 4_t and goomber gravity be upped?
            auto max_speed = tile_to_game(3_t) * dir;
            if (ng::abs(vel.x) < ng::abs(max_speed)) {
                vel.x += max_speed * 10 * dt;
            }
        } break;
        case (Enemy_Type::SYNCED_JUMPER): {
            if (in_range && !ground) {
                auto x_diff = p_pos->x - pos.x;
                vel.x += sign(x_diff) * tile_to_game(2_t) * dt;
            }
        } break;
        case (Enemy_Type::FLYING_RAT): {
            enemy_face_player(this, p_pos);
            Bool player_below =
                (ng::abs(p_pos->x - pos.x) < tile_to_game(1_t)) &&
                (p_pos->y - pos.y > tile_to_game(-1_t));
            if (player_below) {
                vel.y += tile_to_game(36_t) * dt;
                ai_ticker.elapsed = 0;
            } else {
                auto x = sin(cast(float) ai_ticker.elapsed * ng::TAU32);
                vel = Vector2{tile_to_game(0_t), tile_to_game(-5_t)} * x;
            }
        } break;
        case (Enemy_Type::RUNNY_RABBIT): {
            if (ground) {
                sprite.change_to_animation(0);
            }
            auto dir = player->pos.x - pos.x < 0.f ? -1.f : +1.f;
            const auto x_speed = tile_to_game(3_t);
            const auto x_accel = x_speed * 32;
            if (ground && in_range) {
                enemy_face_player(this, p_pos);
            }
            if (!in_range) {
                sprite.Restart();
            }
            if (ground) {
                if (ai_ticker.elapsed > 0.5_s) {
                    auto h = pos.y - player->pos.y;
                    h = ng::max(h, 0.0_g);
                    auto d = (player->pos.x - pos.x);
                    auto g = -(data->gravity);
                    auto max_height = h + 8.0_g;
                    auto v_y = 2 * ng::sqrt(-g * max_height);
                    // quadratic formula to get jump vel from landing pos
                    auto discriminant = ng::sqrt(v_y * v_y - 4 * g * -h);
                    auto v_x = 2 * g * d / (-v_y - discriminant);
                    vel.y += -v_y;
                    vel.x += v_x;

                    if (h > tile_to_game(2_t)) {
                        sprite.change_to_animation(2);
                        auto emit_y_offset = data->hitbox.b();
                        auto emit_x_offset = data->hitbox.r();
                        for (int i = 0; i < 3; i += 1) {
                            auto emit_pos = pos;
                            emit_pos.x += fx_rng.get(-1, 1) * emit_x_offset;
                            emit_pos.y += emit_y_offset;
                            particle_system->Emit(
                                Particle_Type::SMOKE_8, emit_pos,
                                fx_rng.get({-1, 0}, {1, 2}) * 64.0_g);
                        }
                    } else {
                        sprite.change_to_animation(1);
                    }
                    sprite.Restart();
                    ai_ticker.elapsed = 0.0_s;
                }
            } else {
                auto new_xvel = vel.x + x_accel * dir * dt;
                if (vel.x == 0._g || sign(x_accel) != (facing_left ? +1 : -1)) {
                    if (ng::abs(new_xvel) < ng::abs(x_speed)) {
                        vel.x = new_xvel;
                    }
                }

                ai_ticker.elapsed = 0.0_s;
            }
            if (!ground) {
                if (vel.y < -tile_to_game(16_t)) {
                    sprite.change_to_animation(2);
                    for (int times = runny_rabbit.smoke_timer.run(dt);
                         times > 0; times -= 1) {
                        particle_system->Emit(
                            Particle_Type::SMOKE_8, pos + vel * dt,
                            vel / 8 + fx_rng.get({-1, -1}, {1, 1}) * 32.0_g);
                    }
                } else if (vel.y > 0.0_g) {
                    sprite.change_to_animation(1);
                }
            }
        } break;
        case (Enemy_Type::CANNON_TOAST): {
            if (in_range) {
                enemy_face_player(this, p_pos);
                if (ai_ticker.elapsed > 2.0_s) {
                    ai_ticker.elapsed = 0;
                    auto dir = (player->pos - pos).hat();
                    if (dir == Vector2{}) {
                        dir = {1, 0};
                    }
                    for (int i = 0; i < 2; i += 1) {
                        Projectile new_projectile = {};
                        new_projectile.location = location;
                        new_projectile.hurts_enemies = False;
                        new_projectile.hurts_player = True;
                        new_projectile.hitbox = {-2, -2, 4, 4};
                        new_projectile.damage = 2_hp;
                        new_projectile.hit_sound =
                            sound_from_filename("data/sound/generic_hit.wav"_s);
                        new_projectile.hit_sound.update_volume(0.25f);
                        new_projectile.pos = pos;
                        new_projectile.pos += Vector2{dir.y, -dir.x} * 2;
                        new_projectile.pos += dir * (-12 + i * 16._g);
                        new_projectile.texture = texture_load(
                            "data/sprites/weapons/gun_projectile.png"_s);
                        new_projectile.vel = dir * tile_to_game(7_t);
                        entity_manager->projectiles.push(new_projectile);
                        for (int i2 = fx_rng.get(4, 8); i2 > 0;
                             i2 -= 1) { // emit particles
                            auto particle_direction = new_projectile.vel / 2;
                            constexpr auto scale = 24.f;
                            particle_direction +=
                                fx_rng.get({-1, -1}, {+1, +1}) * scale;
                            {
                                auto type = Particle_Type::SMOKE_8;
                                particle_system->Emit(
                                    type, pos, particle_direction,
                                    get_particle_data(type)->default_lifetime +
                                        fx_rng.get(-0.25_s, +0.25_s));
                            }
                        }
                        // @Todo: emit sound
                    }
                }
            }
        } break;
        case (Enemy_Type::CHARGING_CHEETAH): {
            if (in_range) {
                // let the timer accumulate while the player's in range
            } else if (ai_ticker.elapsed < 1.0_s) {
                // the player's gone out of range, and the ticker didn't
                // get into the charging stage, so reset the timer
                cheetah_go_idle(this, p_pos);
            }
            if (ai_ticker.elapsed > 0.0_s) {
                if (ai_ticker.elapsed <= 1.0_s) { // preparing to charge
                    enemy_face_player(this, p_pos);
                    sprite.change_to_animation(1);
                } else { // charging
                    sprite.change_to_animation(2);
                    auto dir = facing_left ? -1.f : +1.f;
                    auto max_speed = tile_to_game(15_t) * dir;
                    if (ng::abs(vel.x) < ng::abs(max_speed)) {
                        vel.x += max_speed * 10 * dt;
                    }
                    if (ai_ticker.elapsed > 1.35_s) { // done charging
                        cheetah_go_idle(this, p_pos);
                    }
                }
            }
        } break;
        }
        if (vel != orig_vel) {
            motoring = True;
        }
    }
    constexpr auto friction = 300._g;
    if (ground && !motoring) {
        vel.x = vel.x > 0._g ? ng::max(0._g, (vel.x - friction * dt))
                             : ng::min(0._g, (vel.x + friction * dt));
    }
    /* if (dt > 0.s) */
    {
        auto hitbox = get_data()->hitbox;
        Collide_State collide_state = {};
        collide_state.pos = pos;
        collide_state.delta = vel * dt;
        collide_state.collider_bottom =
            Rect{hitbox.x + 1, hitbox.cy(), hitbox.w - 2, hitbox.h / 2};
        collide_state.collider_top =
            Rect{hitbox.x + 1, hitbox.y, hitbox.w - 2, hitbox.h / 2};
        collide_state.collider_right =
            Rect{hitbox.cx(), hitbox.y + 1, hitbox.w / 2, hitbox.h - 2};
        collide_state.collider_left =
            Rect{hitbox.x, hitbox.y + 1, hitbox.w / 2, hitbox.h - 2};
        auto result = collide(collide_state, room);
        switch (type) {
        default: {
        } break;
        case (Enemy_Type::GOOMBER): {
            if (result.left.did_collide) {
                facing_left = False;
            }
            if (result.right.did_collide) {
                facing_left = True;
            }
        } break;
        case (Enemy_Type::CHARGING_CHEETAH): {
            if ((result.left.did_collide && facing_left) ||
                (result.right.did_collide && !facing_left)) {
                ng_assert(result.new_state.delta.x == 0._g);
                cheetah_go_idle(this, null);
            }
        } break;
        }
        pos = result.new_state.pos;
        if (result.bottom.did_collide) { // @Boilerplate
            vel.y = ng_min(vel.y, 0.0_g);
        }
        if (result.top.did_collide) {
            vel.y = ng_max(vel.y, 0.0_g);
        }
        if (result.right.did_collide) {
            vel.x = ng_min(vel.x, 0.0_g);
        }
        if (result.left.did_collide) {
            vel.x = ng_max(vel.x, 0.0_g);
        }
        { ground = result.bottom.did_collide; }
    }
    ng_for(entity_manager->enemies) {
        if (&it == this) {
            // Last simulated enemy
            // \see world.cc
            break;
        }
        if (it.location == location && it.alive()) {
            auto aabb = data->hitbox + pos;
            if (aabb.intersects(it.get_data()->hitbox + it.pos)) {
                // collide with other enemy
                auto dir = (it.pos - pos).hat();
                if (dir.magsq() == 0) {
                    dir = {1, 0};
                    dir *= tile_to_game(128_t);
                    if (vel.mag() < tile_to_game(6_t)) {
                        vel -= dir * dt;
                    }
                    if (it.vel.mag() < tile_to_game(6_t)) {
                        it.vel += dir * dt;
                    }
                } else {
                    dir *= tile_to_game(128_t);
                    if (vel.mag() < tile_to_game(6_t)) {
                        vel -= dir * dt;
                    }
                }
            }
        }
    }
    sprite.update(dt);
    sprite.flip = facing_left ? Flip_Mode::HORIZONTAL : Flip_Mode::NONE;
}
void Enemy::Render() {
    auto anim = sprite.current_anim();
#if defined(NDEBUG) || 1
    Rect occluder{pos.x, pos.y, px_to_game(anim->atlas.w),
                  px_to_game(anim->atlas.h)};
    if ((occluder + anim->offset).intersects(camera->world_rect()))
#endif
    {
        Vector2 final_pos = camera->game_to_local(pos);
        ::Render(renderer, &sprite,
                 /*game_to_px*/ (final_pos.x),
                 /*game_to_px*/ (final_pos.y));
    }
}
void do_death_effects(Texture *t, Vector2 pos) {
    Vector2 texture_min = {-px_to_game(t->w) / 2, -px_to_game(t->h) / 2};
    Vector2 texture_max = {+px_to_game(t->w) / 2, +px_to_game(t->h) / 2};
    auto emit = [&](Particle_Type type) {
        particle_system->Emit(type, pos + fx_rng.get(texture_min, texture_max),
                              fx_rng.get({-80._g, -80._g}, {+80._g, +80._g}),
                              get_particle_data(type)->default_lifetime *
                                  fx_rng.get(0.8f, 1.25f));
    };
    int num_sparks_x = 0;
    int num_sparks_y = 0;
    auto spark_data = get_particle_data(Particle_Type::SPARK);
    {
        auto spark_texture = cast(Texture *)(&spark_data->texture);
        num_sparks_x = t->w / spark_texture->w;
        num_sparks_y = t->h / spark_texture->h;
        num_sparks_x *= 2;
        num_sparks_y *= 2;
    }
    for (int j = 0; j < num_sparks_y; j += 1) {
        auto y = ng::lerp(texture_min.y / 2, texture_max.y / 2,
                          cast(float) j / num_sparks_y);
        for (int i = 0; i < num_sparks_x; i += 1) {
            auto x = ng::lerp(texture_min.x / 2, texture_max.x / 2,
                              cast(float) i / num_sparks_x);
            auto part_pos = pos + Vector2{x, y};
            particle_system->Emit(Particle_Type::SPARK, part_pos,
                                  fx_rng.get({-1, -1}, {1, 1}) * 80.0_g,
                                  spark_data->default_lifetime * 1.5);
        }
    }
    for (int i = fx_rng.get(3, 6, &rng_mostly_min); i > 0; i -= 1) {
        emit(Particle_Type::SMOKE_16);
    }
    for (int i = fx_rng.get(5, 9, &rng_mostly_min); i > 0; i -= 1) {
        emit(Particle_Type::SMOKE_8);
    }
}
void Enemy::take_damage(u16 damage, Vector2 knockback) {
    health -= damage;
    if (alive()) {
        // @Todo: knockback scaling factor for big guys
        vel += knockback;
    } else { // die
        do_death_effects(&sprite.current_anim()->atlas.base, pos);
    }
}

void Enemy::respond_to_player_jump(Player *player) {
    if (within_enemy_ai_range(player, this)) {
        switch (type) {
        default: {
        } break;
        case (Enemy_Type::SYNCED_JUMPER): {
            if (ground) {
                vel.y = tile_to_game(-16_t);
            }
        } break;
        }
    }
}
