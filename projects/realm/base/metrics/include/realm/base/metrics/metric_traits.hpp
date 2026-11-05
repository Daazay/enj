#pragma once

namespace realm::base::metrics
{

// Specializations provide metric_type, an optional name, and an optional consteval, noexcept make().
template<auto Id>
struct MetricTraits;

}  // namespace realm::base::metrics
