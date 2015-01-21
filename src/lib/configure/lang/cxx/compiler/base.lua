--- Basic C++ compiler
-- @classmod configure.lang.cxx.compiler.base
local Compiler = table.update(
	{},
	require('configure.lang.c.compiler.base')
)

Compiler.exception = true

return Compiler
