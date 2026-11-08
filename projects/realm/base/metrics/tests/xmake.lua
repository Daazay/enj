target("tests-realm-base-metrics")
    set_kind("binary")
    add_tests("default")
    set_default(false)
    add_defines("RE_ENABLE_ASSERTIONS=1")
    add_files("src/test_*.cpp")
    add_headerfiles("src/*.hpp")

    add_syslinks("pthread")

    add_deps("realm-base-metrics")
    add_rules("third-party.gtest")
target_end()
