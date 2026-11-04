set_xmakever("3.1.1")

set_project("realm")
set_version("0.1.0")

add_rules("mode.debug", "mode.release")
set_defaultmode("debug")

-- Shared config
includes("config/xmake/*.lua")
add_rules(
    "realm.architecture",
    "realm.build_config",
    "realm.platform",
    "realm.toolchain"
)

-- Custom tasks
add_plugindirs("config/xmake/tasks")

-- Auto regenerate compile_commands
add_rules("plugin.compile_commands.autoupdate")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".", lsp = "clangd"})

-- Third-parties
includes("config/xmake/packages/*.lua")

-- Projects
includes("projects")
