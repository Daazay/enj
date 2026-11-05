add_requires("magic_enum v0.9.8")

rule("third-party.magic_enum")
    on_load(function (target)
        target:add("packages", "magic_enum", {public = true})
    end)

    on_config(function (target)
        -- Disable magic_enum's internal assertions.
        target:add("defines", "MAGIC_ENUM_NO_ASSERT")

        -- Enable hash-based lookup for faster enum_name.
        -- NOTE: Comment out if you prefer the default linear search.
        -- target:add("defines", "MAGIC_ENUM_ENABLE_HASH")
    end)
rule_end()