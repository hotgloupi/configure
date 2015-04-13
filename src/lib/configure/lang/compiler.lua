--- Generic compiler tools.
-- @module configure.lang.compiler
local M = {}

--- Find a compiler using the environment and a compiler list.
--
-- This function should not be used directly, instead use specialized function
-- for the targetted language (configure.lang.LANG.find()).
--
-- @param module The compiler module
-- @param module.env_name The environment variable name used to store the compiler.
-- @param module.description Compiler kind description.
-- @param module.compilers A compiler list to check.
-- @param args Compiler arguments
-- @param args.build The build instance.
function M.find(module, args)
	local env_name = args.env_name or module.env_name
	args.env_name = env_name
	local binary_name = args.build:path_option(env_name, module.description)
	local compiler = nil
	if binary_name then
		local binary_path = args.build:fs():which(binary_name)
		if not binary_path then
			error("Cannot find a valid executable for " .. tostring(binary_name))
		end
		for _, cls in ipairs(module.compilers) do
			if cls:usable_with_binary(args.build, binary_path) then
				args.binary_path = binary_path
				compiler = cls:new(args)
				break
			end
		end
	else
		compiler = M.select_first_compiler(module.compilers, args)
	end
	if compiler then
		args.build:env():set(env_name, compiler.binary_path)
	else
		error("Couldn't find any compiler")
	end
	return compiler
end

--- Utility function that iter through a list of compiler and returns
-- the first available.
--
-- @param compilers The compiler list to iter through
-- @param args Arguments forwarded to the compiler constructor
function M.select_first_compiler(compilers, args)
	for _, cls in ipairs(compilers) do
		args.build:debug("Checking if compiler", cls.name, "is available")
		for _, binary_name in ipairs(cls.binary_names) do
			local path = args.build:fs():which(binary_name)
			if path then
				args.build:status("Found", cls.lang:upper(), "compiler", cls.name, "at", path)
				args.binary_path = path
				return cls:new(args)
			end
		end
	end
end

return M
