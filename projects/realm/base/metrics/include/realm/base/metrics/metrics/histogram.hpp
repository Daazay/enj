#pragma once

#include <realm/base/metrics/metric_value_storage.hpp>

#include <array>

namespace realm::base::metrics
{

template<traits::MetricNumber T = f32, usize BucketCount = 10, typename Policy = StoragePolicy::Unsafe>
class MetricHistogram
{
public:
    static_assert(BucketCount >= 2, "histogram must have at least 2 buckets");

public:
    using value_type = T;
    using count_type = u64;
    using policy     = Policy;

    using bounds_type  = std::array<T, BucketCount - 1>;
    using storage_type = MetricValueStorage<Policy, count_type, count_type{}>;

    static constexpr usize bucket_count = BucketCount;

public:
    constexpr MetricHistogram() noexcept = default;
    constexpr MetricHistogram(bounds_type bounds) noexcept
        : _bounds{ bounds }
    {
    }

public:
    RE_NODISCARD constexpr T          bound(usize idx) const noexcept { return _bounds[idx]; }
    RE_NODISCARD constexpr count_type count(usize idx) const noexcept { return _counts[idx].load(); }

    RE_NODISCARD constexpr count_type total() const noexcept
    {
        count_type total{};
        for (auto const& bucket : _counts)
        {
            total += bucket.load();
        }
        return total;
    }

public:
    RE_FORCE_INLINE constexpr void observe(value_type v) noexcept
    {
        for (usize i = 0; i < _bounds.size(); ++i)
        {
            if (v <= _bounds[i])
            {
                _counts[i].add(1);
                return;
            }
        }
        _counts[BucketCount - 1].add(1);
    }

public:
    RE_FORCE_INLINE constexpr void reset() noexcept
    {
        for (auto& bucket : _counts)
        {
            bucket.reset();
        }
    }

private:
    bounds_type                           _bounds{};
    std::array<storage_type, BucketCount> _counts{};
};

template<traits::MetricNumber T = f32, usize BucketCount = 10>
using AtomicMetricHistogram = MetricHistogram<T, BucketCount, StoragePolicy::Safe>;

}  // namespace realm::base::metrics
