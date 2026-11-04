target("realm-version")
    set_kind("headeronly")

    set_configdir("$(builddir)/generated/$(plat)/$(arch)/$(mode)/config")
    set_configvar("VERSION_SUB", get_config("mode"))
    add_configfiles("version.hpp.in", {filename = "realm/config/version.hpp"})
    add_includedirs("$(builddir)/generated/$(plat)/$(arch)/$(mode)/config", {public = true})
target_end()
