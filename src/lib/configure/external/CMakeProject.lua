--- @classmod configure.external.CMakeProject
local Project = require('configure.external.Project')
local CMakeProject = table.update({}, Project)

function CMakeProject:new(o)
	assert(o.compiler ~= nil)
	return Project.new(self, o)
end

function CMakeProject:_init()
	Project._init(self)
	self.cmake = self._build:fs():which('cmake')
end

function CMakeProject:configure(args)
	local default = {
		CMAKE_INSTALL_PREFIX = self:step_directory('install'),
		CMAKE_VERBOSE_MAKEFILE = true,
	}
	if self.compiler.lang == 'c' then
		default['CMAKE_C_COMPILER'] = self.compiler.binary_path
	elseif self.compiler.lang == 'c++' then
		default['CMAKE_CXX_COMPILER'] = self.compiler.binary_path
	end
	local configure_variables = table.update(default, args.variables or {})

	local vars = {}
	for k,v in pairs(configure_variables) do
		if type(v) == 'boolean' then
			k = k .. ':BOOL'
			v = v and 'YES' or 'NO'
		elseif getmetatable(v) == Path then
			k = k .. ':PATH'
			v = tostring(v)
		elseif getmetatable(v) == Node then
			k = k .. ':PATH'
			v = tostring(v:path())
		end
		table.append(
			vars, '-D' .. k .. '=' .. v
		)
	end
	table.sort(vars)
	local command = table.extend(
		{self.cmake, self:step_directory('source')},
		vars
	)
	return Project.configure(
		self,
		{
			command = command,
			working_directory = self:step_directory('build'),
			env = { CC = self.compiler.binary_path },
		}
	)
end

function CMakeProject:build(args)
	return Project.build(
		self,
		{
			command = {self.cmake, '--build', '.'},
			working_directory = self:step_directory('build'),
			env = { MAKEFLAGS = '', },
		}
	)
end

function CMakeProject:install(args)
	return Project.install(
		self,
		{
			command = {self.cmake, '--build', '.', '--target', 'install'},
			working_directory = self:step_directory('build'),
			env = { MAKEFLAGS = '', },
		}
	)
end

return CMakeProject

