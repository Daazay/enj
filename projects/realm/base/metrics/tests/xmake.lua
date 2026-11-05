target("realm-base-metrics-tests")
    set_kind("binary")
    set_default(false)
    add_files("src/test_*.cpp")
    add_deps("realm-base-metrics")
    add_rules("third-party.gtest")
    if is_plat("linux") then
        add_syslinks("pthread")
    end
    add_tests("default")
target_end()
