includes("common")
includes("debug")
includes("metrics")

target("realm-base")
    set_kind("phony")
    add_deps(
        "realm-base-common",
        "realm-base-debug",
        "realm-base-metrics"
    )
target_end()
