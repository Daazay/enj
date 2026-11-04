includes("version")

target("realm")
    set_kind("phony")
    add_deps("realm-version")
target_end()