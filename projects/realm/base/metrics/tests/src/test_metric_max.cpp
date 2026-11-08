#include "test_support.hpp"

#include <realm/base/metrics/metric_max.hpp>

#include <cmath>
#include <limits>

namespace
{

using namespace realm;
using namespace realm::base::metrics;

static_assert(
    []
    {
        MetricMax<i32> metric;
        metric.observe(2);
        metric.observe(5);
        return metric.value() == 5;
    }());
static_assert(!std::is_copy_constructible_v<MetricMax<i32>>);
static_assert(!std::is_move_constructible_v<AtomicMetricMax<i32>>);

template<typename P>
class MetricMaxTest : public testing::Test
{
};

TYPED_TEST_SUITE(MetricMaxTest, tests::ScalarPolicies, tests::PolicyNames);

//

template<typename P>
class MetricMaxFloatingTest : public testing::Test
{
};
TYPED_TEST_SUITE(MetricMaxFloatingTest, tests::FloatingPolicies, tests::PolicyNames);

//

TYPED_TEST(MetricMaxTest, InitializesToIdentityAndSupportsExplicitInitialValue)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricMax<T, typename TypeParam::template storage_type<T>>;

    Metric const metric;
    Metric const initialized{ T{ 4 } };
    constexpr T  identity =
        std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest();
    EXPECT_EQ(metric.value(), identity);
    EXPECT_EQ(initialized.value(), T{ 4 });
}

TYPED_TEST(MetricMaxTest, RetainsExtremeAndReturnsResultAfterEachObservation)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricMax<T, typename TypeParam::template storage_type<T>>;

    Metric metric;
    EXPECT_EQ(metric.observe(T{ 2 }), T{ 2 });
    EXPECT_EQ(metric.observe(T{ 5 }), T{ 5 });
    EXPECT_EQ(metric.observe(T{ 3 }), T{ 5 });
    EXPECT_EQ(metric.observe(T{ 5 }), T{ 5 });
    EXPECT_EQ(metric.value(), T{ 5 });
}

TYPED_TEST(MetricMaxTest, ResetRestoresIdentityInsteadOfInitialValue)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricMax<T, typename TypeParam::template storage_type<T>>;

    Metric metric{ T{ 4 } };
    metric.observe(T{ 5 });
    metric.reset();
    constexpr T identity =
        std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest();
    EXPECT_EQ(metric.value(), identity);
    EXPECT_EQ(metric.observe(T{ 2 }), T{ 2 });
}

//

TYPED_TEST(MetricMaxFloatingTest, HandlesInfiniteObservations)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricMax<T, typename TypeParam::template storage_type<T>>;

    auto const inf = std::numeric_limits<T>::infinity();
    Metric     metric;
    metric.observe(-inf);
    metric.observe(inf);
    EXPECT_EQ(metric.value(), inf);
}

TYPED_TEST(MetricMaxFloatingTest, NaNAndTiesFollowCurrentSampleFirstSelectionPolicy)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricMax<T, typename TypeParam::template storage_type<T>>;

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

TEST(AtomicMetricMaxTest, ConcurrentWritersRetainGlobalExtreme)
{
    AtomicMetricMax<u64> metric;
    tests::runWriters(
        [&](usize writer)
        {
            for (usize i = 0; i < tests::k_iterations; ++i)
            {
                metric.observe((writer * tests::k_iterations) + i);
            }
        });
    EXPECT_EQ(metric.value(), tests::k_operations - 1);
}
}  // namespace
