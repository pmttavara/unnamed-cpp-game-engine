#pragma once
#include "general.h"
void draw_rect(Rect r, SDL_Color colour = SDL_Color{0, 0, 0, 64});
extern ng::allocator *video_allocator;
struct Texture {
    SDL_Texture *tex = null;
    int w = 0;
    int h = 0;
    float alpha = 1.0;
};
Texture texture_load(ng::string filename);
Texture texture_from_sdl(SDL_Texture *c_texture);

struct Rect_To_Draw {
    Rect rect;
    SDL_Color colour;
};
/* singleton */ struct Renderer {
    struct SDL_Renderer *c_renderer = null;
    struct SDL_Window *c_window = null;
    Bool fullscreen = False;
    float c_scale = 1.0;
    ng::array<SDL_Texture *> textures = {video_allocator};
    ng::array<ng::string> filenames = {video_allocator};
    SDL_Texture *AddTexture(ng::string filename);
    ng::string filename_of(SDL_Texture *texture);
    void ToggleFullscreen();
    ng::array<Rect_To_Draw> rects_to_draw = {video_allocator};
};
extern Renderer *renderer;
void Render(Renderer *renderer, Texture *t, Rect src, Rect dest,
            Flip_Mode mode = Flip_Mode::NONE);
void Render(Renderer *renderer, Texture *t, Vector2 pos,
            Flip_Mode mode = Flip_Mode::NONE);
struct TextureAtlas {
    Texture base;
    int w = 0;
    int h = 0;
    int atlasWidth = 0;
    int atlasHeight = 0; // \brief Size of the atlas in "sprites".
    int index = 0; // \brief Index in the spritesheet.
    int num_frames = 0;
    void MoveTo(int i) { index = i % num_frames; }
};
TextureAtlas texture_atlas_create(ng::string filename, s32 imageWidth, s32 imageHeight);
void Render(Renderer *renderer, TextureAtlas *ta, f32 x, f32 y,
            Flip_Mode mode = Flip_Mode::NONE);
void Render(Renderer *renderer, TextureAtlas *ta, Rect dest,
            Flip_Mode mode = Flip_Mode::NONE);
struct Animation {
    TextureAtlas atlas;
    f32 frametime = 0.0_s;
    Vector2 offset = {}; // \brief Offset from which to render
    void Reset() { atlas.MoveTo(0); }
};                               
Animation animation_create(ng::string filename, s32 imageWidth, s32 imageHeight,
                           u16 fps, Vector2 offset = {});
using animation_ptr = u16;
struct Sprite {
    ng::array<Animation> animations = {video_allocator};
    f32 animation_timer = 0.0_s;
    int current_animation_index = 0;

    Flip_Mode flip = Flip_Mode::NONE;

    void change_to_animation(int animationIndex);
    void update(f32 dt);
    void Restart();
    Animation *current_anim() {
        return &animations[current_animation_index];
    }
};
Sprite sprite_copy(Sprite *rhs);
void sprite_destroy(Sprite *sprite);

void Render(Renderer *renderer, Sprite *s, f32 x, f32 y);
