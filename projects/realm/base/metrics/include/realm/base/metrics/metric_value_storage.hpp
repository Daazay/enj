#pragma once

#include <realm/base/common/defines.hpp>
#include <realm/base/common/typedefs.hpp>

#include <algorithm>
#include <atomic>
#include <concepts>
#include <type_traits>

namespace realm::base::metrics::traits
{

template<typename T>
concept MetricNumber = std::is_arithmetic_v<T> && !std::same_as<T, bool> && std::same_as<T, std::remove_cv_t<T>>;

}  // namespace realm::base::metrics::traits

namespace realm::base::metrics
{

struct StoragePolicy
{
public:
    // Callers guarantee single-threaded access or external synchronization.
    struct Unsafe final
    {
    };

    // Individual values use relaxed, lock-free atomics; compound reads are not snapshots.
    // This does not synchronize unrelated data. Coherent collection/reset requires external coordination.
    struct Atomic final
    {
    };
};

// Integer arithmetic results must remain representable. No overflow handling is provided.
template<typename Policy, traits::MetricNumber T, T InitialValue = T{}>
struct MetricValueStorage;

template<traits::MetricNumber T, T InitialValue>
struct MetricValueStorage<StoragePolicy::Unsafe, T, InitialValue>
{
public:
    using value_type = T;
    using policy     = StoragePolicy::Unsafe;

    static constexpr value_type initial_value = InitialValue;

public:
    constexpr MetricValueStorage() noexcept = default;
    constexpr explicit MetricValueStorage(value_type v) noexcept
        : _value(v)
    {
    }

    // A metric owns its accumulated state. Copying it would silently detach updates from its store.
    MetricValueStorage(MetricValueStorage const&)            = delete;
    MetricValueStorage& operator=(MetricValueStorage const&) = delete;
    MetricValueStorage(MetricValueStorage&&)                 = delete;
    MetricValueStorage& operator=(MetricValueStorage&&)      = delete;

public:
    RE_NODISCARD RE_FORCE_INLINE constexpr value_type load() const noexcept { return _value; }

public:
    RE_FORCE_INLINE constexpr void store(value_type v) noexcept { _value = v; }
    RE_FORCE_INLINE constexpr void add(value_type v) noexcept { _value += v; }

    RE_FORCE_INLINE constexpr void maxWith(value_type v) noexcept { _value = std::max(_value, v); }
    RE_FORCE_INLINE constexpr void minWith(value_type v) noexcept { _value = std::min(_value, v); }

public:
    RE_FORCE_INLINE constexpr void reset() noexcept { _value = initial_value; }

private:
    value_type _value{ initial_value };
};

template<traits::MetricNumber T, T InitialValue>
struct MetricValueStorage<StoragePolicy::Atomic, T, InitialValue>
{
public:
    static_assert(std::atomic<T>::is_always_lock_free);

public:
    using value_type = T;
    using policy     = StoragePolicy::Atomic;

    static constexpr value_type initial_value = InitialValue;

public:
    constexpr MetricValueStorage() noexcept = default;
    constexpr explicit MetricValueStorage(value_type v) noexcept
        : _value(v)
    {
    }

public:
    RE_NODISCARD RE_FORCE_INLINE constexpr value_type load() const noexcept
    {
        return _value.load(std::memory_order_relaxed);
    }

public:
    RE_FORCE_INLINE constexpr void store(value_type v) noexcept { _value.store(v, std::memory_order_relaxed); }
    RE_FORCE_INLINE constexpr void add(value_type v) noexcept { _value.fetch_add(v, std::memory_order_relaxed); }

    RE_FORCE_INLINE constexpr void maxWith(value_type v) noexcept
    {
        auto current = _value.load(std::memory_order_relaxed);
        while (current < v && !_value.compare_exchange_weak(current, v, std::memory_order_relaxed))
        {
        }
    }

    RE_FORCE_INLINE constexpr void minWith(value_type v) noexcept
    {
        auto current = _value.load(std::memory_order_relaxed);
        while (current > v && !_value.compare_exchange_weak(current, v, std::memory_order_relaxed))
        {
        }
    }

public:
    RE_FORCE_INLINE constexpr void reset() noexcept { _value.store(initial_value, std::memory_order_relaxed); }

private:
    std::atomic<value_type> _value{ initial_value };
};

}  // namespace realm::base::metrics
