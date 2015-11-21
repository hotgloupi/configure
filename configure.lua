
require "package"
require "debug"

local cxx = require "configure.lang.cxx"
local c = require "configure.lang.c"
local modules = require "configure.modules"

local version = '0.0.1'

return function(build)
	build:status("Building on", build:host(), "for", build:target())

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
		local target = fs:copy(
			p,
			"share/configure/lib/configure" /
				p:relative_path(build:project_directory() / "src/lib/configure")
		)
		target:set_property("install", true)
	end

	local compiler = cxx.compiler.find{
		build = build,
		standard = 'c++11',
		optimization = (build_type == 'debug') and 'no' or 'fastest',
		debug_runtime = (build_type == 'debug'),
		debug = (build_type == 'debug'),
		runtime = 'static',
	}

	local lua = build:include({
		directory = 'lua',
		args = {compiler = compiler}
	})

	local libs = {lua,}
	local library_directories = {}
	local include_directories = {'src'}

	table.extend(libs, modules.boost.find{
		compiler = compiler,
		components = {
			'filesystem',
			'serialization',
			'iostreams',
			'exception',
			'system',
		}
	})

	if build:host():os() == Platform.OS.windows then
		table.extend(libs, {
			cxx.Library:new{name = 'Shlwapi', system = true, kind = 'static'},
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
		},
		runtime = 'static'
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
	table.extend(test_libs, modules.boost.find{
		compiler = compiler,
		components = {'unit_test_framework'},
		kind = build:target():os() == Platform.OS.windows and 'static' or 'shared',
	})
	local defines = {}

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
			install = false,
			runtime = build:host():is_windows() and 'static' or 'shared',
		}
		unit_tests:add_source(bin)
		unit_tests:add_shell_command(ShellCommand:new(bin))
	end
	build:add_rule(unit_tests)
end
