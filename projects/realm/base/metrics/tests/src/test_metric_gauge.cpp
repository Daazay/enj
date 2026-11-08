#include "test_support.hpp"

#include <realm/base/metrics/metric_gauge.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{

using namespace realm;
using namespace realm::base::metrics;

static_assert(
    []
    {
        MetricGauge<i32> metric;
        // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
        if (metric.set(3) != 3 || metric++ != 3 || metric-- != 4)
        {
            return false;
        }
        return metric.exchange(7) == 3 && metric.value() == 7;
    }());
static_assert(std::same_as<MetricGauge<>::value_type, i64>);
static_assert(!std::is_copy_constructible_v<MetricGauge<>>);
static_assert(!std::is_move_constructible_v<AtomicMetricGauge<>>);

template<typename P>
class MetricGaugeTest : public testing::Test
{
};

TYPED_TEST_SUITE(MetricGaugeTest, tests::ScalarPolicies, tests::PolicyNames);

//

TYPED_TEST(MetricGaugeTest, InitializesDefaultAndExplicitValues)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricGauge<T, typename TypeParam::template storage_type<T>>;

    Metric const metric;
    Metric const initialized{ T{ 7 } };
    EXPECT_EQ(metric.value(), T{ 0 });
    EXPECT_EQ(initialized.value(), T{ 7 });
}

TYPED_TEST(MetricGaugeTest, SetReturnsNewValueAndExchangeReturnsPreviousValue)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricGauge<T, typename TypeParam::template storage_type<T>>;

    Metric metric;
    EXPECT_EQ(metric.set(T{ 12 }), T{ 12 });
    EXPECT_EQ(metric.value(), T{ 12 });
    EXPECT_EQ(metric.exchange(T{ 20 }), T{ 12 });
    EXPECT_EQ(metric.value(), T{ 20 });
    EXPECT_EQ(&(metric = T{ 5 }), &metric);
    EXPECT_EQ(metric.value(), T{ 5 });
}

TYPED_TEST(MetricGaugeTest, ArithmeticAndOperatorsReturnTheirDocumentedValues)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricGauge<T, typename TypeParam::template storage_type<T>>;

    Metric metric{ T{ 10 } };
    EXPECT_EQ(metric.increment(T{ 4 }), T{ 14 });
    EXPECT_EQ(metric.decrement(T{ 2 }), T{ 12 });
    EXPECT_EQ(metric.increment(), T{ 13 });
    EXPECT_EQ(metric.decrement(), T{ 12 });
    EXPECT_EQ(&(metric += T{ 3 }), &metric);
    EXPECT_EQ(&(metric -= T{ 2 }), &metric);
    EXPECT_EQ(metric.value(), T{ 13 });
    EXPECT_EQ(&(++metric), &metric);
    EXPECT_EQ(metric.value(), T{ 14 });
    EXPECT_EQ(&(--metric), &metric);
    EXPECT_EQ(metric.value(), T{ 13 });
    EXPECT_EQ(metric++, T{ 13 });
    EXPECT_EQ(metric--, T{ 14 });
    EXPECT_EQ(metric.value(), T{ 13 });
}

TYPED_TEST(MetricGaugeTest, ResetReturnsToZeroInsteadOfInitialValue)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricGauge<T, typename TypeParam::template storage_type<T>>;

    Metric metric{ T{ 12 } };
    metric.reset();
    EXPECT_EQ(metric.value(), T{ 0 });
}

TYPED_TEST(MetricGaugeTest, HandlesWrappingOrFloatingPointRounding)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricGauge<T, typename TypeParam::template storage_type<T>>;

    if constexpr (std::integral<T>)
    {
        Metric metric{ std::numeric_limits<T>::max() };
        EXPECT_EQ(metric++, std::numeric_limits<T>::max());
        constexpr T wrapped = std::is_unsigned_v<T> ? T{ 0 } : std::numeric_limits<T>::min();
        EXPECT_EQ(metric.value(), wrapped);
        EXPECT_EQ(metric--, wrapped);
        EXPECT_EQ(metric.value(), std::numeric_limits<T>::max());
    }
    else
    {
        auto const boundary = std::ldexp(T{ 1 }, std::numeric_limits<T>::digits);
        Metric     metric{ boundary };
        EXPECT_EQ(metric++, boundary);
        EXPECT_EQ(metric.value(), boundary);
        metric.set(-boundary);
        EXPECT_EQ(metric--, -boundary);
        EXPECT_EQ(metric.value(), -boundary);
    }
}

TEST(AtomicMetricGaugeTest, ConcurrentArithmeticDoesNotLoseUpdates)
{
    AtomicMetricGauge<> metric;
    tests::runWriters(
        [&](usize)
        {
            for (usize i = 0; i < tests::k_iterations; ++i)
            {
                metric.increment(2);
                metric.decrement();
            }
        });
    EXPECT_EQ(metric.value(), static_cast<i64>(tests::k_operations));
}

TEST(AtomicMetricGaugeTest, ConcurrentPostfixReturnsUniquePreviousValues)
{
    AtomicMetricGauge<u64> increasing;
    AtomicMetricGauge<u64> decreasing{ tests::k_operations };
    std::vector<u64>       increments(tests::k_operations);
    std::vector<u64>       decrements(tests::k_operations);
    tests::runWriters(
        [&](usize writer)
        {
            for (usize i = 0; i < tests::k_iterations; ++i)
            {
                auto const index  = (writer * tests::k_iterations) + i;
                increments[index] = increasing++;
                decrements[index] = decreasing--;
            }
        });
    std::ranges::sort(increments);
    std::ranges::sort(decrements);
    EXPECT_EQ(increments, tests::sequence(tests::k_operations));
    EXPECT_EQ(decrements, tests::sequence(tests::k_operations, 1));
    EXPECT_EQ(increasing.value(), tests::k_operations);
    EXPECT_EQ(decreasing.value(), u64{ 0 });
}

TEST(AtomicMetricGaugeTest, ConcurrentExchangesReturnEachWrittenValueOnce)
{
    AtomicMetricGauge<u64> metric;
    std::vector<u64>       previous(tests::k_operations + 1);
    tests::runWriters(
        [&](usize writer)
        {
            for (usize i = 0; i < tests::k_iterations; ++i)
            {
                auto const index = (writer * tests::k_iterations) + i;
                previous[index]  = metric.exchange(index + 1);
            }
        });
    previous.back() = metric.value();
    std::ranges::sort(previous);
    EXPECT_EQ(previous, tests::sequence(tests::k_operations + 1));
}
}  // namespace
