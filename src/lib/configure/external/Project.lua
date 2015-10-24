--- @classmod configure.external.Project
local Project = {}

--- Create a new Project
-- @param args
-- @param args.build The build instance
-- @param args.name Name of the project to add
-- @param[opt] args.root_directory Where to place all project files
function Project:new(o)
	assert(o ~= nil)
	assert(o.build ~= nil)
	o._build = o.build
	o.build = nil
	assert(o.name ~= nil)
	setmetatable(o, self)
	self.__index = self
	o:_init()
	return o
end

function Project:_init()
	if self.root_directory == nil then
		self.root_directory = self._build:directory() / self.name
	end
	if self.extract_directory == nil then
		self.extract_directory = self:step_directory('source')
	end
	if self.stamps_directory == nil then
		self.stamps_directory = self.root_directory / 'stamps'
	end
	self.steps = {}
	self.files = {}
	self._build:debug("Add project", self.name, "in", self.root_directory)
end


--- Checkout sources from git
--
-- @param args
-- @param args.url
-- @param args.tag
function Project:git_checkout(args)
	return self:add_step{
		name = 'download',
	}
end

--- Download a tarball
--
-- @param args
-- @param args.url The download link
-- @param args.filename The filename if it cannot be infered from the url.
-- @param args.method Method used to retreive the sources (defaults to 'fetch')
function Project:download(args)
	local args = table.update(
		{method = 'fetch'},
		args
	)
	local callbacks = {
		fetch = self._download_tarball,
	}
	local cb = callbacks[args.method]
	if cb == nil then
		self.build:error("The method " .. tostring(args.method) .. " does not exist")
	end
	return cb(self, args)
end

function Project:_download_tarball(args)
	local filename = args.filename
	if filename == nil then
		local index = args.url:find("/[^/]*$")
		filename = args.url:sub(index + 1, -1)
	end
	local download_dir = self:step_directory('download')
	local tarball = download_dir / filename
	return self:add_step{
		name = 'download',
		targets = {
			[0] = {
				{self._build:configure_program(), '-E', 'fetch', args.url, tarball},
				{self._build:configure_program(), '-E', 'extract', tarball, self:step_directory('extract')}
			}
		},
		sources = args.sources,
	}
end

function Project:configure(args)
	return self:add_step{
		name = 'configure',
		targets = {
			[0] = {args.command},
		},
		working_directory = args.working_directory,
		env = args.env,
		sources = args.sources,
	}
end

function Project:build(args)
	return self:add_step{
		name = 'build',
		targets = {
			[0] = {args.command},
		},
		working_directory = args.working_directory,
		env = args.env,
		sources = args.sources,
	}
end

function Project:install(args)
	return self:add_step{
		name = 'install',
		targets = {
			[0] = {args.command}
		},
		working_directory = args.working_directory,
		env = args.env,
		sources = args.sources,
	}
end

function Project:stamp_node(name)
	return self._build:target_node(self.root_directory / 'stamps' / name)
end

--- Add a step to the project
--
-- @param args
-- @string args.name Step name
-- @param args.targets A table of commands. Each key is either a `Path`
-- to the target file or `0` if no specific file is required. The value is a
-- list of commands to be triggered.
-- @param[opt] args.working_directory Where to trigger the commands
-- @param[opt] args.env Environ to use for the command
-- @param[opt] args.directory The step directory
-- @param[opt] args.sources Dependency nodes
function Project:add_step(args)
	local name = args.name
	local directory = args.directory or self:step_directory(name)
	local stamp = self:stamp_node(name)

	local stamped_rule = Rule:new():add_target(stamp)

	for _, source in ipairs(args.sources or {}) do
		stamped_rule:add_source(source)
	end

	local previous = table.update({}, args.dependencies or {})
	if #previous == 0 then
		if #self.steps > 0 then
			table.append(previous, self.steps[#self.steps])
		end
	end
	for _, p in ipairs(previous) do
		stamped_rule:add_source(p)
	end
	for target, commands in pairs(args.targets or {}) do
		local rule = stamped_rule
		if target ~= 0 then
			rule = Rule:new():add_target(self._build:target_node(target))
			for _, p in ipairs(previous) do
				rule:add_source(p)
				print(target, 'depends on', p)
			end
			stamped_rule:add_source(self._build:target_node(target))
		end
		for _, command in ipairs(commands) do
			local cmd = ShellCommand:new(table.unpack(command))
			if args.working_directory ~= nil then
				cmd:working_directory(args.working_directory)
			end
			if args.env ~= nil then
				cmd:env(args.env)
			end
			rule:add_shell_command(cmd)
		end

		if rule ~= stamped_rule then self._build:add_rule(rule) end
	end

	stamped_rule:add_shell_command(
		ShellCommand:new(self._build:configure_program(), '-E', 'touch', stamp)
	)
	table.append(self.steps, stamp)
	self._build:add_rule(stamped_rule)
	return self
end

function Project:last_step()
	if #self.steps > 0 then
		return self.steps[#self.steps]
	end
end

function Project:step_directory(name)
	local attr = '_internal_' .. name .. '_directory'
	if self[attr] == nil then
		local dir = self[name .. '_directory']
		if dir ~= nil then
			if type(dir) == 'string' then
				dir = Path:new(dir)
			elseif getmetatable(dir) == Node then
				dir = dir:path()
			elseif getmetatable(dir) ~= Path then
				self._build:error("Attribute '" .. name .. "_directory' is not a Path, a Node or a string")
			end
			if not dir:is_absolute() then
				dir = self.root_directory / dir
			end
		else
			dir = self.root_directory / name
		end
		self[attr] = dir
		self._build:directory_node(dir) -- Add the directory node to the build
	end
	return self[attr]
end

--- Node file created in a step.
--
-- @param args
-- @param args.path The relative path to the built file
-- @param[opt] args.step The step where the file should be located (defaults to 'install')
-- @param[opt] args.is_directory True when the node is a directory node (defaults to `false`)
function Project:node(args)
	local step = args.step or 'install'
	local path = self:step_directory(step) / args.path
	local node = self.files[tostring(path)]
	if node == nil then
		if args.is_directory == true then
			node = self._build:directory_node(path)
		else
			node = self._build:target_node(path)
			self._build:add_rule(
				Rule:new():add_source(self:stamp_node(step)):add_target(node)
			)
		end
		self.files[tostring(path)] = node
	end
	return node
end


function Project:directory_node(args)
	return self:node(table.update({is_directory = true}, args))
end

return Project
