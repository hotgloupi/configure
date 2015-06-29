@c
Feature: Header dependencies are honored

	Scenario: Simple macro change
		Given a source file test.c
		"""
		#include "test.h"
		int main()
		{ return (ANSWER == 42 ? 0 : 1); }
		"""
		And a source file test.h
		"""
		#define ANSWER 32
		"""
		And a project configuration
		"""
		local c = require('configure.lang.c')

		return function(build)
			local compiler = c.compiler.find{build = build}
			local exe = compiler:link_executable{
				name = "test",
				sources = {'test.c', },
			}
		end
		"""
		When I configure and build
		And a source file test.h
		"""
		#define ANSWER 42
		"""
		And I build everything
		Then I can launch bin/test

	Scenario: Indirect include
		Given a source file test.c
		"""
		#include "test.h"
		int main()
		{ return (ANSWER == 42 ? 0 : 1); }
		"""
		And a source file test.h
		"""
		#include "answer.h"
		"""
		And a source file answer.h
		"""
		#define ANSWER 32
		"""
		And a project configuration
		"""
		local c = require('configure.lang.c')

		return function(build)
			local compiler = c.compiler.find{build = build}
			local exe = compiler:link_executable{
				name = "test",
				sources = {'test.c', },
			}
		end
		"""
		When I configure and build
		And a source file answer.h
		"""
		#define ANSWER 42
		"""
		And I build everything
		Then I can launch bin/test
