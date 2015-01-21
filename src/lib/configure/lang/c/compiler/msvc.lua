--- MSVC compiler implementation
-- @classmod configure.lang.c.compiler.msvc

local Super = require('configure.lang.c.compiler.base')
local Compiler = table.update({}, Super)

Compiler.name = 'msvc'
Compiler.binary_names = {'cl.exe', }
Compiler.lang = 'c'
Compiler._language_flag = '-TC'

function Compiler:new(args)
	o = Super.new(self, args)
	o.link_path = o:find_tool("LINK", "Link program", "link.exe")
	o.lib_path = o:find_tool("LIB", "Lib program", "lib.exe")
	return o
end

function Compiler:_build_object(args)
	command = { self.binary_path, '-nologo', self._language_flag }
	if self.exception == true then
		table.append(command, '-EHsc')
	end
	local defines = table.extend({}, args.defines)
	if self.runtime == 'static' then
		if self.threading then
			table.append(command, '-MT')
		else
			table.append(command, '-ML')
		end
	else -- dynamic runtime
		if self.threading then
			table.append(command, '-MD')
		else
			table.append(command, '-ML') -- Single threaded app
			defines.append('_DLL')       -- XXX is it enough for dynamic runtime ?
		end
	end

	for _, dir in ipairs(args.include_directories) do
		table.extend(command, {'-I', dir})
	end
	for _, define in ipairs(defines) do
		if define[2] == nil then
			table.insert(command, '-D' .. define[1])
		else
			table.insert(command, '-D' .. define[1] .. '=' .. tostring(define[2]))
		end
	end

	for _, file in ipairs(args.include_files) do
		table.extend(command, {'-FI', file})
	end

	table.extend(command, {"-c", args.source, '-Fo' .. tostring(args.target:path())})
	self.build:add_rule(
		Rule:new()
			:add_source(args.source)
			:add_target(args.target)
			:add_shell_command(ShellCommand:new(table.unpack(command)))
	)
	return args.target
end

function Compiler:_link_executable(args)
	command = {self.link_path, '-nologo',}

	for _, dir in ipairs(args.library_directories)
	do
		table.append(command, '-LIBPATH:' .. tostring(dir:path()))
	end
	library_sources = {}
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
	if args.kind == 'static' then
		table.extend(command, {
			self.lib_path, '-nologo',
			'-OUT:' .. tostring(args.target:path())
		})
		table.extend(command, args.objects)
	elseif args.kind == 'shared' then
		error("Not implemented")
	end
	self.build:add_rule(
		Rule:new()
			:add_sources(args.objects)
			:add_target(args.target)
			:add_shell_command(ShellCommand:new(table.unpack(command)))
	)
	return args.target
end

function Compiler:_library_extension(kind, ext)
	if ext then return ext end
	if kind == 'shared' then return ".dll" else return ".lib" end
end

function Compiler:_object_extension(ext)
	return ext or ".obj"
end

return Compiler
