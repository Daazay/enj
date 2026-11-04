rule("realm.platform")
    on_config(function (target)
        if target:is_plat("linux") then
            target:add("defines", "RE_PLATFORM_LINUX")
        end
    end)
rule_end()
