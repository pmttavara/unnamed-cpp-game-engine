#include "audio.h"

ng::allocator *audio_allocator = null;

Sound sound_from_filename(ng::string filename) {
    Sound result = {};
    result.chunk = audio->add_sound(filename);
    return result;
}
void audio_destroy(Audio *audio) {
    ng_for(audio->sounds) {
        Mix_FreeChunk(it.value);
        ng::free_string(&it.key, audio_allocator);
    }
    ng_for(audio->music_data) { Mix_FreeMusic(it); }
    ng_for(audio->music_filenames) { free_string(&it, audio_allocator); }
    Mix_CloseAudio();
}
void Sound::play(Bool loop) {
    if (!chunk) {
        return;
    }
    update_channel();
    if (current_channel != -1) { // still playing
        if (retrigger) {         // whatever, just retrigger
            stop();
        } else { // nothing left to do
            return;
        }
    }
    auto play_sound = [&] { return Mix_PlayChannel(-1, chunk, loop ? -1 : 0); };
    auto channel = play_sound();
    if (channel == -1) {
        auto num_sound_channels = Mix_AllocateChannels(-1);
        if (num_sound_channels < 1024) {
            Mix_AllocateChannels(cast(int)(num_sound_channels *
                                           1.5)); // grow like a dynamic array
            channel = play_sound();
        }
    }
    if (channel != -1) {
        Mix_Volume(channel,
                   cast(int)(volume *
                             (audio->sound_volume() * audio->global_volume) *
                             MIX_MAX_VOLUME));
    }
    current_channel = channel;
}
void Sound::stop() {
    update_channel();
    if (current_channel != -1) {
        Mix_HaltChannel(current_channel);
    }
}
void Sound::update_volume() {
    update_channel();
    if (current_channel != -1) {
        Mix_Volume(current_channel, cast(int)(volume * MIX_MAX_VOLUME));
    }
}
void Sound::update_volume(float value) {
    volume = value;
    update_volume();
}
void Sound::update_channel() {
    if (Mix_GetChunk(current_channel) == chunk &&
        Mix_Playing(current_channel)) { // Channel is playing and it's playing
                                        // our chunk, so we're done.
        // @Incomplete @FixMe: What if we stopped, but we think we're still
        // playing because another Sound is playing the same chunk on the same
        // channel? Rare case, but would probably be incredibly noticeable.
    } else {
        current_channel = -1;
    }
}
Audio *audio = null;
#include "world.h"
void audio_apply_room_music(Room::MusicInfo music_info) {
    if (music_info.use_music_info) {
        if (music_info.filename == Room::MusicInfo::no_music) {
            // continue any playing music, i.e., do nothing
        } else if (music_info.filename == Room::MusicInfo::silence) {
            audio->stop_music(ms_to_s(music_info.fadeout_time));
        } else {
            audio->play_song(music_info.filename,
                             ms_to_s(music_info.fadeout_time),
                             ms_to_s(music_info.fadein_time), music_info.loop);
        }
    }
}
static void set_sound_volume(Audio *audio, float volume) {
    for (int i = 0, num_channels = Mix_AllocateChannels(-1); i < num_channels;
         i += 1) {
        auto channel_volume = cast(float) Mix_Volume(i, -1) / MIX_MAX_VOLUME;
        { // get original volume of sound
            channel_volume /= audio->sound_volume();
            channel_volume /= audio->global_volume;
        }
        channel_volume *= volume;
        Mix_Volume(i, cast(int)(channel_volume * MIX_MAX_VOLUME));
    }
}
void Audio::update_sound(f32 dt) {
    USE dt;
    set_sound_volume(this, sound_volume() * global_volume);
}
void Audio::update_music(f32 dt) {
    if (!music_paused) {
        if (music_fade_type != Fade_Type::NONE) {
            auto delta = dt / current_music_fade_time;
            if (music_fade_type == Fade_Type::IN) {
                music_fader += delta;
            } else {
                music_fader -= delta;
            }
            if (music_fader > 1) {
                music_fader = 1;
                music_fade_type = Fade_Type::NONE;
            }
            if (music_fader < 0) {
                music_fader = 0;
                music_fade_type = Fade_Type::NONE;
            }
        }
        if (music_fader <= 0) {
            if ((music_fade_type != Fade_Type::IN) && queued_song < 0) {
                current_song = -1;
            } else {
                start_music_data(queued_song, queued_music_loop,
                                 queued_music_fade_time);
            }
        }
        Mix_VolumeMusic(cast(int)(
            music_fader * (music_volume() * global_volume) * MIX_MAX_VOLUME));
    }
}
Mix_Chunk *Audio::add_sound(ng::string filename) {
    auto found = sounds[filename];
    if (!found.found) {
        auto new_sound = Mix_LoadWAV(ng::c_str(filename));
        // ng_assert(new_sound);
        auto string = copy_string(filename, audio_allocator);
        ng_assert(string.ptr);
        sounds.insert(string, new_sound);
        return new_sound;
    } else {
        return *found.value;
    }
}
void Audio::pause_sound() { Mix_Pause(-1); }
void Audio::resume_sound() { Mix_Resume(-1); }
void Audio::pause_music() {
    if (!music_paused) {
        music_paused = True;
        Mix_PauseMusic();
    }
}
void Audio::resume_music() {
    if (music_paused) {
        music_paused = False;
        Mix_ResumeMusic();
    }
}
void Audio::restart_music() { Mix_RewindMusic(); }
void Audio::fade_music_in(f32 fade_time) {
    if (fade_time > 0.f) {
        music_fade_type = Fade_Type::IN;
        current_music_fade_time = fade_time;
    } else {
        music_fader = 1.f;
        music_fade_type = Fade_Type::NONE;
        current_music_fade_time = 0.f;
    }
}
void Audio::fade_music_out(f32 fade_time) {
    if (fade_time > 0.f) {
        music_fade_type = Fade_Type::OUT;
        current_music_fade_time = fade_time;
    } else {
        music_fader = 0.f;
        music_fade_type = Fade_Type::NONE;
        current_music_fade_time = 0.f;
    }
}
int Audio::find_song(ng::string filename) {
    ng_for(music_filenames) {
        auto it_index = &it - music_filenames.begin();
        if (it == filename) {
            return it_index;
        }
    };
    return -1;
}
void Audio::play_song(ng::string filename, f32 fadeout, f32 fadein, Bool loop) {
    auto song_to_play = find_song(filename);
    if (song_to_play == -1) {
        add_song(filename);
        song_to_play = music_data.count - 1;
    }
    if (current_song == -1 && music_fader <= 0) {
        start_music_data(song_to_play, loop, fadein);
    } else {
        if (song_to_play == current_song) {
            queue_music_data(-1, False, 0);
            fade_music_in(fadein);
        } else {
            queue_music_data(song_to_play, loop, fadein);
            fade_music_out(fadeout);
        }
    }
}
void Audio::add_song(ng::string filename) {
    auto index = find_song(filename);
    if (index == -1) {
        auto data = music_data.push(Mix_LoadMUS(ng::c_str(filename)));
        music_filenames.push(copy_string(filename, audio_allocator));
        ng_assert(data != null, "Unable to load song '%0'.", filename);
    }
}
void Audio::stop_music(f32 fade_time) {
    fade_music_out(fade_time);
    queued_song = -1;
}
void Audio::start_music_data(int song, Bool loop, f32 fadein) {
    if (song < 0) {
        return;
    }
    auto result = Mix_FadeInMusicPos(music_data[song], loop ? -1 : 0, 0, 0.0);
    USE result;
    // ng_assert(result == 0, "Mixer error");
    current_song = song;
    queued_song = -1;
    fade_music_in(fadein);
}
void Audio::queue_music_data(int song, Bool loop, f32 fadein) {
    queued_song = song;
    queued_music_loop = loop;
    queued_music_fade_time = fadein;
}
