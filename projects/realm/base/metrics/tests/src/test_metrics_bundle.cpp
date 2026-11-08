#include "realm/base/common/defines.hpp"

#include "test_support.hpp"

#include <realm/base/common/string_literal.hpp>
#include <realm/base/metrics/metric_counter.hpp>
#include <realm/base/metrics/metric_gauge.hpp>
#include <realm/base/metrics/metric_histogram.hpp>
#include <realm/base/metrics/metric_max.hpp>
#include <realm/base/metrics/metric_mean.hpp>
#include <realm/base/metrics/metric_min.hpp>
#include <realm/base/metrics/metrics_bundle.hpp>

#include <limits>
#include <string_view>

namespace
{

using namespace realm;
using namespace realm::base::metrics;

enum class Id : u8
{
    counter          = 3,
    gauge            = 7,
    minimum          = 11,
    maximum          = 19,
    mean             = 23,
    histogram        = 31,
    atomic_counter   = 43,
    atomic_gauge     = 47,
    atomic_minimum   = 53,
    atomic_maximum   = 59,
    atomic_mean      = 61,
    atomic_histogram = 67,
    non_resettable   = 71,
    configured       = 79,
    second_counter   = 83
};

std::array<i32, 3> configured_bounds{ 1, 4, 16 };

class NonResettableMetric
{
public:
    constexpr explicit NonResettableMetric(i32 initial) noexcept
        : _value{ initial }
    {
    }
    NonResettableMetric(NonResettableMetric const&)            = delete;
    NonResettableMetric& operator=(NonResettableMetric const&) = delete;
    NonResettableMetric(NonResettableMetric&&)                 = delete;
    NonResettableMetric& operator=(NonResettableMetric&&)      = delete;
    constexpr ~NonResettableMetric() noexcept                  = default;
    RE_NODISCARD constexpr i32 value() const noexcept { return _value; }

private:
    i32 _value;
};
}  // namespace

namespace realm::base::metrics
{
template<>
struct MetricTraits<Id::counter>
{
    using metric_type                             = MetricCounter<>;
    static constexpr auto             name        = StringLiteral{ "frames_total" };
    static constexpr std::string_view description = "Completed frames";
};
template<>
struct MetricTraits<Id::second_counter>
{
    using metric_type = MetricCounter<>;
};
template<>
struct MetricTraits<Id::gauge>
{
    using metric_type = MetricGauge<>;
    static constexpr metric_type make() noexcept { return metric_type{ 42 }; }
};
template<>
struct MetricTraits<Id::minimum>
{
    using metric_type = MetricMin<f64>;
};
template<>
struct MetricTraits<Id::maximum>
{
    using metric_type = MetricMax<f64>;
};
template<>
struct MetricTraits<Id::mean>
{
    using metric_type = MetricMean<>;
};
template<>
struct MetricTraits<Id::histogram>
{
    using metric_type = MetricHistogram<f64, 4>;
    static constexpr metric_type make() noexcept { return metric_type{ { 1, 4, 16 } }; }
};
template<>
struct MetricTraits<Id::atomic_counter>
{
    using metric_type = AtomicMetricCounter<>;
};
template<>
struct MetricTraits<Id::atomic_gauge>
{
    using metric_type = AtomicMetricGauge<>;
};
template<>
struct MetricTraits<Id::atomic_minimum>
{
    using metric_type = AtomicMetricMin<f64>;
};
template<>
struct MetricTraits<Id::atomic_maximum>
{
    using metric_type = AtomicMetricMax<f64>;
};
template<>
struct MetricTraits<Id::atomic_mean>
{
    using metric_type = AtomicMetricMean<>;
};
template<>
struct MetricTraits<Id::atomic_histogram>
{
    using metric_type = AtomicMetricHistogram<f64, 4>;
    static constexpr metric_type make() noexcept { return metric_type{ { 1, 4, 16 } }; }
};
template<>
struct MetricTraits<Id::non_resettable>
{
    using metric_type = NonResettableMetric;
    static constexpr metric_type make() noexcept { return metric_type{ 99 }; }
};
template<>
struct MetricTraits<Id::configured>
{
    using metric_type = MetricHistogram<i32, 4>;
    static metric_type make() noexcept { return metric_type{ configured_bounds }; }
};
}  // namespace realm::base::metrics

namespace
{

using Bundle =
    MetricsBundle<Id, Id::gauge, Id::counter, Id::histogram, Id::mean, Id::minimum, Id::maximum, Id::atomic_counter,
                  Id::atomic_gauge, Id::atomic_minimum, Id::atomic_maximum, Id::atomic_mean, Id::atomic_histogram,
                  Id::non_resettable, Id::configured, Id::second_counter>;
using Info = MetricInfo<Id>;

static_assert(!std::is_copy_constructible_v<Bundle>);
static_assert(!std::is_move_constructible_v<Bundle>);
static_assert(std::same_as<decltype(std::declval<Bundle&>().get<Id::counter>()), MetricCounter<>&>);
static_assert(std::same_as<decltype(std::declval<Bundle const&>().get<Id::counter>()), MetricCounter<> const&>);
static_assert(
    []
    {
        MetricsBundle<Id, Id::counter, Id::gauge, Id::histogram> metrics;
        metrics.get<Id::counter>().increment(3);
        metrics.get<Id::histogram>().observe(2);
        return metrics.get<Id::counter>().value() == 3 && metrics.get<Id::gauge>().value() == 42 &&
               metrics.get<Id::histogram>().count(1) == 1;
    }());

struct CounterOnlyVisitor
{
    void operator()(MetricCounter<> const& /*metric*/, Info const& /*info*/) const {}
};
struct UnrelatedVisitor
{
    void operator()(std::string_view /*value*/, Info const& /*info*/) const {}
};

template<typename Visitor>
concept CanVisitAll = requires(Bundle const& metrics, Visitor visitor) { metrics.visit(visitor); };
template<typename Visitor>
concept CanVisitSome = requires(Bundle const& metrics, Visitor visitor) { metrics.visitIfInvocable(visitor); };

static_assert(!CanVisitAll<CounterOnlyVisitor>);
static_assert(CanVisitSome<CounterOnlyVisitor>);
static_assert(!CanVisitSome<UnrelatedVisitor>);

class MetricsBundleTest : public testing::Test
{
protected:
    void SetUp() override { configured_bounds = { 1, 4, 16 }; }
};

//

TEST_F(MetricsBundleTest, ConstructsEveryMetricIncludingNonMovableCustomFactories)
{
    Bundle const metrics;
    EXPECT_EQ(metrics.metrics_count, usize{ 15 });
    EXPECT_EQ(metrics.get<Id::counter>().value(), u64{ 0 });
    EXPECT_EQ(metrics.get<Id::gauge>().value(), 42);
    EXPECT_EQ(metrics.get<Id::atomic_counter>().value(), u64{ 0 });
    EXPECT_EQ(metrics.get<Id::histogram>().bound(2), 16);
    EXPECT_EQ(metrics.get<Id::atomic_histogram>().bound(2), 16);
    EXPECT_EQ(metrics.get<Id::non_resettable>().value(), 99);
}

TEST_F(MetricsBundleTest, RetrievesSeparateInstancesOfTheSameConcreteTypeById)
{
    Bundle metrics;
    metrics.get<Id::counter>().increment(3);
    metrics.get<Id::second_counter>().increment(7);
    Bundle const& read_only = metrics;
    EXPECT_EQ(read_only.get<Id::counter>().value(), u64{ 3 });
    EXPECT_EQ(read_only.get<Id::second_counter>().value(), u64{ 7 });
    EXPECT_NE(&read_only.get<Id::counter>(), &read_only.get<Id::second_counter>());
}

TEST_F(MetricsBundleTest, CopiesRuntimeConfiguredBoundsAtConstruction)
{
    configured_bounds = { 2, 8, 32 };
    Bundle const metrics;
    configured_bounds = { 3, 9, 27 };
    EXPECT_EQ(metrics.get<Id::configured>().bound(0), 2);
    EXPECT_EQ(metrics.get<Id::configured>().bound(1), 8);
    EXPECT_EQ(metrics.get<Id::configured>().bound(2), 32);
}

TEST_F(MetricsBundleTest, ExhaustiveVisitorReceivesConstMetricsInDeclaredOrder)
{
    Bundle const    metrics;
    std::vector<Id> visited;
    metrics.visit(
        [&](auto const& metric, Info const& info)
        {
            static_assert(std::is_const_v<std::remove_reference_t<decltype(metric)>>);
            visited.push_back(info.id);
            return false;  // Return values must not short-circuit visitation.
        });
    std::vector<Id> const expected{ Id::gauge,          Id::counter,        Id::histogram,      Id::mean,
                                    Id::minimum,        Id::maximum,        Id::atomic_counter, Id::atomic_gauge,
                                    Id::atomic_minimum, Id::atomic_maximum, Id::atomic_mean,    Id::atomic_histogram,
                                    Id::non_resettable, Id::configured,     Id::second_counter };
    EXPECT_EQ(visited, expected);
    Bundle const& read_only = metrics;
    usize         count     = 0;
    auto          visitor   = [&](auto const&, Info const&)
    {
        ++count;
    };
    read_only.visit(visitor);
    EXPECT_EQ(count, metrics.metrics_count);
}

TEST_F(MetricsBundleTest, MetadataUsesOverridesFallbackNamesAndStaticLifetime)
{
    Info const* retained = nullptr;
    {
        Bundle const metrics;
        metrics.visit(
            [&](auto const&, Info const& info)
            {
                if (info.id == Id::counter)
                {
                    EXPECT_EQ(info.name, "frames_total");
                    EXPECT_EQ(info.description, "Completed frames");
                    EXPECT_EQ(&info, &MetricDescriptor<Id::counter>::info);
                    retained = &info;
                }
                else if (info.id == Id::atomic_counter)
                {
                    EXPECT_EQ(info.name, "atomic_counter");
                    EXPECT_TRUE(info.description.empty());
                }
            });
    }
    ASSERT_NE(retained, nullptr);
    EXPECT_EQ(retained->id, Id::counter);
    EXPECT_EQ(retained->name, "frames_total");
}

TEST_F(MetricsBundleTest, PartialVisitorSkipsNonMatchingTypesAndVisitsRepeatedTypes)
{
    Bundle const    metrics;
    std::vector<Id> visited;
    metrics.visitIfInvocable([&](MetricCounter<> const&, Info const& info) { visited.push_back(info.id); });
    std::vector<Id> const expected{ Id::counter, Id::second_counter };
    EXPECT_EQ(visited, expected);
    usize histograms = 0;
    metrics.visitIfInvocable(
        [&](MetricHistogram<f64, 4> const&, Info const& info)
        {
            EXPECT_EQ(info.id, Id::histogram);
            ++histograms;
        });
    EXPECT_EQ(histograms, usize{ 1 });
}

struct NonCopyableVisitor
{
    usize& count;
    explicit NonCopyableVisitor(usize& value) noexcept
        : count{ value }
    {
    }
    NonCopyableVisitor(NonCopyableVisitor const&)            = delete;
    NonCopyableVisitor& operator=(NonCopyableVisitor const&) = delete;
    NonCopyableVisitor(NonCopyableVisitor&&)                 = delete;
    NonCopyableVisitor& operator=(NonCopyableVisitor&&)      = delete;
    ~NonCopyableVisitor()                                    = default;

    template<typename Metric>
    void operator()(RE_MAYBE_UNUSED Metric const& m, RE_MAYBE_UNUSED Info const& info)
    {
        ++count;
    }
};

TEST_F(MetricsBundleTest, SupportsLvalueAndTemporaryVisitorsWithoutCopyingOrMoving)
{
    Bundle const       metrics;
    usize              count = 0;
    NonCopyableVisitor visitor{ count };
    metrics.visit(visitor);
    metrics.visit(NonCopyableVisitor{ count });
    metrics.visitIfInvocable(visitor);
    EXPECT_EQ(count, 3 * metrics.metrics_count);
}

TEST_F(MetricsBundleTest, ResetAllResetsEverySupportedMetricAndSkipsMetricsWithoutReset)
{
    Bundle metrics;
    metrics.get<Id::counter>().increment(3);
    metrics.get<Id::second_counter>().increment(5);
    metrics.get<Id::gauge>().set(7);
    metrics.get<Id::minimum>().observe(2);
    metrics.get<Id::maximum>().observe(8);
    metrics.get<Id::mean>().observe(4);
    metrics.get<Id::histogram>().observe(3);
    metrics.get<Id::atomic_counter>().increment(3);
    metrics.get<Id::atomic_gauge>().set(7);
    metrics.get<Id::atomic_minimum>().observe(2);
    metrics.get<Id::atomic_maximum>().observe(8);
    metrics.get<Id::atomic_mean>().observe(4);
    metrics.get<Id::atomic_histogram>().observe(3);
    metrics.get<Id::configured>().observe(3);
    metrics.resetAll();
    EXPECT_EQ(metrics.get<Id::counter>().value(), u64{ 0 });
    EXPECT_EQ(metrics.get<Id::second_counter>().value(), u64{ 0 });
    EXPECT_EQ(metrics.get<Id::gauge>().value(), 0);
    EXPECT_EQ(metrics.get<Id::minimum>().value(), std::numeric_limits<f64>::infinity());
    EXPECT_EQ(metrics.get<Id::maximum>().value(), -std::numeric_limits<f64>::infinity());
    EXPECT_EQ(metrics.get<Id::mean>().snapshot().count, u64{ 0 });
    EXPECT_EQ(metrics.get<Id::mean>().snapshot().sum, 0);
    EXPECT_EQ(metrics.get<Id::histogram>().total(), u64{ 0 });
    EXPECT_EQ(metrics.get<Id::histogram>().bound(1), 4);
    EXPECT_EQ(metrics.get<Id::atomic_counter>().value(), u64{ 0 });
    EXPECT_EQ(metrics.get<Id::atomic_gauge>().value(), 0);
    EXPECT_EQ(metrics.get<Id::atomic_minimum>().value(), std::numeric_limits<f64>::infinity());
    EXPECT_EQ(metrics.get<Id::atomic_maximum>().value(), -std::numeric_limits<f64>::infinity());
    EXPECT_EQ(metrics.get<Id::atomic_mean>().snapshot().count, u64{ 0 });
    EXPECT_EQ(metrics.get<Id::atomic_mean>().snapshot().sum, 0);
    EXPECT_EQ(metrics.get<Id::atomic_histogram>().total(), u64{ 0 });
    EXPECT_EQ(metrics.get<Id::configured>().total(), u64{ 0 });
    EXPECT_EQ(metrics.get<Id::configured>().bound(1), 4);
    EXPECT_EQ(metrics.get<Id::non_resettable>().value(), 99);
}

// GoogleTest's death-test macro uses goto internally.
// NOLINTBEGIN(cppcoreguidelines-avoid-goto)
TEST(MetricsBundleDeathTest, ValidatesRuntimeHistogramFactoryBoundsInEveryBuild)
{
    EXPECT_DEATH((
                     []
                     {
                         configured_bounds = { 4, 2, 16 };
                         Bundle const metrics;
                         RE_IGNORE(metrics);
                     }()),
                 "strictly increasing");
}
// NOLINTEND(cppcoreguidelines-avoid-goto)
}  // namespace
