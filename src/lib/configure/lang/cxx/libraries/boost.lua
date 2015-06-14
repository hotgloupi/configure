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
	build:debug("Found boost version header at", boost_version_header)
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
			build:error("Couldn't find Boost library directory (checked " .. table.tostring(dirs) .. ")")
		end
	)
	local components = args.components
	if components == nil then
		build:error("You must provide a list of Boost libraries to search for")
	end

	local component_files = {}
	for _, lib in ipairs(fs:glob(boost_library_dir, "libboost_*")) do
		for _, component in ipairs(components) do
			local filename = tostring(lib:path():filename())
			if filename:starts_with("libboost_" .. component) or
				(build:target():os() == Platform.OS.windows and
				 filename:starts_with("boost_")) then
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

		local function arg(name, default)
			local res = args[component .. '_' .. name]
			if res == nil then return default end
			return res
		end

		local abi_flags = {
			threading = arg('threading', args.compiler.threading),
			static_runtime = arg('static_runtime', args.compiler.runtime == 'static'),
			debug_runtime = arg('debug_runtime', args.compiler.debug_runtime),
			debug = arg('debug', args.compiler.debug),
		}
		local files, selected, unknown = filtered, {}, {}
		for _, f in ipairs(files) do
			-- Boost library files are as follow:
			-- (lib)?boost_<COMPONENT>(-<FLAGS>)?.(lib|a|so)(.<VERSION>)?
			local flags = tostring(f:path():filename()):match("-[^.]*")
			local parts = {}
			if flags ~= nil then
				parts = flags:strip('-'):split('-')
			end
			local check = nil
			for i, part in ipairs(parts) do
				if check == false then break end
				if part == "mt" then
					check = abi_flags[threading]
				elseif part:starts_with('vc') then
					-- TODO: Check against toolset
				elseif part:match("%d+_%d+_%d+") then
					-- TODO: Check the version
				elseif part:match("^s?g?d?p?n?$") then
					local file_abi_flags = {
						static_runtime = part:find('s') ~= nil,
						debug_runtime = part:find('g') ~= nil,
						debug = part:find('d') ~= nil,
						stlport = part:find('p') ~= nil,
						native_iostreams = part:find('n') ~= nil,
					}
					build:debug("Found abi flags of", f, (function()
						local res = ""
						for k, v in pairs(file_abi_flags) do
							res = res .. ' ' .. k .. '=' .. tostring(v);
						end
					end)())
					for k, v in pairs(abi_flags) do
						if k ~= 'threading' and v ~= file_abi_flags[k] then
							build:debug("Ignore", f, "(The", k, "abi flag",
							            (v and "is not" or "is"), " present)")
							check = false
							break
						else
							check = true
						end
					end
				else
					build:error("Unknown boost library name part '" .. part .. "'")
				end
			end
			if check == true then
				table.append(selected, f)
			elseif check == nil then
				table.append(unknown, f)
			end
		end

		if #selected > 0 then
			files = selected
		elseif #unknown > 0 then
			files = unknown
		else
			build:error("Couldn't find any library file for Boost component '"
			            .. component .. "' in:", table.tostring(files))
		end

		if #files > 1 then
			build:error("Too many file selected for Boost component '"
			            .. component .. "':", table.tostring(files))
		end

		local defines = default_component_defines(component, kind, threading)
		table.extend(defines, args[component .. '_defines'] or args.defines or {})

		build:debug("Boost component '" .. component .. "' defines:", table.tostring(defines))
		table.append(res, Library:new{
			name = "Boost." .. component,
			include_directories = { boost_include_dir },
			files = files,
			defines = defines,
		})
	end
	return res
end

return M
