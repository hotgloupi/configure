--- C++ language tools
-- @module configure.lang.cxx

return {
	--- Compilers
	compiler = require "configure.lang.cxx.compiler",

	--- @{configure.lang.cxx.Library}
	Library = require("configure.lang.cxx.Library"),
}
