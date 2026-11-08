#pragma once

#include <realm/base/common/defines.hpp>
#include <realm/base/common/typedefs.hpp>

#include <array>
#include <atomic>
#include <bit>
#include <concepts>
#include <type_traits>

namespace realm::base::metrics::traits
{

// clang-format off
template<typename T>
concept MetricNumber = std::is_arithmetic_v<T> && !std::same_as<T, bool> && std::same_as<T, std::remove_cv_t<T>>;

template<typename Fn, typename T>
concept MetricValueTransformFn = MetricNumber<T> && requires(Fn fn, T value)
{
    { fn(value) } noexcept -> std::same_as<T>;
};

template<typename S>
concept MetricValueStorage = MetricNumber<typename S::value_type> && requires(S& s, S const& cs, typename S::value_type value)
{
    { cs.load()                                                        } noexcept -> std::same_as<typename S::value_type>;
    { s.fetchAdd(value)                                                } noexcept -> std::same_as<typename S::value_type>;
    { s.fetchSub(value)                                                } noexcept -> std::same_as<typename S::value_type>;
    { s.exchange(value)                                                } noexcept -> std::same_as<typename S::value_type>;
    { s.store(value)                                                   } noexcept -> std::same_as<void>;
    { s.add(value)                                                     } noexcept -> std::same_as<typename S::value_type>;
    { s.sub(value)                                                     } noexcept -> std::same_as<typename S::value_type>;
    { s.transform([](typename S::value_type v) noexcept { return v; }) } noexcept -> std::same_as<typename S::value_type>;
};
// clang-format on

}  // namespace realm::base::metrics::traits

namespace realm::base::metrics::detail
{

// Match atomic integer arithmetic: wrap modulo 2^N, including signed integers.
template<traits::MetricNumber T>
RE_FORCE_INLINE constexpr T arithmeticAdd(T lhs, T rhs) noexcept
{
    if constexpr (std::integral<T>)
    {
        return static_cast<T>(static_cast<std::make_unsigned_t<T>>(lhs) + static_cast<std::make_unsigned_t<T>>(rhs));
    }
    else
    {
        return lhs + rhs;
    }
}

template<traits::MetricNumber T>
RE_FORCE_INLINE constexpr T arithmeticSubtract(T lhs, T rhs) noexcept
{
    if constexpr (std::integral<T>)
    {
        return static_cast<T>(static_cast<std::make_unsigned_t<T>>(lhs) - static_cast<std::make_unsigned_t<T>>(rhs));
    }
    else
    {
        return lhs - rhs;
    }
}

template<traits::MetricNumber T>
struct MetricValueStoragePlain final
{
public:
    using value_type = T;

public:
    constexpr MetricValueStoragePlain() noexcept = default;
    constexpr explicit MetricValueStoragePlain(value_type initial) noexcept
        : _value{ initial }
    {
    }

    MetricValueStoragePlain(MetricValueStoragePlain const&)            = delete;
    MetricValueStoragePlain& operator=(MetricValueStoragePlain const&) = delete;
    MetricValueStoragePlain(MetricValueStoragePlain&&)                 = delete;
    MetricValueStoragePlain& operator=(MetricValueStoragePlain&&)      = delete;

    constexpr ~MetricValueStoragePlain() = default;

public:
    RE_NODISCARD RE_FORCE_INLINE constexpr value_type load() const noexcept { return _value; }

    RE_FORCE_INLINE constexpr void store(value_type value) noexcept { _value = value; }

    RE_FORCE_INLINE constexpr value_type fetchAdd(value_type value) noexcept
    {
        auto const previous = _value;
        _value              = arithmeticAdd(previous, value);
        return previous;
    }

    RE_FORCE_INLINE constexpr value_type fetchSub(value_type value) noexcept
    {
        auto const previous = _value;
        _value              = arithmeticSubtract(previous, value);
        return previous;
    }

    RE_FORCE_INLINE constexpr value_type exchange(value_type value) noexcept
    {
        auto const previous = _value;
        _value              = value;
        return previous;
    }

    RE_FORCE_INLINE constexpr value_type add(value_type value) noexcept
    {
        fetchAdd(value);
        return _value;
    }

    RE_FORCE_INLINE constexpr value_type sub(value_type value) noexcept
    {
        fetchSub(value);
        return _value;
    }

    template<typename Fn>
        requires traits::MetricValueTransformFn<Fn, value_type>
    RE_FORCE_INLINE constexpr value_type transform(Fn fn) noexcept
    {
        _value = fn(_value);
        return _value;
    }

private:
    value_type _value{};
};

template<traits::MetricNumber T>
    requires std::atomic<T>::is_always_lock_free
struct MetricValueStorageAtomic final
{
public:
    using value_type = T;

public:
    constexpr MetricValueStorageAtomic() noexcept = default;
    constexpr explicit MetricValueStorageAtomic(value_type initial) noexcept
        : _value{ initial }
    {
    }

    MetricValueStorageAtomic(MetricValueStorageAtomic const&)            = delete;
    MetricValueStorageAtomic& operator=(MetricValueStorageAtomic const&) = delete;
    MetricValueStorageAtomic(MetricValueStorageAtomic&&)                 = delete;
    MetricValueStorageAtomic& operator=(MetricValueStorageAtomic&&)      = delete;

    constexpr ~MetricValueStorageAtomic() = default;

public:
    RE_NODISCARD RE_FORCE_INLINE constexpr value_type load() const noexcept
    {
        return _value.load(std::memory_order_relaxed);
    }

    RE_FORCE_INLINE constexpr void store(value_type value) noexcept { _value.store(value, std::memory_order_relaxed); }

    RE_FORCE_INLINE value_type fetchAdd(value_type value) noexcept
    {
        return _value.fetch_add(value, std::memory_order_relaxed);
    }

    RE_FORCE_INLINE value_type fetchSub(value_type value) noexcept
    {
        return _value.fetch_sub(value, std::memory_order_relaxed);
    }

    RE_FORCE_INLINE value_type exchange(value_type value) noexcept
    {
        return _value.exchange(value, std::memory_order_relaxed);
    }

    RE_FORCE_INLINE value_type add(value_type value) noexcept { return arithmeticAdd(fetchAdd(value), value); }

    RE_FORCE_INLINE value_type sub(value_type value) noexcept { return arithmeticSubtract(fetchSub(value), value); }

    // The transform can run repeatedly under contention; it must have no side effects.
    template<typename Fn>
        requires traits::MetricValueTransformFn<Fn, value_type>
    RE_FORCE_INLINE constexpr value_type transform(Fn fn) noexcept
    {
        auto current = _value.load(std::memory_order_relaxed);
        auto next    = fn(current);

        // Compare representations so transforms preserve signed zero and NaN payloads.
        auto unchanged = [](value_type lhs, value_type rhs) noexcept
        {
            if constexpr (std::integral<value_type>)
            {
                return lhs == rhs;
            }
            else if constexpr (sizeof(value_type) == sizeof(u32))
            {
                return std::bit_cast<u32>(lhs) == std::bit_cast<u32>(rhs);
            }
            else if constexpr (sizeof(value_type) == sizeof(u64))
            {
                return std::bit_cast<u64>(lhs) == std::bit_cast<u64>(rhs);
            }
            else
            {
                using bits = std::array<std::byte, sizeof(value_type)>;
                return std::bit_cast<bits>(lhs) == std::bit_cast<bits>(rhs);
            }
        };
        while (!unchanged(next, current) &&
               !_value.compare_exchange_weak(current, next, std::memory_order_relaxed, std::memory_order_relaxed))
        {
            next = fn(current);
        }

        return next;
    }

private:
    std::atomic<value_type> _value{};
};

static_assert(traits::MetricValueStorage<MetricValueStoragePlain<int>>);
static_assert(traits::MetricValueStorage<MetricValueStorageAtomic<int>>);

}  // namespace realm::base::metrics::detail
