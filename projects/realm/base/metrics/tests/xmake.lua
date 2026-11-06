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

-- Compile-only checks include positive controls so a broken test setup cannot pass every rejection.
target("realm-base-metrics-declaration-tests")
    set_kind("object")
    set_default(false)
    add_deps("realm-base-metrics")

    local factory_cases = {
        "default_factory", "factory_missing_noexcept", "factory_wrong_return",
        "factory_wrong_arguments", "factory_not_constant", "invalid_name", "custom_factory"
    }
    for index, name in ipairs(factory_cases) do
        add_tests(name, {
            files = "compile/factory.cpp",
            defines = "RE_METRIC_TEST_CASE=" .. (index - 1),
            build_should_pass = index == 1 or index == 7,
            build_should_fail = index ~= 1 and index ~= 7
        })
    end

    local histogram_cases = {
        "valid_bounds", "descending_bounds", "duplicate_bounds", "nan_upper_bound",
        "nan_lower_bound", "infinite_bound", "missing_bounds"
    }
    for index, name in ipairs(histogram_cases) do
        add_tests(name, {
            files = "compile/histogram.cpp",
            defines = "RE_METRIC_TEST_CASE=" .. (index - 1),
            build_should_pass = index == 1,
            build_should_fail = index ~= 1
        })
    end

    local membership_cases = {
        "explicit_membership", "duplicate_ids", "mixed_enums", "missing_traits", "duplicate_names",
        "fallback_large_id", "empty_name", "excluded_id", "excluded_name", "non_enum_id", "empty_membership"
    }
    for index, name in ipairs(membership_cases) do
        add_tests(name, {
            files = "compile/membership.cpp",
            defines = "RE_METRIC_TEST_CASE=" .. (index - 1),
            build_should_pass = index == 1 or index == 6,
            build_should_fail = index ~= 1 and index ~= 6
        })
    end
target_end()
