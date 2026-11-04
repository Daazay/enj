add_requires("fmt 12.2.0")

rule("third-party.fmtlib")
    on_load(function (target)
        target:add("packages", "fmt", {public = true})
    end)

    on_config(function (target)
        target:add("defines", "FMT_EXCEPTIONS=0", {public=true})

        -- Enforce compile-time checking of ALL format strings
        target:add("defines", "FMT_ENFORCE_COMPILE_STRING", {public = true})

        -- Enable header-only mode for maximum runtime speed
        target:add("defines", "FMT_HEADER_ONLY", {public = true})
    end)
rule_end()