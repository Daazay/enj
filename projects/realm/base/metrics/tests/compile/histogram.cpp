#include <realm/base/metrics/metrics/histogram.hpp>

#include <limits>

using Histogram = realm::base::metrics::MetricHistogram<double, 3>;

#if RE_METRIC_TEST_CASE == 1
Histogram histogram{ { 20.0, 10.0 } };
#elif RE_METRIC_TEST_CASE == 2
Histogram histogram{ { 10.0, 10.0 } };
#elif RE_METRIC_TEST_CASE == 3
Histogram histogram{ { 10.0, std::numeric_limits<double>::quiet_NaN() } };
#elif RE_METRIC_TEST_CASE == 4
Histogram histogram{ { std::numeric_limits<double>::quiet_NaN(), 20.0 } };
#elif RE_METRIC_TEST_CASE == 5
Histogram histogram{ { 10.0, std::numeric_limits<double>::infinity() } };
#elif RE_METRIC_TEST_CASE == 6
Histogram histogram;
#else
constexpr Histogram histogram{ { 10.0, 20.0 } };
static_assert(histogram.bound(0) == 10.0);
static_assert(histogram.bound(1) == 20.0);
static_assert(histogram.total() == 0);
#endif
