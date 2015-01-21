--- Clang compiler implementation (C specialization)
-- @classmod configure.lang.c.compiler.clang
local Compiler = table.update(
	{},
	require('configure.lang.c.compiler.gcc')
)
Compiler.name = 'clang'
Compiler.binary_names = {'clang', }

return Compiler
