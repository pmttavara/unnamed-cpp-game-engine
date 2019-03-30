#pragma once
#include "general.h"
struct Camera_Target {
    Vector2 *pos = null;
    u16 *location = null;
};
/* singleton */ struct Camera {
    Camera_Target target = {};
    Vector2 pos = {0, 0};
    u16 location = 0xFFFF;
    Bool smooth = False;
    Vector2 half_life = {1._s, 1._s};
    Rect world_rect();
    void update(f64 dt, Bool constrain_to_room = True);
    Vector2 pixel_pos();
    Vector2 game_to_local(Vector2 pos);
};
extern Camera *camera;
Bool is_targeting(Camera *camera, Vector2 *pos, u16 *location);
