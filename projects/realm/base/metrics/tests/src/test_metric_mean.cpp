#include "test_support.hpp"

#include <realm/base/metrics/metric_mean.hpp>

#include <cmath>
#include <limits>

namespace
{

using namespace realm;
using namespace realm::base::metrics;

static_assert(
    []
    {
        MetricMean<> metric;
        metric.observe(1);
        metric.observe(2);
        return metric.value() == 1.5 && metric.snapshot().sum == 3 && metric.snapshot().count == 2;
    }());
static_assert(!std::is_copy_constructible_v<MetricMean<>>);
static_assert(!std::is_move_constructible_v<AtomicMetricMean<>>);

template<typename P>
class MetricMeanTest : public testing::Test
{
};

TYPED_TEST_SUITE(MetricMeanTest, tests::ScalarPolicies, tests::PolicyNames);

//

template<typename P>
class MetricMeanFloatingTest : public testing::Test
{
};

TYPED_TEST_SUITE(MetricMeanFloatingTest, tests::FloatingPolicies, tests::PolicyNames);

//

TYPED_TEST(MetricMeanTest, EmptyMeanAndSnapshotAreZero)
{
    using T = typename TypeParam::value_type;
    using Metric =
        MetricMean<T, typename TypeParam::template storage_type<T>, typename TypeParam::template storage_type<u64>>;

    Metric const metric;
    EXPECT_EQ(metric.snapshot().sum, T{ 0 });
    EXPECT_EQ(metric.snapshot().count, u64{ 0 });
    EXPECT_DOUBLE_EQ(metric.snapshot().mean(), 0);
    EXPECT_DOUBLE_EQ(metric.value(), 0);
}

TYPED_TEST(MetricMeanTest, TracksSumCountAndFractionalMean)
{
    using T = typename TypeParam::value_type;
    using Metric =
        MetricMean<T, typename TypeParam::template storage_type<T>, typename TypeParam::template storage_type<u64>>;

    Metric metric;
    metric.observe(T{ 1 });
    metric.observe(T{ 2 });
    EXPECT_EQ(metric.snapshot().sum, T{ 3 });
    EXPECT_EQ(metric.snapshot().count, u64{ 2 });
    EXPECT_DOUBLE_EQ(metric.value(), 1.5);
    metric.observe(T{ 7 });
    EXPECT_DOUBLE_EQ(metric.value(), 10.0 / 3.0);
    EXPECT_DOUBLE_EQ(metric.snapshot().mean(), metric.value());
}

TYPED_TEST(MetricMeanTest, SnapshotIsIndependentOfSubsequentObservations)
{
    using T = typename TypeParam::value_type;
    using Metric =
        MetricMean<T, typename TypeParam::template storage_type<T>, typename TypeParam::template storage_type<u64>>;

    Metric metric;
    metric.observe(T{ 1 });
    metric.observe(T{ 2 });
    auto const snapshot = metric.snapshot();
    metric.observe(T{ 9 });
    EXPECT_EQ(snapshot.sum, T{ 3 });
    EXPECT_EQ(snapshot.count, u64{ 2 });
    EXPECT_DOUBLE_EQ(snapshot.mean(), 1.5);
    EXPECT_DOUBLE_EQ(metric.value(), 4.0);
}

TYPED_TEST(MetricMeanTest, ResetClearsBothAccumulators)
{
    using T = typename TypeParam::value_type;
    using Metric =
        MetricMean<T, typename TypeParam::template storage_type<T>, typename TypeParam::template storage_type<u64>>;

    Metric metric;
    metric.observe(T{ 7 });
    metric.reset();
    EXPECT_EQ(metric.snapshot().sum, T{ 0 });
    EXPECT_EQ(metric.snapshot().count, u64{ 0 });
    EXPECT_DOUBLE_EQ(metric.value(), 0);
    metric.observe(T{ 3 });
    EXPECT_DOUBLE_EQ(metric.value(), 3);
}

TYPED_TEST(MetricMeanTest, SupportsMixedSumAndCountStoragePolicies)
{
    using T            = typename TypeParam::value_type;
    using CountStorage = std::conditional_t<TypeParam::k_atomic, detail::MetricValueStoragePlain<u64>,
                                            detail::MetricValueStorageAtomic<u64>>;

    MetricMean<T, typename TypeParam::template storage_type<T>, CountStorage> metric;
    metric.observe(T{ 2 });
    metric.observe(T{ 4 });
    EXPECT_EQ(metric.snapshot().sum, T{ 6 });
    EXPECT_EQ(metric.snapshot().count, u64{ 2 });
    EXPECT_DOUBLE_EQ(metric.value(), 3);
    metric.reset();
    EXPECT_DOUBLE_EQ(metric.value(), 0);
}

//

TYPED_TEST(MetricMeanFloatingTest, RetainsIEEEInfinityAndNaNSemantics)
{
    using T = typename TypeParam::value_type;
    using Metric =
        MetricMean<T, typename TypeParam::template storage_type<T>, typename TypeParam::template storage_type<u64>>;

    Metric metric;
    metric.observe(std::numeric_limits<T>::infinity());
    EXPECT_EQ(metric.value(), std::numeric_limits<T>::infinity());
    metric.reset();
    metric.observe(std::numeric_limits<T>::quiet_NaN());
    EXPECT_TRUE(std::isnan(metric.value()));
    EXPECT_EQ(metric.snapshot().count, u64{ 1 });
}

TEST(AtomicMetricMeanTest, ConcurrentWritersProduceExactTotalsAfterJoining)
{
    AtomicMetricMean<> metric;
    tests::runWriters(
        [&](usize writer)
        {
            for (usize i = 0; i < tests::k_iterations; ++i)
            {
                metric.observe(static_cast<f64>(writer + 1));
            }
        });
    auto const snapshot = metric.snapshot();
    EXPECT_EQ(snapshot.count, tests::k_operations);
    EXPECT_DOUBLE_EQ(snapshot.sum, static_cast<f64>(tests::k_iterations * 10));
    EXPECT_DOUBLE_EQ(snapshot.mean(), 2.5);
    EXPECT_DOUBLE_EQ(metric.value(), 2.5);
}
}  // namespace
