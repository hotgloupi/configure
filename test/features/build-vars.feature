Feature: Configure build variables

	Scenario: Build variable
		Given a project configuration
		"""
		return function(build)
			assert(
				build:string_option("test-var", "testing") == "LOL"
			)
		end
		"""
		When I configure with build test_var=LOL
		Then build variable TEST_VAR in build equals "LOL"

	Scenario: Set variable to multiple builds
		Given a project configuration
		"""
		return function(build)
			assert(
				build:string_option("test-var", "testing") == "LOL"
			)
		end
		"""
		When I configure with test_var=LOL build1 build2
		Then build variable TEST_VAR in build1 equals "LOL"
		And build variable TEST_VAR in build2 equals "LOL"

	Scenario: Command line variable after the build
		Given a project configuration
		"""
		return function(build)
			assert(
				build:string_option("test-var", "testing") == "LOL"
			)
		end
		"""
		When I configure with build test_var=LOL
		Then build variable TEST_VAR in build equals "LOL"

	Scenario Outline: Boolean variable
		Given a project configuration
		"""
		return function(build)
			assert(
				build:bool_option("test-var", "testing") == <lua_value>
			)
		end
		"""
		When I configure with build test_var=<command_line_value>
		Then build variable TEST_VAR in build equals "<printed_value>"

		Examples:
			| command_line_value | lua_value | printed_value |
			| 0                  | false     | 0             |
			| NO                 | false     | 0             |
			| no                 | false     | 0             |
			| false              | false     | 0             |
			| False              | false     | 0             |
			| 1                  | true      | 1             |
			| YES                | true      | 1             |
			| yes                | true      | 1             |
			| true               | true      | 1             |
			| True               | true      | 1             |


