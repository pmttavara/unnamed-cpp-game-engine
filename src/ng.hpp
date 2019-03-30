#pragma once // ng.hpp - No-dependency General header
// Define NG_DEFINE in a cpp file to implement this header. The file
// cannot have already been included without NG_DEFINE ealier in the cpp file.
#ifndef null
#define null nullptr
#endif
#ifndef cast
#define cast(...) (__VA_ARGS__) // variadic so templates work
#endif
#define ng_countof(x) (sizeof(x) / sizeof((x)[0]))
#define NG_CAT_(x, y) x##y
#define NG_CAT(x, y) NG_CAT_(x, y)
#define NG_ANON(x) NG_CAT(x, __LINE__) // anonymous variable
#define NG_STRINGIZE_(x) #x
#define NG_STRINGIZE(x) NG_STRINGIZE_(x)
#if defined(_MSC_VER)
#define NG_INLINE __forceinline
#elif defined(__GNUC__)
#define NG_INLINE inline __attribute__((always_inline))
#else
#define NG_INLINE inline
#endif
#ifndef force_inline
#define force_inline NG_INLINE
#endif
#ifdef NG_UNITY_BUILD
#ifndef NG_DEFINE
#define NG_DEFINE
#endif // NG_DEFINE
#ifdef __COUNTER__
// "Global code" is made by specializing a template function on __COUNTER__,
// recursing into the previous counter specialization, then running user code.
// run_global_functions() calls the final specialization at the end of the file
// (so this is only for unity builds). The recursion stops at the base case: a
// no-op specialization of <__COUNTER__> at the beginning of the file. Since
// others might use __COUNTER__, the unspecialized function calls the previous
// specialization (which might also be unspecialized) so that "gaps" in
// specialization are skipped.
#define NG_UNITY_BUILD_BEGIN()                                                 \
    namespace ng {                                                             \
    template <u64 N> static inline void global_fn() { global_fn<N - 1>(); }    \
    template <> static inline void global_fn<__COUNTER__>() {} /* Base case */ \
    void run_global_code(); /* Call at the top of main() */                    \
    }
#define ng_global_(N, ...)                                                     \
    template <> static inline void ng::global_fn<N>() {                        \
        ng::global_fn<N - 1>();                                                \
        __VA_ARGS__;                                                           \
    }
#define ng_global(...) ng_global_(__COUNTER__, __VA_ARGS__)
#define ng_construct(x, ...)                                                   \
    (::new (&(x)) ng::remove_reference<decltype((x))>{__VA_ARGS__})
// Usage: auto ng_init(x, new int[100]);
// RAIIClass ng_init(y); // gets default-constructed with no CRT static init!
#define ng_init(x, ...)                                                        \
    x{__VA_ARGS__};                                                            \
    ng_global(if (!ng::has_static_init()) { ng_construct(x, __VA_ARGS__); });
#define NG_UNITY_BUILD_END()                                                   \
    void ng::run_global_code() { global_fn<__COUNTER__ - 1>(); }
#endif // __COUNTER__
#endif // NG_UNITY_BUILD
///////////////////////////////// Assert macro /////////////////////////////////
#if defined(_MSC_VER)
extern "C" void __cdecl __debugbreak(void);
#define ng_break()                                                             \
    do {                                                                       \
        __debugbreak();                                                        \
    } while (0)
#elif (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
#define ng_break()                                                             \
    do {                                                                       \
        __asm__ __volatile__("int $3\n\t");                                    \
    } while (0)
#else // we tried
#define ng_break()                                                             \
    do {                                                                       \
        (*(int *)0 = 0);                                                       \
    } while (0)
#endif // platform
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define NG_FUNCTION __func__
#elif ((__GNUC__ >= 2) || defined(_MSC_VER))
#define NG_FUNCTION __FUNCTION__
#else
#define NG_FUNCTION ""
#endif // platform
#ifndef NDEBUG
#define ng_assert(e, ...)                                                      \
    if (e) {                                                                   \
    } else {                                                                   \
        ng::print("Assert fired at %0:%1(%2): '%3'\n\t", __FILE__,             \
                  NG_STRINGIZE(__LINE__), NG_FUNCTION, #e);                    \
        ng::print("" __VA_ARGS__);                                             \
        ng::print("\n");                                                       \
        ng_break();                                                            \
    }
#else
#define ng_assert(e, ...)                                                      \
    if (e) {                                                                   \
    } else                                                                     \
        (void)("" __VA_ARGS__);
#endif // NDEBUG
#define ng_fore(x) for (auto &&x)
#define ng_for(e)                                                              \
    ng_fore(it : (e)) if ((it, false)) {}                                      \
    else
#ifndef foreach
#define foreach ng_fore
#endif
#ifndef NDEBUG
struct ng_bool { // Doesn't implicitly coerce to/from goddamn INTEGERS.
    bool b;
    NG_INLINE ng_bool(bool b = false) : b{b} {}
    NG_INLINE ng_bool(ng_bool &b) : b{b} {}
    NG_INLINE ng_bool(ng_bool &&b) : b{b} {}
    NG_INLINE ng_bool(ng_bool const &b) : b{b} {}
    NG_INLINE ng_bool &operator=(ng_bool &o) { return b = o.b, *this; }
    NG_INLINE ng_bool &operator=(ng_bool &&o) { return b = o.b, *this; }
    NG_INLINE ng_bool &operator=(ng_bool const &o) { return b = o.b, *this; }
    template <class T> NG_INLINE ng_bool(T &&) = delete;
    NG_INLINE explicit operator bool() const { return b; }
};
NG_INLINE ng_bool operator!(ng_bool x) { return !x.b; }
NG_INLINE ng_bool operator==(ng_bool a, ng_bool b) { return a.b == b.b; }
NG_INLINE ng_bool operator!=(ng_bool a, ng_bool b) { return a.b != b.b; }
#define ng_true (ng_bool{true})
#define ng_false (ng_bool{false})
#else // The compile-time checking won't affect release perf.
using ng_bool = bool;
#define ng_true true
#define ng_false false
#endif // NDEBUG
namespace ng {
struct dummy {};
template <typename F> struct deferrer {
    F f;
    NG_INLINE ~deferrer() { f(); }
};
template <typename F> NG_INLINE deferrer<F> operator*(dummy, F f) {
    return {f};
}
#define ng_defer auto NG_ANON(_defer) = ng::dummy{} *[&]()
#ifndef defer
#define defer ng_defer
#endif
namespace int_types {
using u8 = unsigned char;
using s8 = signed char;
using u16 = unsigned short;
using s16 = signed short;
using u32 = unsigned int;
using s32 = signed int;
using u64 = unsigned long long;
using s64 = signed long long;
using f32 = float;
using f64 = double;
using usize = decltype(sizeof(void *));
using uptr = usize;
static_assert(sizeof(s32) == 4, "s32 not 32-bit");
static_assert(sizeof(s64) == 8, "s64 not 64-bit");
} // namespace int_types
using namespace int_types;
// Returns whether the CRT initialized global variables at startup before main.
bool has_static_init();
struct true_t {
    enum { value = true };
};
struct false_t {
    enum { value = false };
};
template <class A, class B> struct is_same_impl : false_t {};
template <class T> struct is_same_impl<T, T> : true_t {};
template <class A, class B> constexpr bool is_same = is_same_impl<A, B>::value;
template <bool, class T, class> struct cond_impl { using C = T; };
template <class T, class F> struct cond_impl<false, T, F> { using C = F; };
template <bool b, class T, class F>
using conditional = typename cond_impl<b, T, F>::C;
#define ng_swap(a, b)                                                          \
    do {                                                                       \
        auto c{a};                                                             \
        a = b;                                                                 \
        b = c;                                                                 \
    } while (0)
#define ng_min(a, b) ((a) < (b) ? (a) : (b))
#define ng_max(a, b) ((a) > (b) ? (a) : (b))
#define ng_clamp(t, min, max) ((t) > (min) ? (t) < (max) ? (t) : (max) : (min))
#define ng_lerp(a, b, t) ((a) * (1 - (t)) + (b) * (t))
#define ng_abs(x) ((x) > 0 ? (x) : -(x))
template <class T> NG_INLINE void swap(T &a, T &b) { ng_swap(a, b); }
template <class T> NG_INLINE auto(min)(T a, T b) { return ng_min(a, b); }
template <class T> NG_INLINE auto(max)(T a, T b) { return ng_max(a, b); }
template <class T> NG_INLINE T clamp(T t, T min, T max) {
    return ng_clamp(t, min, max);
}
template <class T> NG_INLINE T lerp(T a, T b, float t) {
    return ng_lerp(a, b, t);
}
#if 0 // ndef NG_NO_EXTERNS
s8 abs(s8 value);
s16 abs(s16 value);
s32 abs(s32 value);
s64 abs(s64 value);
float abs(float value);
double abs(double value);
#else
NG_INLINE s8 abs(s8 x) { return ng_abs(x); }
NG_INLINE s16 abs(s16 x) { return ng_abs(x); }
NG_INLINE s32 abs(s32 x) { return ng_abs(x); }
NG_INLINE s64 abs(s64 x) { return ng_abs(x); }
NG_INLINE float abs(float value) {
    auto punned = (u32 &)value & 0x7FFFFFFF;
    return (float &)punned;
}
NG_INLINE double abs(double value) {
    auto punned = (u64 &)value & 0x7FFFFFFFFFFFFFFF;
    return (double &)punned;
}
#endif
float mod(float x, float y);
double mod(double x, double y);
float floor(float value);
double floor(double value);
float ceil(float value);
double ceil(double value);
float round(float value);
double round(double value);
float sqrt(float value);
double sqrt(double value);
float pow(float base, float exponent);
double pow(double base, double exponent);
float sin(float x);
double sin(double x);
float cos(float x);
double cos(double x);
float tan(float x);
double tan(double x);
float atan2(float y, float x);
double atan2(double y, double x);
// template <class T = float> constexpr auto pi() {
//     return (T)3.1415926535897932384626433832795028841971;
// }
constexpr static const f32 TAU32 = 6.28318530717958647692528676655900576839434f;
constexpr static const f64 TAU64 = 6.283185307179586476925286766559005768394338;
#ifndef NG_NO_EXTERNS
void *malloc(usize size);
void *realloc(void *ptr, usize new_size);
void free(void *ptr);
void *memset(void *destination, u8 value, usize size);
void *memcpy(void *destination, const void *source, usize size);
void *memmove(void *destination, const void *source, usize size);
usize strlen(const char *c_string);
int strcmp(const char *c_string_a, const char *c_string_b);
int strncmp(const char *c_string_a, const char *c_string_b, usize size);
int memcmp(const void *memblock_a, const void *memblock_b, usize size);
#else
NG_INLINE void *memset(void *destination, u8 value, usize size) {
    while (size > 0) {
        size -= 1;
        ((u8 *)destination)[size] = value;
    }
    return destination;
}
NG_INLINE void *memcpy(void *destination, const void *source, usize size) {
    for (usize i = 0; i < size; i += 1)
        ((u8 *)destination)[i] = ((u8 *)source)[i];
    return destination;
}
NG_INLINE void *memmove(void *destination, const void *source, usize size) {
    u8 *d = (u8 *)destination;
    u8 *s = (u8 *)source;
    if (s == d) {
        return destination;
    }
    if (s < d) {
        // copy from back
        s += size - 1;
        d += size - 1;
        while (size) {
            *d = *s;
            d -= 1;
            s -= 1;
            size -= 1;
        }
    } else {
        // copy from front
        while (size)
            *d = *s;
        d += 1;
        s += 1;
        size -= 1;
    }
    return destination;
}
NG_INLINE usize strlen(const char *s) {
    usize len = 0;
    while (s[len] != '\0')
        len += 1;
    return len;
}
NG_INLINE int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) {
        a += 1;
        b += 1;
    }
    return *(u8 *)a - *(u8 *)b;
}
NG_INLINE int strncmp(const char *a, const char *b, usize n) {
    while (n > 0 && *a != '\0' && *a == *b) {
        a += 1;
        b += 1;
        n -= 1;
    }
    if (n > 0) {
        return *(u8 *)a - *(u8 *)b;
    } else {
        return 0;
    }
}
NG_INLINE int memcmp(const void *a, const void *b, usize n) {
    auto u1 = (u8 *)a, u2 = (u8 *)b;
    for (; n > 0; u1 += 1, u2 += 1) {
        if (*u1 != *u2) {
            return *u1 - *u2;
        }
        n -= 1;
    }
    return 0;
}
#endif // NG_NO_EXTERNS
struct mt19937_64 {
    enum { n = 312 };
    u64 index = 0;
    u64 data[n];
    mt19937_64(u64 seed = 5489) { this->seed(seed); }
    void seed(u64 seed = 5489);
    u64 next_u64();
    // yields a float in [0, 1)
    NG_INLINE double next_double() {
        return (next_u64() >> 11) * (1.0 / (1ll << 53));
    }
    // yields a float in [0, 1)
    NG_INLINE float next_float() {
        return (next_u64() >> 32) * (1.f / (1ll << 32));
    }
};
} // namespace ng
#ifdef _WIN32
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
#define NG_DO_PLACE_NEW
#endif // __PLACEMENT_NEW_INLINE
#ifndef __PLACEMENT_VEC_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE
#define NG_DO_PLACE_NEW_ARR
#endif // __PLACEMENT_VEC_NEW_INLINE
#else
#define NG_DO_PLACE_NEW
#define NG_DO_PLACE_NEW_ARR
#endif // platform
#ifdef NG_DO_PLACE_NEW
NG_INLINE void *operator new(ng::usize, void *location) { return location; }
NG_INLINE void operator delete(void *, void *) {}
#endif // NG_DO_PLACE_NEW
#ifdef NG_DO_PLACE_NEW_ARR
void *operator new[](ng::usize, void *) = delete;
void operator delete[](void *, void *) = delete;
#endif // NG_DO_PLACE_NEW_ARR
#undef NG_DO_PLACE_NEW
#undef NG_DO_PLACE_NEW_ARR
#ifdef NG_DEFINE
// This declaration means the compiler will generate code to execute the
// function as part of the initializers for global static variables. This
// function cannot be optimized away into any direct memory, so the variable
// will have the value 0 (false) like all other variables in BSS until CRT code
// runs the static initializers. If there is no CRT (for example when compiling
// with cl.exe -nodefaultlib) then the variable will stay 0.
static bool run_static_init() {
    volatile bool result = true;
    return result;
}
static bool has_static_init_impl = run_static_init();
bool ng::has_static_init() { return has_static_init_impl; }
#ifndef NG_NO_EXTERNS
#if defined(_WIN32)
#define NG_CDECL __cdecl
#ifdef _DLL
#define NG_DECLSPEC __declspec(dllimport)
#else
#define NG_DECLSPEC
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define NG_CDECL
#define NG_DECLSPEC
#else
#error This platform is not supported.
#endif // platform
extern "C" {
int NG_CDECL abs(int);
long NG_CDECL labs(long);
// long long NG_CDECL llabs(long long);
double NG_CDECL fabs(double);
float NG_CDECL fabsf(float);
double NG_CDECL fmod(double, double);
NG_DECLSPEC float NG_CDECL fmodf(float, float);
NG_DECLSPEC double NG_CDECL floor(double);
NG_DECLSPEC float NG_CDECL floorf(float);
NG_DECLSPEC double NG_CDECL ceil(double);
NG_DECLSPEC float NG_CDECL ceilf(float);
NG_DECLSPEC double NG_CDECL round(double);
NG_DECLSPEC float NG_CDECL roundf(float);
double NG_CDECL sqrt(double);
NG_DECLSPEC float NG_CDECL sqrtf(float);
double NG_CDECL pow(double, double);
NG_DECLSPEC float NG_CDECL powf(float, float);
double NG_CDECL sin(double);
NG_DECLSPEC float NG_CDECL sinf(float);
double NG_CDECL cos(double);
NG_DECLSPEC float NG_CDECL cosf(float);
double NG_CDECL tan(double);
NG_DECLSPEC float NG_CDECL tanf(float);
double NG_CDECL atan2(double, double);
NG_DECLSPEC float NG_CDECL atan2f(float, float);
NG_DECLSPEC __declspec(restrict) void *NG_CDECL malloc(ng::usize);
NG_DECLSPEC __declspec(restrict) void *NG_CDECL realloc(void *, ng::usize);
NG_DECLSPEC void NG_CDECL free(void *);
NG_DECLSPEC int NG_CDECL printf(const char *, ...);
NG_DECLSPEC void *NG_CDECL memset(void *, int, ng::usize);
NG_DECLSPEC void *NG_CDECL memcpy(void *, void const *, ng::usize);
NG_DECLSPEC void *NG_CDECL memmove(void *, void const *, ng::usize);
NG_DECLSPEC ng::usize NG_CDECL strlen(const char *);
NG_DECLSPEC int NG_CDECL strcmp(const char *, const char *);
NG_DECLSPEC int NG_CDECL memcmp(const void *, const void *, ng::usize);
NG_DECLSPEC int NG_CDECL strncmp(const char *, const char *, ng::usize);
}
namespace ng {
#if 0 // ndef NG_NO_EXTERNS
s8 abs(s8 value) { return (s8)::abs((int)value); }
s16 abs(s16 value) { return (s16)::abs((int)value); }
s32 abs(s32 value) { return (s32)::labs((long)value); }
// s64 abs(s64 value) { return (s64)::llabs((long long)value); }
float abs(float value) { return ::fabsf(value); }
double abs(double value) { return ::fabs(value); }
#endif
float mod(float x, float y) { return ::fmodf(x, y); }
double mod(double x, double y) { return ::fmod(x, y); }
float floor(float value) { return ::floorf(value); }
double floor(double value) { return ::floor(value); }
float ceil(float value) { return ::ceilf(value); }
double ceil(double value) { return ::ceil(value); }
float round(float value) { return (f32)::round(value); }
// double round(double value) { return ::round(value); }
float sqrt(float value) { return ::sqrtf(value); }
double sqrt(double value) { return ::sqrt(value); }
float pow(float base, float exp) { return ::powf(base, exp); }
double pow(double base, double exp) { return ::pow(base, exp); }
float sin(float x) { return ::sinf(x); }
double sin(double x) { return ::sin(x); }
float cos(float x) { return ::cosf(x); }
double cos(double x) { return ::cos(x); }
float tan(float x) { return ::tanf(x); }
double tan(double x) { return ::tan(x); }
float atan2(float y, float x) { return ::atan2f(y, x); }
double atan2(double y, double x) { return ::atan2(y, x); }
void *malloc(usize size) { return ::malloc(size); }
void *realloc(void *ptr, usize new_size) { return ::realloc(ptr, new_size); }
void free(void *ptr) { ::free(ptr); }
void *memset(void *destination, u8 value, usize size) {
    return ::memset(destination, (int)value, size);
}
void *memcpy(void *destination, const void *source, usize size) {
    return ::memcpy(destination, source, size);
}
void *memmove(void *destination, const void *source, usize size) {
    return ::memmove(destination, source, size);
}
usize strlen(const char *s) { return ::strlen(s); }
int strcmp(const char *a, const char *b) { return ::strcmp(a, b); }
int memcmp(const void *a, const void *b, usize n) { return ::memcmp(a, b, n); }
int strncmp(const char *a, const char *b, usize n) {
    return ::strncmp(a, b, n);
}
} // namespace ng
#endif // NG_NO_EXTERNS
namespace ng {
namespace mt19937_constants {
enum : u64 {
    w = 64,
    n = 312,
    m = 156,
    r = 31,
    a = 0xB5026F5AA96619E9,
    u = 29,
    d = 0x5555555555555555,
    s = 17,
    b = 0x71D67FFFEDA60000,
    t = 37,
    c = 0xFFF7EEE000000000,
    l = 43,
    f = 6364136223846793005,
    lower_mask = 0x7FFFFFFF,
    upper_mask = ~lower_mask,
};
}
void mt19937_64::seed(u64 seed) {
    using namespace mt19937_constants;
    index = n + 1;
    data[0] = seed;
    for (u64 i = 1; i < n; ++i) {
        data[i] = (f * (data[i - 1] ^ (data[i - 1] >> (w - 2))) + i);
    }
}
u64 mt19937_64::next_u64() {
    using namespace mt19937_constants;
    if (index == 0 || index > n + 1) {
        // uninitialized
        seed();
    }
    if (index > n) {
        // do the twist
        for (u64 i = 0; i < n; ++i) {
            u64 x = (data[i] & upper_mask) + (data[(i + 1) % n] & lower_mask);
            u64 xA = x >> 1;
            if (x % 2 != 0) {
                xA ^= a;
            }
            data[i] = data[(i + m) % n] ^ xA;
        }
        index = 1;
    }
    u64 y = data[index - 1];
    y = y ^ ((y >> u) & d);
    y = y ^ ((y << s) & b);
    y = y ^ ((y << t) & c);
    y = y ^ (y >> l);
    index += 1;
    return y;
}
} // namespace ng
#endif // NG_DEFINE
namespace ng {
// print functions
struct print_buffer {
    u8 *str = nullptr;
    u64 len = 0;
    u64 cap = 0;
    void putchar(char c);
};
using print_buffer_proc = void(print_buffer);
extern print_buffer_proc *output_print_buffer;
struct format_int {
    s64 x = 0;
    bool is_signed = false;
    int leading_zeroes = 0;
    int leading_spaces = 0;
    int radix = 10;
    format_int(s64 x, bool is_signed = true, int radix = 10,
               int leading_zeroes = 0, int leading_spaces = 0);
};
struct format_char {
    u32 c; // @Unicode: support in constructor etc.
    format_char(char x) { c = x; }
};
struct format_float {
    f64 x;
    int num_chars;
    format_float(f64 x, int num_chars);
    void print(print_buffer *buffer);
};
void print_init();
void print_item(print_buffer *buf, print_buffer src);
void print_item(print_buffer *buf, bool b);
void print_item(print_buffer *buf, ng_bool b);
void print_item(print_buffer *buf, s8 i);
void print_item(print_buffer *buf, u8 u);
void print_item(print_buffer *buf, s16 i);
void print_item(print_buffer *buf, u16 u);
void print_item(print_buffer *buf, s32 i);
void print_item(print_buffer *buf, u32 u);
void print_item(print_buffer *buf, s64 i);
void print_item(print_buffer *buf, u64 u);
void print_item(print_buffer *buf, f32 f);
void print_item(print_buffer *buf, f64 f);
void print_item(print_buffer *buf, const format_int &fmt);
void print_item(print_buffer *buf, const format_float &fmt);
void print_item(print_buffer *buf, const format_char &fmt);
void print_item(print_buffer *buf, const char *ptr);
void print_item(print_buffer *buf, void *ptr);
template <class T> NG_INLINE int print_get_item_size(const T &) { return -1; }
int print_get_item_size(print_buffer src);
int print_get_item_size(bool b);
int print_get_item_size(s8 i);
int print_get_item_size(u8 u);
int print_get_item_size(s16 i);
int print_get_item_size(u16 u);
int print_get_item_size(s32 i);
int print_get_item_size(u32 u);
int print_get_item_size(s64 i);
int print_get_item_size(u64 u);
int print_get_item_size(f32 f);
int print_get_item_size(f64 f);
int print_get_item_size(const char *ptr);
int print_get_item_size(void *ptr);
// clang-format off
template <u64 A, u64 B, u64... Rest> constexpr u64 max_of = (A > B) ? max_of<A, Rest...> : max_of<B, Rest...>;
template <u64 A, u64 B> constexpr u64 max_of<A, B> = (A > B) ? A : B;
template <class Who, class Guy, class... Guys> constexpr int variant_tag() {
    if constexpr(sizeof...(Guys) > 0) {
        if constexpr(is_same<Guy, Who>) {
            return sizeof...(Guys);
        } else {
            return variant_tag<Who, Guys...>();
        }
    } else {
        if constexpr(is_same<Guy, Who>) {
            return 0;
        } else {
            return -1;
        }
    }
}
template <u64 N> using smallest_tag = conditional<(N < 1ull <<  8),  u8,
                                      conditional<(N < 1ull << 16), u16,
                                      conditional<(N < 1ull << 32), u32, u64>>>; // clang-format on
template <class... Guys> struct variant {
    using type_tag = smallest_tag<sizeof...(Guys)>;
    type_tag type = -1;
    alignas(max_of<0, alignof(Guys)...>) char data[max_of<0, sizeof(Guys)...>];
    template <class T> static void verify() {
        static_assert(variant_tag<T, Guys...>() != -1, "not a variant member");
    }
    template <class T> NG_INLINE bool is() {
        return verify<T>(), type == variant_tag<T, Guys...>();
    }
    template <class T> NG_INLINE T &as() {
        return verify<T>(), reinterpret_cast<T &>(data);
    }
    variant() = default;
    template <class T> NG_INLINE variant(T t) {
        (T &)data = t;
        type = variant_tag<T, Guys...>();
    }
};
/* clang-format off */ #define NG_VARIANT_GENERIC_FUNCTION(ret, name, params, args, Ty, ...) template <class Ty, class... Ty##s> ret name params { auto code = [&] { __VA_ARGS__; }; if constexpr(sizeof...(Ty##s) > 0) if (type < 0) ng_break(); else if (type == variant_tag<Ty, Ty, Ty##s...>()) return code(); else return name<Ty##s...> args; else if (type != 0) ng_break(); return code(); } /* clang-format on */
NG_VARIANT_GENERIC_FUNCTION(
    void, print_variant, (print_buffer * buf, int type, char *data),
    (buf, type, data), T, print_item(buf, *reinterpret_cast<const T &>(*data)));
NG_VARIANT_GENERIC_FUNCTION(
    int, print_get_variant_size, (int type, char *data), (type, data), T,
    return print_get_item_size(*reinterpret_cast<const T &>(*data)));
bool is_alpha(char c);
bool is_digit(char c);
bool str_to_s64(const char **str, s64 *result);
template <class... Args> void print(const char *fmt, const Args &... args) {
    if (!output_print_buffer) {
        return;
    }
    print_buffer buffer = mprint(fmt, args...);
    output_print_buffer(buffer);
    free(buffer.str);
}
template <class... Args>
u64 print_get_size(const char *fmt, const Args &... args) {
    auto n = ng::strlen(fmt);
    if
        constexpr(sizeof...(Args) == 0) { return n; }
    else {
        u64 total = 0;
        constexpr auto num_args = (s64)sizeof...(Args);
        variant<const Args *...> arg_ptrs[num_args] = {&args...};
        for (auto p = &fmt[0]; p < &fmt[n];) {
            if (*p == '%') {
                auto new_p = p + 1;
                s64 arg_index = 0;
                if (str_to_s64(&new_p, &arg_index)) {
                    if (arg_index < 0) {
                        arg_index += num_args;
                    }
                    if (arg_index >= 0 && arg_index < num_args) {
                        auto arg = arg_ptrs[arg_index];
                        int given = print_get_variant_size<const Args *...>(
                            arg.type, arg.data);
                        if (given < 0) {
                            print_buffer size_calculator = {};
                            print_variant<const Args *...>(&size_calculator,
                                                           arg.type, arg.data);
                            total += size_calculator.len;
                        } else {
                            total += given;
                        }
                        p = new_p;
                        continue;
                    }
                }
            }
            p += 1;
            total += 1;
        }
        return total;
    }
}
template <class... Args>
void sprint(print_buffer *buffer, const char *fmt, const Args &... args) {
    auto n = ng::strlen(fmt);
    if
        constexpr(sizeof...(Args) == 0) { print_item(buffer, fmt); }
    else {
        constexpr auto num_args = (s64)sizeof...(Args);
        variant<const Args *...> arg_ptrs[num_args] = {&args...};
        for (auto p = &fmt[0]; p < &fmt[n];) {
            if (*p == '%') {
                auto new_p = p + 1;
                s64 arg_index = 0;
                if (str_to_s64(&new_p, &arg_index)) {
                    if (arg_index < 0) {
                        arg_index += num_args;
                    }
                    if (arg_index >= 0 && arg_index < num_args) {
                        auto arg = arg_ptrs[arg_index];
                        print_variant<const Args *...>(buffer, arg.type,
                                                       arg.data);
                        p = new_p;
                        continue;
                    }
                }
            }
            buffer->putchar(*p);
            p += 1;
        }
    }
}
// Free the buffer returned by this using ng::free(buffer.str)
template <class... Args>
print_buffer mprint(const char *fmt, const Args &... args) {
    print_buffer result = {};
    result.cap = print_get_size(fmt, args...);
    result.str = (u8 *)malloc(result.cap);
    if (result.str != nullptr) {
        sprint(&result, fmt, args...);
    }
    return result;
}
#ifdef NG_DEFINE
void ng::print_buffer::putchar(char c) { // @Unicode: should take u32
    if (len < cap) {
        str[len] = c;
    }
    len += 1; // so you can still tell how much it would have written
}
#ifdef _WIN32
#pragma comment(lib, "kernel32.lib")
extern "C" {
extern NG_DECLSPEC int __stdcall WriteFile(void *hFile, const void *lpBuffer,
                                           unsigned int nNumberOfBytesToWrite,
                                           unsigned int *lpNumberOfBytesWritten,
                                           struct _OVERLAPPED *lpOverlapped);
extern NG_DECLSPEC void *__stdcall GetStdHandle(unsigned int nStdHandle);
}
static void write_print_buffer_default(print_buffer buffer) {
    u32 bytes_written;
    u32 size_to_write = (u32)((min)(buffer.len, buffer.cap));
    WriteFile(GetStdHandle(-11), buffer.str, size_to_write, &bytes_written,
              nullptr);
}
#else
#ifdef NG_NO_EXTERNS
#error This platform is not yet supported without the C Runtime.
#endif // NG_NO_EXTERNS
static void write_print_buffer_default(print_buffer buffer) {
    ::printf("%.*s", (int)buffer.len, buffer.str);
}
#endif // platform
print_buffer_proc *output_print_buffer = &write_print_buffer_default;
void print_init() {
    if (!has_static_init()) {
        output_print_buffer = &write_print_buffer_default;
    }
}
bool is_digit(char c) { return c >= '0' && c <= '9'; }
bool str_to_s64(const char **str, s64 *result) {
    if (!str || !*str || !result) {
        return false;
    }
    s64 number = 0;
    bool negative = false;
    if (**str == '-') {
        negative = true;
        *str += 1;
    }
    bool is_number = false;
    for (; is_digit(**str); *str += 1) {
        is_number = true;
        number = (number * 10) + (**str - '0');
        if (number < 0) {
            return false; // overflowed: too many digits
        }
    }
    if (negative) {
        number = -number;
    }
    if (is_number) {
        *result = number;
    }
    return is_number;
}
format_int::format_int(s64 x, bool is_signed, int radix, int leading_zeroes,
                       int leading_spaces) {
    this->x = x;
    this->is_signed = is_signed;
    this->leading_zeroes = leading_zeroes;
    this->leading_spaces = leading_spaces;
    this->radix = radix;
}
#if 0
void print_item(print_buffer *buf, const format_int &fmt) {
    static const u64 divisors[] = {
        9223372036854775808u, 12157665459056928801u, 4611686018427387904u,
        7450580596923828125u, 4738381338321616896u,  3909821048582988049u,
        9223372036854775808u, 12157665459056928801u, 10000000000000000000u,
        5559917313492231481u, 2218611106740436992u,  8650415919381337933u,
        2177953337809371136u, 6568408355712890625u,  1152921504606846976u,
        2862423051509815793u, 6746640616477458432u,  799006685782884121u,
        1638400000000000000u, 3243919932521508681u,  6221821273427820544u,
        504036361936467383u,  876488338465357824u,   1490116119384765625u,
        2481152873203736576u,
    }; // b^floor(log_b(2^63)) for various b
    auto radix = fmt.radix;
    if (radix < 2 || radix > (int)(sizeof(divisors) / sizeof(u64))) {
        return;
    }
    auto x = fmt.x;
    if (x < 0 && fmt.is_signed) {
        buf->putchar('-');
        x = -x;
    }
    auto ux = (u64)x;
    auto divisor = divisors[radix - 2];
    int num_printed = 0;
    bool print_zero = false;
    do {
        auto v = ux / divisor;
        int digit = v % radix;
        if (digit > 0 || print_zero || num_printed < fmt.leading_zeroes || divisor == 1) {
            print_zero = true;
            buf->putchar("0123456789"
                         "abcdefghijklmnopqrstuvwxyz"
                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "@$"[digit]);
        }
    } while (divisor /= radix);
}
#endif
int print_get_item_size(const format_int &) { return 64; } // should be log(x)
void print_item(print_buffer *buf, const format_int &fmt) {
    auto radix = fmt.radix;
    if (radix < 2 || radix > 64) {
        return;
    }
    auto x = fmt.x;
    if (x < 0 && fmt.is_signed) {
        buf->putchar('-');
        x = -x;
    }
    auto ux = (u64)x;
    char num_buffer[64];
    const auto end = &num_buffer[ng_countof(num_buffer)];
    auto p = end - 1;
    bool has_printed_nonzeroes = false;
    do {
        int digit = ux % radix;
        if (digit > 0 || has_printed_nonzeroes || p == end - 1) {
            *p = "0123456789"
                 "abcdefghijklmnopqrstuvwxyz"
                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                 "@$"[digit];
            has_printed_nonzeroes = true;
            p -= 1;
        }
    } while (ux /= radix);
    for (; p > end - fmt.leading_zeroes - 1; p -= 1) {
        *p = '0';
    }
    for (; p > end - fmt.leading_zeroes - 1 - fmt.leading_spaces; p -= 1) {
        *p = ' ';
    }
    for (p += 1; p < end; p += 1) {
        buf->putchar(*p);
    }
}
format_float::format_float(f64 x, int num_chars) {
    this->x = x;
    this->num_chars = num_chars;
}
int print_get_item_size(const format_float &f) { return f.num_chars; }
void print_item(print_buffer *buf, const format_float &fmt) {
    auto num_chars = fmt.num_chars;
    if (num_chars < 6) {
        return;
    }
    auto x = fmt.x;
    if ((u64 &)x & 0x8000000000000000ull) {
        buf->putchar('-');
        num_chars -= 1;
        x = -x;
    }
    if (x == 0.0) {
        buf->putchar('0');
        return;
    }
    if (((u64 &)x >> 52 & 0x7ff) == 0x7ff) {
        print_item(buf, x == x ? "inf" : "nan");
        return;
    }
    if (x > pow(10.0, (max)(0, num_chars - 1)) ||
        x < pow(10.0, (min)(0, 5 - num_chars))) { // @Todo: scientific notation
        while (num_chars > 5) {
            buf->putchar(' ');
            num_chars -= 1;
        }
        print_item(buf, "(sci)");
    } else { // fits w/full precision
        auto whole_part = static_cast<u64>(x);
        auto old_len = buf->len;
        print_item(buf, format_int(whole_part));
        num_chars -= (s64)(buf->len - old_len);
        x -= whole_part;
        if (x > 0 && num_chars > 2) {
            buf->putchar('.');
            num_chars -= 1;
            while (x != 0 && num_chars > 0) {
                x *= 10;
                s64 digit = (int)x;
                buf->putchar(digit + '0');
                num_chars -= 1;
                x -= digit;
            }
        }
    }
}
void print_item(print_buffer *buf, const format_char &fmt) {
    buf->putchar(fmt.c);
}
void print_item(print_buffer *buf, print_buffer src) {
    // The user requested that we print this buffer, so if len is longer than
    // cap, then this buffer is technically invalid, but there's nothing we can
    // really do about it.
    u64 len = (min)(src.len, src.cap);
    for (u64 i = 0; i < len; i += 1) {
        buf->putchar(src.str[i]);
    }
}
int print_get_item_size(print_buffer src) { return (min)(src.len, src.cap); }
void print_item(print_buffer *buf, bool b) {
    print_item(buf, b ? "true" : "false");
}
void print_item(print_buffer *buf, ng_bool b) { print_item(buf, b.b); }
int print_get_item_size(bool) { return 5; }
void print_item(print_buffer *buf, s8 i) { print_item(buf, (s64)i); }
int print_get_item_size(s8) { return 8; }
void print_item(print_buffer *buf, u8 u) { print_item(buf, (u64)u); }
int print_get_item_size(u8) { return 8; }
void print_item(print_buffer *buf, s16 i) { print_item(buf, (s64)i); }
int print_get_item_size(s16) { return 16; }
void print_item(print_buffer *buf, u16 u) { print_item(buf, (u64)u); }
int print_get_item_size(u16) { return 16; }
void print_item(print_buffer *buf, s32 i) { print_item(buf, (s64)i); }
int print_get_item_size(s32) { return 32; }
void print_item(print_buffer *buf, u32 u) { print_item(buf, (u64)u); }
int print_get_item_size(u32) { return 32; }
void print_item(print_buffer *buf, s64 i) { print_item(buf, format_int(i)); }
int print_get_item_size(s64) { return 64; }
void print_item(print_buffer *buf, u64 u) {
    print_item(buf, format_int(u, false));
}
int print_get_item_size(u64) { return 64; }
void print_item(print_buffer *buf, void *ptr) {
    print_item(buf, format_int((uptr)ptr, false, 16));
}
int print_get_item_size(void *) { return 64; } // @Volatile: platform
constexpr auto f32_chars = 9 + sizeof('-') + sizeof('.');
void print_item(print_buffer *buf, f32 f) {
    print_item(buf, format_float(f, f32_chars));
}
int print_get_item_size(f32) { return f32_chars; }
constexpr auto f64_chars = 17 + sizeof('-') + sizeof('.');
void print_item(print_buffer *buf, f64 f) {
    print_item(buf, format_float(f, f64_chars));
}
int print_get_item_size(f64) { return f64_chars; }
void print_item(print_buffer *buf, const char *ptr) {
    if (ptr) {
        while (*ptr != '\0') {
            buf->putchar(*ptr);
            ptr += 1;
        }
    } else {
        print_item(buf, "(null)");
    }
}
int print_get_item_size(const char *ptr) { return ptr ? strlen(ptr) : 6; }
#endif // NG_DEFINE
// allocator construct
enum struct allocate_mode { allocate = 0, resize = 1, free = 2, free_all = 3 };
using allocator_proc = void *(allocate_mode mode, struct allocator *self,
                              u64 size, u64 old_size, void *block, s64 options);
struct allocator {
    allocator_proc *proc;
    NG_INLINE void *allocate(u64 size, s64 options = 0) {
        return proc(allocate_mode::allocate, this, size, 0, nullptr, options);
    }
    NG_INLINE void *resize(void *block, u64 new_size, u64 old_size,
                           s64 options = 0) {
        return proc(allocate_mode::resize, this, new_size, old_size, block,
                    options);
    }
    NG_INLINE void free(void *block, u64 old_size, s64 options = 0) {
        proc(allocate_mode::free, this, 0, old_size, block, options);
    }
    NG_INLINE void free_all(s64 options = 0) {
        proc(allocate_mode::free_all, this, 0, 0, nullptr, options);
    }
    template <class T> T *make_new() { return new (allocate(sizeof(T))) T{}; }
    template <class T> void make_delete(T *t) {
        t->~T();
        free((void *)t, sizeof(T));
    }
    template <class T> T *make_new_array(u64 count) {
        auto result = (T *)allocate(count * sizeof(T));
        for (u64 i = 0; i < count; i += 1) {
            new (result + i) T{};
        }
        return result;
    }
    template <class T> void make_delete_array(T *data, u64 count) {
        free(data, count * sizeof(T));
    }
};
extern allocator *default_allocator;
#define ng_allocator_constructor()                                             \
    allocator {                                                                \
        [](allocate_mode m, allocator *self, u64 s, u64 os, void *b, s64 o) {  \
            return ((decltype(this))self)->call(m, s, os, b, o);               \
        }                                                                      \
    }
struct mallocator : allocator {
    mallocator() : ng_allocator_constructor() {}
    void *call(allocate_mode mode, u64 n, u64 old_n, void *blk, s64 opt);
};
struct stack_allocator : allocator {
    constexpr static auto STACK_SIZE = 4096 * 8; // 8 pages most of the time
    alignas(8) char data[STACK_SIZE];
    char *head = data;
    stack_allocator() : ng_allocator_constructor() {}
    void *call(allocate_mode mode, u64 n, u64 old_n, void *blk, s64 opt);
};
struct pool_allocator : allocator {
    allocator *parent = {};
    void *pool = nullptr;
    char *head = nullptr;
    pool_allocator() : ng_allocator_constructor() {}
    void *call(allocate_mode mode, u64 n, u64 old_n, void *blk, s64 opt);
};
pool_allocator create_pool(usize pool_size = 4096 * 32,
                           allocator *parent = default_allocator);
void release_pool(pool_allocator *pool);
struct proxy_allocator : allocator {
    allocator *parent = {};
    char *name = nullptr;
    u64 allocations = 0;
    u64 allocated_bytes = 0;
    proxy_allocator() : ng_allocator_constructor() {}
    void *call(allocate_mode mode, u64 n, u64 old_n, void *blk, s64 opt);
};
proxy_allocator create_proxy_allocator(char *name,
                                       allocator *parent = default_allocator);
void destroy_proxy_allocator(proxy_allocator *proxy);
#ifdef NG_DEFINE
static mallocator default_allocator_ = {};
allocator *default_allocator = &default_allocator_;
static void allocator_init() {
    if (!has_static_init()) {
        default_allocator_ = {};
        default_allocator = &default_allocator_;
    }
}
void *mallocator::call(allocate_mode mode, u64 size, u64, void *old_block,
                       s64) {
    void *result = nullptr;
    switch (mode) {
    case (allocate_mode::allocate): {
        ng_assert(size > 0);
        result = malloc(size);
    } break;
    case (allocate_mode::resize): {
        result = realloc(old_block, size);
    } break;
    case (allocate_mode::free): {
        ng_assert(old_block != nullptr);
        ng::free(old_block);
    } break;
    default:
    case (allocate_mode::free_all): { // nop
    }
    }
    return result;
}
void *stack_allocator::call(allocate_mode mode, u64 size, u64 old_size,
                            void *old_block, s64) {
    switch (mode) {
    case (allocate_mode::allocate): {
        ng_assert(size > 0);
        if (head + size > data + STACK_SIZE) { // request too big
            return nullptr;
        } else {
            auto result = (void *)head;
            head += size;
            return result;
        }
    } break;
    case (allocate_mode::resize): {
        if ((char *)old_block + old_size == head) { // Most recent alloc
            if (head - old_size + size > data + STACK_SIZE) {
                // request is too big, even eating the old memory
                return nullptr;
            } else { // grow old block (or, technically, maybe shrink)
                head -= old_size;
                head += size;
                return old_block;
            }
        } else if (head + size > data + STACK_SIZE) {
            // The request is too big
            return nullptr;
        } else {
            if (size <= old_size) { // new block fits in old one
                // can't move head; block is somewhere in middle of stack
                return old_block;
            } else {
                auto result = (void *)head;
                head += size;
                if (old_block) {
                    memcpy(result, old_block, old_size);
                }
                return result;
            }
        }
    } break;
    case (allocate_mode::free): {
        // nop
    } break;
    case (allocate_mode::free_all): {
        head = data;
    } break;
    }
    return nullptr;
}
// struct Allocation { void *ptr; u64 len; char *name;};
// static Allocation allocations[1024 * 1024] = {}; // @Hack
// static int cur_allocation = 0;
// static void pushalloc(void *blk, u64 len, char *c) {
//     ng_assert(blk != nullptr && len > 0);
//     allocations[cur_allocation++] = {blk, len, c};
//     if (cur_allocation >= 1024 * 1024) ng_break();
// }
// static void dealloc(void *old_block, u64 old_len, char *) {
//     ng_assert(old_block != nullptr && old_len > 0);
//     ng_for(allocations) {
//         if (it.ptr == old_block && it.len == old_len) {
//             it = {};
//         }
//     }
// }
// static void realloc(void *old_block, u64 old_len,
// void *new_block, u64 new_len, char *c) {
//     dealloc(old_block, old_len, c);
//     pushalloc(new_block, new_len, c);
// }
void *proxy_allocator::call(allocate_mode mode, u64 size, u64 old_size,
                            void *old_block, s64 options) {
    auto result =
        parent->proc(mode, parent, size, old_size, old_block, options);
    switch (mode) {
    case (allocate_mode::allocate): {
        if (result) { // succeeded
            allocations += 1;
            allocated_bytes += size;
            // pushalloc(result, size, name);
        }
    } break;
    case (allocate_mode::resize): {
        if (result) {         // succeeded
            if (!old_block) { // is a new allocation
                ng_assert(old_size == 0);
                allocations += 1;
                allocated_bytes += size;
                // pushalloc(result, size, name);
            } else {
                allocated_bytes -= old_size;
                allocated_bytes += size;
                // realloc(old_block, old_size, result, size, name);
            }
        } else {
            if (size == 0 && old_block) { // is a free
                allocations -= 1;
                // dealloc(old_block, old_size, name);
            } else {
                // failed; old allocation is still valid
            }
        }
    } break;
    case (allocate_mode::free): {
        ng_assert(allocations > 0);
        allocations -= 1;
        ng_assert(allocated_bytes >= old_size);
        allocated_bytes -= old_size;
        // dealloc(old_block, old_size, name);
    } break;
    case (allocate_mode::free_all): {
        allocations = 0;
        allocated_bytes = 0;
    } break;
    }
    return result;
}
proxy_allocator create_proxy_allocator(char *name, allocator *parent) {
    proxy_allocator result = {};
    result.parent = parent;
    result.name = name;
    return result;
}
void destroy_proxy_allocator(proxy_allocator *proxy) {
    ng_assert(proxy->allocations == 0 && proxy->allocated_bytes == 0,
              "Allocator '%0' leaked %1 bytes over %2 allocations.\n",
              proxy->name, proxy->allocated_bytes, proxy->allocations);
}
#endif // NG_DEFINE
// string class
struct string {
    u64 len = 0;
    u8 *ptr = nullptr;
    NG_INLINE constexpr string() = default;
    NG_INLINE constexpr string(u64 len, u8 *ptr) : len{len}, ptr{ptr} {}
    bool null_terminated();
    const char *c_str();
    u8 &operator[](u64 n);
    string(const char *c_str);
    string &operator=(const char *c_str);
    string substr(u64 index, usize length);
    NG_INLINE auto begin() { return ptr; }
    NG_INLINE auto end() { return ptr + len; }

    NG_INLINE string(print_buffer b) : len{ng_min(b.len, b.cap)}, ptr{b.str} {}
};
void print_item(print_buffer *buf, string s);
string copy_string(string s, allocator *a = default_allocator);
void free_string(string *s, allocator *a = default_allocator);
NG_INLINE constexpr static const string operator""_s(const char *str,
                                                     usize length) {
    return string{(u64)length, (u8 *)str};
}
struct c_str {
    allocator *alloc;
    char *mem;
    u64 length;
    c_str(string source, allocator *allocator = default_allocator);
    c_str(const c_str &) = delete;
    ~c_str();
    NG_INLINE operator const char *() { return mem; }
};
} // namespace ng
bool operator==(ng::string lhs, ng::string rhs);
bool operator!=(ng::string lhs, ng::string rhs);
bool operator==(ng::string lhs, const char *rhs);
bool operator!=(ng::string lhs, const char *rhs);
bool operator==(const char *lhs, ng::string rhs);
bool operator!=(const char *lhs, ng::string rhs);
namespace ng {
template <class... Strings>
auto concatenate(allocator *alloc, Strings &&... args) {
    constexpr auto num_args = sizeof...(Strings);
    string result = {};
    string strings[num_args] = {args...};
    u64 total_length = 0;
    for (int i = 0; i < num_args; i += 1) {
        total_length += strings[i].len;
    }
    result.len = total_length;
    result.ptr = (u8 *)alloc->allocate(result.len);
    if (result.ptr) {
        auto head = result.ptr;
        for (int i = 0; i < num_args; i += 1) {
            memcpy(head, strings[i].ptr, strings[i].len);
            head += strings[i].len;
        }
    } else {
        result = {};
    }
    return result;
}
} // namespace ng
#ifdef NG_DEFINE
namespace ng {
string print_buffer_to_string(print_buffer buf) {
    return string{buf.len, buf.str};
}
// string class
void print_item(print_buffer *buf, string s) {
    if (s.ptr != nullptr) {
        for (int i = 0, end = (int)s.len; i < end; i += 1) {
            buf->putchar(s.ptr[i]);
        }
    }
}
bool string::null_terminated() {
    if (len > 0 && ptr != nullptr) {
        for (u64 i = len; i > 0; i -= 1) {
            if (ptr[i - 1] == '\0') {
                return true;
            }
        }
    }
    return false;
}
static const char empty_c_str[] = "";
const char *string::c_str() {
    if (null_terminated()) {
        return (const char *)ptr;
    } else {
        return empty_c_str;
    }
}
u8 &string::operator[](u64 n) {
    ng_assert(n >= 0 && n < len);
    return ptr[n];
}
string::string(const char *c_str) {
    ptr = (u8 *)c_str;
    while (c_str && *c_str++)
        ;
    len = c_str - (char *)ptr - 1;
}
string &string::operator=(const char *c_str) { return *this = string(c_str); }
string string::substr(u64 index, u64 length) {
    auto real_index = (min)(index, len);
    auto real_length = length;
    if (real_index + real_length > len) {
        real_length = len - real_index;
    }
    return string{real_length, ptr + real_index};
}
string copy_string(string s, allocator *a) {
    string result = {};
    result.ptr = (u8 *)a->allocate(s.len);
    result.len = s.len;
    memcpy(result.ptr, s.ptr, s.len);
    return result;
}
void free_string(string *s, allocator *a) {
    if (s->ptr) {
        a->free(s->ptr, s->len);
    }
    s->ptr = nullptr;
    s->len = 0;
}
c_str::c_str(string source, allocator *alloc) : alloc{alloc} {
    length = source.len + 1;
    mem = (char *)alloc->allocate(length);
    ng_assert(mem != nullptr);
    memcpy(mem, source.ptr, source.len);
    mem[length - 1] = '\0';
}
c_str::~c_str() { alloc->free(mem, length); }
} // namespace ng
bool operator==(ng::string lhs, ng::string rhs) {
    if (lhs.len != rhs.len) {
        return false;
    }
    if (lhs.ptr == rhs.ptr) {
        return true;
    }
    if (lhs.ptr == nullptr || rhs.ptr == nullptr) {
        return false;
    }
    return ng::memcmp(lhs.ptr, rhs.ptr, (ng::usize)lhs.len) == 0;
}
bool operator!=(ng::string lhs, ng::string rhs) { return !(lhs == rhs); }
bool operator==(ng::string lhs, const char *rhs) {
    if (rhs == nullptr) {
        return lhs.ptr == nullptr;
    }
    return lhs == ng::string{ng::strlen(rhs), (ng::u8 *)rhs};
}
bool operator!=(ng::string lhs, const char *rhs) { return !(lhs == rhs); }
bool operator==(const char *lhs, ng::string rhs) { return rhs == lhs; }
bool operator!=(const char *lhs, ng::string rhs) { return rhs != lhs; }
#endif // NG_DEFINE
namespace ng {
int utf8strlen(const char *str);
#ifdef NG_DEFINE
int utf8strlen(const char *str) {
    int result = 0;
    while (*str) {
        if (*str & 0x80) {        // high bit set
            if (!(*str & 0x40)) { // continuation bit not set
                result += 1;      // it's not a continuation byte
            }
        } else { // it's ascii
            result += 1;
        }
        str += 1;
    }
    return result;
}
#endif // NG_DEFINE
// containers
template <typename T> struct array {
    allocator *alloc = default_allocator;
    s64 count = 0;
    u64 capacity = 0;
    T *data = nullptr;

    // ~array() { ng_assert(data == nullptr, "Free me."); }
    inline T &operator[](int n) {
        ng_assert(n < count);
        return data[n];
    }
    array copy(allocator *a = nullptr) const {
        array result = {};
        if (a) {
            result.alloc = a;
        } else {
            result.alloc = alloc;
        }
        result.count = count;
        if (count > 0) {
            result.capacity = count;
            result.data = (T *)result.alloc->allocate(this->count * sizeof(T));
            memcpy(result.data, data, result.capacity * sizeof(T));
        }
        return result;
    }
    void amortize(s64 new_count) {
        auto new_capacity = capacity;
        while ((u64)new_count > new_capacity) {
            new_capacity = new_capacity * 3 / 2;
            new_capacity += 8;
        }
        data = (T *)alloc->resize(data, new_capacity * sizeof(T),
                                  capacity * sizeof(T));
        capacity = new_capacity;
    }
    void reserve(u64 new_capacity) {
        if (new_capacity > capacity) {
            data = (T *)alloc->resize(data, new_capacity * sizeof(T),
                                      capacity * sizeof(T));
            capacity = new_capacity;
        }
    }
    void resize(s64 new_len, const T &value = {}) {
        reserve(new_len);
        for (s64 i = count; i < new_len; i += 1) {
            data[i] = value;
        }
        count = new_len;
    }
    void release() {
        if (data) {
            alloc->free(data, capacity * sizeof(T));
            data = nullptr;
        }
        capacity = 0;
        count = 0;
    }
    T *push(const T &value = {}) {
        if ((u64)count >= capacity) {
            amortize(count + 1);
        }
        count += 1;
        data[count - 1] = value;
        return &data[count - 1];
    }
    T pop() {
        T result = {};
        ng_assert(count > 0, "Popped an empty ng::array");
        if (count > 0) {
            result = data[count - 1];
            count -= 1;
        }
        return result;
    }
    T *insert(s64 index, const T &value = {}) {
        if (index >= count) {
            return push(value);
        } else {
            if ((u64)count >= capacity) {
                amortize(count + 1);
            }
            memmove(data + index + 1, data + index,
                    (count - index - 1) * sizeof(T));
            count += 1;
            data[index] = value;
            return &data[index];
        }
    }
    T remove(s64 index) {
        ng_assert(index < count, "index = %0, count = %1", index, count);
        swap(data[index], data[count - 1]);
        return pop();
    }
    // T remove_ordered(s64 index) {
    //     ng_assert(index < count);
    //     T result = data[index];
    //     memmove(data + index, data + index + 1,
    //             (count - index - 1) * sizeof(T));
    //     count -= 1;
    //     return result;
    // }
    NG_INLINE auto begin() const { return data; }
    NG_INLINE auto end() const { return data + count; }
};
template <class T> void print_item(print_buffer *buf, const array<T> &a) {
    sprint(buf, "array(count = %0, capacity = %1) {", a.count, a.capacity);
    for (auto it = a.begin(), end = a.end(); it != end; ++it) {
        print_item(buf, *it);
        if (it < end - 1) {
            print_item(buf, ", ");
        }
    }
    buf->putchar('}');
}
template <class T> int print_get_item_size(const array<T> &a) {
    int total = sizeof("array(count = , capacity = ) {");
    total += print_get_item_size(a.count);
    total += print_get_item_size(a.capacity);
    for (auto it = a.begin(), end = a.end(); it != end; ++it) {
        total += print_get_item_size(*it); // @Todo @Bug: might return -1!!
        // @Todo: write a wrapper that gets the size if it can, otherwise it
        // calculates the size by sprint()ing it.
        if (it < end - 1) {
            total += sizeof(", ");
        }
    }
    total += sizeof('}');
    return total;
}
template <class T> struct auto_array : array<T> {
    using base = array<T>;
    auto_array copy(allocator *a = nullptr) const {
        auto_array result = {};
        if (a) {
            result.alloc = a;
        } else {
            result.alloc = alloc;
        }
        result.count = count;
        if (count > 0) {
            result.capacity = count;
            result.data = (T *)result.alloc->allocate(this->count * sizeof(T));
            for (s64 i = 0; i < result.count; i += 1) {
                new (result.data + i) T{data[i]};
            }
        }
        return result;
    }
    auto_array() : base{} {}
    auto_array(auto_array &&rhs) : base{static_cast<base &&>(rhs)} { rhs = {}; }
    auto_array(const auto_array &rhs) : auto_array{rhs.copy()} {}
    auto_array &operator=(auto_array &&rhs) {
        base::operator=(static_cast<base &&>(rhs));
        rhs = {};
    }
    auto_array &operator=(const auto_array &rhs) {
        base::operator=(rhs.copy());

    }
    ~auto_array() { base::release(); }
};
template <typename Key, typename Value> struct map {
    struct slot {
        bool occupied = false;
        u64 hash = 0;
        Key key = {};
        Value value = {};
    };
    allocator *alloc = default_allocator;
    slot *slots = nullptr;
    u64 capacity = 0;
    int count = 0;
    map(allocator *a = default_allocator) : alloc{a} {}
    map copy(allocator *a = default_allocator) {
        map result = {};
        if (a) {
            result.alloc = a;
        } else {
            result.alloc = this->alloc;
        }
        if (this->capacity > 0) {
            result.allocate(this->capacity);
            memcpy(result.slots, this->slots, this->capacity * sizeof(slot));
        }
        result.count = this->count;
        return result;
    }
    void allocate(u64 new_capacity) {
        capacity = new_capacity;
        slots = alloc->make_new_array<slot>(new_capacity);
    }
    void expand() {
        auto old_slots = slots;
        auto old_cap = capacity;

        auto new_cap = capacity * 3 / 2;
        if (new_cap < 8) {
            new_cap = 8;
        }
        allocate(new_cap);
        for (u64 i = 0; i < old_cap; i += 1) {
            auto slot = old_slots[i];
            if (slot.occupied) {
                insert(slot.key, slot.value);
            }
        }
        if (old_slots) {
            alloc->free(old_slots, old_cap);
        }
    }
    void release() {
        if (slots) {
            alloc->make_delete_array(slots, capacity);
        }
    }
    Value *insert(Key key, Value value) {
        auto hash = get_hash(key);
        auto index = find_index(hash, key);
        if (index == -1) {
            if ((u64)count >= capacity) {
                expand();
            }
            index = hash % capacity;
            while (slots[index].occupied) { // linear probe
                index += 1;
                if ((u64)index >= capacity) {
                    index = 0;
                }
            }
            count += 1;
        }
        auto slot = &slots[index];
        slot->occupied = true;
        slot->hash = hash;
        slot->key = key;
        slot->value = value;
        return &slot->value;
    }
    void remove(Key key) {
        auto hash = get_hash(key);
        auto index = find_index(hash, key);
        if (index != -1) {
            slots[index].occupied = false;
        }
    }
    int find_index(u64 hash, Key key) {
        if (count <= 0) {
            return -1;
        }
        auto slot = hash % capacity;
        auto index = slot;
        while (slots[index].occupied) {
            if (slots[index].hash == hash) {
                if (slots[index].key == key) {
                    return index;
                }
            }
            // linear probe
            index += 1;
            if (index >= capacity) {
                index = 0;
            }
            if (index == slot) { // Looped; all slots are full.
                return -1;
            }
        }
        return -1;
    }
    struct find_result {
        bool found = false;
        Value *value = null;
    };
    find_result operator[](Key key) {
        find_result result = {};
        auto hash = get_hash(key);
        auto index = find_index(hash, key);
        if (index != -1) {
            result.value = &slots[index].value;
            result.found = true;
        }
        return result;
    }
    struct slot_iterator {
        slot *ptr;
        slot *end;
        void operator++() {
            do {
                ptr += 1;
            } while (ptr < end && !ptr->occupied);
        }
        slot_iterator(slot *ptr, slot *end) : ptr{ptr}, end{end} {
            operator++();
        }
        slot_iterator(slot *end) : ptr{end}, end{end} {}
        bool operator!=(const dummy &) { return ptr < end; }
        operator slot *() { return ptr; }
    };
    slot_iterator begin() { return {slots, slots + capacity}; }
    dummy end() { return {}; }
};
u64 get_hash(string key);
u64 get_hash(u64 key);
u64 get_hash(void *ptr);
#ifdef NG_DEFINE
u64 get_hash(string key) { // djb2
    u64 result = 53817321;
    ng_for(key) { result += (result << 5) + it; }
    return result;
}
u64 get_hash(u64 x) {
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    x = x ^ (x >> 31);
    return x;
}
u64 get_hash(void *ptr) { return get_hash((u64)(uptr)ptr); }
#endif // NG_DEFINE
} // namespace ng
