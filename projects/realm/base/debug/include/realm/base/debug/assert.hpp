#pragma once

#include <realm/base/common/defines.hpp>
#include <realm/base/common/typedefs.hpp>

#include <array>
#include <source_location>
#include <string_view>
#include <utility>

#include <fmt/base.h>

namespace realm::base::debug
{

#if !defined(RE_ENABLE_ASSERTIONS)
    #if defined(RE_BUILD_CONFIG_DEBUG)
        #define RE_ENABLE_ASSERTIONS 1
    #else
        #define RE_ENABLE_ASSERTIONS 0
    #endif
#endif
inline constexpr bool k_enable_assertions = RE_ENABLE_ASSERTIONS;

#if !defined(RE_MAX_ASSERT_MESSAGE_SIZE)
    #define RE_MAX_ASSERT_MESSAGE_SIZE (2_KB - 1)
#endif
inline constexpr usize k_max_assert_message_size = RE_MAX_ASSERT_MESSAGE_SIZE;

//

struct AssertionFailedInfo
{
    std::string_view     expression;
    std::string_view     message;
    std::source_location location;
};

struct AssertionHandler
{
    using callback_fn = void (*)(AssertionFailedInfo const& info, void* ctx);

    callback_fn cb_fn;
    void*       ctx;
};

AssertionHandler getDefaultAssertionHandler() noexcept;

AssertionHandler getGlobalAssertionHandler() noexcept;
AssertionHandler setGlobalAssertionHandler(AssertionHandler const& handler) noexcept;

AssertionHandler getThreadLocalAssertionHandler() noexcept;
AssertionHandler setThreadLocalAssertionHandler(AssertionHandler const& handler) noexcept;

}  // namespace realm::base::debug

namespace realm::base::debug::detail
{

RE_NO_RETURN void onAssertionFailed(AssertionFailedInfo const& info) noexcept;

template<typename... Args>
RE_NO_RETURN RE_NO_INLINE void onAssertionFailed(std::string_view expression, std::source_location location,
                                                 fmt::format_string<Args...> format, Args&&... args) noexcept
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,-warnings-as-errors)
    std::array<char, k_max_assert_message_size + 1> buffer;

    auto const result  = fmt::format_to_n(buffer.data(), buffer.size() - 1, format, std::forward<Args>(args)...);
    auto const written = static_cast<usize>(result.out - buffer.data());

    onAssertionFailed(AssertionFailedInfo{
        .expression = expression, .message = std::string_view{ buffer.data(), written }, .location = location });
}

}  // namespace realm::base::debug::detail

#if !defined(RE_VERIFY_IMPL)
    #define RE_VERIFY_IMPL(expression, format, ...)                                                               \
        do                                                                                                        \
        {                                                                                                         \
            if (!(expression)) [[unlikely]]                                                                       \
            {                                                                                                     \
                ::realm::base::debug::detail::onAssertionFailed(                                                  \
                    RE_STRINGIZE(expression), std::source_location::current(), format __VA_OPT__(, __VA_ARGS__)); \
            }                                                                                                     \
        } while (false)
#endif

#define RE_VERIFY(expression)                  RE_VERIFY_IMPL(expression, "")
#define RE_VERIFY_MSG(expression, format, ...) RE_VERIFY_IMPL(expression, format __VA_OPT__(, __VA_ARGS__))

#if !defined(RE_ASSERT_IMPL)
    #define RE_ASSERT_IMPL(expression, format, ...) RE_VERIFY_IMPL(expression, format __VA_OPT__(, __VA_ARGS__))
#endif

#if RE_ENABLE_ASSERTIONS
    #define RE_ASSERT(expression)                  RE_ASSERT_IMPL(expression, "")
    #define RE_ASSERT_MSG(expression, format, ...) RE_ASSERT_IMPL(expression, format __VA_OPT__(, __VA_ARGS__))
#else
    #define RE_ASSERT(expression)                  RE_IGNORE(expression)
    #define RE_ASSERT_MSG(expression, format, ...) RE_IGNORE(expression)
#endif
