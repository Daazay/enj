includes("base")
includes("version")

target("realm")
    set_kind("phony")
    add_deps(
        "realm-base",
        "realm-version"
    )
target_end()