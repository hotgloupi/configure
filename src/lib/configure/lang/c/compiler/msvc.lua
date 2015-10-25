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
		shared_library_directory = Super.optional_args.executable_directory
	}
)

function Compiler:new(args)
	local o = Super.new(self, args)
	o.link_path = o:find_tool("LINK", "Link program", "link.exe")
	o.lib_path = o:find_tool("LIB", "Lib program", "lib.exe")
	return o
end

Compiler._optimization_flags = {
	no = {"-Od", },
	yes = {"-Ox", },
	size = {"-O1", },
	harder = {"-O2", },
	fastest = {"-O2", },
}

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
	local defines = table.extend({}, args.defines)
	if args.runtime == 'static' then
		if args.threading then
			table.append(command, '-MT')
		else
			table.append(command, '-ML')
		end
	else -- dynamic runtime
		if args.threading then
			table.append(command, '-MD')
		else
			table.append(command, '-ML') -- Single threaded app
			defines.append({'_DLL'})       -- XXX is it enough for dynamic runtime ?
		end
	end

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

	table.extend(command, {"-c", args.source, '-Fo' .. tostring(args.target:path())})
	self.build:add_rule(
		Rule:new()
			:add_source(args.source)
			:add_sources(args.install_nodes)
			:add_target(args.target)
			:add_shell_command(ShellCommand:new(table.unpack(command)))
	)
	return args.target
end

function Compiler:_link_executable(args)
	local command = {self.link_path, '-nologo',}

	if args.debug then
		table.append(command, '-DEBUG')
	end
	if args.coverage then
		table.append(command, '-Profile')
	end

	for _, dir in ipairs(args.library_directories)
	do
		table.append(command, '-LIBPATH:' .. tostring(dir:path()))
	end
	local library_sources = {}
	for _, lib in ipairs(args.libraries)
	do
		if lib.system then
			if lib.kind == 'static' then
				table.append(command, lib.name .. '.lib')
			elseif lib.kind == 'shared' then
				table.append(command, lib.name .. '.lib')
			end
		else
			for _, file in ipairs(lib.files) do
				local path = file
				if getmetatable(file) == Node then
					table.append(library_sources, file)
					path = file:path()
				end
				table.append(command, '-LIBPATH:' .. tostring(path:parent_path()))
				table.append(command, path:filename())
			end
		end
	end
	table.extend(command, args.objects)
	table.append(command, "-OUT:" .. tostring(args.target:path()))
	self.build:add_rule(
		Rule:new()
			:add_sources(args.objects)
			:add_sources(library_sources)
			:add_target(args.target)
			:add_shell_command(ShellCommand:new(table.unpack(command)))
	)
	return args.target
end

function Compiler:_link_library(args)
	local command = {}
	local linker_lib = nil
	local rule = Rule:new():add_sources(args.objects):add_target(args.target)
	if args.kind == 'shared' then
		linker_lib = self.build:target_node(
			args.import_library_directory / (args.target:path():stem() + ".lib")
		)
		rule:add_target(linker_lib)
		table.extend(command, {self.link_path, '-DLL', '-IMPLIB:' .. tostring(linker_lib:path())})
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
	table.extend(command, args.objects)

	self.build:add_rule(
		rule:add_shell_command(ShellCommand:new(table.unpack(command)))
	)
	return linker_lib
end

function Compiler:_library_extension(kind, ext)
	if ext then return ext end
	if kind == 'shared' then return ".dll" else return ".lib" end
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
