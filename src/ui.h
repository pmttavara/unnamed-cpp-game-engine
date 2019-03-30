#pragma once
#include "audio.h"
#include "renderer.h"
struct Ui;
/* singleton */ struct TextBox {
  float open_amount = 0.f;
  ng::map<u16, Texture> text_strings = {video_allocator};
  Texture *current_text_string = null;
  Texture *queued_text_string = null;
  f32 open_close_time = 0.1_s;
  enum struct Mode : u8 { Open, Closed, Opening, Closing } mode = Mode::Closed;
  Ui *ui = null;
  void AddTextString(ng::string str, u16 id);
  void ChangeToTextString(u16 id);
  void Open();
  void Close();
  Bool isOpen();
  Bool isOpening();
  Bool isClosed();
  Bool isClosing();
  void update(f32 dt);
};
enum struct Fade_Type;
enum struct Fade_Color { BLACK, WHITE };
/* singleton */ struct Ui {
    TTF_Font *font = null;
    ng::array<Texture> elements = {video_allocator};
    Sprite save_spot_sprite =
        {}; // @Todo @Cleanup @Refactor: Move this into the World structure
    Timer button_fader = {0.0_s, 1.0_s}; // Used for all button prompts
    Bool should_render_textbox = False;
    Bool should_render_prompt_interaction = False;
    Bool using_controller = False;
    Bool interaction_prompt = False; // \brief Cleared each update
    Bool should_render_save_icon = False;
    TextBox text_box = {};
    // fader
    Fade_Type current_fade_type = Fade_Type::NONE;
    Fade_Color current_fade_color = Fade_Color::BLACK;
    Timer fade_timer = {};
    void start_fade(Fade_Type type, Fade_Color color, f32 fade_time);
    void update_fader(f32 dt);
    void render_fader();
    //
    void init_textures();
    void init_sounds();
    Texture string_to_texture(ng::string str);
    void HandleInput(struct Input_State *input,
                     struct Engine_State *engine_state);
    void update(f32 dt, Engine_State *engine_state);
    void Render(struct Engine_State *engine_state);
    enum struct Menu { Main, Pause, Settings } active_menu = Menu::Main;
    Menu last_menu = Menu::Main;
    int current_menu_item = 0;
    Timer menu_fader = {0.0_s, 1.0_s};
    Bool really_menu = False; // for when paused and you press menu
    Bool really_quit = False; // for when paused and you press quit
    Bool really_delete =
        False; // for when in settings and you press delete save
    Bool savegame_exists =
        False; // @Todo: turn into function that checks engine state?
    Sound nav_bloop = {};
    Sound action_bloop = {};
    //
    Bool in_volume_widget = False;
    float global_volume_adjuster = 0.f;
    float volume_ratio_adjuster = 0.f;
    Sound volume_adjust_click = {};
    Bool play_widget_music = False;
    //
};
void ui_destroy(Ui *ui);
extern Ui *ui;

void ui_render_animated_tiles(Ui *ui, struct Room *room);

void ui_apply_room_fade(Ui *ui, struct Room *room);
