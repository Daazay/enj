#include "metric_test_declarations.hpp"

#include <memory>
#include <variant>

#include <gtest/gtest.h>

namespace
{

using namespace realm;
using namespace realm::base::metrics;
using namespace realm::base::metrics::tests;

}  // namespace

TEST(MetricUsageTest, CompileTimeLookupsReferToSameMetric)
{
    EngineMetrics metrics;

    auto& counter = metrics.get<MetricId::draw_calls>();
    counter.add(3);

    auto& named_counter = metrics.getByName<"draw_calls">();

    EXPECT_EQ(std::addressof(counter), std::addressof(named_counter));
    named_counter.inc();
    EXPECT_EQ(counter.get(), 4u);
}

TEST(MetricUsageTest, RuntimeLookupsKeepRepeatedMetricTypesDistinct)
{
    EngineMetrics metrics;

    auto frames     = metrics.get(MetricId::frames);
    auto draw_calls = metrics.getByName("draw_calls");

    if (!frames || !draw_calls)
    {
        FAIL() << "Expected both counter metrics";
    }
    ASSERT_EQ(frames->index(), 0u);
    ASSERT_EQ(draw_calls->index(), 1u);

    std::get<0>(*frames).value.inc();
    std::get<1>(*draw_calls).value.add(3);

    EXPECT_EQ(metrics.get<MetricId::frames>().get(), 1u);
    EXPECT_EQ(metrics.get<MetricId::draw_calls>().get(), 3u);
}

TEST(MetricUsageTest, ConstLookupsExposeCurrentValues)
{
    EngineMetrics metrics;

    auto const& view = metrics;

    metrics.get<MetricId::draw_calls>().add(4);

    EXPECT_EQ(view.get<MetricId::draw_calls>().get(), 4u);
    EXPECT_EQ(view.getByName<"draw_calls">().get(), 4u);

    auto by_id   = view.get(MetricId::draw_calls);
    auto by_name = view.getByName("draw_calls");

    if (!by_id || !by_name)
    {
        FAIL() << "Expected lookup by ID and name to succeed";
    }
    ASSERT_EQ(by_id->index(), 1u);
    ASSERT_EQ(by_name->index(), 1u);
    EXPECT_EQ(std::get<1>(*by_id).value.get(), 4u);
    EXPECT_EQ(std::get<1>(*by_name).value.get(), 4u);
}

TEST(MetricUsageTest, UnknownRuntimeLookupsReturnEmpty)
{
    EngineMetrics metrics;

    auto const& view = metrics;

    EXPECT_FALSE(metrics.get(unknown_metric_id));
    EXPECT_FALSE(metrics.getByName("missing"));
    EXPECT_FALSE(metrics.getByName(""));
    EXPECT_FALSE(view.get(unknown_metric_id));
    EXPECT_FALSE(view.getByName("missing"));
    EXPECT_FALSE(view.getByName(""));
}

TEST(MetricUsageTest, ObservesAndResetsConfiguredMetric)
{
    EngineMetrics metrics;

    auto histogram = metrics.getByName("frame_time");

    if (!histogram)
    {
        FAIL() << "Expected the frame-time histogram";
    }
    ASSERT_EQ(histogram->index(), 2u);

    std::get<2>(*histogram).value.observe(15);
    auto& stored = metrics.get<MetricId::frame_time>();

    EXPECT_EQ(stored.count(1), 1u);
    EXPECT_EQ(stored.total(), 1u);

    stored.reset();

    EXPECT_EQ(stored.total(), 0u);
    EXPECT_EQ(stored.bound(0), 10);
}

TEST(MetricUsageTest, StorageInstancesHaveIndependentValues)
{
    EngineMetrics first;
    EngineMetrics second;

    first.get<MetricId::frames>().inc();
    first.get<MetricId::active_entities>().set(10);

    EXPECT_EQ(second.get<MetricId::frames>().get(), 0u);
    EXPECT_EQ(second.get<MetricId::active_entities>().get(), 5);
}
