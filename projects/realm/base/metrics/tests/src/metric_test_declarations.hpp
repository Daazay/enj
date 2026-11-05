#pragma once

#include <realm/base/metrics/metric_storage.hpp>
#include <realm/base/metrics/metrics/counter.hpp>
#include <realm/base/metrics/metrics/gauge.hpp>
#include <realm/base/metrics/metrics/histogram.hpp>

namespace realm::base::metrics::tests
{

enum class MetricId : u8
{
    frames          = 2,
    draw_calls      = 7,
    frame_time      = 11,
    active_entities = 15,
};

}  // namespace realm::base::metrics::tests

namespace realm::base::metrics
{

template<>
struct MetricTraits<tests::MetricId::frames>
{
    using metric_type = AtomicMetricCounter<>;
};

template<>
struct MetricTraits<tests::MetricId::draw_calls>
{
    using metric_type                      = AtomicMetricCounter<>;
    static constexpr std::string_view name = "draw_calls";
};

template<>
struct MetricTraits<tests::MetricId::frame_time>
{
    using metric_type = AtomicMetricHistogram<i64, 3>;

    static consteval metric_type make() noexcept { return metric_type{ { 10, 20 } }; }
};

template<>
struct MetricTraits<tests::MetricId::active_entities>
{
    using metric_type = MetricGauge<i64, 5>;
};

}  // namespace realm::base::metrics

namespace realm::base::metrics::tests
{

using EngineMetrics = MetricStorage<MetricId>;

inline constexpr MetricId unknown_metric_id = static_cast<MetricId>(99);

}  // namespace realm::base::metrics::tests
