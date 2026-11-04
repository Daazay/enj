target("realm-version")
    set_kind("headeronly")

    set_configdir("$(builddir)/generated/$(plat)/$(arch)/$(mode)/version")
    set_configvar("VERSION_SUB", get_config("mode"))
    add_configfiles("version.hpp.in", {filename = "realm/version/version.hpp"})
    add_includedirs("$(builddir)/generated/$(plat)/$(arch)/$(mode)/version", {public = true})

    add_deps("realm-base-common")
target_end()
