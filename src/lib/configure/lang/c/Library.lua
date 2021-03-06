--- Define library base class.
-- @classmod configure.lang.c.Library
return {
	--- Create a new library instance
	--
	-- @param self
	-- @param args
	-- @param args.name The name of the library
	-- @param args.system Whether or not the library is available at system level
	-- @param args.include_directories Include directories
	-- @param args.files list of Node or path to files to link against
	-- @param args.runtime_files list of Node or path to files that are needed at runtime
	-- @param args.install_node a target `Node` to depends on when the library part of the build
	-- @param args.defines List of defines
	-- @param args.bundle A table of extra information (version, executable, ...)
	-- @param args.dependencies A table of libraries implicitly needed
	new = function(self, args)
		assert(args.name)
		local o = {}
		o.name = args.name
		o.system = args.system or false
		o.include_directories = args.include_directories or {}
		o.files = args.files or {}
		o.runtime_files = args.runtime_files or {}
		o.directories = {}
		for _, f in ipairs(o.files) do
			local path = f
			if getmetatable(f) == Node then
				path = f:path()
			end
			table.append(o.directories, path:parent_path())
		end
		o.defines = args.defines or {}
		o.dependencies = args.dependencies or {}
		o.kind = args.kind
		o.bundle = args.bundle or {}
		o.install_node = args.install_node or nil
		assert(o.kind == 'static' or o.kind == 'shared')
		setmetatable(o, self)
		self.__index = self
		return o
	end,
}
