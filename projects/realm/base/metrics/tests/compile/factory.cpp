#include <realm/base/metrics/detail/metric_descriptor.hpp>
#include <realm/base/metrics/metrics/counter.hpp>

enum class TestId
{
    counter,
};

namespace realm::base::metrics
{

template<>
struct MetricTraits<TestId::counter>
{
    using metric_type = MetricCounter<>;

#if RE_METRIC_TEST_CASE != 5
    static constexpr std::string_view name = "counter";
#endif

#if RE_METRIC_TEST_CASE == 1
    static consteval metric_type make() { return metric_type{ 7 }; }
#elif RE_METRIC_TEST_CASE == 2
    static consteval int make() noexcept { return 7; }
#elif RE_METRIC_TEST_CASE == 3
    static consteval metric_type make(int) noexcept { return metric_type{ 7 }; }
#elif RE_METRIC_TEST_CASE == 4
    static metric_type make() noexcept { return metric_type{ 7 }; }
#elif RE_METRIC_TEST_CASE == 5
    static constexpr int name = 7;
#elif RE_METRIC_TEST_CASE == 6
    static consteval metric_type make() noexcept { return metric_type{ 7 }; }
#endif
};

}  // namespace realm::base::metrics

using Descriptor = realm::base::metrics::detail::MetricDescriptor<TestId::counter>;
constexpr Descriptor descriptor;
static_assert(Descriptor::name == "counter");
#if RE_METRIC_TEST_CASE == 6
static_assert(descriptor.metric.get() == 7);
#else
static_assert(descriptor.metric.get() == 0);
#endif
