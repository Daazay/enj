#pragma once

#include <realm/base/common/defines.hpp>
#include <realm/base/common/result.hpp>
#include <realm/base/common/string_literal.hpp>

#include <algorithm>
#include <array>
#include <string_view>
#include <type_traits>

namespace realm::base::metrics::detail
{

template<typename MetricId, typename... Descriptors>
class MetricSchema
{
private:
    template<typename T, usize N>
    RE_NODISCARD static constexpr usize findIndex(std::array<T, N> const& values, T const& value)
    {
        for (usize i = 0; i < N; ++i)
        {
            if (values[i] == value)
            {
                return i;
            }
        }
        return N;
    }

    template<typename T, usize N>
    RE_NODISCARD static constexpr bool hasEmpty(std::array<T, N> const& values)
    {
        return std::ranges::any_of(values, [](auto const& value) { return value.empty(); });
    }

    template<typename T, usize N>
    RE_NODISCARD static constexpr bool hasDuplicates(std::array<T, N> const& values)
    {
        for (usize i = 0; i < N; ++i)
        {
            for (usize j = i + 1; j < N; ++j)
            {
                if (values[i] == values[j])
                {
                    return true;
                }
            }
        }
        return false;
    }

    static_assert(std::is_enum_v<MetricId>);
    static_assert((std::is_same_v<std::remove_cv_t<decltype(Descriptors::id)>, MetricId> && ...),
                  "all descriptor ids must match MetricId");

private:
    static constexpr auto ids   = std::array<MetricId, sizeof...(Descriptors)>{ Descriptors::id... };
    static constexpr auto names = std::array<std::string_view, sizeof...(Descriptors)>{ Descriptors::name... };

    static_assert(sizeof...(Descriptors) > 0, "metric storage requires at least one reflected ID");
    static_assert(!hasDuplicates(ids), "metric IDs must be unique");
    static_assert(!hasEmpty(names), "metric names must be non-empty");
    static_assert(!hasDuplicates(names), "metric names must be unique");

public:
    using metric_id                      = MetricId;
    static constexpr usize metrics_count = sizeof...(Descriptors);

    template<MetricId Id>
    RE_NODISCARD static constexpr usize indexOf() noexcept
    {
        return findIndex(ids, Id);
    }

    template<MetricId Id>
    RE_NODISCARD static constexpr std::string_view name() noexcept
    {
        constexpr auto index = indexOf<Id>();
        static_assert(index < metrics_count, "unknown metric id");
        return names[index];
    }

    template<StringLiteral Name>
    RE_NODISCARD static constexpr usize indexOfName() noexcept
    {
        return findIndex(names, Name.view());
    }

    RE_NODISCARD static constexpr MetricId         idAt(usize i) noexcept { return ids[i]; }
    RE_NODISCARD static constexpr std::string_view nameAt(usize i) noexcept { return names[i]; }

public:
    RE_NODISCARD static constexpr Opt<usize> indexOf(MetricId id) noexcept
    {
        auto const i = findIndex(ids, id);
        if (i < metrics_count)
        {
            return i;
        }
        return std::nullopt;
    }

    RE_NODISCARD static constexpr Opt<usize> indexOfName(std::string_view name) noexcept
    {
        auto const index = findIndex(names, name);
        if (index < metrics_count)
        {
            return index;
        }
        return std::nullopt;
    }

    RE_NODISCARD static constexpr Opt<MetricId> idByName(std::string_view name) noexcept
    {
        if (auto index = indexOfName(name))
        {
            return ids[*index];
        }
        return std::nullopt;
    }

    RE_NODISCARD static constexpr Opt<std::string_view> name(MetricId id) noexcept
    {
        if (auto i = indexOf(id))
        {
            return names[*i];
        }
        return std::nullopt;
    }
};

}  // namespace realm::base::metrics::detail
