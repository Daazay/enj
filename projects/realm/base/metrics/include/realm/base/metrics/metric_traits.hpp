#pragma once

#include <realm/base/common/typedefs.hpp>

#include <concepts>
#include <type_traits>

namespace realm::base::metrics
{

template<auto MetricId>
struct MetricTraits
{
};

}  // namespace realm::base::metrics

namespace realm::base::metrics::traits
{

template<auto MetricId>
concept MetricTraits =
    std::is_enum_v<decltype(MetricId)> && requires { typename metrics::MetricTraits<MetricId>::metric_type; };

}  // namespace realm::base::metrics::traits
