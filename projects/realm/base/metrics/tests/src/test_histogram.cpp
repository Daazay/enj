#include "metric_test_support.hpp"

#include <realm/base/metrics/metrics/histogram.hpp>

namespace
{

using namespace realm;
using namespace realm::base::metrics;
using namespace realm::base::metrics::tests;

template<typename Policy>
class MetricHistogramTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(MetricHistogramTest, StoragePolicies);

//

TYPED_TEST(MetricHistogramTest, StartsWithEmptyBucketsAndRetainsBounds)
{
    MetricHistogram<i64, 3, TypeParam> histogram{ { 10, 20 } };
    EXPECT_EQ(histogram.bound(0), 10);
    EXPECT_EQ(histogram.bound(1), 20);
    EXPECT_EQ(histogram.total(), 0u);
    for (usize bucket = 0; bucket < histogram.bucket_count; ++bucket)
    {
        EXPECT_EQ(histogram.count(bucket), 0u);
    }
}

TYPED_TEST(MetricHistogramTest, UsesInclusiveUpperBoundsAndOverflowBucket)
{
    MetricHistogram<i64, 3, TypeParam> histogram{ { 10, 20 } };
    for (auto sample : { -1, 10, 11, 20, 21 })
    {
        histogram.observe(sample);
    }
    EXPECT_EQ(histogram.count(0), 2u);
    EXPECT_EQ(histogram.count(1), 2u);
    EXPECT_EQ(histogram.count(2), 1u);
    EXPECT_EQ(histogram.total(), 5u);
}

TYPED_TEST(MetricHistogramTest, SupportsMinimumBucketCount)
{
    MetricHistogram<i64, 2, TypeParam> histogram{ { 0 } };
    histogram.observe(0);
    histogram.observe(1);
    EXPECT_EQ(histogram.count(0), 1u);
    EXPECT_EQ(histogram.count(1), 1u);
    EXPECT_EQ(histogram.total(), 2u);
}

TYPED_TEST(MetricHistogramTest, SupportsFloatingPointSamples)
{
    MetricHistogram<f64, 3, TypeParam> histogram{ { -0.5, 0.5 } };
    histogram.observe(-0.5);
    histogram.observe(0.25);
    histogram.observe(0.75);
    EXPECT_EQ(histogram.count(0), 1u);
    EXPECT_EQ(histogram.count(1), 1u);
    EXPECT_EQ(histogram.count(2), 1u);
}

TYPED_TEST(MetricHistogramTest, ResetClearsBucketsAndPreservesBounds)
{
    MetricHistogram<i64, 3, TypeParam> histogram{ { 10, 20 } };
    histogram.observe(1);
    histogram.observe(15);
    histogram.observe(30);
    histogram.reset();
    EXPECT_EQ(histogram.total(), 0u);
    for (usize bucket = 0; bucket < histogram.bucket_count; ++bucket)
    {
        EXPECT_EQ(histogram.count(bucket), 0u);
    }
    EXPECT_EQ(histogram.bound(0), 10);
    EXPECT_EQ(histogram.bound(1), 20);
    histogram.observe(15);
    EXPECT_EQ(histogram.count(1), 1u);
    EXPECT_EQ(histogram.total(), 1u);
}

TEST(AtomicMetricHistogramTest, AccumulatesConcurrentUpdates)
{
    AtomicMetricHistogram<i64, 3> histogram{ { 1, 2 } };
    runConcurrently([&](i64 worker) { histogram.observe(worker); });
    EXPECT_EQ(histogram.count(0), 2 * samples_per_worker);
    EXPECT_EQ(histogram.count(1), samples_per_worker);
    EXPECT_EQ(histogram.count(2), samples_per_worker);
    EXPECT_EQ(histogram.total(), sample_count);
}

}  // namespace
