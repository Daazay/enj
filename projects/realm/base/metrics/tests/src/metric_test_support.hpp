#pragma once

#include <realm/base/common/typedefs.hpp>
#include <realm/base/metrics/metric_value_storage.hpp>

#include <array>
#include <thread>

#include <gtest/gtest.h>

namespace realm::base::metrics::tests
{

using StoragePolicies = ::testing::Types<StoragePolicy::Unsafe, StoragePolicy::Safe>;

inline constexpr i64 worker_count       = 4;
inline constexpr i64 samples_per_worker = 2000;
inline constexpr i64 sample_count       = worker_count * samples_per_worker;

template<typename Operation>
void runConcurrently(Operation operation)
{
    std::array<std::jthread, worker_count> workers;
    for (i64 worker = 0; worker < worker_count; ++worker)
    {
        workers[worker] = std::jthread{ [operation, worker]
                                        {
                                            for (i64 sample = 0; sample < samples_per_worker; ++sample)
                                            {
                                                operation(worker);
                                            }
                                        } };
    }
}

}  // namespace realm::base::metrics::tests
