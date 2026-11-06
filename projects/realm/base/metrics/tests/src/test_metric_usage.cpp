#include "metric_test_declarations.hpp"

#include <memory>

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

    auto* frames     = metrics.find<AtomicMetricCounter<>>(MetricId::frames);
    auto* draw_calls = metrics.findByName<AtomicMetricCounter<>>("draw_calls");

    if (frames == nullptr || draw_calls == nullptr)
    {
        FAIL() << "Expected both counter metrics";
    }
    EXPECT_EQ(frames, std::addressof(metrics.get<MetricId::frames>()));
    EXPECT_EQ(draw_calls, std::addressof(metrics.get<MetricId::draw_calls>()));

    frames->inc();
    draw_calls->add(3);

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

    auto by_id   = view.find<AtomicMetricCounter<>>(MetricId::draw_calls);
    auto by_name = view.findByName<AtomicMetricCounter<>>("draw_calls");

    if (!by_id || !by_name)
    {
        FAIL() << "Expected lookup by ID and name to succeed";
    }
    EXPECT_EQ(by_id, by_name);
    EXPECT_EQ(by_id->get(), 4u);
    EXPECT_EQ(by_name->get(), 4u);
}

TEST(MetricUsageTest, UnknownRuntimeLookupsReturnEmpty)
{
    EngineMetrics metrics;

    auto const& view = metrics;

    EXPECT_FALSE(metrics.find<AtomicMetricCounter<>>(unknown_metric_id));
    EXPECT_FALSE(metrics.findByName<AtomicMetricCounter<>>("missing"));
    EXPECT_FALSE(metrics.findByName<AtomicMetricCounter<>>(""));
    EXPECT_FALSE(view.find<AtomicMetricCounter<>>(unknown_metric_id));
    EXPECT_FALSE(view.findByName<AtomicMetricCounter<>>("missing"));
    EXPECT_FALSE(view.findByName<AtomicMetricCounter<>>(""));
}

TEST(MetricUsageTest, ObservesAndResetsConfiguredMetric)
{
    EngineMetrics metrics;

    auto histogram = metrics.findByName<AtomicMetricHistogram<i64, 3>>("frame_time");

    if (!histogram)
    {
        FAIL() << "Expected the frame-time histogram";
    }
    histogram->observe(15);
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

TEST(MetricUsageTest, RuntimeTypeMismatchesReturnNullWithoutChangingMetrics)
{
    EngineMetrics metrics;
    auto const&   view = metrics;

    EXPECT_EQ(metrics.find<AtomicMetricCounter<>>(MetricId::active_entities), nullptr);
    EXPECT_EQ(metrics.findByName<AtomicMetricCounter<>>("active_entities"), nullptr);
    EXPECT_EQ(view.find<AtomicMetricCounter<>>(MetricId::active_entities), nullptr);
    EXPECT_EQ(view.findByName<AtomicMetricCounter<>>("active_entities"), nullptr);

    auto* gauge = metrics.find<MetricGauge<i64, 5>>(MetricId::active_entities);
    ASSERT_NE(gauge, nullptr);
    gauge->set(12);
    EXPECT_EQ(metrics.get<MetricId::active_entities>().get(), 12);
    EXPECT_EQ(metrics.get<MetricId::frames>().get(), 0u);
}

TEST(MetricUsageTest, OverlappingSubsetsHaveIndependentValuesAndExplicitOrder)
{
    using ClientMetrics = MetricStorage<MetricId::frames, MetricId::draw_calls>;
    using ServerMetrics = MetricStorage<MetricId::active_entities, MetricId::frames>;

    ClientMetrics client;
    ServerMetrics server;

    client.get<MetricId::frames>().add(2);
    server.get<MetricId::frames>().add(7);
    client.getByName<"draw_calls">().inc();
    server.get<MetricId::active_entities>().set(100);

    EXPECT_EQ(client.get<MetricId::frames>().get(), 2u);
    EXPECT_EQ(server.get<MetricId::frames>().get(), 7u);
    EXPECT_EQ(client.get<MetricId::draw_calls>().get(), 1u);
    EXPECT_EQ(server.get<MetricId::active_entities>().get(), 100);
    EXPECT_EQ(ClientMetrics::metrics_count, 2u);
    EXPECT_EQ(ServerMetrics::metrics_count, 2u);
    EXPECT_EQ(ClientMetrics::idAt(0), MetricId::frames);
    EXPECT_EQ(ServerMetrics::idAt(0), MetricId::active_entities);
    EXPECT_EQ(ServerMetrics::nameAt(1), "frames");

    auto const& view = server;
    EXPECT_EQ(server.find<AtomicMetricCounter<>>(MetricId::draw_calls), nullptr);
    EXPECT_EQ(view.findByName<AtomicMetricCounter<>>("draw_calls"), nullptr);
    EXPECT_FALSE(server.get(MetricId::draw_calls));
    EXPECT_FALSE(view.getByName("draw_calls"));

    // A requested type can also be absent from the entire subset.
    using Histogram = AtomicMetricHistogram<i64, 3>;
    EXPECT_EQ(server.find<Histogram>(MetricId::frame_time), nullptr);
    EXPECT_EQ(view.findByName<Histogram>("frame_time"), nullptr);
    EXPECT_FALSE(ServerMetrics::indexOf(MetricId::draw_calls));
    EXPECT_FALSE(ClientMetrics::idByName("active_entities"));
}
