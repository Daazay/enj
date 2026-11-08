#pragma once

#include <realm/base/metrics/metric_value_storage.hpp>


namespace realm::base::metrics
{

template<traits::MetricNumber T = f64, traits::MetricValueStorage ValueStorage = detail::MetricValueStoragePlain<T>,
         traits::MetricValueStorage CountStorage = detail::MetricValueStoragePlain<u64>>
    requires std::same_as<T, typename ValueStorage::value_type> && std::same_as<u64, typename CountStorage::value_type>
class MetricMean
{
public:
    using value_type = T;
    using count_type = u64;

    using value_storage_type = ValueStorage;
    using count_storage_type = CountStorage;

public:
    struct Snapshot
    {
    public:
        RE_NODISCARD RE_FORCE_INLINE constexpr f64 mean() const noexcept
        {
            return count == 0 ? 0.0 : static_cast<f64>(sum) / static_cast<f64>(count);
        }

    public:
        value_type sum;
        count_type count;
    };

public:
    constexpr MetricMean() noexcept = default;

    MetricMean(MetricMean const&)            = delete;
    MetricMean& operator=(MetricMean const&) = delete;
    MetricMean(MetricMean&&)                 = delete;
    MetricMean& operator=(MetricMean&&)      = delete;

    constexpr ~MetricMean() noexcept = default;

public:
    RE_FORCE_INLINE constexpr void observe(value_type value) noexcept
    {
        _sum.add(value);
        _count.add(1);
    }

    RE_NODISCARD RE_FORCE_INLINE constexpr Snapshot snapshot() const noexcept { return { _sum.load(), _count.load() }; }

    RE_NODISCARD RE_FORCE_INLINE constexpr f64 value() const noexcept { return snapshot().mean(); }

    RE_FORCE_INLINE constexpr void reset() noexcept
    {
        _sum.store(0);
        _count.store(0);
    }

private:
    ValueStorage _sum{};
    CountStorage _count{};
};

template<traits::MetricNumber T = f64>
using AtomicMetricMean = MetricMean<T, detail::MetricValueStorageAtomic<T>, detail::MetricValueStorageAtomic<u64>>;

}  // namespace realm::base::metrics
