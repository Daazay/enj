#pragma once

#include <realm/base/metrics/metric_value_storage.hpp>

#include <algorithm>
#include <limits>

namespace realm::base::metrics
{

template<traits::MetricNumber T, traits::MetricValueStorage ValueStorage = detail::MetricValueStoragePlain<T>>
    requires std::same_as<T, typename ValueStorage::value_type>
class MetricMax
{
public:
    using value_type         = T;
    using value_storage_type = ValueStorage;

private:
    static constexpr value_type k_identity = std::numeric_limits<value_type>::has_infinity
                                                 ? -std::numeric_limits<value_type>::infinity()
                                                 : std::numeric_limits<value_type>::lowest();

public:
    constexpr MetricMax() noexcept = default;

    constexpr explicit MetricMax(value_type initial) noexcept
        : _storage{ initial }
    {
    }

    MetricMax(MetricMax const&)            = delete;
    MetricMax& operator=(MetricMax const&) = delete;
    MetricMax(MetricMax&&)                 = delete;
    MetricMax& operator=(MetricMax&&)      = delete;

    constexpr ~MetricMax() noexcept = default;

public:
    RE_NODISCARD RE_FORCE_INLINE constexpr value_type value() const noexcept { return _storage.load(); }

    RE_FORCE_INLINE constexpr value_type observe(value_type sample) noexcept
    {
        return _storage.transform([sample](value_type current) noexcept -> value_type
                                  { return std::max(sample, current); });
    }

    RE_FORCE_INLINE constexpr void reset() noexcept { return _storage.store(k_identity); }

private:
    value_storage_type _storage{ k_identity };
};

template<traits::MetricNumber T>
using AtomicMetricMax = MetricMax<T, detail::MetricValueStorageAtomic<T>>;

}  // namespace realm::base::metrics
