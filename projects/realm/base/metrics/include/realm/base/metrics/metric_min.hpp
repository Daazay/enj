#pragma once

#include <realm/base/metrics/metric_value_storage.hpp>

#include <algorithm>
#include <limits>

namespace realm::base::metrics
{

template<traits::MetricNumber T, traits::MetricValueStorage ValueStorage = detail::MetricValueStoragePlain<T>>
    requires std::same_as<T, typename ValueStorage::value_type>
class MetricMin
{
public:
    using value_type         = T;
    using value_storage_type = ValueStorage;

private:
    static constexpr value_type k_identity = std::numeric_limits<value_type>::has_infinity
                                                 ? std::numeric_limits<value_type>::infinity()
                                                 : std::numeric_limits<value_type>::max();

public:
    constexpr MetricMin() noexcept = default;

    constexpr explicit MetricMin(value_type initial) noexcept
        : _storage{ initial }
    {
    }

    MetricMin(MetricMin const&)            = delete;
    MetricMin& operator=(MetricMin const&) = delete;
    MetricMin(MetricMin&&)                 = delete;
    MetricMin& operator=(MetricMin&&)      = delete;

    constexpr ~MetricMin() noexcept = default;

public:
    RE_NODISCARD RE_FORCE_INLINE constexpr value_type value() const noexcept { return _storage.load(); }

    RE_FORCE_INLINE constexpr value_type observe(value_type sample) noexcept
    {
        return _storage.transform([sample](value_type current) noexcept -> value_type
                                  { return std::min(sample, current); });
    }

    RE_FORCE_INLINE constexpr void reset() noexcept { return _storage.store(k_identity); }

private:
    value_storage_type _storage{ k_identity };
};

template<traits::MetricNumber T>
using AtomicMetricMin = MetricMin<T, detail::MetricValueStorageAtomic<T>>;

}  // namespace realm::base::metrics
