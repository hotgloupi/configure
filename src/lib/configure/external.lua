--- @module configure.external

local M = {}

M.Project = {}

--- Create a new Project
-- @param args
-- @param args.build The build instance
-- @param args.name Name of the project to add
-- @param[opt] args.root_directory Where to place all project files
function M.Project:new(o)
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

function M.Project:_init()
	if self.root_directory == nil then
		self.root_directory = self._build:directory() / self.name
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
function M.Project:git_checkout(args)
	return self:add_step{
		name = 'download',
	}
end

--- Download a tarball
--
-- @param args
-- @param args.url The download link
-- @param args.filename The filename if it cannot be infered from the url.
function M.Project:download_tarball(args)
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
				{self._build:configure_program(), '-E', 'extract', tarball, self:step_directory('source')}
			}
		}
	}
end

function M.Project:configure(args)
	return self:add_step{
		name = 'configure',
		targets = {
			[0] = {args.command},
		},
		working_directory = args.working_directory,
		env = args.env,
	}
end

function M.Project:build(args)
	return self:add_step{
		name = 'build',
		targets = {
			[0] = {args.command},
		}
	}
end

function M.Project:install(args)
	return self:add_step{
		name = 'install',
		targets = {
			[0] = {args.command}
		}
	}
end

function M.Project:_stamp(name)
	return self._build:target_node(self.root_directory / 'stamps' / name)
end

function M.Project:add_step(args)
	local name = args.name
	local directory = self:step_directory(name)
	local stamp = self:_stamp(name)

	local stamped_rule = Rule:new():add_target(stamp)

	for _, target in ipairs(args.targets or {}) do
		stamped_rule:add_target(self._build:target_node(target))
	end

	for target, commands in pairs(args.targets) do
		local rule = stamped_rule
		if target ~= 0 then
			rule = Rule:new():add_target(self._build:target_node(target))
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
	local previous = table.update({}, args.dependencies or {})
	if #previous == 0 then
		if #self.steps > 0 then
			table.append(previous, self.steps[#self.steps])
		end
	end
	for _, p in ipairs(previous) do
		stamped_rule:add_source(p)
	end
	table.append(self.steps, stamp)
	self._build:add_rule(stamped_rule)
	return self
end

function M.Project:step_directory(name)
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
function M.Project:node(args)
	local step = args.step or 'install'
	local path = self:step_directory(step) / args.path
	local node = self.files[tostring(path)]
	if node == nil then
		if args.is_directory == true then
			node = self._build:directory_node(path)
		else
			node = self._build:target_node(path)
			self._build:add_rule(
				Rule:new():add_source(self:_stamp(step)):add_target(node)
			)
		end
		self.files[tostring(path)] = node
	end
	return node
end


function M.Project:directory_node(args)
	return self:node(table.update({is_directory = true}, args))
end

return M
