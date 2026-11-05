#include "metric_test_support.hpp"

#include <realm/base/metrics/metrics/max.hpp>

namespace
{

using namespace realm;
using namespace realm::base::metrics;
using namespace realm::base::metrics::tests;

template<typename Policy>
class MetricMaxTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(MetricMaxTest, StoragePolicies);

//

TYPED_TEST(MetricMaxTest, StartsAtNumericLimit)
{
    MetricMax<i64, min_value<i64>, TypeParam> const metric;
    EXPECT_EQ(metric.get(), min_value<i64>);
}

TYPED_TEST(MetricMaxTest, RetainsExtremeAcrossObservations)
{
    MetricMax<i64, min_value<i64>, TypeParam> metric;
    for (auto sample : { -8, -4, -10, -2, -6 })
    {
        metric.observe(sample);
    }
    EXPECT_EQ(metric.get(), -2);
}

TYPED_TEST(MetricMaxTest, UpdateHasSameBehaviorAsObserve)
{
    MetricMax<i64, min_value<i64>, TypeParam> metric;
    for (auto sample : { -8, -4, -10, -2, -6 })
    {
        metric.update(sample);
    }
    EXPECT_EQ(metric.get(), -2);
}

TYPED_TEST(MetricMaxTest, ResetsToTemplateValueAfterExplicitInitialization)
{
    MetricMax<i64, 5, TypeParam> metric{ 10 };
    EXPECT_EQ(metric.get(), 10);
    metric.observe(-3);
    metric.reset();
    EXPECT_EQ(metric.get(), 5);
}

TYPED_TEST(MetricMaxTest, SupportsNegativeFloatingPointValues)
{
    MetricMax<f64, min_value<f64>, TypeParam> metric;
    metric.observe(-4.5);
    metric.observe(-2.5);
    EXPECT_DOUBLE_EQ(metric.get(), -2.5);
}

TEST(AtomicMetricMaxTest, AccumulatesConcurrentUpdates)
{
    AtomicMetricMax<> metric;
    runConcurrently([&](i64 worker) { metric.observe(worker - 2); });
    EXPECT_EQ(metric.get(), 1);
}

}  // namespace
