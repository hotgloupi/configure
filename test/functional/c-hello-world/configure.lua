require "package"

local c = require('configure.lang.c')

function configure(build)
	local compiler = c.compiler.find{build = build}
	compiler:link_executable{
		name = "hello",
		sources = {'main.c', },
	}
end
