#include "player.h"
#include "camera.h"
#include "engine_state.h" // @Remove
#include "filesystem.h"
#include "input.h"
#include "particle.h"
#include "ui.h"
using ng::operator""_s;
Bool within_enemy_ai_range(Player *p, Enemy *e);
const auto focus_x_offset = tile_to_game(+1_t);
const auto focus_y_offset = tile_to_game(-2_t);
const auto focus_y_airborne_offset = tile_to_game(-1_t);
const float focus_x_vel_multiplier = 0.25f;
Vector2 half_life_independentifier(Camera *camera) {
    // Most values were calculated with these half life values during testing.
    return {0.25f / camera->half_life.x, 0.125f / camera->half_life.y};
}
float focus_mul_to_fix_camera(float camera_half_life) {
    return 1.415f * camera_half_life; // Black magic. Is it root 2? I don't know
}
float focus_y_vel_multiplier(float y_vel, float gravity, Camera *camera) {
    (void)y_vel;
    (void)gravity;
    (void)camera;
    if (y_vel > 0.0_g) {
        return focus_mul_to_fix_camera(camera->half_life.y);
    }
    return 0.15f * half_life_independentifier(camera).y;
}
const auto damageboost_time = 0.25_s;
const auto damagestun_time = 0.50_s;
Player *player = null, *player_sbs = null;
static Player::CharacterController getctrl() {
    Player::CharacterController r = {};

    const auto walk_speed = tile_to_game(1_t) * 5.0f;

    r.walkSpeed = walk_speed;

    const auto jump_height = tile_to_game(1_t) * 3.1f;
    const auto jump_distance = tile_to_game(1_t) * 5.1f;

    const auto apex_height = jump_height;
    const auto apex_distance = jump_distance / 2;
    const auto apex_time = apex_distance / walk_speed;

    // g = -2h/(t_h^2)         but down is positive so it's >0.
    const auto jump_gravity = 2 * apex_height / (apex_time * apex_time);
    r.jumpingGravity = jump_gravity;
    r.fallingGravity = jump_gravity * 1.0f;
    const auto jump_vel = 2 * apex_height / apex_time;
    r.jumpVel = jump_vel;

    const auto jump_post_distance = tile_to_game(1_t) * 0.5f;
    const auto jump_pre_height = tile_to_game(1_t) * 1.5f;

    r.jump_post_time = jump_post_distance / walk_speed;
    r.jump_pre_time = jump_pre_height / jump_vel;

    // r.fallingGravity = tile_to_game(32_t);
    // r.jumpingGravity = tile_to_game(22_t);
    // r.jumpVel = tile_to_game(12_t);
    // r.walkSpeed = tile_to_game(5_t);
    r.walkAccel = r.walkSpeed * 8.0f;
    r.turnAccel = r.walkAccel * 2.0f;
    r.friction = r.walkAccel * 0.5f;
    r.airSpeed = r.walkSpeed * 1.0f;
    r.airAccel = r.walkAccel * 0.75f;
    r.airTurn = r.turnAccel * 0.75f;
    r.airDrag = r.friction * 0.75f;
    // r.jump_post_time = 0.133_s;
    // r.jump_pre_time = 0.133_s;
    r.jump_release_force = r.fallingGravity * 2;
    return r;
}
void Player::target_me(Camera *camera) {
    camera->target = {&focus_target, &location};
}
void Player::init() {
    *this = Player{};
    max_health = 10_hp;
    health = max_health;
    ctrl = getctrl();
    damage_invincibility.expiration = damagestun_time + damageboost_time;
    damage_invincibility.expire();
    jump_pre_input.expiration = ctrl.jump_pre_time;
    jump_pre_input.expire();
    jump_post_input.expiration = ctrl.jump_post_time;
    jump_post_input.expire();
    jump_powerup = True;
    sounds.push(sound_from_filename("data/sound/land.wav"_s))
        ->update_volume(0.f);
    sounds.push(sound_from_filename("data/sound/jump_launch.wav"_s))
        ->update_volume(1.f);
    sounds.push(sound_from_filename("data/sound/save_spot.wav"_s))
        ->update_volume(1.f);
    sprite.animations.push(animation_create("data/sprites/player/stand.png"_s,
                                            16_px, 16_px, 3_fps, {-8, -8}));
    sprite.animations.push(animation_create("data/sprites/player/run.png"_s,
                                            16_px, 24_px, 18_fps, {-8, -16}));
    sprite.animations.push(animation_create("data/sprites/player/skid.png"_s,
                                            16_px, 16_px, 8_fps, {-8, -8}));
    sprite.animations.push(animation_create("data/sprites/player/air.png"_s,
                                            16_px, 24_px, 0_fps, {-8, -12}));
    sprite.animations.push(
        animation_create("data/sprites/player/fall_start.png"_s, 16_px, 24_px,
                         0_fps, {-8, -12}));
    sprite.animations.push(
        animation_create("data/sprites/player/fall_loop.png"_s, 16_px, 24_px,
                         14_fps, {-8, -12}));
    sprite.animations.push(animation_create(
        "data/sprites/player/stun_air.png"_s, 16_px, 32_px, 8_fps, {-8, -17}));
    sprite.animations.push(
        animation_create("data/sprites/player/stun_ground.png"_s, 16_px, 16_px,
                         4_fps, {-8, -8}));
    eyes.animations.push(animation_create("data/sprites/player/eyes_main.png"_s,
                                          8_px, 2_px, 7_fps, {-3, -4}));
    eyes.animations.push(
        animation_create("data/sprites/player/eyes_main_stun.png"_s, 8_px, 2_px,
                         15_fps, {-3, -4}));
    eyes.animations.push(animation_create("data/sprites/player/eyes_hurt.png"_s,
                                          8_px, 2_px, 7_fps, {-3, -4}));
    eyes.animations.push(
        animation_create("data/sprites/player/eyes_hurt_stun.png"_s, 8_px, 2_px,
                         15_fps, {-3, -4}));
    hitbox = {-5, -8, 10, 16};
    colliders.x = {-5, -3, 10, 6};
    colliders.y = {-3, -8, 6, 16};
}
void Player::Jump() {
    do_jump_release = False;
    {
        sounds[1].stop(); // @Todo @Hack: why didn't I use retrigger?
        sounds[1].play();
    }
    jump_post_input.expire();
    jump_pre_input.expire();
    vel.y = -ctrl.jumpVel;
    mstate.Jump();
    ng_for(entity_manager->enemies) { // respond to player jump
        if (it.location == location && it.alive()) {
            it.respond_to_player_jump(this);
        }
    }
}
void Player::Fall() { mstate.Fall(); }
// @Todo: Majorly simplify HandleInput to just flip some bools, and do all these
// calculations in update(). Way too many times, I'm needing dt.
void Player::HandleInput(Input_State *in) {
    in_vec = input_get_analog_vector(in);
    // clang-format off
    a = in->keys[KEY(Z)]?true:false; a_new = a_new || in->keys_new[KEY(Z)];
    b = in->keys[KEY(X)]?true:false; b_new = b_new || in->keys_new[KEY(X)];
    x = in->keys[KEY(A)]?true:false; x_new = x_new || in->keys_new[KEY(A)];
    y = in->keys[KEY(S)]?true:false; y_new = y_new || in->keys_new[KEY(S)];
    // down = in->keys[KEY(DOWN)]; down_new = down_new || in->keys_new[KEY(DOWN)];
    wants_to_interact = wants_to_interact || in->keys_new[KEY(DOWN)];
    // clang-format on
}
void Player::update(f32 dt, Room *room) {
    // handle input
    f32 gravity = ctrl.fallingGravity;
    if (a_new) {                    // even when stunned
        jump_pre_input.elapsed = 0; // will jump this frame if possible
    }
    Bool ground = mstate.Grounded();
    if (!mstate.stunned) {
        if (a && jump_powerup) {
            gravity = ctrl.jumpingGravity;
        }
        {
            Bool pre = !jump_pre_input.expired() || a_new;
            Bool post = !jump_post_input.expired();
            if (pre && (ground || post)) {
                Jump();
            }
            // ng::print("%0%1", pre ? "pre\r\n" : "", post ? "post\r\n" : "");
        }
        Bool can_flip = True;
        if (arsenal.count > 0) {
            auto wep = &arsenal[current_weapon];
            auto wdata = weapon_get_data(wep->type);
            if (x_new) {
                Bool can_switch_weapons =
                    wep->state == Weapon::State::Inactive &&
                    True; // entity_manager->projectiles.count <= 0;
                if (can_switch_weapons) {
                    current_weapon = (current_weapon + 1) % arsenal.count;
                }
            }
            Bool attack_button =
                wdata->mode == Weapon_Data::Trigger_Mode::HOLD ? b : b_new;
            if (attack_button) {
                wep->Attack();
            }
            Bool in_first_half_of_swing =
                !(wep->state == Weapon::State::Swinging &&
                  wep->timer.elapsed > wep->timer.expiration / 2);
            if (wep->state == Weapon::State::Cooldown ||
                !in_first_half_of_swing) {
                can_flip = False;
            }
        }
        if (in_vec.x != 0 && can_flip) {
            in_vec.x > 0 ? mstate.FaceRight() : mstate.FaceLeft();
        }
        if (in_vec.x != 0) {
            f32 accelerations[4] = {ctrl.walkAccel, ctrl.turnAccel,
                                    ctrl.airAccel, ctrl.airTurn};
            Bool turn = sign(vel.x) != sign(in_vec.x);
            Bool air = !ground;
            accel.x =
                accelerations[turn ? (air ? 3 : 1) : (air ? 2 : 0)] * in_vec.x;
        }

    } else {
    }
    accel.y = gravity;
    if (!a && mstate.Jumping()) {
        do_jump_release = True;
        Fall();
    }

    // update
    if (!dead) {
        if (do_jump_release) {
            vel.y += ctrl.jump_release_force * dt;
            if (vel.y >= 0.0_g) {
                do_jump_release = False;
            }
        }
        if (mstate.stunned && !ground) {
            damage_invincibility.elapsed = 0;
        }
        damage_invincibility.elapsed += dt;
        if (damage_invincibility.elapsed > damagestun_time) {
            mstate.stunned = False;
        }
        jump_post_input.elapsed += dt;
        jump_pre_input.elapsed += dt;
        sprite.update(dt);
        bleed_timer.elapsed += dt;
        eyes.update(dt);
        ng_for(arsenal) { it.update(dt); }
        if (dt > 0.0_g) {
            f32 max_speed = mstate.Grounded() ? ctrl.walkSpeed : ctrl.airSpeed;
            { // limit max speed when kiting
                Bool moving_forward =
                    sign(vel.x) != (mstate.FacingRight() ? -1 : 1);
                Bool backtracking = !moving_forward;
                if (arsenal.count > 0) {
                    auto wep = &arsenal[current_weapon];
                    auto data = weapon_get_data(wep->type);
                    Bool swinging = wep->state == Weapon::State::Swinging ||
                                    wep->state == Weapon::State::Cooldown;
                    if (backtracking && swinging) {
                        if (data->type == Weapon_Data::Attack_Type::RANGED) {
                            max_speed *= .25f;
                        } else {
                            max_speed *= .75f;
                        }
                    }
                }
            }
            // @Todo: is this correct?
            if (ng::abs(vel.x) > ng::abs(max_speed)) {
                vel.x /= ng::abs(vel.x);
                vel.x *= ng::abs(max_speed);
            }
        }
        friction = 0.0_g;
        if (accel.x == 0.0_g) {
            friction = (mstate.Grounded() ? ctrl.friction : ctrl.airDrag);
        }
        if (mstate.stunned) {
            friction = (mstate.Grounded() ? ctrl.friction
                                          : 0._g); // ballistic trajectory
        }
        if (vel.x != 0.0_g) {
            vel.x = vel.x > 0._g ? ng::max(0._g, vel.x - friction * dt)
                                 : ng::min(0._g, vel.x + friction * dt);
            // if (friction != 0.0_g) {
            //     vel.x = 0.0_g;
            // }
        }
        auto new_pos = pos;
        new_pos += (vel * dt) + (accel * dt * dt);
        auto delta = new_pos - pos;
        vel += accel * dt;
        accel = {0, 0};
        if (dt > 0._s) { // collision
            Collide_State cstate = {};
            cstate.pos = pos;
            cstate.delta = delta;
            cstate.collider_bottom = {colliders.y.x, colliders.y.cy(),
                                      colliders.y.w, colliders.y.h / 2};
            cstate.collider_top = {colliders.y.x, colliders.y.y, colliders.y.w,
                                   colliders.y.h / 2};
            cstate.collider_right = {colliders.x.cx(), colliders.x.y,
                                     colliders.x.w / 2, colliders.x.h};
            cstate.collider_left = {colliders.x.x, colliders.x.y,
                                    colliders.x.w / 2, colliders.x.h};
            auto result = collide(cstate, room);
            { // bottom collision extra logic
                Bool was_grounded = mstate.Grounded();
                Bool was_jumping = mstate.Jumping();
                if (result.bottom.did_collide) {
                    if (!was_grounded) {
                        mstate.Land();
                        do_jump_release = False;
                        const auto min_land_vel = tile_to_game(05_t);
                        const auto max_land_vel = tile_to_game(50_t);
                        if (vel.y > min_land_vel) { // play a landing sound
                            auto land_vel = ng::min(vel.y, max_land_vel);
                            land_vel -= min_land_vel;
                            auto range = max_land_vel - min_land_vel;
                            auto ratio = land_vel / range;
                            sounds[0].update_volume(ratio);
                            sounds[0].play();
                        }
                    }
                    vel.y = 0._g; // unconditionally zero y-vel to "clip" floor
                } else {
                    if (was_grounded) {
                        if (vel.y < 0) {
                            mstate.Jump();
                        }
                        jump_post_input.elapsed = 0;
                    }
                    if (was_grounded || was_jumping) {
                        if (vel.y >= 0) {
                            Fall();
                        }
                    }
                }
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
            { // check for extras after player is in a fully valid position
                Bool took_an_exit = False;
                Bool any_save_spots = False;
                auto check_interactable = [&](Room_Collision_Info *ci) {
                    if (mstate.stunned) {
                        return;
                    }
                    auto is_a_save_spot =
                        ci->type() == Collision_Tile_Type::SAVE_SPOT;
                    if (is_a_save_spot) {
                        any_save_spots = True;
                    }
                    if (ci->tile.is_exit()) {
                        int index = ci->tile.exit_number();
                        auto exit_info = room->get_exit_info(index);
                        switch (cast(Room_Exit_Mode) exit_info.mode) {
                        default: {
                        } break;
                        case (Room_Exit_Mode::ON_TOUCH): {
                            TakeExit(ci);
                            took_an_exit = True;
                        } break;
                        case (Room_Exit_Mode::ON_INTERACTION): {
                            if (wants_to_interact) {
                                TakeExit(ci);
                                took_an_exit = True;
                            } else {
                                ui->interaction_prompt =
                                    True; // cleared each update
                            }
                        } break;
                        }
                    } else if (is_a_save_spot) {
                        if (wants_to_interact) {
                            savegame_create_and_write(this, ci, &engine_state);
                            sounds[2].play();
                            ui->savegame_exists = True; // @Remove?
                        } else {
                            ui->interaction_prompt =
                                True; // cleared each update
                        }
                    }
                };
                auto collisions = room->get_colliding_tiles(hitbox + pos);
                defer { collisions.release(); };
                ng_for(collisions) { check_interactable(&it); }
                ui->should_render_save_icon = any_save_spots;
                Room_Collision_Test_Result *collision_test_results[4] = {
                    &result.bottom, &result.top, &result.right, &result.left};
                ng_for(collision_test_results) {
                    if ((it)->did_hit_ontouch_exit) {
                        check_interactable(&(it)->first_ontouch_exit);
                    }
                }
                if (took_an_exit) {
                    return;
                }
                Bool took_damage = False;
                auto max_damage_taken = 0_hp;
                ng_for(entity_manager->enemies) {
                    if (it.location == room->id && it.alive()) {
                        auto aabb = hitbox + pos;
                        auto data = it.get_data();
                        auto enemy_aabb = data->player_collider + it.pos;
                        auto collided = aabb.intersects(enemy_aabb);
                        if (collided) {
                            // @Bug: Goomber doesn't hurt unmoving player...?
                            if (it.type == Enemy_Type::CHARGING_CHEETAH) {
                                if (ng::abs(it.vel.x) <
                                    tile_to_game(3_t)) { // @Hack @FixMe
                                    took_damage = False;
                                } else {
                                    took_damage = True;
                                }
                            } else if (it.type == Enemy_Type::RUNNY_RABBIT) {
                                if (!it.ground) {
                                    // Airborne, so we assume jumping.
                                    // @Incomplete: doesn't check if falling.
                                    took_damage = True;
                                }
                            } else if (it.type == Enemy_Type::CANNON_TOAST) {
                                took_damage = False;
                            } else {
                                took_damage = True;
                            }
                            if (took_damage) {
                                auto amt = data->damageplayer_amount;
                                if (max_damage_taken < amt) {
                                    max_damage_taken = amt;
                                }
                            }
                        }
                        // auto dir = (it.pos - pos).hat();
                        // if (dir.magsq() == 0) {
                        //     dir = {1, 0};
                        // }
                        // dir *= tile_to_game(128_t);
                        // if (vel.mag() < tile_to_game(6_t)) {
                        //     vel -= dir * dt;
                        // }
                    }
                }
                if (took_damage) {
                    // still stuns even with 0 damage
                    player->take_damage(max_damage_taken);
                }
                ng_for(entity_manager->npcs) {
                    if (it.location == room->id) {
                        auto aabb = hitbox + pos;
                        if (aabb.intersects(it.hitbox + it.pos)) {
                            if (wants_to_interact && !mstate.stunned) {
                                if (it.talk_string_id) {
                                    ui->text_box.ChangeToTextString(
                                        it.talk_string_id);
                                    if (ui->text_box.isClosed()) {
                                        ui->text_box.Open();
                                    }
                                }
                            } else {
                                ui->interaction_prompt = True;
                            }
                        }
                    }
                }
            }
        }
        draw_rect(hitbox + pos);
        if (is_targeting(camera, &focus_target, &location)) {
            update_focus_target();
        }
    } else {
        constexpr auto fade_out_time = 0.125_s;
        constexpr auto fade_in_time = 0.125_s;
        // dead, waiting for player to press a button to restart.
        if (engine_state.mode == Engine_Mode::IN_ENGINE) {
            if (!restarting && (a_new || b_new || x_new || y_new)) {
                // @Migrate all this to a more appropriate place.
                ui->start_fade(Fade_Type::OUT, Fade_Color::WHITE,
                               fade_out_time);
                // audio->fade_music_out(fade_out_time);
                restarting = True;
            }
            if (restarting && ui->fade_timer.expired()) { // @Hack
                // @Todo: run a proper restart routine and not this @Hacky stuff
                // @Hack @Todo @Bug: Particles don't restart using this.
                void begin_game(Engine_State * engine_state);
                begin_game(&engine_state);
                // @Hack: we must start_fade() after calling begin_game(),
                // because that calls room_transition() which does a
                // room-info-based fade.
                ui->start_fade(Fade_Type::IN, Fade_Color::WHITE, fade_in_time);
                restarting = False;
            }
        }
    }

    UpdateSprites();

    // reset input bools for next update
    a = False;
    a_new = False;
    b = False;
    b_new = False;
    x = False;
    x_new = False;
    y = False;
    y_new = False;
    down = False;
    down_new = False;
    wants_to_interact = False;
}
void Player::take_damage(u16 damage) {
    if (!damage_invincibility.expired()) {
        return;
    }
    health -= damage;
    if (!alive()) {
        Die();
    }
    damage_invincibility.elapsed = 0;
    auto horizontal_recoil = -60._g;
    auto vertical_recoil = -120._g;
    auto x_recoil =
        mstate.FacingRight() ? horizontal_recoil : -horizontal_recoil;
    vel = {x_recoil, vertical_recoil};
    mstate.ground = MovementState::GroundState::Falling;
    if (arsenal.count > 0) {
        arsenal[current_weapon].state = Weapon::State::Inactive;
    }
    for (int i = 0; i < 32; i += 1) {
        particle_system->Emit(Particle_Type::BLEED, pos,
                              fx_rng.get({-2, -2}, {2, 0}) * 120.0_g, 1.0_s);
    }
    // auto anim = sprite.current_anim();
    Vector2 spark_off = {hitbox.w / 2, hitbox.h / 2};
    auto spark_data = get_particle_data(Particle_Type::SPARK);
    auto lifetime = spark_data->default_lifetime * 1.5f;
    for (int i = 0; i < 4; i += 1) {
        particle_system->Emit(Particle_Type::SPARK,
                              pos + fx_rng.get(-spark_off, spark_off),
                              fx_rng.get({-1, -1}, {1, 1}) * 120.0_g, lifetime);
    }
    // @Todo: 'Player has taken damage' sound!!! @@@
    // @Todo: 'Player has taken damage' sound!!! @@@
    // @Todo: 'Player has taken damage' sound!!! @@@
    // @Todo: 'Player has taken damage' sound!!! @@@
    // @Todo: 'Player has taken damage' sound!!! @@@
    // @Todo: 'Player has taken damage' sound!!! @@@
    // @Todo: 'Player has taken damage' sound!!! @@@
    // @Todo: 'Player has taken damage' sound!!! @@@
    mstate.stunned = True;
    sprite.Restart();
    eyes.Restart();
}
void Player::Die() {
    dead = True;
    const Vector2 min = {-50, -25};
    const Vector2 max = {+50, +25};
    void do_death_effects(Texture * t, Vector2 pos);
    do_death_effects(&sprite.current_anim()->atlas.base, pos);
    camera->target = {&pos, &location};
    vel = {0, 0};
    ui->text_box.Close();
    ui->text_box.ChangeToTextString(0xFFFF);
    // audio->play_song("data/music/failure_0.ogg"_s, 0.1_s, 0.0_s, False);
    health = 0_hp;
#if 0
  {
    // @Remove
    init();
    location = world->cur_id();
    target_me();
  }
#endif
}
void Player::UpdateSprites() {
    // eye position variables
    enum eyepos {
        None,

        Main,
        Jumping,
        RunBob,
        StunAir,
        StunGround1,
        StunGround2,
        StunGround3,
        StunGround4,

        NumEyePositions
    } epos = Main;
    if (mstate.stunned) {
        if (mstate.Grounded()) {
            if (sprite.current_animation_index != 7) {
                sprite.change_to_animation(7);
                auto anim = sprite.current_anim();
                // Ensure the whole animation plays
                auto time_left = damagestun_time - damage_invincibility.elapsed;
                ng_assert(time_left > 0);
                anim->frametime =
                    (time_left / anim->atlas.num_frames) + 0.01_s; // @Hack
            }
            auto anim = sprite.current_anim();
            { // check frame to move the eyes around
                auto idx = anim->atlas.index;
                epos = cast(eyepos)(StunGround1 + idx);
            }
        } else {
            sprite.change_to_animation(6);
            epos = StunAir;
        }
    } else {
        if (mstate.Grounded()) {
            if (vel.x != 0) {
                if (mstate.FacingRight() ==
                    vel.x > 0) {                   // facing movement direction
                    sprite.change_to_animation(1); // running
                    auto tex = sprite.current_anim();
                    // {
                    //   auto original_fps = 18fps;
                    //   auto ratio_of_speed_to_max_speed =
                    //       (ng::abs(vel.x) / ctrl.walkSpeed);
                    //   auto new_fps = u8(ratio_of_speed_to_max_speed *
                    //   original_fps);
                    //   tex->frametime = u16(fps_to_ms(new_fps));
                    // }
                    { // check for bobbing up in animation for eyes
                        // auto elapsed = tex->timer.elapsed;
                        // auto expiration = tex->timer.expiration;
                        // int frame_num = elapsed / expiration;
                        int frame_num = tex->atlas.index;
                        frame_num %= 6; // eyes in 6 frame loop
                        if (frame_num == 2 ||
                            frame_num == 3) { // 3rd and 4th frames bob up
                            epos = RunBob;
                        }
                    }
                } else {
                    sprite.change_to_animation(2); // skidding
                }
            } else {
                // standing
                sprite.change_to_animation(0);
            }
        } else {
            if (vel.y >= 0.0_g) {
                if (vel.y > 192.0_g) {
                    // fall loop
                    sprite.change_to_animation(5);
                } else {
                    // fall start
                    sprite.change_to_animation(4);
                    if (vel.y > 96.0_g) {
                        sprite.current_anim()->atlas.MoveTo(1);
                    } else {
                        sprite.current_anim()->atlas.MoveTo(0);
                    }
                }
            } else {
                // jump
                sprite.change_to_animation(3);
                epos = Jumping;
            }
        }
    }
    sprite.flip = mstate.FacingLeft() ? Flip_Mode::HORIZONTAL : Flip_Mode::NONE;
    { // change eye animation
        auto stunned = mstate.stunned;
        auto halfdead = (health <= max_health / 2);
        if (halfdead) {
            if (stunned) {
                eyes.change_to_animation(3);
            } else {
                eyes.change_to_animation(2);
            }
        } else {
            if (stunned) {
                eyes.change_to_animation(1);
            } else {
                eyes.change_to_animation(0);
            }
        }

        if (bleed_timer.expired()) { // bleed particles
            bleed_timer.elapsed = 0;

            int num_particles = max_health - health;

            for (int i = 0; i < num_particles; i += 1) {
                auto type = Particle_Type::BLEED;
                auto ratio = 0.5f;
                if (num_particles > 1) {
                    ratio = cast(float) i / num_particles;
                }
                Vector2 partvel = {};
                partvel.x = ng::lerp(-192._g, +192._g, ratio);
                partvel.y = fx_rng.get(-224._g, -192._g, &rng_mostly_min);
                particle_system->Emit(
                    type, pos + fx_rng.get({-1, -1}, {+1, +1}) * 6,
                    partvel + vel,
                    get_particle_data(type)->default_lifetime +
                        fx_rng.get(-0.25_s, +0.25_s));
            }
        }
    }
    Vector2 general_eye_offset = {-4_px, -1_px}; // relative to top-left

    Vector2 eye_offsets[NumEyePositions] = {}; // relative to center

    eye_offsets[Main] /*        */ = {+1_px, -3_px};
    eye_offsets[Jumping] /*     */ = {+1_px, -4_px};
    eye_offsets[RunBob] /*      */ = {+1_px, -4_px};
    eye_offsets[StunAir] /*     */ = {+1_px, -3_px};
    eye_offsets[StunGround1] /* */ = {+0_px, +1_px};
    eye_offsets[StunGround2] /* */ = {+0_px, +0_px};
    eye_offsets[StunGround3] /* */ = {+0_px, -1_px};
    eye_offsets[StunGround4] /* */ = {+0_px, -2_px};

    Vector2 offset = eye_offsets[epos];
    if (sprite.flip == Flip_Mode::HORIZONTAL) {
        offset.x = -offset.x;
    }
    eyes.current_anim()->offset = offset + general_eye_offset;
}
void Player::TakeExit(Room_Collision_Info *info) {
    wants_to_interact = False; // won't be updated again until in other room
    ng_for(Player::sounds) { it.stop(); }
    auto exit_info =
        world->get_room(location)->get_exit_info(info->tile.exit_number());
    ng_assert(cast(Room_Exit_Mode) exit_info.mode != Room_Exit_Mode::INACTIVE);

    engine_state.mode = Engine_Mode::TRANSITION_FREEZE;

    player->transitioning_rooms = True;
    player->room_exit_info = exit_info;

    auto room = world->get_room(exit_info.destination);
    ui_apply_room_fade(ui, room);
}
void Player::update_focus_target() {
    focus_target = pos;
    if (camera->smooth) {
        auto x_factor = focus_x_vel_multiplier;
        if (arsenal.count > 0 &&
            weapon_get_data(arsenal[current_weapon].type)->type ==
                Weapon_Data::Attack_Type::RANGED)
            x_factor *= 2;
        Vector2 offset{vel.x * x_factor,
                       vel.y * focus_y_vel_multiplier(vel.y, accel.y, camera)};
        offset.x += (mstate.FacingRight() ? +1.f : -1.f) * focus_x_offset;
        if (mstate.Grounded()) {
            offset.y += focus_y_offset;
        } else {
            offset.y += focus_y_airborne_offset;
        }
        // offset *= ng::clamp(cast(float) globals.screen_width / DEFAULT_WIDTH,
        //                     0.25f, 1.0f);
        focus_target += offset;
    }
    {
        auto t_x = 0.01f / camera->half_life.x;
        auto t_y = 0.01f / camera->half_life.y;
        auto r = world->get_room(location);
        auto w = tile_to_game(r->w);
        auto h = tile_to_game(r->h);
        auto inner_bound_x = ng::lerp(globals.screen_center.x, 0.0_g, t_x);
        auto inner_bound_y = ng::lerp(globals.screen_center.y, 0.0_g, t_y);
        auto min_x = inner_bound_x;
        auto max_x = w - inner_bound_x;
        auto min_y = inner_bound_y;
        auto max_y = h - inner_bound_y;
        if (min_x >= max_x) {
            focus_target.x = (w / 2);
        } else {
            focus_target.x = ng::clamp(focus_target.x, min_x, max_x);
        }
        if (min_y >= max_y) {
            focus_target.y = (h / 2);
        } else {
            focus_target.y = ng::clamp(focus_target.y, min_y, max_y);
        }
    }
    draw_rect({focus_target.x, focus_target.y, 1._g, 1._g});
}
void Player::Render(Camera *camera) {
    if (dead) {
        return;
    }
    if (location != camera->location) {
        return;
    }
    Vector2 final_pos = camera->game_to_local(pos);
    auto should_render_sprite = True;
    {
        if (!mstate.stunned && !damage_invincibility.expired() &&
            cast(int)(damage_invincibility.elapsed / 0.016_s) % 2 < 1) {
            should_render_sprite = False;
        }
    }
    if (should_render_sprite) {
        ::Render(renderer, &sprite,
                 /*game_to_px*/ (final_pos.x),
                 /*game_to_px*/ (final_pos.y));
    }
    ::Render(renderer, &eyes,
             /*game_to_px*/ (final_pos.x),
             /*game_to_px*/ (final_pos.y));
    if (arsenal.count > 0) {
        auto wep = &arsenal[current_weapon];
        Bool should_be_holding_weapon = True;
        if (mstate.stunned) {
            should_be_holding_weapon = False;
        }
        if (should_be_holding_weapon) {
            Vector2 offset = {};
            { // get run-bob vert. offset from eyes
                auto animation = eyes.current_anim();
                s32 run_bob = animation->offset.y + 4_px;
                offset.y = run_bob;
            }
            wep->Render(offset);
            draw_rect(arsenal[current_weapon].getAABB());
        }
    }
}

void player_destroy(Player *player) {
    player->arsenal.release();
    player->sounds.release();
    sprite_destroy(&player->sprite);
}

Player player_copy(Player *rhs) {
    Player result = *rhs;
    result.sprite = sprite_copy(&rhs->sprite);
    result.arsenal = array_deep_copy(rhs->arsenal, &weapon_copy);
    return result;
}
