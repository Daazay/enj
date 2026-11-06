#pragma once

#include <realm/base/metrics/detail/metric_descriptor.hpp>
#include <realm/base/metrics/detail/metric_reference.hpp>
#include <realm/base/metrics/detail/metric_schema.hpp>

#include <memory>
#include <tuple>
#include <utility>

namespace realm::base::metrics::detail
{

template<typename MetricId, typename... Descriptors>
class MetricStorageData : public MetricSchema<MetricId, Descriptors...>
{
private:
    using schema_type = MetricSchema<MetricId, Descriptors...>;

public:
    using schema_type::indexOf;
    using schema_type::indexOfName;
    using schema_type::metrics_count;

public:
    using metric_id  = MetricId;
    using store_type = std::tuple<Descriptors...>;

    using any_metric_ref = typename MetricVariant<store_type, false, std::make_index_sequence<metrics_count>>::type;
    using any_metric_const_ref =
        typename MetricVariant<store_type, true, std::make_index_sequence<metrics_count>>::type;

public:
    constexpr MetricStorageData() noexcept = default;

    MetricStorageData(MetricStorageData const&)            = delete;
    MetricStorageData(MetricStorageData&&)                 = delete;
    MetricStorageData& operator=(MetricStorageData const&) = delete;
    MetricStorageData& operator=(MetricStorageData&&)      = delete;

    constexpr ~MetricStorageData() noexcept = default;

public:
    template<MetricId Id>
    RE_NODISCARD RE_FORCE_INLINE constexpr decltype(auto) get() & noexcept
    {
        constexpr auto index = schema_type::template indexOf<Id>();
        static_assert(index < metrics_count, "unknown metric id");
        return (std::get<index>(_store).metric);
    }

    template<MetricId Id>
    RE_NODISCARD RE_FORCE_INLINE constexpr decltype(auto) get() const& noexcept
    {
        constexpr auto index = schema_type::template indexOf<Id>();
        static_assert(index < metrics_count, "unknown metric id");
        return (std::get<index>(_store).metric);
    }

    template<MetricId Id>
    void get() && = delete;

    template<MetricId Id>
    void get() const&& = delete;

public:
    template<StringLiteral Name>
    RE_NODISCARD RE_FORCE_INLINE constexpr decltype(auto) getByName() & noexcept
    {
        constexpr auto index = schema_type::template indexOfName<Name>();
        static_assert(index < metrics_count, "unknown metric name");
        return (std::get<index>(_store).metric);
    }

    template<StringLiteral Name>
    RE_NODISCARD RE_FORCE_INLINE constexpr decltype(auto) getByName() const& noexcept
    {
        constexpr auto index = schema_type::template indexOfName<Name>();
        static_assert(index < metrics_count, "unknown metric name");
        return (std::get<index>(_store).metric);
    }

    template<StringLiteral Name>
    void getByName() && = delete;

    template<StringLiteral Name>
    void getByName() const&& = delete;

public:
    // Runtime lookups return a borrowed pointer, or nullptr for an unknown ID/name or type mismatch.
    // The pointer remains valid only while this storage exists.
    template<typename Metric>
    RE_NODISCARD constexpr Metric* find(MetricId id) & noexcept
    {
        return pointerAt<Metric>(*this, indexOf(id).value_or(metrics_count));
    }

    template<typename Metric>
    RE_NODISCARD constexpr Metric const* find(MetricId id) const& noexcept
    {
        return pointerAt<Metric>(*this, indexOf(id).value_or(metrics_count));
    }

    template<typename Metric>
    Metric* find(MetricId) && = delete;

    template<typename Metric>
    Metric const* find(MetricId) const&& = delete;

    template<typename Metric>
    RE_NODISCARD constexpr Metric* findByName(std::string_view name) & noexcept
    {
        return pointerAt<Metric>(*this, indexOfName(name).value_or(metrics_count));
    }

    template<typename Metric>
    RE_NODISCARD constexpr Metric const* findByName(std::string_view name) const& noexcept
    {
        return pointerAt<Metric>(*this, indexOfName(name).value_or(metrics_count));
    }

    template<typename Metric>
    Metric* findByName(std::string_view) && = delete;

    template<typename Metric>
    Metric const* findByName(std::string_view) const&& = delete;

public:
    RE_NODISCARD constexpr Opt<any_metric_ref> get(MetricId id) & noexcept
    {
        return referenceAt<any_metric_ref>(*this, indexOf(id).value_or(metrics_count));
    }

    RE_NODISCARD constexpr Opt<any_metric_const_ref> get(MetricId id) const& noexcept
    {
        return referenceAt<any_metric_const_ref>(*this, indexOf(id).value_or(metrics_count));
    }

    RE_NODISCARD Opt<any_metric_ref> get(MetricId) &&            = delete;
    RE_NODISCARD Opt<any_metric_const_ref> get(MetricId) const&& = delete;

public:
    RE_NODISCARD constexpr Opt<any_metric_ref> getByName(std::string_view name) & noexcept
    {
        return referenceAt<any_metric_ref>(*this, indexOfName(name).value_or(metrics_count));
    }

    RE_NODISCARD constexpr Opt<any_metric_const_ref> getByName(std::string_view name) const& noexcept
    {
        return referenceAt<any_metric_const_ref>(*this, indexOfName(name).value_or(metrics_count));
    }

    RE_NODISCARD Opt<any_metric_ref> getByName(std::string_view) &&            = delete;
    RE_NODISCARD Opt<any_metric_const_ref> getByName(std::string_view) const&& = delete;

private:
    template<typename Metric, usize Index = 0, typename Self>
    RE_NODISCARD static constexpr auto pointerAt(Self& self, usize index) noexcept
        -> std::conditional_t<std::is_const_v<Self>, Metric const*, Metric*>
    {
        static_assert(std::is_same_v<Metric, std::remove_cvref_t<Metric>>, "find requires an unqualified metric type");
        if constexpr (Index < metrics_count)
        {
            if (Index == index)
            {
                if constexpr (std::is_same_v<Metric, typename std::tuple_element_t<Index, store_type>::metric_type>)
                {
                    return std::addressof(std::get<Index>(self._store).metric);
                }
                return nullptr;
            }
            return pointerAt<Metric, Index + 1>(self, index);
        }
        return nullptr;
    }

    template<typename Reference, usize Index = 0, typename Self>
    RE_NODISCARD static constexpr Opt<Reference> referenceAt(Self& self, usize index) noexcept
    {
        if constexpr (Index < metrics_count)
        {
            if (Index == index)
            {
                return Opt<Reference>{ std::in_place, std::in_place_index<Index>, std::get<Index>(self._store).metric };
            }
            return referenceAt<Reference, Index + 1>(self, index);
        }
        return std::nullopt;
    }

private:
    store_type _store{};
};

}  // namespace realm::base::metrics::detail

namespace realm::base::metrics
{

// The explicit list defines membership and iteration order. All IDs have the same enum type.
template<auto FirstId, decltype(FirstId)... Ids>
class MetricStorage : public detail::MetricStorageData<decltype(FirstId), detail::MetricDescriptor<FirstId>,
                                                       detail::MetricDescriptor<Ids>...>
{
private:
    static_assert(std::is_enum_v<decltype(FirstId)>, "MetricStorage requires enum IDs");
};

}  // namespace realm::base::metrics
