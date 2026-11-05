#include "metric_test_support.hpp"

#include <realm/base/metrics/metrics/mean.hpp>

namespace
{

using namespace realm;
using namespace realm::base::metrics;
using namespace realm::base::metrics::tests;

template<typename Policy>
class MetricMeanTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(MetricMeanTest, StoragePolicies);

//

TYPED_TEST(MetricMeanTest, ReturnsZeroWithoutSamples)
{
    MetricMean<i64, TypeParam> const mean;
    EXPECT_DOUBLE_EQ(mean.get(), 0.0);
    EXPECT_DOUBLE_EQ(mean.sum(), 0.0);
    EXPECT_EQ(mean.count(), 0u);
}

TYPED_TEST(MetricMeanTest, PreservesFractionalMeanForIntegerSamples)
{
    MetricMean<i64, TypeParam> mean;
    mean.add(2);
    mean.add(5);
    EXPECT_DOUBLE_EQ(mean.get(), 3.5);
    EXPECT_DOUBLE_EQ(mean.sum(), 7.0);
    EXPECT_EQ(mean.count(), 2u);
}

TYPED_TEST(MetricMeanTest, SupportsSignedFloatingPointSamples)
{
    MetricMean<f64, TypeParam> mean;
    mean.add(-2.5);
    mean.add(1.0);
    EXPECT_DOUBLE_EQ(mean.get(), -0.75);
    EXPECT_DOUBLE_EQ(mean.sum(), -1.5);
    EXPECT_EQ(mean.count(), 2u);
}

TYPED_TEST(MetricMeanTest, ResetClearsSamplesAndAllowsReuse)
{
    MetricMean<i64, TypeParam> mean;
    mean.add(10);
    mean.reset();
    EXPECT_DOUBLE_EQ(mean.get(), 0.0);
    EXPECT_DOUBLE_EQ(mean.sum(), 0.0);
    EXPECT_EQ(mean.count(), 0u);
    mean.add(4);
    EXPECT_DOUBLE_EQ(mean.get(), 4.0);
    EXPECT_EQ(mean.count(), 1u);
}

TEST(AtomicMetricMeanTest, AccumulatesConcurrentUpdates)
{
    AtomicMetricMean<i64> mean;
    runConcurrently([&](i64 worker) { mean.add(worker); });
    EXPECT_EQ(mean.count(), sample_count);
    EXPECT_DOUBLE_EQ(mean.sum(), 6 * samples_per_worker);
    EXPECT_DOUBLE_EQ(mean.get(), 1.5);
}

}  // namespace
