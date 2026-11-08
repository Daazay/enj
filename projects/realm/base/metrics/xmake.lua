includes("tests")

target("realm-base-metrics")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_headerfiles("include/(realm/base/metrics/*.hpp)")

    add_deps(
        "realm-base-common",
        "realm-base-debug"
    )
    add_rules("third-party.magic_enum")
target_end()
