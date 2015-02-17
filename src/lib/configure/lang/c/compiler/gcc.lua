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

Compiler._optimization_flags = {
	no = "-O0",
	yes = "-O1",
	size = "-Os",
	harder = "-O2",
	fastest = "-O3",
}

function Compiler:_add_optimization_flag(cmd, args)
	table.append(cmd, self._optimization_flags[args.optimization])
end

function Compiler:_add_language_flag(cmd, args)
	table.extend(cmd, {'-x', self.lang})
end

function Compiler:_add_standard_flag(cmd, args)
	if args.standard then table.append(cmd, '-std=' .. args.standard) end
end

function Compiler:_add_standard_library_flag(cmd, args)
	if args.standard_library then
		table.insert(cmd, '-stdlib=' .. args.standard_library)
	end
end

function Compiler:_add_coverage_flag(cmd, args)
	if args.coverage then table.append(cmd, '--coverage') end
end

function Compiler:_add_debug_flag(cmd, args)
	if args.debug then table.insert(cmd, '-g') end
end

function Compiler:_add_warnings_flag(cmd, args)
	if args.warnings then table.extend(cmd, {'-Wall', '-Wextra'}) end
end

function Compiler:_build_object(args)
	command = {self.binary_path}

	self:_add_language_flag(command, args)
	self:_add_optimization_flag(command, args)
	self:_add_standard_flag(command, args)
	self:_add_standard_library_flag(command, args)
	self:_add_coverage_flag(command, args)
	self:_add_debug_flag(command, args)
	self:_add_warnings_flag(command, args)

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
function Compiler:_add_linker_flags(cmd, args, sources)
	self:_add_standard_flag(cmd, args)
	self:_add_standard_library_flag(cmd, args)
	self:_add_coverage_flag(cmd, args)
	self:_add_optimization_flag(cmd, args)
end

function Compiler:_add_linker_library_flags(cmd, args, sources)
	for _, dir in ipairs(args.library_directories)
	do
		table.extend(cmd, {'-L', dir})
	end

	for _, lib in ipairs(args.libraries)
	do
		if lib.system then
			if self.build:host():os() ~= Platform.OS.osx then
				if lib.kind == 'static' then
					table.append(cmd, '-Wl,-Bstatic')
				end
				if lib.kind == 'shared' then
					table.append(cmd, '-Wl,-Bdynamic')
				end
			end
			table.append(cmd, "-l" .. lib.name)
		else
			for _, file in ipairs(lib.files) do
				table.append(cmd, file)
				if getmetatable(file) == Node then
					table.append(sources, file)
				end
			end
		end
	end
	if self.build:host():os() ~= Platform.OS.osx then
		table.append(cmd, '-Wl,-Bdynamic')
	end
end

function Compiler:_link_executable(args)
	command = {self.binary_path, }

	local sources = {}
	self:_add_linker_flags(command, args, sources)
	table.extend(command, args.objects)
	self:_add_linker_library_flags(command, args, sources)

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
