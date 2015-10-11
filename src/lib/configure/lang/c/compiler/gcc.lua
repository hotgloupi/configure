--- GCC compiler implementation
-- @classmod configure.lang.c.compiler.gcc
local BaseCompiler = require('configure.lang.c.compiler.base')
local tools = require('configure.tools')
local Compiler = table.update(
	{},
	BaseCompiler
)

Compiler.name = "gcc"
Compiler.binary_names = {'gcc', }
Compiler.lang = 'c'

function Compiler:init()
	BaseCompiler.init(self)
	self.ar = self:find_tool("AR", "ar program", "ar")
	self.ranlib = self:find_tool("RANLIB", "ranlib program", "ranlib")
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
	local command = {self.binary}

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

	local defines = {}
	for _, define in ipairs(args.defines) do
		if define[2] == nil then
			table.append(defines, '-D' .. define[1])
		else
			table.append(defines, '-D' .. define[1] .. '=' .. tostring(define[2]))
		end
	end
	table.extend(command, tools.unique(defines))

	for _, file in ipairs(args.include_files) do
		table.extend(command, {'-include', file})
	end
	table.extend(command, {"-c", args.source, '-o', args.target})
	self.build:add_rule(
		Rule:new()
			:add_source(args.source)
			:add_sources(args.install_nodes)
			:add_target(args.target)
			:add_shell_command(ShellCommand:new(table.unpack(command)))
	)
	return args.target
end

function Compiler:_add_rpath_flag(cmd, args)
	local shared_library_files = {}
	for _, lib in ipairs(args.libraries) do
		if lib.kind == 'shared' then
			table.extend(shared_library_files, lib.files)
		else
			assert(lib.kind == 'static')
		end
	end
	shared_library_files = tools.normalize_files(self.build, shared_library_files)
	local dirs = {}
	local build_dir = tostring(self.build:directory())
	local target_dir = args.target:path():parent_path()
	for _, file in ipairs(shared_library_files) do
		if tostring(file:path()):starts_with(build_dir) then
			table.append(dirs, file:relative_path(target_dir):parent_path())
		end
	end
	dirs = tools.unique(dirs)

	table.extend(cmd, self:_gen_rpath_flags(dirs))
end

function Compiler:_gen_rpath_flags(dirs)
	local rpath = ''
	for _, dir in ipairs(dirs) do
		if rpath ~= '' then rpath = rpath .. ':' end
		rpath = rpath .. tostring('$ORIGIN' / dir)
	end
	if rpath ~= '' then
		return {'-Wl,-rpath,' .. rpath}
	else
		return {}
	end
end

function Compiler:_add_unresolved_symbols_policy_flag(cmd, args)
	if args.allow_unresolved_symbols then
		table.append(cmd, '-Wl,-undefined=dynamic_lookup')
	end
end

-- Generic linker flags generation
function Compiler:_add_linker_flags(cmd, args, sources)
	self:_add_standard_flag(cmd, args)
	self:_add_standard_library_flag(cmd, args)
	self:_add_coverage_flag(cmd, args)
	self:_add_optimization_flag(cmd, args)
	self:_add_unresolved_symbols_policy_flag(cmd, args)
	self:_add_rpath_flag(cmd, args)
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
		if args.threading then
			table.append(cmd, '-pthread')
		end
		if args.runtime == 'static' then
			table.append(cmd, '-static')
		elseif args.runtime == 'shared' then
			table.append(cmd, '-Wl,-Bdynamic')
		else
			self.build:error("Invalid value for the runtime argument: " .. tostring(args.runtime))
		end
	else
		if args.runtime == 'static' then
			self.build:warning("Static runtime linking is not supported on OS X")
		end
	end
end

function Compiler:_link_executable(args)
	local command = {self.binary, }

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
				:add_shell_command(ShellCommand:new(self.ar, 'rcs', args.target, table.unpack(args.objects)))
				:add_shell_command(ShellCommand:new(self.ranlib, args.target))
		)
	else
		assert(args.kind == 'shared')
		local command = { self.binary,}
		if self.build:host():os() == Platform.OS.osx then
			table.extend(
				command,
				{'-dynamiclib', '-Wl,-install_name,' .. tostring(args.target:path())}
			)
		else
			table.extend(
				command,
				{'-shared', '-Wl,-soname,' .. tostring(args.target:path())}
			)
		end
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
	end
	return args.target
end

function Compiler:_system_include_directories()
	local out = Process:check_output(
		{self.binary, '-E', '-x', 'c++', '-', '-v'},
		{
			stdin = Process.Stream.DEVNULL,
			stderr = Process.Stream.PIPE,
			stdout = Process.Stream.DEVNULL,
			ignore_errors = true,
		}
	)
	local res = {}
	local include_started = false
	for _, line in ipairs(out:split('\n')) do
		if line:starts_with('#include') then
			include_started = true
		elseif line:starts_with("End of search list.") then
			break
		elseif include_started then
			line = line:strip()
			if line:starts_with('/') then
				local path = Path:new(line)
				if path:is_directory() then
					self.build:debug("Found", self.binary_path, "system include dir:", path)
					table.append(res, path)
				end
			end
		end
	end
	return res
end



function Compiler:_system_library_directories()
	local cmd = {self.binary,  '-Xlinker', '--verbose'}
	if self.name == 'clang' then
		if self.build:host():os() == Platform.OS.osx then
			cmd = { self.binary, '-Xlinker', '-v' }
		else
			cmd = { self.binary, '-Xlinker', '--verbose' }
		end
	end
	self:_add_language_flag(cmd)
	table.append(cmd, '/dev/null')
	local out = Process:check_output(
		cmd,
		{
			stdin = Process.Stream.DEVNULL,
			stderr = Process.Stream.PIPE,
			stdout = Process.Stream.PIPE,
			ignore_errors = true,
		}
	)

	local library_directories = {}
	if self.build:host():os() == Platform.OS.osx then
		local current = 'none'
		for _, line in ipairs(out:split('\n')) do
			if line:starts_with("Library search paths:") then
				current = 'libraries'
			elseif line:starts_with("Framework search paths:") then
				current = 'frameworks'
			else
				if current == 'libraries' then
					line = line:strip()
					if line:starts_with('/') then
						local path = Path:new(line)
						if path:is_directory() then
							self.build:debug("Found", self.binary_path, "system library dir:", path)
							table.append(library_directories, path)
						end
					end
				end
			end
		end
	else
		for _, line in ipairs(out:split('\n')) do
			if line:find("SEARCH_DIR") ~= nil then
				for p in line:gmatch('".-"') do
					p = Path:new(p:strip('"='))
					if p:is_directory() then
						table.append(library_directories, p)
					end
				end
			end
		end
	end
	self.build:debug("Found system library directories:",
	                 table.tostring(library_directories))
	return library_directories
end

return Compiler
