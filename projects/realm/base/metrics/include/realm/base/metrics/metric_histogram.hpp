#pragma once

#include <realm/base/debug/assert.hpp>
#include <realm/base/metrics/metric_value_storage.hpp>

#include <array>

namespace realm::base::metrics
{

// Each bound is inclusive. The last bucket counts values above the last bound.
template<traits::MetricNumber T = f32, usize BucketCount = 10,
         traits::MetricValueStorage CountStorage = detail::MetricValueStoragePlain<u64>>
    requires(BucketCount >= 2) && std::same_as<u64, typename CountStorage::value_type>
class MetricHistogram
{
public:
    using value_type         = T;
    using count_type         = u64;
    using count_storage_type = CountStorage;
    using bounds_type        = std::array<value_type, BucketCount - 1>;

    static constexpr usize k_bucket_count = BucketCount;

    using Snapshot = std::array<count_type, k_bucket_count>;

public:
    constexpr explicit MetricHistogram(bounds_type bounds) noexcept
        : _bounds{ bounds }
    {
        validateBounds(_bounds);
    }

    MetricHistogram(MetricHistogram const&)            = delete;
    MetricHistogram& operator=(MetricHistogram const&) = delete;
    MetricHistogram(MetricHistogram&&)                 = delete;
    MetricHistogram& operator=(MetricHistogram&&)      = delete;

    constexpr ~MetricHistogram() noexcept = default;

public:
    RE_NODISCARD constexpr value_type bound(usize index) const noexcept
    {
        RE_ASSERT_MSG(index < _bounds.size(), "Histogram bound index {} is out of range [0, {})", index,
                      _bounds.size());
        return _bounds[index];
    }

    RE_NODISCARD constexpr count_type count(usize index) const noexcept
    {
        RE_ASSERT_MSG(index < k_bucket_count, "Histogram bucket index {} is out of range [0, {})", index,
                      k_bucket_count);
        return _counts[index].load();
    }

    RE_NODISCARD constexpr count_type total() const noexcept
    {
        count_type result{};
        for (auto const& bucket : _counts)
        {
            result += bucket.load();
        }
        return result;
    }

    RE_FORCE_INLINE constexpr void observe(value_type sample) noexcept
    {
        if constexpr (std::floating_point<value_type>)
        {
            // Check for NaN
            // NOLINTNEXTLINE(misc-redundant-expression)
            if (sample != sample)
            {
                return;
            }
        }
        for (usize i = 0; i < _bounds.size(); ++i)
        {
            if (sample <= _bounds[i])
            {
                _counts[i].add(1);
                return;
            }
        }
        _counts[BucketCount - 1].add(1);
    }

    RE_NODISCARD constexpr Snapshot snapshot() const noexcept
    {
        Snapshot result{};
        for (usize i = 0; i < k_bucket_count; ++i)
        {
            result[i] = _counts[i].load();
        }
        return result;
    }

    constexpr void reset() noexcept
    {
        for (auto& bucket : _counts)
        {
            bucket.store(0);
        }
    }

private:
    static constexpr void validateBounds(bounds_type const& bounds) noexcept
    {
        for (usize i = 0; i < bounds.size(); ++i)
        {
            if constexpr (std::floating_point<value_type>)
            {
                // NOLINTNEXTLINE(misc-redundant-expression)
                RE_VERIFY_MSG(bounds[i] == bounds[i], "Histogram bound {} must not be NaN", i);
            }
            if (i > 0)
            {
                RE_VERIFY_MSG(bounds[i - 1] < bounds[i],
                              "Histogram bounds must be strictly increasing: bound[{}] must be less than bound[{}]",
                              i - 1, i);
            }
        }
    }

private:
    bounds_type const                        _bounds;
    std::array<CountStorage, k_bucket_count> _counts{};
};

template<traits::MetricNumber T = f32, usize BucketCount = 10>
using AtomicMetricHistogram = MetricHistogram<T, BucketCount, detail::MetricValueStorageAtomic<u64>>;

}  // namespace realm::base::metrics
