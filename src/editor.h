#pragma once
#include "general.h"
#include "world.h"

struct Input_State;
extern ng::allocator *editor_allocator;

enum struct Horizontal_Align { LEFT = 0, CENTER = 1, RIGHT = 2 };
void debug_draw_text(ng::string str, Vector2 pos,
                     Horizontal_Align align = Horizontal_Align::LEFT);

struct Tile_Cursor {
    s16 x = 0_t;
    s16 y = 0_t;
};

struct Gtile_Cursor {
    Tile_Cursor where;
    int layer = 0;
};

struct Ctile_Cursor {
    Tile_Cursor where;
    // @Stub
};

struct Tileset_Cursor {
    int x = 0;
    int y = 0;
};

Graphical_Tile *gtile_cursor_to_gtile_ptr(Gtile_Cursor cursor, Room *room);
u8 tileset_cursor_to_gtile_index(Tileset_Cursor cursor, TextureAtlas *tileset);
Tileset_Cursor gtile_index_to_tileset_cursor(u8 index, TextureAtlas *tileset);
Collision_Tile *ctile_cursor_to_ctile_ptr(Ctile_Cursor cursor, Room *room);

enum struct Edit_Type {
    INVALID,

    GTILE_EDIT,
    CTILE_EDIT,
    ETILE_EDIT,
    AUTO_MODE_EDIT,
    ROOM_ID_EDIT,

    NUM_EDITOR_OPERATION_TYPES
};
struct Gtile_Edit {
    Gtile_Cursor where = {};
    Graphical_Tile before = {};
    Graphical_Tile after = {};
};
struct Ctile_Edit {
    Ctile_Cursor where = {};
    Collision_Tile before = {};
    Collision_Tile after = {};
};
struct Etile_Edit {
    Tile_Cursor where = {};
    Enemy_Type before = Enemy_Type::UNINITIALIZED;
    Enemy_Type after = Enemy_Type::UNINITIALIZED;
};
struct Auto_Mode_Edit {
    Ctile_Edit ctile_edit = {};
    ng::array<Gtile_Edit> gtile_edits = {editor_allocator};
};
struct Room_Id_Edit {
    u16 before = 0xFFFF;
    u16 after = 0xFFFF;
};
struct Edit {
    Edit_Type type = Edit_Type::INVALID;
    // @Todo: should be a union, but they suck so we're eating memory for now
    Gtile_Edit gtile_edit;
    Ctile_Edit ctile_edit;
    Etile_Edit etile_edit;
    Auto_Mode_Edit auto_mode_edit;
    Room_Id_Edit room_id_edit;
};
struct Edit_History {
    ng::array<Edit> edits = {editor_allocator};
    int history_size = 0;
};
enum struct Editor_Mode {
    GRAPHICAL_MODE,
    COLLISION_MODE,
    AUTOMATIC_MODE,
    ENEMY_MODE,
};

struct Editor_Input_Box;

/* singleton */ struct Editor_State {
    Bool initialized = False;
    Vector2 view_pos = {};
    Vector2 view_vel = {};
    u16 location = 0xFFFF;

    Bool render_editor_mode_even_when_in_engine = False;
    Editor_Mode current_mode = Editor_Mode::GRAPHICAL_MODE;
    Edit_History edit_history = {};
    Bool in_bounds = False;

    Gtile_Cursor g_mode_cursor = {};
    Bool in_gtem = False;
    Tileset_Cursor gtem_tileset_selection = {};

    Bool g_mode_clipboard_valid = False;
    Tileset_Cursor g_mode_clipboard = {};
    ng::array<Bool> layer_hiddens = {editor_allocator};
    ng::array<Bool> layer_solos = {editor_allocator};

    Ctile_Cursor c_mode_cursor = {};
    Bool in_ctem = False;
    Collision_Tile_Type ctem_type_selection = {};

    Tile_Cursor e_mode_cursor = {};
    Bool in_etem = False;
    Enemy_Type etem_type_selection = {};

    Bool c_mode_clipboard_valid = False;
    Collision_Tile_Type c_mode_clipboard = {};

    Tile_Cursor auto_mode_cursor = {};

    Editor_Input_Box *current_text_input = null;
    ng::array<u8> current_text_input_buffer = {editor_allocator};

    ng::array<Rect_To_Draw> rects_to_draw = {editor_allocator};
};
extern Editor_State editor;
Editor_State editor_create();
void editor_destroy(Editor_State *);

// @Todo @Robustness: event queue rather than calling functions randomly
// @Todo: make all of these take Editor_State *
void toggle_between_engine_and_editor();
void editor_handle_sdl_mouse_button_event(SDL_MouseButtonEvent event);

void editor_handle_sdl_mouse_wheel_event(SDL_MouseWheelEvent event);
void editor_handle_sdl_text_input_event(SDL_TextInputEvent event);
void editor_handle_sdl_text_editing_event(SDL_TextEditingEvent event);
void editor_handle_input(Input_State *in);
void editor_update(Editor_State *editor, f64 dt, Input_State *in);
void editor_render_world(Editor_State *editor, Room *room);
void editor_render(Editor_State *editor, f64 t);

void editor_draw_rect(SDL_Rect r, SDL_Color c = {0, 0, 0, 128});
void editor_draw_rect_scaled(SDL_Rect r, SDL_Color c = {0, 0, 0, 128});
