includes("tests")

target("realm-base-metrics")
    set_kind("headeronly")
    add_includedirs("include", {public = true})
    add_headerfiles("include/(realm/base/metrics/**.hpp)")

    add_deps("realm-base-common")
    add_rules("third-party.magic_enum")
target_end()