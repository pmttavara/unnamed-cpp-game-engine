#include "renderer.h"
ng::allocator *video_allocator = null;
Renderer *renderer = null;
void draw_rect(Rect r, SDL_Color colour) {
#if 0
    renderer->rects_to_draw.push({r, colour});
#else
    USE r;
    USE colour;
#endif
}
SDL_Texture *Renderer::AddTexture(ng::string filename) {
    ng_for(filenames) {
        if (it == filename) {
            auto it_index = &it - filenames.begin();
            return textures[it_index];
        }
    }
    // new texture must be loaded
    auto new_texture = IMG_LoadTexture(c_renderer, ng::c_str(filename));
    ng_assert(new_texture != null,
              "SDL Error: '%0', c_renderer = %1, filename = %2", SDL_GetError(),
              c_renderer, filename);
    SDL_SetTextureBlendMode(new_texture, SDL_BLENDMODE_BLEND);
    textures.push(new_texture);
    filenames.push(copy_string(filename, video_allocator));
    return new_texture;
}
void Render(Renderer *renderer, Texture *t, Rect src, Rect dest,
            Flip_Mode mode) {
    auto alpha = t->alpha;
    if (alpha <= 0.f) {
        return;
    }
    if (alpha > 1.f) {
        alpha = 1.f;
    }
    SDL_Rect c_src = src.sdl_rect();
    auto cdest = dest;
    cdest.x *= renderer->c_scale;
    cdest.y *= renderer->c_scale;
    cdest.w *= renderer->c_scale;
    cdest.h *= renderer->c_scale;
    SDL_Rect c_dest = cdest.sdl_rect();
    SDL_SetTextureAlphaMod(t->tex, cast(u8)(alpha * 255.f));
    SDL_RendererFlip flip = cast(SDL_RendererFlip) mode;
    ng_assert(SDL_RenderCopyEx(renderer->c_renderer, t->tex, &c_src, &c_dest,
                               0.0, null, flip) == 0,
              "Error rendering. renderer->c_renderer = %0, t->tex = %1",
              renderer->c_renderer, t->tex);
}
void Render(Renderer *renderer, Texture *t, Vector2 pos, Flip_Mode mode) {
    auto W = px_to_game(t->w);
    auto H = px_to_game(t->h);
    Render(renderer, t, Rect{0, 0, W, H}, Rect{pos.x, pos.y, W, H}, mode);
}
ng::string Renderer::filename_of(SDL_Texture *texture) {
    ng::string result = {};
    ng_for(textures) {
        if (it == texture) {
            int it_index = &it - textures.begin();
            result = filenames[it_index];
            break;
        }
    }
    return result;
}
void Renderer::ToggleFullscreen() {
    u32 flag = fullscreen ? SDL_WINDOW_SHOWN : SDL_WINDOW_FULLSCREEN;
    ng_assert(SDL_SetWindowFullscreen(c_window, flag) == 0,
              "Error changing fullscreen mode.");
    fullscreen = !fullscreen;
}
Texture texture_load(ng::string filename) {
    Texture result = {};
    result.tex = renderer->AddTexture(filename);
    SDL_QueryTexture(result.tex, null, null, &result.w, &result.h);
    return result;
}
Texture texture_from_sdl(SDL_Texture *c_texture) {
    Texture result = {};
    int idx = renderer->textures.count;
    ng_for(renderer->textures) {
        if (it == c_texture) {
            idx = &it - renderer->textures.begin();
            break;
        }
    }
    if (idx >= renderer->textures.count) {
        // unique texture, add to list
        renderer->textures.push(c_texture);
        // add a dummy filename so that all indices still match, preserving that
        // simple associative container technique (@Hack!!!)
        renderer->filenames.push(ng::string{});
    } else {
        // not unique, no operation necessary
    }
    result.tex = c_texture;
    SDL_QueryTexture(result.tex, null, null, &result.w, &result.h);
    return result;
}
TextureAtlas texture_atlas_create(ng::string filename, int imageWidth,
                                  int imageHeight) {
    TextureAtlas result = {};
    result.base = texture_load(filename);
    ng_assert(imageWidth <= result.base.w);
    ng_assert(imageHeight <= result.base.h);
    ng_assert(imageWidth > 0);
    ng_assert(imageHeight > 0);
    result.w = imageWidth;
    result.h = imageHeight;
    result.atlasWidth = result.base.w / imageWidth;
    result.atlasHeight = result.base.h / imageHeight;
    ng_assert(result.atlasWidth > 0);
    ng_assert(result.atlasHeight > 0);
    result.num_frames = result.atlasWidth * result.atlasHeight;
    return result;
}
void Render(Renderer *renderer, TextureAtlas *ta, f32 x, f32 y,
            Flip_Mode mode) {
    // location of the appropriate sprite on the atlas, based on the current
    // index
    auto atlas_x = px_to_game((ta->index % ta->atlasWidth) * ta->w);
    auto atlas_y = px_to_game((ta->index / ta->atlasWidth) * ta->h);
    Rect src(atlas_x, atlas_y, px_to_game(ta->w), px_to_game(ta->h));
    Rect dest(
        /*px_to_game*/ (x),
        /*px_to_game*/ (y), px_to_game(ta->w), px_to_game(ta->h));
    Render(renderer, &ta->base, src, dest, mode);
}
void Render(Renderer *renderer, TextureAtlas *ta, Rect dest, Flip_Mode mode) {
    // location of the appropriate sprite on the atlas, based on the current
    // index
    auto atlas_x = px_to_game((ta->index % ta->atlasWidth) * ta->w);
    auto atlas_y = px_to_game((ta->index / ta->atlasWidth) * ta->h);
    Rect src(atlas_x, atlas_y, px_to_game(ta->w), px_to_game(ta->h));
    Render(renderer, &ta->base, src, dest, mode);
}
Animation animation_create(ng::string filename, s32 imageWidth, s32 imageHeight,
                           u16 fps, Vector2 offset) {
    Animation result = {};
    result.atlas = texture_atlas_create(filename, imageWidth, imageHeight);
    result.frametime = fps_to_s(fps);
    result.offset = offset;
    return result;
}
Sprite sprite_copy(Sprite *rhs) {
    Sprite result = *rhs;
    result.animations = rhs->animations.copy();
    return result;
}
void sprite_destroy(Sprite *sprite) { sprite->animations.release(); }
void Render(Renderer *renderer, Sprite *s, f32 x, f32 y) {
    ng_assert(s->animations.count > 0);
    if (s->animations.count == 0) {
        return;
    }
    auto anim = s->current_anim();
    Render(renderer, &anim->atlas, x + anim->offset.x, y + anim->offset.y,
           s->flip);
}
void Sprite::change_to_animation(int animationIndex) {
    if (animationIndex == current_animation_index || animations.count == 0) {
        return;
    }
    ng_assert(animationIndex < animations.count);
    current_animation_index = animationIndex;
    ng_assert(current_anim());
    Restart();
}
void Sprite::update(f32 dt) {
    if (animations.count == 0) {
        return;
    }
    animation_timer += dt;
    auto anim = &animations[current_animation_index];
    if (anim->frametime > 0.0_s) {
        anim->atlas.MoveTo(animation_timer / anim->frametime);
    } else {
        anim->atlas.MoveTo(0);
    }
}
void Sprite::Restart() {
    animation_timer = 0.0_s;
    update(0.0_s);
}
