--- C compilers
local M = {
	gcc = require("configure.lang.c.compiler.gcc"),
	clang = require("configure.lang.c.compiler.clang"),
	msvc = require("configure.lang.c.compiler.msvc"),
}

M.env_name = "CC"

M.description = "C compiler"

M.compilers = {
	M.gcc,
	M.clang,
	M.msvc,
}

--- Find a C compiler.
--
-- Check first if the environment variable has been set to a compiler, or
-- iterate through known compiler and return the first one available.
--
-- @param args Compiler options
-- @param args.build The current build instance
-- @param args.env_name The env variable to look (defaults to "CC")
function M.find(args)
	return require('configure.lang.compiler').find(M, args)
end

return M
