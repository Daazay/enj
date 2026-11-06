#include "metric_test_declarations.hpp"

#include <realm/base/metrics/metrics/max.hpp>
#include <realm/base/metrics/metrics/mean.hpp>
#include <realm/base/metrics/metrics/min.hpp>

#include <type_traits>
#include <utility>
#include <variant>

#include <gtest/gtest.h>

namespace
{

using namespace realm;
using namespace realm::base::metrics;
using namespace realm::base::metrics::tests;

template<typename Storage>
concept HasRvalueIdLookup = requires(Storage&& storage) { std::move(storage).template get<MetricId::frames>(); };

template<typename Storage>
concept HasRvalueNameLookup = requires(Storage&& storage) { std::move(storage).template getByName<"draw_calls">(); };

template<typename Storage>
concept HasRvalueRuntimeIdLookup = requires(Storage&& storage) { std::move(storage).get(MetricId::frames); };

template<typename Storage>
concept HasRvalueRuntimeNameLookup = requires(Storage&& storage) { std::move(storage).getByName("draw_calls"); };

template<typename Storage>
concept HasRvalueFind =
    requires(Storage&& storage) { std::move(storage).template find<AtomicMetricCounter<>>(MetricId::frames); };

template<typename Storage>
concept HasRvalueFindByName =
    requires(Storage&& storage) { std::move(storage).template findByName<AtomicMetricCounter<>>("frames"); };

static_assert(!HasRvalueFind<EngineMetrics> && !HasRvalueFind<EngineMetrics const>);
static_assert(!HasRvalueFindByName<EngineMetrics> && !HasRvalueFindByName<EngineMetrics const>);
static_assert(std::is_same_v<decltype(std::declval<EngineMetrics&>().find<AtomicMetricCounter<>>(MetricId::frames)),
                             AtomicMetricCounter<>*>);
static_assert(
    std::is_same_v<decltype(std::declval<EngineMetrics const&>().find<AtomicMetricCounter<>>(MetricId::frames)),
                   AtomicMetricCounter<> const*>);
static_assert(std::is_same_v<decltype(std::declval<EngineMetrics&>().findByName<AtomicMetricCounter<>>("frames")),
                             AtomicMetricCounter<>*>);
static_assert(std::is_same_v<decltype(std::declval<EngineMetrics const&>().findByName<AtomicMetricCounter<>>("frames")),
                             AtomicMetricCounter<> const*>);

template<typename Metric>
constexpr bool ownsNonTransferableState = !std::is_copy_constructible_v<Metric> && !std::is_copy_assignable_v<Metric> &&
                                          !std::is_move_constructible_v<Metric> && !std::is_move_assignable_v<Metric>;

static_assert(ownsNonTransferableState<MetricCounter<>>);
static_assert(ownsNonTransferableState<MetricGauge<>>);
static_assert(ownsNonTransferableState<MetricMin<>>);
static_assert(ownsNonTransferableState<MetricMax<>>);
static_assert(ownsNonTransferableState<MetricMean<>>);
static_assert(ownsNonTransferableState<MetricHistogram<>>);
static_assert(ownsNonTransferableState<AtomicMetricCounter<>>);
static_assert(ownsNonTransferableState<AtomicMetricGauge<>>);
static_assert(ownsNonTransferableState<AtomicMetricMin<>>);
static_assert(ownsNonTransferableState<AtomicMetricMax<>>);
static_assert(ownsNonTransferableState<AtomicMetricMean<>>);
static_assert(ownsNonTransferableState<AtomicMetricHistogram<>>);

static_assert(EngineMetrics::metrics_count == 4);
static_assert(EngineMetrics::indexOf<MetricId::draw_calls>() == 1);
static_assert(EngineMetrics::indexOfName<"draw_calls">() == 1);
static_assert(EngineMetrics::name<MetricId::frames>() == "frames");
static_assert(!std::is_copy_constructible_v<EngineMetrics>);
static_assert(!std::is_move_constructible_v<EngineMetrics>);
static_assert(!HasRvalueIdLookup<EngineMetrics> && !HasRvalueIdLookup<EngineMetrics const>);
static_assert(!HasRvalueNameLookup<EngineMetrics> && !HasRvalueNameLookup<EngineMetrics const>);
static_assert(!HasRvalueRuntimeIdLookup<EngineMetrics> && !HasRvalueRuntimeIdLookup<EngineMetrics const>);
static_assert(!HasRvalueRuntimeNameLookup<EngineMetrics> && !HasRvalueRuntimeNameLookup<EngineMetrics const>);
static_assert(std::is_same_v<decltype(std::declval<EngineMetrics&>().get<MetricId::frames>()), AtomicMetricCounter<>&>);
static_assert(std::is_same_v<decltype(std::declval<EngineMetrics const&>().getByName<"draw_calls">()),
                             AtomicMetricCounter<> const&>);
static_assert(std::is_const_v<std::remove_reference_t<
                  decltype(std::declval<std::variant_alternative_t<0, EngineMetrics::any_metric_const_ref>>().value)>>);
static_assert(!traits::MetricNumber<bool>);
static_assert(!traits::MetricNumber<int const>);
static_assert(std::is_same_v<AtomicMetricCounter<>::policy, StoragePolicy::Atomic>);

constexpr bool supportsConstantEvaluation()
{
    MetricCounter<u64, 5> counter;

    counter.inc();
    counter.add(3);

    if (counter.get() != 9)
    {
        return false;
    }

    counter.reset();

    MetricHistogram<i64, 3> histogram{ { 10, 20 } };

    histogram.observe(10);
    histogram.observe(20);
    histogram.observe(21);

    if (histogram.total() != 3 || histogram.count(0) != 1 || histogram.count(1) != 1 || histogram.count(2) != 1)
    {
        return false;
    }

    histogram.reset();
    return counter.get() == 5 && histogram.total() == 0;
}

static_assert(supportsConstantEvaluation());

}  // namespace

TEST(MetricDeclarationTest, IncludesExplicitSparseAndLargeIdsInDeclarationOrder)
{
    EXPECT_EQ(EngineMetrics::metrics_count, 4u);
    EXPECT_EQ(EngineMetrics::idAt(0), MetricId::frames);
    EXPECT_EQ(EngineMetrics::idAt(1), MetricId::draw_calls);
    EXPECT_EQ(EngineMetrics::idAt(2), MetricId::frame_time);
    EXPECT_EQ(EngineMetrics::idAt(3), MetricId::active_entities);
}

TEST(MetricDeclarationTest, SupportsExplicitAndFallbackNamesBeyondDiscoveryRange)
{
    EXPECT_EQ(EngineMetrics::nameAt(0), "frames");
    EXPECT_EQ(EngineMetrics::nameAt(1), "draw_calls");
    EXPECT_EQ(EngineMetrics::nameAt(2), "frame_time");
    EXPECT_EQ(EngineMetrics::nameAt(3), "active_entities");
    EXPECT_EQ(EngineMetrics::name(MetricId::frame_time), "frame_time");
    EXPECT_EQ(EngineMetrics::idByName("draw_calls"), MetricId::draw_calls);
    EXPECT_EQ(EngineMetrics::indexOf(MetricId::draw_calls), 1u);
    EXPECT_EQ(EngineMetrics::indexOfName("draw_calls"), 1u);
}

TEST(MetricDeclarationTest, UnknownMetadataReturnsEmpty)
{
    EXPECT_FALSE(EngineMetrics::indexOf(unknown_metric_id));
    EXPECT_FALSE(EngineMetrics::name(unknown_metric_id));
    EXPECT_FALSE(EngineMetrics::indexOfName("missing"));
    EXPECT_FALSE(EngineMetrics::idByName("missing"));
    EXPECT_FALSE(EngineMetrics::idByName("drawCalls"));
    EXPECT_FALSE(EngineMetrics::idByName(""));
}

TEST(MetricDeclarationTest, InitializesDefaultValuesAndCustomFactories)
{
    EngineMetrics metrics;

    EXPECT_EQ(metrics.get<MetricId::frames>().get(), 0u);
    EXPECT_EQ(metrics.get<MetricId::active_entities>().get(), 5);

    auto const& histogram = metrics.get<MetricId::frame_time>();

    EXPECT_EQ(histogram.bound(0), 10);
    EXPECT_EQ(histogram.bound(1), 20);
    EXPECT_EQ(histogram.total(), 0u);
}
