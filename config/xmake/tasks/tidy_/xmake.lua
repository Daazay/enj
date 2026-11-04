task("tidy_")
    set_category("plugin")

    on_run(function ()
        import("lib.detect.find_tool")
        import("core.base.option")
        import("core.base.json")
        import("async.runjobs")

        local tidy = find_tool("clang-tidy")
        if not tidy then
            raise("clang-tidy not found in PATH")
        end

        local root     = os.projectdir()
        local checkdir = "projects"
        local cdb_file = path.join(root, "compile_commands.json")

        -- Only translation units can be analyzed directly.
        -- Headers are analyzed transitively through the TUs that include them
        -- (see HeaderFilterRegex in .clang-tidy).
        local tu_exts = {
            [".c"]   = true, [".cc"]   = true, [".cpp"] = true,
            [".cxx"] = true, [".cppm"] = true, [".ixx"] = true,
        }
        local hdr_exts = {
            [".h"]   = true, [".hh"]  = true, [".hpp"] = true, [".hxx"] = true,
            [".inl"] = true, [".ipp"] = true, [".tpp"] = true,
        }

        local function norm(p)
            return path.normalize(p):gsub("\\", "/")
        end

        local function ext_of(p)
            local e = path.extension(p)
            return e and e:lower() or nil
        end

        local function is_tu(p)
            local e = ext_of(p); return e and tu_exts[e] or false
        end

        local function is_hdr(p)
            local e = ext_of(p); return e and hdr_exts[e] or false
        end

        local function in_checkdir(rel)
            return norm(rel):startswith(checkdir .. "/")
        end

        ------------------------------------------------------------------
        -- compile_commands.json

        local function read_cdb()
            if not os.isfile(cdb_file) then return nil end
            local data = try {
                function ()
                    return json.loadfile(cdb_file)
                end,
                catch {
                    function (errors)
                        print("Warning: failed to parse compile_commands.json: %s", errors)
                    end
                }
            }
            if type(data) ~= "table" then return nil end
            return data
        end

        local function cdb_known_tus(cdb)
            local known = {}
            for _, e in ipairs(cdb) do
                local f = e.file
                if f then
                    if not path.is_absolute(f) and e.directory then
                        f = path.join(e.directory, f)
                    end
                    known[norm(f)] = true
                end
            end
            return known
        end

        local function ensure_compile_commands()
            if not option.get("refresh") and os.isfile(cdb_file) then
                return
            end
            print("Generating compile_commands.json ...")
            local ok = try { function ()
                os.execv("xmake", { "project", "-k", "compile_commands",
                                    "--lsp=clangd" })
                return true
            end }
            if not ok or not os.isfile(cdb_file) then
                raise("failed to generate compile_commands.json " ..
                      "(run `xmake project -k compile_commands --lsp=clangd` " ..
                      "manually to diagnose)")
            end
        end

        ------------------------------------------------------------------
        -- file collection

        local function collect_all_tus()
            local files = {}
            for _, f in ipairs(os.files(path.join(root, checkdir, "**"))) do
                if is_tu(f) then
                    table.insert(files, f)
                end
            end
            return files
        end

        local function git_staged_paths()
            -- `--relative` makes git emit paths relative to `root`, which matters when the project lives in a monorepo.
            local out = try { function ()
                return os.iorunv("git", {
                    "-C", root, "diff", "--cached",
                    "--name-only", "--diff-filter=ACMR", "--relative",
                })
            end }
            if not out then return {} end  -- not a git repo / git missing
            local paths = {}
            for line in out:gmatch("[^\r\n]+") do
                line = line:gsub("\\", "/")
                if in_checkdir(line) then
                    table.insert(paths, line)
                end
            end
            return paths
        end

        local function collect_staged_tus()
            local paths = git_staged_paths()
            local tus, has_headers = {}, false
            for _, rel in ipairs(paths) do
                if is_tu(rel) then
                    table.insert(tus, path.join(root, rel))
                elseif is_hdr(rel) then
                    has_headers = true
                end
            end
            return tus, has_headers
        end

        ------------------------------------------------------------------
        -- main

        ensure_compile_commands()

        local cdb   = read_cdb()
        local known = cdb and cdb_known_tus(cdb) or nil

        local files
        if option.get("all") then
            files = collect_all_tus()
        else
            local has_headers
            files, has_headers = collect_staged_tus()
            if #files == 0 then
                if has_headers then
                    print("Only headers were staged; clang-tidy analyzes "
                       .. "headers through the TUs that include them.")
                    print("Run `xmake tidy_ --all` to re-check every TU.")
                else
                    print("No staged C/C++ translation units to process.")
                end
                return
            end
        end

        -- Drop files clang-tidy has no compile command for;
        -- otherwise it prints "Skipping <file>. Compile command not found." (which would also trip --warnings-as-errors).
        if known then
            local filtered, skipped = {}, 0
            for _, f in ipairs(files) do
                if known[norm(f)] then
                    table.insert(filtered, f)
                else
                    skipped = skipped + 1
                end
            end
            if skipped > 0 then
                print("Skipping %d file(s) not present in compile_commands.json.",
                      skipped)
            end
            files = filtered
        end

        if #files == 0 then
            print("Nothing to do.")
            return
        end

        local fix       = option.get("fix")
        local tidy_args = { "-p", root }
        if fix then
            table.insert(tidy_args, "--fix-errors")
        else
            table.insert(tidy_args, "--warnings-as-errors=*")
        end

        local extra = option.get("extra")
        if extra then
            for a in extra:gmatch("%S+") do
                table.insert(tidy_args, a)
            end
        end

        -- --fix must be serial: two clang-tidy processes fixing the same
        -- header concurrently will clobber each other's edits.
        local jobs
        if fix then
            jobs = 1
            local j = option.get("jobs")
            if j and j ~= "auto" then
                print("Note: --fix runs serially to avoid concurrent " ..
                      "edits to shared headers.")
            end
        else
            jobs = tonumber(option.get("jobs")) or os.default_njob()
        end

        local failed = {}
        runjobs("tidy", function (index)
            local f  = files[index]
            local ok = try { function ()
                os.execv(tidy.program, table.join(tidy_args, f))
                return true
            end }
            if not ok then
                table.insert(failed, f)
            end
        end, { total = #files, comax = jobs })

        if fix then
            print("Done: %d file(s) processed (clang-tidy --fix-errors).", #files)
            print("Review the diff, then re-run `xmake tidy_` to confirm.")
            return
        end

        if #failed > 0 then
            print("")
            print("FAILED: %d / %d file(s):", #failed, #files)
            for _, f in ipairs(failed) do
                print("  - " .. f)
            end
            print("")
            print("Run `xmake tidy_ --fix` to apply auto-fixes.")
            raise("clang-tidy failed")
        end

        print("OK: %d / %d file(s) passed.", #files, #files)
    end)

    set_menu {
        usage       = "xmake tidy_ [options]",
        description = "Run clang-tidy using the project's compilation database",
        options = {
            { nil, "fix",     "k",  nil,    "Apply auto-fixes in place (serial; safe on shared headers)" },
            { nil, "all",     "k",  nil,    "Check every TU under projects/ (ignores staging)" },
            { nil, "jobs",    "kv", "auto", "Number of parallel clang-tidy jobs" },
            { nil, "refresh", "k",  nil,    "Regenerate compile_commands.json before running" },
            { nil, "extra",   "kv", nil,    "Extra clang-tidy arguments (space-separated)" },
        },
    }
task_end()