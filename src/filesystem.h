// @Todo: main cfg file
// @Todo: package format
#pragma once
#include "general.h"
#include "weapon.h"
#include "world.h"
extern ng::allocator *filesystem_allocator;
struct RW { // @Todo @Remove
    SDL_RWops *rw = null;
    ng::string filename = {};
    using elem_t = u8;
    using pos_t = u16;
    using data_t = ng::array<elem_t>;
    RW() = default;
    RW(ng::string filename, ng::string rwmode);
    RW(const RW &) = delete;
    RW(RW &&r) : rw(r.rw) { r.rw = null; }
    ~RW();
    void write_imp(const u8 *ptr, usize n, usize size = sizeof(u8));
    void write(u8 byte);
    void write(ng::string string);
    void write(data_t *data);
    data_t read(usize length);
    enum struct Seek : u8 { Beginning, Cursor, End };
    static int SeekToSDLSeek(Seek seek_mode);
    pos_t seek(s64 offset, Seek mode = Seek::Beginning);
    pos_t pos();
    pos_t length();
    u8 peek();
};
struct Savegame { // @Incomplete
    u16 player_location = 0xFFFF;
    s16 tile_x = 0_t, tile_y = 0_t;
    Bool has_weapon[cast(int) Weapon_Type::NUM_WEAPON_TYPES] = {False};
    Bool jump_powerup = False; // i guess
};
void savegame_create_and_write(Player *player, Room_Collision_Info *ci,
                               Engine_State *engine_state);
Savegame savegame_create(Engine_State *engine_state);
void savegame_write(RW *rw, Savegame *savegame);
void savegame_delete(RW *rw);
Bool savegame_load(RW *rw, Savegame *save);
void savegame_apply(Savegame *savegame, Engine_State *engine_state);

struct Config { // @Incomplete
    float global_volume = 1.0f;
    float volume_ratio = 0.5f;
};
Config config_create(Engine_State *engine_state); // @Todo
void config_write(RW *rw, Config *savegame);
void config_delete(RW *rw);
Bool config_load(RW *rw, Config *save);
void config_apply(Config *savegame, Engine_State *engine_state);

struct Room;
const int MAX_FILENAME_LENGTH = 256;
const int QTAR_VERSION = 5; // \version 5
// \version 5
Room LoadRoom(struct RW *file);
// \version 5
void SaveRoom(struct RW *file, Room *room);
