#pragma once

#include <realm/base/metrics/metric_value_storage.hpp>

namespace realm::base::metrics
{

template<traits::MetricNumber T = i64, T InitialValue = max_value<T>, typename Policy = StoragePolicy::Unsafe>
class MetricMin
{
public:
    using storage_type = MetricValueStorage<Policy, T, InitialValue>;
    using value_type   = T;
    using policy       = Policy;

public:
    constexpr MetricMin() noexcept = default;
    constexpr explicit MetricMin(value_type v) noexcept
        : _storage{ v }
    {
    }

public:
    RE_NODISCARD RE_FORCE_INLINE constexpr value_type get() const noexcept { return _storage.load(); }

public:
    RE_FORCE_INLINE constexpr void observe(value_type v) noexcept { _storage.minWith(v); }
    RE_FORCE_INLINE constexpr void update(value_type v) noexcept { _storage.minWith(v); }

public:
    RE_FORCE_INLINE constexpr void reset() noexcept { _storage.reset(); }

private:
    storage_type _storage;
};

template<traits::MetricNumber T = i64, T InitialValue = max_value<T>>
using AtomicMetricMin = MetricMin<T, InitialValue, StoragePolicy::Atomic>;

}  // namespace realm::base::metrics
