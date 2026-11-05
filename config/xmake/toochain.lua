rule("realm.toolchain")
    on_config(function (target)
        -- Common
        target:set("languages", "cxx23")
        target:set("exceptions", "no-cxx")
        target:set("warnings", "allextra", "error")
        target:set("encodings", "utf-8")

        -- Per toolchain configurations
        if target:has_tool("cxx", "clang", "clangxx") then
            target:add("defines", "RE_COMPILER_CLANG")

            if target:is_plat("linux") then
                target:add("ldflags", "-static-libstdc++", "-static-libgcc")
                target:add("shflags", "-static-libstdc++", "-static-libgcc")
            end
        elseif target:has_tool("cxx", "gcc", "gxx") then
            target:add("defines", "RE_COMPILER_GCC")

            if target:is_plat("linux") then
                target:add("ldflags", "-static-libstdc++", "-static-libgcc")
                target:add("shflags", "-static-libstdc++", "-static-libgcc")
            end
        end
    end)
rule_end()
