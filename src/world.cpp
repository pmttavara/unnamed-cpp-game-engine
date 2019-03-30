#include "world.h"
#include "camera.h"
#include "particle.h"
#include "player.h"
#include "ui.h"

ng::allocator *world_allocator = null;

const ng::string Room::MusicInfo::no_music = "none"_s;
const ng::string Room::MusicInfo::silence = "silence"_s;
Room_Exit Room::get_exit_info(int index) {
    ng_assert(index < exits.count);
    return exits[index];
}
static s16 layer_dim(s16 dim, s16 /*screen*/, u8 /*parallax*/) {
    s16 result = dim;
    // @Todo @Hack
    // @@ Figure out what the dimensions should be for
    // a parallax-mapped layer! Is it dependent on screen size?
    // If so, how to implement variable resolution?
    // In the meantime, much space is wasted.
    return result;
}
s16 layer_width(s16 w, u8 parallax) { return layer_dim(w, -1, parallax); }
s16 layer_height(s16 h, u8 parallax) { return layer_dim(h, -1, parallax); }
void Room::SetTile(layer_index index, s16 x, s16 y, Graphical_Tile new_tile) {
    layer_index i = index;
    ng_assert(i < numLayers());
    auto layer = &layers[i];
    auto lw = layer_width(w, layer->parallax);
    auto lh = layer_height(h, layer->parallax);
    if (x < 0 || y < 0 || x >= lw || y >= lh) {
        ng_assert(!"Tile index out of room bounds.");
    }
    layer->tiles[(x + (y * lw))] = new_tile;
}
void Room::Render(Room::LayerMode mode) {
    for (layer_index i = getBegin(mode); i < getEnd(mode); i += 1) {
        RenderLayer(i);
    }
}
void Room::RenderLayer(Room::layer_index i) {
    if (layers[i].tileset.base.tex == null) {
        return;
    }
    auto parallax = layers[i].parallax / 255.f;
    auto tiles = &layers[i].tiles;
    auto tileset = layers[i].tileset;
    auto lw = layer_width(w, u8(parallax));
    auto lh = layer_height(h, u8(parallax));
    Vector2 origin_rel_to_cam = camera->game_to_local({0, 0});
#if !NDEBUG // don't cull, we want to be able to use the editor nicely
    auto cull_min_x = cast(s16) 0;
    auto cull_min_y = cast(s16) 0;
    auto cull_max_x = cast(s16) lw;
    auto cull_max_y = cast(s16) lh;
#else
    auto cull_min = -origin_rel_to_cam * parallax;
    cull_min = Vector2::max(cull_min, {0, 0});
    auto cull_min_x = game_to_tile(cull_min.x);
    auto cull_min_y = game_to_tile(cull_min.y);
    auto cull_max =
        cull_min + Vector2{globals.screen_width + tile_to_game(1_t),
                           globals.screen_height + tile_to_game(1_t)};
    cull_max = Vector2::min(cull_max, {tile_to_game(lw), tile_to_game(lh)});
    auto cull_max_x = game_to_tile(cull_max.x);
    auto cull_max_y = game_to_tile(cull_max.y);
#endif
    auto layer_pos = origin_rel_to_cam * parallax;
    if (w < px_to_tile(globals.screen_width)) {
        layer_pos.x = origin_rel_to_cam.x;
    }
    if (h < px_to_tile(globals.screen_height)) {
        layer_pos.y = origin_rel_to_cam.y;
    }
    for (s16 y = cull_min_y; y < cull_max_y; y += 1) {
        for (s16 x = cull_min_x; x < cull_max_x; x += 1) {
            auto tile = &(*tiles)[x + y * lw];
            if (tile->index == 255) {
                // this is an empty tile
                continue;
            }
            auto tile_pos = layer_pos;
            tile_pos += Vector2{tile_to_game(x), tile_to_game(y)};
            tileset.MoveTo(tile->index);
            ::Render(renderer, &tileset,
                     /*game_to_px*/ (tile_pos.x),
                     /*game_to_px*/ (tile_pos.y), Flip_Mode::NONE);
        }
    }
}
ng::array<Room_Collision_Info> Room::get_colliding_tiles(Rect rect) {
    // @FixMe: this routine converts the game coords into tile coords, causing
    // an asymmetric truncation. The consequence is that every object which is
    // flush with a block to the right or to the bottom constantly triggers the
    // collision.
    ng::array<Room_Collision_Info> tiles = {world_allocator};
    auto left = game_to_tile(rect.l());
    auto right = game_to_tile(rect.r());
    auto top = game_to_tile(rect.t());
    auto bottom = game_to_tile(rect.b());
    for (auto y = top; y >= 0_t && y <= bottom && y < h; y += 1) {
        for (auto x = left; x >= 0_t && x <= right && x < w; x += 1) {
            Room_Collision_Info info = {};
            info.tile = at(x, y);
            info.x = x;
            info.y = y;
            tiles.push(info);
        }
    }
    if (rect.t() < 0) {
        Room_Collision_Info info = {};
        info.tile.type =
            Collision_Tile_Type::ROOM_EDGE_TOP; // @Todo: rename
                                                // Collision_Tile_Type
        info.x = 0;
        info.y = -1;
        tiles.push(info);
    }
    if (rect.b() > tile_to_game(h)) {
        Room_Collision_Info info = {};
        info.tile.type = Collision_Tile_Type::ROOM_EDGE_BOTTOM;
        info.x = 0;
        info.y = h;
        tiles.push(info);
    }
    if (rect.l() < 0) {
        Room_Collision_Info info = {};
        info.tile.type = Collision_Tile_Type::ROOM_EDGE_LEFT;
        info.x = -1;
        info.y = 0;
        tiles.push(info);
    }
    if (rect.r() > tile_to_game(w)) {
        Room_Collision_Info info = {};
        info.tile.type = Collision_Tile_Type::ROOM_EDGE_RIGHT;
        info.x = w;
        info.y = 0;
        tiles.push(info);
    }
    return tiles;
}
Room_Layer room_layer_create(ng::string tileset_filename, u8 parallax,
                             ng::array<Graphical_Tile> tiles) {
    Room_Layer result = {};
    result.tileset = texture_atlas_create(tileset_filename, tile_to_px(1_t),
                                          tile_to_px(1_t));
    result.parallax = parallax;
    result.tiles = tiles;
    return result;
}

World *world = null, *world_sbs = null;
Room *World::AddRoom(Room room) {
    auto id = room.id;
    ng_assert(get_room(id) == null, "Room %0 already exists.", id);
    ng_assert(rooms.count < 0xFFFF, "Too many rooms.");
    auto new_room = rooms.insert(id, (room));
    auto music_filename = new_room->music_info.filename;
    // defer { free_string(); };
    if (music_filename != Room::MusicInfo::silence &&
        music_filename != Room::MusicInfo::no_music) {
        audio->add_song(music_filename);
    }
    return new_room;
}
void World::DeleteRoom(u16 id) {
    auto found = rooms[id];
    if (found.found) {
        // @Todo: move this to a room_destroy() function or something.
        free_string(&found.value->music_info.filename, world_allocator);
        rooms.remove(id);
    }
}
Room *World::get_room(u16 id) {
    auto found = rooms[id];
    if (found.found) {
        return found.value;
    }
    return null;
}
void room_transition(World *world, Player *player) {
    player->jump_post_input.expire();
    player->jump_pre_input.expire();
    player->damage_invincibility.expire();
    player->mstate.stunned = False; // just in case it's somehow True
    player->mstate.Fall();
    if (player->arsenal.count > 0) {
        auto wep = &player->arsenal[player->current_weapon];
        auto wepdata = weapon_get_data(wep->type);
        wep->state = Weapon::State::Inactive;
        wepdata->swing_sound.stop();
        wepdata->hit_sound.stop();
    }
    player->sprite.Restart();
    player->eyes.Restart();
    auto room = world->get_room(player->location);
    auto exit_info = player->room_exit_info;
    player->location = exit_info.destination;
    if (!exit_info.preserve_x) {
        player->pos.x = exit_info.player_x;
    }
    if (!exit_info.preserve_y) {
        player->pos.y = exit_info.player_y;
    }
    if (!exit_info.preserve_vel) {
        player->vel = {0, 0};
    }
    player->pos.x += tile_to_game(exit_info.offset_x);
    player->pos.y += tile_to_game(exit_info.offset_y);
    if (is_targeting(camera, &player->focus_target, &player->location)) {
        // teleport camera to player
        auto old_smooth = camera->smooth;
        camera->smooth = False;
        player->update_focus_target();
        camera->update(0._s);
        camera->smooth = old_smooth;
    }
    entity_manager->projectiles.count = 0;
    { // visual stuff
        particle_system->ClearAll();
        ui->start_fade(Fade_Type::IN, ui->current_fade_color,
                       world->room_fade_in_length);
        ui->text_box.Close();
        ui->text_box.open_amount = 0.f;
        ui->text_box.ChangeToTextString(0xFFFF);
    }
    player->transitioning_rooms = False;
    player->room_exit_info = {};
    audio_apply_room_music(room->music_info);
}
Pickup_Data pickup_data[cast(usize) Pickup_Type::NUM_PICKUP_TYPES] = {};
Entity_Manager entity_manager_copy(Entity_Manager *rhs) {
    Entity_Manager result = {};
    result.enemies = array_deep_copy(rhs->enemies, &enemy_copy);
    result.npcs = array_deep_copy(rhs->npcs, &npc_copy);
    result.pickups = rhs->pickups.copy();
    result.projectiles = rhs->projectiles.copy();
    return result;
}
void entity_manager_destroy(Entity_Manager *em) {
    array_deep_destroy(&em->enemies, &enemy_destroy);
}
Entity_Manager *entity_manager = null, *entity_manager_sbs = null;
void do_hit_block(Room *room, Room_Collision_Info hit_block) {
    auto remove_block = [](Room *room, s16 x, s16 y) {
        { // remove collision tile
            room->at(x, y).type = Collision_Tile_Type::AIR;
        }
        { // remove graphical tile
            // @Incomplete: how exactly should this work?

            auto idx = x + y * layer_width(room->w, 255);

            // auto layer = &room->layers[room->firstForegroundLayer()];
            // auto lw = layer_width(room->w, layer->parallax);
            // layer->tiles[x + y * lw].index = 0xFF;
            ng_for(room->layers) {
                if (it.parallax == 255) {
                    it.tiles[idx].index = 0xFF;
                }
            }
        }
        { // particles
            for (auto i = fx_rng.get(2, 5); i > 0; i -= 1) {
                particle_system->Emit(
                    Particle_Type::SMOKE_16,
                    {tile_to_game(x) + (tile_to_game(1_t) / 2),
                     tile_to_game(y) + (tile_to_game(1_t) / 2)},
                    fx_rng.get({-64, -64}, {64, 64}));
            }
        }
    };
    switch (hit_block.type()) {
    default: {
    } break;
    case (Collision_Tile_Type::BREAKABLE): {
        remove_block(room, hit_block.x, hit_block.y);
    } break;
    case (Collision_Tile_Type::BREAKABLE_HEART): {
        remove_block(room, hit_block.x, hit_block.y);
        Pickup new_heart = {};
        new_heart.type = Pickup_Type::HEART;
        new_heart.info.heal_amount = BREAKABLE_HEART_HEAL_AMT;
        new_heart.location = room->id;
        new_heart.pos = {tile_to_game(hit_block.x) + (tile_to_game(1_t) / 2),
                         tile_to_game(hit_block.y) + (tile_to_game(1_t) / 2)};
        entity_manager->pickups.push((new_heart));
    } break;
    }
}
void do_hit_projectile(Projectile *proj, Vector2 other_entity_vel) {
    particle_system->Emit(Particle_Type::SPARK, proj->pos,
                          -(proj->vel + other_entity_vel) / 2 +
                              fx_rng.get({-128, -128}, {128, 128}));
    // @Todo: make particles "clump" less
    particle_system->Emit(Particle_Type::SMOKE_8, proj->pos,
                          fx_rng.get({-64, -64}, {+64, +64}));
    {
        auto volume = proj->hit_sound.volume;
        if (!camera->world_rect().intersects(proj->hitbox + proj->pos)) {
            auto ratio = (camera->pixel_pos() - proj->pos).mag() /
                         globals.screen_center.mag();
            ratio *= 2;
            volume /= ratio;
        }
        proj->hit_sound.update_volume(volume);
        proj->hit_sound.play();
    }
    proj->invalidate();
}
void do_hit_two_projectiles(Projectile *proj1, Projectile *proj2) {
    particle_system->Emit(Particle_Type::SPARK, (proj1->pos + proj2->pos) / 2,
                          -(proj1->vel + proj2->vel) / 2);
    particle_system->Emit(Particle_Type::SMOKE_8, (proj1->pos + proj2->pos) / 2,
                          fx_rng.get({-64, -64}, {+64, +64}));
    {
        // auto wr = camera->world_rect();
        auto volume1 = proj1->hit_sound.volume;
        if (!camera->world_rect().intersects(proj1->hitbox + proj1->pos)) {
            auto ratio = (camera->pixel_pos() - proj1->pos).mag() /
                         globals.screen_center.mag();
            ratio *= 2;
            volume1 /= ratio;
        }
        auto volume2 = proj2->hit_sound.volume;
        if (!camera->world_rect().intersects(proj2->hitbox + proj2->pos)) {
            auto ratio = (camera->pixel_pos() - proj2->pos).mag() /
                         globals.screen_center.mag();
            ratio *= 2;
            volume2 /= ratio;
        }
        proj1->hit_sound.update_volume(volume1 / 2);
        proj2->hit_sound.update_volume(volume2 / 2);
        proj1->hit_sound.play();
        proj2->hit_sound.play();
    }
    proj1->invalidate();
    proj2->invalidate();
}
Bool check_collided(Collision_Tile ct);
void Entity_Manager::update(f32 dt, World *world) {
    if (!camera) { // @Cleanup: Weird to depend on Camera for sim.
        return;
    }
    if (camera->location ==
        0xFFFF) { // @Cleanup: Weird to depend on Camera for sim.
        return;
    }
    if (player->location == camera->location) {
        auto room = world->get_room(player->location);
        player->update(dt, room);
    }
    {
        auto player_aabb = player->hitbox + player->pos;
        ng_for(projectiles) {
            if (it.location ==
                camera->location) { // Okay, important @Todo here: I know that I
                                    // only want to Render() things in the same
                                    // Room as the Camera, but I don't know
                                    // whether I want to update() things in the
                                    // same Room as the Camera or as the Player.
                                    // I'm leaning towards Camera. 2017-04-30

                auto proj_aabb = it.hitbox + it.pos;

                draw_rect(proj_aabb); // before adjustment
                {
                    auto delta_x = it.vel.x * dt;
                    if (delta_x < 0.f) {
                        proj_aabb.x += delta_x;
                    }
                    proj_aabb.w += ng::abs(delta_x);
                }
                {
                    auto delta_y = it.vel.y * dt;
                    if (delta_y < 0.f) {
                        proj_aabb.y += delta_y;
                    }
                    proj_aabb.h += ng::abs(delta_y);
                }
                draw_rect(proj_aabb); // after adjustment

                { // collide with world
                    auto proj = &it;
                    Bool hit_tile = False;
                    auto room = world->get_room(proj->location);
                    auto collisions = room->get_colliding_tiles(proj_aabb);
                    defer { collisions.release(); };
                    ng_for(collisions) {
                        if (check_collided(it.tile)) {
                            Bool should_do_effects = False; // @Todo: do all
                                                            // this for melee
                                                            // weapons too!

                            if (it.tile.is_exit()) {
                                auto exit_number = it.tile.exit_number();
                                if (exit_number <= 4) {

                                    auto edge_exit_info =
                                        room->get_exit_info(exit_number);
                                    auto mode = cast(Room_Exit_Mode)
                                                    edge_exit_info.mode;

                                    if (mode != Room_Exit_Mode::ON_TOUCH ||
                                        room->is_indoors) {
                                        should_do_effects = True;
                                    }
                                }
                            } else {
                                should_do_effects = True;
                            }

                            do_hit_block(room, it);
                            if (should_do_effects) {
                                do_hit_projectile(proj, {0, 0});
                            }
                            proj->invalidate();
                            hit_tile = True;
                            break;
                        }
                    }
                    if (hit_tile) {
                        continue;
                    }
                }
                { // collide with player
                    if (it.hurts_player) {
                        if (player->location == it.location &&
                            player->alive()) {
                            if (proj_aabb.intersects(player_aabb)) {
                                player->take_damage(it.damage);
                                do_hit_projectile(&it, player->vel);
                                continue;
                            }
                        }
                    }
                }
                { // collide with enemies
                    if (it.hurts_enemies) {
                        auto proj = &it;
                        int num_enemies_hit = 0;
                        Vector2 enemy_vel = {}; // summation of vels
                        ng_for(enemies) if (it.location == proj->location &&
                                            it.alive()) {
                            auto enemy_data = it.get_data();
                            auto enemy_aabb = enemy_data->hitbox + it.pos;
                            if (proj_aabb.intersects(enemy_aabb)) {
                                auto knockback = proj->knockback;
                                if (proj->pos.x > it.pos.x) {
                                    knockback.x = -knockback.x;
                                }
                                it.take_damage(proj->damage, knockback);
                                num_enemies_hit += 1;
                                enemy_vel += it.vel;
                            }
                        }
                        if (num_enemies_hit > 0) {
                            enemy_vel /=
                                cast(float) num_enemies_hit; // get average
                            do_hit_projectile(&it, enemy_vel);
                            continue;
                        }
                    }
                }
                { // collide with other projectiles
                    auto proj1 = &it;
                    ng_for(projectiles) {
                        auto proj2 = &it;
                        if (proj1 == proj2) {
                            // At this point, the other projectile is exactly
                            // the current one. This means that all of the
                            // projectiles after this have not actually been
                            // updated yet. So, we break.

                            // Don't worry about breaking out of the loop while
                            // there are still other projectiles that the
                            // current one is colliding with. Each other
                            // projectile will take care of the collision at its
                            // own update time. This also solves False positives
                            // with projectiles that would actually be safely
                            // out of the way once updated.

                            // @Todo: Actually, is that proper? If projectile 2
                            // got out of the way, and it would have collided,
                            // then doesn't that mean that it potentially
                            // crossed paths with projectile 1? I think I need
                            // to do some collision more complicated than just
                            // rectangle intersection.
                            break;
                        }
                        if (proj2->location == proj1->location &&
                            !proj2->is_invalid()) {
                            if (proj1->hurts_player == proj2->hurts_enemies) {
                                // they aren't projectiles from the same "team",
                                // so it's not
                                // friendly fire
                                auto proj2_aabb = proj2->hitbox + proj2->pos;
                                if (proj_aabb.intersects(proj2_aabb)) {
                                    do_hit_two_projectiles(proj1, proj2);
                                }
                            }
                        }
                    }
                }
                it.pos += it.vel * dt;
            }
        }
    }
    ng_for(enemies) {
        if (it.location == camera->location &&
            it.alive()) { // Okay, important @Todo here: I know that I only
                          // want to Render() things in the same Room as the
                          // Camera, but I don't know whether I want to
                          // update() things in the same Room as the Camera
                          // or as the Player. I'm leaning towards Camera.
                          // 2017-04-30
            it.update(dt, world->get_room(it.location));
        }
    }
    ng_for(npcs) {
        if (it.location == camera->location) {
            it.update(dt, world->get_room(it.location));
        }
    }
    ng_for(pickups) {
        if (it.location == player->location) {
            auto data = &pickup_data[cast(u8) it.type];
            auto radius = data->pickup_radius;
            auto radius_sq = radius * radius;
            if ((it.pos - player->pos).magsq() < radius_sq) {
                auto play_pickup_sound = [&data] { data->sound.play(); };
                switch (it.type) {
                default: { play_pickup_sound(); } break;
                case (Pickup_Type::HEART): {
                    player->health += it.info.heal_amount;
                    if (player->health > player->max_health) {
                        player->health = player->max_health;
                    }
                    play_pickup_sound();
                    it.type = Pickup_Type::UNINITIALIZED;
                } break;
                }
            }
        }
    }
    for (int i = 0; i < enemies.count; i += 1) {
        if (!enemies[i].alive()) {
            enemies.remove(i);
            i -= 1;
        }
    }
    for (int i = 0; i < pickups.count; i += 1) {
        if (pickups[i].type == Pickup_Type::UNINITIALIZED) {
            pickups.remove(i);
            i -= 1;
        }
    }
    for (int i = 0; i < projectiles.count; i += 1) {
        auto &it = projectiles[i];
        if (it.is_invalid() ||
            !(it.hitbox + it.pos)
                 .intersects(Rect{
                     0._g, 0._g, tile_to_game(world->get_room(it.location)->w),
                     tile_to_game(world->get_room(it.location)->h)})) {
            projectiles.remove(i);
            i -= 1;
        }
    }
}
void Entity_Manager::render_below_player() { // @Todo: take Camera as parameter.
    ng_for(npcs) {
        if (it.location == camera->location) {
            it.Render();
        }
    }
    ng_for(enemies) {
        if (it.location == camera->location && it.alive()) {
            it.Render();
        }
    }
}
void Entity_Manager::render_above_player() {
    ng_for(pickups) {
        if (it.location == camera->location) {
            auto sprite = &pickup_data[cast(u8) it.type].sprite;
            auto final_pos = camera->game_to_local(it.pos);
            ::Render(renderer, sprite,
                     /*game_to_px*/ (final_pos.x),
                     /*game_to_px*/ (final_pos.y));
        }
    }
    ng_for(projectiles) {
        if (it.location == camera->location) {
            auto final_pos = camera->game_to_local(it.pos);
            final_pos -=
                {px_to_game(it.texture.w) / 2, px_to_game(it.texture.h) / 2};
            final_pos = {cast(f32) /*game_to_px*/ (final_pos.x),
                         cast(f32) /*game_to_px*/ (final_pos.y)};
            ::Render(renderer, &it.texture, final_pos, Flip_Mode::NONE);
        }
    }
}
const int NPC_STAND = 0;
const f32 NPC_STAND_TIME = 1.0_s;
const int NPC_WALK = 1;
const f32 NPC_WALK_TIME = 1.0_s;
void Npc::update(f32 dt, Room *room) {
    ng_assert(room);
    const auto gravity = tile_to_game(48_t);
    vel.y += gravity * dt;
    if (current_action_mode == -1) {
        current_action_mode = NPC_STAND;
        action_timer.expiration = NPC_STAND_TIME * game_rng.get(0.9f, 1.5f);
    }
    if (action_timer.run(dt)) {
        if (current_action_mode == NPC_STAND) {
            current_action_mode = NPC_WALK;
            action_timer.expiration = NPC_WALK_TIME * game_rng.get(0.9f, 1.5f);
        } else if (current_action_mode == NPC_WALK) {
            current_action_mode = NPC_STAND;
            action_timer.expiration = NPC_STAND_TIME * game_rng.get(0.9f, 1.5f);
            facing_left = !facing_left;
        } else {
            ng_assert(False);
        }
    }
    if (current_action_mode == NPC_STAND) {
        vel.x = 0;
    } else if (current_action_mode == NPC_WALK) {
        const auto walk_accel = walk_speed * 8;
        auto dvx = walk_accel * dt;
        if (facing_left) {
            dvx = -dvx;
        }
        vel.x += dvx;
        auto mag = ng::abs(vel.x);
        if (mag >= walk_speed) {
            vel.x *= walk_speed / mag;
        }
    } else {
        ng_assert(False);
    }
    if (dt > 0._s) {
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
    }
}
void Npc::Render() {
    auto final_pos = camera->game_to_local(pos);
    sprite.flip = (facing_left ? Flip_Mode::HORIZONTAL : Flip_Mode::NONE);
    ::Render(renderer, &sprite,
             /*game_to_px*/ (final_pos.x),
             /*game_to_px*/ (final_pos.y));
}

namespace ng {
void print_item(ng::print_buffer *buf, const Room &r) {
    sprint(buf, "{room id=%0}", r.id);
}
} // namespace ng

Room room_create() {
    Room result = {};
    return result;
}
Room room_copy(Room *room) {
    Room result = {};
    result = *room; // basic structure copy
    // deep copies
    if (result.filename.ptr) {
        result.filename = copy_string(room->filename, world_allocator);
    }
    result.layers = array_deep_copy(room->layers, +[](Room_Layer *l) {
        Room_Layer result = *l;
        result.tiles = l->tiles.copy();
        return result;
    });
    result.collision = room->collision.copy();
    result.exits = room->exits.copy();
    result.enemies = room->enemies.copy();
    return result;
}
void room_destroy(Room *room) {
    if (room->music_info.filename.ptr) {
        free_string(&room->music_info.filename, world_allocator);
    }
    if (room->filename.ptr) {
        free_string(&room->filename, world_allocator);
    }
    room->layers.release();
    room->collision.release();
    room->exits.release();
    room->enemies.release();
}

World world_create() {
    World result = {};
    return result;
}
World world_copy(World *world) {
    World result = {};
    result.rooms.allocate(world->rooms.capacity);
    ng_for(world->rooms) {
        auto copy = room_copy(&it.value);
        result.rooms.insert(copy.id, copy);
    }
    return result;
}
void world_destroy(World *world) {
    ng_for(world->rooms) { room_destroy(&it.value); }
}
