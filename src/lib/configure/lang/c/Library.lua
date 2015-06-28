--- Define library base class.
-- @classmod configure.lang.c.Library
return {
	--- Create a new library instance
	--
	-- @param self
	-- @param args
	-- @param args.name The name of the library
	-- @param[opt] args.system Whether or not the library is available at system level
	-- @param[opt] args.include_directories Include directories
	-- @param[opt] args.files list of Node or path to files to link against
	-- @param[opt] args.runtime_files list of Node or path to files that are needed at runtime
	-- @param[opt] args.defines List of defines
	new = function(self, args)
		assert(args.name)
		local o = {}
		o.name = args.name
		o.system = args.system or false
		o.include_directories = args.include_directories or {}
		o.files = args.files or {}
		o.runtime_files = args.runtime_files or {}
		o.defines = args.defines or {}
		o.kind = args.kind
		assert(o.kind == nil or o.kind == 'static' or o.kind == 'shared')
		setmetatable(o, self)
		self.__index = self
		return o
	end,
}
