#define NG_DEFINE
#include "ng.hpp"

#include "general.h"

#include "camera.h"
#include "editor.h"
#include "engine_state.h" // @Remove
#include "filesystem.h"
#include "input.h"
#include "particle.h"
#include "player.h"
#include "ui.h"
#include "world.h"

Globals globals = {};

void profile_render(Renderer *renderer);
void profile_toggle();
void profile_init();
void profile_quit();

template <class T> struct temp_arg {
    T t;
    template <class... Args> temp_arg(Args &&... args) : t{(Args &&) args...} {}
    operator T &() { return t; }
};
Room make_default_room() {
    Room result = {};
    result.music_info.filename = copy_string("silence"_s, world_allocator);
    result.collision.resize(1);
    result.id = 0x0000;
    return result;
};
Room quick_generate_room(u16 id, s16 w, s16 h, int num_layers,
                         int first_foreground_layer) {
    Room result = {};
    result.id = id;
    result.w = w;
    result.h = h;
    result.layers.resize(num_layers);
    ng_for(result.layers) { it.tiles.resize(w * h); }
    result.first_fg_layer = cast(Room::layer_index) first_foreground_layer;
    result.collision.resize(w * h);
    result.enemies.resize(w * h);
    result.exits.resize(4);
    return result;
}

void load_game_data() {
    world->rooms.insert(0x0000, make_default_room());
    // {
    //   auto rw = RW("data/test/example_v1.qtr"_s, "rb"_s);
    //   world->AddRoom(LoadRoom(&rw));
    // }
    // {
    //   auto rw = RW("data/test/example2.qtr"_s, "rb"_s);
    //   world->AddRoom(LoadRoom(&rw));
    // }
    {
        auto rw = RW("data/test/tester.qtr"_s, "rb"_s);
        world->AddRoom(LoadRoom(&rw));
    }
    if (0) {
        auto onymis_proto = quick_generate_room(0x000F, 128, 16, 5, 3);
        onymis_proto.layers[0].parallax = 0;
        ng_for(onymis_proto.layers) it.tileset =
            texture_atlas_create("data/tilesets/prairlea.png"_s, 16, 16);
        auto rw = RW("data/test/onymis_proto.qtr"_s, "wb"_s);
    }
    if (0) {
        auto gameplay_test = quick_generate_room(0x0010, 64, 64, 2, 1);
        ng_for(gameplay_test.layers) {
            it.tileset =
                texture_atlas_create("data/tilesets/test.png"_s, 16, 16);
        }
        auto rw = RW("data/test/gameplay_test.qtr"_s, "wb"_s);
        SaveRoom(&rw, &gameplay_test);
    }
    {
        world->AddRoom(LoadRoom(
            &cast(RW &) temp_arg<RW>{"data/test/gameplay_test.qtr"_s, "rb"_s}));
    }
    {
        auto rw = RW("data/test/onymis_proto.qtr"_s, "rb"_s);
        world->AddRoom(LoadRoom(&rw));
    }
    {
        init_enemy_data();
        auto g = Enemy_Type::GOOMBER;
        auto rr = Enemy_Type::RUNNY_RABBIT;
        auto fr = Enemy_Type::FLYING_RAT;
        auto sj = Enemy_Type::SYNCED_JUMPER;
        auto ct = Enemy_Type::CANNON_TOAST;
        auto cc = Enemy_Type::CHARGING_CHEETAH;
        auto add_enemy = [](auto &&a, auto &&b, auto &&c, auto &&d) {
            USE a;
            entity_manager->enemies.push({a, b, c, d});
        };
        for (auto i = 0_t; i < 32_t; i += 1) {
            add_enemy(sj, cast(u16) 0xBEEF, i, 8_t);
        }
        auto id = cast(u16) 0xF00D;
        for (int i = 0; i < 1; ++i) {
            add_enemy(g, id, 11_t, 8_t);
            add_enemy(ct, id, 11_t, 8_t);
            add_enemy(sj, id, 15_t, 8_t);
            add_enemy(cc, id, 9_t, 8_t);
            add_enemy(rr, id, 27_t, 8_t);
            add_enemy(rr, id, 29_t, 8_t);
            add_enemy(fr, id, 21_t, 16_t);
            add_enemy(fr, id, 23_t, 16_t);
        }
        USE g;
        USE rr;
        USE fr;
        USE sj;
        USE ct;
        USE cc;
    }
    {
        Npc npc = {};
        npc.location = 0xF00D;
        npc.npc_id = 0xABCD;
        npc.pos = {tile_to_game(9_t), tile_to_game(1_t)};
        npc.vel = {0._g, 0._g};
        npc.hitbox = Rect{-8, -20, 16, 40};
        Sprite npc_spr = {};
        {
            npc_spr.animations.push(animation_create(
                "data/sprites/crono2.png"_s, 16_px, 40_px, 0_fps, {-8, -20}));
        }
        npc.sprite = npc_spr;
        npc.walk_speed = tile_to_game(2_t);
        npc.talk_string_id = 0x0001;
        entity_manager->npcs.push(npc);
        ui->text_box.AddTextString(
            "Have you seen the strange creatures popping up recently?"_s,
            0x0001);
    }
    {
        Npc npc = {};
        npc.location = 0xF00D;
        npc.npc_id = 0xCAFE;
        npc.pos = {tile_to_game(10_t), tile_to_game(8_t)};
        npc.vel = {0._g, 0._g};
        npc.hitbox = Rect{-8, -16, 16, 32};
        Sprite npc_spr = {};
        {
            npc_spr.animations.push(animation_create(
                "data/sprites/mario.png"_s, 16_px, 32_px, 0_fps, {-8, -16}));
        }
        npc.sprite = npc_spr;
        npc.walk_speed = tile_to_game(3_t);
        npc.talk_string_id = 0x0006;
        entity_manager->npcs.push(npc);
        ui->text_box.AddTextString(
            "I used to have a hammer, but I can't find it anymore."_s, 0x0006);
    }
    {
        Npc npc = {};
        npc.location = 0xF00D;
        npc.npc_id = 0xBABE;
        npc.pos = {tile_to_game(11_t), tile_to_game(8_t)};
        npc.vel = {0._g, 0._g};
        npc.hitbox = Rect{-8, -12, 16, 24};
        Sprite npc_spr = {};
        {
            npc_spr.animations.push(
                animation_create("data/sprites/small_mario.png"_s, 16_px, 24_px,
                                 0_fps, {-8, -12}));
        }
        npc.sprite = npc_spr;
        npc.walk_speed = tile_to_game(1_t);
        npc.talk_string_id = 0x0007;
        entity_manager->npcs.push(npc);
        ui->text_box.AddTextString("Placeholder text"_s, 0x0007);
    }
    {
        Npc npc = {};
        npc.location = 0x000f;
        npc.npc_id = 0x5739;
        npc.pos = {tile_to_game(32_t), tile_to_game(1_t)};
        npc.vel = {};
        npc.hitbox = Rect{-8, -12, 16, 24};
        {
            npc.sprite.animations.push(animation_create(
                "data/sprites/mario.png"_s, 16_px, 32_px, 0_fps, {-8, -16}));
        }
        ui->text_box.AddTextString("sup ma frend"_s, 0x5739);
        npc.walk_speed = tile_to_game(4_t);
        npc.talk_string_id = 0x5739;
        entity_manager->npcs.push(npc);
    }
    {
        { // heart pickup data
            Sprite pickup_sprite = {};
            pickup_sprite.animations.push(animation_create(
                "data/sprites/heart.png"_s, 12_px, 12_px, 0_fps, {-6, -6}));
            auto pickup_sound =
                sound_from_filename("data/sound/jump_rise.wav"_s);
            auto data = &pickup_data[cast(u8) Pickup_Type::HEART];
            data->sprite = pickup_sprite;
            data->sound = pickup_sound;
            data->pickup_radius = 12._g;
        }
        Pickup heart = {};
        heart.type = Pickup_Type::HEART;
        heart.info.heal_amount = 2_hp;
        heart.location = 0xF00D;
        heart.pos = {tile_to_game(5_t), tile_to_game(5_t)};
        entity_manager->pickups.push(heart);
    }
    init_weapon_data();
    player->init();
    engine_state.game_data_loaded = True;
}
void unload_game_data(Engine_State *) {
    deinit_weapon_data();
    deinit_enemy_data();
}

void begin_game(Engine_State *engine_state) {
    ui->last_menu = Ui::Menu::Pause;
    ui->active_menu = Ui::Menu::Pause;
    if (!engine_state->game_data_loaded) {
        auto real_world = world; // @Hack
        world = world_sbs;
        defer { world = real_world; };
        auto real_player = player;
        player = player_sbs;
        defer { player = real_player; };
        auto real_entman = entity_manager;
        entity_manager = entity_manager_sbs;
        defer { entity_manager = real_entman; };
        load_game_data(); // @Hack
    }
    ng_assert(engine_state->game_data_loaded);
    *world = world_copy(world_sbs); // @Todo @Editor: after saving the world,
                                    // the SBS needs to be updated so that when
                                    // you restart during the current game
                                    // session, it doesn't load a stale version
                                    // of the room.
    *player = player_copy(player_sbs);
    *entity_manager = entity_manager_copy(entity_manager_sbs);
    // @Todo: ask for save, (load|make new) game, go to right room, cutscenes
    // etc.
    {
        RW rw(GAME_NAME ".sav"_s, "r+"_s);
        Savegame save = {};
        if (savegame_load(&rw, &save)) {
            savegame_apply(&save, engine_state);
        } else {
            // @Todo: new game
            player->location = 0xF00D;
            camera->location = 0xF00D;
            {
                player->pos = {0.5f, 18.5f};
                player->pos *= tile_to_game(1_t);
                player->mstate.Land();
            }
            audio->play_song("data/music/dr_wu_style.ogg"_s, 0.5_s, 0.0_s,
                             True);
        }
    }
    {
        camera->smooth = True;
        camera->half_life = {0.25_s, 0.125_s};
        camera->pos = player->pos;
#if 0
        camera->target = {&player->pos, &player->location};
#else
        player->target_me(camera);
#endif
    }
    audio->resume_sound();
    audio->resume_music();
    engine_state->mode = Engine_Mode::IN_ENGINE;

    game_rng.engine.seed();
    fx_rng.engine.seed();
}
void pause_game(Engine_State *engine_state) {
    if (engine_state->mode == Engine_Mode::IN_ENGINE) {
        engine_state->mode = Engine_Mode::PAUSED;
        audio->pause_sound();
        // audio->pause_music();
    }
}
void unpause_game(Engine_State *engine_state) {
    if (engine_state->mode == Engine_Mode::PAUSED) {
        engine_state->mode = Engine_Mode::IN_ENGINE;
        audio->resume_sound();
        // audio->resume_music();
    }
}
void go_to_main_menu(Engine_State *engine_state) {
    engine_state->mode = Engine_Mode::MAIN_MENU;
    {
        ui->savegame_exists = savegame_load(
            &cast(RW &) temp_arg<RW>{GAME_NAME ".sav"_s, "r+b"_s}, null);
    }
    {
        particle_system->ClearAll();
        audio->play_song("data/music/tighter_theme.ogg"_s, 0.5_s, 0.0_s, True);
    }
    editor_destroy(&editor); // @Todo: make Engine_State contain Editor_State
}

void destroy_window_and_renderer() {
    SDL_HideWindow(renderer->c_window);
    ng_for(renderer->textures) { SDL_DestroyTexture(it); }
    renderer->textures.release();
    SDL_DestroyRenderer(renderer->c_renderer);
    renderer->c_renderer = null;
    SDL_DestroyWindow(renderer->c_window);
    renderer->c_window = null;
    ng_for(renderer->filenames) { free_string(&it, video_allocator); }
    renderer->filenames.release();
    renderer->rects_to_draw.release();
}
float get_good_window_scale(int w, int h) {
    int ratio_w = w / (DEFAULT_WIDTH + 50);
    int ratio_h = h / (DEFAULT_HEIGHT + 50);
    int result = ng::min(ratio_w, ratio_h);
    result = ng::max(result, 1);
    return cast(float) result;
}
f64 get_current_time() { return SDL_GetTicks() / 1000.0; };

void game_tick(f64 t, f64 dt);
void game_render(f64 t);

void update_screen_center(float) {
    globals.screen_center = {px_to_game(globals.screen_width) / 2._g,
                             px_to_game(globals.screen_height) / 2._g};
    // globals.screen_center /= scale;
}

float round_scale(float scale) {
    constexpr int factor = 4;
    if (scale > 1.0f) {
        return (cast(int) ng::round(scale * factor)) / factor;
    }
    return ng::round(scale);
}

void set_scale_and_update_width_and_height(int w, int h) {
    auto w_scale = cast(float) w / DEFAULT_WIDTH;
    auto h_scale = cast(float) h / DEFAULT_HEIGHT;
    globals.screen_width = w;
    globals.screen_height = h;
    auto scale = ng::min(w_scale, h_scale);
    scale = round_scale(scale);
    globals.screen_width /= scale;
    globals.screen_height /= scale;
    renderer->c_scale = scale;
    update_screen_center(renderer->c_scale);
}

int main() {
    ng::allocator_init();
    ng::print_init();
    ng::stack_allocator stack_allocator = {};
    auto make_proxy_allocator = [](ng::allocator *creator, char *name,
                                   ng::allocator *parent) -> ng::allocator * {
        auto data = creator->make_new<ng::proxy_allocator>();
        *data = ng::create_proxy_allocator(name, parent);
        return data;
    };
    auto free_proxy_allocator = [](ng::allocator *creator,
                                   ng::allocator *proxy_to_free) {
        auto proxy_data = (ng::proxy_allocator *)proxy_to_free;
        ng::destroy_proxy_allocator(proxy_data);
        creator->make_delete(proxy_data);
    };
    auto main_allocator =
        make_proxy_allocator(&stack_allocator, "main_proc", &stack_allocator);
    defer { free_proxy_allocator(&stack_allocator, main_allocator); };
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER |
                 SDL_INIT_TIMER) != 0) {
        ng::print("Initialization error: %0", SDL_GetError());
        return 1;
    }
    defer { SDL_Quit(); };
#ifdef NDEBUG
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_CRITICAL);
#else
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
#endif
    video_allocator =
        make_proxy_allocator(main_allocator, "video", ng::default_allocator);
    defer { free_proxy_allocator(main_allocator, video_allocator); };

    renderer = video_allocator->make_new<Renderer>();
    defer {
        destroy_window_and_renderer();
        video_allocator->make_delete(renderer);
    };

    int update_rate = DEFAULT_UPDATERATE;
    SDL_DisplayMode display = {};
    { // create window
        SDL_GetDesktopDisplayMode(0, &display);
        if (display.refresh_rate > 0) {
            update_rate = display.refresh_rate;
        }
        if (update_rate < 10_fps) {
            update_rate = 10_fps;
        }

        auto scale = get_good_window_scale(display.w, display.h);
        set_scale_and_update_width_and_height(DEFAULT_WIDTH * scale,
                                              DEFAULT_HEIGHT * scale);
        renderer->c_window = SDL_CreateWindow(
            GAME_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            globals.screen_width * scale, globals.screen_height * scale,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    }
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@ @@
    // @@    @Todo: Go through all references to screen_* and fix scaling!!!! @@
    // @@ @@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    if (renderer->c_window == null) {
        ng::print("Initialization error: %0", SDL_GetError());
        return 1;
    }
    { // create renderer
        renderer->c_renderer = SDL_CreateRenderer(renderer->c_window, -1,
                                                  SDL_RENDERER_ACCELERATED);
    }
    if (renderer->c_renderer == null) {
        ng::print("Initialization error: %0", SDL_GetError());
        return 1;
    }
    // No defer code for window/renderer: they're destroyed immediately on quit,
    // in order to look cool.

    if (TTF_Init() != 0) {
        ng::print("Initialization error: %0", TTF_GetError());
        return 1;
    }
    defer { TTF_Quit(); };
    ui = video_allocator->make_new<Ui>();
    // the defer for ui_destroy is down after the audio allocator
    ui->text_box.ui = ui;
    {
        ui->font = TTF_OpenFont("code_golf_neue.ttf", 8);
        ng_assert(ui->font != null);
        // render "Loading" text and show screen
        auto ren = renderer->c_renderer;
        auto surf = TTF_RenderText_Blended(ui->font, "Loading",
                                           SDL_Color{180, 180, 200, 255});
        defer { SDL_FreeSurface(surf); };
        auto c_tex = SDL_CreateTextureFromSurface(ren, surf);
        defer { SDL_DestroyTexture(c_tex); };
        // query size
        int w = 0, h = 0;
        SDL_QueryTexture(c_tex, null, null, &w, &h);
        // render texture
        auto scale = renderer->c_scale;
        SDL_Rect dest = {cast(int)((globals.screen_width - w) / 2),
                         cast(int)((globals.screen_height - h) / 2), w, h};
        USE scale;
        dest.x *= scale;
        dest.y *= scale;
        dest.w *= scale;
        dest.h *= scale;
        SDL_ShowWindow(renderer->c_window);
        SDL_SetRenderDrawColor(renderer->c_renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer->c_renderer);
        SDL_RenderCopy(ren, c_tex, null, &dest);
        SDL_RenderPresent(renderer->c_renderer);
    }
    defer { TTF_CloseFont(ui->font); };

    profile_init();
    defer { profile_quit(); };

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        ng::print("Initialization error: %0", IMG_GetError());
        return 1;
    }
    defer { IMG_Quit(); };
    {
        auto window_icon_surface = IMG_Load(GAME_NAME ".png");
        defer { SDL_FreeSurface(window_icon_surface); };
        SDL_SetWindowIcon(renderer->c_window, window_icon_surface);
    }
    ui->init_textures();

    world_allocator =
        make_proxy_allocator(main_allocator, "world", ng::default_allocator);
    defer { free_proxy_allocator(main_allocator, world_allocator); };

    world_sbs = world_allocator->make_new<World>();
    *world_sbs = world_create();
    defer {
        world_destroy(world_sbs);
        world_allocator->make_delete(world_sbs);
    };
    world = world_allocator->make_new<World>();
    defer { world_allocator->make_delete(world); };
    entity_manager = world_allocator->make_new<Entity_Manager>();
    defer {
        entity_manager_destroy(entity_manager);
        world_allocator->make_delete(entity_manager);
    };
    entity_manager_sbs = world_allocator->make_new<Entity_Manager>();
    defer {
        entity_manager_destroy(entity_manager_sbs);
        world_allocator->make_delete(entity_manager_sbs);
    };
    camera = world_allocator->make_new<Camera>();
    defer { world_allocator->make_delete(camera); };
    particle_system = video_allocator->make_new<Particle_System>();
    defer {
        particle_system_destroy(particle_system);
        video_allocator->make_delete(particle_system);
    };
    if (Mix_Init(MIX_INIT_OGG) == 0) {
        ng::print("Initialization error: %0", Mix_GetError());
        return 1;
    }
    defer { Mix_Quit(); };

    audio_allocator =
        make_proxy_allocator(main_allocator, "audio", ng::default_allocator);
    defer { free_proxy_allocator(main_allocator, audio_allocator); };
    {
        audio = audio_allocator->make_new<Audio>();
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048);
        Bool audio_opened = (Mix_QuerySpec(null, null, null) == 1);
        if (!audio_opened) { // @Todo: something more sophisticated than this
            // ng_assert(False);
        }
        Mix_AllocateChannels(24);
    }
    defer {
        audio_destroy(audio);
        audio_allocator->make_delete(audio);
    };
    ui->init_sounds(); // delayed till now because it needed audio
    defer {
        ui_destroy(ui);
        video_allocator->make_delete(ui);
    };
    player = world_allocator->make_new<Player>();
    defer {
        player_destroy(player);
        world_allocator->make_delete(player);
    };
    player_sbs = world_allocator->make_new<Player>();
    defer {
        player_destroy(player_sbs);
        world_allocator->make_delete(player_sbs);
    };
    {
        SDL_SetRenderDrawBlendMode(renderer->c_renderer, SDL_BLENDMODE_BLEND);
        ui->using_controller = False;
    }
    filesystem_allocator = make_proxy_allocator(main_allocator, "filesystem",
                                                ng::default_allocator);
    defer { free_proxy_allocator(main_allocator, filesystem_allocator); };

    defer { editor_destroy(&editor); };
    // @Todo: init_entire_game_or_whatever();
    { // make sure the save file exists
        auto rw = SDL_RWFromFile(GAME_NAME ".sav", "rb");
        if (!rw) { // file doesn't even exist
            rw = SDL_RWFromFile(GAME_NAME ".sav", "wb");
        }
        ng_assert(rw);
        SDL_RWclose(rw);
    }
    SDL_StopTextInput();
    engine_state.mode = Engine_Mode::MAIN_MENU;
    {
        // audio->music_fader = 0.1f; // @Hack: Why is this needed?
        go_to_main_menu(&engine_state);
    }
    audio->global_volume = 0.05f;
    // audio->volume_ratio = 1.f;
    // begin_game(&engine_state);
    defer { unload_game_data(&engine_state); };
    // @Todo: let editor add/remove layers, change tilesets, parallax value, etc
    {
        auto xf00d = world->get_room(0xf00d);
        USE xf00d;
    }
    f64 t = 0.0_s;
    const f64 dt = fps_to_s(DEFAULT_UPDATERATE);
    for (;;) {
        game_tick(t, dt);
        game_render(t);
        profile_render(renderer);
        SDL_RenderPresent(renderer->c_renderer);
        t += dt;
        SDL_Delay(s_to_ms(dt));
        if (engine_state.should_quit) {
            exit(0);
            destroy_window_and_renderer();
            audio->stop_music();
            audio->global_volume = 0.f;
            return 0;
        }
    }
}

void game_tick(f64 t, f64 dt) {
    USE t;
    Profile();

    Input_State in = {};
    in.keys = SDL_GetKeyboardState(null);

    if (engine_state.mode == Engine_Mode::IN_EDITOR) {
        if (editor.current_mode == Editor_Mode::AUTOMATIC_MODE) {
            editor.in_ctem = False;
            editor.in_gtem = False;
        }
    }
    // handle events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        default: {
        } break;
        case SDL_WINDOWEVENT: {
            if (event.window.event ==
                SDL_WINDOWEVENT_RESIZED) { // handle window resize
                auto new_w = event.window.data1;
                auto new_h = event.window.data2;
                set_scale_and_update_width_and_height(new_w, new_h);
            }
        } break;
        case SDL_QUIT: {
            engine_state.should_quit = True;
        } break;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            input_update_from_sdl_keyboard_event(&in, &event.key);

            // @Migrate into editor_handle_sdl_key_event()
            audio->global_volume = ng::clamp(audio->global_volume, 0.f, 1.f);
        } break;
        case (SDL_MOUSEBUTTONDOWN):
        case (SDL_MOUSEBUTTONUP): {
            editor_handle_sdl_mouse_button_event(event.button);
        } break;
        case (SDL_MOUSEWHEEL): {
            editor_handle_sdl_mouse_wheel_event(event.wheel);
        } break;

        case (SDL_TEXTINPUT): {
            editor_handle_sdl_text_input_event(event.text);
        } break;
            // case (SDL_TEXTEDITING): {
            //     editor_handle_sdl_text_editing_event(event.edit);
            // } break;
        }
    }
    if (in.keys_new[KEY(F3)]) {
        profile_toggle();
    }

    // input_update_from_sdl_keystate(in, SDL_GetKeyboardState(null));
    { // faux fullscreen
        if (in.keys_new[KEY(RETURN)] &&
            (in.keys[KEY(LALT)] || in.keys[KEY(RALT)])) {
            in.keys_new[KEY(RETURN)] = False; // so later functions see nothing
            renderer->fullscreen = !renderer->fullscreen;
            auto win = renderer->c_window;
            int w = 0;
            int h = 0;
            SDL_DisplayMode display = {};
            SDL_GetDesktopDisplayMode(0, &display);
            if (renderer->fullscreen) {
                SDL_SetWindowBordered(win, SDL_FALSE);
                SDL_SetWindowResizable(win, SDL_FALSE);
                w = display.w;
                h = display.h;
                set_scale_and_update_width_and_height(w, h);
            } else {
                SDL_SetWindowBordered(win, SDL_TRUE);
                SDL_SetWindowResizable(win, SDL_TRUE);
                auto scale = get_good_window_scale(display.w, display.h);
                w = cast(int)(DEFAULT_WIDTH * scale);
                h = cast(int)(DEFAULT_HEIGHT * scale);
                set_scale_and_update_width_and_height(w, h);
            }
            w = globals.screen_width * renderer->c_scale;
            h = globals.screen_height * renderer->c_scale;
            SDL_SetWindowSize(win, w, h);
            SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED);
        }
    }
    update_screen_center(renderer->c_scale);
    if (engine_state.mode == Engine_Mode::IN_ENGINE) {
        player->HandleInput(&in);
    }
    ui->HandleInput(&in, &engine_state);
    editor_handle_input(&in);
    auto flags = SDL_GetWindowFlags(renderer->c_window);
    if (flags & SDL_WINDOW_INPUT_FOCUS) {

    } else {
        // pause_game(&engine_state); // @Todo: this causes problems
        // by pausing while debugging!
    }

    if (engine_state.mode == Engine_Mode::TRANSITION_FREEZE) {
        ng_assert(player->transitioning_rooms == True);
        player->room_transition_timer.elapsed += dt;
        if (player->room_transition_timer.expired()) {
            player->room_transition_timer.elapsed = 0;

            room_transition(world, player);
            engine_state.mode = Engine_Mode::IN_ENGINE;

            // Forget everything about the simulation and start fresh.
            return;
        }
    }
    renderer->rects_to_draw.count = 0;
    {
        auto sim_dt = dt; // in-game dt, versus dt for menu etc.
        if (engine_state.mode == Engine_Mode::PAUSED) {
            sim_dt = 0.0_s;
        }
        audio->update_sound(dt);
        if (engine_state.mode == Engine_Mode::IN_ENGINE) {
            entity_manager->update(sim_dt, world);
            particle_system->update(sim_dt);
        }
        if (engine_state.mode == Engine_Mode::IN_ENGINE ||
            engine_state.mode == Engine_Mode::PAUSED) {
            camera->update(sim_dt);
            ui->text_box.update(sim_dt);
        }
        ui->menu_fader.elapsed += dt; // even when paused
        ui->update(dt, &engine_state);
        audio->update_music(dt);
        if (engine_state.mode == Engine_Mode::IN_EDITOR) {
            editor_update(&editor, dt, &in);
        }
    }
}
void game_render(f64 t) {
    Profile();
    // render

    SDL_SetRenderDrawColor(renderer->c_renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer->c_renderer);

    Bool render_world = (engine_state.mode == Engine_Mode::IN_ENGINE ||
                         engine_state.mode == Engine_Mode::PAUSED ||
                         engine_state.mode == Engine_Mode::TRANSITION_FREEZE);
    Bool render_particles = render_world;
    Bool render_entities = render_world;
    Bool render_player =
        render_world || engine_state.mode == Engine_Mode::IN_EDITOR;
    Bool render_ui = True;

    Bool render_editor = (engine_state.mode == Engine_Mode::IN_EDITOR ||
                          (editor.render_editor_mode_even_when_in_engine &&
                           engine_state.mode == Engine_Mode::IN_ENGINE));

    auto room = world->get_room(camera->location);
    if (!render_editor) {
        if (render_particles) {
            particle_system->render_background_particles();
        }
        if (render_world) {
            room->Render(Room::LayerMode::Background);
        }
        if (render_entities) {
            entity_manager->render_below_player();
        }
        if (render_player) {
            player->Render(camera);
        }
        if (render_entities) {
            entity_manager->render_above_player();
        }
        if (render_particles) {
            particle_system->render_midground_particles();
        }
        if (render_world) {
            ui_render_animated_tiles(ui, room);
            room->Render(Room::LayerMode::Foreground);
        }
        if (render_particles) {
            particle_system->render_frontground_particles();
        }
        if (render_ui) {
            ui->Render(&engine_state);
        }
    } else {
        editor_render_world(&editor, room);
        entity_manager->render_below_player();
        entity_manager->render_above_player();
        particle_system->render_background_particles();
        particle_system->render_midground_particles();
        particle_system->render_frontground_particles();
        player->Render(camera);
        editor_render(&editor, t);
    }

    ng_for(renderer->rects_to_draw) {
        auto rect = it.rect;
        Vector2 rel_pos = camera->game_to_local(rect.pos());
        rect.x = rel_pos.x;
        rect.y = rel_pos.y;
        rect.x *= renderer->c_scale;
        rect.y *= renderer->c_scale;
        rect.w *= renderer->c_scale;
        rect.h *= renderer->c_scale;
        SDL_Rect c_aabb = rect.sdl_rect();
        SDL_SetRenderDrawColor(renderer->c_renderer, it.colour.r, it.colour.g,
                               it.colour.b, it.colour.a);
        SDL_RenderDrawRect(renderer->c_renderer, &c_aabb);
    }
    SDL_RenderPresent(renderer->c_renderer);
}
