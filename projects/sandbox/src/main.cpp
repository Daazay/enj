#include <realm/base/debug/assert.hpp>
#include <realm/version/version.hpp>

#include <fmt/base.h>

int main()
{
    fmt::println("version: {}", realm::version::k_version_str);
    RE_ASSERT_MSG(realm::version::k_version == realm::version::Version(1, 0, 0, 0), "wrong version!");
    return 0;
}
