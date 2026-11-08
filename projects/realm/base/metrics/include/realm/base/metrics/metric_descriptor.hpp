#pragma once

#include <realm/base/common/typedefs.hpp>
#include <realm/base/metrics/metric_info.hpp>
#include <realm/base/metrics/metric_traits.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>

namespace realm::base::metrics
{

template<auto MetricId>
    requires traits::MetricTraits<MetricId>
struct MetricDescriptor
{
public:
    static_assert(std::is_enum_v<decltype(MetricId)>);

public:
    using metric_traits = metrics::MetricTraits<MetricId>;
    using metric_type   = typename metric_traits::metric_type;
    using metric_id     = decltype(MetricId);

    static constexpr metric_id        id   = MetricId;
    static constexpr std::string_view name = []() consteval
    {
        if constexpr (requires {
                          { metric_traits::name } -> std::convertible_to<std::string_view>;
                      })
        {
            return std::string_view{ metric_traits::name };
        }
        else
        {
            return magic_enum::enum_name<id>();
        }
    }();

    static constexpr std::string_view description = []() consteval
    {
        if constexpr (requires {
                          { metric_traits::description } -> std::convertible_to<std::string_view>;
                      })
        {
            return std::string_view{ metric_traits::description };
        }
        else
        {
            return std::string_view{};
        }
    }();

    static constexpr MetricInfo<metric_id> info{ id, name, description };

public:
    static constexpr metric_type make() noexcept
    {
        if constexpr (requires {
                          { metric_traits::make() } noexcept -> std::same_as<metric_type>;
                      })
        {
            return metric_traits::make();
        }
        else
        {
            return metric_type{};
        }
    }
};

}  // namespace realm::base::metrics
