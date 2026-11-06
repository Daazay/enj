#pragma once

namespace realm::base::cvars
{

// Specialize for each enum ID, following the MetricTraits pattern.
//
// Required members:
//   using value_type = ...;                     An owning, non-cv value type.
//   static constexpr value_type default_value;  The compiled default.
//
// Optional members:
//   static constexpr std::string_view name;         Defaults to the enum name.
//   static constexpr std::string_view description;  Defaults to an empty description.
//
// Traits describe a variable, not an instance. Independent storages share these declarations,
// but own separate values. Supported value types and synchronization are defined by storage.
template<auto Id>
struct CVarTraits;

}  // namespace realm::base::cvars
