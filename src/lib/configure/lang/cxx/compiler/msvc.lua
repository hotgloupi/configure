--- MSVC compiler implementation (C++ specialization)
-- @classmod configure.lang.cxx.compiler.gcc
local Compiler = table.update(
	table.update({}, require('configure.lang.cxx.compiler.base')),
	require('configure.lang.c.compiler.msvc')
)
Compiler.lang = 'c++'
Compiler._language_flag = '-TP'

return Compiler
