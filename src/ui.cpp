#include "ui.h"
#include "audio.h"
#include "camera.h"
#include "engine_state.h"
#include "filesystem.h"
#include "input.h"
#include "player.h"
#include "world.h"
using ng::operator""_s;
void TextBox::AddTextString(ng::string str, u16 id) {
    ng_assert(text_strings.count < 0xFFFF);
    text_strings.insert(id, ui->string_to_texture(str));
}
void TextBox::ChangeToTextString(u16 id) {
    Texture *text = null;
    if (id != 0xFFFF) {
        auto found = text_strings[id];
        ng_assert(found.found, "Text with id %0 not found. # strings = ", id,
                  text_strings.count);
        if (found.found) {
            text = found.value;
        }
    }
    queued_text_string = text;
    if (isClosed()) {
        current_text_string = text;
    } else {
        Close();
    }
}
void TextBox::Open() {
    if (!isOpen()) {
        mode = Mode::Opening;
    }
}
void TextBox::Close() {
    if (!isClosed()) {
        mode = Mode::Closing;
    }
}
Bool TextBox::isOpen() { return mode == Mode::Open; }
Bool TextBox::isOpening() { return mode == Mode::Opening; }
Bool TextBox::isClosed() { return mode == Mode::Closed; }
Bool TextBox::isClosing() { return mode == Mode::Closing; }
void TextBox::update(f32 dt) {
    switch (mode) {
    default: {
    } break;
    case Mode::Closed: {
        current_text_string = queued_text_string;
        if (current_text_string) {
            Open();
        }
    } break;
    case Mode::Opening: {
        open_amount += dt / open_close_time;
    } break;
    case Mode::Closing: {
        open_amount -= dt / open_close_time;
    } break;
    }
    if (open_amount > 1.f) {
        open_amount = 1.f;
        mode = Mode::Open;
    }
    if (open_amount < 0.f) {
        open_amount = 0.f;
        mode = Mode::Closed;
    }
}
SDL_Color black_color = {0, 2, 33, 0};
SDL_Color white_color = {255, 253, 247, 0};
void Ui::start_fade(Fade_Type type, Fade_Color color, f32 fade_time) {
    current_fade_type = type;
    current_fade_color = color;
    fade_timer.expiration = fade_time;
    fade_timer.elapsed = 0;
}
void Ui::update_fader(f32 dt) {
    fade_timer.elapsed += dt;  
}
static void fill_colour(SDL_Color color) {
    SDL_SetRenderDrawColor(renderer->c_renderer, color.r, color.g, color.b,
                           color.a);
    SDL_RenderFillRect(renderer->c_renderer, null);
    SDL_SetRenderDrawColor(renderer->c_renderer, 0, 0, 0, 255);
}
void Ui::render_fader() {
    if (current_fade_type != Fade_Type::NONE) {
        float new_alpha = (fade_timer.elapsed / fade_timer.expiration);
        if (current_fade_type == Fade_Type::IN) {
            new_alpha = 1.f - new_alpha;
        }
        new_alpha = ng::clamp(new_alpha, 0.f, 1.f);
        SDL_Color draw_color =
            (current_fade_color == Fade_Color::BLACK ? black_color
                                                     : white_color);
        draw_color.a = cast(u8)(new_alpha * 255.f);
        fill_colour(draw_color);
    }
}
Ui *ui = null;
enum {
    txt_loading_idx,
    txt_title_idx,
    txt_newgame_idx,
    txt_continue_idx,
    txt_settings_idx,
    txt_quit_idx,
    txt_mainmenu_idx,
    txt_delete_savefile_idx,
    txt_back_idx,
    txt_really_idx,
    //
    gear_idx,
    textbox_idx,
    y_but_idx,
    s_key_idx,
    arrow_down_idx,
    save_icon_idx,
    dot_idx,
    speaker_off_idx,
    volume_triangle_idx,
    //
    NUM_UI_ELEMENTS
};
void Ui::init_textures() {
    elements.resize(NUM_UI_ELEMENTS);
    {
        elements[gear_idx] = texture_load("data/sprites/ui/gear.png"_s);
        elements[textbox_idx] = texture_load("data/sprites/ui/text_box.png"_s);
        elements[y_but_idx] = texture_load("data/sprites/ui/controller_y.png"_s);
        elements[s_key_idx] = texture_load("data/sprites/ui/keyboard_s.png"_s);
        elements[arrow_down_idx] = texture_load("data/sprites/ui/arrow_down.png"_s);
        elements[save_icon_idx] = texture_load("data/sprites/ui/save_icon.png"_s);
        elements[dot_idx] = texture_load("data/sprites/ui/dot.png"_s);
        elements[speaker_off_idx] =
            texture_load("data/sprites/ui/speaker_off.png"_s);
        elements[volume_triangle_idx] =
            texture_load("data/sprites/ui/volume_triangle.png"_s);
        save_spot_sprite.animations.push(
            animation_create("data/sprites/ui/save_spot.png"_s, 16_px, 16_px, 10_fps));
    }
    {
        elements[txt_loading_idx] = string_to_texture("Loading"_s);
        elements[txt_title_idx] = string_to_texture(GAME_NAME ""_s);
        elements[txt_newgame_idx] = string_to_texture("New Game"_s);
        elements[txt_continue_idx] = string_to_texture("Continue"_s);
        elements[txt_settings_idx] = string_to_texture("Settings"_s);
        elements[txt_quit_idx] = string_to_texture("Quit"_s);
        elements[txt_mainmenu_idx] = string_to_texture("Main Menu"_s);
        elements[txt_delete_savefile_idx] =
            string_to_texture("Delete Save File"_s);
        elements[txt_back_idx] = string_to_texture("Back"_s);
        elements[txt_really_idx] = string_to_texture("Really?"_s);
    }
}
void Ui::init_sounds() {
    nav_bloop = sound_from_filename("data/sound/ui_tick.wav"_s);
    nav_bloop.update_volume(0.4f);
    action_bloop = sound_from_filename("data/sound/ui_action.wav"_s);
    action_bloop.update_volume(1.f);
    volume_adjust_click = sound_from_filename("data/sound/ui_tick.wav"_s);
    volume_adjust_click.retrigger = False;
    volume_adjust_click.update_volume(1.f);
}
const auto border = 4;
Texture Ui::string_to_texture(ng::string str) {
    ng_assert(font != null);
    auto r = renderer->c_renderer;
    ng_assert(r != null);
    auto tex = &ui->elements[textbox_idx];
    auto surf = TTF_RenderText_Blended_Wrapped(font, ng::c_str(str),
                                               SDL_Color{255, 245, 240, 255},
                                               tex->w - border * 2);
    defer { SDL_FreeSurface(surf); };
    ng_assert(surf);
    auto c_tex = SDL_CreateTextureFromSurface(r, surf);
    ng_assert(c_tex);
    return texture_from_sdl(c_tex);
}
SDL_Texture *string_to_sdl_texture(TTF_Font *font, ng::string str) {
    auto r = renderer->c_renderer;
    ng_assert(r != null);
    auto surf = TTF_RenderText_Blended(font, ng::c_str(str),
                                       SDL_Color{255, 245, 240, 255});
    ng_assert(surf);
    defer { SDL_FreeSurface(surf); };
    auto c_tex = SDL_CreateTextureFromSurface(r, surf);
    ng_assert(c_tex);
    return c_tex;
}
static void enter_volume_widget(Ui *ui) {
    ui->in_volume_widget = True;
    { // Play our own custom music if no other music is playing, or if it's
        // fading out. This helps the user see how they're changing the volume.
        ui->play_widget_music = ((audio->current_song < 0) ||
                                 (audio->music_fade_type == Fade_Type::OUT &&
                                  audio->queued_song < 0));
        if (ui->play_widget_music) {
            // audio->play_song(
            //   "data/music/TOP_MEMES.ogg"_s // @Todo: choose good sample song
            //   in volume menu , 0.25s, 0.25s, True);
        }
    }
}
static void leave_volume_widget(Ui *ui) {
    ui->in_volume_widget = False;
    if (ui->play_widget_music) { // We're done playing our custom music.
        ui->play_widget_music = False;
        audio->stop_music(0.25_s);
    }
}
static void clear_ui_state(Ui *ui) {
    leave_volume_widget(ui);
    ui->current_menu_item = 0;
    ui->really_menu = False;
    ui->really_quit = False;
    ui->really_delete = False;
}
static void ui_begin_game(Ui *ui, Engine_State *engine_state) {
    clear_ui_state(ui);
    {
        SDL_RenderClear(renderer->c_renderer);
        auto loading_texture = &ui->elements[txt_loading_idx];
        ::Render(renderer, loading_texture,
                 globals.screen_center -
                     Vector2{loading_texture->w / 2, loading_texture->h / 2});
        SDL_RenderPresent(renderer->c_renderer);
    }
    // ui->nav_bloop.Stop();
    // ui->action_bloop.Stop();
    void begin_game(Engine_State * engine_state);
    begin_game(engine_state);
    ui->start_fade(Fade_Type::IN, Fade_Color::BLACK, 0.5_s);
}
static void ui_pause_game(Ui *ui, Engine_State *engine_state) {
    clear_ui_state(ui);
    void pause_game(Engine_State * engine_state);
    pause_game(engine_state);
}
static void ui_unpause_game(Ui *ui, Engine_State *engine_state) {
    // ui->nav_bloop.Stop();
    // ui->action_bloop.Stop();
    ui->last_menu = Ui::Menu::Pause;
    ui->active_menu = Ui::Menu::Pause;
    clear_ui_state(ui);
    void unpause_game(Engine_State * engine_state);
    unpause_game(engine_state);
}
static void ui_enter_settings(Ui *ui, Engine_State *) {
    ui->last_menu = ui->active_menu;
    ui->active_menu = Ui::Menu::Settings;
    clear_ui_state(ui);
}
static void ui_leave_settings(Ui *ui, Engine_State *) {
    ui->active_menu = ui->last_menu;
    clear_ui_state(ui);
    ui->current_menu_item = 1;
}
static void ui_want_go_to_menu(Ui *ui, Engine_State *) {
    clear_ui_state(ui);
    ui->action_bloop.stop();
    ui->nav_bloop.play();
    ui->really_menu = True;
    ui->current_menu_item = 2; // same item
}
static void ui_go_to_menu(Ui *ui, Engine_State *engine_state) {
    ui->last_menu = Ui::Menu::Main;
    ui->active_menu = Ui::Menu::Main;
    clear_ui_state(ui);
    ui->start_fade(Fade_Type::NONE, Fade_Color::BLACK, 0._s);
    ui->current_menu_item = 0; // point player to quit
    void go_to_main_menu(Engine_State * engine_state);
    go_to_main_menu(engine_state);
}
static void ui_want_quit(Ui *ui, Engine_State *) {
    clear_ui_state(ui);
    ui->action_bloop.stop();
    ui->nav_bloop.play();
    ui->really_quit = True;
    ui->current_menu_item = 3; // same item
}
static void ui_quit(Ui *ui, Engine_State *engine_state) {
    ui->nav_bloop.stop();
    ui->action_bloop.stop();
    engine_state->should_quit = True;
};
static void ui_want_delete_save(Ui *ui, Engine_State *) {
    clear_ui_state(ui);
    ui->action_bloop.stop();
    ui->nav_bloop.play();
    ui->really_delete = True;
    ui->current_menu_item = 2; // same item
}
static void ui_delete_save(Ui *ui, Engine_State *engine_state) {
    clear_ui_state(ui);
    ui->current_menu_item = 2;
    { // @Incomplete
        RW rw(GAME_NAME ".sav"_s, "r+b"_s);
        savegame_delete(&rw);
        ui->savegame_exists = False;
    }
    if (engine_state->mode != Engine_Mode::MAIN_MENU) {
        ui_go_to_menu(ui, engine_state);
        ui->current_menu_item = 0; // point player to new game
    }
}
void Ui::HandleInput(Input_State *input, Engine_State *engine_state) {
    if (engine_state->mode == Engine_Mode::IN_ENGINE) {
        if (input->keys_new[KEY(S)]) {
            text_box.Close();
            text_box.ChangeToTextString(0xFFFF);
        }
        if (input->keys_new[KEY(RETURN)] || input->keys_new[KEY(ESCAPE)]) {
            ui_pause_game(this, engine_state);
        }
    } else if (engine_state->mode == Engine_Mode::MAIN_MENU ||
               engine_state->mode == Engine_Mode::PAUSED) {
        auto ui_nav_fx = [this] {
            nav_bloop.play();
            menu_fader.elapsed = 0;
        };
        auto ui_action_fx = [this] {
            nav_bloop.stop();
            action_bloop.play();
            menu_fader.elapsed = 0;
        };
        if (!in_volume_widget) { // handle up/down navigation
            int num_menu_items = 0;
            switch (active_menu) {
            case (Menu::Main): {
                num_menu_items = 3;
            } break;
            case (Menu::Pause): {
                num_menu_items = 4;
            } break;
            case (Menu::Settings): {
                num_menu_items = savegame_exists ? 4 : 3;
            } break;
            }
            auto old_item = current_menu_item;
            if (input->keys_new[KEY(DOWN)]) {
                current_menu_item += 1;
            } else if (input->keys_new[KEY(UP)]) {
                current_menu_item -= 1;
            }
            if (current_menu_item < 0) {
                current_menu_item = num_menu_items - 1;
            }
            if (current_menu_item >= num_menu_items) {
                current_menu_item = 0;
            }
            if (current_menu_item != old_item) {
                ui_nav_fx();
            }
        }
        { // call out to the proper function
            // @TODO: @Todo: @UI: @Ui:
            // @@@
            // @
            // @
            // @
            // @Todo: "New Game" should have a plus icon to its left, "Settings"
            // a gear, "Quit" a door, etc. This helps you get to the language
            // select menu without being able to read the current language.

            using Func = void(Ui *, Engine_State *);
            Func *functions[] = {null, null, null, null};
            switch (active_menu) {
            default: {
            } break;
            case (Menu::Main): {
                functions[0] = &ui_begin_game;
                functions[1] = &ui_enter_settings;
                functions[2] = &ui_quit;
            } break;
            case (Menu::Pause): {
                functions[0] = &ui_unpause_game;
                functions[1] = &ui_enter_settings;
                functions[2] =
                    really_menu ? &ui_go_to_menu : &ui_want_go_to_menu;
                functions[3] = really_quit ? &ui_quit : &ui_want_quit;
                if (input->keys_new[KEY(X)] || input->keys_new[KEY(ESCAPE)]) {
                    ui_unpause_game(this, engine_state);
                }
            } break;
            case (Menu::Settings): {
                functions[0] = [](Ui *ui, Engine_State *engine_state) {
                    ui->in_volume_widget ? leave_volume_widget(ui)
                                         : enter_volume_widget(ui);
                    USE engine_state;
                };
                functions[1] = null; // @Todo: language select
                if (savegame_exists) {
                    functions[2] =
                        really_delete ? &ui_delete_save : &ui_want_delete_save;
                }
                auto back_button_index = savegame_exists ? 3 : 2;
                functions[back_button_index] = &ui_leave_settings;
                if (input->keys_new[KEY(ESCAPE)]) {
                    if (last_menu == Menu::Pause) {
                        ui_unpause_game(this, engine_state);
                    }
                    if (last_menu == Menu::Main) {
                        ui_leave_settings(this, engine_state);
                    }
                } else if (input->keys_new[KEY(X)]) {
                    if (in_volume_widget) {
                        leave_volume_widget(this);
                        ui_action_fx();
                    } else {
                        ui_leave_settings(this, engine_state);
                        ui_nav_fx();
                    }
                }
                if (in_volume_widget) {
                    auto in_vec = input_get_analog_vector(input);
                    const auto movement_factor = 1;
                    global_volume_adjuster = in_vec.x * movement_factor;
                    volume_ratio_adjuster = in_vec.y * movement_factor;
                }
            } break;
            }
            if (input->keys_new[KEY(Z)] || input->keys_new[KEY(RETURN)]) {
                auto f = functions[current_menu_item];
                if (f) {
                    ui_nav_fx();
                    f(this, engine_state);
                }
            }
        }
    }
}
#pragma clang diagnostic ignored "-Wunused-parameter"
Vector2 get_text_box_pos(Texture *text_box, Camera *camera, Player *player) {
    ng::print("globals.screen_width = %0 ; text_box->w = %1\n", globals.screen_width, text_box->w);
    s32 x = globals.screen_width / 2 - text_box->w / 2;
    s32 y = 16_px;
    if (camera->game_to_local(player->pos).y < globals.screen_height / 2) {
        y = globals.screen_height - text_box->h - 16_px;
    }
    return {px_to_game(x), px_to_game(y)};
}
void Ui::update(f32 dt, Engine_State *engine_state) {
    if (menu_fader.expired()) {
        menu_fader.elapsed = 0;
    }
    if (engine_state->mode == Engine_Mode::IN_ENGINE) { // update text box
        button_fader.elapsed += dt;
        if (button_fader.expired()) {
            button_fader.elapsed = 0;
        }
    }
    should_render_textbox = False;
    should_render_prompt_interaction = False;
    if (!text_box.isClosed() &&
        (engine_state->mode == Engine_Mode::IN_ENGINE ||
         engine_state->mode == Engine_Mode::PAUSED ||
         engine_state->mode == Engine_Mode::TRANSITION_FREEZE)) {
        should_render_textbox = True;
    }
    if (interaction_prompt) {
        interaction_prompt = False;
        should_render_prompt_interaction = True;
    }
    if (engine_state->mode == Engine_Mode::IN_ENGINE) {
        save_spot_sprite.update(dt);
    }
    if (engine_state->mode == Engine_Mode::IN_ENGINE ||
        engine_state->mode == Engine_Mode::TRANSITION_FREEZE) {
        update_fader(dt);
    }
    if (engine_state->mode == Engine_Mode::IN_ENGINE) {
        active_menu = Menu::Pause;
        last_menu = Menu::Pause;
    }
    if (active_menu == Menu::Settings) {
        if (in_volume_widget) {
            auto g_vol = audio->global_volume;
            auto v_rat = audio->volume_ratio;
            { // modify
                g_vol += global_volume_adjuster * dt;
                if (g_vol > 0.f) {
                    v_rat += volume_ratio_adjuster / g_vol * dt;
#if 1
                    // "Linearize" the volume adjustment
                    if (g_vol < 1.f) {
                        v_rat -= global_volume_adjuster * (v_rat - 0.5f) /
                                 g_vol * dt;
                    }
#endif
                }
                global_volume_adjuster = 0.f;
                volume_ratio_adjuster = 0.f;
            }
            g_vol = ng::clamp(g_vol, 0.f, 1.f);
            v_rat = ng::clamp(v_rat, 0.f, 1.f);
            { // apply changes
                if (g_vol != audio->global_volume ||
                    v_rat != audio->volume_ratio) {
                    volume_adjust_click.play();
                }
                audio->global_volume = g_vol;
                audio->volume_ratio = v_rat;
            }
        }
    }
}
void Ui::Render(Engine_State *engine_state) {
    auto get_alpha_value = [](Timer *fader) {
        auto t = fader->elapsed;
        auto t_f = fader->expiration;
        auto x = t / t_f;
        auto y = ng::cos(x * ng::TAU32);
        y = y * 0.5f + 0.5f; // shift cos result to [0, 1]
        y *= 3.f / 4.f;
        y += 1.f / 4.f;
        return y;
    };
    Bool should_render_any_flashing_prompt =
        should_render_prompt_interaction || should_render_textbox;
    if (!should_render_any_flashing_prompt) {
        button_fader.elapsed = 0; // stays fresh until needed
    } else {
        float button_prompt_alpha = 0.f;
        button_prompt_alpha = get_alpha_value(&button_fader);
        if (should_render_textbox) {
            auto tex = &elements[textbox_idx];
            auto text_box_pos = get_text_box_pos(tex, camera, player);
            text_box_pos /= renderer->c_scale;
            { // Render text box
                // auto old_scale = renderer->c_scale;
                // renderer->c_scale = 1.0f;
                // defer { renderer->c_scale = old_scale; }; // @Hack
                auto amt = text_box.open_amount;
                Rect src = {};
                {
                    src.x = 0._g;
                    src.y = px_to_game(tex->h) / 2 * (1.f - amt);
                    src.w = px_to_game(tex->w);
                    src.h = px_to_game(tex->h) * amt;
                }
                Rect dest = {text_box_pos.x + src.x, text_box_pos.y + src.y,
                             src.w, src.h};
                ::Render(renderer, tex, src, dest, Flip_Mode::NONE);
                // Render text
                auto string_tex = text_box.current_text_string;
                if (string_tex) {
                    string_tex->alpha = 0.5f + (amt * 0.5f);
                    auto raw = string_tex->tex;
                    auto render_pos =
                        Vector2{text_box_pos.x + px_to_game(border),
                                text_box_pos.y + px_to_game(border)};
                    SDL_SetTextureColorMod(raw, 16, 16, 64);
                    for (auto y = -1_px; y <= 1_px; y += 1)
                        for (auto x = -1_px; x <= 1_px; x += 1)
                            if (y == x || y == -x) {
                                continue;
                            } else {
                                ::Render(renderer, string_tex,
                                         render_pos + Vector2{px_to_game(x),
                                                              px_to_game(y)});
                            }
                    SDL_SetTextureColorMod(raw, 255, 255, 255);
                    ::Render(renderer, string_tex, render_pos);
                }
            }
            auto x = text_box_pos.x + 224 - 16 - 2;
            auto y = text_box_pos.y + 64 - 16 - 2;
            Vector2 p{x, y};
            auto button =
                &elements[ui->using_controller ? y_but_idx : s_key_idx];
            button->alpha = button_prompt_alpha;
            ::Render(renderer, button, p, Flip_Mode::NONE);
        }
        if (should_render_prompt_interaction) {
            auto save_icon = &elements[save_icon_idx];
            auto button = &elements[arrow_down_idx];

            auto button_pos = camera->game_to_local(player->pos);
            button_pos.x -= 8;
            button_pos.y -= 24;
            button_pos.x += player->mstate.FacingRight() ? 1._g : 0._g;
            if (should_render_save_icon) {
                button_pos.x -= 7;
            }
            auto save_icon_pos = button_pos;
            save_icon_pos.x += 16;
            save_icon_pos.y += 1;
            if (!should_render_save_icon) {
                button_pos.x = ng::clamp(button_pos.x, 0._g,
                                         px_to_game(globals.screen_width - button->w));
                button_pos.y = ng::clamp(button_pos.y, 0._g,
                                         px_to_game(globals.screen_height - button->h));
            } else {
                auto icons_left = ng::min(button_pos.x, save_icon_pos.x);
                auto icons_right = ng::max(button_pos.x, save_icon_pos.x);
                auto icons_top = ng::min(button_pos.y, save_icon_pos.y);
                auto icons_bottom = ng::max(button_pos.y, save_icon_pos.y);
                {
                    if (icons_left < 0._g) {
                        f32 diff = 0._g - icons_left;
                        button_pos.x += diff;
                        save_icon_pos.x += diff;
                    }
                }
                {
                    auto edge = px_to_game(globals.screen_width - save_icon->w);
                    if (icons_right > edge) {
                        f32 diff = icons_right - edge;
                        button_pos.x -= diff;
                        save_icon_pos.x -= diff;
                    }
                }
                {
                    if (icons_top < 0._g) {
                        f32 diff = 0._g - icons_top;
                        button_pos.y += diff;
                        save_icon_pos.y += diff;
                    }
                }
                {
                    auto edge = px_to_game(globals.screen_height - save_icon->h);
                    if (icons_bottom > edge) {
                        f32 diff = icons_bottom - edge;
                        button_pos.y -= diff;
                        save_icon_pos.y -= diff;
                    }
                }
            }
            save_icon->alpha =
                button_prompt_alpha * (should_render_save_icon ? 1.f : 0.f);
            ::Render(renderer, save_icon, save_icon_pos, Flip_Mode::NONE);
            button->alpha = button_prompt_alpha;
            ::Render(renderer, button, button_pos, Flip_Mode::NONE);
        }
    }
    render_fader();
    if (engine_state->mode == Engine_Mode::PAUSED) {
        auto dimmer = black_color;
        dimmer.a = 127;
        fill_colour(dimmer);
    }
    if (engine_state->mode == Engine_Mode::MAIN_MENU ||
        engine_state->mode == Engine_Mode::PAUSED) {
        auto menu_text_top_position = globals.screen_center;
        menu_text_top_position.y *= 1.125f;
        auto render_menu_text = [&](Texture *text_texture, int item_index,
                                    Vector2 pos = {0, 0}) {
            Vector2 sep = {0, 10};
            auto default_pos =
                menu_text_top_position + sep * cast(float) item_index;
            auto texture_raw = text_texture->tex;
            auto render_pos = default_pos;
            if (pos.x != 0) {
                render_pos.x = pos.x;
            }
            if (pos.y != 0) {
                render_pos.y = pos.y;
            }
            render_pos -= Vector2{text_texture->w / 2, text_texture->h / 2};
            auto selected = current_menu_item == item_index;
            if (selected) {
                text_texture->alpha = get_alpha_value(&menu_fader)
                    // * 0.6f + 0.4f //
                    ;
            } else {
                text_texture->alpha = 1.f;
            }
            SDL_SetTextureColorMod(texture_raw, 16, 16, 64);
            for (auto y = -1_px; y <= 1_px; y += 1)
                for (auto x = -1_px; x <= 1_px; x += 1)
                    if (y == x || y == -x) {
                        continue;
                    } else {
                        ::Render(renderer, text_texture,
                                 render_pos +
                                     Vector2{px_to_game(x), px_to_game(y)});
                    }
            u8 r = selected || item_index == -1 ? 255 : 180;
            u8 g = selected || item_index == -1 ? 255 : 180;
            u8 b = selected || item_index == -1 ? 224 : 200;
            SDL_SetTextureColorMod(texture_raw, r, g, b);
            ::Render(renderer, text_texture, render_pos);
        };
        switch (active_menu) {
        default: {
        } break;
        case (Menu::Main): {
            render_menu_text(&elements[txt_title_idx], -1, globals.screen_center);
            render_menu_text(
                &elements[savegame_exists ? txt_continue_idx : txt_newgame_idx],
                0);
            render_menu_text(&elements[txt_settings_idx], 1);
            render_menu_text(&elements[txt_quit_idx], 2);
        } break;
        case (Menu::Pause): {
            render_menu_text(&elements[txt_continue_idx], 0);
            render_menu_text(&elements[txt_settings_idx], 1);
            render_menu_text(

                &elements[really_menu ? txt_really_idx : txt_mainmenu_idx], 2);
            render_menu_text(
                &elements[really_quit ? txt_really_idx : txt_quit_idx], 3);
        } break;
        case (Menu::Settings): {
            {
                const auto widget_size = px_to_game(24_px);
                const Vector2 volume_widget_center =
                    globals.screen_center +
                    Vector2{px_to_game(0_px), px_to_game(-10_px)};
                const Vector2 vol_min = {volume_widget_center.x - widget_size,
                                         volume_widget_center.y};
                const Vector2 vol_max_music = {
                    volume_widget_center.x + widget_size,
                    volume_widget_center.y - widget_size};
                const Vector2 vol_max_sound = {
                    volume_widget_center.x + widget_size,
                    volume_widget_center.y + widget_size};
                //
                // Vector2 vecs[4] = {vol_min, vol_max_music, vol_max_sound,
                // vol_min}; SDL_Point pts[4]; ng_for(pts) *it =
                // SDL_Point{cast(int) game_to_px(vecs[it_index].x),
                //                          cast(int)
                //                          game_to_px(vecs[it_index].y)};
                // SDL_SetRenderDrawColor(renderer->c_renderer, 255, 255, 255,
                // 255); SDL_RenderDrawLines(renderer->c_renderer, pts, 4);
                {
                    auto volume_triangle = &elements[volume_triangle_idx];
                    render_menu_text(volume_triangle, in_volume_widget ? -1 : 0,
                                     volume_widget_center);
                }
                auto volume_dot_pos = ng::lerp(
                    vol_min,
                    ng::lerp(vol_max_music, vol_max_sound, audio->volume_ratio),
                    audio->global_volume);
                render_menu_text(&elements[dot_idx], in_volume_widget ? 0 : 0,
                                 volume_dot_pos);

                {
                    auto speaker_off = &elements[speaker_off_idx];
                    render_menu_text(speaker_off, in_volume_widget ? -1 : 0,
                                     vol_min - Vector2{12, 0});
                }
            }
            render_menu_text(&elements[arrow_down_idx],
                             1); // @Incomplete: placeholder button
            if (savegame_exists) {
                render_menu_text(
                    &elements[really_delete ? txt_really_idx
                                            : txt_delete_savefile_idx],
                    2);
            }
            auto back_button_index = savegame_exists ? 3 : 2;
            render_menu_text(&elements[txt_back_idx], back_button_index);
        } break;
        }
    }
}
void ui_destroy(Ui *ui) {
    ui->elements.release();
    sprite_destroy(&ui->save_spot_sprite);
    ui->text_box.text_strings.release();
}
void ui_render_animated_tiles(Ui *ui, Room *room) {
    auto collision = &room->collision;
    ng_for(*collision) {
        if (it.type == Collision_Tile_Type::SAVE_SPOT) {
            usize i = &it - collision->begin();
            auto x = cast(s16)(i % room->w);
            auto y = cast(s16)(i / room->w);
            auto final_pos = camera->game_to_local(
                Vector2{tile_to_game(x), tile_to_game(y)});
            ::Render(renderer, &ui->save_spot_sprite,
                /*game_to_px*/(final_pos.x),
                /*game_to_px*/(final_pos.y));
        }
    }
}
void ui_apply_room_fade(Ui *ui, Room *room) {
    Fade_Color fade_color = {};
    if (room->is_indoors) {
        fade_color = Fade_Color::BLACK;
    } else {
        fade_color = Fade_Color::WHITE;
    }
    ui->start_fade(Fade_Type::OUT, fade_color, world->room_fade_out_length);
}
