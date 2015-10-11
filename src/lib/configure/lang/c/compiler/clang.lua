--- Clang compiler implementation (C specialization)
-- @classmod configure.lang.c.compiler.clang
local Compiler = table.update(
	{},
	require('configure.lang.c.compiler.gcc')
)
Compiler.name = 'clang'
Compiler.binary_names = {'clang', }

function Compiler:_gen_rpath_flags(dirs)
	local res = {}
	local origin = self.build:target():is_osx() and '@loader_path' or '$ORIGIN'
	for _, dir in ipairs(dirs) do
		table.append(res, '-Wl,-rpath,' .. tostring(origin / dir))
	end
	return res
end

function Compiler:_add_unresolved_symbols_policy_flag(cmd, args)
	if args.allow_unresolved_symbols then
		table.append(cmd, '-Wl,-undefined,dynamic_lookup')
	end
end

return Compiler
