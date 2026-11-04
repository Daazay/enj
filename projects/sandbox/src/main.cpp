#include <realm/config/version.hpp>

#include <fmt/base.h>

int main()
{
    fmt::println("version: {}", realm::config::k_version_str);
    return 0;
}
