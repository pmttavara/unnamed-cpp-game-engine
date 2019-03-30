#include "camera.h"
#include "world.h"
Camera *camera = null;
Rect Camera::world_rect() {
    auto s = globals.screen_center;
    Rect result = {pos.x - s.x, pos.y - s.y, s.x * 2, s.y * 2};
    return result;
}
void Camera::update(f64 dt, Bool constrain_to_room) {
    if (target.pos) {
        if (smooth) {
            using namespace ng;
            // auto t = Vector2{dt / half_life.x, dt / half_life.y};
            auto t_x = dt / half_life.x;
            auto t_y = dt / half_life.y;
            auto new_pos_x = lerp(pos.x, target.pos->x, 1 - ng::pow(2, -t_x));
            auto new_pos_y = lerp(pos.y, target.pos->y, 1 - ng::pow(2, -t_y));
            // if (abs(new_pos_x - pos.x) > 0.1_g) { // reduce micro-movement
            //     // @Todo @Robustness: make this framerate-independent
                 pos.x = new_pos_x;
            // }
            // if (abs(new_pos_y - pos.y) > 0.1_g) {
                 pos.y = new_pos_y;
            // }
        } else {
            pos = *target.pos;
        }
    }
    if (target.location) {
        location = *target.location;
    }
    if (constrain_to_room) {
        auto room = world->get_room(location);
        if (room) {
            if (tile_to_px(room->w) >=
                globals.screen_width) {
                pos.x = ng::clamp(
                    pos.x, globals.screen_center.x,
                    (tile_to_game(room->w) - globals.screen_center.x));
            } else {
                pos.x = tile_to_game(room->w) / 2._g;
            }
            if (tile_to_px(room->h) >=
                globals.screen_height) {
                pos.y = ng::clamp(
                    pos.y, globals.screen_center.y,
                    (tile_to_game(room->h) - globals.screen_center.y));
            } else {
                pos.y = tile_to_game(room->h) / 2._g;
            }
        }
    }
}
Vector2 Camera::pixel_pos() { // returns pixel-perfect position
    auto campos = pos;
    campos.x = px_to_game(game_to_px(campos.x));
    campos.y = px_to_game(game_to_px(campos.y));
    return campos;
}
Vector2 Camera::game_to_local(Vector2 pos) {
    return pos - this->pos + globals.screen_center;
}
Bool is_targeting(Camera *camera, Vector2 *pos, u16 *location) {
    return camera->target.pos == pos && camera->target.location == location;
}
