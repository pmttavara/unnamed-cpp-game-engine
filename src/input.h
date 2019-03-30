#pragma once
#include "general.h"
#define KEY(x) SDL_SCANCODE_##x // @Temporary @Cleanup
struct Input_State {
    const u8 *keys = null;
    Bool keys_new[SDL_NUM_SCANCODES] = {};
    Bool keys_rep[SDL_NUM_SCANCODES] = {}; // Key repeat, mostly for editor
    // Bool key_left = False, key_left_new = False;
    // Bool key_right = False, key_right_new = False;
    // Bool key_up = False, key_up_new = False;
    // Bool key_down = False, key_down_new = False;
    // Bool key_z = False, key_z_new = False;
    // Bool key_x = False, key_x_new = False;
    // Bool key_a = False, key_a_new = False;
    // Bool key_s = False, key_s_new = False;
    // Bool key_escape = False, key_escape_new = False;
    // Bool key_enter = False, key_enter_new = False;

    // @Todo: Gamepad support for the game.

    // Bool has_pad = False;
    //
    // float pad_horizontal;
    // float pad_vertical;
    // Bool pad_a, pad_a_new;
};
Vector2 input_get_analog_vector(Input_State *in);
void input_update_from_sdl_keyboard_event(Input_State *in,
                                          SDL_KeyboardEvent *event);
// void input_update_from_sdl_keystate(Input_State *in, u8 *keystate);