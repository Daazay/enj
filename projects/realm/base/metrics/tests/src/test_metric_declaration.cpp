#include "metric_test_declarations.hpp"

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
static_assert(std::is_same_v<AtomicMetricCounter<>::policy, StoragePolicy::Safe>);

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

TEST(MetricDeclarationTest, ReflectsSparseEnumValues)
{
    EXPECT_EQ(EngineMetrics::metrics_count, 4u);
    EXPECT_EQ(EngineMetrics::idAt(0), MetricId::frames);
    EXPECT_EQ(EngineMetrics::idAt(1), MetricId::draw_calls);
    EXPECT_EQ(EngineMetrics::idAt(2), MetricId::frame_time);
    EXPECT_EQ(EngineMetrics::idAt(3), MetricId::active_entities);
}

TEST(MetricDeclarationTest, SupportsReflectedAndCustomNames)
{
    EXPECT_EQ(EngineMetrics::nameAt(0), "frames");
    EXPECT_EQ(EngineMetrics::nameAt(1), "draw_calls");
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
