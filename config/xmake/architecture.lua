rule("realm.architecture")
    on_config(function (target)
        if target:is_arch("x64", "x86_64") then
            target:add("defines", "RE_ARCH_X64")
        end
    end)
rule_end()