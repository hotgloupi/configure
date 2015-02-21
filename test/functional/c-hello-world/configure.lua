require "package"

local c = require('configure.lang.c')

function configure(build)
	local compiler = c.compiler.find{build = build}
	local exe = compiler:link_executable{
		name = "hello",
		sources = {'main.c', },
	}
	build:add_rule(
		Rule:new()
			:add_target(build:virtual_node("check"))
			:add_shell_command(ShellCommand:new(exe))
			:add_source(exe)
	)
end
