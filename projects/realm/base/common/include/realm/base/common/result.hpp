#pragma once

#include <expected>
#include <optional>

namespace realm
{

template<typename T>
using Opt = std::optional<T>;

template<typename T, typename E>
using Result = std::expected<T, E>;

}  // namespace realm
