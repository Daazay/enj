#include "realm/base/metrics/metric_descriptor.hpp"
#include "realm/base/metrics/metric_gauge.hpp"
#include "realm/base/metrics/metric_histogram.hpp"
#include "realm/base/metrics/metric_max.hpp"
#include "realm/base/metrics/metric_mean.hpp"
#include "realm/base/metrics/metric_min.hpp"
#include "realm/base/metrics/metrics_bundle.hpp"

#include <realm/base/debug/assert.hpp>
#include <realm/base/metrics/metric_counter.hpp>
#include <realm/version/version.hpp>

#include <string_view>

#include <fmt/base.h>

using namespace realm;
using namespace realm::base::metrics;

enum class SandboxMetricId : u8
{
    counter,
    gauge,
    mean,
    min,
    max,
    histogram
};

namespace realm::base::metrics
{

template<>
struct MetricTraits<SandboxMetricId::counter>
{
    using metric_type = MetricCounter<>;

    static constexpr std::string_view name        = "counter";
    static constexpr std::string_view description = "Just some plain counter";
};

template<>
struct MetricTraits<SandboxMetricId::gauge>
{
    using metric_type = AtomicMetricGauge<>;

    static constexpr std::string_view name        = "gauge";
    static constexpr std::string_view description = "Just some atomic gauge";
};

template<>
struct MetricTraits<SandboxMetricId::mean>
{
    using metric_type = AtomicMetricMean<>;

    static constexpr std::string_view name        = "mean";
    static constexpr std::string_view description = "Just some atomic mean";
};

template<>
struct MetricTraits<SandboxMetricId::min>
{
    using metric_type = MetricMin<u32>;

    static constexpr std::string_view name        = "min";
    static constexpr std::string_view description = "Just some plain min";
};

template<>
struct MetricTraits<SandboxMetricId::max>
{
    using metric_type = AtomicMetricMax<u32>;

    static constexpr std::string_view name        = "max";
    static constexpr std::string_view description = "Just some atomic min";
};

template<>
struct MetricTraits<SandboxMetricId::histogram>
{
    using metric_type = MetricHistogram<float, 5>;

    static constexpr metric_type make() noexcept { return MetricHistogram<float, 5>{ { 0.0, 10.0, 15.0, 40.0 } }; }
};

}  // namespace realm::base::metrics

namespace
{

template<typename MetricT>
void printMetricValue(MetricT const& metric)
{
    if constexpr (requires { metric.value(); })
    {
        fmt::print("{}", metric.value());
    }
    else if constexpr (requires {
                           metric.snapshot();
                           metric.total();
                           metric.count(usize{ 0 });
                       })
    {
        auto const snap = metric.snapshot();
        auto const n    = metric.k_bucket_count;

        fmt::print("[");
        for (usize i = 0; i < n; ++i)
        {
            if (i)
            {
                fmt::print(", ");
            }
            fmt::print("{}", snap[i]);
        }
        fmt::print("] (total={})", metric.total());
    }
    else
    {
        fmt::print("<no scalar value>");
    }
}

}  // namespace

using StorageMetricsBundle =
    MetricsBundle<SandboxMetricId, SandboxMetricId::counter, SandboxMetricId::gauge, SandboxMetricId::mean,
                  SandboxMetricId::min, SandboxMetricId::max, SandboxMetricId::histogram>;

int main()
{
    StorageMetricsBundle bundle;

    bundle.get<SandboxMetricId::counter>().increment(42);
    bundle.get<SandboxMetricId::gauge>().set(-7);
    bundle.get<SandboxMetricId::mean>().observe(3.5);
    bundle.get<SandboxMetricId::mean>().observe(4.5);
    bundle.get<SandboxMetricId::min>().observe(12);
    bundle.get<SandboxMetricId::max>().observe(77);

    auto& h = bundle.get<SandboxMetricId::histogram>();
    h.observe(3.0f);
    h.observe(12.0f);
    h.observe(12.0f);
    h.observe(100.0f);

    bundle.visit(
        [](auto const& metric, MetricInfo<SandboxMetricId> const& info)
        {
            fmt::print("{:20} {:40} = ", info.name, info.description);
            printMetricValue(metric);
            fmt::print("\n");
        });

    return 0;
}
