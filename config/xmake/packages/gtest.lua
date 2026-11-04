add_requires("gtest v1.18.0")

rule("third-party.gtest")
    on_load(function (target)
        target:add("packages", "gtest", {public = true})
        target:add("links", "gtest_main", {public = true})
    end)

    on_config(function (target)
    end)
rule_end()