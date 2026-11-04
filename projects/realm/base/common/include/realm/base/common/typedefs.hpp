#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace realm
{

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using usize = std::size_t;
using isize = std::ptrdiff_t;

using byte = std::byte;

using f32 = float;
using f64 = double;

static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);

static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);

static_assert(sizeof(isize) == sizeof(void*));
static_assert(sizeof(usize) == sizeof(void*));

static_assert(sizeof(byte) == 1);

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);


// Numeric limits

template<typename T>
inline constexpr T max_value = std::numeric_limits<T>::max();

template<typename T>
inline constexpr T min_value = std::numeric_limits<T>::lowest();

//

inline constexpr u8 u8_max = max_value<u8>;
inline constexpr u8 u8_min = min_value<u8>;

inline constexpr u16 u16_max = max_value<u16>;
inline constexpr u16 u16_min = min_value<u16>;

inline constexpr u32 u32_max = max_value<u32>;
inline constexpr u32 u32_min = min_value<u32>;

inline constexpr u64 u64_max = max_value<u64>;
inline constexpr u64 u64_min = min_value<u64>;

inline constexpr i8 i8_max = max_value<i8>;
inline constexpr i8 i8_min = min_value<i8>;

inline constexpr i16 i16_max = max_value<i16>;
inline constexpr i16 i16_min = min_value<i16>;

inline constexpr i32 i32_max = max_value<i32>;
inline constexpr i32 i32_min = min_value<i32>;

inline constexpr i64 i64_max = max_value<i64>;
inline constexpr i64 i64_min = min_value<i64>;

inline constexpr f32 f32_max = max_value<f32>;
inline constexpr f32 f32_min = min_value<f32>;
inline constexpr f32 f32_eps = std::numeric_limits<f32>::epsilon();

inline constexpr f64 f64_max = max_value<f64>;
inline constexpr f64 f64_min = min_value<f64>;
inline constexpr f64 f64_eps = std::numeric_limits<f64>::epsilon();

//

inline constexpr usize KB = 1024;
inline constexpr usize MB = 1024 * KB;
inline constexpr usize GB = 1024 * MB;
inline constexpr usize TB = 1024 * GB;

constexpr usize operator""_KB(unsigned long long v) noexcept
{
    return static_cast<usize>(v) * KB;
}
constexpr usize operator""_MB(unsigned long long v) noexcept
{
    return static_cast<usize>(v) * MB;
}
constexpr usize operator""_GB(unsigned long long v) noexcept
{
    return static_cast<usize>(v) * GB;
}
constexpr usize operator""_TB(unsigned long long v) noexcept
{
    return static_cast<usize>(v) * TB;
}

//

inline constexpr usize Kb = 1000;
inline constexpr usize Mb = 1000 * Kb;
inline constexpr usize Gb = 1000 * Mb;
inline constexpr usize Tb = 1000 * Gb;

constexpr usize operator""_Kb(unsigned long long v) noexcept
{
    return static_cast<usize>(v) * Kb;
}
constexpr usize operator""_Mb(unsigned long long v) noexcept
{
    return static_cast<usize>(v) * Mb;
}
constexpr usize operator""_Gb(unsigned long long v) noexcept
{
    return static_cast<usize>(v) * Gb;
}
constexpr usize operator""_Tb(unsigned long long v) noexcept
{
    return static_cast<usize>(v) * Tb;
}

#if !defined(REALM_CACHE_LINE_SIZE)
    #define REALM_CACHE_LINE_SIZE 64
#endif
inline constexpr usize cache_line_size = REALM_CACHE_LINE_SIZE;

static_assert(cache_line_size >= 16, "cache_line_size must be a reasonable power-of-two constant");
static_assert((cache_line_size & (cache_line_size - 1)) == 0, "cache_line_size must be a power of two");

}  // namespace realm
