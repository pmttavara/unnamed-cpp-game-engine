#pragma once
#include "audio.h"
#include "enemy.h"

constexpr auto BREAKABLE_HEART_HEAL_AMT = 5_hp;

extern ng::allocator *world_allocator;

enum struct Collision_Tile_Type : u8 {
    AIR = 0x00,
    SOLID = 0x01,
    HALF_TALL = 0x02,

    BREAKABLE,
    BREAKABLE_HEART,

    SAVE_SPOT,

    // EXITS
    ROOM_EDGE_TOP = 0xF0,
    ROOM_EDGE_BOTTOM = 0xF1,
    ROOM_EDGE_LEFT = 0xF2,
    ROOM_EDGE_RIGHT = 0xF3,
    CUSTOM_EXIT_0 = 0xF4,
    CUSTOM_EXIT_1 = 0xF5,
    CUSTOM_EXIT_2 = 0xF6,
    CUSTOM_EXIT_3 = 0xF7,
    CUSTOM_EXIT_4 = 0xF8,
    CUSTOM_EXIT_5 = 0xF9,
    CUSTOM_EXIT_6 = 0xFA,
    CUSTOM_EXIT_7 = 0xFB,
    CUSTOM_EXIT_8 = 0xFC,
    CUSTOM_EXIT_9 = 0xFD,
    CUSTOM_EXIT_A = 0xFE,
    CUSTOM_EXIT_B = 0xFF,
};
struct Collision_Tile {
    Collision_Tile_Type type = Collision_Tile_Type::AIR;
    int exit_number() {
        int result = static_cast<int>(type) -
                     static_cast<int>(Collision_Tile_Type::ROOM_EDGE_TOP);
        if (result < 0) {
            result = -1;
        }
        return result;
    }
    Bool is_exit() { return exit_number() >= 0; }
};
struct Graphical_Tile {
    u8 index = 0xFF;
};
struct Rect;
struct Enemy;
s16 layer_width(s16 w, u8 parallax);
s16 layer_height(s16 h, u8 parallax);
struct Room_Collision_Info {
    s16 x = 0_t, y = 0_t;
    Collision_Tile tile = {};
    Collision_Tile_Type type() { return tile.type; }
};
static_assert(sizeof(Room_Collision_Info) == 6, "");
// @Volatile: Edit RoomReader when you modify this struct's layout
enum struct Room_Exit_Mode : u8 { INACTIVE, ON_INTERACTION, ON_TOUCH };
struct Room_Exit {
    Room_Exit_Mode mode = Room_Exit_Mode::INACTIVE;
    Bool preserve_x = False;
    Bool preserve_y = False;
    Bool preserve_vel = False;
    u8 _dummy_padding = 0;
    u16 destination = 0xFFFF;
    f32 player_x = 0._g, player_y = 0._g; // Player pos in destination room
    // How many tiles to offset CURRENT room (to align ground/walls when
    // preserving x/y). Positive values move CURRENT room RIGHT/DOWN rel. to
    // destination
    s16 offset_x = 0_t, offset_y = 0_t;
};
// static_assert(sizeof(Room_Exit) == 16, "");
struct Room_Layer {
    TextureAtlas tileset = {};
    u8 parallax = 255; // \brief Cam position multiplier out of 255
    ng::array<Graphical_Tile> tiles = {world_allocator};
    Room_Layer() = default;
};
Room_Layer room_layer_create(ng::string tileset_filename, u8 parallax,
                             ng::array<Graphical_Tile> tiles);
// static_assert(sizeof(Room_Layer) == 32, "");
struct Room {
    ng::string filename = {};
    Room_Exit get_exit_info(int index);
    int numExits() { return exits.count; }
    struct MusicInfo { // @Migrate this out to an Audio_Music_Transition struct
        Bool use_music_info = False;
        Bool loop = False;
        u16 fadeout_time = 0; // \brief Time to fade out the previous song, ms
        u16 fadein_time = 0;  // \brief Time to fade in this room's song, ms
        ng::string filename = no_music;
        static const ng::string
            no_music;                    // \brief Continue playing current song
        static const ng::string silence; // \brief Turn off any playing music
    };
    u16 id = 0xFFFF;
    Bool is_indoors = False;
    using layer_index = u8;
    layer_index numLayers() { return cast(u8) layers.count; }
    layer_index firstForegroundLayer() { return first_fg_layer; }
    enum struct LayerMode : u8 { Background, Foreground };
    Room() = default;
    /* */ Collision_Tile &at(s16 x, s16 y) /* */ {
        return collision[static_cast<int>((x + (y * w)))];
    }
    // const Collision_Tile &at(s16 x, s16 y) const {
    //     return collision[static_cast<int>((x + (y * w)))];
    // }
    void SetTile(layer_index index, s16 x, s16 y, Graphical_Tile new_tile);
    void Render(LayerMode mode);
    void RenderLayer(layer_index i);
    ng::array<Room_Collision_Info> get_colliding_tiles(Rect rect);
    MusicInfo music_info = {};
    s16 w = 0_t;
    s16 h = 0_t;
    layer_index getBegin(LayerMode mode) {
        return (mode == Room::LayerMode::Foreground ? first_fg_layer : 0);
    }
    layer_index getEnd(LayerMode mode) {
        return (mode == Room::LayerMode::Foreground ? numLayers()
                                                    : first_fg_layer);
    }
    layer_index first_fg_layer = 0; // \brief First layer drawn after the player
    ng::array<Room_Layer> layers = {
        world_allocator}; // \brief 0 is farthest behind scene, n frontmost
    ng::array<Collision_Tile> collision = {world_allocator};
    ng::array<Room_Exit> exits = {world_allocator};
    ng::array<Enemy_Type> enemies = {world_allocator};
};
namespace ng {
void print_item(ng::print_buffer *, const Room &);
}
/* singleton */ struct World {
    f32 room_fade_out_length = 0.016_s * 5;    // @Migrate these to Player
    f32 room_fade_pause_length = 0.016_s * 12; // @Migrate these to Player
    f32 room_fade_in_length = 0.016_s * 7;     // @Migrate these to Player

    ng::map<u16, Room> rooms = {world_allocator};
    Room *AddRoom(Room room);
    void DeleteRoom(u16 id);
    Room *get_room(u16 id);
};
void audio_apply_room_music(Room::MusicInfo music_info);
void room_transition(World *world, Player *player);
// @Todo: Room *add_room(World *world, Room room);
// @Todo: void delete_room(World *world, u16 id);
// @Todo: Room *get_room(World *world, u16 id);
// @Todo: Room *current_room(World *world);
// @Todo: u16 cur_id();
// @Todo: void go_to_room(World *world, u16 id);
// @Todo: void room_transition(World *world);
// @Todo: void take_exit(World *world, int index);
Room room_create();
Room room_copy(Room *room);
void room_destroy(Room *room);
World world_create();
World world_copy(World *world);
void world_destroy(World *world);

extern World *world, *world_sbs;
struct Npc {
    u16 location = 0xFFFF;
    u16 npc_id = 0xFFFF;
    Vector2 pos = {};
    Vector2 vel = {};
    Rect hitbox = {};
    Sprite sprite = {};
    u16 talk_string_id = 0xFFFF;

    float walk_speed = 0;
    Bool facing_left = False;
    int current_action_mode = -1;
    Timer action_timer = {};
    void update(f32 dt, Room *room);
    void Render();
};
inline void npc_destroy(Npc *npc) { sprite_destroy(&npc->sprite); }
inline Npc npc_copy(Npc *rhs) {
    Npc result = *rhs;
    result.sprite = sprite_copy(&rhs->sprite);
    return result;
}
enum struct Pickup_Type : u8 { UNINITIALIZED, HEART, WEAPON, NUM_PICKUP_TYPES };
struct Pickup {
    u16 location = 0xFFFF;
    Vector2 pos = {};
    Pickup_Type type = Pickup_Type::UNINITIALIZED;
    union {
        u16 heal_amount;
        u8 weapon_type;
    } info;
};
struct Pickup_Data {
    Sprite sprite;
    Sound sound;
    f32 pickup_radius;
};
extern Pickup_Data pickup_data[cast(usize) Pickup_Type::NUM_PICKUP_TYPES];
struct Projectile {
    Texture texture = {};
    Sound hit_sound = {};
    u16 location = 0xFFFF;
    Vector2 pos = {};
    Vector2 vel = {};
    Rect hitbox = {};
    u16 damage = 0_hp;
    Vector2 knockback = {};
    Bool hurts_enemies = False;
    Bool hurts_player = False;

    void invalidate() { damage = 0_hp; }
    Bool is_invalid() { return damage == 0_hp; }
};
void do_hit_block(Room *room, Room_Collision_Info hit_block);
void do_hit_projectile(Projectile *proj, Vector2 other_entity_vel);
Bool check_collided(Collision_Tile ct);
/* singleton */ struct Entity_Manager {
    ng::array<Enemy> enemies = {world_allocator};
    ng::array<Npc> npcs = {world_allocator};
    ng::array<Pickup> pickups = {world_allocator};
    ng::array<Projectile> projectiles = {world_allocator};

    void update(f32 dt, World *world);
    void render_below_player();
    void render_above_player();
};
Entity_Manager entity_manager_copy(Entity_Manager *rhs);
void entity_manager_destroy(Entity_Manager *rhs);
extern Entity_Manager *entity_manager, *entity_manager_sbs;
struct Collide_State {
    Vector2 pos = {};
    Vector2 delta = {};
    Rect collider_left = {};
    Rect collider_right = {};
    Rect collider_top = {};
    Rect collider_bottom = {};
};
struct Room_Collision_Test_Result {
    Bool did_collide = False;
    Bool did_hit_ontouch_exit = False;
    Room_Collision_Info first_ontouch_exit = {};
};
struct Collide_Result {
    Collide_State new_state = {};
    Room_Collision_Test_Result bottom = {}, top = {}, right = {}, left = {};
};
Collide_Result collide(Collide_State collide_state, Room *room);
