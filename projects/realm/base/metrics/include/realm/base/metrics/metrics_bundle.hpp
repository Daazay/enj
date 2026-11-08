#pragma once

#include <realm/base/common/defines.hpp>
#include <realm/base/metrics/metric_descriptor.hpp>

#include <array>
#include <concepts>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace realm::base::metrics::detail
{

template<auto Id, auto... Ids>
consteval usize findId() noexcept
{
    constexpr usize count = sizeof...(Ids);

    std::array<decltype(Id), count> values{ Ids... };

    for (usize i = 0; i < values.size(); ++i)
    {
        if (values[i] == Id)
        {
            return i;
        }
    }

    return count;
}

template<typename MetricId, MetricId... Ids>
consteval bool uniqueIds() noexcept
{
    std::array<MetricId, sizeof...(Ids)> values{ Ids... };
    for (usize i = 0; i < values.size(); ++i)
    {
        for (usize j = i + 1; j < values.size(); ++j)
        {
            if (values[i] == values[j])
            {
                return false;
            }
        }
    }
    return true;
}

// tuple forwards these proxies into its elements.
// Conversion produces a prvalue directly in the destination, so even atomic/non-movable metrics need no move.
template<auto Id>
struct MetricFactory
{
    constexpr operator typename MetricDescriptor<Id>::metric_type() const noexcept
    {
        return MetricDescriptor<Id>::make();
    }
};

}  // namespace realm::base::metrics::detail

namespace realm::base::metrics::traits
{

// Exhaustive: every metric type in the bundle must be handled.
// Use for export/serialization where silently missing a metric is a bug.
template<typename Fn, typename MetricId, typename... MetricTypes>
concept MetricsBundleVisitor = (std::invocable<Fn&, MetricTypes const&, MetricInfo<MetricId> const&> && ...);

// Partial: at least one metric type must be handled; the rest are silently skipped.
// Adding a metric type to the bundle later will NOT break existing visitors.
template<typename Fn, typename MetricId, typename... MetricTypes>
concept PartialMetricsBundleVisitor = (std::invocable<Fn&, MetricTypes const&, MetricInfo<MetricId> const&> || ...);

}  // namespace realm::base::metrics::traits

namespace realm::base::metrics
{

template<typename MetricId, MetricId... Ids>
    requires std::is_enum_v<MetricId> && (traits::MetricTraits<Ids> && ...)
class MetricsBundle
{
public:
    static constexpr usize metrics_count = sizeof...(Ids);

    static_assert(metrics_count > 0, "A metrics bundle must contain at least one metric");
    static_assert(detail::uniqueIds<MetricId, Ids...>(), "A metrics bundle must not contain duplicate IDs");

    using metric_id    = MetricId;
    using storage_type = std::tuple<typename MetricDescriptor<Ids>::metric_type...>;

public:
    constexpr MetricsBundle() noexcept
        : _metrics{ detail::MetricFactory<Ids>{}... }
    {
    }

    MetricsBundle(MetricsBundle const&)            = delete;
    MetricsBundle& operator=(MetricsBundle const&) = delete;
    MetricsBundle(MetricsBundle&&)                 = delete;
    MetricsBundle& operator=(MetricsBundle&&)      = delete;

    constexpr ~MetricsBundle() = default;

public:
    template<MetricId Id>
    RE_NODISCARD RE_FORCE_INLINE constexpr auto& get() noexcept
    {
        constexpr auto idx = detail::findId<Id, Ids...>();
        static_assert(idx < metrics_count, "no metric registered for this enum value.");
        return std::get<idx>(_metrics);
    }

    template<MetricId Id>
    RE_NODISCARD RE_FORCE_INLINE constexpr auto const& get() const noexcept
    {
        constexpr auto idx = detail::findId<Id, Ids...>();
        static_assert(idx < metrics_count, "no metric registered for this enum value.");
        return std::get<idx>(_metrics);
    }

    template<typename Visitor>
        requires traits::MetricsBundleVisitor<Visitor, MetricId, typename MetricDescriptor<Ids>::metric_type...>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    RE_FORCE_INLINE constexpr void visit(Visitor&& visitor) const
    {
        visitImpl(visitor, std::make_index_sequence<metrics_count>{});
    }

    template<typename Visitor>
        requires traits::PartialMetricsBundleVisitor<Visitor, MetricId, typename MetricDescriptor<Ids>::metric_type...>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    RE_FORCE_INLINE constexpr void visitIfInvocable(Visitor&& visitor) const
    {
        visitIfImpl(visitor, std::make_index_sequence<metrics_count>{});
    }

    RE_FORCE_INLINE constexpr void resetAll() noexcept { (resetOne<Ids>(), ...); }

private:
    template<typename Visitor, usize... Is>
    RE_FORCE_INLINE constexpr void visitImpl(Visitor& visitor, RE_MAYBE_UNUSED std::index_sequence<Is...> ids) const
    {
        (static_cast<void>(std::invoke(visitor, std::get<Is>(_metrics), MetricDescriptor<Ids>::info)), ...);
    }

    template<typename Visitor, usize... Is>
    RE_FORCE_INLINE constexpr void visitIfImpl(Visitor& visitor, RE_MAYBE_UNUSED std::index_sequence<Is...> ids) const
    {
        (visitOneIf<Is>(visitor), ...);
    }

    template<usize I, typename Visitor>
    RE_FORCE_INLINE constexpr void visitOneIf(Visitor& visitor) const
    {
        static constexpr MetricId ids[] = { Ids... };

        using MT = typename MetricDescriptor<ids[I]>::metric_type;

        if constexpr (std::invocable<Visitor&, MT const&, MetricInfo<MetricId> const&>)
        {
            std::invoke(visitor, std::get<I>(_metrics), MetricDescriptor<ids[I]>::info);
        }
    }

    template<MetricId Id>
    RE_FORCE_INLINE constexpr void resetOne() noexcept
    {
        if constexpr (requires { get<Id>().reset(); })
        {
            get<Id>().reset();
        }
    }

private:
    storage_type _metrics;
};

}  // namespace realm::base::metrics
