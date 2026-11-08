#include "test_support.hpp"

#include <realm/base/metrics/metric_histogram.hpp>

#include <cmath>
#include <limits>

namespace
{

using namespace realm;
using namespace realm::base::metrics;

static_assert(
    []
    {
        MetricHistogram<i32, 4> metric{ { 0, 2, 5 } };
        metric.observe(2);
        return metric.bound(1) == 2 && metric.count(1) == 1 && metric.total() == 1;
    }());
static_assert(!std::is_default_constructible_v<MetricHistogram<>>);
static_assert(!std::is_copy_constructible_v<MetricHistogram<>>);
static_assert(!std::is_move_constructible_v<AtomicMetricHistogram<>>);

template<typename P>
class MetricHistogramTest : public testing::Test
{
};

TYPED_TEST_SUITE(MetricHistogramTest, tests::ScalarPolicies, tests::PolicyNames);

//

template<typename P>
class MetricHistogramFloatingTest : public testing::Test
{
};

TYPED_TEST_SUITE(MetricHistogramFloatingTest, tests::FloatingPolicies, tests::PolicyNames);

//

TYPED_TEST(MetricHistogramTest, InitializesBoundsAndZeroCounts)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricHistogram<T, 4, typename TypeParam::template storage_type<u64>>;

    Metric const metric{ { 0, 2, 5 } };
    EXPECT_EQ(metric.k_bucket_count, usize{ 4 });
    EXPECT_EQ(metric.bound(0), T{ 0 });
    EXPECT_EQ(metric.bound(1), T{ 2 });
    EXPECT_EQ(metric.bound(2), T{ 5 });
    EXPECT_EQ(metric.total(), u64{ 0 });
    constexpr std::array<u64, 4> empty{};
    EXPECT_EQ(metric.snapshot(), empty);
}

TYPED_TEST(MetricHistogramTest, UsesInclusiveNonUniformUpperBounds)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricHistogram<T, 4, typename TypeParam::template storage_type<u64>>;

    Metric metric{ { 0, 2, 5 } };
    for (T const sample : { T{ 0 }, T{ 1 }, T{ 2 }, T{ 3 }, T{ 5 }, T{ 6 } })
    {
        metric.observe(sample);
    }
    constexpr std::array<u64, 4> expected{ 1, 2, 2, 1 };
    EXPECT_EQ(metric.snapshot(), expected);
    EXPECT_EQ(metric.total(), u64{ 6 });
    for (usize i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(metric.count(i), expected[i]);
    }
}

TYPED_TEST(MetricHistogramTest, HandlesFullNumericRangeWithoutConvertingSampleType)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricHistogram<T, 4, typename TypeParam::template storage_type<u64>>;

    Metric metric{ { 0, 2, 5 } };
    metric.observe(std::numeric_limits<T>::lowest());
    metric.observe(std::numeric_limits<T>::max());
    constexpr std::array<u64, 4> expected{ 1, 0, 0, 1 };
    EXPECT_EQ(metric.snapshot(), expected);
}

TYPED_TEST(MetricHistogramTest, ResetClearsCountsAndPreservesBounds)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricHistogram<T, 4, typename TypeParam::template storage_type<u64>>;

    Metric metric{ { 0, 2, 5 } };
    metric.observe(T{ 3 });
    metric.reset();
    constexpr std::array<u64, 4> empty{};
    EXPECT_EQ(metric.snapshot(), empty);
    EXPECT_EQ(metric.total(), u64{ 0 });
    EXPECT_EQ(metric.bound(1), T{ 2 });
    metric.observe(T{ 2 });
    EXPECT_EQ(metric.count(1), u64{ 1 });
}

TYPED_TEST(MetricHistogramTest, SnapshotIsIndependentOfSubsequentObservations)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricHistogram<T, 4, typename TypeParam::template storage_type<u64>>;

    Metric metric{ { 0, 2, 5 } };
    metric.observe(T{ 1 });
    auto const snapshot = metric.snapshot();
    metric.observe(T{ 3 });
    constexpr std::array<u64, 4> expected{ 0, 1, 0, 0 };
    EXPECT_EQ(snapshot, expected);
    EXPECT_EQ(metric.total(), u64{ 2 });
}

//

TYPED_TEST(MetricHistogramFloatingTest, ClassifiesAdjacentRepresentableValuesAroundBoundary)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricHistogram<T, 4, typename TypeParam::template storage_type<u64>>;

    Metric metric{ { 0, 2, 5 } };
    metric.observe(std::nextafter(T{ 2 }, T{ 0 }));
    metric.observe(T{ 2 });
    metric.observe(std::nextafter(T{ 2 }, std::numeric_limits<T>::infinity()));
    constexpr std::array<u64, 4> expected{ 0, 2, 1, 0 };
    EXPECT_EQ(metric.snapshot(), expected);
}

TYPED_TEST(MetricHistogramFloatingTest, IgnoresNaNAndAcceptsInfiniteBoundsAndSamples)
{
    using T      = typename TypeParam::value_type;
    using Metric = MetricHistogram<T, 4, typename TypeParam::template storage_type<u64>>;

    auto const inf = std::numeric_limits<T>::infinity();
    Metric     metric{ { 0, 2, 5 } };
    metric.observe(std::numeric_limits<T>::quiet_NaN());
    EXPECT_EQ(metric.total(), u64{ 0 });
    metric.observe(-inf);
    metric.observe(inf);
    EXPECT_EQ(metric.count(0), u64{ 1 });
    EXPECT_EQ(metric.count(3), u64{ 1 });
    MetricHistogram<T, 3, typename TypeParam::template storage_type<u64>> infinite{ { -inf, inf } };
    infinite.observe(-inf);
    infinite.observe(T{ 0 });
    infinite.observe(inf);
    constexpr std::array<u64, 3> expected{ 1, 2, 0 };
    EXPECT_EQ(infinite.snapshot(), expected);
}

TEST(MetricHistogramTest, PreservesExactClassificationOfLargeUnsignedIntegers)
{
    constexpr auto          high    = u64{ 1 } << 63;
    constexpr auto          largest = std::numeric_limits<u64>::max();
    MetricHistogram<u64, 4> metric{ { 0, high, largest - 1 } };
    for (u64 const sample : { u64{ 0 }, u64{ 1 }, high, high + 1, largest - 1, largest })
    {
        metric.observe(sample);
    }
    constexpr std::array<u64, 4> expected{ 1, 2, 2, 1 };
    EXPECT_EQ(metric.snapshot(), expected);
}

// GoogleTest's death-test macros use goto internally.
// NOLINTBEGIN(cppcoreguidelines-avoid-goto)
TEST(MetricHistogramDeathTest, RejectsDuplicateAndDescendingBoundsInEveryBuild)
{
    using Plain  = MetricHistogram<i32, 4>;
    using Atomic = AtomicMetricHistogram<i32, 4>;

    EXPECT_DEATH((Plain{ { 1, 1, 2 } }), "strictly increasing");
    EXPECT_DEATH((Plain{ { 2, 1, 3 } }), "strictly increasing");
    EXPECT_DEATH((Atomic{ { 1, 1, 2 } }), "strictly increasing");
    EXPECT_DEATH((Atomic{ { 2, 1, 3 } }), "strictly increasing");
}

TEST(MetricHistogramDeathTest, RejectsNaNBoundsInEveryBuild)
{
    using Plain  = MetricHistogram<f64, 2>;
    using Atomic = AtomicMetricHistogram<f64, 2>;

    auto const nan = std::numeric_limits<f64>::quiet_NaN();
    EXPECT_DEATH((Plain{ { nan } }), "must not be NaN");
    EXPECT_DEATH((Atomic{ { nan } }), "must not be NaN");
}

TEST(MetricHistogramDeathTest, ChecksBoundAndBucketIndices)
{
    MetricHistogram<i32, 4> const metric{ { 0, 2, 5 } };
    EXPECT_DEATH(RE_IGNORE(metric.bound(3)), "bound index");
    EXPECT_DEATH(RE_IGNORE(metric.count(4)), "bucket index");
    AtomicMetricHistogram<i32, 4> const atomic{ { 0, 2, 5 } };
    EXPECT_DEATH(RE_IGNORE(atomic.bound(3)), "bound index");
    EXPECT_DEATH(RE_IGNORE(atomic.count(4)), "bucket index");
}

// NOLINTEND(cppcoreguidelines-avoid-goto)

TEST(AtomicMetricHistogramTest, ConcurrentWritersIncrementSharedBucketsWithoutLostSamples)
{
    AtomicMetricHistogram<u64, 4> metric{ { 0, 1, 2 } };
    tests::runWriters(
        [&](usize)
        {
            for (usize i = 0; i < tests::k_iterations; ++i)
            {
                metric.observe(i % 4);
            }
        });
    constexpr u64                per_bucket = tests::k_operations / 4;
    constexpr std::array<u64, 4> expected{ per_bucket, per_bucket, per_bucket, per_bucket };
    EXPECT_EQ(metric.snapshot(), expected);
    EXPECT_EQ(metric.total(), tests::k_operations);
}
}  // namespace
