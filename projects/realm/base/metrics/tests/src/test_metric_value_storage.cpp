#include "test_support.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace
{

using namespace realm;
using namespace realm::base::metrics;

static_assert(!traits::MetricNumber<bool>);
static_assert(!traits::MetricNumber<i32 const>);
static_assert(!traits::MetricValueStorage<i32>);
static_assert(traits::MetricValueStorage<detail::MetricValueStoragePlain<i32>>);
static_assert(traits::MetricValueStorage<detail::MetricValueStorageAtomic<i32>>);
static_assert(
    []
    {
        detail::MetricValueStoragePlain<i32> storage{ 2 };
        if (storage.fetchAdd(3) != 2 || storage.fetchSub(1) != 5)
        {
            return false;
        }
        return storage.exchange(9) == 4 && storage.load() == 9;
    }());

template<typename P>
class MetricValueStorageTest : public testing::Test
{
};

TYPED_TEST_SUITE(MetricValueStorageTest, tests::ScalarPolicies, tests::PolicyNames);

//

TYPED_TEST(MetricValueStorageTest, InitializesLoadsAndStores)
{
    using T       = typename TypeParam::value_type;
    using Storage = typename TypeParam::template storage_type<T>;

    Storage       storage;
    Storage const initialized{ T{ 7 } };
    EXPECT_EQ(storage.load(), T{ 0 });
    EXPECT_EQ(initialized.load(), T{ 7 });
    storage.store(T{ 3 });
    EXPECT_EQ(storage.load(), T{ 3 });
}

TYPED_TEST(MetricValueStorageTest, FetchOperationsAndExchangeReturnPreviousValue)
{
    using T       = typename TypeParam::value_type;
    using Storage = typename TypeParam::template storage_type<T>;

    Storage storage{ T{ 3 } };
    EXPECT_EQ(storage.fetchAdd(T{ 4 }), T{ 3 });
    EXPECT_EQ(storage.load(), T{ 7 });
    EXPECT_EQ(storage.fetchSub(T{ 2 }), T{ 7 });
    EXPECT_EQ(storage.load(), T{ 5 });
    EXPECT_EQ(storage.exchange(T{ 9 }), T{ 5 });
    EXPECT_EQ(storage.load(), T{ 9 });
}

TYPED_TEST(MetricValueStorageTest, AddSubtractAndTransformReturnNewValue)
{
    using T       = typename TypeParam::value_type;
    using Storage = typename TypeParam::template storage_type<T>;

    Storage storage{ T{ 3 } };
    EXPECT_EQ(storage.add(T{ 4 }), T{ 7 });
    EXPECT_EQ(storage.sub(T{ 2 }), T{ 5 });
    EXPECT_EQ(storage.transform([](T current) noexcept -> T { return current * T{ 2 }; }), T{ 10 });
    EXPECT_EQ(storage.load(), T{ 10 });
}

template<typename P>
class MetricValueStorageFloatingTest : public testing::Test
{
};
TYPED_TEST_SUITE(MetricValueStorageFloatingTest, tests::FloatingPolicies, tests::PolicyNames);

TYPED_TEST(MetricValueStorageFloatingTest, TransformPreservesSignedZeroAndNaNPayloadBits)
{
    using T       = typename TypeParam::value_type;
    using Bits    = std::conditional_t<sizeof(T) == sizeof(u32), u32, u64>;
    using Storage = typename TypeParam::template storage_type<T>;

    Storage storage{ T{ 0 } };
    storage.transform([](T) noexcept -> T { return -T{ 0 }; });
    EXPECT_TRUE(std::signbit(storage.load()));
    auto const bits = static_cast<Bits>(std::bit_cast<Bits>(std::numeric_limits<T>::quiet_NaN()) | Bits{ 1 });
    auto const nan  = std::bit_cast<T>(bits);
    storage.transform([nan](T) noexcept -> T { return nan; });
    EXPECT_EQ(std::bit_cast<Bits>(storage.load()), bits);
    storage.transform([](T current) noexcept -> T { return current; });
    EXPECT_EQ(std::bit_cast<Bits>(storage.load()), bits);
}

TEST(AtomicMetricValueStorageTest, ConcurrentFetchAddReturnsUniquePreviousValues)
{
    detail::MetricValueStorageAtomic<u64> storage;
    std::vector<u64>                      previous(tests::k_operations);
    tests::runWriters(
        [&](usize writer)
        {
            for (usize i = 0; i < tests::k_iterations; ++i)
            {
                previous[(writer * tests::k_iterations) + i] = storage.fetchAdd(1);
            }
        });
    std::ranges::sort(previous);
    EXPECT_EQ(previous, tests::sequence(tests::k_operations));
    EXPECT_EQ(storage.load(), tests::k_operations);
}
}  // namespace
