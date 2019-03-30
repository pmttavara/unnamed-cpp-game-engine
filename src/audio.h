#pragma once
#include "general.h"
extern ng::allocator *audio_allocator;
struct Sound {
    Mix_Chunk *chunk = null;
    int current_channel = -1;
    Bool retrigger = True;
    float volume = 1.f;

    void update_volume();
    void update_volume(float new_volume);
    void play(Bool loop = False);
    void stop();
    void update_channel();
};
Sound sound_from_filename(ng::string filename);
//
enum struct Fade_Type { NONE, OUT, IN };
struct Audio {
    float global_volume = 1.f;
    float volume_ratio = 0.5f;
    float sound_volume() { return volume_ratio; }
    float music_volume() { return 1.f - volume_ratio; }
    //
    ng::map<ng::string, Mix_Chunk *> sounds = {audio_allocator}; 
    Bool sound_paused = False;
    //
    ng::array<ng::string> music_filenames = {audio_allocator};
    ng::array<Mix_Music *> music_data = {audio_allocator};
    Fade_Type music_fade_type = Fade_Type::NONE;
    float music_fader = 1.f;
    int current_song = -1;
    f32 current_music_fade_time = 0._s;
    int queued_song = -1;
    f32 queued_music_fade_time = 0._s;
    Bool queued_music_loop = False;
    Bool music_paused = False;
    //
    void update_sound(f32 dt);
    void update_music(f32 dt);
    //
    Mix_Chunk *add_sound(ng::string filename);
    void pause_sound();
    void resume_sound();
    //
    void pause_music();
    void resume_music();
    void restart_music();
    void fade_music_in(f32 fade_time);
    void fade_music_out(f32 fade_time);
    int find_song(ng::string filename);
    void play_song(ng::string filename, f32 fadeout, f32 fadein,
                   Bool loop);
    void add_song(ng::string filename);
    void stop_music(f32 fade_time = 0);
    void start_music_data(int song, Bool loop, f32 fadein);
    void queue_music_data(int song, Bool loop, f32 fadein);
};
void audio_destroy(Audio *audio);
// void audio_apply_room_music(Room::MusicInfo music_info);
extern Audio *audio;
