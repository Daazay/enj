#include "metric_test_support.hpp"

#include <realm/base/metrics/metrics/gauge.hpp>

namespace
{

using namespace realm;
using namespace realm::base::metrics;
using namespace realm::base::metrics::tests;

template<typename Policy>
class MetricGaugeTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(MetricGaugeTest, StoragePolicies);

//

TYPED_TEST(MetricGaugeTest, StartsAtConfiguredValue)
{
    MetricGauge<i64, -2, TypeParam> const gauge;
    EXPECT_EQ(gauge.get(), -2);
}

TYPED_TEST(MetricGaugeTest, SetsAndAdjustsSignedValues)
{
    MetricGauge<i64, 0, TypeParam> gauge;
    gauge.set(-3);
    gauge.inc();
    EXPECT_EQ(gauge.get(), -2);
    gauge.dec();
    EXPECT_EQ(gauge.get(), -3);
    gauge.add(7);
    EXPECT_EQ(gauge.get(), 4);
}

TYPED_TEST(MetricGaugeTest, ResetsToTemplateValueAfterExplicitInitialization)
{
    MetricGauge<i64, 2, TypeParam> gauge{ -10 };
    EXPECT_EQ(gauge.get(), -10);
    gauge.reset();
    EXPECT_EQ(gauge.get(), 2);
}

TEST(AtomicMetricGaugeTest, AccumulatesConcurrentUpdates)
{
    AtomicMetricGauge<> gauge;
    runConcurrently([&](i64 worker) { gauge.add(worker - 2); });
    EXPECT_EQ(gauge.get(), -2 * static_cast<i64>(samples_per_worker));
}

}  // namespace
