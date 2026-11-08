#include "realm/base/debug/assert.hpp"

#include <array>
#include <atomic>
#include <cstdlib>
#include <utility>

#include <fmt/base.h>

#if defined(RE_PLATFORM_LINUX)
    #include <unistd.h>
#endif

namespace
{

using namespace realm;
using namespace realm::base::debug;

inline constexpr usize k_diag_buf_size = 4_KB;

struct ThreadContext
{
    AssertionHandler handler{};
    bool             in_handler{ false };
};

std::atomic<AssertionHandler> g_handler{ {} };
RE_THREAD_LOCAL ThreadContext t_thread_ctx{};

void writeStderr(char const* data, usize len) noexcept
{
#if defined(RE_PLATFORM_LINUX)
    ::write(2, data, len);
#elif defined(RE_PLATFORM_WINDOWS)
    ::_write(2, data, static_cast<unsigned>(len));
#else
    #error "Not implemented"
#endif
}

void defaultAssertionCallback(AssertionFailedInfo const& info, RE_MAYBE_UNUSED void* ctx) noexcept
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    std::array<char, k_diag_buf_size> buffer;

    auto const result = fmt::format_to_n(buffer.data(), buffer.size(),
                                         "Assertion failed!\n"
                                         "  expression: {}\n"
                                         "  message:    {}\n"
                                         "  location:   {}:{} in {}\n",
                                         info.expression, info.message, info.location.file_name(), info.location.line(),
                                         info.location.function_name());

    auto written = static_cast<usize>(result.out - buffer.data());

    // Guarantee a trailing '\n' so the diagnostic never merges with the next shell prompt / log line.
    if (written < buffer.size() && (written == 0 || buffer[written - 1] != '\n'))
    {
        buffer[written++] = '\n';
    }

    writeStderr(buffer.data(), written);
}

}  // namespace

namespace realm::base::debug::detail
{

RE_NO_RETURN void onAssertionFailed(AssertionFailedInfo const& info) noexcept
{
    if (!t_thread_ctx.in_handler)
    {
        t_thread_ctx.in_handler = true;

        AssertionHandler handler = getThreadLocalAssertionHandler();

        if (handler.cb_fn == nullptr)
        {
            handler = getGlobalAssertionHandler();
        }
        if (handler.cb_fn == nullptr)
        {
            handler = getDefaultAssertionHandler();
        }

        handler.cb_fn(info, handler.ctx);
    }

    std::abort();
}

}  // namespace realm::base::debug::detail

namespace realm::base::debug
{

AssertionHandler getDefaultAssertionHandler() noexcept
{
    return AssertionHandler{ .cb_fn = &defaultAssertionCallback, .ctx = nullptr };
}

AssertionHandler getGlobalAssertionHandler() noexcept
{
    return g_handler.load(std::memory_order_acquire);
}

AssertionHandler setGlobalAssertionHandler(AssertionHandler const& handler) noexcept
{
    return g_handler.exchange(handler, std::memory_order_acq_rel);
}

AssertionHandler getThreadLocalAssertionHandler() noexcept
{
    return t_thread_ctx.handler;
}

AssertionHandler setThreadLocalAssertionHandler(AssertionHandler const& handler) noexcept
{
    return std::exchange(t_thread_ctx.handler, handler);
}

}  // namespace realm::base::debug
