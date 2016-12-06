--- Common C family languages compiler interface
--
-- Arguments given to the constructor `new` are used as _defaults_ in other
-- methods. For example, if the `standard` given is `c11`, it will be used
-- unless specified otherwise. However, for arguments of type list (like
-- `include_directories`), the default value is concatenated to the given one.
--
-- For example, a compiler constructed with `defines = {{'A', 1}, {'B', 2}`, when
-- the method `link_executable` is called with `defines = {'B', 3}`, the sources
-- will be compiled with `A=1` and `B=3`.
--
-- Accepted arguments:
-- ---
--
--  `env_name`
--  : The environment name used to store the compiler binary path (defaults
--  to CC or CXX for C/C++ compilers)
--
--  `coverage`
--  : Ask the compiler to generate coverage infomation (default `false`).
--
--  `exception`
--  : Enable or disable explicitly exception (default to `nil`).
--
--  `runtime`
--  : Select the runtime kind ('static' or 'shared', defaults to 'shared').
--
--  `debug_runtime`
--  : Select the debug version of the runtime (default to `false`).
--
--  `threading`
--  : Enable or disable threading support (defaults to true).
--
--  `debug`
--  : Enable debug informations (defaults to true).
--
--  `warnings`
--  : Enable warnings (defaults to true).
--
--  `defines`
--  : A list of defines. Elements can be a pair `{key, value}` or a `string`
--  (defaults to an empty list).
--
--  `executable_directory`
--  : Directory where executable are generated (default to `bin/`).
--
--  `include_directories`
--  : A list of includes directories, elements can be of type `string`, `Node`
--  or `Path` (defaults to an empty list).
--
--  `library_directories`
--  : A list of library directories, elements can be of type `string`, `Node`
--  or `Path` (defaults to an empty list).
--
--  `include_files`
--  : A list of headers to include on the command line (defaults to an empty
--  list).
--
--  `libraries`
--  : A list of `Library` instances to link against (default to an empty list).
--
--  `export\_libraries`
--  : A list of `Library` instance to link against and re-export (defaults to
--  an empty list).
--
--  `standard`
--  : The standard used to compile. Accepted values depends on the language
--    used: `c89`, `c99` or `c11` are valid values for a C compiler.
--
--  `object_directory`
--  : Directory where objects are generated (default to the relative path of
--  the source file being compiled).
--
--  `static\_library_directory`
--  : Directory where static libraries are generated (default to `lib/`).
--
--  `shared\_library_directory`
--  : Directory where shared libraries are generated (default to `bin/` on
--  Windows, `lib/` on other OSes).
--
-- `standard\_library`
-- : Applicable to languages that support alternate standard libraries (C++)
--
-- `optimization`
-- : Specify the optimization level (defaults to "no") can be one of:
--    - "size": Small program size
--    - "no": No optimization is made
--    - "yes": Safe optimization level
--    - "harder": Try harder to make your code fast
--    - "fastest": Tell the compiler to do its best (ignoring code size).
--
-- `allow_unresolved_symbols`
-- : Whether or not ignore unresolved symbols (defaults to false)
--
-- `export_dynamic`
-- : Exported symbols are exposed in the dynamic symbol table (default to false).
--
-- `big_object`
-- : Enable big object file generation (Increase the number of sections per
-- object) (defaults to false).
--
-- @classmod configure.lang.c.compiler.base

local undefined = {}
local tools = require('configure.tools')

local M = {
	--- Library type used by the compiler.
	Library = require('configure.lang.c.Library'),

	optional_args = {
		env_name = nil,
		coverage = false,
		exception = nil,
		threading = true,
		runtime = 'shared',
		debug_runtime = false,
		debug = true,
		defines = {},
		executable_directory = 'bin',
		include_directories = {},
		library_directories = {},
		include_files = {},
		install_executable = true,
		install_shared_library = true,
		install_static_library = false,
		libraries = {},
		export_libraries = {},
		object_directory = nil,
		shared_library_directory = 'lib',
		static_library_directory = 'lib',
		standard = nil,
		warnings = true,
		standard_library = nil,
		optimization = "no",
		allow_unresolved_symbols = false,
		export_dynamic = false,
		big_object = false,
	},
}

--- Public interface
-- @section

--- Create a new compiler instance.
--
-- This methods should not be used directly, see @{configure.lang.c.compiler.find}
-- or the `find()` method related to the targeted language.
--
-- @param args
-- @param args.binary_path The binary path
-- @param args.build The build instance.
-- @param args.... see @{lang.c.base}
function M:new(args)
	assert(args.binary_path ~= nil)
	assert(args.build ~= nil)
	local o = table.update(
		table.update({}, self.optional_args),
		args
	)
	for key, value in pairs(o) do
		args.build:debug(self.name, key, '=', tostring(value))
	end
	setmetatable(o, self)
	self.__index = self
	o:init()
	return o
end



--- Initialize the compiler.
--
-- This method is called at the end of the new() method when all attributes
-- are set. A sub-class should call its parent method before doing anything.
function M:init()
	self.binary = self.build:file_node(self.binary_path)
	--self.binary:set_lazy_property(
	--	"version",
	--	self._find_compiler_version,
	--	self
	--)
end


function M:_find_compiler_version()
end


--- Link an executable.
--
-- @param args
-- @param args.name The target executable name.
-- @param args.sources Sources to be compiled.
-- @param args.... see @{lang.c.base}
-- @return The target executable
function M:link_executable(args)
	local standard = args.standard or self.standard
	local standard_library = args.standard_library or self.standard_library
	local directory = Path:new(args.directory or self.executable_directory)
	local libraries = self:_libraries(args)
	local target = self:_link_executable{
		objects = self:_build_objects(args),
		target = self.build:target_node(
			(directory / args.name) + self:_executable_extension(args.extension)
		),
		standard = standard,
		standard_library = standard_library,
		libraries = libraries,
		export_libraries = self:_export_libraries(args),
		library_directories = self:_library_directories(args),
		coverage = self:_coverage(args),
		threading = self:_threading(args),
		debug = self:_debug(args),
		debug_runtime = self:_debug_runtime(args),
		exception = self:_exception(args),
		warnings = self:_warnings(args),
		optimization = self:_optimization(args),
		allow_unresolved_symbols = self:_allow_unresolved_symbols(args),
		export_dynamic = self:_export_dynamic(args),
		runtime = self:_runtime(args),
	}

	if self:_install_executable(args) then
		target:set_property("install", true)
		for _, dep in ipairs(libraries) do
			self:_set_install_property(dep)
		end
	end
	self.build:add_rule(Rule:new():add_target(target))
	return target
end

--- Link a static library.
--
-- @param args Same as @{link_library}
function M:link_static_library(args)
	args.kind = 'static'
	return self:link_library(args)
end

--- Link a shared library.
--
-- @param args Same as @{link_library}
function M:link_shared_library(args)
	args.kind = 'shared'
	return self:link_library(args)
end

--- Link a library.
--
-- @param args
-- @param args.name The target library name.
-- @param args.kind 'shared' or 'static'
-- @param args.sources Sources to be compiled.
-- @param args.filename_prefix Prefix used (OS dependent)
-- @param args.extension Extension of the library (Compiler dependant).
-- @param args.public_defines Preprocessor definition of the resulting library.
-- @param args.import_library_directory Where to generate the import library
--             when compiling a shared library on windows (defaults to the static library directory)
-- @param args.... see @{lang.c.base}
-- @return A @{Library} instance
function M:link_library(args)
	assert(args.kind == 'shared' or args.kind == 'static')
	local standard = args.standard or self.standard
	local standard_library = args.standard_library or self.standard_library
	local directory = Path:new(args.directory or (
		args.kind == 'shared'
		and self.shared_library_directory
		or self.static_library_directory
	))
	local prefix = self:_library_filename_prefix(args.kind, args.filename_prefix)
	local runtime = self:_runtime(args)
	local libraries = self:_libraries(args)

	local lib = self:_link_library{
		name = args.name,
		objects = self:_build_objects(args),
		target = self.build:target_node(
			(directory / (prefix .. args.name)) +
			self:_library_extension(args.kind, args.extension, true)
		),
		kind = args.kind,
		standard = standard,
		standard_library = standard_library,
		import_library_directory = args.import_library_directory or self.static_library_directory,
		libraries = libraries,
		export_libraries = self:_export_libraries(args),
		library_directories = self:_library_directories(args),
		coverage = self:_coverage(args),
		threading = self:_threading(args),
		debug = self:_debug(args),
		debug_runtime = self:_debug_runtime(args),
		exception = self:_exception(args),
		warnings = self:_warnings(args),
		optimization = self:_optimization(args),
		allow_unresolved_symbols = self:_allow_unresolved_symbols(args),
		export_dynamic = self:_export_dynamic(args),
		runtime = runtime,
	}

	table.extend(lib.defines, args.public_defines or {})
	table.extend(lib.include_directories, self:_include_directories(args))
	lib.runtime = args.runtime
	lib.kind = args.kind
	lib.install_node = args.install_node
	lib.libraries = libraries
	if self:_install_library(args, args.kind) then
		self:_set_install_property(lib)
		for _, dep in ipairs(libraries) do
			self:_set_install_property(dep)
		end
	end
	return lib
end

function M:_set_install_property(lib)
	if lib.kind == 'shared' and #lib.runtime_files == 0 then
		for _, file in ipairs(lib.files) do
			file:set_property("install", true)
		end
	else
		for _, file in ipairs(lib.runtime_files) do
			file:set_property("install", true)
		end
	end
	for _, dep in ipairs(lib.dependencies) do
		self:_set_install_property(lib)
	end
end

--- Check if a binary refers to this compiler instance.
--
-- @param build Current build instance
-- @param path Absolute path to the executable.
-- @return true if an instance of this compiler could be created with this
-- binary path.
function M:usable_with_binary(build, path)
	for _, binary_name in ipairs(self.binary_names) do
		if tostring(path:filename()):starts_with(binary_name) then return true end
	end
	return false
end

--- Canonical library filename
--
-- @string name
-- @string kind
-- @treturn Path the filename
function M:canonical_library_filename(name, kind)
	return 'lib' .. name ..  self:_library_extension(kind)
end


--- Find system library from name and kind
--
-- @string name
-- @string kind
-- @treturn Library
function M:find_system_library(name, kind)
	local file = self:find_system_library_file(name, kind)
	return self.Library:new{
		name = name,
		kind = self:library_kind_from_filename(file),
		files = {file},
	}
end

function M:find_system_library_from_filename(filename, kind)
	local file = self:find_system_library_file_from_filename(filename)
	return self.Library:new{
		name = tostring(file:path():stem()),
		kind = self:library_kind_from_filename(file, kind),
		files = {file},
	}
end

function M:library_kind_from_filename(file, kind)
	if kind ~= nil then return kind end
	local ext = tostring(tools.path(file):ext())
	if ext == '.a' then return 'static' end
	if ext == '.so' or ext == '.dylib' then return 'shared' end
	self.build:error("Cannot deduce the kind of '" .. tostring(file) .. "'")
end

--- Find system library file from name and kind
--
-- @string name
-- @string kind
-- @treturn Node
function M:find_system_library_file(name, kind)
	local try_kinds = {}
	if kind == nil then
		try_kinds = {'static', 'shared'}
	else
		try_kinds = {kind}
	end
	local library_directories = self:system_library_directories()
	for _, kind in ipairs(try_kinds) do
		local filename = self:canonical_library_filename(name, kind)
		for _, dir in ipairs(library_directories) do
			local file = dir / filename
			if file:exists() then return self.build:file_node(file) end
		end
	end
	self.build:error("Couldn't find library '" .. name .. "'")
end

function M:find_system_library_file_from_filename(filename)
	local library_directories = self:system_library_directories()
	for _, dir in ipairs(library_directories) do
		local file = dir / filename
		if file:exists() then return self.build:file_node(file) end
	end
	self.build:error("Couldn't find library '" .. filename .. "'")
end


--- Try to compile some source code
function M:try_build_object(name, content, args)
	return self.binary:set_cached_property(
		"check-" .. name,
		function()
			args = self:_normalize_build_object_args(args)

			local dir = TemporaryDirectory:new()
			args.source = dir:path() / (name .. '.c')
			args.target = args.source + '.o'

			local f = assert(io.open(tostring(args.source) , 'w'))
			f:write(content)
			f:close()

			local commands = self:_build_object(args).commands
			for _, cmd in ipairs(commands) do
				if Process:call(cmd) ~= 0 then return false end
			end
			return true
		end
	)
end

function M:has_include(name, args)
	local check_name = name:gsub('/', '-'):gsub('%.', '-'):gsub('\\', '-')
	return self:try_build_object(
		"has-include-" .. check_name,
		"#include <" .. name .. ">",
		args or {}
	)
end

-------------------------------------------------------------------------------
--- Private methods
--
-- Utilities used internally.
-- @section

function M:_normalize_build_object_args(args)
	local res = table.update({}, args)
	res.object_directory = Path:new(args.object_directory or self.object_directory)
	res.object_extension = self:_object_extension(args.object_extension)
	res.include_directories = self:_include_directories(args)
	res.include_files = self:_include_files(args)
	res.install_nodes = self:_install_nodes(args)
	res.defines = self:_defines(args)
	res.standard = args.standard or self.standard
	res.standard_library = args.standard_library or self.standard_library
	res.coverage = self:_coverage(args)
	res.threading = self:_threading(args)
	res.debug = self:_debug(args)
	res.debug_runtime = self:_debug_runtime(args)
	res.exception = self:_exception(args)
	res.warnings = self:_warnings(args)
	res.optimization = self:_optimization(args)
	res.big_object = self:_big_object(args)
	res.runtime = self:_runtime(args)
	return res
end

--- Compile source files into objects
--
-- @param args see @{configure.lang.c.base}
-- @return The list of objects
function M:_build_objects(args)
	args = self:_normalize_build_object_args(args)
	local objects = {}
	for idx, source in ipairs(args.sources) do
		if getmetatable(source) ~= Node then
			source = self.build:source_node(Path:new(source))
		end
		source:set_property('language', self.lang)
		source:set_property('include_directories', args.include_directories)

		local res = self:_build_object(
			table.update({
				source = source:path(),
				target = self.build:target_node(
					args.object_directory / (
						source:relative_path(self.build:project_directory()) +
						args.object_extension
					)
				):path(),
			}, args)
		)
		objects[idx] = self.build:target_node(res.targets[1])

		local rule = Rule:new()
			:add_sources(tools.normalize_files(self.build, res.sources))
			:add_targets(tools.normalize_files(self.build, res.targets))
		for _, cmd in ipairs(res.commands) do
			rule:add_shell_command(ShellCommand:new(table.unpack(cmd)))
		end
		self.build:add_rule(rule)
	end
	return objects
end

--- Concat and normalize include directories from argument, libraries
--  arguments compiler and compiler libraries.
--
-- @param args
-- @param args.include_directories
function M:_include_directories(args)
	local dirs = {}
	for _, list in ipairs({
		{args}, args.libraries or {}, args.export_libraries or {},
		{self}, self.libraries or {}, self.export_libraries or {}}) do
		for _, obj in ipairs(list) do
			table.extend(dirs, obj.include_directories or {})
		end
	end
	return tools.unique(tools.normalize_directories(self.build, dirs))
end

--- Concat all the install nodes found in libraries
function M:_install_nodes(args)
	local install_nodes = {}
	for _, list in ipairs({
		args.libraries or {}, args.export_libraries or {},
		self.libraries or {}, self.export_libraries or {}}) do
		for _, lib in ipairs(list) do
			if lib.install_node ~= nil then
				table.append(install_nodes, lib.install_node)
			end
		end
	end
	return tools.unique(install_nodes)
end

--- Concat and normalize library directories from argument and compiler.
--
-- @param args
-- @param args.library_directories
function M:_library_directories(args)
	local dirs = {}
	table.extend(dirs, args.library_directories or {})
	table.extend(dirs, self.library_directories)
	return tools.unique(tools.normalize_directories(self.build, dirs))
end


local function concat_libraries(libs)
	local res = {}
	for _, lib in ipairs(libs) do
		table.append(res, lib)
		table.extend(res, concat_libraries(lib.libraries or {}))
	end
	return res
end

--- Concat arguments libraries and compiler libraries
--
-- @param args
-- @param args.libraries
-- @return A list of libraries
function M:_libraries(args)
	local libs = {}
	table.extend(libs, self.libraries)
	table.extend(libs, args.libraries or {})
	return tools.unique(concat_libraries(libs))
end

--- Concat arguments export libraries and compiler export libraries
--
-- @param args
-- @param[opt] args.libraries
-- @return A list of libraries
function M:_export_libraries(args)
	local libs = {}
	table.extend(libs, self.export_libraries)
	table.extend(libs, args.export_libraries or {})
	return libs
end

--- Concat and normalize defines
-- @param args
-- @param args.defines
-- @return A list of pairs `{key, value}` where value is `nil` or a `string`.
function M:_defines(args)
	local res = {}
	for _, list in ipairs({
		self.libraries or {}, self.export_libraries or {}, {self},
		args.libraries or {}, args.export_libraries or {}, {args},
	}) do
		for _, obj in ipairs(list) do
			table.extend(res, obj.defines or {})
		end
	end
	for i, v in ipairs(res)
	do
		if type(v) ~= 'table' then v = {v, nil} end
		res[i] = v
	end
	return res
end

--- Concat include files.
--
-- @param args
-- @param args.include_files
-- @return a list of include files
function M:_include_files(args)
	local res = {}
	table.extend(res, self.include_files)
	table.extend(res, args.include_files or {})
	return res
end

--- Coverage state
--
-- @param args
-- @tparam[opt=false] bool args.coverage
-- @treturn bool Coverage state
function M:_coverage(args)
	if args.coverage == nil then return self.coverage end
	return args.coverage
end

--- Threading state
--
-- @param args
-- @tparam bool args.threading
-- @treturn bool
function M:_threading(args)
	if args.threading == nil then
		return self.threading
	else
		return args.threading
	end
end

--- debug state
--
-- @param args
-- @tparam bool args.debug
-- @treturn bool
function M:_debug(args)
	if args.debug == nil then
		return self.debug
	else
		return args.debug
	end
end

--- debug runtime state
--
-- @param args
-- @tparam[opt] bool args.debug_runtime
-- @treturn bool
function M:_debug_runtime(args)
	if args.debug_runtime == nil then
		return self.debug_runtime
	else
		return args.debug_runtime
	end
end


--- Exception state
--
-- @param args
-- @tparam bool args.exception
-- @treturn bool
function M:_exception(args)
	if args.exception == nil then
		return self.exception
	else
		return args.exception
	end
end

--- Big object
--
-- @param args
-- @tparam[opt] bool args.big_object
-- @treturn bool
function M:_big_object(args)
	if args.big_object == nil then
		return self.big_object
	else
		return args.big_object
	end
end

--- warnings state
--
-- @param args
-- @tparam bool args.warnings
-- @treturn bool
function M:_warnings(args)
	if args.warnings == nil then
		return self.warnings
	else
		return args.warnings
	end
end

--- Install state
function M:_install_library(args, kind)
	if args.install == nil then
		return self['install_' .. kind .. '_library']
	else
		return args.install
	end
end

function M:_install_executable(args)
	if args.install == nil then
		return self.install_executable
	else
		return args.install
	end
end

--- Optiomization level.
--
-- @param args
-- @tparam string args.optimization
-- @treturn string
function M:_optimization(args)
	local lvl
	if args.optimization then
		lvl = args.optimization
	else
		lvl = self.optimization
	end
	return lvl
end

--- Unresolved symbols policy
-- @param args
-- @tparam[opt] bool args.allow_unresolved_symbols
-- @treturn bool
function M:_allow_unresolved_symbols(args)
	if args.allow_unresolved_symbols ~= nil then
		return args.allow_unresolved_symbols
	else
		return self.allow_unresolved_symbols
	end
end

--- Runtime link mode
--
-- @param args
-- @tparam string args.runtime
-- @treturn string 'shared' or 'static'
function M:_runtime(args)
	return args.runtime or self.runtime
end

--- Export dynamic switch
--
-- @param args
-- @tparam[opt] bool args.export_dynamic
-- @treturn bool
function M:_export_dynamic(args)
	local res = args.export_dynamic
	if res == nil then
		res = self.export_dynamic
	end
	return res
end

--- Built objects extension
--
-- @param ext Extension or nil
function M:_object_extension(ext)
	return ext or ".o"
end

--- Built executable extension
--
-- @param ext Extension or nil
function M:_executable_extension(ext)
	if self.build:target():os() == Platform.OS.windows then
		return ext or ".exe"
	else
		return ext or ''
	end
end

function M:_library_extension(kind, ext)
	if ext ~= nil then return ext end
	if kind == 'shared' then
		if self.build:target():os() == Platform.OS.osx then
			return ".dylib"
		else
			return ".so"
		end
	else
		return ".a"
	end
end

function M:_library_filename_prefix(kind, prefix)
	if prefix ~= nil then return prefix end
	return 'lib'
end

--- Find a program and add it as an option.
--
-- @string var_name The name of the option variable
-- @string description Description of the tool
-- @string default_value Default program name
-- @treturn Path Full path to the binary
function M:find_tool(var_name, description, default_value)
	local path = self.build:path_option(var_name, description, Path:new(default_value))
	if not path:is_absolute() then
		path = self.build:fs():which(path)
		if not path then
			error(
				"Couldn't find " .. default_value ..
				" program (try to set explicitly " .. var_name .. ")"
			)
		end
		self.build:debug("Found", var_name, "at", path)
		self.build:env():set(var_name, path)
	end
	return self.build:file_node(path)
end

--- System include directories used by the compiler implicitly
--
-- @return A list of directories
function M:system_include_directories()
	return self.binary:set_cached_property(
		self.env_name .. "-system-include-directories",
		function () return self:_system_include_directories() end
	)
end

--- System library directories used by the compiler implicitly
--
-- @return A list of directories
function M:system_library_directories()
	return self.binary:set_cached_property(
		self.env_name .. "-system-library-directories",
		function () return self:_system_library_directories() end
	)
end

--- Implementation API methods
--
-- These methods are implemented in final classes.
-- @section

--- Compile a source to an object.
--
-- @param args
-- @param args.source Source Node
-- @param args.target Target Node
-- @param args.include_directories A list of include directories.
-- @param args.defines A list of defines
-- @param args.standard
-- @param args.coverage Coverage state
function M:_build_object(args)
	error("Not implemented")
end

--- Link objects to an executable.
--
-- @param args
-- @tparam table args.objects Objects to link together
-- @tparam Node args.target Target executable node
-- @string args.standard The C standard used
-- @param args.coverage Coverage state
function M:_link_executable(args)
	error("Not implemented")
end

--- Link objects to a library.
--
-- @param args
-- @param args.kind 'shared' or 'static'
-- @param args.objects Objects to link together
-- @param args.target Target library node
-- @param args.standard
-- @param args.libraries List of libraries
-- @param args.coverage Coverage state
function M:_link_library(args)
	error("Not implemented")
end

function M:_system_include_directories()
	error("Not implemented")
end

function M:_system_library_directories()
	error("Not implemented")
end

return M
