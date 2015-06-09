@c
Feature: C executable

	Scenario: Hello world
		Given a project configuration
		"""
		local c = require('configure.lang.c')

		function configure(build)
			local compiler = c.compiler.find{build = build}
			local exe = compiler:link_executable{
				name = "hello-world",
				sources = {'main.c', },
			}
		end
		"""
		And a source file main.c
		"""
		#include <stdio.h>
		int main() { printf("Hello, world!\n"); return 0; }
		"""
		When I configure and build
		Then I can launch bin/hello-world

	Scenario: Static runtime
		Given a project configuration
		"""
		local c = require('configure.lang.c')

		function configure(build)
			local compiler = c.compiler.find{build = build}
			local exe = compiler:link_executable{
				name = "hello-world",
				sources = {'main.c', },
				runtime = 'static',
				extension = '.exe'
			}
		end
		"""
		And a source file main.c
		"""
		#include <stdio.h>
		int main() { printf("Hello, world!\n"); return 0; }
		"""
		When I configure and build
		Then I can launch bin/hello-world.exe
		And build/bin/hello-world.exe is a static executable
