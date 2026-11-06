#pragma once

#include <realm/base/metrics/metric_value_storage.hpp>

namespace realm::base::metrics
{

template<traits::MetricNumber T = f32, typename Policy = StoragePolicy::Unsafe>
class MetricMean
{
public:
    using value_type = T;
    using sum_type   = f64;
    using count_type = u64;
    using policy     = Policy;

    using sum_storage_type   = MetricValueStorage<Policy, sum_type, sum_type{}>;
    using count_storage_type = MetricValueStorage<Policy, count_type, count_type{}>;

public:
    constexpr MetricMean() noexcept = default;

public:
    // With Atomic policy, sum/count are independent loads, not a consistent snapshot.
    RE_NODISCARD RE_FORCE_INLINE constexpr sum_type get() const noexcept
    {
        auto const sample_count = _count.load();
        if (sample_count == 0)
        {
            return sum_type{};
        }
        return _sum.load() / static_cast<sum_type>(sample_count);
    }

    RE_NODISCARD RE_FORCE_INLINE constexpr sum_type   sum() const noexcept { return _sum.load(); }
    RE_NODISCARD RE_FORCE_INLINE constexpr count_type count() const noexcept { return _count.load(); }

public:
    RE_FORCE_INLINE constexpr void add(value_type v) noexcept
    {
        _sum.add(v);
        _count.add(1);
    }

public:
    // Coordinate with readers and writers externally to establish a coherent measurement interval.
    RE_FORCE_INLINE constexpr void reset() noexcept
    {
        _sum.reset();
        _count.reset();
    }

private:
    sum_storage_type   _sum;
    count_storage_type _count;
};

template<traits::MetricNumber T = f32>
using AtomicMetricMean = MetricMean<T, StoragePolicy::Atomic>;

}  // namespace realm::base::metrics
