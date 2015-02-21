

function configure(build)
	build:add_rule(
		Rule:new()
			:add_target(build:virtual_node("check"))
			:add_shell_command(ShellCommand:new("echo", "empty"))
	)
end
