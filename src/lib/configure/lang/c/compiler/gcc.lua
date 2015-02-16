--- GCC compiler implementation
-- @classmod configure.lang.c.compiler.gcc
local BaseCompiler = require('configure.lang.c.compiler.base')
local Compiler = table.update(
	{},
	BaseCompiler
)

Compiler.name = "gcc"
Compiler.binary_names = {'gcc', }
Compiler.lang = 'c'

function Compiler:new(args)
	o = BaseCompiler.new(self, args)
	o.ar_path = o:find_tool("AR", "ar program", "ar")
	o.ranlib_path = o:find_tool("RANLIB", "ranlib program", "ranlib")
	return o
end

function Compiler:_build_object(args)
	command = {self.binary_path, '-x', self.lang}
	if args.standard then table.insert(command, '-std=' .. args.standard) end
	if args.standard_library then
		table.insert(command, '-stdlib=' .. args.standard_library)
	end
	if args.coverage then table.insert(command, '--coverage') end
	if args.debug then table.insert(command, '-g') end
	if args.warnings then table.extend(command, {'-Wall', '-Wextra'}) end
	for _, dir in ipairs(args.include_directories) do
		table.extend(command, {'-I', dir})
	end
	for _, define in ipairs(args.defines) do
		if define[2] == nil then
			table.insert(command, '-D' .. define[1])
		else
			table.insert(command, '-D' .. define[1] .. '=' .. tostring(define[2]))
		end
	end
	for _, file in ipairs(args.include_files) do
		table.extend(command, {'-include', file})
	end
	table.extend(command, {"-c", args.source, '-o', args.target})
	self.build:add_rule(
		Rule:new()
			:add_source(args.source)
			:add_target(args.target)
			:add_shell_command(ShellCommand:new(table.unpack(command)))
	)
	return args.target
end

-- Generic linker flags generation
function Compiler:_linker_flags(args, sources)
	local res = {}
	for _, dir in ipairs(args.library_directories)
	do
		table.extend(res, {'-L', dir})
	end

	for _, lib in ipairs(args.libraries)
	do
		if lib.system then
			if self.build:host():os() ~= Platform.OS.osx then
				if lib.kind == 'static' then
					table.append(res, '-Wl,-Bstatic')
				end
				if lib.kind == 'shared' then
					table.append(res, '-Wl,-Bdynamic')
				end
			end
			table.append(res, "-l" .. lib.name)
		else
			for _, file in ipairs(lib.files) do
				table.append(res, file)
				if getmetatable(file) == Node then
					table.append(sources, file)
				end
			end
		end
	end
	if self.build:host():os() ~= Platform.OS.osx then
		table.append(res, '-Wl,-Bdynamic')
	end
	return res
end

function Compiler:_link_executable(args)
	command = {self.binary_path, }
	if args.standard then table.insert(command, '-std=' .. args.standard) end
	if args.standard_library then
		table.insert(command, '-stdlib=' .. args.standard_library)
	end
	if args.coverage then table.insert(command, '--coverage') end
	table.extend(command, args.objects)

	local sources = {}
	table.extend(command, self:_linker_flags(args, sources))

	table.extend(command, {"-o", args.target})
	self.build:add_rule(
		Rule:new()
			:add_sources(args.objects)
			:add_sources(sources)
			:add_target(args.target)
			:add_shell_command(ShellCommand:new(table.unpack(command)))
	)
	return args.target
end

function Compiler:_link_library(args)
	if args.kind == 'static' then
		self.build:add_rule(
			Rule:new()
				:add_sources(args.objects)
				:add_target(args.target)
				:add_shell_command(ShellCommand:new(self.ar_path, 'rcs', args.target, table.unpack(args.objects)))
				:add_shell_command(ShellCommand:new(self.ranlib_path, args.target))
		)
	else
		assert(args.kind == 'shared')
		error("NOT IMPLEMENTED")
	end
	return args.target
end

return Compiler
