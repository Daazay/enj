#pragma once

#include <realm/base/metrics/metric_value_storage.hpp>

namespace realm::base::metrics
{

template<traits::MetricNumber T = i64, traits::MetricValueStorage ValueStorage = detail::MetricValueStoragePlain<T>>
    requires std::same_as<T, typename ValueStorage::value_type>
class MetricGauge
{
public:
    using value_type         = T;
    using value_storage_type = ValueStorage;

public:
    constexpr MetricGauge() noexcept = default;

    constexpr explicit MetricGauge(value_type initial) noexcept
        : _storage{ initial }
    {
    }

    MetricGauge(MetricGauge const&)            = delete;
    MetricGauge& operator=(MetricGauge const&) = delete;
    MetricGauge(MetricGauge&&)                 = delete;
    MetricGauge& operator=(MetricGauge&&)      = delete;

    constexpr ~MetricGauge() noexcept = default;

public:
    RE_NODISCARD RE_FORCE_INLINE constexpr value_type value() const noexcept { return _storage.load(); }

    RE_FORCE_INLINE constexpr value_type increment(value_type amount = 1) noexcept { return _storage.add(amount); }
    RE_FORCE_INLINE constexpr value_type decrement(value_type amount = 1) noexcept { return _storage.sub(amount); }

    RE_FORCE_INLINE constexpr value_type set(value_type value) noexcept
    {
        _storage.store(value);
        return value;
    }

    RE_FORCE_INLINE constexpr value_type exchange(value_type value) noexcept { return _storage.exchange(value); }

    constexpr void reset() noexcept { _storage.store(value_type{ 0 }); }

public:
    constexpr MetricGauge& operator=(value_type value) noexcept
    {
        set(value);
        return *this;
    }

    constexpr MetricGauge& operator+=(value_type delta) noexcept
    {
        increment(delta);
        return *this;
    }

    constexpr MetricGauge& operator-=(value_type delta) noexcept
    {
        decrement(delta);
        return *this;
    }

    constexpr MetricGauge& operator++() noexcept
    {
        increment();
        return *this;
    }

    constexpr MetricGauge& operator--() noexcept
    {
        decrement();
        return *this;
    }

    RE_FORCE_INLINE constexpr value_type operator++(int) noexcept { return _storage.fetchAdd(1); }

    RE_FORCE_INLINE constexpr value_type operator--(int) noexcept { return _storage.fetchSub(1); }

private:
    value_storage_type _storage{};
};

template<traits::MetricNumber T = i64>
using AtomicMetricGauge = MetricGauge<T, detail::MetricValueStorageAtomic<T>>;

}  // namespace realm::base::metrics
