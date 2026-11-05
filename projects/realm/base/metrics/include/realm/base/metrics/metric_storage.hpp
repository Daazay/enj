#pragma once

#include <realm/base/metrics/detail/metric_descriptor.hpp>
#include <realm/base/metrics/detail/metric_reference.hpp>
#include <realm/base/metrics/detail/metric_schema.hpp>

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

template<typename MetricId, typename Seq>
struct ReflectedMetricStorage;

template<typename MetricId, usize... Is>
struct ReflectedMetricStorage<MetricId, std::index_sequence<Is...>>
{
public:
    using type = MetricStorageData<MetricId, MetricDescriptor<magic_enum::enum_value<MetricId, Is>()>...>;
};

}  // namespace realm::base::metrics::detail

namespace realm::base::metrics
{

template<typename MetricId>
class MetricStorage
    : public detail::ReflectedMetricStorage<MetricId,
                                            std::make_index_sequence<magic_enum::enum_count<MetricId>()>>::type
{
private:
    static_assert(std::is_enum_v<MetricId>, "MetricStorage requires an enum ID type");
};

}  // namespace realm::base::metrics
