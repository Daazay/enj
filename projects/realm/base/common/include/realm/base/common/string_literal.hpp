#pragma once

#include <realm/base/common/defines.hpp>
#include <realm/base/common/typedefs.hpp>

#include <array>
#include <string_view>

namespace realm
{

template<usize N>
struct StringLiteral
{
public:
    static constexpr usize capacity = N;

public:
    constexpr StringLiteral() noexcept = default;

    constexpr StringLiteral(char const (&s)[N]) noexcept
    {
        for (usize i = 0; i < N; ++i)
        {
            chars[i] = s[i];
        }
    }

public:
    RE_NODISCARD constexpr const char* data() const noexcept { return chars.data(); }
    // NOLINTNEXTLINE(readability-identifier-naming)
    RE_NODISCARD constexpr const char* c_str() const noexcept { return chars.data(); }

    RE_NODISCARD constexpr usize size() const noexcept { return N - 1; }
    RE_NODISCARD constexpr usize length() const noexcept { return N - 1; }

    RE_NODISCARD constexpr std::string_view view() const noexcept { return { chars.data(), N - 1 }; }

    RE_NODISCARD constexpr operator std::string_view() const noexcept { return view(); }

    RE_NODISCARD constexpr char operator[](usize i) const noexcept { return chars[i]; }

public:
    constexpr bool operator==(StringLiteral const&) const noexcept  = default;
    constexpr auto operator<=>(StringLiteral const&) const noexcept = default;

public:
    std::array<char, N> chars{};
};

template<usize N>
StringLiteral(char const (&)[N]) -> StringLiteral<N>;

}  // namespace realm
