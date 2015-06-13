--- C++ Boost libraries
-- @module configure.lang.cxx.libraries

local M = {}

local function default_component_defines(component, kind, threading)
	if component == 'unit_test_framework' and kind == 'shared' then
		return {'BOOST_TEST_DYN_LINK'}
	end
	return {}
end

--- Find Boost libraries
--
-- @param args
-- @param args.compiler A compiler instance
-- @param args.components A list of components
-- @param[opt] args.version The required version (defaults to the latest found)
-- @param[opt] args.env_prefix A prefix for all environment variables (default to BOOST)
-- @param[opt] args.kind 'shared' or 'static' (default to 'static')
-- @param[opt] args.defines A list of preprocessor definitions
-- @param[opt] args.threading Search for threading enable libraries (default to the compiler default)
-- @param[opt] args.<COMPONENT>_kind Select the 'static' or 'shared' version of a component.
-- @param[opt] args.<COMPONENT>_defines A list of preprocessor definitions
function M.find(args)
	local env_prefix = args.env_prefix or 'BOOST'
	local build = args.compiler.build
	local fs = build:fs()
	local boost_root = build:path_option(
		env_prefix .. '-root',
		"Boost root directory"
	)
	local boost_include_dir = build:lazy_path_option(
		env_prefix .. '-include-dir',
		"Boost include directory",
		function ()
			local dirs = {}
			if boost_root ~= nil then
				table.append(dirs, boost_root)
				table.append(dirs, boost_root / 'include')
				print(boost_root / "include")
			end
			table.extend(dirs, args.compiler:system_include_directories())
			return fs:find_file(
				dirs,
				'boost/version.hpp'
			):path():parent_path():parent_path()
		end
	)
	local boost_version_header = build:file_node(boost_include_dir / 'boost/version.hpp')
	if not boost_version_header:path():exists() then
		build:error("Couldn't find 'boost/version.hpp' in", boost_include_dir)
	end
	local boost_library_dir = build:lazy_path_option(
		env_prefix .. '-library-dir',
		"Boost library dir",
		function ()
			local dirs = {}
			if boost_root then
				table.append(dirs, boost_root / 'lib')
				table.append(dirs, boost_root / 'stage/lib')
			end
			table.extend(dirs, args.compiler:system_library_directories())
			for _, dir in ipairs(dirs) do
				build:debug("Examining library directory", dir)
				for _, lib in ipairs(fs:glob(dir, "libboost_*")) do
					-- return when some file is found
					return dir
				end
			end
		end
	)
	local components = args.components
	if components == nil then
		build:error("You must provide a list of Boost libraries to search for")
	end

	local component_files = {}
	for _, lib in ipairs(fs:glob(boost_library_dir, "libboost_*")) do
		for _, component in ipairs(components) do
			if tostring(lib:path():filename()):starts_with("libboost_" .. component) then
				if component_files[component] == nil then
					component_files[component] = {}
				end
				table.append(component_files[component], lib)
				build:debug("Found Boost library", lib, "for component", component)
			end
		end
	end

	local Library = require('configure.lang.cxx.Library')
	local res = {}
	for _, component in ipairs(components) do
		if component_files[component] == nil then
			build:error("Couldn't find library files for boost component '" .. component .. "'")
		end
		local files = component_files[component]
		local filtered = {}
		local kind = args[component .. '_kind'] or args.kind or 'static'
		-- Filter files based on the kind selected ('static' or 'shared')
		local ext = args.compiler:_library_extension(kind)
		for i, f in ipairs(files) do
			if tostring(f:path()):ends_with(ext) then
				build:debug("Select", f, "(ends with '" .. ext .."')")
				table.append(filtered, f)
			else
				build:debug("Ignore", f, "(do not end with '" .. ext .."')")
			end
		end

		-- Find out if file names have the threading flag ('-mt')
		local has_threading_flag = false
		for _, f in ipairs(filtered) do
			if string.find(tostring(f:path()), '-mt.') ~= nil then
				has_threading_flag = true
				build:debug("Selected files have the threading flag ('-mt')")
				break
			end
		end

		if #filtered > 1 then
			-- If more than one file is selected, let's try to filter them
			if has_threading_flag then
				files, filtered = filtered, {}
				-- checking the threading model.
				local threading = args[component .. '_threading']
				if threading == nil then
					threading = args.compiler.threading
				end
				build:debug("Boost component '" .. component .. "' threading mode:", threading)
				for _, f in ipairs(files) do
					if tostring(f:path()):find('-mt.') ~= nil then
						if threading then
							table.append(filtered, f)
							build:debug("Select threading enabled file", f)
						end
					else
						if not threading then
							table.append(filtered, f)
							build:debug("Select threading disabled file", f)
						end
					end
				end
			end
		end

		build:debug("Found boost." .. component, "files:", table.tostring(filtered))
		if #filtered == 0 then
			build:error("Couldn't find any library file for Boost component '" .. component .. "'")
		end

		if #filtered > 1 then
			build:error("Too many file selected for Boost component '" .. component .. "':", table.tostring(files))
		end

		local defines = default_component_defines(component, kind, threading)
		table.extend(defines, args[component .. '_defines'] or args.defines or {})

		build:debug("Boost component '" .. component .. "' defines:", table.tostring(defines))
		table.append(res, Library:new{
			name = "Boost." .. component,
			include_directories = { boost_include_dir },
			files = filtered,
			defines = defines,
		})
	end
	return res
end

return M
