#pragma once
#define GAME_NAME "Qta"

#include "ng.hpp"
using namespace ng::int_types;
using ng::operator""_s;

#define Bool ng_bool
#define True ng_true
#define False ng_false

#define USE (void)

#define int8_t /*    */ s8
#define uint8_t /*   */ u8
#define int16_t /*  */ s16
#define uint16_t /* */ u16
#define int32_t /*  */ s32
#define uint32_t /* */ u32
#define int64_t /*  */ s64
#define uint64_t /* */ u64

#define Sint8 int8_t
#define Uint8 uint8_t
#define Sint16 int16_t
#define Uint16 uint16_t
#define Sint32 int32_t
#define Uint32 uint32_t
#define Sint64 int64_t
#define Uint64 uint64_t
#define SDL_MAIN_HANDLED
#include "SDL2.h"
#undef Sint8
#undef Uint8
#undef Sint16
#undef Uint16
#undef Sint32
#undef Uint32
#undef Sint64
#undef Uint64
#undef SDL_MAIN_HANDLED
#undef int8_t
#undef uint8_t
#undef int16_t
#undef uint16_t
#undef int32_t
#undef uint32_t
#undef int64_t
#undef uint64_t

struct Vector2 {
    float x = 0.f;
    float y = 0.f;
    NG_INLINE Vector2() = default;
    NG_INLINE Vector2(float x, float y) : x{x}, y{y} {}
    NG_INLINE Vector2 &operator=(const Vector2 &v) {
        return x = v.x, y = v.y, *this;
    }
    NG_INLINE Vector2 &operator+=(const Vector2 &v) {
        return x += v.x, y += v.y, *this;
    }
    NG_INLINE Vector2 &operator-=(const Vector2 &v) {
        return x -= v.x, y -= v.y, *this;
    }
    NG_INLINE Vector2 &operator*=(float v) { return x *= v, y *= v, *this; }
    NG_INLINE Vector2 &operator/=(float v) { return x /= v, y /= v, *this; }
    NG_INLINE Vector2 operator+(const Vector2 &v) const {
        return {x + v.x, y + v.y};
    }
    NG_INLINE Vector2 operator-(const Vector2 &v) const {
        return {x - v.x, y - v.y};
    }
    NG_INLINE Vector2 operator*(float v) const { return {x * v, y * v}; }
    NG_INLINE Vector2 operator/(float v) const { return {x / v, y / v}; }
    NG_INLINE Vector2 operator-() const { return {-x, -y}; }
    Vector2 hat() const { return magsq() == 0 ? Vector2{0, 0} : *this / mag(); }
    NG_INLINE bool operator==(const Vector2 &v) const {
        return x == v.x && y == v.y;
    }
    NG_INLINE bool operator!=(const Vector2 &v) const {
        return x != v.x || y != v.y;
    }
    float magsq() const { return x * x + y * y; }
    float mag() const { return ng::sqrt(magsq()); }
    float dot(const Vector2 &l, const Vector2 &r) {
        return l.x * r.x + l.y * r.y;
    }
    static Vector2 min(const Vector2 &l, const Vector2 &r) {
        return {ng::min(l.x, r.x), ng::min(l.y, r.y)};
    }
    static Vector2 max(const Vector2 &l, const Vector2 &r) {
        return {ng::max(l.x, r.x), ng::max(l.y, r.y)};
    }
};
namespace ng {
NG_INLINE int print_get_item_size(const Vector2 &) {
    return 2 * print_get_item_size(float{}) + 4;
}
inline void print_item(print_buffer *buffer, const Vector2 &v) {
    ng::sprint(buffer, "(%0, %1)", v.x, v.y);
}
} // namespace ng
// units
#define OP(T, suffix, arg)                                                     \
    NG_INLINE constexpr T operator"" suffix(arg value) {                       \
        return static_cast<T>(value);                                          \
    }
#define OP_FLOAT long double
#define OP_INT unsigned long long
OP(f32, _g, OP_FLOAT);
OP(s32, _px, OP_INT);
OP(s16, _t, OP_INT);
OP(f32, _s, OP_FLOAT);
// OP(u32, _ms, OP_INT);
OP(u16, _fps, OP_INT);
OP(s32, _hp, OP_INT);
#undef OP
#undef OP_FLOAT
#undef OP_INT

constexpr auto TILESIZE = 16.0_g;
constexpr auto DEFAULT_WIDTH = 256_px;
constexpr auto DEFAULT_HEIGHT = 240_px;
constexpr auto DEFAULT_UPDATERATE = 60_fps;

// s <-> ms
NG_INLINE u32 s_to_ms(f32 s) { return cast(u32)(s * 1000); }
NG_INLINE f32 ms_to_s(u32 ms) { return ms / 1000.f; }
// s <-> fps
NG_INLINE u8 s_to_fps(f32 s) { return cast(u8)(1.f / s); }
NG_INLINE f32 fps_to_s(u16 fps) { return 1.f / fps; }
// ms <-> fps
NG_INLINE u8 ms_to_fps(u32 ms) { return s_to_fps(ms_to_s(ms)); }
NG_INLINE u32 fps_to_ms(u16 fps) { return s_to_ms(fps_to_s(fps)); }

// game <-> px
NG_INLINE f32 px_to_game(s32 px) { return cast(f32) px; }
NG_INLINE s32 game_to_px(f32 game) { return cast(s32) ng::round(game); }
// game <-> tile
NG_INLINE s16 game_to_tile(f32 game) { return cast(s16)(game / TILESIZE); }
NG_INLINE f32 tile_to_game(s16 tile) { return cast(f32) tile * TILESIZE; }
// px <-> tile
NG_INLINE s16 px_to_tile(s32 px) { return game_to_tile(px_to_game(px)); }
NG_INLINE s32 tile_to_px(s16 tile) { return game_to_px(tile_to_game(tile)); }

enum struct Flip_Mode { NONE, HORIZONTAL, VERTICAL, BOTH };

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    NG_INLINE Rect() = default;
    NG_INLINE Rect(float x, float y, float w, float h)
        : x{x}, y{y}, w{w}, h{h} {}
    Rect(SDL_Rect rect) {
        x = cast(float) rect.x;
        w = cast(float) rect.w;
        y = cast(float) rect.y;
        h = cast(float) rect.h;
    }
    Vector2 pos() { return Vector2{x, y}; }
    Vector2 size() { return Vector2{w, h}; }
    Bool operator==(Rect rhs) {
        return x == rhs.x && y == rhs.y && w == rhs.w && h == rhs.h;
    }
    Bool empty() { return w <= 0 || h <= 0; }
    Rect &operator+=(Vector2 pos) {
        x += pos.x;
        y += pos.y;
        return *this;
    }
    Rect &operator-=(Vector2 pos) { return *this += -pos; }
    Rect &operator*=(float mul) {
        w *= mul;
        h *= mul;
        return *this;
    }
    Rect operator+(Vector2 pos) { return Rect(*this) += pos; }
    Rect operator-(Vector2 pos) { return *this + -pos; }
    Rect operator*(float mul) { return Rect(*this) *= mul; }
    Rect &Flip(Flip_Mode mode) {
        if (cast(int) mode & cast(int) Flip_Mode::HORIZONTAL) {
            x = -(x + w);
        }
        if (cast(int) mode & cast(int) Flip_Mode::VERTICAL) {
            y = -(y + h);
        }
        return *this;
    }
    NG_INLINE float l() { return x; }
    NG_INLINE float r() { return x + w; }
    NG_INLINE float t() { return y; }
    NG_INLINE float b() { return y + h; }
    NG_INLINE float cx() { return x + w / 2; }
    NG_INLINE float cy() { return y + h / 2; }
    Bool intersects(Rect other) {
        return x < other.x + other.w && x + w > other.x &&
               y < other.y + other.h && y + h > other.y;
    }
    static Rect Intersection(Rect r1, Rect r2) {
        Rect result = {};
        result.x = ng::max(r1.x, r2.x);
        result.w = ng::min(r1.x + r1.w, r2.x + r2.w) - result.x;
        result.y = ng::max(r1.y, r2.y);
        result.h = ng::min(r1.y + r1.h, r2.y + r2.h) - result.y;
        return result;
    }
    SDL_Rect sdl_rect() {
        return {cast(int) game_to_px(x), cast(int) game_to_px(y),
                cast(int) game_to_px(w), cast(int) game_to_px(h)};
    }
};

static NG_INLINE float rng_linear(float x) { return x; };
static NG_INLINE float rng_mostly_min(float x) { return ng::pow(x, 7); }
static NG_INLINE float rng_mostly_max(float x) { return 1 - ng::pow(x, 7); }
static inline float rng_mostly_mid(float x) {
    // 1|        _/  This distribution looks
    //  |   ____/    something like this.
    //  | _/         <--
    //  |/_________
    //  0         1
    return 64.f * ng::pow(x - 0.5f, 7) + 0.5f;
}

using Rng_Distribution = float(float);

struct Rng {
    ng::mt19937_64 engine = {};
    float get(float min, float max, Rng_Distribution *dist = &rng_linear) {
        auto raw = engine.next_float();
        auto range = max - min;
        return dist(raw) * range + min;
    }
    int get(int min, int max, Rng_Distribution *dist = &rng_linear) {
        return cast(int) get(cast(float) min, cast(float) max, dist);
    }
    Vector2 get(Vector2 min, Vector2 max) {
        auto center_of_bounding_ellipse = (max + min) / 2;
        auto radius_of_bounding_ellipse = (max - min).mag() / 2;
        auto r = get(-0.5f, 0.5f) * radius_of_bounding_ellipse;
        auto theta = get(0.0f, ng::TAU32);

        Vector2 result = {ng::cos(theta), ng::sin(theta)};
        result *= r;
        result += center_of_bounding_ellipse;
        return result;
    }
};

extern Rng game_rng;
extern Rng fx_rng;

struct Timer {
    f32 elapsed = 0._s;
    f32 expiration = 0._s;
    int run(f32 dt) {
        int times_triggered = 0;
        elapsed += dt;
        while (elapsed > expiration) {
            elapsed -= expiration;
            times_triggered += 1;
        }
        return times_triggered;
    }
    NG_INLINE Bool expired() { return elapsed >= expiration; }
    void expire() { elapsed = expiration; }
};

struct Globals {
    s32 screen_width = 0;
    s32 screen_height = 0;
    Vector2 screen_center = {};

    struct Audio *audio = null;
    struct Camera *camera = null;
    struct Engine_State *engine_state = null;
    struct Entity_Manager *entity_manager = null;
    struct Particle_System *particle_system = null;
    struct Renderer *renderer = null;
    struct Ui *ui = null;
    struct World *world = null;
    struct Player *player = null;
};
extern Globals globals;

struct Auto_Profile {
    ng::string location;
    const char *func;
    ng::string label;
    u64 cycle_count;
    u64 perf_count;
    Auto_Profile(ng::string location, const char *func, ng::string label);
    ~Auto_Profile();
};
#define Profile(...)                                                           \
    Auto_Profile NG_ANON(_prof)(                                               \
        NG_CAT(__FILE__ ":" NG_STRINGIZE(__LINE__), _s), NG_FUNCTION,          \
        NG_CAT("" __VA_ARGS__, _s));

int sign(float x);

template <class T>
ng::array<T> array_deep_copy(ng::array<T> source, T (*copier)(T *)) {
    ng::array<T> result = {source.alloc};
    result.reserve(source.count);
    ng_for(source) { result.push(copier(&it)); }
    return result;
}
template <class T>
void array_deep_destroy(ng::array<T> *source, void (*destructor)(T *)) {
    while (source->count) {
        T corpse = source->pop();
        destructor(&corpse);
    }
    source->release();
}

const auto cgn_w = 6_px;
const auto cgn_h = 8_px;
