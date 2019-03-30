#include "filesystem.h"
#include "camera.h"
#include "engine_state.h" // @Todo: move all global state into a Globals struct
#include "player.h"
#include "world.h"
ng::allocator *filesystem_allocator = null;
RW::RW(ng::string filename, ng::string rwmode) {
    rw = SDL_RWFromFile(ng::c_str(filename), ng::c_str(rwmode));
    ng_assert(rw != null);
    this->filename = filename;
}
void RW::write_imp(const u8 *ptr, usize n, usize size) {
    ng_assert(SDL_RWwrite(rw, ptr, size, n) == n, "Error writing to file.");
}
void RW::write(ng::string str) { write_imp(cast(u8 *) str.ptr, str.len); }
void RW::write(u8 byte) {
    u8 x = byte;
    write_imp(&x, 1);
}
void RW::write(data_t *data) { write_imp(data->data, data->count); }
RW::data_t RW::read(usize length) {
    data_t result = {filesystem_allocator};
    result.resize(length);
    SDL_RWread(rw, cast(void *) result.data, 1, length);
    return result;
}
int RW::SeekToSDLSeek(Seek seek_mode) {
    switch (seek_mode) {
    case Seek::Beginning: {
        return RW_SEEK_SET;
    } break;
    case Seek::Cursor: {
        return RW_SEEK_CUR;
    } break;
    case Seek::End: {
        return RW_SEEK_END;
    } break;
    }
    return -1;
}
RW::pos_t RW::seek(s64 offset, Seek mode) {
    auto result = SDL_RWseek(rw, offset, SeekToSDLSeek(mode));
    ng_assert(result != -1 && result < 0x10000);
    return result & 0xFFFF;
}
RW::pos_t RW::pos() {
    auto result = seek(0, Seek::Cursor);
    return result;
}
RW::pos_t RW::length() {
    auto result = SDL_RWsize(rw);
    ng_assert(result != -1 && result < 0x10000);
    return result & 0xFFFF;
}
u8 RW::peek() {
    u8 result = 0;
    SDL_RWread(rw, &result, 1, 1);
    SDL_RWseek(rw, -1, RW_SEEK_CUR);
    return result;
}
RW::~RW() {
    if (rw) {
        SDL_RWclose(rw);
    }
}
void savegame_create_and_write(Player *player, Room_Collision_Info *ci,
                               Engine_State *) {
    // @Incomplete
    Savegame savegame = {};
    savegame.player_location = player->location;
    savegame.tile_x = ci->x;
    savegame.tile_y = ci->y;
    ng_for(player->arsenal) { savegame.has_weapon[cast(int) it.type] = True; }
    savegame.jump_powerup = player->jump_powerup;
    {
        RW rw(GAME_NAME ".sav"_s, "r+b"_s);
        savegame_write(&rw, &savegame);
    }
}
Savegame savegame_create(Engine_State *engine_state) { // @Incomplete
    USE engine_state;
    return {};
}
void savegame_write(RW *rw, Savegame *savegame) { // @Volatile @Bug: Endianness
                                                  // differences between the
                                                  // writer and the reader
    u8 *p = cast(u8 *) savegame;
    for (usize i = 0; i < sizeof(Savegame);
         i += 1) { // @Todo: make this cleaner than some loop
        rw->write(p[i]);
    }
}
void savegame_delete(RW *rw) {
    Savegame s = {};
    savegame_write(rw, &s);
}
Bool savegame_load(RW *rw, Savegame *save) {
    Bool is_valid_savegame = False;
    Savegame result = {};
    if (rw->length() >= sizeof(Savegame)) {
        defer { rw->seek(0); };
        auto savegame_data = rw->read(sizeof(Savegame));
        defer { savegame_data.release(); };
        result = *cast(Savegame *) savegame_data.data;
        if (result.player_location != 0xFFFF) {
            is_valid_savegame = True;
        }
    }

    if (is_valid_savegame) {
        if (save) {
            *save = result;
        }
    }
    return is_valid_savegame;
}
void savegame_apply(Savegame *savegame, Engine_State *engine_state) {
    // @Todo: apply savegame to world stuff
    player->location = savegame->player_location;
    player->transitioning_rooms = True;
    player->room_exit_info = {};
    player->room_exit_info.destination = player->location;
    player->room_exit_info.preserve_x = True;
    player->room_exit_info.preserve_y = True;
    player->room_exit_info.preserve_vel = True;
    player->target_me(camera);
    room_transition(world, player);
    player->pos.x = tile_to_game(savegame->tile_x) + tile_to_game(1_t) / 2;
    player->pos.y = tile_to_game(savegame->tile_y) + tile_to_game(1_t) / 2;
#define count_of(x) ((sizeof(x) / sizeof(0 [x])) / !(sizeof(x) % sizeof(0 [x])))
    for (int i = 0; i < cast(int) count_of(savegame->has_weapon); i += 1) {
        if (savegame->has_weapon[i] == True) {
            Weapon new_weapon = weapon_from_type(cast(Weapon_Type) i, player);
            player->arsenal.push(new_weapon);
        }
    }
    player->jump_powerup = savegame->jump_powerup;
    engine_state->savegame_loaded = True;
}

union Alias {
    int i;
    float f;
};
const u16 HEADER_SIZE = 32;      // One per file
const u16 EXIT_HEADER_SIZE = 4;  // One per file
const u16 EXIT_SIZE = 24;        // One per exit (obviously)
const u16 LAYER_HEADER_SIZE = 8; // One per layer
static float get_float(RW::elem_t *ptr) {
    Alias bits;
    bits.i = {ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24)};
    return bits.f;
}
static void set_float(RW::elem_t *ptr, float f) {
    Alias bits;
    bits.f = f;
    ptr[0] = static_cast<RW::elem_t>(bits.i >> 0 & 0xFF);
    ptr[1] = static_cast<RW::elem_t>(bits.i >> 8 & 0xFF);
    ptr[2] = static_cast<RW::elem_t>(bits.i >> 16 & 0xFF);
    ptr[3] = static_cast<RW::elem_t>(bits.i >> 24 & 0xFF);
}
RW::data_t read_at(RW *file, int offset, int length) {
    u16 original_pos = file->pos();
    file->seek(offset);
    RW::data_t data = file->read(length);
    file->seek(original_pos);
    return data;
}
static void write_at(RW *file, int offset, RW::data_t data) {
    // u16 original_pos = file.pos();
    // file.seek(offset);
    // file.write(data);
    // file.seek(original_pos);
    auto pos = SDL_RWseek(file->rw, 0, RW_SEEK_CUR);
    SDL_RWseek(file->rw, offset, RW_SEEK_SET);
    SDL_RWwrite(file->rw, data.data, 1, data.count);
    SDL_RWseek(file->rw, pos, RW_SEEK_SET);
}
static ng::string get_string(RW *file, int offset) {
    RW::data_t result = read_at(file, offset, MAX_FILENAME_LENGTH);
    defer { result.release(); };
    return copy_string(ng::string(cast(const char *) result.data),
                       world_allocator); // reads til NUL
}
// \brief Call this after writing all other data, and write the return value to
// the location in the file to store the "ng::string" (the offset).
static u16 save_string(RW *file, ng::string data) {
    u16 offset = file->pos();
    file->write(data);
    if (!data.null_terminated()) {
        file->write(cast(u8) '\0');
    }
    return offset;
}
static void write_offset_to(RW *file, u16 offset_to_write,
                            u16 location_to_write_offset) {
    RW::data_t offset_data = {world_allocator};
    defer { offset_data.release(); };
    offset_data.resize(2);
    offset_data[0] = offset_to_write >> 0 & 0xFF;
    offset_data[1] = offset_to_write >> 8 & 0xFF;
    write_at(file, location_to_write_offset, offset_data);
}
Room LoadRoom(RW *file) {
    Room result = {};
    result.filename = copy_string(file->filename, world_allocator);
    // read header and init room
    RW::data_t header = file->read(HEADER_SIZE);
    defer { header.release(); };
    ng_assert(header[0] == 'Q' && header[1] == 'T' && header[2] == 'A' &&
              header[3] == 'R');
    ng_assert(header[4] == QTAR_VERSION);
    const auto room_w = static_cast<s16>(header[5] | (header[6] << 8));
    result.w = room_w;
    const auto room_h = static_cast<s16>(header[7] | (header[8] << 8));
    result.h = room_h;
    const auto room_size = (room_w * room_h);
    const Room::layer_index num_layers = header[9];
    const Room::layer_index first_fg_layer = header[10];
    auto mi = &result.music_info;
    mi->use_music_info = !!(header[11] & 0x80);
    mi->loop = !!(header[11] & 0x40);
    mi->fadein_time = cast(u16)(header[15] | (header[16] << 8));
    mi->fadeout_time = cast(u16)(header[17] | (header[18] << 8));
    {
        auto offset = header[19] | (header[20] << 8);
        ng_assert(offset <= 0xFFFF);
        auto music_filename = get_string(file, offset & 0xfFfF);
        mi->filename = music_filename;
    }
    u16 id = cast(u16)(header[21] | (header[22] << 8));
    result.id = id;
    // load exits
    RW::data_t exit_header = file->read(EXIT_HEADER_SIZE);
    defer { exit_header.release(); };
    const unsigned int num_exits = exit_header[0];
    ng::array<Room_Exit> exits = {world_allocator};
    exits.resize(num_exits);
    {
        for (unsigned int i = 0; i < num_exits; i += 1) {
            RW::data_t exit_data = file->read(EXIT_SIZE);
            defer { exit_data.release(); };
            exits[i].mode =
                static_cast<Room_Exit_Mode>((exit_data[0] & 0xF0) >> 4);
            exits[i].preserve_x =
                ((exit_data[0] & 0x08) >> 3) != 0 ? True : False;
            exits[i].preserve_y =
                ((exit_data[0] & 0x04) >> 2) != 0 ? True : False;
            exits[i].preserve_vel =
                ((exit_data[0] & 0x02) >> 1) != 0 ? True : False;
            exits[i]._dummy_padding = exit_data[1];
            exits[i].destination =
                cast(u16)(exit_data[2] | (exit_data[3] << 8));
            exits[i].player_x = get_float(exit_data.data + 4);
            exits[i].player_y = get_float(exit_data.data + 8);
            exits[i].offset_x =
                static_cast<s16>(exit_data[12] | (exit_data[13] << 8));
            exits[i].offset_y =
                static_cast<s16>(exit_data[14] | (exit_data[15] << 8));
            // more bots
        }
    }
    result.exits = exits;
    result.collision.resize(room_w * room_h);
    { // load collision
        RW::data_t collision_data = file->read(room_size);
        defer { collision_data.release(); };
        for (u16 i = 0; i < room_size; i += 1) {
            s16 x = (i % room_w);
            s16 y = (i / room_w);
            result.at(x, y).type =
                static_cast<Collision_Tile_Type>(collision_data[i]);
        }
    }
    result.layers.resize(num_layers);
    { // load layers
        // const unsigned layer_size = LAYER_HEADER_SIZE + room_size; @Todo:
        // apparently unused. Why?
        for (unsigned int i = 0; i < num_layers; i += 1) {
            RW::data_t layer_header = file->read(LAYER_HEADER_SIZE);
            defer { layer_header.release(); };
            ng::string tileset_file =
                get_string(file, layer_header[0] | (layer_header[1] << 8));
            defer { free_string(&tileset_file, world_allocator); };
            if (tileset_file == "") {
                continue;
            }
            auto parallax = static_cast<decltype(Room_Layer::parallax)>(
                layer_header[2]); // @FixMe: rewrite this when parallax sizes
                                  // are fixed
            RW::data_t layer_tiles = file->read(room_size);
            defer { layer_tiles.release(); };
            ng::array<Graphical_Tile> tiles = {world_allocator};
            tiles.resize(room_size);
            for (int j = 0; j < tiles.count; j += 1) {
                tiles[j].index = layer_tiles[j];
            }
            result.layers[i] = room_layer_create(tileset_file, parallax, tiles);
        }
    }
    result.first_fg_layer = first_fg_layer;
    result.id = id;
    result.enemies.resize(room_w * room_h);
    { // load enemy data
        ng_assert(result.w > 0 && result.h > 0, "result.w = %0, result.h = %1",
                  result.w, result.h);
        u32 enemies_size = result.w * result.h;
        result.enemies.reserve(enemies_size);
        for (usize i = 0; i < enemies_size; i += 1) {
            auto enemy_type = file->read(sizeof(Enemy_Type));
            defer { enemy_type.release(); };
            ng_assert(enemy_type.count == sizeof(Enemy_Type));
            auto etype_ptr = cast(Enemy_Type *) enemy_type.data;
            result.enemies[i] = *etype_ptr;
        }
    }
    return result;
}
void SaveRoom(RW *file, Room *room) {
    auto room_w = room->w;
    auto room_h = room->h;
    u16 music_name_offset = {};
    ng::array<u16> layer_tileset_offsets = {filesystem_allocator};
    layer_tileset_offsets.reserve(room->numLayers());
    const int room_size = room_w * room_h;
    { // write header
        RW::data_t header = {world_allocator};
        header.resize(HEADER_SIZE);
        header[0] = 'Q';
        header[1] = 'T';
        header[2] = 'A';
        header[3] = 'R';
        header[4] = QTAR_VERSION;
        header[5] = room_w >> 0 & 0xFF;
        header[6] = room_w >> 8 & 0xFF;
        header[7] = room_h >> 0 & 0xFF;
        header[8] = room_h >> 8 & 0xFF;
        header[9] = room->numLayers() & 0xFF;
        header[10] = room->firstForegroundLayer() & 0xFF;
        header[11] |= (room->music_info.use_music_info ? 1 : 0) << 7;
        header[11] |= (room->music_info.loop ? 1 : 0) << 6;
        header[15] = room->music_info.fadein_time >> 0 & 0xFF;
        header[16] = room->music_info.fadein_time >> 8 & 0xFF;
        header[17] = room->music_info.fadeout_time >> 0 & 0xFF;
        header[18] = room->music_info.fadeout_time >> 8 & 0xFF;
        music_name_offset = 19;
        header[21] = room->id >> 0 & 0xFF;
        header[22] = room->id >> 8 & 0xFF;
        file->write(&header);
    }
    { // write exits
        int num_exits = room->numExits();
        { // write exit header
            RW::data_t exit_header = {world_allocator};
            exit_header.resize(EXIT_HEADER_SIZE);
            exit_header[0] = num_exits & 0xFF;
            file->write(&exit_header);
        }
        for (int i = 0; i < num_exits; i += 1) {
            RW::data_t data = {world_allocator};
            data.resize(EXIT_SIZE);
            Room_Exit exit = room->get_exit_info(i);
            data[0] |= static_cast<u8>(exit.mode) << 4;
            data[0] |= (exit.preserve_x ? 1 : 0) << 3;
            data[0] |= (exit.preserve_y ? 1 : 0) << 2;
            data[0] |= (exit.preserve_vel ? 1 : 0) << 1;
            data[1] |= exit._dummy_padding;
            data[2] = exit.destination >> 0 & 0xFF;
            data[3] = exit.destination >> 8 & 0XFF;
            set_float(data.data + 4, exit.player_x);
            set_float(data.data + 8, exit.player_y);
            data[12] = exit.offset_x >> 0 & 0xFF;
            data[13] = exit.offset_x >> 8 & 0xFF;
            data[14] = exit.offset_y >> 0 & 0xFF;
            data[15] = exit.offset_y >> 8 & 0xFF;
            file->write(&data);
        }
    }
    { // write collision
        RW::data_t collision = {world_allocator};
        collision.resize(room_size);
        for (int i = 0; i < static_cast<signed>(room_size); i += 1) {
            collision[i] = cast(u8)(
                room->at(cast(s16)(i % room_w), cast(s16)(i / room_w)).type);
        }
        file->write(&collision);
    }
    { // write layers
        ng_for(room->layers) {
            RW::data_t layer_header = {world_allocator};
            layer_header.resize(LAYER_HEADER_SIZE);
            layer_tileset_offsets.push(file->pos());
            layer_header[2] = it.parallax;
            RW::data_t layer_tiles = {world_allocator};
            layer_tiles.resize(room_size);
            for (int i = 0; i < room_size; i += 1) {
                layer_tiles[i] = it.tiles[i].index;
            }
            file->write(&layer_header);
            file->write(&layer_tiles);
        };
    }
    { // write enemies
        ng_assert(sizeof(Enemy_Type) == sizeof(u8) && sizeof(u8) == 1);
        int len = cast(int)(room->w) * room->h;
        ng_assert(room->enemies.count == len, "enemies = %0", room->enemies);
        for (int i = 0; i < len; i += 1) {
            file->write(cast(u8) room->enemies[i]);
        }
    }
    { // write strings
        write_offset_to(file, save_string(file, room->music_info.filename),
                        music_name_offset);
        for (Room::layer_index i = 0; i < room->numLayers(); i += 1) {
            ng::string filename;
            if (room->layers[i].tileset.base.tex == null) {
                filename = "";
            } else {
                filename =
                    renderer->filename_of(room->layers[i].tileset.base.tex);
            }
            write_offset_to(file, save_string(file, filename),
                            layer_tileset_offsets[i]);
        }
    }
}
