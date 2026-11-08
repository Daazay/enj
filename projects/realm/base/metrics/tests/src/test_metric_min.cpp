#include "test_support.hpp"

#include <realm/base/metrics/metric_min.hpp>

#include <cmath>
#include <limits>

namespace
{

using namespace realm;
using namespace realm::base::metrics;

static_assert(
    []
    {
        MetricMin<i32> metric;
        metric.observe(5);
        metric.observe(3);
        return metric.value() == 3;
    }());
static_assert(!std::is_copy_constructible_v<MetricMin<i32>>);
static_assert(!std::is_move_constructible_v<AtomicMetricMin<i32>>);

template<typename P>
class MetricMinTest : public testing::Test
{
};

TYPED_TEST_SUITE(MetricMinTest, tests::ScalarPolicies, tests::PolicyNames);

//

template<typename P>
class MetricMinFloatingTest : public testing::Test
{
};

TYPED_TEST_SUITE(MetricMinFloatingTest, tests::FloatingPolicies, tests::PolicyNames);

//

TYPED_TEST(MetricMinTest, InitializesToIdentityAndSupportsExplicitInitialValue)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricMin<T, typename TypeParam::template storage_type<T>>;

    Metric const metric;
    Metric const initialized{ T{ 4 } };
    constexpr T  identity =
        std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity() : std::numeric_limits<T>::max();
    EXPECT_EQ(metric.value(), identity);
    EXPECT_EQ(initialized.value(), T{ 4 });
}

TYPED_TEST(MetricMinTest, RetainsExtremeAndReturnsResultAfterEachObservation)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricMin<T, typename TypeParam::template storage_type<T>>;

    Metric metric;
    EXPECT_EQ(metric.observe(T{ 5 }), T{ 5 });
    EXPECT_EQ(metric.observe(T{ 3 }), T{ 3 });
    EXPECT_EQ(metric.observe(T{ 7 }), T{ 3 });
    EXPECT_EQ(metric.observe(T{ 3 }), T{ 3 });
    EXPECT_EQ(metric.value(), T{ 3 });
}

TYPED_TEST(MetricMinTest, ResetRestoresIdentityInsteadOfInitialValue)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricMin<T, typename TypeParam::template storage_type<T>>;

    Metric metric{ T{ 4 } };
    metric.observe(T{ 3 });
    metric.reset();
    constexpr T identity =
        std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity() : std::numeric_limits<T>::max();
    EXPECT_EQ(metric.value(), identity);
    EXPECT_EQ(metric.observe(T{ 5 }), T{ 5 });
}

//

TYPED_TEST(MetricMinFloatingTest, HandlesInfiniteObservations)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricMin<T, typename TypeParam::template storage_type<T>>;

    auto const inf = std::numeric_limits<T>::infinity();
    Metric     metric;
    metric.observe(-inf);
    metric.observe(inf);
    EXPECT_EQ(metric.value(), -inf);
}

TYPED_TEST(MetricMinFloatingTest, NaNAndTiesFollowCurrentSampleFirstSelectionPolicy)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricMin<T, typename TypeParam::template storage_type<T>>;

    Metric metric{ T{ 4 } };
    metric.observe(std::numeric_limits<T>::quiet_NaN());
    EXPECT_TRUE(std::isnan(metric.value()));
    metric.observe(T{ 3 });
    EXPECT_EQ(metric.value(), T{ 3 });
    metric.reset();
    metric.observe(T{ 0 });
    metric.observe(-T{ 0 });
    EXPECT_TRUE(std::signbit(metric.value()));
    metric.observe(T{ 0 });
    EXPECT_FALSE(std::signbit(metric.value()));
}

TEST(AtomicMetricMinTest, ConcurrentWritersRetainGlobalExtreme)
{
    AtomicMetricMin<u64> metric;
    tests::runWriters(
        [&](usize writer)
        {
            for (usize i = 0; i < tests::k_iterations; ++i)
            {
                metric.observe((writer * tests::k_iterations) + i);
            }
        });
    EXPECT_EQ(metric.value(), u64{ 0 });
}
}  // namespace
