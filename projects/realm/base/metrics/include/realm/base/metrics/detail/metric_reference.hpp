#pragma once

#include <realm/base/common/typedefs.hpp>

#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace realm::base::metrics::detail
{

template<usize Idx, typename Metric>
struct MetricRef
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    Metric& value;
};

template<typename Descriptors, bool IsConst, typename Indices>
struct MetricVariant;

// clang-format off
template<typename Descriptors, bool IsConst, usize... Is>
struct MetricVariant<Descriptors, IsConst, std::index_sequence<Is...>>
{
private:
    template<usize Idx>
    using element_t = std::conditional_t<IsConst,
        std::add_const_t<typename std::tuple_element_t<Idx, Descriptors>::metric_type>,
                         typename std::tuple_element_t<Idx, Descriptors>::metric_type>;

public:
    using type = std::variant<MetricRef<Is, element_t<Is>>...>;
};
// clang-format on

}  // namespace realm::base::metrics::detail
