target("sandbox")
    set_kind("binary")
    set_default(true)
    add_files("src/*.cpp")

    add_deps("realm")
    add_rules("third-party.fmtlib")
target_end()
