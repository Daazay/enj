#pragma once

#include <realm/base/common/typedefs.hpp>
#include <realm/base/metrics/metric_traits.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

#include <magic_enum.hpp>

namespace realm::base::metrics::detail
{

template<auto Id>
struct MetricDescriptor
{
private:
    static_assert(std::is_enum_v<decltype(Id)>, "metric IDs must be enum values");
    static_assert(requires { typename MetricTraits<Id>::metric_type; }, "MetricTraits specialization not found");

public:
    using metric_traits = MetricTraits<Id>;
    using metric_type   = typename metric_traits::metric_type;
    using metric_id     = decltype(Id);

    static constexpr metric_id        id   = Id;
    static constexpr std::string_view name = []
    {
        if constexpr (requires {
                          { metric_traits::name } -> std::convertible_to<std::string_view>;
                      })
        {
            return std::string_view{ metric_traits::name };
        }
        else
        {
            return magic_enum::enum_name(Id);
        }
    }();

private:
    static consteval metric_type makeMetric()
    {
        if constexpr (requires {
                          { metric_traits::make() } noexcept -> std::same_as<metric_type>;
                      })
        {
            return metric_traits::make();
        }
        else if constexpr (std::is_default_constructible_v<metric_type>)
        {
            return metric_type{};
        }
        else
        {
            static_assert(always_false<metric_type>,
                          "MetricTraits must provide make() or metric_type must be default-constructible");
        }
    }

public:
    metric_type metric = makeMetric();
};

}  // namespace realm::base::metrics::detail
