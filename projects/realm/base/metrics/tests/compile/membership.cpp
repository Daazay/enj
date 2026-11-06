#include <realm/base/metrics/metric_storage.hpp>
#include <realm/base/metrics/metrics/counter.hpp>

enum class TestId : realm::u32
{
    first          = 1000,
    second         = 50000,
    missing_traits = 70000,
};

enum class OtherId
{
    third,
};

namespace realm::base::metrics
{

template<>
struct MetricTraits<TestId::first>
{
    using metric_type = MetricCounter<>;
#if RE_METRIC_TEST_CASE == 6
    static constexpr std::string_view name = "";
#endif
};

template<>
struct MetricTraits<TestId::second>
{
    using metric_type = MetricCounter<>;
#if RE_METRIC_TEST_CASE == 4
    static constexpr std::string_view name = "first";
#else
    static constexpr std::string_view name = "custom.second";
#endif
};

}  // namespace realm::base::metrics

using namespace realm::base::metrics;

#if RE_METRIC_TEST_CASE == 1
MetricStorage<TestId::first, TestId::first> metrics;
#elif RE_METRIC_TEST_CASE == 2
MetricStorage<TestId::first, OtherId::third> metrics;
#elif RE_METRIC_TEST_CASE == 3
MetricStorage<TestId::missing_traits> metrics;
#elif RE_METRIC_TEST_CASE == 5
using Metrics = MetricStorage<TestId::first>;
Metrics metrics;
static_assert(Metrics::name<TestId::first>() == "first");
static_assert(Metrics::indexOfName<"first">() == 0);
#elif RE_METRIC_TEST_CASE == 7
MetricStorage<TestId::first> metrics;
auto&                        excluded = metrics.get<TestId::second>();
#elif RE_METRIC_TEST_CASE == 8
MetricStorage<TestId::first> metrics;
auto&                        excluded = metrics.getByName<"custom.second">();
#elif RE_METRIC_TEST_CASE == 9
MetricStorage<1000> metrics;
#elif RE_METRIC_TEST_CASE == 10
MetricStorage<> metrics;
#else
using Metrics = MetricStorage<TestId::second, TestId::first>;
Metrics metrics;
static_assert(Metrics::metrics_count == 2);
static_assert(Metrics::idAt(0) == TestId::second);
static_assert(Metrics::indexOf<TestId::first>() == 1);
static_assert(Metrics::indexOfName<"first">() == 1);
static_assert(Metrics::indexOfName<"custom.second">() == 0);
static_assert(Metrics::name<TestId::first>() == "first");
static_assert(Metrics::name<TestId::second>() == "custom.second");
#endif
