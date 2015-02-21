
c = require('configure.lang.c')

function configure(build)
	local compiler = c.compiler.find{build = build}
	libtest = compiler:link_shared_library{
		name = 'test',
		sources = {'libtest.c'}
	}
	exe = compiler:link_executable{
		name = 'test',
		sources = {'main.c'},
		libraries = {libtest,}
	}
	build:add_rule(
		Rule:new()
			:add_target(build:virtual_node("check"))
			:add_shell_command(ShellCommand:new(exe))
			:add_source(exe)
	)
end
