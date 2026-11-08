#include "test_support.hpp"

#include <realm/base/metrics/metric_counter.hpp>

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
        MetricCounter<i32> metric;
        metric += 3;
        auto const previous = metric++;
        ++metric;
        return previous == 3 && metric.value() == 5;
    }());
static_assert(sizeof(MetricCounter<>) == sizeof(u64));
static_assert(!std::is_copy_constructible_v<MetricCounter<>>);
static_assert(!std::is_move_constructible_v<AtomicMetricCounter<>>);

template<typename P>
class MetricCounterTest : public testing::Test
{
};

TYPED_TEST_SUITE(MetricCounterTest, tests::ScalarPolicies, tests::PolicyNames);

//

TYPED_TEST(MetricCounterTest, InitializesDefaultAndExplicitValues)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricCounter<T, typename TypeParam::template storage_type<T>>;

    Metric const metric;
    Metric const initialized{ T{ 7 } };
    EXPECT_EQ(metric.value(), T{ 0 });
    EXPECT_EQ(initialized.value(), T{ 7 });
}

TYPED_TEST(MetricCounterTest, IncrementAndOperatorsReturnTheirDocumentedValues)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricCounter<T, typename TypeParam::template storage_type<T>>;

    Metric metric;
    EXPECT_EQ(metric.increment(), T{ 1 });
    EXPECT_EQ(metric.increment(T{ 4 }), T{ 5 });
    EXPECT_EQ(&(metric += T{ 3 }), &metric);
    EXPECT_EQ(metric.value(), T{ 8 });
    EXPECT_EQ(&(++metric), &metric);
    EXPECT_EQ(metric.value(), T{ 9 });
    EXPECT_EQ(metric++, T{ 9 });
    EXPECT_EQ(metric.value(), T{ 10 });
}

TYPED_TEST(MetricCounterTest, ResetReturnsToZeroInsteadOfInitialValue)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricCounter<T, typename TypeParam::template storage_type<T>>;

    Metric metric{ T{ 7 } };
    metric.increment(T{ 3 });
    metric.reset();

    EXPECT_EQ(metric.value(), T{ 0 });
    EXPECT_EQ(metric.increment(), T{ 1 });
}

TYPED_TEST(MetricCounterTest, HandlesWrappingOrFloatingPointRounding)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricCounter<T, typename TypeParam::template storage_type<T>>;

    if constexpr (std::integral<T>)
    {
        Metric metric{ std::numeric_limits<T>::max() };
        EXPECT_EQ(metric++, std::numeric_limits<T>::max());
        constexpr T expected = std::is_unsigned_v<T> ? T{ 0 } : std::numeric_limits<T>::min();
        EXPECT_EQ(metric.value(), expected);
    }
    else
    {
        // At 2^digits, adding one rounds away; subtracting one from the new
        // value would not reconstruct the old value returned by postfix.
        auto const boundary = std::ldexp(T{ 1 }, std::numeric_limits<T>::digits);
        Metric     metric{ boundary };
        EXPECT_EQ(metric++, boundary);
        EXPECT_EQ(metric.value(), boundary);
    }
}

TEST(AtomicMetricCounterTest, ConcurrentPostfixReturnsEveryPreviousValueOnce)
{
    AtomicMetricCounter<> metric;
    std::vector<u64>      previous(tests::k_operations);
    tests::runWriters(
        [&](usize writer)
        {
            for (usize i = 0; i < tests::k_iterations; ++i)
            {
                previous[(writer * tests::k_iterations) + i] = metric++;
            }
        });
    std::ranges::sort(previous);
    EXPECT_EQ(previous, tests::sequence(tests::k_operations));
    EXPECT_EQ(metric.value(), tests::k_operations);
}
}  // namespace
