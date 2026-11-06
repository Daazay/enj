#pragma once

namespace realm::base::metrics
{

// Specializations provide metric_type, an optional constexpr name convertible to std::string_view,
// and an optional consteval, noexcept make(). Names default to the enum entry's name and must be
// non-empty and unique within each storage.
template<auto Id>
struct MetricTraits;

}  // namespace realm::base::metrics
