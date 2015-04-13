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
--  : Enable or disable explicitly exception (default `nil`).
--
--  `runtime`
--  : Select the runtime kind ('static' or 'shared', defaults to 'shared').
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
-- @classmod configure.lang.c.compiler.base

local undefined = {}

local M = {
	--- Library type used by the compiler.
	Library = require('configure.lang.c.Library'),

	optional_args = {
		env_name = nil,
		coverage = false,
		exception = nil,
		threading = true,
		runtime = 'shared',
		debug = true,
		defines = {},
		executable_directory = 'bin',
		include_directories = {},
		library_directories = {},
		include_files = {},
		libraries = {},
		object_directory = nil,
		shared_library_directory = 'lib',
		static_library_directory = 'lib',
		standard = nil,
		warnings = true,
		standard_library = nil,
		optimization = "no",
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
	o = table.update(
		table.update({}, self.optional_args),
		args
	)
	for key, value in pairs(o) do
		args.build:debug(self.name, key, '=', value)
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
	local libraries = self:_libraries(args)
	local directory = Path:new(args.directory or self.executable_directory)
	local target = self:_link_executable{
		objects = self:_build_objects(args),
		target = self.build:target_node(
			(directory / args.name) + self:_executable_extension(args.extension)
		),
		standard = standard,
		standard_library = standard_library,
		libraries = libraries,
		library_directories = self:_library_directories(args),
		coverage = self:_coverage(args),
		threading = self:_threading(args),
		debug = self:_debug(args),
		exception = self:_exception(args),
		warnings = self:_warnings(args),
		optimization = self:_optimization(args),
	}
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
-- @param[opt] args.filename_prefix Prefix used (OS dependent)
-- @param[opt] args.extension Extension of the library (Compiler dependant).
-- @param args.... see @{lang.c.base}
-- @return A @{Library} instance
function M:link_library(args)
	assert(args.kind == 'shared' or args.kind == 'static')
	local standard = args.standard or self.standard
	local standard_library = args.standard_library or self.standard_library
	local libraries = self:_libraries(args)
	local directory = Path:new(args.directory or (
		args.kind == 'shared'
		and self.shared_library_directory
		or self.static_library_directory
	))
	local prefix = self:_library_filename_prefix(args.filename_prefix)
	local target = self:_link_library{
		objects = self:_build_objects(args),
		target = self.build:target_node(
			(directory / (prefix .. args.name)) +
			self:_library_extension(args.kind, args.extension)
		),
		kind = args.kind,
		standard = standard,
		standard_library = standard_library,
		libraries = libraries,
		library_directories = self:_library_directories(args),
		coverage = self:_coverage(args),
		threading = self:_threading(args),
		debug = self:_debug(args),
		exception = self:_exception(args),
		warnings = self:_warnings(args),
		optimization = self:_optimization(args),
	}
	self.build:add_rule(Rule:new():add_target(target))
	return self.Library:new{
		name = args.name,
		system = false,
		files = {target},
		include_directories = self:_include_directories(args),
		defines = self:_defines(args),
	}
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

-------------------------------------------------------------------------------
--- Private methods
--
-- Utilities used internally.
-- @section

--- Compile source files into objects
--
-- @param args see @{configure.lang.c.base}
-- @return The list of objects
function M:_build_objects(args)
	local object_directory = Path:new(args.object_directory or self.object_directory)
	local object_extension = self:_object_extension(args.object_extension)
	local include_directories = self:_include_directories(args)
	local include_files = self:_include_files(args)
	local defines = self:_defines(args)
	local standard = args.standard or self.standard
	local standard_library = args.standard_library or self.standard_library
	local libraries = self:_libraries(args)
	local coverage = self:_coverage(args)
	local threading = self:_threading(args)
	local debug = self:_debug(args)
	local exception = self:_exception(args)
	local warnings = self:_warnings(args)
	local optimization = self:_optimization(args)
	local objects = {}
	for idx, source in ipairs(args.sources) do
		if getmetatable(source) ~= Node then
			source = self.build:source_node(Path:new(source))
		end
		source:set_property('language', self.lang)
		source:set_property('include_directories', include_directories)
		objects[idx] = self:_build_object{
			source = source,
			target = self.build:target_node(
				object_directory / (
					source:relative_path(self.build:project_directory()) +
					object_extension
				)
			),
			extension = object_extension,
			include_directories = include_directories,
			defines = defines,
			standard = standard,
			standard_library = standard_library,
			coverage = coverage,
			include_files = include_files,
			threading = threading,
			debug = debug,
			exception = exception,
			warnings = warnings,
			optimization = optimization,
		}
	end
	return objects
end

--- Concat and normalize include directories from argument, libraries
--  arguments compiler and compiler libraries.
--
-- @param args
-- @param[opt] args.include_directories
function M:_include_directories(args)
	local dirs = {}
	table.extend(dirs, args.include_directories or {})
	for _, lib in ipairs(args.libraries or {})
	do
		table.extend(dirs, lib.include_directories)
	end
	table.extend(dirs, self.include_directories)
	for _, lib in ipairs(self.libraries)
	do
		table.extend(dirs, lib.include_directories)
	end
	return self:_normalize_directories(dirs)
end
--
--- Concat and normalize library directories from argument and compiler.
--
-- @param args
-- @param[opt] args.library_directories
function M:_library_directories(args)
	local dirs = {}
	table.extend(dirs, args.library_directories or {})
	table.extend(dirs, self.library_directories)
	return self:_normalize_directories(dirs)
end

--- Concat arguments libraries and compiler libraries
--
-- @param args
-- @param[opt] args.libraries
-- @return A list of libraries
function M:_libraries(args)
	local libs = {}
	table.extend(libs, self.libraries)
	table.extend(libs, args.libraries or {})
	return libs
end

--- Concat and normalize defines
-- @param args
-- @param[opt] args.defines
-- @return A list of pairs `{key, value}` where value is `nil` or a `string`.
function M:_defines(args)
	local res = {}
	for _, lib in ipairs(self.libraries) do
		table.extend(res, lib.defines)
	end
	table.extend(res, self.defines)

	for _, lib in ipairs(args.libraries or {}) do
		table.extend(res, lib.defines)
	end
	table.extend(res, args.defines or {})
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
-- @tparam[opt] bool args.threading
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
-- @tparam[opt] bool args.debug
-- @treturn bool
function M:_debug(args)
	if args.debug == nil then
		return self.debug
	else
		return args.debug
	end
end


--- Exception state
--
-- @param args
-- @tparam[opt] bool args.exception
-- @treturn bool
function M:_exception(args)
	if args.exception == nil then
		return self.exception
	else
		return args.exception
	end
end

--- warnings state
--
-- @param args
-- @tparam[opt] bool args.warnings
-- @treturn bool
function M:_warnings(args)
	if args.warnings == nil then
		return self.warnings
	else
		return args.warnings
	end
end

--- Optiomization level.
--
-- @param args
-- @tparam[opt] string args.optimization
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

--- Convert directories to directory nodes if needed.
--
-- @param dirs a list of string, path or nodes
-- @return a list of directory `Node`s
function M:_normalize_directories(dirs)
	res = {}
	for _, dir in ipairs(dirs)
	do
		if type(dir) == "string" then
			dir = Path:new(dir)
		end
		if getmetatable(dir) == Path then
			if not dir:is_absolute() then
				dir = self.build:project_directory() / dir
			end
			dir = self.build:directory_node(dir)
		elseif getmetatable(dir) ~= Node then
			error("Expected string, Path or Node, got " .. tostring(dir))
		end
		table.append(res, dir)
	end
	return res
end

--- Built objects extension
--
-- @param[opt] ext Extension or nil
function M:_object_extension(ext)
	return ext or ".o"
end

--- Built executable extension
--
-- @param[opt] ext Extension or nil
function M:_executable_extension(ext)
	if self.build:target():os() == Platform.OS.windows then
		return ext or ".exe"
	else
		return ext or ''
	end
end

function M:_library_extension(kind, ext)
	if ext then return ext end
	if kind == 'shared' then return ".so" else return ".a" end
end

function M:_library_filename_prefix(kind, prefix)
	if prefix then return prefix end
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

function M:system_include_directories()
	return self.binary:set_cached_property(
		self.env_name .. "-system-include-directories",
		function () return self:_system_include_directories() end
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

return M
