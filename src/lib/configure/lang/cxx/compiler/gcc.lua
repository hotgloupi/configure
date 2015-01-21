--- GCC compiler implementation (C++ specialization)
-- @classmod configure.lang.cxx.compiler.gcc
local Compiler = table.update(
	table.update({}, require('configure.lang.cxx.compiler.base')),
	require('configure.lang.c.compiler.gcc')
)
Compiler.binary_names = {'g++', }
Compiler.lang = 'c++'

return Compiler
