
require "package"
require "debug"

local deps = require "configure.dependency"
local cxx = require "configure.lang.cxx"
local c = require "configure.lang.c"

local version = '0.0.1'

function configure(build)
	build:status("Building on", build:host():os_string())

	local with_coverage = build:bool_option(
		"coverage",
		"Enable coverage build",
		false
	)
	local build_type = build:string_option(
		"build_type",
		"Set build type",
		'debug'
	):lower()

	if with_coverage then
		print("Coverage enabled:", with_coverage)
	end

	local fs = build:fs()
	for i, p in pairs(fs:rglob("src/lib/configure", "*.lua"))
	do
		fs:copy(
			p,
			"share/configure/lib/configure" /
				p:relative_path(build:project_directory() / "src/lib/configure")
		)
	end
	local compiler = cxx.compiler.find{
		build = build,
		standard = 'c++11',
	}

	lua_srcs = {
		"lua/src/lapi.c",
		"lua/src/lcode.c",
		"lua/src/lctype.c",
		"lua/src/ldebug.c",
		"lua/src/ldo.c",
		"lua/src/ldump.c",
		"lua/src/lfunc.c",
		"lua/src/lgc.c",
		"lua/src/llex.c",
		"lua/src/lmem.c",
		"lua/src/lobject.c",
		"lua/src/lopcodes.c",
		"lua/src/lparser.c",
		"lua/src/lstate.c",
		"lua/src/lstring.c",
		"lua/src/ltable.c",
		"lua/src/ltm.c",
		"lua/src/lundump.c",
		"lua/src/lvm.c",
		"lua/src/lzio.c",
		"lua/src/lauxlib.c",
		"lua/src/lbaselib.c",
		"lua/src/lbitlib.c",
		"lua/src/lcorolib.c",
		"lua/src/ldblib.c",
		"lua/src/liolib.c",
		"lua/src/lmathlib.c",
		"lua/src/loslib.c",
		"lua/src/lstrlib.c",
		"lua/src/ltablib.c",
		"lua/src/loadlib.c",
		"lua/src/linit.c",
	}

	local lua = compiler:link_static_library{
		name = "lua",
		directory = "lib",
		sources = lua_srcs,
		include_directories = {"lua/src"},
		object_directory = 'objects',
	}
	local libs = {lua,}
	local library_directories = {}
	local include_directories = {'src'}

	local boost_include_dir = build:path_option("BOOST_INCLUDE_DIR", "Boost include dir")
	local boost_library_dir = build:path_option("BOOST_LIBRARY_DIR", "Boost library dir")
	table.extend(include_directories, {boost_include_dir})
	table.extend(library_directories, {boost_library_dir})

	if build:host():os() == Platform.OS.windows then
		build:status("XXX Using boost auto link feature")
		table.extend(libs, {
			cxx.Library:new{name = 'Shlwapi', system = true, kind = 'static'},
		})
	else
		table.extend(libs, {
			cxx.Library:new{name = 'boost_filesystem', system = true, kind = 'static'},
			cxx.Library:new{name = 'boost_serialization', system = true, kind = 'static'},
			cxx.Library:new{name = 'boost_iostreams', system = true, kind = 'static'},
			cxx.Library:new{name = 'boost_exception', system = true, kind = 'static'},
			cxx.Library:new{name = 'boost_system', system = true, kind = 'static'},
		})
	end

	local libconfigure = compiler:link_static_library{
		name = 'configure',
		sources = fs:rglob("src/configure", "*.cpp"),
		include_directories = include_directories,
		library_directories = library_directories,
		libraries = libs,
		coverage = with_coverage,
		defines = {
			{'CONFIGURE_VERSION_STRING', '\"' .. build_type .. '-' .. version .. '\"'}
		}
	}

	local configure_exe = compiler:link_executable{
		name = "configure",
		sources = {'src/main.cpp'},
		libraries = table.extend({libconfigure}, libs),
		include_directories = include_directories,
		library_directories = library_directories,
		coverage = with_coverage,
		runtime = 'static'
	}

	local test_libs = table.extend({libconfigure}, libs)
	local defines = {}
	if build:host():os() ~= Platform.OS.windows then
		table.append(
			test_libs,
			cxx.Library:new{
				name = "boost_unit_test_framework",
				system = true,
				kind = 'shared',
				defines = {'BOOST_TEST_DYN_LINK'},
			}
		)
	end

	local test_include_directories = table.append({}, 'test/unit')
	local unit_tests = Rule:new():add_target(build:virtual_node("check"))
	for i, src in pairs(fs:rglob("test/unit", "*.cpp"))
	do
		local test_name = src:path():stem()

		local defines = {{"BOOST_TEST_MODULE", test_name},}
		if tostring(test_name) == 'process' then
			table.append(defines, "BOOST_TEST_IGNORE_SIGCHLD")
		end
		local bin = compiler:link_executable{
			name = test_name,
			directory = 'test/unit',
			sources = {src, },
			libraries = test_libs,
			defines = defines,
			include_directories = test_include_directories,
			include_files = {
				(
					build:host():os() == Platform.OS.osx and
					'boost/test/unit_test.hpp' or
					'boost/test/included/unit_test.hpp'
				),
			},
			coverage = with_coverage,
			library_directories = library_directories,
		}
		unit_tests:add_source(bin)
		unit_tests:add_shell_command(ShellCommand:new(bin))
	end
	build:add_rule(unit_tests)
end
