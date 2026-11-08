includes("common")
includes("debug")

target("realm-base")
    set_kind("phony")
    add_deps(
        "realm-base-common",
        "realm-base-debug"
    )
target_end()
