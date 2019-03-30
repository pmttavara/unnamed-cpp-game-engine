#include "world.h"
struct Collision_Resolution_Data {
    s16 tile_offset = 0_t;
    f32 offset_from_collider = 0._g;
};
void collide_directional(f32 *pos_coord, f32 *delta_coord,
                         Collision_Resolution_Data res_data,
                         Room_Collision_Test_Result *collision_test_result,
                         Rect aabb, Room *room, f32 delta,
                         Bool is_a_horizontal_check,
                         Bool checking_direction_is_negative);
Collide_Result collide(Collide_State input_state, Room *room) {
    Collide_Result result = {};
    result.new_state = input_state;
    auto state = &result.new_state;
    f32 delta_x = state->delta.x;
    f32 delta_y = state->delta.y;
    auto c_bottom = [&](f32 delta) {
        auto pos_coord = &state->pos.y;
        auto delta_coord = &state->delta.y;
        Rect aabb{state->collider_bottom.x + state->pos.x,
                  state->collider_bottom.y + state->pos.y,
                  state->collider_bottom.w, state->collider_bottom.h + delta};
        Collision_Resolution_Data res_data = {};
        res_data.tile_offset = 0_t;
        res_data.offset_from_collider = state->collider_bottom.b();
        collide_directional(pos_coord, delta_coord, res_data, &result.bottom,
                            aabb, room, delta, False, False);
    };
    auto c_top = [&](f32 delta) {
        auto pos_coord = &state->pos.y;
        auto delta_coord = &state->delta.y;
        Rect aabb{state->collider_top.x + state->pos.x,
                  state->collider_top.y + state->pos.y + delta,
                  state->collider_top.w, state->collider_top.h - delta};
        Collision_Resolution_Data res_data = {};
        res_data.tile_offset = 1_t;
        res_data.offset_from_collider = state->collider_top.t();
        collide_directional(pos_coord, delta_coord, res_data, &result.top, aabb,
                            room, delta, False, True);
    };
    auto c_right = [&](f32 delta) {
        auto pos_coord = &state->pos.x;
        auto delta_coord = &state->delta.x;
        Rect aabb{state->collider_right.x + state->pos.x,
                  state->collider_right.y + state->pos.y,
                  state->collider_right.w + delta, state->collider_right.h};
        Collision_Resolution_Data res_data = {};
        res_data.tile_offset = 0_t;
        res_data.offset_from_collider = state->collider_right.r();
        collide_directional(pos_coord, delta_coord, res_data, &result.right,
                            aabb, room, delta, True, False);
    };
    auto c_left = [&](f32 delta) {
        auto pos_coord = &state->pos.x;
        auto delta_coord = &state->delta.x;
        Rect aabb{state->collider_left.x + state->pos.x + delta,
                  state->collider_left.y + state->pos.y,
                  state->collider_left.w - delta, state->collider_left.h};
        Collision_Resolution_Data res_data = {};
        res_data.tile_offset = 1_t;
        res_data.offset_from_collider = state->collider_left.l();
        collide_directional(pos_coord, delta_coord, res_data, &result.left,
                            aabb, room, delta, True, True);
    };
    if (delta_y > 0) {
        c_bottom(delta_y);
        c_top(0._g);
    } else {
        c_top(delta_y);
        c_bottom(0._g);
    }
    if (delta_x > 0) {
        c_right(delta_x);
        c_left(0._g);
    } else {
        c_left(delta_x);
        c_right(0._g);
    }
    return result;
}
Bool check_collided(Collision_Tile ct) {
    return ct.type == Collision_Tile_Type::SOLID ||
           ct.type == Collision_Tile_Type::BREAKABLE ||
           ct.type == Collision_Tile_Type::BREAKABLE_HEART ||
           (ct.exit_number() >= 0 && ct.exit_number() < 4);
}
void collide_directional(f32 *pos_coord, f32 *delta_coord,
                         Collision_Resolution_Data res_data,
                         Room_Collision_Test_Result *collision_test_result,
                         Rect aabb, Room *room, f32 delta,
                         Bool is_a_horizontal_check,
                         Bool checking_direction_is_negative) {
    auto collisions = room->get_colliding_tiles(aabb);
    defer { collisions.release(); };
    Room_Collision_Info first_ci = {};
    Bool collided = False;
    ng_for(collisions) {
        if (check_collided(it.tile)) {
            collided = True;
            first_ci = it;
            break;
        }
    }
    ng_for(collisions) {
        auto collision = it;
        if (collision.tile.is_exit() &&
            cast(Room_Exit_Mode)
                    room->get_exit_info(collision.tile.exit_number())
                        .mode == Room_Exit_Mode::ON_TOUCH) {
            collision_test_result->did_hit_ontouch_exit = True;
            collision_test_result->first_ontouch_exit = collision;
            break;
        }
    }
    collision_test_result->did_collide = collided;
    if (collided) {
        Bool should_zero_delta =
            (checking_direction_is_negative ? (*delta_coord < 0._g)
                                            : (*delta_coord > 0._g));
        if (should_zero_delta) {
            *delta_coord = 0._g;
        }
        *pos_coord =
            tile_to_game((is_a_horizontal_check ? first_ci.x : first_ci.y) +
                         res_data.tile_offset) -
            res_data.offset_from_collider;
    } else {
        *pos_coord += delta;
    }
}
