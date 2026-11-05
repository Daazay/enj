#include "metric_test_support.hpp"

#include <realm/base/metrics/metrics/counter.hpp>

namespace
{

using namespace realm;
using namespace realm::base::metrics;
using namespace realm::base::metrics::tests;

template<typename Policy>
class MetricCounterTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(MetricCounterTest, StoragePolicies);

//

TYPED_TEST(MetricCounterTest, StartsAtConfiguredValue)
{
    MetricCounter<u64, 5, TypeParam> const counter;
    EXPECT_EQ(counter.get(), 5u);
}

TYPED_TEST(MetricCounterTest, IncrementsAndAdds)
{
    MetricCounter<u64, 0, TypeParam> counter;
    counter.inc();
    counter.add(4);
    EXPECT_EQ(counter.get(), 5u);
}

TYPED_TEST(MetricCounterTest, ResetsToTemplateValueAfterExplicitInitialization)
{
    MetricCounter<u64, 5, TypeParam> counter{ 10 };
    EXPECT_EQ(counter.get(), 10u);
    counter.inc();
    counter.reset();
    EXPECT_EQ(counter.get(), 5u);
    counter.inc();
    EXPECT_EQ(counter.get(), 6u);
}

TEST(AtomicMetricCounterTest, AccumulatesConcurrentUpdates)
{
    AtomicMetricCounter<> counter;
    runConcurrently([&](i64) { counter.inc(); });
    EXPECT_EQ(counter.get(), sample_count);
}

}  // namespace
