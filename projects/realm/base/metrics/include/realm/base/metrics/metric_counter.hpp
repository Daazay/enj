#pragma once

#include <realm/base/metrics/metric_value_storage.hpp>

namespace realm::base::metrics
{

template<traits::MetricNumber T = u64, traits::MetricValueStorage ValueStorage = detail::MetricValueStoragePlain<T>>
    requires std::same_as<T, typename ValueStorage::value_type>
class MetricCounter
{
public:
    using value_type         = T;
    using value_storage_type = ValueStorage;

public:
    constexpr MetricCounter() noexcept = default;

    constexpr explicit MetricCounter(value_type initial) noexcept
        : _storage{ initial }
    {
    }

    MetricCounter(MetricCounter const&)            = delete;
    MetricCounter& operator=(MetricCounter const&) = delete;
    MetricCounter(MetricCounter&&)                 = delete;
    MetricCounter& operator=(MetricCounter&&)      = delete;

    constexpr ~MetricCounter() noexcept = default;

public:
    RE_NODISCARD RE_FORCE_INLINE constexpr value_type value() const noexcept { return _storage.load(); }

    RE_FORCE_INLINE constexpr value_type increment(value_type amount = 1) noexcept { return _storage.add(amount); }

    RE_FORCE_INLINE constexpr void reset() noexcept { return _storage.store(0); }

public:
    RE_FORCE_INLINE constexpr MetricCounter& operator+=(value_type v) noexcept
    {
        increment(v);
        return *this;
    }

    RE_FORCE_INLINE constexpr MetricCounter& operator++() noexcept
    {
        increment();
        return *this;
    }

    RE_FORCE_INLINE constexpr value_type operator++(int) noexcept { return _storage.fetchAdd(1); }

private:
    value_storage_type _storage{};
};

template<traits::MetricNumber T = u64>
using AtomicMetricCounter = MetricCounter<T, detail::MetricValueStorageAtomic<T>>;

}  // namespace realm::base::metrics
