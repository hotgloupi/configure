@cxx
Feature: C++ executable

	Scenario Outline: Hello world
		Given a system executable <compiler>
		And a project configuration
		"""
		local cxx = require('configure.lang.cxx')

		function configure(build)
			local compiler = cxx.compiler.find{build = build}
			local exe = compiler:link_executable{
				name = "hello-world",
				sources = {'main.cpp', },
			}
		end
		"""
		And a source file main.cpp
		"""
		#include <iostream>
		int main() { std::cout << "Hello, world!\n"; return 0; }
		"""
		When I configure and build
		Then I can launch bin/hello-world

		Examples: C++ compilers
			| compiler |
			| g++-4.4 |
			| g++-4.5 |
			| g++-4.6 |
			| g++-4.7 |
			| g++-4.8 |
			| clang++ |
			| cl.exe |
