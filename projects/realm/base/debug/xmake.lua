target("realm-base-debug")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_headerfiles("include/(realm/base/debug/*.hpp)")
    add_files("src/*.cpp")

    add_deps("realm-base-common")
    add_rules("third-party.fmtlib")
target_end()
