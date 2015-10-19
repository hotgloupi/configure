--- @classmod configure.external.AutotoolsProject
local Project = require('configure.external.Project')
local AutotoolsProject = table.update({}, Project)

function AutotoolsProject:new(o)
	assert(o.compiler ~= nil)
	return Project.new(self, o)
end

function AutotoolsProject:_init()
	Project._init(self)
	self.make = self._build:fs():which('make')
	if self.make == nil then error("Make is not available") end
end

function AutotoolsProject:configure(args)
	local command = {
		self:step_directory('source') / 'configure',
		'--prefix', tostring(self:step_directory('install'))
	}
	table.extend(command, args.args or {})
	return Project.configure(
		self,
		{
			command = command,
			working_directory = self:step_directory('build'),
			env = table.update({ CC = self.compiler.binary_path }, args.env or {}),
			sources = args.sources,
		}
	)
end

function AutotoolsProject:build(args)
	local command = table.extend({self.make}, args.args or {})
	return Project.build(
		self,
		{
			command = command,
			working_directory = self:step_directory('build'),
			env = { MAKEFLAGS = '', },
			sources = args.sources,
		}
	)
end

function AutotoolsProject:install(args)
	return Project.install(
		self,
		{
			command = {self.make, 'install'},
			working_directory = self:step_directory('build'),
			env = { MAKEFLAGS = '', },
			sources = args.sources,
		}
	)
end

return AutotoolsProject
