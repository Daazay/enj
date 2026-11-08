#pragma once

#include <string_view>
#include <type_traits>

namespace realm::base::metrics
{

// String views refer to static descriptor metadata; the enum ID retains its type.
template<typename MetricId>
    requires std::is_enum_v<MetricId>
struct MetricInfo
{
    MetricId         id;
    std::string_view name;
    std::string_view description;
};

}  // namespace realm::base::metrics
