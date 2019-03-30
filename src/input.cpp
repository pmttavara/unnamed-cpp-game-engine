#include "input.h"
Vector2 input_get_analog_vector(Input_State *in) {
    Vector2 result = {};
    result.x = in->keys[SDL_SCANCODE_RIGHT] - in->keys[SDL_SCANCODE_LEFT];
    result.y = in->keys[SDL_SCANCODE_DOWN] - in->keys[SDL_SCANCODE_UP];
    return result.hat();
}

void input_update_from_sdl_keyboard_event(Input_State *in,
                                          SDL_KeyboardEvent *event) {
    if (!event->repeat) {
        in->keys_new[event->keysym.scancode] = (event->state == SDL_PRESSED);
    }
    in->keys_rep[event->keysym.scancode] = (event->state == SDL_PRESSED);
}
