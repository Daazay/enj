rule("realm.build_config")
    on_config(function (target)
        if is_mode("debug") then
            target:add("defines", "RE_BUILD_CONFIG_DEBUG")
        elseif is_mode("release") then
            target:add("defines", "RE_BUILD_CONFIG_RELEASE")
        end
    end)
rule_end()
