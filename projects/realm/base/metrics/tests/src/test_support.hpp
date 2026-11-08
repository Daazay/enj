#pragma once

#include <realm/base/metrics/metric_value_storage.hpp>

#include <array>
#include <barrier>
#include <numeric>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace realm::base::metrics::tests
{

template<typename T, bool Atomic>
struct Policy
{
    using value_type               = T;
    static constexpr bool k_atomic = Atomic;

    template<typename U>
    using storage_type =
        std::conditional_t<Atomic, detail::MetricValueStorageAtomic<U>, detail::MetricValueStoragePlain<U>>;
};

using ScalarPolicies   = testing::Types<Policy<i32, false>, Policy<i32, true>, Policy<u32, false>, Policy<u32, true>,
                                        Policy<i64, false>, Policy<i64, true>, Policy<u64, false>, Policy<u64, true>,
                                        Policy<f32, false>, Policy<f32, true>, Policy<f64, false>, Policy<f64, true>>;
using FloatingPolicies = testing::Types<Policy<f32, false>, Policy<f32, true>, Policy<f64, false>, Policy<f64, true>>;

struct PolicyNames
{
    template<typename P>
    static std::string GetName([[maybe_unused]] int index)
    {
        using T            = typename P::value_type;
        std::string result = P::k_atomic ? "Atomic" : "Plain";
        if constexpr (std::same_as<T, i32>)
        {
            result += "I32";
        }
        else if constexpr (std::same_as<T, u32>)
        {
            result += "U32";
        }
        else if constexpr (std::same_as<T, i64>)
        {
            result += "I64";
        }
        else if constexpr (std::same_as<T, u64>)
        {
            result += "U64";
        }
        else if constexpr (std::same_as<T, f32>)
        {
            result += "F32";
        }
        else if constexpr (std::same_as<T, f64>)
        {
            result += "F64";
        }
        return result;
    }
};

inline constexpr usize k_writer_count = 4;
inline constexpr usize k_iterations   = 2000;
inline constexpr usize k_operations   = k_writer_count * k_iterations;

template<typename Fn>
void runWriters(Fn fn)
{
    std::barrier                             start{ static_cast<isize>(k_writer_count) };
    std::array<std::jthread, k_writer_count> writers;
    for (usize writer = 0; writer < writers.size(); ++writer)
    {
        writers[writer] = std::jthread(
            [&, writer]
            {
                start.arrive_and_wait();
                fn(writer);
            });
    }
    // jthread destruction joins before fn and the caller's captured data expire.
}

inline std::vector<u64> sequence(usize count, u64 first = 0)
{
    std::vector<u64> values(count);
    // clang-tidy may use a standard library without C++23 ranges::iota.
    // NOLINTNEXTLINE(modernize-use-ranges)
    std::iota(values.begin(), values.end(), first);
    return values;
}

}  // namespace realm::base::metrics::tests
