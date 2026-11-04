#include <realm/version/version.hpp>

#include <fmt/base.h>

int main()
{
    fmt::println("version: {}", realm::version::k_version_str);
    return 0;
}
