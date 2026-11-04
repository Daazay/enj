includes("common")

target("realm-base")
    set_kind("phony")
    add_deps("realm-base-common")
target_end()
