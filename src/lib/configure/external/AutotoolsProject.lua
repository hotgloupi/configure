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
end

function AutotoolsProject:configure(args)
	return Project.configure(
		self,
		{
			command = {
				self:step_directory('source') / 'configure',
				'--prefix', tostring(self:step_directory('install'))
			},
			working_directory = self:step_directory('build'),
			env = { CC = self.compiler.binary_path },
			sources = args.sources,
		}
	)
end

function AutotoolsProject:build(args)
	return Project.build(
		self,
		{
			command = {self.make},
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
