#include "metric_test_support.hpp"

#include <realm/base/metrics/metrics/min.hpp>

namespace
{

using namespace realm;
using namespace realm::base::metrics;
using namespace realm::base::metrics::tests;

template<typename Policy>
class MetricMinTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(MetricMinTest, StoragePolicies);

//

TYPED_TEST(MetricMinTest, StartsAtNumericLimit)
{
    MetricMin<i64, max_value<i64>, TypeParam> const metric;
    EXPECT_EQ(metric.get(), max_value<i64>);
}

TYPED_TEST(MetricMinTest, RetainsExtremeAcrossObservations)
{
    MetricMin<i64, max_value<i64>, TypeParam> metric;
    for (auto sample : { 8, -4, 2, -8, 10 })
    {
        metric.observe(sample);
    }
    EXPECT_EQ(metric.get(), -8);
}

TYPED_TEST(MetricMinTest, UpdateHasSameBehaviorAsObserve)
{
    MetricMin<i64, max_value<i64>, TypeParam> metric;
    for (auto sample : { 8, -4, 2, -8, 10 })
    {
        metric.update(sample);
    }
    EXPECT_EQ(metric.get(), -8);
}

TYPED_TEST(MetricMinTest, ResetsToTemplateValueAfterExplicitInitialization)
{
    MetricMin<i64, 5, TypeParam> metric{ 10 };
    EXPECT_EQ(metric.get(), 10);
    metric.observe(-3);
    metric.reset();
    EXPECT_EQ(metric.get(), 5);
}

TYPED_TEST(MetricMinTest, SupportsNegativeFloatingPointValues)
{
    MetricMin<f64, max_value<f64>, TypeParam> metric;
    metric.observe(-4.5);
    metric.observe(-2.5);
    EXPECT_DOUBLE_EQ(metric.get(), -4.5);
}

TEST(AtomicMetricMinTest, AccumulatesConcurrentUpdates)
{
    AtomicMetricMin<> metric;
    runConcurrently([&](i64 worker) { metric.observe(worker - 2); });
    EXPECT_EQ(metric.get(), -2);
}

}  // namespace
