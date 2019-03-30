#include "weapon.h"
#include "camera.h"
#include "particle.h"
#include "player.h"
#include "world.h"
using ng::operator""_s;
Weapon_Data weapon_data[cast(int) Weapon_Type::NUM_WEAPON_TYPES] = {};
void deinit_weapon_data() {
    ng_for(weapon_data) { it.sprite.animations.release(); }
}
void init_weapon_data() {
    { // Fickle Sickle
        Sprite weapon_sprite;
        { // @FixMe: make real sprite
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/sickle_inactive.png"_s,
                                 16_px, 16_px, 1_fps, {-8, -6}));
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/sickle_swinging.png"_s,
                                 16_px, 16_px, 25_fps, {-10, -6}));
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/sickle_cooldown.png"_s,
                                 16_px, 16_px, 15_fps, {0, -6}));
        }
        auto weapon_hit_sound =
            sound_from_filename("data/sound/generic_hit.wav"_s);
        weapon_hit_sound.update_volume(1.0f);
        auto weapon_swing_sound =
            sound_from_filename("data/sound/whoosh.wav"_s);
        weapon_swing_sound.update_volume(0.8f);
        auto type = Weapon_Type::FICKLE_SICKLE;
        auto wdata = &weapon_data[cast(int) type];
        wdata->sprite = weapon_sprite;
        wdata->type = Weapon_Data::Attack_Type::MELEE;
        wdata->mode = Weapon_Data::Trigger_Mode::TAP;
        wdata->hit_sound = weapon_hit_sound;
        wdata->swing_sound = weapon_swing_sound;
        wdata->damage = 1_hp;
        wdata->hitbox = {0._g, -6._g, 24._g, 12._g};
        wdata->enemy_knockback = {32, -32};
        wdata->lift_amount = 0;
        wdata->swing = 0.1_s;
        wdata->cooldown = 0.12_s;
    }
    { // Vine Whip
        Sprite weapon_sprite;
        {
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/vine_inactive.png"_s,
                                 16_px, 8_px, 1_fps, {-8, 0}));
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/vine_swinging.png"_s,
                                 32_px, 16_px, 25_fps, {-24, -8}));
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/vine_cooldown.png"_s,
                                 32_px, 16_px, 15_fps, {-8, -8}));
        }
        auto weapon_hit_sound =
            sound_from_filename("data/sound/generic_hit.wav"_s);
        weapon_hit_sound.update_volume(1.0f);
        ng::string s("data/sound/whoosh.wav");
        auto weapon_swing_sound = sound_from_filename(s);
        weapon_swing_sound.update_volume(0.8f);
        auto type = Weapon_Type::VINE_WHIP;
        auto wdata = &weapon_data[cast(int) type];
        wdata->sprite = weapon_sprite;
        wdata->type = Weapon_Data::Attack_Type::MELEE;
        wdata->mode = Weapon_Data::Trigger_Mode::TAP;
        wdata->hit_sound = (weapon_hit_sound);
        wdata->swing_sound = (weapon_swing_sound);
        wdata->damage = 2_hp;
        wdata->hitbox = {0._g, -4._g, 24._g, 8._g};
        wdata->enemy_knockback = {64, -64};
        wdata->lift_amount = 0;
        wdata->swing = 0.2_s;
        wdata->cooldown = 0.3_s;
    }
    { // Li'l Drill
        Sprite weapon_sprite;
        {
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/mandible_inactive.png"_s,
                                 32_px, 16_px, 1_fps, {-8, -8}));
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/mandible_swinging.png"_s,
                                 32_px, 16_px, 12_fps, {-24, -8}));
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/mandible_cooldown.png"_s,
                                 32_px, 16_px, 7_fps, {0, -8}));
        }
        auto weapon_hit_sound =
            sound_from_filename("data/sound/generic_hit.wav"_s);
        weapon_hit_sound.update_volume(1.0f);
        auto weapon_swing_sound =
            sound_from_filename("data/sound/whoosh.wav"_s);
        weapon_swing_sound.update_volume(0.8f);
        auto type = Weapon_Type::LIL_DRILL;
        auto wdata = &weapon_data[cast(int) type];
        wdata->sprite = (weapon_sprite);
        wdata->type = Weapon_Data::Attack_Type::MELEE;
        wdata->mode = Weapon_Data::Trigger_Mode::HOLD;
        wdata->hit_sound = (weapon_hit_sound);
        wdata->swing_sound = (weapon_swing_sound);
        wdata->damage = 1_hp;
        wdata->hitbox = {0._g, -8._g, 16._g, 15._g};
        wdata->enemy_knockback = {16, -16};
        wdata->lift_amount = 0;
        wdata->swing = 0.0_s;
        wdata->cooldown = 0.2_s;
    }
    { // Mandible Mandoble
        Sprite weapon_sprite;
        {
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/mandible_inactive.png"_s,
                                 32_px, 16_px, 1_fps, {-8, -8}));
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/mandible_swinging.png"_s,
                                 32_px, 16_px, 12_fps, {-24, -8}));
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/mandible_cooldown.png"_s,
                                 32_px, 16_px, 7_fps, {0, -8}));
        }
        auto weapon_hit_sound =
            sound_from_filename("data/sound/generic_hit.wav"_s);
        weapon_hit_sound.update_volume(1.0f);
        auto weapon_swing_sound =
            sound_from_filename("data/sound/whoosh.wav"_s);
        weapon_swing_sound.update_volume(0.8f);
        auto type = Weapon_Type::MANDIBLE_MANDOBLE;
        auto wdata = &weapon_data[cast(int) type];
        wdata->sprite = (weapon_sprite);
        wdata->type = Weapon_Data::Attack_Type::MELEE;
        wdata->mode = Weapon_Data::Trigger_Mode::TAP;
        wdata->hit_sound = (weapon_hit_sound);
        wdata->swing_sound = (weapon_swing_sound);
        wdata->damage = 4_hp;
        wdata->hitbox = {0._g, -8._g, 32._g, 15._g};
        wdata->enemy_knockback = {128, -128};
        wdata->lift_amount = cast(s16) tile_to_game(-8_t);
        wdata->swing = 0.3_s;
        wdata->cooldown = 0.4_s;
    }
    { // gun
        Sprite weapon_sprite;
        {
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/gun_inactive.png"_s,
                                 16_px, 8_px, 1_fps, {2, -3}));
            weapon_sprite.animations.push();
            weapon_sprite.animations.push(
                animation_create("data/sprites/weapons/gun_cooldown.png"_s,
                                 16_px, 8_px, 18_fps, {2, -3}));
        }
        auto weapon_hit_sound =
            sound_from_filename("data/sound/generic_hit.wav"_s);
        weapon_hit_sound.update_volume(1.0f);
        auto weapon_swing_sound =
            sound_from_filename("data/sound/gun_shoot.wav"_s);
        weapon_swing_sound.update_volume(0.25f);
        auto type = Weapon_Type::UNNAMED_GUN;
        auto wdata = &weapon_data[cast(int) type];
        wdata->sprite = (weapon_sprite);
        wdata->type = Weapon_Data::Attack_Type::RANGED;
        wdata->mode = Weapon_Data::Trigger_Mode::HOLD;
        wdata->hit_sound = (weapon_hit_sound);
        wdata->swing_sound = (weapon_swing_sound);
        wdata->damage = 1_hp;
        wdata->hitbox = {0._g, -6._g, 0._g, 12._g};
        wdata->enemy_knockback = {8, -8};
        wdata->lift_amount = 0;
        wdata->swing = 0.0_s;
        wdata->cooldown = 0.25_s;
        {
            wdata->projectile.texture =
                texture_load("data/sprites/weapons/gun_projectile.png"_s);
            wdata->projectile.hitbox = {-2._g, -2._g, 4._g, 4._g};
            wdata->projectile.vel = {tile_to_game(12_t), 0._g};
        }
    }
}
Weapon weapon_from_type(Weapon_Type type, Player *owner) {
    Weapon result = {};
    result.type = type;
    result.owner = owner;
    auto data = weapon_get_data(type);
    result.sprite = sprite_copy(&data->sprite);
    result.timer.expiration = data->swing;
    result.timer.expire();
    return result;
}
Weapon weapon_copy(Weapon *rhs) {
    Weapon result = *rhs;
    result.sprite = sprite_copy(&rhs->sprite);
    return result;
}
void weapon_destroy(Weapon *weapon) { weapon->sprite.animations.release(); }
void Weapon::update(f32 dt) {
    timer.elapsed += dt;
    sprite.update(dt);
    { // update sprite
        switch (state) {
        case State::Inactive: {
            sprite.change_to_animation(0);
        } break;
        case State::Swinging: {
        } break;
        case State::Cooldown: {
        } break;
        }
    }
    if (state == State::Cooldown) {
        if (timer.expired()) {
            sprite.change_to_animation(0);
            sprite.Restart();
            timer.elapsed = 0;
            timer.expiration = 0;
            state = State::Inactive;
        }
    } else if (state == State::Swinging) {
        if (timer.expired()) {
            Hit();
        }
    }
    sprite.flip =
        owner->mstate.FacingLeft() ? Flip_Mode::HORIZONTAL : Flip_Mode::NONE;
}
void Weapon::Attack() {
    if (state == State::Inactive) {
        auto data = weapon_get_data(type);
        if (data->swing > 0.0_s) {
            sprite.change_to_animation(1);
            sprite.Restart();
            state = State::Swinging;
            timer.elapsed = 0;
            timer.expiration = data->swing;
        } else {
            Hit();
        }
        data->swing_sound.play();
    }
}
void Weapon::Hit() {
    auto data = weapon_get_data(type);
    auto aabb = getAABB();
    timer.elapsed = 0;
    timer.expiration = data->cooldown;
    state = State::Cooldown;
    sprite.change_to_animation(2);
    sprite.Restart();
    if (data->type == Weapon_Data::Attack_Type::RANGED) {
        Projectile new_projectile = {};
        new_projectile.texture = data->projectile.texture;
        new_projectile.hit_sound = data->hit_sound;
        new_projectile.location = owner->location;
        {
            auto flip = owner->mstate.FacingLeft();
            new_projectile.pos = {flip ? aabb.r() : aabb.l(), aabb.cy()};
            new_projectile.vel = data->projectile.vel;
            if (flip) {
                new_projectile.vel.x = -new_projectile.vel.x;
            }
        }
        new_projectile.hitbox = data->projectile.hitbox;
        new_projectile.damage = data->damage;
        new_projectile.hurts_enemies = True;
        new_projectile.hurts_player = False;
        new_projectile.knockback = data->enemy_knockback;
        entity_manager->projectiles.push(new_projectile);
    } else {
        Bool should_play_sound = False;
        {
            ng_for(entity_manager->enemies) {
                if (it.location == owner->location && it.alive()) {
                    auto enem_aabb = it.get_data()->hitbox + it.pos;
                    if (aabb.intersects(enem_aabb)) {
                        should_play_sound = True;
                        auto spark_x = ng::clamp(it.pos.x, aabb.l(), aabb.r());
                        auto spark_y = ng::clamp(it.pos.y, aabb.t(), aabb.b());
                        particle_system->Emit(
                            Particle_Type::SPARK, {spark_x, spark_y},
                            (owner->vel + it.vel) / 2 +
                                fx_rng.get({-128, -128}, {128, 128}));
                        particle_system->Emit(Particle_Type::SMOKE_8,
                                              {spark_x, spark_y},
                                              fx_rng.get({-32, -32}, {32, 32}));
                        {
                            auto calculated_knockback = data->enemy_knockback;
                            if (owner->mstate.FacingRight()) {
                                calculated_knockback.x *= +1;
                            } else {
                                calculated_knockback.x *= -1;
                            }
                            it.take_damage(data->damage, calculated_knockback);
                        }
                    }
                }
            }
            ng_for(entity_manager->projectiles) {
                if (owner->location == it.location) {
                    auto proj_aabb = it.hitbox + it.pos;
                    if (aabb.intersects(proj_aabb)) {
                        do_hit_projectile(&it, owner->vel);
                    }
                }
            }
        }
        auto room = world->get_room(owner->location);
        auto collisions = room->get_colliding_tiles(aabb);
        defer { collisions.release(); };
        {
            auto flip = owner->mstate.FacingLeft();
            Bool did_hit_a_block = False;
            auto spark_x = flip ? aabb.l() : aabb.r();
            ng_for(collisions) {
                if (check_collided(it.tile)) {
                    should_play_sound = True;
                    did_hit_a_block = True;
                    {
                        auto new_spark_x =
                            tile_to_game(it.x + (flip ? 1_t : 0_t));
                        if (flip ? (new_spark_x > spark_x)
                                 : (new_spark_x < spark_x)) {
                            spark_x = new_spark_x;
                        }
                    }
                }
                do_hit_block(room, it);
            }
            if (did_hit_a_block) {
                auto spark_y = (aabb.cy());
                particle_system->Emit(Particle_Type::SPARK, {spark_x, spark_y},
                                      (owner->vel) /
                                              2 + // avg owner's & block's vel
                                          fx_rng.get({-128, -128}, {128, 128}));
                particle_system->Emit(Particle_Type::SMOKE_8,
                                      {spark_x, spark_y},
                                      fx_rng.get({-32, -32}, {32, 32}));
            }
        }
        if (should_play_sound) {
            data->swing_sound.stop();
            data->hit_sound.play();
        }
        if (data->lift_amount != 0) {
            if (!owner->mstate.Grounded()) {
                owner->vel.y = data->lift_amount;
                if (owner->mstate.Jumping()) {
                    owner->mstate.Fall();
                }
            }
        }
    }
}
Rect Weapon::getAABB() {
    auto data = weapon_get_data(type);
    Rect result = data->hitbox;
    result.Flip(owner->mstate.FacingLeft() ? Flip_Mode::HORIZONTAL
                                           : Flip_Mode::NONE);
    result.x += owner->pos.x;
    result.y += owner->pos.y;
    return result;
}
Weapon_Data *weapon_get_data(Weapon_Type type) {
    return &weapon_data[cast(int) type];
}
void Weapon::Render(Vector2 offset) {
    if (sprite.animations.count == 0) {
        return;
    }
    auto aabb_pos = owner->pos;
    if (sprite.flip == Flip_Mode::HORIZONTAL) {
        aabb_pos.x -= sprite.current_anim()->offset.x * 2._g;
        aabb_pos.x -= sprite.current_anim()->atlas.w * 1._g;
    }
    aabb_pos += offset;
    auto pos = camera->game_to_local(aabb_pos);
    ::Render(renderer, &sprite,
             /*game_to_px*/ (pos.x),
             /*game_to_px*/ (pos.y));
}
