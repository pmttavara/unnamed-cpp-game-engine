#include "editor.h"
#include "camera.h"
#include "engine_state.h"
#include "filesystem.h"
#include "input.h"
#include "player.h"
#include "ui.h"

ng::allocator *editor_allocator = null;

Graphical_Tile *gtile_cursor_to_gtile_ptr(Gtile_Cursor cursor, Room *room) {
    auto layer = &room->layers[cursor.layer];
    auto lw = layer_width(room->w, layer->parallax);
    auto graphical_tile = &(layer->tiles)[cursor.where.x + cursor.where.y * lw];
    return graphical_tile;
}
u8 tileset_cursor_to_gtile_index(Tileset_Cursor cursor, TextureAtlas *tileset) {
    ng_assert(cursor.x < tileset->atlasWidth &&
              cursor.y < tileset->atlasHeight);
    auto idx = cursor.x + cursor.y * tileset->atlasWidth;
    return cast(u8) idx;
}
Tileset_Cursor gtile_index_to_tileset_cursor(u8 index, TextureAtlas *tileset) {
    Tileset_Cursor result = {};
    ng_assert(index < tileset->num_frames);
    result.x = index % tileset->atlasWidth;
    result.y = index / tileset->atlasWidth;
    return result;
}
Collision_Tile *ctile_cursor_to_ctile_ptr(Ctile_Cursor cursor, Room *room) {
    return &room->at(cursor.where.x, cursor.where.y);
}

Enemy_Type *etile_cursor_to_etile_ptr(Tile_Cursor cursor, Room *room) {
    return &room->enemies[cursor.x + (cursor.y * room->w)];
}

void edit_destroy(Edit *edit) {
    if (edit->type == Edit_Type::AUTO_MODE_EDIT) {
        edit->auto_mode_edit.gtile_edits.release();
    }
}

//
//
//
//

struct Editor_Temp_String {
    ng::string string = {};
    Bool should_free = False;
};

using Editor_Input_Get_String_Function =
    Editor_Temp_String(Editor_State *editor);
using Editor_Input_Complete_Function = void(Editor_State *editor);

struct Editor_Input_Box {
    SDL_Rect input_rect = {};
    Editor_Input_Get_String_Function *get_string = null;
    Editor_Input_Complete_Function *complete = null;
};

RW::data_t read_at(RW *file, int offset, int length);

Editor_Input_Box editor_room_filename_input = {
    {4, 256 + 24 * 0, 256, 16},
    [](Editor_State *editor) -> Editor_Temp_String {
        Editor_Temp_String result = {};

        auto room = world->get_room(editor->location); // @Global
        result.string = room->filename;
        result.should_free = False;
        return result;
    },
    [](Editor_State *editor) {
        auto filename =
            ng::copy_string({editor->current_text_input_buffer.count,
                             editor->current_text_input_buffer.data});
        RW room_to_open{filename, "rb"};              // @Hack
        auto id_data = read_at(&room_to_open, 21, 2); // @Volatile @Hack
        auto id = cast(u16)(id_data[0] | (id_data[1] << 8));
        auto room_at_id = world->get_room(id);
        if (!room_at_id) {
            Room room_to_add = LoadRoom(&room_to_open);
            room_at_id = world->AddRoom(room_to_add);
        }
        editor->location = id;
        editor->view_pos = {};
    }};

Editor_Input_Box editor_room_music_filename_input = {
    {4, 256 + 24 * 1, 256, 16},
    [](Editor_State *editor) -> Editor_Temp_String {
        Editor_Temp_String result = {};
        auto room = world->get_room(editor->location); // @Global
        result.string = room->music_info.filename;
        result.should_free = False;
        return result;
    },
    [](Editor_State *editor) {
        auto current_room = world->get_room(editor->location);
        auto music_filename = &current_room->music_info.filename;
        ng::free_string(music_filename, world_allocator);
        auto new_music_filename =
            ng::copy_string({editor->current_text_input_buffer.count,
                             editor->current_text_input_buffer.data});
        *music_filename = new_music_filename;
    }};

Editor_Input_Box editor_layer_tileset_filename_input = {
    {4, 256 + 24 * 2, 256, 16},
    [](Editor_State *editor) -> Editor_Temp_String {
        Editor_Temp_String result = {};
        auto room = world->get_room(editor->location); // @Global
        auto layer_tileset = room->layers[editor->g_mode_cursor.layer].tileset;
        result.string =
            renderer->filename_of(layer_tileset.base.tex); // @Global
        result.should_free = False;
        return result;
    },
    [](Editor_State *editor) {
        auto current_room = world->get_room(editor->location);
        auto layer = &current_room->layers[editor->g_mode_cursor.layer];
        ng::string new_layer_tileset_filename = {
            editor->current_text_input_buffer.count,
            editor->current_text_input_buffer.data};
        layer->tileset.base = texture_load(new_layer_tileset_filename);
    }};

Editor_Input_Box test_rect = {
    {256, 256, 64, 64},
    [](Editor_State *editor) -> Editor_Temp_String {
        USE editor;
        Editor_Temp_String result = {};
        auto buf = ng::mprint("Testing!");
        defer { ng::free(buf.str); };
        result.string = ng::copy_string(ng::string{buf}); // @Speed
        result.should_free = True;
        return result;
    },
    [](Editor_State *editor) {
        auto str = ng::string(editor->current_text_input_buffer.count,
                              editor->current_text_input_buffer.data);
        ng::print("%0", str);
    }};

Editor_Input_Box *editor_inputs[] = {
    &editor_room_filename_input, &editor_room_music_filename_input,
    &editor_layer_tileset_filename_input, &test_rect};

//
//
//
//
//

Editor_State editor = {};
Editor_State editor_create() {
    editor_allocator = ng::default_allocator;
    Editor_State result = {};
    result.initialized = True;
    return result;
}
void editor_destroy(Editor_State *editor) {
    editor->initialized = False;
    editor->edit_history.history_size = 0;
    ng_for(editor->edit_history.edits) { edit_destroy(&it); }
    editor->edit_history.edits.release();
    editor->current_text_input_buffer.release();
    editor->layer_hiddens.release();
    editor->layer_solos.release();
    editor->rects_to_draw.release();
}
static void editor_do_edit(Edit *edit, Room *room) {
    ng_assert(edit->type != Edit_Type::INVALID);
    switch (edit->type) {
    default: {
    } break;
    case (Edit_Type::GTILE_EDIT): {
        auto gtile = gtile_cursor_to_gtile_ptr(edit->gtile_edit.where, room);
        *gtile = edit->gtile_edit.after;
    } break;
    case (Edit_Type::CTILE_EDIT): {
        auto ctile = ctile_cursor_to_ctile_ptr(edit->ctile_edit.where, room);
        *ctile = edit->ctile_edit.after;
    } break;
    case (Edit_Type::ETILE_EDIT): {
        auto etile = etile_cursor_to_etile_ptr(edit->etile_edit.where, room);
        *etile = edit->etile_edit.after;
    };
    case (Edit_Type::AUTO_MODE_EDIT): {
        {
            Edit ctile_edit = {}; // @Todo: lots of @Boilerplate
            ctile_edit.type = Edit_Type::CTILE_EDIT;
            ctile_edit.ctile_edit = edit->auto_mode_edit.ctile_edit;
            editor_do_edit(&ctile_edit, room);
        }
        ng_for(edit->auto_mode_edit.gtile_edits) {
            Edit gtile_edit = {};
            gtile_edit.type = Edit_Type::GTILE_EDIT;
            gtile_edit.gtile_edit = it;
            editor_do_edit(&gtile_edit, room);
        }
    } break;
    };
}
static void editor_do_edit_reverse(Edit *edit, Room *room) {
    ng_assert(edit->type != Edit_Type::INVALID);
    switch (edit->type) {
    default: {
    } break;
    case (Edit_Type::GTILE_EDIT): {
        auto gtile = gtile_cursor_to_gtile_ptr(edit->gtile_edit.where, room);
        *gtile = edit->gtile_edit.before;
    } break;
    case (Edit_Type::CTILE_EDIT): {
        auto ctile = ctile_cursor_to_ctile_ptr(edit->ctile_edit.where, room);
        *ctile = edit->ctile_edit.before;
    } break;
    case (Edit_Type::ETILE_EDIT): {
        auto etile = etile_cursor_to_etile_ptr(edit->etile_edit.where, room);
        *etile = edit->etile_edit.before;
    } break;
    case (Edit_Type::AUTO_MODE_EDIT): {
        ng_for(edit->auto_mode_edit.gtile_edits) {
            Edit gtile_edit = {};
            gtile_edit.type = Edit_Type::GTILE_EDIT;
            gtile_edit.gtile_edit = it;
            editor_do_edit_reverse(&gtile_edit, room);
        }
        {
            Edit ctile_edit = {}; // @Todo: lots of @Boilerplate
            ctile_edit.type = Edit_Type::CTILE_EDIT;
            ctile_edit.ctile_edit = edit->auto_mode_edit.ctile_edit;
            editor_do_edit_reverse(&ctile_edit, room);
        }
    } break;
    };
};
static void editor_push_and_apply_edit(Edit *edit, Room *room,
                                       Editor_State *editor) {
    auto hist = &editor->edit_history;
    editor_do_edit(edit, room);
    for (auto i = hist->history_size; i < hist->edits.count; i += 1) {
        edit_destroy(&hist->edits[i]);
    }
    hist->edits.resize(hist->history_size + 1);
    hist->edits[hist->history_size] = *edit;
    hist->history_size += 1;
}
static void editor_undo(Editor_State *editor, Room *room) {
    auto hist = &editor->edit_history;
    if (hist->history_size > 0) {
        auto edit = &hist->edits[hist->history_size - 1];
        editor_do_edit_reverse(edit, room);
        hist->history_size -= 1;
    }
}
static void editor_redo(Editor_State *editor, Room *room) {
    auto hist = &editor->edit_history;
    if (hist->history_size < cast(int) hist->edits.count) {
        auto edit = &hist->edits[hist->history_size];
        editor_do_edit(edit, room);
        hist->history_size += 1;
        ui->nav_bloop.play(); // @Todo: take ui as parameter
    }
}
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//       @Refactor: All of these (*)tile edits have much @Boilerplate.
//
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
static Edit editor_make_gtile_index_edit(Gtile_Cursor where, u8 new_index,
                                         Room *room) {
    Edit edit = {};
    {
        edit.type = Edit_Type::GTILE_EDIT;
        edit.gtile_edit.where = where;
        auto gtile = gtile_cursor_to_gtile_ptr(where, room);
        edit.gtile_edit.before = *gtile;
        edit.gtile_edit.after = *gtile;
        edit.gtile_edit.after.index = new_index;
    }
    return edit;
}
static Edit editor_make_ctile_type_edit(Ctile_Cursor where,
                                        Collision_Tile_Type new_type,
                                        Room *room) {
    Edit edit = {};
    {
        edit.type = Edit_Type::CTILE_EDIT;
        edit.ctile_edit.where = where;
        auto ctile = ctile_cursor_to_ctile_ptr(where, room);
        edit.ctile_edit.before = *ctile;
        edit.ctile_edit.after = *ctile;
        edit.ctile_edit.after.type = new_type;
    }
    return edit;
}
static Edit editor_make_etile_type_edit(Tile_Cursor where, Enemy_Type new_type,
                                        Room *room) {
    Edit edit = {};
    {
        edit.type = Edit_Type::ETILE_EDIT;
        edit.etile_edit.where = where;
        auto etile = etile_cursor_to_etile_ptr(where, room);
        edit.etile_edit.before = *etile;
        edit.etile_edit.after = new_type;
    }
    return edit;
}
enum Auto_Mode_Edit_Type { SOLID, AIR };
Edit editor_make_auto_mode_edit(Auto_Mode_Edit_Type, Room *room,
                                Tile_Cursor auto_mode_cursor) {
    Edit edit = {};
    edit.type = Edit_Type::AUTO_MODE_EDIT;
    auto a_edit = &edit.auto_mode_edit;
    Ctile_Cursor ctile_cursor = {};
    ctile_cursor.where = auto_mode_cursor;
    a_edit->ctile_edit = editor_make_ctile_type_edit(
                             ctile_cursor, Collision_Tile_Type::SOLID, room)
                             .ctile_edit;

    auto check_ctile = [](Tile_Cursor where, int x_off, int y_off,
                          Room *room) -> Bool {
        Ctile_Cursor ctile = {};
        ctile.where = where;
        ctile.where.x += x_off;
        ctile.where.y += y_off;
        auto collision_tile = ctile_cursor_to_ctile_ptr(ctile, room);
        return check_collided(*collision_tile);
    };
    auto push = [&](Gtile_Cursor where, u8 index) {
        a_edit->gtile_edits.push(
            editor_make_gtile_index_edit(where, index, room).gtile_edit);
    };
    // rock
    for (int j = -1; j <= 1; j += 1) {
        for (int i = -1; i <= 1; i += 1) {
            Gtile_Cursor gtile_cursor = {};
            gtile_cursor.where = auto_mode_cursor;
            gtile_cursor.where.x += i;
            gtile_cursor.where.y += j;
            gtile_cursor.layer = room->first_fg_layer;

            Bool ctiles[3][3] = {};
            for (int ctile_j = -1; ctile_j <= 1; ctile_j += 1) {
                for (int ctile_i = -1; ctile_i <= 1; ctile_i += 1) {
                    ctiles[ctile_i + 1][ctile_j + 1] =
                        check_ctile(gtile_cursor.where, ctile_i, ctile_j, room);
                }
            }

            u8 gtile_index = 0x00;
            if (ctiles[1][1]) {
                gtile_index = 0x11;
                if (ctiles[1][0]) {
                    if (ctiles[1][2]) {
                        gtile_index += 0x00;
                    } else {
                        gtile_index += 0x10;
                    }
                } else {
                    if (ctiles[1][2]) {
                        gtile_index -= 0x10;
                    } else {
                        gtile_index += 0x20;
                    }
                }
                if (ctiles[0][1]) {
                    if (ctiles[2][1]) {
                        gtile_index += 0x00;
                    } else {
                        gtile_index += 0x01;
                    }
                } else {
                    if (ctiles[2][1]) {
                        gtile_index -= 0x01;
                    } else {
                        gtile_index += 0x02;
                    }
                }
            } else {
                gtile_index = 0x19;
                if (ctiles[1][0]) {
                    if (ctiles[1][2]) {
                        gtile_index += 0x00;
                    } else {
                        gtile_index -= 0x10;
                    }
                } else {
                }
                if (ctiles[0][1]) {
                    if (ctiles[2][1]) {
                        gtile_index += 0x02;
                    } else {
                        gtile_index -= 0x01;
                    }
                } else {
                    if (ctiles[2][1]) {
                        gtile_index += 0x01;
                    } else {
                        gtile_index += 0x00;
                    }
                }
                if (!(ctiles[1][0] || ctiles[0][1] || ctiles[2][1])) {
                    gtile_index = 0xFF;
                }
                Bool right_of_cap =
                    (ctiles[0][1] && !ctiles[0][0]) && (!ctiles[2][1]);
                Bool left_of_cap =
                    (ctiles[2][1] && !ctiles[2][0]) && (!ctiles[0][1]);
                if (right_of_cap || left_of_cap) {
                    gtile_index = 0xFF;
                }
            }
            push(gtile_cursor, gtile_index);
        }
    }
    // grass
    for (int j = -1; j <= 1; j += 1) {
        for (int i = -1; i <= 1; i += 1) {
            Gtile_Cursor gtile_cursor = {};
            gtile_cursor.where = auto_mode_cursor;
            gtile_cursor.where.x += i;
            gtile_cursor.where.y += j;
            gtile_cursor.layer = room->first_fg_layer + 1;

            Bool c_center = check_ctile(gtile_cursor.where, 0, 0, room);
            if (!c_center) {
                continue;
            }

            Bool c_above = check_ctile(gtile_cursor.where, 0, -1, room);
            // Bool c_below = check_ctile(gtile_cursor.where, 0, 1, room);
            Bool c_left = check_ctile(gtile_cursor.where, -1, 0, room);
            Bool c_right = check_ctile(gtile_cursor.where, 1, 0, room);
            {
                u8 gtile_index = 0x0D;
                if (c_above) {
                    gtile_index = 0xFF;

                } else {
                    if (c_left) {
                        if (c_right) {
                            gtile_index += 0x00;
                        } else {
                            gtile_index += 0x01;
                        }
                    } else {
                        if (c_right) {
                            gtile_index -= 0x01;
                        } else {
                            gtile_index += 0x02;
                        }
                    }
                }
                push(gtile_cursor, gtile_index);
            }
        }
    }
    return edit;
}
static Bool gtile_edit_does_nothing(Gtile_Edit *e) {
    return e->before.index == e->after.index;
}
static Bool
gtile_edit_results_equal(Gtile_Edit *a,
                         Gtile_Edit *b) { // @Cleanup: struct equality
    return a->where.where.x == b->where.where.x &&
           a->where.where.y == b->where.where.y &&
           a->after.index == b->after.index;
}
static Bool ctile_edit_does_nothing(Ctile_Edit *e) {
    return e->before.type == e->after.type;
}
static Bool etile_edit_does_nothing(Etile_Edit *e) {
    return e->before == e->after;
}
static Bool
ctile_edit_results_equal(Ctile_Edit *a,
                         Ctile_Edit *b) { // @Cleanup: struct equality
    return a->where.where.x == b->where.where.x &&
           a->where.where.y == b->where.where.y &&
           a->after.type == b->after.type;
}
static Bool
etile_edit_results_equal(Etile_Edit *a,
                         Etile_Edit *b) { // @Cleanup: struct equality
    return a->where.x == b->where.x && a->where.y == b->where.y &&
           a->after == b->after;
}
static Bool auto_edit_does_nothing(Auto_Mode_Edit *e) {
    if (!ctile_edit_does_nothing(&e->ctile_edit)) {
        return False;
    }
    ng_for(e->gtile_edits) {
        if (!gtile_edit_does_nothing(&it)) {
            return False;
        }
    }
    return True;
}
static Bool auto_edit_results_equal(Auto_Mode_Edit *a, Auto_Mode_Edit *b) {
    if (!ctile_edit_results_equal(&a->ctile_edit, &b->ctile_edit)) {
        return False;
    }
    // @Todo @Incomplete: How to handle the gtile edits?
    return True;
}
static Bool edit_does_nothing(Edit *e) { // @Cleanup: boilerplate
    switch (e->type) {
    default: { return False; } break;
    case (Edit_Type::GTILE_EDIT): {
        return gtile_edit_does_nothing(&e->gtile_edit);
    } break;
    case (Edit_Type::CTILE_EDIT): {
        return ctile_edit_does_nothing(&e->ctile_edit);
    } break;
    case (Edit_Type::AUTO_MODE_EDIT): {
        return auto_edit_does_nothing(&e->auto_mode_edit);
    } break;
    case (Edit_Type::ETILE_EDIT): {
        return etile_edit_does_nothing(&e->etile_edit);
    } break;
    }
}
static Bool edit_results_equal(Edit *a, Edit *b) { // @Cleanup: boilerplate
    if (a->type != b->type) {
        return False;
    }
    switch (a->type) {
    default: { ng_assert(False); } break;
    case (Edit_Type::GTILE_EDIT): {
        return gtile_edit_results_equal(&a->gtile_edit, &b->gtile_edit);
    } break;
    case (Edit_Type::CTILE_EDIT): {
        return ctile_edit_results_equal(&a->ctile_edit, &b->ctile_edit);
    } break;
    case (Edit_Type::AUTO_MODE_EDIT): {
        return auto_edit_results_equal(&a->auto_mode_edit, &b->auto_mode_edit);
    } break;
    case (Edit_Type::ETILE_EDIT): {
        return etile_edit_results_equal(&a->etile_edit, &b->etile_edit);
    } break;
    }
    return False;
}
static Bool editor_paste_is_pointless(Edit *most_recent_edit, Edit *new_edit) {
    return edit_results_equal(most_recent_edit, new_edit) ||
           edit_does_nothing(new_edit);
};
static void editor_gtem_enter(Editor_State *editor, Room *room) {
    auto tileset = &room->layers[editor->g_mode_cursor.layer].tileset;
    auto gtile = gtile_cursor_to_gtile_ptr(editor->g_mode_cursor, room);
    editor->gtem_tileset_selection =
        gtile_index_to_tileset_cursor(gtile->index, tileset);
    editor->in_gtem = True;
    ui->nav_bloop.play(); // @Todo: take ui as parameter
};
static void editor_gtem_accept(Editor_State *editor, Room *room) {
    auto tileset = &room->layers[editor->g_mode_cursor.layer].tileset;
    auto new_edit = editor_make_gtile_index_edit(
        editor->g_mode_cursor,
        tileset_cursor_to_gtile_index(editor->gtem_tileset_selection, tileset),
        room);
    if (!gtile_edit_does_nothing(&new_edit.gtile_edit)) {
        editor_push_and_apply_edit(&new_edit, room, editor);
        ui->action_bloop.play();
    }
    editor->in_gtem = False;
};
static void editor_gtem_cancel(Editor_State *editor) {
    editor->gtem_tileset_selection = {};
    editor->in_gtem = False;
    ui->nav_bloop.play();
};
static void editor_ctem_enter(Editor_State *editor, Room *room) {
    auto ctile = ctile_cursor_to_ctile_ptr(editor->c_mode_cursor, room);
    editor->ctem_type_selection = ctile->type;
    editor->in_ctem = True;
    ui->nav_bloop.play(); // @Todo: take ui as parameter
};
static void editor_ctem_accept(Editor_State *editor, Room *room) {
    auto new_edit = editor_make_ctile_type_edit(
        editor->c_mode_cursor, editor->ctem_type_selection, room);
    if (!ctile_edit_does_nothing(&new_edit.ctile_edit)) {
        editor_push_and_apply_edit(&new_edit, room, editor);
        ui->action_bloop.play();
    }
    editor->in_ctem = False;
};
static void editor_ctem_cancel(Editor_State *editor) {
    editor->ctem_type_selection = {};
    editor->in_ctem = False;
    ui->nav_bloop.play();
};
static void editor_etem_enter(Editor_State *editor, Room *room) {
    auto etile = etile_cursor_to_etile_ptr(editor->e_mode_cursor, room);
    editor->etem_type_selection = *etile;
    editor->in_etem = True;
    ui->nav_bloop.play(); // @Todo: take ui as parameter
};
static void editor_etem_accept(Editor_State *editor, Room *room) {
    auto new_edit = editor_make_etile_type_edit(
        editor->e_mode_cursor, editor->etem_type_selection, room);
    if (!etile_edit_does_nothing(&new_edit.etile_edit)) {
        editor_push_and_apply_edit(&new_edit, room, editor);
        ui->action_bloop.play();
    }
    editor->in_etem = False;
};
static void editor_etem_cancel(Editor_State *editor) {
    editor->etem_type_selection = {};
    editor->in_etem = False;
    ui->nav_bloop.play();
};

int signed_mod(int x, int divisor) {
    // x %= divisor;
    // if (x < 0) {
    //     x += divisor;
    // }
    // return x;

    // @Speed: C++ negative mod is garbage, screw it, just loop.
    while (x < 0) {
        x += divisor;
    }
    while (x >= divisor) {
        x -= divisor;
    }
    return x;
}

static void editor_clamp_variables() {
    auto room = world->get_room(editor.location);
    // @Todo: kind of a @Hack, why must this function exist? Can the range
    // checking not be done at the appropriate places?
    editor.g_mode_cursor.layer =
        signed_mod(editor.g_mode_cursor.layer, room->numLayers());
    if (editor.in_gtem) {
        auto tileset = &room->layers[editor.g_mode_cursor.layer].tileset;
        editor.gtem_tileset_selection.x =
            signed_mod(editor.gtem_tileset_selection.x, tileset->w);
        editor.gtem_tileset_selection.y =
            signed_mod(editor.gtem_tileset_selection.y, tileset->h);
    }
    // ctem doesn't need a modulo because it's modifying a byte, which is right.
    if (editor.in_etem) {
        auto num_enemy_types = cast(int) Enemy_Type::NUM_ENEMY_TYPES;
        auto bounded_type =
            signed_mod(cast(int) editor.etem_type_selection, num_enemy_types);
        editor.etem_type_selection = cast(Enemy_Type) bounded_type;
    }
}
void editor_update_layers(Editor_State *editor, Room *room) {
    editor->layer_hiddens.resize(room->layers.count);
    editor->layer_solos.resize(room->layers.count);
}
void toggle_between_engine_and_editor() {
    if (engine_state.mode == Engine_Mode::IN_ENGINE) {
        if (!editor.initialized) {
            editor = editor_create();
            editor.location = player->location;
            auto room = world->get_room(editor.location);
            editor.g_mode_cursor.layer = room->firstForegroundLayer();
            // editor.view_pos = player->pos;
            editor.view_pos = camera->pos;
        }
        engine_state.mode = Engine_Mode::IN_EDITOR;
        ui->action_bloop.retrigger = False;
        editor_update_layers(&editor, world->get_room(editor.location));
    } else if (engine_state.mode == Engine_Mode::IN_EDITOR) {
        engine_state.mode = Engine_Mode::IN_ENGINE;
        ui->action_bloop.retrigger = True;
    }
}
static SDL_Point mouse_pos = {};
enum struct Click_State { UP, DOWN, PRESSED, RELEASED };
static Click_State lmb = Click_State::UP;
static Click_State rmb = Click_State::UP;
Bool editor_gui_rect(SDL_Rect rect,
                     SDL_Color render_color = {32, 32, 32, 128}) {
    Bool hover = (SDL_PointInRect(&mouse_pos, &rect) == SDL_TRUE);
    Bool click = (lmb == Click_State::RELEASED);

    if (hover) {
        if (lmb == Click_State::DOWN) {
            render_color.r /= 2;
            render_color.g /= 2;
            render_color.b /= 2;
        } else if (lmb == Click_State::UP) {
            render_color.r *= 2;
            render_color.g *= 2;
            render_color.b *= 2;
        }
    }
    editor_draw_rect(rect, render_color);
    return hover && click;
}
static void editor_set_current_input(Editor_State *editor,
                                     Editor_Input_Box *input) {
    editor->current_text_input = input;
    SDL_StartTextInput();
    editor_ctem_cancel(editor);
    editor_gtem_cancel(editor);
    // SDL_SetTextInputRect(&editor->current_text_input->rect);
    auto string = editor->current_text_input->get_string(editor);
    defer {
        if (string.should_free) {
            ng::free_string(&string.string);
        }
    };
    editor->current_text_input_buffer.resize(string.string.len);
    ng::memcpy(editor->current_text_input_buffer.data, string.string.ptr,
               string.string.len);
}
static Editor_Temp_String
editor_get_string_from_input(Editor_State *editor,
                             Editor_Input_Box *which_input) {
    return which_input->get_string(editor);
}

void editor_handle_sdl_mouse_button_event(SDL_MouseButtonEvent event) {
    Click_State *s = null;
    if (event.button == SDL_BUTTON_LEFT) {
        s = &lmb;
    } else if (event.button == SDL_BUTTON_RIGHT) {
        s = &rmb;
    } else {
        ng_assert(False); // @Incomplete
    }
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        *s = Click_State::PRESSED;
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        *s = Click_State::RELEASED;
    } else {
        ng_assert(False);
    }
}
void editor_handle_sdl_mouse_wheel_event(SDL_MouseWheelEvent event) {
    USE event;
    // if (engine_state.mode == Engine_Mode::IN_EDITOR) {
    //     renderer->c_scale += event.y / 10.f;
    //     SDL_RenderSetScale(renderer->c_renderer, renderer->c_scale,
    //                        renderer->c_scale);
    // }
}
void editor_handle_sdl_text_input_event(SDL_TextInputEvent event) {
    ng_for(event.text) {
        if (it == '\0') {
            break;
        }
        editor.current_text_input_buffer.push(it);
    }
}
void editor_handle_sdl_text_editing_event(SDL_TextEditingEvent event) {
    ng::print(event.text);
    ng_for(event.text) {
        // if (it == '\0') {
        //     break;
        // }
        // editor.current_text_input_buffer.push(it);
        editor.current_text_input_buffer[&it - event.text] = it;
    }
}
void editor_handle_input(Input_State *in) { // editor handle input
    if (in->keys_new[SDL_GetScancodeFromKey(SDLK_F8)]) {
        toggle_between_engine_and_editor();
        return;
    } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_F7)]) {
        if (editor.initialized) { // might be called before entering editor
            editor.render_editor_mode_even_when_in_engine =
                !editor.render_editor_mode_even_when_in_engine;
        }
    }
    if (!editor.initialized) {
        return;
    }
    if (engine_state.mode != Engine_Mode::IN_EDITOR) {
        return;
    }
    auto room = world->get_room(editor.location);
    // keyboard
    Bool control_pressed = (in->keys[KEY(LCTRL)] || in->keys[KEY(RCTRL)]);
    if (SDL_IsTextInputActive()) {
        if (in->keys_rep[SDL_GetScancodeFromKey(SDLK_BACKSPACE)]) {
            if (editor.current_text_input_buffer.count > 0) {
                editor.current_text_input_buffer.pop();
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_RETURN)]) {
            editor.current_text_input->complete(&editor);
            SDL_StopTextInput();
        }
    } else {
        if (in->keys_new[SDL_GetScancodeFromKey(SDLK_1)]) {
            editor_gtem_cancel(&editor);
            editor_ctem_cancel(&editor);
            editor_etem_cancel(&editor);
            editor.current_mode = Editor_Mode::GRAPHICAL_MODE;
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_2)]) {
            editor_gtem_cancel(&editor);
            editor_ctem_cancel(&editor);
            editor_etem_cancel(&editor);
            editor.current_mode = Editor_Mode::COLLISION_MODE;
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_3)]) {
            editor_gtem_cancel(&editor);
            editor_ctem_cancel(&editor);
            editor_etem_cancel(&editor);
            editor.current_mode = Editor_Mode::AUTOMATIC_MODE;
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_4)]) {
            editor_gtem_cancel(&editor);
            editor_ctem_cancel(&editor);
            editor_etem_cancel(&editor);
            editor.current_mode = Editor_Mode::ENEMY_MODE;
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_RETURN)]) {
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_ESCAPE)]) {
            if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (editor.in_gtem) {
                    editor_gtem_cancel(&editor);
                }
            } else if (editor.current_mode == Editor_Mode::COLLISION_MODE) {
                if (editor.in_ctem) {
                    editor_ctem_cancel(&editor);
                }
            } else if (editor.current_mode == Editor_Mode::ENEMY_MODE) {
                if (editor.in_etem) {
                    editor_etem_cancel(&editor);
                }
            } else {
                // ng_assert(False);
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_PAGEUP)]) {
            if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (!editor.in_gtem) {
                    editor.g_mode_cursor.layer += 1;
                    // @Todo @Robustness: event queue rather than calling
                    // functions randomly
                    editor_clamp_variables();
                }
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_PAGEDOWN)]) {
            if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (!editor.in_gtem) {
                    editor.g_mode_cursor.layer -= 1;
                    editor_clamp_variables();
                }
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_EQUALS)]) {
            audio->global_volume += 0.1f;
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_MINUS)]) {
            audio->global_volume -= 0.1f;
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_HOME)]) {
            if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                auto hidden = &editor.layer_hiddens[editor.g_mode_cursor.layer];
                *hidden = !*hidden;
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_END)]) {
            if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                auto solo = &editor.layer_solos[editor.g_mode_cursor.layer];
                *solo = !*solo;
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_RIGHT)]) {
            if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (editor.in_gtem) {
                    editor.gtem_tileset_selection.x += 1;
                    editor_clamp_variables();
                    ui->nav_bloop.play();
                }
            }
            if (editor.current_mode == Editor_Mode::COLLISION_MODE) {
                if (editor.in_ctem) {
                    editor.ctem_type_selection = cast(Collision_Tile_Type)(
                        cast(int) editor.ctem_type_selection + 16);
                    editor_clamp_variables();
                    ui->nav_bloop.play();
                }
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_LEFT)]) {
            if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (editor.in_gtem) {
                    editor.gtem_tileset_selection.x -= 1;
                    editor_clamp_variables();
                    ui->nav_bloop.play();
                }
            }
            if (editor.current_mode == Editor_Mode::COLLISION_MODE) {
                if (editor.in_ctem) {
                    editor.ctem_type_selection = cast(Collision_Tile_Type)(
                        cast(int) editor.ctem_type_selection - 16);
                    editor_clamp_variables();
                    ui->nav_bloop.play();
                }
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_DOWN)]) {
            if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (editor.in_gtem) {
                    editor.gtem_tileset_selection.y += 1;
                    editor_clamp_variables();
                    ui->nav_bloop.play();
                }
            }
            if (editor.current_mode == Editor_Mode::COLLISION_MODE) {
                if (editor.in_ctem) {
                    editor.ctem_type_selection = cast(Collision_Tile_Type)(
                        cast(int) editor.ctem_type_selection - 1);
                    editor_clamp_variables();
                    ui->nav_bloop.play();
                }
            }
            if (editor.current_mode == Editor_Mode::ENEMY_MODE) {
                if (editor.in_etem) {
                    editor.etem_type_selection = cast(Enemy_Type)(
                        cast(int) editor.etem_type_selection - 1);
                    editor_clamp_variables();
                    ui->nav_bloop.play();
                }
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_UP)]) {
            if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (editor.in_gtem) {
                    editor.gtem_tileset_selection.y -= 1;
                    editor_clamp_variables();
                    ui->nav_bloop.play();
                }
            }
            if (editor.current_mode == Editor_Mode::COLLISION_MODE) {
                if (editor.in_ctem) {
                    editor.ctem_type_selection = cast(Collision_Tile_Type)(
                        cast(int) editor.ctem_type_selection + 1);
                    editor_clamp_variables();
                    ui->nav_bloop.play();
                }
            }
            if (editor.current_mode == Editor_Mode::ENEMY_MODE) {
                if (editor.in_etem) {
                    editor.etem_type_selection = cast(Enemy_Type)(
                        cast(int) editor.etem_type_selection + 1);
                    editor_clamp_variables();
                    ui->nav_bloop.play();
                }
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_z)]) {
            if (control_pressed) {
                if (editor.edit_history.history_size > 0) {
                    ui->nav_bloop.play();
                }
                if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                    if (editor.in_gtem) {
                        editor_gtem_cancel(&editor);
                    }
                }
                if (editor.current_mode == Editor_Mode::COLLISION_MODE) {
                    if (editor.in_ctem) {
                        editor_ctem_cancel(&editor);
                    }
                }
                editor_undo(&editor, room);
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_y)]) {
            if (control_pressed) {
                editor_redo(&editor, room);
            }
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_x)]) {
        } else if (in->keys_new[SDL_GetScancodeFromKey(SDLK_s)]) {
            if (control_pressed) { // @Temporary @Cleanup
                RW rw(room->filename, "wb");
                SaveRoom(&rw, room);
                *world_sbs->get_room(editor.location) = room_copy(room);
                ui->action_bloop.play();
            }
        }
        auto tileset = &room->layers[editor.g_mode_cursor.layer].tileset;
        if (editor.in_bounds) {
            if (in->keys[KEY(LCTRL)] || in->keys[KEY(RCTRL)]) {
                if (in->keys[SDL_GetScancodeFromKey(SDLK_c)]) {
                    if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                        if (!editor.in_gtem) { // copy into clipboard
                            auto gtile = gtile_cursor_to_gtile_ptr(
                                editor.g_mode_cursor, room);
                            auto old_cursor = editor.g_mode_clipboard;
                            editor.g_mode_clipboard =
                                gtile_index_to_tileset_cursor(gtile->index,
                                                              tileset);
                            if (!editor.g_mode_clipboard_valid ||
                                old_cursor.x != editor.g_mode_clipboard.x ||
                                old_cursor.y != editor.g_mode_clipboard.y) {
                                // clipboard was empty or changed
                                ui->action_bloop.play();
                            }
                            editor.g_mode_clipboard_valid = True;
                        }
                    }
                    if (editor.current_mode == Editor_Mode::COLLISION_MODE) {
                        if (!editor.in_ctem) { // copy into clipboard
                            auto ctile = ctile_cursor_to_ctile_ptr(
                                editor.c_mode_cursor, room);
                            auto type = editor.c_mode_clipboard;
                            editor.c_mode_clipboard = ctile->type;
                            if (!editor.c_mode_clipboard_valid ||
                                type != editor.c_mode_clipboard) {
                                // clipboard was empty or changed
                                ui->action_bloop.play();
                            }
                            editor.c_mode_clipboard_valid = True;
                        }
                    }
                } else if (in->keys[SDL_GetScancodeFromKey(
                               SDLK_v)]) { // paste from clipboard
                    auto peek_history = [&] {
                        return editor.edit_history
                            .edits[editor.edit_history.history_size - 1];
                    };
                    Edit most_recent_edit = {};
                    if (editor.edit_history.history_size > 0) {
                        most_recent_edit = peek_history();
                    }
                    Edit new_edit = {};
                    Bool is_valid_paste = False;
                    if (editor.current_mode == Editor_Mode::GRAPHICAL_MODE) {
                        is_valid_paste =
                            !editor.in_gtem && editor.g_mode_clipboard_valid;
                        if (is_valid_paste) {
                            new_edit = editor_make_gtile_index_edit(
                                editor.g_mode_cursor,
                                tileset_cursor_to_gtile_index(
                                    editor.g_mode_clipboard, tileset),
                                room);
                        }
                    }
                    if (editor.current_mode == Editor_Mode::COLLISION_MODE) {
                        is_valid_paste =
                            !editor.in_ctem && editor.c_mode_clipboard_valid;
                        if (is_valid_paste) {
                            new_edit = editor_make_ctile_type_edit(
                                editor.c_mode_cursor, editor.c_mode_clipboard,
                                room);
                        }
                    }
                    if (is_valid_paste && !editor_paste_is_pointless(
                                              &most_recent_edit, &new_edit)) {
                        editor_push_and_apply_edit(&new_edit, room, &editor);
                        ui->action_bloop.play();
                    }
                }
            }
        }
    }

    // mouse
    SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);

    if (editor.current_mode == Editor_Mode::AUTOMATIC_MODE) {
        if (lmb == Click_State::DOWN) {
            auto edit = editor_make_auto_mode_edit(
                Auto_Mode_Edit_Type::SOLID, room, editor.auto_mode_cursor);
            if (!edit_does_nothing(&edit)) {
                editor_push_and_apply_edit(&edit, room, &editor);
            }
        } else if (rmb == Click_State::DOWN) {
            auto edit = editor_make_auto_mode_edit(
                Auto_Mode_Edit_Type::AIR, room, editor.auto_mode_cursor);
            if (!edit_does_nothing(&edit)) {
                editor_push_and_apply_edit(&edit, room, &editor);
            }
        }
    }

    [](Editor_State *editor, World *world) {
        auto room = world->get_room(editor->location);
        ng_assert(room);
        auto clicked = False;
        // Check text input rects.
        ng_for(editor_inputs) {
            if (editor_gui_rect(it->input_rect)) {
                clicked = True;
                auto string = editor_get_string_from_input(editor, it);
                defer {
                    if (string.should_free) {
                        ng::free_string(&string.string);
                    }
                };
                editor_set_current_input(editor, it);
            }
        }
        if (!clicked && lmb == Click_State::RELEASED) {
            if (SDL_IsTextInputActive()) { // We left all text input rects.
                SDL_StopTextInput();
                editor->current_text_input_buffer.count = 0;
                editor->current_text_input = null;
            } else if (editor->current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (editor->in_gtem) {
                    editor_gtem_accept(editor, room);
                } else {
                    if (editor->in_bounds) {
                        editor_gtem_enter(editor, room);
                    }
                }
            } else if (editor->current_mode == Editor_Mode::COLLISION_MODE) {
                if (editor->in_ctem) {
                    editor_ctem_accept(editor, room);
                } else {
                    if (editor->in_bounds) {
                        editor_ctem_enter(editor, room);
                    }
                }
            } else if (editor->current_mode == Editor_Mode::AUTOMATIC_MODE) {
                if (editor->in_bounds) {
                }
            } else if (editor->current_mode == Editor_Mode::ENEMY_MODE) {
                if (editor->in_etem) {
                    editor_etem_accept(editor, room);
                } else {
                    if (editor->in_bounds) {
                        editor_etem_enter(editor, room);
                    }
                }
            }
        }
    }(&editor, world);

    // Clicks are no longer new, so clear them.
    if (lmb == Click_State::PRESSED) {
        lmb = Click_State::DOWN;
    }
    if (lmb == Click_State::RELEASED) {
        lmb = Click_State::UP;
    }
}

void editor_draw_rect_scaled(SDL_Rect r, SDL_Color c) {
    r.x *= ::renderer->c_scale;
    r.y *= ::renderer->c_scale;
    r.w *= ::renderer->c_scale;
    r.h *= ::renderer->c_scale; // @Cleanup @Global
    editor_draw_rect(r, c);
}
void sdl_renderfillrect_scaled(SDL_Renderer *renderer, SDL_Rect r) {
    // float x_scale = 0.0f;
    // float y_scale = 0.0f;
    // SDL_RenderGetScale(renderer, &x_scale, &y_scale);
    // SDL_RenderSetScale(renderer, ::renderer->c_scale, ::renderer->c_scale);
    r.x *= ::renderer->c_scale;
    r.y *= ::renderer->c_scale;
    r.w *= ::renderer->c_scale;
    r.h *= ::renderer->c_scale; // @Cleanup @Global
    SDL_RenderFillRect(renderer, &r);
    // SDL_RenderSetScale(renderer, x_scale, y_scale);
}
void sdl_renderdrawrect_scaled(SDL_Renderer *renderer, SDL_Rect r) {
    // float x_scale = 0.0f;
    // float y_scale = 0.0f;
    // SDL_RenderGetScale(renderer, &x_scale, &y_scale);
    // SDL_RenderSetScale(renderer, ::renderer->c_scale, ::renderer->c_scale);
    r.x *= ::renderer->c_scale;
    r.y *= ::renderer->c_scale;
    r.w *= ::renderer->c_scale;
    r.h *= ::renderer->c_scale; // @Cleanup @Global
    SDL_RenderDrawRect(renderer, &r);
    // SDL_RenderSetScale(renderer, x_scale, y_scale);
}

void editor_draw_rect(SDL_Rect r, SDL_Color c) {
    editor.rects_to_draw.push({r, c});
}

void editor_render_text(ng::string str, Vector2 pos,
                        Horizontal_Align align) { // @FixMe: expensive
    debug_draw_text(str, pos, align);
}
void editor_update(Editor_State *editor, f64 dt, Input_State *in) {
    // editor update
    auto room = world->get_room(editor->location);
    if (SDL_IsTextInputActive()) {
        return;
    }
    if (!editor->in_gtem && !editor->in_ctem && !editor->in_etem) {
        auto input = input_get_analog_vector(in);
        input.x += cast(float)(in->keys[KEY(D)] - in->keys[KEY(A)]);
        input.y += cast(float)(in->keys[KEY(S)] - in->keys[KEY(W)]);
        if (sign(input.x) != sign(editor->view_vel.x)) {
            editor->view_vel.x = 0;
        }
        if (sign(input.y) != sign(editor->view_vel.y)) {
            editor->view_vel.y = 0;
        }
        auto accel = input * tile_to_game(64_t);
        editor->view_vel += accel * dt;
        if (editor->view_vel.mag() > tile_to_game(64_t)) { // clamp speed
            editor->view_vel /= editor->view_vel.mag();
            editor->view_vel *= tile_to_game(64_t);
        }
        editor->view_pos += editor->view_vel * dt;

        Vector2 mouse_screen_pos = {};
        mouse_screen_pos.x = mouse_pos.x / renderer->c_scale;
        mouse_screen_pos.y = mouse_pos.y / renderer->c_scale;

        Vector2 mouse_world_pos = {};
        {
            mouse_world_pos = (editor->view_pos - globals.screen_center);
            if (editor->current_mode == Editor_Mode::GRAPHICAL_MODE) {
                auto parallax =
                    room->layers[editor->g_mode_cursor.layer].parallax / 255.f;
                mouse_world_pos *= parallax;
            }
        }
        auto mouse_pos = mouse_world_pos + mouse_screen_pos;

        Tile_Cursor mouse_loc = {};
        mouse_loc.x = game_to_tile(mouse_pos.x);
        mouse_loc.y = game_to_tile(mouse_pos.y);

        editor->in_bounds = (mouse_loc.x >= 0_t && mouse_loc.x < room->w &&
                             mouse_loc.y >= 0_t && mouse_loc.y < room->h);
        if (editor->current_mode == Editor_Mode::GRAPHICAL_MODE) {
            editor->g_mode_cursor.where = mouse_loc;
        } else if (editor->current_mode == Editor_Mode::COLLISION_MODE) {
            editor->c_mode_cursor.where = mouse_loc;
        } else if (editor->current_mode == Editor_Mode::AUTOMATIC_MODE) {
            editor->auto_mode_cursor = mouse_loc;
        } else if (editor->current_mode == Editor_Mode::ENEMY_MODE) {
            editor->e_mode_cursor = mouse_loc;
        } else {
            ng_assert(False);
        }
    }

    editor_clamp_variables();
    if (!editor->in_bounds) {
        if (editor->current_mode == Editor_Mode::GRAPHICAL_MODE) {
            ng_assert(!editor->in_gtem);
        } else if (editor->current_mode == Editor_Mode::COLLISION_MODE) {
            ng_assert(!editor->in_ctem);
        } else if (editor->current_mode == Editor_Mode::ENEMY_MODE) {
            ng_assert(!editor->in_etem);
        }
    }
    if (in->keys[SDL_SCANCODE_LSHIFT] || in->keys[SDL_SCANCODE_RSHIFT]) {
        // carry the player around with you too
        player->pos = editor->view_pos;
        player->location = editor->location;
    }
    auto old_target = camera->target;
    auto old_smooth = camera->smooth;
    camera->target = {&editor->view_pos, &editor->location};
    camera->smooth = False;
    camera->update(0.0f, False);
    camera->target = old_target;
    camera->smooth = old_smooth;
}
using ng::operator""_s;
void editor_render_world(Editor_State *editor, Room *room) {
    Bool any_solos = False;
    ng_for(editor->layer_solos) {
        if (it) {
            any_solos = True;
        }
    }
    auto render_layer = [](Editor_State *editor, Room *room,
                           Room::layer_index i, Bool any_solos) {
        if (i >= editor->layer_solos.count ||
            i >= editor->layer_hiddens.count) {
            return;
        }
        if (any_solos && !editor->layer_solos[i]) {
            return;
        }
        if (editor->layer_hiddens[i]) {
            return;
        }
        room->RenderLayer(i);
    };
    {
        for (auto i = room->getBegin(Room::LayerMode::Background);
             i < room->getEnd(Room::LayerMode::Background); i += 1) {
            render_layer(editor, room, i, any_solos);
        }
        for (auto i = room->getBegin(Room::LayerMode::Foreground);
             i < room->getEnd(Room::LayerMode::Foreground); i += 1) {
            render_layer(editor, room, i, any_solos);
        }
        ui_render_animated_tiles(ui, room);
    }
}
template <class... Args>
void editor_print(Vector2 pos, Horizontal_Align align, const char *fmt,
                  const Args &... args) {
    auto buf = ng::mprint(fmt, args...);
    defer { ng::free(buf.str); };
    editor_render_text(
        {buf}, pos * renderer->c_scale,
        align); // @Todo @Global @Remove: get rid of the scaling factor here
}
void editor_render(Editor_State *editor, f64 t) { // editor render
    Profile();

    auto get_collision_tile_color = [](Collision_Tile ctile) {
        u8 r = 0, g = 0, b = 0, a = 0;
        switch (ctile.type) {
        default: {
            if (ctile.is_exit()) {
                r = 64, g = 192, b = 64;
                a = 64;
            }
        } break;
        case (Collision_Tile_Type::AIR): {
            r = 255, g = 255, b = 192;
            a = 32;
        } break;
        case (Collision_Tile_Type::SOLID): {
            r = 64, g = 64, b = 96;
            a = 96;
        } break;
        case (Collision_Tile_Type::BREAKABLE): {
            r = 196, g = 128, b = 0;
            a = 96;
        } break;
        case (Collision_Tile_Type::BREAKABLE_HEART): {
            r = 255, g = 64, b = 64;
            a = 96;
        } break;
        case (Collision_Tile_Type::SAVE_SPOT): {
            r = 255, g = 255, b = 255;
            a = 192;
        }
        }
        return SDL_Color{r, g, b, a};
    };
    auto scale_u8 = [](u8 a, f32 s) {
        return cast(u8) ng::clamp(a * s, 0.0f, 255.0f);
    };
    auto room = world->get_room(camera->location);
    if (editor->current_mode == Editor_Mode::COLLISION_MODE ||
        editor->current_mode ==
            Editor_Mode::AUTOMATIC_MODE) { // render collision tiles
        auto origin_rel_to_cam = camera->game_to_local({0, 0});
        auto wr = camera->world_rect();
        s16 min_x = game_to_tile(wr.l());
        s16 max_x = game_to_tile(wr.r()) + 1_t;
        s16 min_y = game_to_tile(wr.t());
        s16 max_y = game_to_tile(wr.b()) + 1_t;
        min_x = ng::clamp(min_x, 0_t, room->w);
        max_x = ng::clamp(max_x, 0_t, room->w);
        min_y = ng::clamp(min_y, 0_t, room->h);
        max_y = ng::clamp(max_y, 0_t, room->h);
        // s16 min_x = 0_t;
        // s16 max_x = room->w;
        // s16 min_y = 0_t;
        // s16 max_y = room->h;

        Profile("print ctiles");
        for (auto y = min_y; y < max_y; y += 1) {
            for (auto x = min_x; x < max_x; x += 1) {
                auto ctile = room->at(x, y);
                Bool skip_rect_draw = False;
                if (editor->current_mode == Editor_Mode::COLLISION_MODE &&
                    editor->in_ctem) {
                    if (x == editor->c_mode_cursor.where.x &&
                        y == editor->c_mode_cursor.where.y) {
                        skip_rect_draw = True;
                        ctile.type = editor->ctem_type_selection;
                    }
                }
                auto color = get_collision_tile_color(ctile);
                auto r = color.r;
                auto g = color.g;
                auto b = color.b;
                auto a = color.a;
                Vector2 tile_pos = {origin_rel_to_cam.x + tile_to_game(x),
                                    origin_rel_to_cam.y + tile_to_game(y)};
                if (editor->current_mode == Editor_Mode::COLLISION_MODE) {
                    editor_print(
                        {tile_pos.x + TILESIZE / 2, tile_pos.y + TILESIZE / 2},
                        Horizontal_Align::CENTER, "%0",
                        cast(int) ctile.type); // @Speed
                }
                if (!skip_rect_draw) {
                    SDL_Rect rect = {};
                    rect.x = game_to_px(tile_pos.x) + 1;
                    rect.y = game_to_px(tile_pos.y) + 1;
                    rect.w = tile_to_px(1_t) - 2;
                    rect.h = tile_to_px(1_t) - 2;

                    SDL_SetRenderDrawColor(renderer->c_renderer, r, g, b, a);
                    sdl_renderdrawrect_scaled(renderer->c_renderer, rect);
                    rect.x += 1;
                    rect.w -= 2;
                    rect.y += 1;
                    rect.h -= 2;
                    SDL_SetRenderDrawColor(renderer->c_renderer, r, g, b,
                                           a / 2);
                    sdl_renderdrawrect_scaled(renderer->c_renderer, rect);
                }
            }
        }
    }
    if (editor->current_mode == Editor_Mode::ENEMY_MODE) { // render enemy tiles
        auto origin_rel_to_cam = camera->game_to_local({0, 0});
        auto wr = camera->world_rect();
        s16 min_x = game_to_tile(wr.l());
        s16 max_x = game_to_tile(wr.r()) + 1_t;
        s16 min_y = game_to_tile(wr.t());
        s16 max_y = game_to_tile(wr.b()) + 1_t;
        min_x = ng::clamp(min_x, 0_t, room->w);
        max_x = ng::clamp(max_x, 0_t, room->w);
        min_y = ng::clamp(min_y, 0_t, room->h);
        max_y = ng::clamp(max_y, 0_t, room->h);
        // s16 min_x = 0_t;
        // s16 max_x = room->w;
        // s16 min_y = 0_t;
        // s16 max_y = room->h;

        Profile("print etiles");
        for (auto y = min_y; y < max_y; y += 1) {
            for (auto x = min_x; x < max_x; x += 1) {
                auto etile = &room->enemies[x + (y * room->w)];
                Bool skip_rect_draw = False;
                if (editor->in_etem) {
                    if (x == editor->e_mode_cursor.x &&
                        y == editor->e_mode_cursor.y) {
                        skip_rect_draw = True;
                        *etile = editor->etem_type_selection;
                    }
                }
                if (*etile == Enemy_Type::UNINITIALIZED) {
                    continue;
                }
                auto e_data = &enemy_data[cast(int)(*etile)];
                Vector2 tile_pos = {origin_rel_to_cam.x + tile_to_game(x),
                                    origin_rel_to_cam.y + tile_to_game(y)};
                // Render(renderer, &e_data->sprite.current_anim()->atlas.base,
                //        tile_pos);
                Render(renderer, &e_data->sprite, tile_pos.x, tile_pos.y);
            }
        }
    }
    if (editor->current_mode ==
        Editor_Mode::GRAPHICAL_MODE) { // render graphical cursor indicator
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        if (!editor->in_bounds) {
            r = 255;
            g = 32;
            b = 32;
        } else if (editor->in_gtem) {
            r = 32;
            g = 32;
            b = 255;
        } else {
            r = 32;
            g = 255;
            b = 32;
        }
        auto layer = &world->get_room(editor->location)
                          ->layers[editor->g_mode_cursor.layer];
        {
            auto origin_rel_to_cam = camera->game_to_local({0, 0});
            origin_rel_to_cam *= (layer->parallax / 255.f);
            auto tile_pos =
                origin_rel_to_cam +
                Vector2{tile_to_game(editor->g_mode_cursor.where.x),
                        tile_to_game(editor->g_mode_cursor.where.y)};
            SDL_Rect rect = {cast(int) game_to_px(tile_pos.x),
                             cast(int) game_to_px(tile_pos.y),
                             cast(int) game_to_px(TILESIZE),
                             cast(int) game_to_px(TILESIZE)};
            SDL_SetRenderDrawColor(renderer->c_renderer, r, g, b, 255);
            sdl_renderdrawrect_scaled(renderer->c_renderer, rect);
            rect.x += 1;
            rect.w -= 2;
            rect.y += 1;
            rect.h -= 2;
            SDL_SetRenderDrawColor(renderer->c_renderer, r, g, b, 128);
            sdl_renderdrawrect_scaled(renderer->c_renderer, rect);
        }
    } else if (editor->current_mode ==
               Editor_Mode::COLLISION_MODE) { // render collision cursor
                                              // indicator
        // @Refactor: redundant with graphical cursor indicator
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        u8 a = 0;
        if (!editor->in_bounds) {
            r = 255;
            g = 32;
            b = 32;
        } else if (editor->in_ctem) {
            Collision_Tile preview_ctile = {};
            preview_ctile.type = editor->ctem_type_selection;
            auto color = get_collision_tile_color(preview_ctile);
            r = color.r;
            g = color.g;
            b = color.b;
            a = scale_u8(color.a, 2.f);
        } else {
            r = 32;
            g = 255;
            b = 32;
            a = 255;
        }
        {
            auto origin_rel_to_cam = camera->game_to_local({0, 0});
            auto tile_pos =
                origin_rel_to_cam +
                Vector2{tile_to_game(editor->c_mode_cursor.where.x),
                        tile_to_game(editor->c_mode_cursor.where.y)};
            SDL_Rect rect = {cast(int) game_to_px(tile_pos.x),
                             cast(int) game_to_px(tile_pos.y),
                             cast(int) game_to_px(TILESIZE),
                             cast(int) game_to_px(TILESIZE)};
            SDL_SetRenderDrawColor(renderer->c_renderer, r, g, b, a);
            sdl_renderdrawrect_scaled(renderer->c_renderer, rect);
            rect.x += 1;
            rect.w -= 2;
            rect.y += 1;
            rect.h -= 2;
            SDL_SetRenderDrawColor(renderer->c_renderer, r, g, b, a / 2);
            sdl_renderdrawrect_scaled(renderer->c_renderer, rect);
        }
    } else if (editor->current_mode ==
               Editor_Mode::ENEMY_MODE) { // render enemy cursor indicator
        // @Refactor: redundant with graphical cursor indicator
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        if (!editor->in_bounds) {
            r = 255;
            g = 32;
            b = 32;
        } else if (editor->in_etem) {
            r = 32;
            g = 32;
            b = 255;
        } else {
            r = 32;
            g = 255;
            b = 32;
        }
        {
            auto origin_rel_to_cam = camera->game_to_local({0, 0});
            auto tile_pos = origin_rel_to_cam +
                            Vector2{tile_to_game(editor->e_mode_cursor.x),
                                    tile_to_game(editor->e_mode_cursor.y)};
            SDL_Rect rect = {cast(int) game_to_px(tile_pos.x),
                             cast(int) game_to_px(tile_pos.y),
                             cast(int) game_to_px(TILESIZE),
                             cast(int) game_to_px(TILESIZE)};
            SDL_SetRenderDrawColor(renderer->c_renderer, r, g, b, 255);
            sdl_renderdrawrect_scaled(renderer->c_renderer, rect);
            rect.x += 1;
            rect.w -= 2;
            rect.y += 1;
            rect.h -= 2;
            SDL_SetRenderDrawColor(renderer->c_renderer, r, g, b, 128);
            sdl_renderdrawrect_scaled(renderer->c_renderer, rect);
        }
    } else if (editor->current_mode ==
               Editor_Mode::AUTOMATIC_MODE) { // render auto-mode cursor
                                              // indicator
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        u8 a = 0;
        if (editor->in_bounds) {
            r = 32;
            g = 255;
            b = 32;
            a = 255;
        } else {
            r = 255;
            g = 32;
            b = 32;
        }
        {
            auto origin_rel_to_cam = camera->game_to_local({0, 0});
            auto tile_pos = origin_rel_to_cam +
                            Vector2{tile_to_game(editor->auto_mode_cursor.x),
                                    tile_to_game(editor->auto_mode_cursor.y)};
            SDL_Rect rect = {cast(int) game_to_px(tile_pos.x),
                             cast(int) game_to_px(tile_pos.y),
                             cast(int) game_to_px(TILESIZE),
                             cast(int) game_to_px(TILESIZE)};
            SDL_SetRenderDrawColor(renderer->c_renderer, r, g, b, a);
            sdl_renderdrawrect_scaled(renderer->c_renderer, rect);
            rect.x += 1;
            rect.w -= 2;
            rect.y += 1;
            rect.h -= 2;
            SDL_SetRenderDrawColor(renderer->c_renderer, r, g, b, a / 2);
            sdl_renderdrawrect_scaled(renderer->c_renderer, rect);
        }
    } else {
        ng_assert(False);
    }
    for (int i = editor->edit_history.history_size - 1; i >= 0; i -= 1) {
        auto top_x = 4_px;
        auto top_y = globals.screen_height - 20_px;
        SDL_Rect left_rect = {};
        left_rect.w = 4;
        left_rect.h = 4;
        left_rect.x = top_x;
        left_rect.y = top_y - ((left_rect.h + 1) *
                               (editor->edit_history.history_size - i));
        SDL_Rect right_rect = left_rect;
        right_rect.x += ((left_rect.w) + 1);
        auto edit = editor->edit_history.edits[i];
        if (edit.type == Edit_Type::GTILE_EDIT) {
            SDL_SetRenderDrawColor(renderer->c_renderer, 0, 0, 0, 64);
            sdl_renderdrawrect_scaled(renderer->c_renderer, left_rect);
            SDL_SetRenderDrawColor(renderer->c_renderer, 0, 0, 0, 64);
            sdl_renderdrawrect_scaled(renderer->c_renderer, right_rect);
            auto tileset = world->get_room(editor->location)
                               ->layers[edit.gtile_edit.where.layer]
                               .tileset;
            tileset.MoveTo(edit.gtile_edit.before.index);
            Render(renderer, &tileset, Rect(left_rect), Flip_Mode::NONE);
            tileset.MoveTo(edit.gtile_edit.after.index);
            Render(renderer, &tileset, Rect(right_rect), Flip_Mode::NONE);
        } else if (edit.type == Edit_Type::CTILE_EDIT) {
            auto first = get_collision_tile_color(edit.ctile_edit.before);
            auto second = get_collision_tile_color(edit.ctile_edit.after);
            first.a = scale_u8(first.a, 2.f);
            second.a = scale_u8(second.a, 2.f);
            SDL_SetRenderDrawColor(renderer->c_renderer, first.r, first.g,
                                   first.b, first.a);
            sdl_renderdrawrect_scaled(renderer->c_renderer, left_rect);
            SDL_SetRenderDrawColor(renderer->c_renderer, second.r, second.g,
                                   second.b, second.a);
            sdl_renderdrawrect_scaled(renderer->c_renderer, right_rect);
        } else if (edit.type == Edit_Type::AUTO_MODE_EDIT) {
            auto first =
                get_collision_tile_color(edit.auto_mode_edit.ctile_edit.before);
            auto second =
                get_collision_tile_color(edit.auto_mode_edit.ctile_edit.after);
            first.a = scale_u8(first.a, 2.f);
            second.a = scale_u8(second.a, 2.f);
            SDL_SetRenderDrawColor(renderer->c_renderer, first.r, first.g,
                                   first.b, first.a);
            sdl_renderdrawrect_scaled(renderer->c_renderer, left_rect);
            SDL_SetRenderDrawColor(renderer->c_renderer, second.r, second.g,
                                   second.b, second.a);
            sdl_renderdrawrect_scaled(renderer->c_renderer, right_rect);
        } else {
            ng_assert(False);
        }
    }

    constexpr auto exit_box_w = 44;
    constexpr auto exit_box_h = 28;
    for (int i = 0; i < room->exits.count; i += 1) {
        auto it = room->exits.data[i];
        Vector2 render_pos = {};
        // render_pos.x = globals.screen_width - exit_box_w;
        // render_pos.y = (exit_box_h + 1) * i;
        render_pos.x = (exit_box_w + 1) * i;
        render_pos.y = globals.screen_height - (exit_box_h + 1);
        {
            SDL_Rect r = {};
            r.x = cast(int) render_pos.x;
            r.y = cast(int) render_pos.y;
            r.w = exit_box_w;
            r.h = exit_box_h;
            SDL_SetRenderDrawColor(renderer->c_renderer, 0, 0, 0, 96);
            sdl_renderfillrect_scaled(renderer->c_renderer, r);
        }

        {
            auto pos = render_pos;
            pos.y += 2;
            pos.x += 1;
            auto print = [&](auto fmt, auto... args) {
                editor_print(pos, Horizontal_Align::LEFT, fmt, args...);
                pos.y += 3;
            };
            print("Exit #%0: dest 0x%1", i,
                  ng::format_int(it.destination, false, 16, 4));
            print("mode = %0", (it.mode == Room_Exit_Mode::ON_TOUCH
                                    ? "on touch"
                                    : (it.mode == Room_Exit_Mode::ON_INTERACTION
                                           ? "on interaction"
                                           : "inactive")));
            print("preserve x = %0", it.preserve_x);
            print("preserve y = %0", it.preserve_y);
            print("preserve vel = %0", it.preserve_vel);
            print("player x = %0", it.player_x);
            print("player y = %0", it.player_y);
            print("offset x = %0", it.offset_x);
            print("offset y = %0", it.offset_y);
        }
        {
            auto pos = render_pos;
            pos.y += cgn_h + 2;
        }
    }
    if (engine_state.mode == Engine_Mode::IN_EDITOR) {
        auto editor_z_fn_str = [](Editor_State *editor) {
            if (editor->current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (editor->in_bounds) {
                    if (editor->in_gtem) {
                        return " gtem-apply ";
                    } else {
                        return " gtem-enter ";
                    }
                } else {
                    return " (bad place)";
                }
            } else if (editor->current_mode == Editor_Mode::COLLISION_MODE) {
                if (editor->in_bounds) {
                    if (editor->in_ctem) {
                        return " ctem-apply ";
                    } else {
                        return " ctem-enter ";
                    }
                } else {
                    return " (bad place)";
                }
            } else {
                // @Stub: automatic-mode
                return "";
            }
        };
        auto editor_x_fn_str = [](Editor_State *editor) {
            auto none = " none       ";
            if (editor->current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (editor->in_bounds) {
                    if (editor->in_gtem) {
                        return " gtem-cancel";
                    } else {
                        return none;
                    }
                } else {
                    return none;
                }
            } else if (editor->current_mode == Editor_Mode::COLLISION_MODE) {
                if (editor->in_bounds) {
                    if (editor->in_ctem) {
                        return " ctem-cancel";
                    } else {
                        return none;
                    }
                } else {
                    return none;
                }
            } else {
                // @Stub: automatic-mode
                return "";
            }
        };
        auto editor_home_fn_str = [](Editor_State *editor) {
            auto none = " none           ";
            if (editor->current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (editor->layer_hiddens[editor->g_mode_cursor.layer]) {
                    return " Show the layer ";
                } else {
                    return " Hide the layer ";
                }
            } else {
                // @Stub: collision-mode, automatic-mode
                return none;
            }
        };
        auto editor_end_fn_str = [](Editor_State *editor) {
            auto none = " none           ";
            if (editor->current_mode == Editor_Mode::GRAPHICAL_MODE) {
                if (editor->layer_solos[editor->g_mode_cursor.layer]) {
                    return "Unsolo the layer";
                } else {
                    return " Solo the layer ";
                }
            } else if (editor->current_mode == Editor_Mode::COLLISION_MODE) {
                // if (editor->in_bounds) {
                //   if (editor->in_ctem) {
                //     return " ctem-cancel";
                //   } else {
                //     return none;
                //   }
                // } else {
                return none;
                // }
            } else {
                // @Stub: collision-mode, automatic-mode
                return none;
            }
        };
        editor_render_text("^v<>: move +/-: adjust volume"_s, {4.0_g, 4.0_g},
                           Horizontal_Align::LEFT);
        editor_render_text("ctrl-C/V copy/paste ctrl-Z/Y: undo/redo"_s,
                           {4.0_g, 12.0_g}, Horizontal_Align::LEFT);
        editor_print(
            {2.0_g, 10.0_g}, Horizontal_Align::LEFT, "Layer %0",
            ng::format_int(editor->g_mode_cursor.layer, false, 10, 0, 5));
        editor_print({80.0_g, 2.0_g}, Horizontal_Align::LEFT, " LMB:%0",
                     editor_z_fn_str(editor));
        editor_print({80.0_g, 6.0_g}, Horizontal_Align::LEFT, " Esc:%0",
                     editor_x_fn_str(editor));
        editor_render_text("^ PgUp"_s, {80.0_g, 24.0_g},
                           Horizontal_Align::LEFT);
        editor_render_text("v PgDn"_s, {80.0_g, 36.0_g},
                           Horizontal_Align::LEFT);
        editor_print({88.0_g, 20.0_g}, Horizontal_Align::LEFT, "Home:%0",
                     editor_home_fn_str(editor));
        editor_print({88.0_g, 28.0_g}, Horizontal_Align::LEFT, " End:%0",
                     editor_end_fn_str(editor));
    }
    auto layer = &room->layers[editor->g_mode_cursor.layer];
    if (editor->in_bounds) {
        constexpr auto tileset_x = 4;
        constexpr auto tileset_y = 16;
        constexpr auto size_div = 4;

        const auto tile_preview_x = globals.screen_center.x + 16.0_g;
        const auto tile_preview_y = globals.screen_center.y + 16.0_g;

        auto sprite_sheet = layer->tileset;
        if (editor->current_mode == Editor_Mode::GRAPHICAL_MODE) {
            if (layer->tileset.base.tex != null) {
                auto tex = sprite_sheet.base;
                SDL_Rect rect = {};
                rect.x = tileset_x - 1;
                rect.y = tileset_y - 1;
                rect.w = tex.w / size_div + 2;
                rect.h = tex.h / size_div + 2;
                SDL_SetRenderDrawColor(renderer->c_renderer, 255, 64, 255, 96);
                sdl_renderfillrect_scaled(renderer->c_renderer, rect);
                {
                    SDL_Rect src = {};
                    src.x = 0;
                    src.y = 0;
                    src.w = tex.w;
                    src.h = tex.h;
                    SDL_Rect dest = {};
                    dest.x = tileset_x;
                    dest.y = tileset_y;
                    dest.w = tex.w / size_div;
                    dest.h = tex.h / size_div;
                    Render(renderer, &tex, src, dest);
                }
            }
            { // render gtile at Gtile_Cursor g_mode_cursor using
                // Tileset_Cursor gtem_tileset_selection
                u8 selected_index = 0xFF;
                if (editor->in_gtem) {
                    // we're in gtile select mode, and our Tileset_Cursor is
                    // valid
                    selected_index = tileset_cursor_to_gtile_index(
                        editor->gtem_tileset_selection, &sprite_sheet);
                } else {
                    // we're not in gtile select mode, so preview the tile
                    // at the current Gtile_Cursor
                    selected_index =
                        gtile_cursor_to_gtile_ptr(editor->g_mode_cursor, room)
                            ->index;
                }
                {
                    auto draw_tile_preview = [](Vector2 pos, u8 index,
                                                TextureAtlas *tileset) {
                        SDL_Rect rect = {};
                        rect.x = game_to_px(pos.x) - 1_px;
                        rect.y = game_to_px(pos.y) - 1_px;
                        rect.w = game_to_px(TILESIZE) + 2_px;
                        rect.h = game_to_px(TILESIZE) + 2_px;
                        SDL_SetRenderDrawColor(renderer->c_renderer, 255, 64,
                                               255, index == 0xFF ? 48 : 96);
                        sdl_renderfillrect_scaled(renderer->c_renderer, rect);
                        if (index != 0xFF) {
                            tileset->MoveTo(index);
                            Render(renderer, tileset,
                                   /*game_to_px*/ (pos.x),
                                   /*game_to_px*/ (pos.y));
                        }
                    };
                    if (editor->current_mode == Editor_Mode::GRAPHICAL_MODE) {
                        { // render the gtile in the selection menu
                            draw_tile_preview({tile_preview_x, tile_preview_y},
                                              selected_index, &sprite_sheet);
                        }
                        { // render the gtile in the clipboard
                            u8 clipboard_index = 0xFF;
                            if (editor->g_mode_clipboard_valid) {
                                clipboard_index = tileset_cursor_to_gtile_index(
                                    editor->g_mode_clipboard, &sprite_sheet);
                            }
                            draw_tile_preview(
                                {tile_preview_x + 20._g, tile_preview_y},
                                clipboard_index, &sprite_sheet);
                        }
                    }
                    {
                        auto tile_dim = game_to_px(TILESIZE) / size_div;
                        SDL_Rect rect = {};
                        rect.x = tileset_x +
                                 tile_dim *
                                     (selected_index % sprite_sheet.atlasWidth);
                        rect.y = tileset_y +
                                 tile_dim *
                                     (selected_index / sprite_sheet.atlasWidth);
                        rect.w = tile_dim;
                        rect.h = tile_dim;
                        if (editor->in_gtem) {
                            auto alpha =
                                0.5 + ng::sin(t * ng::TAU32 * 10) * 0.5;
                            u8 a = 255;
                            a = scale_u8(a, cast(float) alpha);
                            SDL_SetRenderDrawColor(renderer->c_renderer, 255,
                                                   255, 255, a);
                        } else {
                            auto alpha =
                                0.5 + ng::sin(t * ng::TAU32 * 2) * 0.375;
                            u8 a = 255;
                            a = scale_u8(a, cast(float) alpha);
                            SDL_SetRenderDrawColor(renderer->c_renderer, 0, 206,
                                                   209, a);
                        }
                        sdl_renderdrawrect_scaled(renderer->c_renderer, rect);
                    }
                }
            }
        }
    }
    auto rect_dim_to_text_pos = [](SDL_Rect r) -> Vector2 {
        r.x += cgn_w;
        r.y += r.h / 2;
        return {r.x, r.y};
    };
    // u16 id = 0xFFFF;
    // Bool is_indoors = False;
    // enum struct LayerMode : u8 { Background, Foreground };
    // void SetTile(layer_index index, s16 x, s16 y,
    //              Graphical_Tile new_tile);
    // void Render(LayerMode mode);
    // void RenderLayer(layer_index i);
    // MusicInfo music_info = {};
    // struct MusicInfo { // @Migrate this out to an Audio_Music_Transition
    // struct
    //     Bool use_music_info = False;
    //     Bool loop = False;
    //     u16 fadeout_time =
    //         0; // \brief Time to fade out the previous song, ms
    //     u16 fadein_time = 0; // \brief Time to fade in this room's song, ms
    //     ng::string filename = no_music;
    //     static const ng::string
    //         no_music;                    // \brief Continue playing current
    //         song
    //     static const ng::string silence; // \brief Turn off any playing music
    // };
    // s16 w = 0_t;
    // s16 h = 0_t;
    // layer_index first_fg_layer = 0; // \brief First layer drawn after the
    // player ng::array<Enemy_Type> enemies = {world_allocator};
    ng_for(editor->rects_to_draw) {
        auto c = it.colour;
        auto r = it.rect.sdl_rect();
        SDL_SetRenderDrawColor(renderer->c_renderer, c.r, c.g, c.b, c.a);
        SDL_RenderFillRect(renderer->c_renderer, &r);
        SDL_RenderDrawRect(renderer->c_renderer, &r);
    }
    editor->rects_to_draw.count = 0;
    auto print_input_box = [&](Editor_Input_Box *input_box) {
        if (SDL_IsTextInputActive() &&
            editor->current_text_input == input_box) {
            ng::string string_to_render = {};
            string_to_render.len = editor->current_text_input_buffer.count;
            string_to_render.ptr = editor->current_text_input_buffer.data;
            { // render cursor
                SDL_Rect cursor_rect = editor->current_text_input->input_rect;

                cursor_rect.x +=
                    (editor->current_text_input_buffer.count + 1) * cgn_w;
                cursor_rect.w = cgn_w;

                cursor_rect.y += (cursor_rect.h / 2) - (cgn_h / 2);
                cursor_rect.h = cgn_h;

                auto alpha = 0.625f + ng::sin(t * ng::TAU32 * 2) * 0.375f;
                auto a = scale_u8(255, alpha);
                SDL_SetRenderDrawColor(renderer->c_renderer, 255, 255, 255, a);
                SDL_RenderFillRect(renderer->c_renderer, &cursor_rect);
            }
            editor_render_text(
                string_to_render,
                rect_dim_to_text_pos(editor->current_text_input->input_rect),
                Horizontal_Align::LEFT);
        } else {
            auto string = editor_get_string_from_input(editor, input_box);
            defer {
                if (string.should_free) {
                    ng::free_string(&string.string);
                }
            };
            editor_render_text(string.string,
                               rect_dim_to_text_pos(input_box->input_rect),
                               Horizontal_Align::LEFT);
        }
    };
    ng_for(editor_inputs) { print_input_box(it); }
}
