#include "general.h"
#include "renderer.h"
#include "ui.h"

int sign(float x) {
    if (x < 0) {
        return -1;
    }
    return x > 0;
}

Rng game_rng = {};
Rng fx_rng = {};

static ng::map<u32, Texture> glyphs = {};

static Texture render_glyph(u32 character) {
    auto ch = cast(u16) character;
    ng_assert(ch == character);
    auto font = ui->font; // @Global
    ng_assert(TTF_GlyphIsProvided(font, ch));
    auto surf =
        TTF_RenderGlyph_Blended(font, ch, SDL_Colour{255, 255, 255, 255});
    defer { SDL_FreeSurface(surf); };
    auto tex = SDL_CreateTextureFromSurface(renderer->c_renderer, surf);
    return texture_from_sdl(tex);
}

Vector2 debug_size_text(ng::string text) {
    Vector2 result = {};
    for (u64 i = 0; i < text.len; i += 1) {
        u32 character = cast(u32) text[i]; // @Unicode @Utf8
        Texture *glyph = null;
        auto glyph_found = glyphs[character];
        if (glyph_found.found) {
            glyph = glyph_found.value;
        } else {
            glyph = glyphs.insert(character, render_glyph(character));
        }
        result.x += glyph->w;
        result.y += glyph->h;
    }
    result.y /= text.len; // Yields average height
    return result;
}

enum struct Horizontal_Align { LEFT = 0, CENTER = 1, RIGHT = 2 };

void debug_draw_text(ng::string text, Vector2 pos, Horizontal_Align align) {
    auto size = debug_size_text(text);
    const float align_factors[3] = {0.0f, 0.5f, 1.0f};
    pos.y -= size.y / 2;
    pos.x -= (size.x * align_factors[cast(int) align]);
    for (u64 i = 0; i < text.len; i += 1) {
        u32 character = cast(u32) text[i]; // @Unicode @Utf8
        auto glyph = *glyphs[character].value;
        Rect dest = {pos.x, pos.y, glyph.w, glyph.h};
        auto ren = renderer->c_renderer; // @Global
        // SDL_Rect c_dest = dest.sdl_rect();
        SDL_Rect c_dest = {dest.x, dest.y, dest.w, dest.h};
        SDL_RenderCopy(ren, glyph.tex, null, &c_dest);
        pos.x += glyph.w;
    }
}

struct Profile_Data {
    ng::string func = {};
    ng::string label = {};
    u64 cycles = 0;
    u64 perf_time = 0;
    u64 call_count = 0;
};
static ng::map<ng::string, Profile_Data> profile_data = {};
static Bool is_profiling = False;
Auto_Profile::Auto_Profile(ng::string location, const char *func,
                           ng::string label) {
    this->location.ptr = null;
    if (!is_profiling) {
        return;
    }
    this->location = location;
    this->func = func;
    this->label = label;
    cycle_count = __rdtsc();
    perf_count = SDL_GetPerformanceCounter();
}
Auto_Profile::~Auto_Profile() {
    if (!is_profiling) {
        return;
    }
    if (!location.len) {
        return; // Profiling was activated in the middle of this zone.
    }
    auto data = profile_data[location].value; // @Speed
    if (!data) {
        data = profile_data.insert(location, {});
        data->func = func;
        data->label = label;
    }
    u64 cycle_diff = __rdtsc() - cycle_count;
    u64 perf_diff = SDL_GetPerformanceCounter() - perf_count;
    data->cycles += cycle_diff;
    data->perf_time += perf_diff;
    data->call_count += 1;
}

void profile_toggle() { is_profiling = !is_profiling; }

void profile_init() {
    profile_data.allocate(64);
    glyphs.allocate(256);
    for (u32 i = 32; i < 127; i += 1) {
        glyphs.insert(i, render_glyph(i));
    }
}
void profile_quit() {
    profile_data.release();
    glyphs.release();
}

void profile_render(Renderer *renderer) {
    if (is_profiling) { // Render profiling data
        u64 max_location_len = 0;
        u64 max_func_len = 0;
        u64 max_label_len = 0;
        int num_texts = 0;
        ng_for(profile_data) {
            auto data = &it.value;
            if (!data->call_count) {
                continue;
            }
            if (max_location_len < it.key.len) {
                max_location_len = it.key.len;
            }
            if (max_label_len < data->label.len) {
                max_label_len = data->label.len;
            }
            if (max_func_len < data->func.len) {
                max_func_len = data->func.len;
            }
            num_texts += 1;
        }
        SDL_Rect rect = {};
        rect.x = 0;
        rect.y = 0;
        rect.w = globals.screen_width * renderer->c_scale;
        rect.h = num_texts * (cgn_h + 2);
        SDL_SetRenderDrawColor(renderer->c_renderer, 0, 0, 0, 127);
        SDL_RenderFillRect(renderer->c_renderer, &rect);
        static const auto sdl_perffreq = 1.0 / SDL_GetPerformanceFrequency();
        int text_vertical = 1;
        ng_for(profile_data) {
            auto data = &it.value;
            if (!data->call_count) {
                continue;
            }
            auto perf_time = data->perf_time * sdl_perffreq;
            u64 cycles_per_call = data->cycles / data->call_count;
            u64 cycles_total = data->cycles;
            auto seconds_per_call = perf_time / data->call_count;

            ng::print_buffer text_buf = {};
            // Since we hope all characters stay onscreen, we can
            // preallocate the correct buffer length.
            text_buf.cap =
                ((globals.screen_width * renderer->c_scale) / cgn_w) + 1;
            text_buf.str = cast(u8 *) ng::malloc(text_buf.cap);
            // But still, @Todo in ng.hpp: format_string() with padding!
            // @Todo in ng.hpp: rename format_* to fmt* or something
            defer { ng::free(text_buf.str); };
            ng::sprint(&text_buf, "%0:", it.key);
            while (text_buf.len < max_location_len + 1) {
                text_buf.putchar(' ');
            }
            auto current_pos = text_buf.len;
            ng::sprint(&text_buf, "%0()", data->func);
            while (text_buf.len < current_pos + max_func_len + 2) {
                text_buf.putchar(' ');
            }
            current_pos = text_buf.len;
            if (data->label.len) {
                ng::sprint(&text_buf, "'%0'", data->label);
            }
            while (text_buf.len < current_pos + max_label_len + 2) {
                text_buf.putchar(' ');
            }
            text_buf.putchar(':');
            text_buf.putchar(' ');
            ng::sprint(&text_buf, "%0c %1c/call %2s %3s/call",
                       ng::format_int(cycles_total, false, 10, 0, 10),
                       ng::format_int(cycles_per_call, false, 10, 0, 10),
                       ng::format_float(perf_time, 14),
                       ng::format_float(seconds_per_call, 14));
            Vector2 pos = {0.0_g, cgn_h * text_vertical};
            ng::string text_string = {text_buf};
            debug_draw_text(text_string, pos, Horizontal_Align::LEFT);
            text_vertical += 1;

            data->cycles = 0;
            data->perf_time = 0;
            data->call_count = 0;
        }
    }
}
