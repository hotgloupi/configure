--- MSVC compiler implementation
-- @classmod configure.lang.c.compiler.msvc

local Super = require('configure.lang.c.compiler.base')
local tools = require('configure.tools')
local Compiler = table.update({}, Super)

Compiler.name = 'msvc'
Compiler.binary_names = {'cl.exe', }
Compiler.lang = 'c'
Compiler._language_flag = '-TC'

Compiler.optional_args = table.update(
	table.update({}, Super.optional_args),
	{
		shared_library_directory = Super.optional_args.executable_directory,
		subsystem = 'console',
		subsystem_version = nil,
		unicode = true,
		debug_runtime = true,
	}
)

function Compiler:init()
	Super.init(self)
	self.link_path = self:find_tool("LINK", "Link program", "link.exe")
	self.lib_path = self:find_tool("LIB", "Lib program", "lib.exe")
	local os_version = self.build:target():os_version()
	local ver = os_version:split(".")
	local winver = "0x0" .. ver[1] .. "0" .. ver[2]
	self.defines = table.extend({
		{'_WIN32_WINNT', winver},
		{'WINVER', winver},
		{'NTDDI_VERSION', winver .. '0000'},
	}, self.defines)
	if self.unicode then
		table.append(self.defines, '_UNICODE')
	end
	if self.subsystem_version == nil then
		self.subsystem_version = os_version
	end
end

Compiler._optimization_flags = {
	no = {"-Od", },
	yes = {"-Ox", },
	size = {"-O1", },
	harder = {"-O2", },
	fastest = {"-O2", },
}

--- Canonical library filename
--
-- @string name
-- @string kind
-- @tparam bool runtime
-- @treturn Path the filename
function Compiler:canonical_library_filename(name, kind, runtime)
	local prefix = kind == 'static' and 'lib' or ''
	return prefix .. name ..  self:_library_extension(kind, runtime)
end


function Compiler:_add_optimization_flag(cmd, args)
	table.extend(cmd, self._optimization_flags[args.optimization])
end

function Compiler:_build_object(args)
	local command = { self.binary_path, '-nologo', self._language_flag }
	self:_add_optimization_flag(command, args)
	if args.exception == true then
		table.append(command, '-EHsc')
	end
	if args.debug then
		table.append(command, '-Z7')
	end
	if args.warnings then
		table.append(command, '-W3')
	end
	if args.big_object then
		table.append(command, '-bigobj')
	end

	local defines = table.extend({}, args.defines)
	local runtime_flag = nil
	if args.runtime == 'static' then
		if args.threading then
			runtime_flag = '-MT'
		else
			runtime_flag = '-ML'
		end
	else -- dynamic runtime
		if args.threading then
			runtime_flag = '-MD'
		else
			runtime_flag = '-ML' -- Single threaded app
			defines.append({'_DLL'})       -- XXX is it enough for dynamic runtime ?
		end
	end
	if args.debug_runtime then
		runtime_flag = runtime_flag .. 'd'
	end
	table.append(command, runtime_flag)

	for _, dir in ipairs(args.include_directories) do
		table.extend(command, {'-I', dir})
	end

	local define_args = {}
	for _, define in ipairs(defines) do
		if define[2] == nil then
			table.insert(define_args, '-D' .. define[1])
		else
			table.insert(define_args, '-D' .. define[1] .. '=' .. tostring(define[2]))
		end
	end
	table.extend(command, tools.unique(define_args))

	for _, file in ipairs(args.include_files) do
		table.extend(command, {'-FI', file})
	end

	table.extend(command, {"-c", args.source, '-Fo' .. tostring(args.target)})
	return {
		sources = table.extend({args.source}, args.install_nodes),
		targets = {args.target},
		commands = {command}
	}
end

function Compiler:_add_linker_library_flags(command, args, sources)
	local link_args = {}
	for _, dir in ipairs(args.library_directories) do
		table.append(link_args, '-LIBPATH:' .. tostring(dir:path()))
	end
	for _, lib in ipairs(args.libraries) do
		if lib.system then
			if lib.kind == 'static' then
				table.append(link_args, lib.name .. '.lib')
			elseif lib.kind == 'shared' then
				table.append(link_args, lib.name .. '.lib')
			end
		else
			for _, file in ipairs(lib.files) do
				local path = file
				if getmetatable(file) == Node then
					table.append(sources, file)
					path = file:path()
				end
				table.append(link_args, '-LIBPATH:' .. tostring(path:parent_path()))
				table.append(link_args, path:filename())
			end
		end
	end
	table.extend(command, tools.unique(link_args))
end

function Compiler:_add_subsystem_flags(cmd, args)
	local subsystem = args.subsystem
	if subsystem == nil then subsystem = self.subsystem end
	if subsystem == false then return end
	local version = args.subsystem_version
	if version == nil then version = self.subsystem_version end
	if version ~= false then
		subsystem = subsystem .. ',' .. version
	end
	table.append(cmd, '-SUBSYSTEM:' .. subsystem)
end

function Compiler:_link_executable(args)
	local command = {self.link_path, '-nologo',}

	self:_add_subsystem_flags(command, args)
	if args.debug then
		table.append(command, '-DEBUG')
	end
	if args.coverage then
		table.append(command, '-Profile')
	end

	local library_sources = {}
	self:_add_linker_library_flags(command, args, library_sources)
	table.extend(command, args.objects)
	table.append(command, "-OUT:" .. tostring(args.target:path()))
	self.build:add_rule(
		Rule:new()
			:add_sources(args.objects)
			:add_sources(tools.unique(library_sources))
			:add_target(args.target)
			:add_shell_command(ShellCommand:new(table.unpack(command)))
	)
	return args.target
end

function Compiler:_link_library(args)
	local command = {}
	local linker_lib = nil
	local rule = Rule:new()
	if args.kind == 'shared' then
		linker_lib = self.build:target_node(
			args.import_library_directory / (args.target:path():stem() + ".lib")
		)
		--rule:add_target(linker_lib)
		table.extend(command, {self.link_path, '-DLL', '-IMPLIB:' .. tostring(linker_lib:path())})
		self:_add_subsystem_flags(command, args)
		if args.debug then
			table.append(command, '-DEBUG')
		end
		if args.coverage then
			table.append(command, '-Profile')
		end
	else
		table.extend(command, {self.lib_path})
		linker_lib = args.target
	end
	table.extend(command, {
		'-nologo',
		'-OUT:' .. tostring(args.target:path())
	})
	local library_sources = {}
	self:_add_linker_library_flags(command, args, library_sources)
	table.extend(command, args.objects)

	self.build:add_rule(
		rule
			:add_sources(args.objects)
			:add_sources(library_sources)
			:add_target(args.target)
			:add_shell_command(ShellCommand:new(table.unpack(command)))
	)
	return self.Library:new{
		name = args.name,
		files = {linker_lib},
		runtime_files = args.kind == 'static' and {} or {args.target},
		kind = args.kind,
	}
end

function Compiler:_library_extension(kind, ext, runtime)
	if ext then return ext end
	if runtime == true and kind == 'shared' then return '.dll' end
	return ".lib"
end

function Compiler:_object_extension(ext)
	return ext or ".obj"
end

local os = require('os')

function Compiler:_system_include_directories()
    local res = {}
    table.extend(res, os.getenv('INCLUDE'):split(';'))
    local dirs = {}
    for _, dir in ipairs(res) do
        local p = Path:new(dir)
        if p:is_directory() then
            self.build:debug('Found system include dir', p)
            table.append(dirs, p)
        end
    end
    return dirs
end

function Compiler:_system_library_directories()
    local res = {}
    table.extend(res, os.getenv('LIB'):split(';'))
    table.extend(res, os.getenv('LIBPATH'):split(';'))
    local dirs = {}
    for _, dir in ipairs(res) do
        local p = Path:new(dir)
        if p:is_directory() then
            self.build:debug('Found system library dir', p)
            table.append(dirs, p)
        else
            self.build:debug('ignore system library dir', p)
        end
    end
    return dirs
end

return Compiler
