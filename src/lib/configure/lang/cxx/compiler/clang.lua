--- Clang compiler implementation (C specialization)
-- @classmod configure.lang.c.compiler.clang
local Compiler = table.update(
	table.update({}, require('configure.lang.cxx.compiler.base')),
	require('configure.lang.c.compiler.clang')
)
Compiler.binary_names = {'clang++',}
Compiler.lang = 'c++'
Compiler.standard_library = 'libc++'

return Compiler
