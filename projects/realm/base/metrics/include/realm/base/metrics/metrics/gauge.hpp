#pragma once

#include <realm/base/metrics/metric_value_storage.hpp>

namespace realm::base::metrics
{

template<traits::MetricNumber T = i64, T InitialValue = T{}, typename Policy = StoragePolicy::Unsafe>
class MetricGauge
{
public:
    using storage_type = MetricValueStorage<Policy, T, InitialValue>;
    using value_type   = T;
    using policy       = Policy;

public:
    constexpr MetricGauge() noexcept = default;
    constexpr explicit MetricGauge(value_type v) noexcept
        : _storage{ v }
    {
    }

public:
    RE_NODISCARD RE_FORCE_INLINE constexpr value_type get() const noexcept { return _storage.load(); }

public:
    RE_FORCE_INLINE constexpr void set(value_type v) noexcept { _storage.store(v); }
    RE_FORCE_INLINE constexpr void add(value_type v) noexcept { _storage.add(v); }

    RE_FORCE_INLINE constexpr void inc() noexcept { _storage.add(1); }
    RE_FORCE_INLINE constexpr void dec() noexcept { _storage.add(-1); }

public:
    RE_FORCE_INLINE constexpr void reset() noexcept { _storage.reset(); }

private:
    storage_type _storage;
};

template<traits::MetricNumber T = i64, T InitialValue = T{}>
using AtomicMetricGauge = MetricGauge<T, InitialValue, StoragePolicy::Atomic>;

}  // namespace realm::base::metrics
