--- Expose C language facilities.
return {
	--- Compilers
	compiler = require "configure.lang.c.compiler",
	--- ${configure.lang.c.Library}
	Library = require "configure.lang.c.Library",
}
