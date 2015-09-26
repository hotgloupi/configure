Feature: Launch lua scripts

	Scenario: Empty arguments
		Given a temporary directory
		And a source file test.lua
		"""
		function test()
			print("TEST")
		end
		"""
		When I configure with -E lua-function test.lua test
		Then the stripped command output is "TEST"

	Scenario: With arguments
		Given a temporary directory
		And a source file test.lua
		"""
		function test(arg1, arg2, arg3)
			print("TEST", arg1, arg2, arg3)
		end
		"""
		When I configure with -E lua-function test.lua test pif paf
		Then the stripped command output is "TEST pif paf nil"
